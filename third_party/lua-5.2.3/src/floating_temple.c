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
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"


static void ft_defaultnewstringhook (lua_State *L, StkId obj, const char * str,
                                     size_t len) {
  setsvalue2s(L, obj, luaS_newlstr(L, str, len));
}

static void ft_defaultnewtablehook (lua_State *L, StkId obj, int b, int c) {
  Table *t = luaH_new(L);
  sethvalue(L, obj, t);
  if (b != 0 || c != 0)
    luaH_resize(L, t, luaO_fb2int(b), luaO_fb2int(c));
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
