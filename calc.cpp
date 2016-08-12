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
    {
    }

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
    Node_p parent;
    Node_p right;

    Node(Token t) :
        token {t} {}

    friend ostream& operator<< (ostream& stream, const Node& self)
    {
        stream << &self << ": " << self.token << endl;
        if (self.parent)
            stream << "  parent: " << self.parent << " (" << self.parent->token << ")" << endl;
        if (self.right)
            stream << "  right: " << self.right << " (" << self.right->token << ")" << endl;

        if (self.parent)
            stream << *self.parent;
        if (self.right)
            stream << *self.right;
        return stream;
    }
};

typedef Node::Node_p Node_p;

class Calc
{
    Node_p root;
    Node_p start;
    Node_p saved; // for returning to lower precedence

    struct StackItem
    {
        Node_p root;
        Node_p start;
        Node_p saved;
        result_t result = 0;

        StackItem(Node_p _root, Node_p _start, Node_p _saved) :
            root { _root },
            start { _start },
            saved { _saved } {}

        StackItem(Node_p _root, result_t _result) :
            root { _root },
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
                stack.push_back(StackItem(root, start, saved));
                root = nullptr;
                saved = nullptr;
                continue;
            }
            if (t == CLOSE_PAREN) {
                if (stack.empty())
                    formula.throw_error("Missed '('");

                Node_p old_root = stack.back().root;
                Node_p old_start = stack.back().start;
                Node_p old_saved = stack.back().saved;
                stack.pop_back();
                if (root->token == OPERATOR)
                    root->token.op.prioritize = true;
                if (!old_root)
                    continue;

                assert(!old_root->parent);

                // put function into parens lowest priority root
                if (old_root->right) {
                    Node_p lowest_root = saved ? saved : root;
                    assert(old_root->right->token == FUNCTION);
                    lowest_root->parent = old_root->right;
                    start->right = old_root->right;
                    old_root->right = start;
                    root = old_saved ? old_saved : old_root;
                    start = old_start;
                    saved = nullptr;
                    continue;
                }
                if (old_root->token == FUNCTION) {
                    if (saved) {
                        saved->parent = old_root;
                        saved = old_root;
                    } else {
                        root->parent = old_root;
                        root = old_root;
                    }
                    continue;
                }

                assert(!old_root->right);
                old_root->right = start;
                root = old_root;
                start = old_start;
                saved = old_saved;
                continue;
            }
            if (t == NUMBER || t == FUNCTION) {
                if (root) {
                    assert(!root->right);
                    root->right = make_shared<Node>(t);
                    if (saved && t == NUMBER) {
                        root = saved;
                        saved = nullptr;
                    }
                } else {
                    root = make_shared<Node>(t);
                    start = root;
                }
                continue;
            }
            {   // t == OPERATOR
                assert(root);
                Node_p node = make_shared<Node>(t);

                if (t > root->token) {
                    // current operator takes precedence
                    assert(root->right);
                    if (root->right->right) {
                        assert(!root->right->right->parent);
                        root->right->right->parent = node;
                    } else {
                        assert(!root->right->parent);
                        root->right->parent = node;
                    }
                    root->right->right = node;
                    saved = root;
                } else {
                    root->parent = node;
                }
                root = node;
            }
        } // while

        if (command)
            command->parse_finished(this);

        if (!stack.empty())
            formula.throw_error("Missed ')'!");
    }

    result_t calculate()
    {
        if (!start)
            throw domain_error("Empty formula!");

        assert(stack.empty());

        result_t result = start->token;
        Node_p node = start->parent;
        while (node || !stack.empty()) {
            if (!node) {
                node = stack.back().root;
                node->right->parent = nullptr;
                node->right->token = result;
                result = stack.back().result;
                stack.pop_back();
            }
            if (node->token == FUNCTION) {
                assert(!node->right);
                cout << node->token << "(" << result << ")";
                result = node->token(result);
                cout << " = " << result << endl;
                node = node->parent;
                continue;
            }
            assert(node->right);
            if (node->right->parent) {
                stack.push_back(StackItem(node, result));
                result = node->right->token;
                node = node->right->parent;
                continue;
            }
            cout << result << node->token << node->right->token;
            result = node->token(result, node->right->token);
            cout << " = " << result << endl;
            node = node->parent;
        }
        cout << endl;
        return result;
    }

    void reset()
    {
        root = nullptr;
        start = nullptr;
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
        cout << *start << endl;
    }
};

void Dump::parse_finished(void *opaque)
{
    assert(opaque);
    static_cast<Calc *>(opaque)->dump();
}


void test()
{
    Calc c;

    try {
        // result_t res = c("(((100))) - 2 * ((3 + 1) + (2 + 16 / 4)) * (11 - 6 - 1000 * 0)");
        // result_t res = c("1 + sqrt(10 - 2 * 3)");
        //c.parse("1 + 5* (2 - 3) * 4");
        //c.parse("1 + 2 * (3 + 4 + 5)");
        //1+2*sqrt(1+8)/(4 + (2 - 3))
        //  1 - 2 * sqrt(9) + 7
        c.parse("1 - 2 * sqrt(9) + 7");
        c.dump();
        result_t res = c.calculate();
        cout << "Result: " << res << endl;
        // 2 * (5 - 4 / 2 - 1)
        if (res != 0) {
            cerr << "Self-test failed!" << endl;
            exit(1);
        }
    } catch (Tokenizer::Error &e) {
        cerr << "Parse error:" << endl << e.what() << endl;
    } catch (domain_error &e) {
        cerr << e.what() << endl;
    }
    exit(0);
}

int main()
{
    //test();
    string line;
    Calc calc;

    while (!cin.eof()) {
        getline(cin, line);
        if (line.empty())
            continue;

        try {
            result_t result = calc(line);
            cout << "Result: " << result << endl;
        } catch (Tokenizer::Error &e) {
            cerr << "Parse error:" << endl << e.what() << endl;
        } catch (domain_error &e) {
            cerr << e.what() << endl;
        }
    }
}
