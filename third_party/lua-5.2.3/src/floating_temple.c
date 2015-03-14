/*
** Hook functions for integration with the Floating Temple distributed
** interpreter
*/


#define floating_temple_c
#define LUA_CORE

#include "lua.h"

#include "floating_temple.h"
#include "lobject.h"
#include "luaconf.h"


#define FT_DEFINE_HOOK_FUNC(install_func, hook_type, hook_var, hook_params, \
                            default_hook_func) \
  static int default_hook_func hook_params { \
    return 0; \
  } \
\
  LUAI_DDEF hook_type hook_var = &default_hook_func; \
\
  LUA_API hook_type install_func (hook_type hook) { \
    hook_type old_hook = hook_var; \
    hook_var = hook; \
    return old_hook; \
  }


FT_DEFINE_HOOK_FUNC(ft_installnewtablehook, ft_NewTableHook, ft_newtablehook,
                    (lua_State *L, StkId obj, int b, int c),
                    ft_defaultnewtablehook)


#undef FT_DEFINE_HOOK_FUNC
