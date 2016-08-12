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

class SQRT : FuncRegistrar<SQRT>
{
public:
    DECLARE_NAME(sqrt);

    result_t eval(result_t opnd) override final
    {
        return sqrt(opnd);
    }
};
ENABLE(SQRT);

class Dump : public CmdRegistrar<Dump>
{
public:
    DECLARE_NAME(dump);

    void parse_finished(void * opaque) override final;
};
ENABLE(Dump);


#undef FUNCTION_DECLARATIONS
#undef ENABLE
#undef DECLARE_NAME

