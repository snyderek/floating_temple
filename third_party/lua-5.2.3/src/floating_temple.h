/*
** Hook functions for integration with the Floating Temple distributed
** interpreter
*/


#ifndef floating_temple_h
#define floating_temple_h


#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


#include "lua.h"

#include "lobject.h"
#include "luaconf.h"


typedef void (*ft_NewStringHook) (lua_State *, StkId, const char *, size_t);
typedef void (*ft_NewTableHook) (lua_State *, StkId, int, int);

LUAI_DDEC ft_NewStringHook ft_newstringhook;
LUAI_DDEC ft_NewTableHook ft_newtablehook;

LUA_API ft_NewStringHook ft_installnewstringhook (ft_NewStringHook hook);
LUA_API ft_NewTableHook ft_installnewtablehook (ft_NewTableHook hook);


#ifdef __cplusplus
}
#endif


#endif
