/*
** Hook functions for integration with the Floating Temple distributed
** interpreter
*/


#include <assert.h>
#include <stddef.h>


#define floating_temple_c
#define LUA_CORE

#include "lua.h"

#include "floating_temple.h"
#include "lobject.h"
#include "luaconf.h"


static int ft_defaultobjectreferencesequalhook
    (const struct ObjectReference *obj_ref1,
     const struct ObjectReference *obj_ref2) {
  assert(obj_ref1 != NULL);
  assert(obj_ref2 != NULL);

  return (obj_ref1 == obj_ref2) ? 1 : 0;
}

LUAI_DDEF ft_ObjectReferencesEqualHook ft_objectreferencesequalhook =
    &ft_defaultobjectreferencesequalhook;

LUA_API ft_ObjectReferencesEqualHook ft_installobjectreferencesequalhook
    (ft_ObjectReferencesEqualHook hook) {
  ft_ObjectReferencesEqualHook old_hook = ft_objectreferencesequalhook;
  ft_objectreferencesequalhook = hook;
  return old_hook;
}


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

FT_DEFINE_HOOK_FUNC(ft_installgettablehook, ft_GetTableHook, ft_gettablehook,
                    (lua_State *L, const TValue *table, const TValue *key,
                     StkId val),
                    ft_defaultgettablehook)

FT_DEFINE_HOOK_FUNC(ft_installsettablehook, ft_SetTableHook, ft_settablehook,
                    (lua_State *L, const TValue *table, const TValue *key,
                     const TValue* val),
                    ft_defaultsettablehook)

FT_DEFINE_HOOK_FUNC(ft_installobjlenhook, ft_ObjLenHook, ft_objlenhook,
                    (lua_State *L, StkId ra, const TValue *rb),
                    ft_defaultobjlenhook)

FT_DEFINE_HOOK_FUNC(ft_installsetlisthook, ft_SetListHook, ft_setlisthook,
                    (lua_State *L, const TValue *table, int n, int c),
                    ft_defaultsetlisthook)


#undef FT_DEFINE_HOOK_FUNC
