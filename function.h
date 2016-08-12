#pragma once

#include <vector>
#include <string>
#include <cmath>

class Function
{
public:
    virtual ~Function() {}
    virtual result_t eval(result_t) = 0;

    static Function * find(const char* str, size_t &len);
};

#define DECLARE_FUNC_REGISTRY \
    std::vector<std::pair<std::string, Function *> > \
    __func_registry

extern DECLARE_FUNC_REGISTRY;
#ifdef FUNCTION_DECLARATIONS
DECLARE_FUNC_REGISTRY;
#define ENABLE_FUNC(FUNC) \
FUNC __func_ ## FUNC
#else
#define ENABLE_FUNC(FUNC)
#endif

inline
Function * Function::find(const char* str, size_t &len)
{
    for(auto it = __func_registry.begin(); it != __func_registry.end(); ++it) {
        std::string &name = it->first;
        if (0 == name.compare(0, string::npos, str, name.size())) {
            len = name.size();
            return it->second;
        }
    }
    return nullptr;
}


template<class Impl>
class Registrar : public Function
{
public:
    Registrar()
    {
        __func_registry.push_back(
            make_pair(
                string(Impl::name),
                static_cast<Function *>(this)));
    }
};

class SQRT : Registrar<SQRT>
{
public:
    static constexpr const char * name = "sqrt";
    result_t eval(result_t opnd) override final
    {
        return sqrt(opnd);
    }
};

ENABLE_FUNC(SQRT);