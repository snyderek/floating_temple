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


/* The hook function returns non-zero if it performed the operation. */
#define FT_DECLARE_HOOK_FUNC(install_func, hook_type, hook_var, hook_params) \
  typedef int (*hook_type) hook_params; \
  LUAI_DDEC hook_type hook_var; \
  LUA_API hook_type install_func (hook_type hook);


FT_DECLARE_HOOK_FUNC(ft_installnewstringhook, ft_NewStringHook,
                     ft_newstringhook,
                     (lua_State *L, StkId obj, const char *str, size_t len))

FT_DECLARE_HOOK_FUNC(ft_installnewtablehook, ft_NewTableHook,
                     ft_newtablehook,
                     (lua_State *L, StkId obj, int b, int c))


#undef FT_DECLARE_HOOK_FUNC


#ifdef __cplusplus
}
#endif


#endif
