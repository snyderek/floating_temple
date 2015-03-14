/*
** Hook functions for integration with the Floating Temple distributed
** interpreter
*/


#include <stddef.h>


#define floating_temple_c
#define LUA_CORE

#include "lua.h"

#include "floating_temple.h"
#include "lobject.h"
#include "luaconf.h"


static int ft_defaultnewstringhook (lua_State *L, StkId obj, const char *str,
                                    size_t len) {
  return 0;
}

static int ft_defaultnewtablehook (lua_State *L, StkId obj, int b, int c) {
  return 0;
}


#define FT_DEFINE_HOOK_FUNC(install_func, hook_type, hook_var, \
                            default_hook_func) \
  LUAI_DDEF hook_type hook_var = &default_hook_func; \
\
  LUA_API hook_type install_func (hook_type hook) { \
    hook_type old_hook = hook_var; \
    hook_var = hook; \
    return old_hook; \
  }


FT_DEFINE_HOOK_FUNC(ft_installnewstringhook, ft_NewStringHook, ft_newstringhook,
                    ft_defaultnewstringhook)

FT_DEFINE_HOOK_FUNC(ft_installnewtablehook, ft_NewTableHook, ft_newtablehook,
                    ft_defaultnewtablehook)


#undef FT_DEFINE_HOOK_FUNC
