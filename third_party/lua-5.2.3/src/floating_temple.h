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


#define FT_DECLARE_HOOK_FUNC(install_func, hook_type, hook_var, hook_params) \
  typedef void (*hook_type) hook_params; \
  LUAI_DDEC hook_type hook_var; \
  LUA_API hook_type install_func (hook_type hook);


FT_DECLARE_HOOK_FUNC(ft_installnewstringhook, ft_NewStringHook,
                     ft_newstringhook,
                     (lua_State *, StkId, const char *, size_t))

FT_DECLARE_HOOK_FUNC(ft_installnewtablehook, ft_NewTableHook,
                     ft_newtablehook,
                     (lua_State *, StkId, int, int))


#undef FT_DECLARE_HOOK_FUNC


#ifdef __cplusplus
}
#endif


#endif
