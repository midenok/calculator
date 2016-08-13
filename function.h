#pragma once

#include <vector>
#include <string>
#include <cmath>

class Function
{
public:
    virtual ~Function() {}
    virtual result_t eval(result_t) = 0;
    virtual const char * name() const = 0;
};

class Command
{
public:
    virtual ~Command() {}
    virtual void parse_finished(void *) = 0;
    virtual const char * name() const = 0;
};

typedef std::vector<std::pair<std::string, Function *> > FuncRegistry;
typedef std::vector<std::pair<std::string, Command *> > CmdRegistry;

extern FuncRegistry __func_registry;
extern CmdRegistry __cmd_registry;

#ifdef FUNCTION_DECLARATIONS
FuncRegistry __func_registry;
CmdRegistry __cmd_registry;
#define ENABLE(NAME) \
NAME __ ## NAME
#else
#define ENABLE(NAME)
#endif



template<class Kind, class Impl, class Registry, Registry * registry>
class Registrar : public Kind
{
public:
    Registrar()
    {
        registry->push_back(
            make_pair(
                string(Impl::name_),
                static_cast<Kind *>(this)));
    }
};

template<class Kind, class Registry, Registry * registry>
class Finder
{
public:
    static
    Kind * find (const char* str, size_t &len)
    {
        for (auto it = registry->begin(); it != registry->end(); ++it) {
            std::string &name = it->first;
            if (0 == name.compare(0, string::npos, str, name.size())) {
                len = name.size();
                return it->second;
            }
        }
        return nullptr;
    }
};

template <class Impl>
class FuncRegistrar :
    public Registrar<Function, Impl, FuncRegistry, &__func_registry>
{};

template <class Impl>
class CmdRegistrar :
    public Registrar<Command, Impl, CmdRegistry, &__cmd_registry>
{};

class Functions :
    public Finder<Function, FuncRegistry, &__func_registry>
{};

class Commands :
    public Finder<Command, CmdRegistry, &__cmd_registry>
{};

#define DECLARE_NAME(NAME) \
const char * name() const override { return name_; } \
static constexpr const char * name_ = #NAME

#define MATH_FUNCTION(NAME) \
class Math_ ##NAME : FuncRegistrar<Math_ ##NAME> \
{ \
public: \
    DECLARE_NAME(NAME); \
    result_t eval(result_t opnd) override final \
    { \
        return NAME(opnd); \
    } \
}; \
ENABLE(Math_ ##NAME)

class Dump : public CmdRegistrar<Dump>
{
public:
    DECLARE_NAME(dump);

    void parse_finished(void * opaque) override final;
};
ENABLE(Dump);

MATH_FUNCTION(acos);
MATH_FUNCTION(asin);
MATH_FUNCTION(atan);
MATH_FUNCTION(ceil);
MATH_FUNCTION(cosh);
MATH_FUNCTION(cos);
MATH_FUNCTION(exp2);
MATH_FUNCTION(expm1);
MATH_FUNCTION(exp);
MATH_FUNCTION(abs);
MATH_FUNCTION(floor);
MATH_FUNCTION(log10);
MATH_FUNCTION(log1p);
MATH_FUNCTION(log2);
MATH_FUNCTION(logb);
MATH_FUNCTION(log);
MATH_FUNCTION(sinh);
MATH_FUNCTION(sin);
MATH_FUNCTION(sqrt);
MATH_FUNCTION(tanh);
MATH_FUNCTION(tan);
MATH_FUNCTION(cbrt);
MATH_FUNCTION(erf);
MATH_FUNCTION(erfc);
MATH_FUNCTION(tgamma);
MATH_FUNCTION(lgamma);
MATH_FUNCTION(trunc);
MATH_FUNCTION(round);
MATH_FUNCTION(nearbyint);

#undef MATH_FUNCTION
#undef ENABLE
#undef DECLARE_NAME
#undef FUNCTION_DECLARATIONS
