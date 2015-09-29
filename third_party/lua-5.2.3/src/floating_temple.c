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


static void ft_defaultlockhook (void) {
}

LUAI_DDEF ft_LockHook ft_lockhook = &ft_defaultlockhook;

LUA_API ft_LockHook ft_installlockhook (ft_LockHook hook) {
  ft_LockHook old_hook = ft_lockhook;
  ft_lockhook = hook;
  return old_hook;
}


static void ft_defaultunlockhook (void) {
}

LUAI_DDEF ft_UnlockHook ft_unlockhook = &ft_defaultunlockhook;

LUA_API ft_UnlockHook ft_installunlockhook (ft_UnlockHook hook) {
  ft_UnlockHook old_hook = ft_unlockhook;
  ft_unlockhook = hook;
  return old_hook;
}


static int ft_defaultobjectreferencesequalhook (const void *ft_obj1,
                                                const void *ft_obj2) {
  assert(ft_obj1 != NULL);
  assert(ft_obj2 != NULL);

  return (ft_obj1 == ft_obj2) ? 1 : 0;
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
                    (lua_State *L, TValue *obj, int b, int c),
                    ft_defaultnewtablehook)

FT_DEFINE_HOOK_FUNC(ft_installgettablehook, ft_GetTableHook, ft_gettablehook,
                    (lua_State *L, const TValue *table, const TValue *key,
                     TValue *val),
                    ft_defaultgettablehook)

FT_DEFINE_HOOK_FUNC(ft_installsettablehook, ft_SetTableHook, ft_settablehook,
                    (lua_State *L, const TValue *table, const TValue *key,
                     const TValue* val),
                    ft_defaultsettablehook)

FT_DEFINE_HOOK_FUNC(ft_installobjlenhook, ft_ObjLenHook, ft_objlenhook,
                    (lua_State *L, TValue *ra, const TValue *rb),
                    ft_defaultobjlenhook)

FT_DEFINE_HOOK_FUNC(ft_installsetlisthook, ft_SetListHook, ft_setlisthook,
                    (lua_State *L, const TValue *ra, int n, int c),
                    ft_defaultsetlisthook)


#undef FT_DEFINE_HOOK_FUNC
