/*
** Hook functions for integration with the Floating Temple distributed
** interpreter
*/


#ifndef floating_temple_h
#define floating_temple_h


#include "luaconf.h"


struct lua_State;
struct lua_TValue;


/* This hook function returns non-zero if the Floating Temple objects are equal.
 */
typedef int (*ft_ObjectReferencesEqualHook) (const void *ft_obj1,
                                             const void *ft_obj2);
LUAI_DDEC ft_ObjectReferencesEqualHook ft_objectreferencesequalhook;
LUA_API ft_ObjectReferencesEqualHook ft_installobjectreferencesequalhook
    (ft_ObjectReferencesEqualHook hook);


/* Each of these hook functions returns non-zero if it performed the operation.
 */
#define FT_DECLARE_HOOK_FUNC(install_func, hook_type, hook_var, hook_params) \
  typedef int (*hook_type) hook_params; \
  LUAI_DDEC hook_type hook_var; \
  LUA_API hook_type install_func (hook_type hook);


FT_DECLARE_HOOK_FUNC(ft_installnewtablehook, ft_NewTableHook,
                     ft_newtablehook,
                     (struct lua_State *L, struct lua_TValue *obj,
                      int b, int c))

FT_DECLARE_HOOK_FUNC(ft_installgettablehook, ft_GetTableHook,
                     ft_gettablehook,
                     (struct lua_State *L, const struct lua_TValue *table,
                      const struct lua_TValue *key, struct lua_TValue *val))

FT_DECLARE_HOOK_FUNC(ft_installsettablehook, ft_SetTableHook,
                     ft_settablehook,
                     (struct lua_State *L, const struct lua_TValue *table,
                      const struct lua_TValue *key,
                      const struct lua_TValue* val))

FT_DECLARE_HOOK_FUNC(ft_installobjlenhook, ft_ObjLenHook,
                     ft_objlenhook,
                     (struct lua_State *L, struct lua_TValue *ra,
                      const struct lua_TValue *rb))

FT_DECLARE_HOOK_FUNC(ft_installsetlisthook, ft_SetListHook,
                     ft_setlisthook,
                     (struct lua_State *L, const struct lua_TValue *ra, int n,
                      int c))

FT_DECLARE_HOOK_FUNC(ft_installtableinserthook, ft_TableInsertHook,
                     ft_tableinserthook,
                     (struct lua_State *L, const struct lua_TValue *table,
                      int pos, const struct lua_TValue *value))


#undef FT_DECLARE_HOOK_FUNC


#endif
