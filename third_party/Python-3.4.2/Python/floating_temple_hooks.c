/* Hook functions for the Floating Temple distributed interpreter */

#include "Python.h"
#include "floating_temple_hooks.h"

static PyObject *
DefaultObjectCreationHook(PyObject *obj)
{
    return obj;
}

object_creation_hook_func dict_creation_hook = DefaultObjectCreationHook;
object_creation_hook_func list_creation_hook = DefaultObjectCreationHook;

object_creation_hook_func
Py_InstallDictCreationHook(object_creation_hook_func new_hook)
{
    object_creation_hook_func old_hook = dict_creation_hook;
    dict_creation_hook = new_hook;
    return old_hook;
}

object_creation_hook_func
Py_InstallListCreationHook(object_creation_hook_func new_hook)
{
    object_creation_hook_func old_hook = list_creation_hook;
    list_creation_hook = new_hook;
    return old_hook;
}
