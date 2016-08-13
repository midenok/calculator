#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <memory>
#include <cassert>
#include <vector>

using namespace std;

enum TokenType
{
    // order matters (see 'expected')
    END,
    COMMAND,
    OPEN_PAREN,
    NUMBER,
    FUNCTION,
    OPERATOR,
    CLOSE_PAREN
};

typedef long double result_t;
//typedef long result_t;

#define FUNCTION_DECLARATIONS
#include "function.h"

struct Token
{
    TokenType type;
    struct Operator
    {
        bool prioritize = false;
        char value;
        Operator(char _operat) :
            value { _operat } {}
    };
    union
    {
        result_t number;
        Operator op;
        Function *func;
        Command *cmd;
    };

    Token(TokenType _type) :
        type { _type } {}

    Token(result_t _num) :
        type { NUMBER },
        number { _num } {}

    Token(char _operat) :
        type { OPERATOR },
        op { _operat } {}

    Token(Function *_func) :
        type { FUNCTION },
        func { _func } {}

    Token(Command *_cmd) :
        type { COMMAND },
        cmd { _cmd } {}

    bool operator== (TokenType _type) const
    {
        return type == _type;
    }
    bool operator!= (TokenType _type) const
    {
        return type != _type;
    }
    operator bool() const
    {
        return type != END;
    }
    bool operator> (Token &rhs) const
    {
        if (type != OPERATOR || rhs.type != OPERATOR)
            return false;
        if (rhs.op.prioritize && !op.prioritize)
            return false;
        if (op.prioritize && !rhs.op.prioritize)
            return true;
        if ((op.value == '*' || op.value == '/') && (rhs.op.value == '+' || rhs.op.value == '-'))  {
            return true;
        }
        return false;
    }
    operator result_t() const
    {
        assert(type == NUMBER);
        return number;
    }
    result_t operator() (result_t left, result_t right) const
    {
        assert(type == OPERATOR);
        switch (op.value) {
        case '+':
            return left + right;
        case '-':
            return left - right;
        case '*':
            return left * right;
        case '/':
            return left / right;
        default:;
        }
        throw runtime_error("Wrong operation!");
    }
    result_t operator() (result_t right) const
    {
        assert(type == FUNCTION);
        return func->eval(right);
    }
    friend ostream& operator<< (ostream& stream, const Token& self)
    {
        switch (self.type) {
        case NUMBER:
            stream << self.number;
            break;
        case OPERATOR:
            stream << ' ' << self.op.value << ' ';
            break;
        case FUNCTION:
            stream << self.func->name();
            break;
        default:;
        }
        return stream;
    }
};


class Tokenizer
{
    const char *formula;
    size_t idx = 0;
    TokenType expected = COMMAND;

public:
    Tokenizer(const char *_formula)
        : formula {_formula}
    {}

    Tokenizer(string &_formula)
        : formula { _formula.c_str() }
    {}

    class Error : public domain_error
    {
        size_t idx;
        string error;

    public:
        Error(const char *descr, size_t _idx) :
            domain_error(descr),
            idx{_idx}
        {}

        virtual const char* what()
        {
            ostringstream s;
            s << domain_error::what() << " at " << idx;
            error = s.str();
            return error.c_str();
        }
    };

    void throw_error(const char *descr) const
    {
        throw Error(descr, idx);
    }

    size_t get_number(long double &out, const char *in) const
    {
        int len = 0;;
        int n = sscanf(in, "%Lf%n", &out, &len);
        if (errno == ERANGE)
            throw Error("Number is too big", idx);

        if (n != 1)
            throw Error("Failed to parse number", idx);
        return len;
    }

    size_t get_number(long &out, const char *in) const
    {
        char *end;
        out = strtol(in, &end, 10);
        if (errno == ERANGE)
            throw Error("Number is too big", idx);
        return end - in;
    }

    Token next_token()
    {
        if (!formula[idx])
            return END;

        while (formula[idx] == ' ')
            if (!formula[++idx])
                return END;

        if (expected == COMMAND) {
            size_t len;
            Command *cmd = Commands::find(&formula[idx], len);
            if (cmd) {
                idx += len;
                expected = NUMBER;
                return cmd;
            }
        }

        char c = formula[idx];

        if (expected < OPERATOR) {
            if (c == '(') {
                ++idx;
                expected = NUMBER;
                return OPEN_PAREN;
            }
            if (expected == OPEN_PAREN)
                throw_error("'(' expected");

            if (c == '-' || c == '+' || isdigit(c)) {
                result_t number;
                idx += get_number(number, &formula[idx]);
                expected = OPERATOR;
                return number;
            }

            size_t len;
            Function *f = Functions::find(&formula[idx], len);
            if (f) {
                idx += len;
                expected = OPEN_PAREN;
                return f;
            }

            throw Error("Operand expected", idx);
        }

        expected = NUMBER;
        ++idx;

        switch (c) {
        case '+':
        case '-':
        case '*':
        case '/':
            return c;
        case ')':
            expected = OPERATOR;
            return CLOSE_PAREN;
        }
        throw Error("Operator expected", --idx);
    }

};

struct Node
{
    typedef shared_ptr<Node> Node_p;

    Token token;
    Node_p left;
    Node_p right;

    Node(Token t) :
        token {t} {}

    friend ostream& operator<< (ostream& stream, const Node& self)
    {
        stream << &self << ": " << self.token << endl;
        if (self.left)
            stream << "  left: " << self.left << " (" << self.left->token << ")" << endl;
        if (self.right)
            stream << "  right: " << self.right << " (" << self.right->token << ")" << endl;

        if (self.left)
            stream << *self.left;
        if (self.right)
            stream << *self.right;
        return stream;
    }
};

typedef Node::Node_p Node_p;

class Calc
{
    Node_p last;
    Node_p root;
    Node_p saved; // for returning to lower precedence

    struct StackItem
    {
        Node_p last;
        Node_p root;
        Node_p saved;
        result_t result = 0;

        StackItem(Node_p _last, Node_p _root, Node_p _saved) :
            last { _last },
            root { _root },
            saved { _saved } {}

        StackItem(Node_p _last, result_t _result) :
            last { _last },
            result { _result } {}

        ~StackItem() {}
    };
    vector<StackItem> stack;

    Command *command = nullptr;

public:
    void parse(Tokenizer formula)
    {
        while (Token t = formula.next_token()) {
            if (t == COMMAND) {
                command = t.cmd;
                continue;
            }
            if (t == OPEN_PAREN) {
                stack.push_back(StackItem(last, root, saved));
                last = nullptr;
                saved = nullptr;
                continue;
            }
            if (t == CLOSE_PAREN) {
                if (stack.empty())
                    formula.throw_error("Missed '('");

                Node_p old_last = stack.back().last;
                Node_p old_root = stack.back().root;
                Node_p old_saved = stack.back().saved;
                stack.pop_back();
                if (last->token == OPERATOR)
                    last->token.op.prioritize = true;
                if (!old_last)
                    continue;

                assert(!old_last->left);

                // put function into parens lowest priority branch
                if (old_last->right) {
                    Node_p lowest = saved ? saved : last;
                    assert(old_last->right->token == FUNCTION);
                    lowest->left = old_last->right;
                    root->right = old_last->right;
                    old_last->right = root;
                    last = old_saved ? old_saved : old_last;
                    root = old_root;
                    saved = nullptr;
                    continue;
                }
                if (old_last->token == FUNCTION) {
                    if (saved) {
                        saved->left = old_last;
                        saved = old_last;
                    } else {
                        last->left = old_last;
                        last = old_last;
                    }
                    continue;
                }

                assert(!old_last->right);
                old_last->right = root;
                last = old_last;
                root = old_root;
                saved = old_saved;
                continue;
            }
            if (t == NUMBER || t == FUNCTION) {
                if (last) {
                    assert(!last->right);
                    last->right = make_shared<Node>(t);
                    if (saved && t == NUMBER) {
                        last = saved;
                        saved = nullptr;
                    }
                } else {
                    last = make_shared<Node>(t);
                    root = last;
                }
                continue;
            }
            {   // t == OPERATOR
                assert(last);
                Node_p node = make_shared<Node>(t);

                if (t > last->token) {
                    // current operator takes precedence
                    assert(last->right);
                    if (last->right->right) {
                        assert(!last->right->right->left);
                        last->right->right->left = node;
                    } else {
                        // this branch should be deprecated
                        // since root->right should always point to last
                        assert(!last->right->left);
                        last->right->left = node;
                    }
                    last->right->right = node;
                    saved = last;
                } else {
                    last->left = node;
                    root->right = node;
                }
                last = node;
            }
        } // while

        if (command)
            command->parse_finished(this);

        if (!stack.empty())
            formula.throw_error("Missed ')'!");
    }

    result_t calculate()
    {
        if (!root)
            throw domain_error("Empty formula!");

        assert(stack.empty());

        result_t result = root->token;
        Node_p node = root->left;
        while (node || !stack.empty()) {
            if (!node) {
                node = stack.back().last;
                node->right->left = nullptr;
                node->right->token = result;
                result = stack.back().result;
                stack.pop_back();
            }
            if (node->token == FUNCTION) {
                assert(!node->right);
                cout << node->token << "(" << result << ")";
                result = node->token(result);
                cout << " = " << result << endl;
                node = node->left;
                continue;
            }
            assert(node->right);
            if (node->right->left) {
                stack.push_back(StackItem(node, result));
                result = node->right->token;
                node = node->right->left;
                continue;
            }
            cout << result << node->token << node->right->token;
            result = node->token(result, node->right->token);
            cout << " = " << result << endl;
            node = node->left;
        }
        return result;
    }

    void reset()
    {
        last = nullptr;
        root = nullptr;
        saved = nullptr;
        stack.clear();
    }

    result_t operator() (Tokenizer formula)
    {
        reset();
        parse(formula);
        result_t result = calculate();
        return result;
    }

    void dump() const
    {
        cout << *root << endl;
    }
};

void Dump::parse_finished(void *opaque)
{
    assert(opaque);
    static_cast<Calc *>(opaque)->dump();
}


int main()
{
    string line;
    Calc calc;

    while (!cin.eof()) {
        getline(cin, line);
        if (line.empty())
            continue;

        try {
            cout << line << endl;
            result_t result = calc(line);
            cout << "Result: " << result << endl << endl;
        } catch (Tokenizer::Error &e) {
            cerr << "Parse error:" << endl << e.what() << endl;
        } catch (domain_error &e) {
            cerr << e.what() << endl;
        }
    }
}
