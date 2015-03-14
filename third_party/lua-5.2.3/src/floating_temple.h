/*
** Hook functions for integration with the Floating Temple distributed
** interpreter
*/


#ifndef floating_temple_h
#define floating_temple_h


#ifdef __cplusplus
extern "C" {
#endif


#include "lua.h"

#include "lobject.h"
#include "luaconf.h"


/* Each of these hook functions returns non-zero if it performed the operation.
 */
#define FT_DECLARE_HOOK_FUNC(install_func, hook_type, hook_var, hook_params) \
  typedef int (*hook_type) hook_params; \
  LUAI_DDEC hook_type hook_var; \
  LUA_API hook_type install_func (hook_type hook);


FT_DECLARE_HOOK_FUNC(ft_installnewtablehook, ft_NewTableHook,
                     ft_newtablehook,
                     (lua_State *L, StkId obj, int b, int c))

FT_DECLARE_HOOK_FUNC(ft_installgettablehook, ft_GetTableHook,
                     ft_gettablehook,
                     (lua_State *L, const TValue *table, const TValue *key,
                      StkId val))

FT_DECLARE_HOOK_FUNC(ft_installsettablehook, ft_SetTableHook,
                     ft_settablehook,
                     (lua_State *L, const TValue *table, const TValue *key,
                      const TValue* val))

FT_DECLARE_HOOK_FUNC(ft_installobjlenhook, ft_ObjLenHook,
                     ft_objlenhook,
                     (lua_State *L, StkId ra, const TValue *rb))

FT_DECLARE_HOOK_FUNC(ft_installsetlisthook, ft_SetListHook,
                     ft_setlisthook,
                     (lua_State *L, const TValue *table, int n, int c))


#undef FT_DECLARE_HOOK_FUNC


#ifdef __cplusplus
}
#endif


#endif
