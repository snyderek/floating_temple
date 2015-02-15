#ifndef Py_DSS_HOOKS_H
#define Py_DSS_HOOKS_H
#ifdef __cplusplus
extern "C" {
#endif


typedef PyObject *(*object_creation_hook_func)(PyObject *);

extern object_creation_hook_func long_creation_hook;
extern object_creation_hook_func list_creation_hook;

PyAPI_FUNC(object_creation_hook_func)
Py_InstallLongCreationHook(object_creation_hook_func new_hook);

PyAPI_FUNC(object_creation_hook_func)
Py_InstallListCreationHook(object_creation_hook_func new_hook);


#ifdef __cplusplus
}
#endif
#endif /* Py_DSS_HOOKS_H */
