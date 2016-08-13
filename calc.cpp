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
    END,
    NUMBER,
    OPERATOR,
    OPEN_PAREN,
    CLOSE_PAREN
};

typedef long double result_t;
//typedef long result_t;

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
    };

    Token(TokenType _type) :
        type { _type } {}

    Token(result_t _num) :
        type { NUMBER },
        number { _num } {}

    Token(char _operat) :
        type { OPERATOR },
        op { _operat }
    {}

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
    friend ostream& operator<< (ostream& stream, const Token& self)
    {
        switch (self.type) {
        case NUMBER:
            stream << self.number;
            break;
        case OPERATOR:
            stream << ' ' << self.op.value << ' ';
        default:;
        }
        return stream;
    }
};


class Tokenizer
{
    const char *formula;
    size_t idx = 0;
    bool number_expected = true;

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

        while (formula[idx] == ' ') {
            ++idx;
            if (!formula[idx])
                return END;
        }

        if (number_expected) {
            if (formula[idx] == '-' || formula[idx] == '+' || isdigit(formula[idx])) {
                result_t number;
                idx += get_number(number, &formula[idx]);
                number_expected = false;
                return number;
            }
            if (formula[idx] == '(') {
                ++idx;
                return OPEN_PAREN;
            }
            throw Error("Number expected", idx);
        }

        number_expected = true;

        switch (char c = formula[idx++]) {
        case '+':
        case '-':
        case '*':
        case '/':
            return c;
        case ')':
            number_expected = false;
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

public:
    void parse(Tokenizer &formula)
    {
        while (Token t = formula.next_token()) {
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
                saved = stack.back().saved;
                stack.pop_back();
                if (last->token == OPERATOR)
                    last->token.op.prioritize = true;
                if (!old_last)
                    continue;
                assert(!old_last->right);
                old_last->right = root;
                last = old_last;
                root = old_root;
                continue;
            }
            if (t == NUMBER) {
                if (last) {
                    assert(!last->right);
                    last->right = make_shared<Node>(t);
                    if (saved) {
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
                        last->right->right->left = node;
                    } else {
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
        }
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
        cout << endl;
        return result;
    }

    result_t operator() (Tokenizer formula)
    {
        last = nullptr;
        root = nullptr;
        saved = nullptr;
        stack.clear();

        parse(formula);
        result_t result = calculate();
        return result;
    }
};

void test()
{
    Calc c;

    result_t res = c("(((100))) - 2 * ((3 + 1) + (2 + 16 / 4)) * (11 - 6 - 1000 * 0)");
    if (res != 0) {
        cerr << "Self-test failed!" << endl;
        exit(1);
    }
}

int main()
{
    test();
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