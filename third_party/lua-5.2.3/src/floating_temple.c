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


LUAI_DDEF ft_NewStringHook ft_newstringhook = &ft_defaultnewstringhook;
LUAI_DDEF ft_NewTableHook ft_newtablehook = &ft_defaultnewtablehook;


LUA_API ft_NewStringHook ft_installnewstringhook (ft_NewStringHook hook) {
  ft_NewStringHook old_hook = ft_newstringhook;
  ft_newstringhook = hook;
  return old_hook;
}

LUA_API ft_NewTableHook ft_installnewtablehook (ft_NewTableHook hook) {
  ft_NewTableHook old_hook = ft_newtablehook;
  ft_newtablehook = hook;
  return old_hook;
}
