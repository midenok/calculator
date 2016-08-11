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
    Node_p parent;
    Node_p left;
    Node_p right;

    Node(Token t) :
        token {t} {}
};

typedef Node::Node_p Node_p;

class Calc
{
    Node_p root;
    Node_p start;

    struct StackItem
    {
        Node_p root;
        Node_p start;
        result_t result = 0;

        StackItem(Node_p _root, Node_p _start) :
            root { _root },
            start { _start } {}

        StackItem(Node_p _root, result_t _result) :
            root { _root },
            result { _result } {}

        ~StackItem() {}
    };
    vector<StackItem> stack;

public:
    void parse(Tokenizer &formula)
    {
        while (Token t = formula.next_token()) {
            if (t == OPEN_PAREN) {
                stack.push_back(StackItem(root, start));
                root = nullptr;
                continue;
            }
            if (t == CLOSE_PAREN) {
                if (stack.empty())
                    formula.throw_error("Missed '('");
                Node_p old_root = stack.back().root;
                Node_p old_start = stack.back().start;
                stack.pop_back();
                if (root->token == OPERATOR)
                    root->token.op.prioritize = true;
                if (!old_root)
                    continue;
                assert(!old_root->right);
                old_root->right = start;
                start->left = root; // quick access to sub-root
                root = old_root;
                start = old_start;
                continue;
            }
            if (t == NUMBER) {
                if (root) {
                    assert(!root->right);
                    root->right = make_shared<Node>(t);
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
                    Node_p rght = root->right;
                    assert(rght);
                    if (rght->parent) {
                        Node_p subr = rght->left; // sub-root
                        assert(subr);
                        node->left = subr;
                        subr->parent = node;
                        rght->left = node; // new sub-root
                    } else
                        rght->parent = node;
                } else {
                    root->parent = node;
                    node->left = root;
                }
                root = node;
            }
        }
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

    result_t operator() (Tokenizer formula)
    {
        root = nullptr;
        start = nullptr;
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