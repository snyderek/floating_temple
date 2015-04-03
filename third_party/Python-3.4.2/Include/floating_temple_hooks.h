#ifndef Py_FLOATING_TEMPLE_HOOKS_H
#define Py_FLOATING_TEMPLE_HOOKS_H
#ifdef __cplusplus
extern "C" {
#endif


typedef PyObject *(*object_creation_hook_func)(PyObject *);

extern object_creation_hook_func list_creation_hook;
extern object_creation_hook_func long_creation_hook;  /* Currently unused */

PyAPI_FUNC(object_creation_hook_func)
Py_InstallListCreationHook(object_creation_hook_func new_hook);

PyAPI_FUNC(object_creation_hook_func)
Py_InstallLongCreationHook(object_creation_hook_func new_hook);


#ifdef __cplusplus
}
#endif
#endif /* Py_FLOATING_TEMPLE_HOOKS_H */
