import ctypes
import sys, os
import atexit

import py

from pypy.conftest import pypydir
from rpython.rtyper.lltypesystem import rffi, lltype
from rpython.rtyper.tool import rffi_platform
from rpython.rtyper.lltypesystem import ll2ctypes
from rpython.rtyper.annlowlevel import llhelper
from rpython.rlib.objectmodel import we_are_translated
from rpython.translator import cdir
from rpython.translator.tool.cbuild import ExternalCompilationInfo
from rpython.translator.gensupp import NameManager
from rpython.tool.udir import udir
from rpython.translator import platform
from pypy.module.cpyext.state import State
from pypy.interpreter.error import OperationError, oefmt
from pypy.interpreter.baseobjspace import W_Root
from pypy.interpreter.gateway import unwrap_spec
from pypy.interpreter.nestedscope import Cell
from pypy.interpreter.module import Module
from pypy.interpreter.function import StaticMethod
from pypy.objspace.std.sliceobject import W_SliceObject
from pypy.module.__builtin__.descriptor import W_Property
from pypy.module.__builtin__.interp_classobj import W_ClassObject
from pypy.module.micronumpy.base import W_NDimArray
from rpython.rlib.entrypoint import entrypoint_lowlevel
from rpython.rlib.rposix import is_valid_fd, validate_fd
from rpython.rlib.unroll import unrolling_iterable
from rpython.rlib.objectmodel import specialize
from rpython.rlib.exports import export_struct
from pypy.module import exceptions
from pypy.module.exceptions import interp_exceptions
# CPython 2.4 compatibility
from py.builtin import BaseException
from rpython.tool.sourcetools import func_with_new_name
from rpython.rtyper.lltypesystem.lloperation import llop

DEBUG_WRAPPER = True

# update these for other platforms
Py_ssize_t = lltype.Typedef(rffi.SSIZE_T, 'Py_ssize_t')
Py_ssize_tP = rffi.CArrayPtr(Py_ssize_t)
size_t = rffi.ULONG
ADDR = lltype.Signed

pypydir = py.path.local(pypydir)
include_dir = pypydir / 'module' / 'cpyext' / 'include'
source_dir = pypydir / 'module' / 'cpyext' / 'src'
translator_c_dir = py.path.local(cdir)
include_dirs = [
    include_dir,
    translator_c_dir,
    udir,
    ]

class CConfig:
    _compilation_info_ = ExternalCompilationInfo(
        include_dirs=include_dirs,
        includes=['Python.h', 'stdarg.h'],
        compile_extra=['-DPy_BUILD_CORE'],
        )

class CConfig2:
    _compilation_info_ = CConfig._compilation_info_

class CConfig_constants:
    _compilation_info_ = CConfig._compilation_info_

VA_LIST_P = rffi.VOIDP # rffi.COpaquePtr('va_list')
CONST_STRING = lltype.Ptr(lltype.Array(lltype.Char,
                                       hints={'nolength': True}),
                          use_cache=False)
CONST_WSTRING = lltype.Ptr(lltype.Array(lltype.UniChar,
                                        hints={'nolength': True}),
                           use_cache=False)
assert CONST_STRING is not rffi.CCHARP
assert CONST_STRING == rffi.CCHARP
assert CONST_WSTRING is not rffi.CWCHARP
assert CONST_WSTRING == rffi.CWCHARP

# FILE* interface
FILEP = rffi.COpaquePtr('FILE')

if sys.platform == 'win32':
    fileno = rffi.llexternal('_fileno', [FILEP], rffi.INT)
else:
    fileno = rffi.llexternal('fileno', [FILEP], rffi.INT)

fopen = rffi.llexternal('fopen', [CONST_STRING, CONST_STRING], FILEP)

_fclose = rffi.llexternal('fclose', [FILEP], rffi.INT)
def fclose(fp):
    if not is_valid_fd(fileno(fp)):
        return -1
    return _fclose(fp)

_fwrite = rffi.llexternal('fwrite',
                         [rffi.VOIDP, rffi.SIZE_T, rffi.SIZE_T, FILEP],
                         rffi.SIZE_T)
def fwrite(buf, sz, n, fp):
    validate_fd(fileno(fp))
    return _fwrite(buf, sz, n, fp)

_fread = rffi.llexternal('fread',
                        [rffi.VOIDP, rffi.SIZE_T, rffi.SIZE_T, FILEP],
                        rffi.SIZE_T)
def fread(buf, sz, n, fp):
    validate_fd(fileno(fp))
    return _fread(buf, sz, n, fp)

_feof = rffi.llexternal('feof', [FILEP], rffi.INT)
def feof(fp):
    validate_fd(fileno(fp))
    return _feof(fp)


constant_names = """
Py_TPFLAGS_READY Py_TPFLAGS_READYING Py_TPFLAGS_HAVE_GETCHARBUFFER
METH_COEXIST METH_STATIC METH_CLASS
METH_NOARGS METH_VARARGS METH_KEYWORDS METH_O
Py_TPFLAGS_HEAPTYPE Py_TPFLAGS_HAVE_CLASS
Py_LT Py_LE Py_EQ Py_NE Py_GT Py_GE
""".split()
for name in constant_names:
    setattr(CConfig_constants, name, rffi_platform.ConstantInteger(name))
udir.join('pypy_decl.h').write("/* Will be filled later */\n")
udir.join('pypy_macros.h').write("/* Will be filled later */\n")
globals().update(rffi_platform.configure(CConfig_constants))

def _copy_header_files(headers, dstdir):
    for header in headers:
        target = dstdir.join(header.basename)
        try:
            header.copy(dstdir)
        except py.error.EACCES:
            target.remove()   # maybe it was a read-only file
            header.copy(dstdir)
        target.chmod(0444) # make the file read-only, to make sure that nobody
                           # edits it by mistake

def copy_header_files(dstdir):
    # XXX: 20 lines of code to recursively copy a directory, really??
    assert dstdir.check(dir=True)
    headers = include_dir.listdir('*.h') + include_dir.listdir('*.inl')
    for name in ("pypy_decl.h", "pypy_macros.h"):
        headers.append(udir.join(name))
    _copy_header_files(headers, dstdir)

    try:
        dstdir.mkdir('numpy')
    except py.error.EEXIST:
        pass
    numpy_dstdir = dstdir / 'numpy'

    numpy_include_dir = include_dir / 'numpy'
    numpy_headers = numpy_include_dir.listdir('*.h') + numpy_include_dir.listdir('*.inl')
    _copy_header_files(numpy_headers, numpy_dstdir)


class NotSpecified(object):
    pass
_NOT_SPECIFIED = NotSpecified()
class CannotFail(object):
    pass
CANNOT_FAIL = CannotFail()

# The same function can be called in three different contexts:
# (1) from C code
# (2) in the test suite, though the "api" object
# (3) from RPython code, for example in the implementation of another function.
#
# In contexts (2) and (3), a function declaring a PyObject argument type will
# receive a wrapped pypy object if the parameter name starts with 'w_', a
# reference (= rffi pointer) otherwise; conversion is automatic.  Context (2)
# only allows calls with a wrapped object.
#
# Functions with a PyObject return type should return a wrapped object.
#
# Functions may raise exceptions.  In context (3), the exception flows normally
# through the calling function.  In context (1) and (2), the exception is
# caught; if it is an OperationError, it is stored in the thread state; other
# exceptions generate a OperationError(w_SystemError); and the funtion returns
# the error value specifed in the API.
#

cpyext_namespace = NameManager('cpyext_')

class ApiFunction:
    def __init__(self, argtypes, restype, callable, error=_NOT_SPECIFIED,
                 c_name=None):
        self.argtypes = argtypes
        self.restype = restype
        self.functype = lltype.Ptr(lltype.FuncType(argtypes, restype))
        self.callable = callable
        if error is not _NOT_SPECIFIED:
            self.error_value = error
        self.c_name = c_name

        # extract the signature from the (CPython-level) code object
        from pypy.interpreter import pycode
        argnames, varargname, kwargname = pycode.cpython_code_signature(callable.func_code)

        assert argnames[0] == 'space'
        self.argnames = argnames[1:]
        assert len(self.argnames) == len(self.argtypes)

    def _freeze_(self):
        return True

    def get_llhelper(self, space):
        llh = getattr(self, '_llhelper', None)
        if llh is None:
            llh = llhelper(self.functype, self.get_wrapper(space))
            self._llhelper = llh
        return llh

    @specialize.memo()
    def get_wrapper(self, space):
        wrapper = getattr(self, '_wrapper', None)
        if wrapper is None:
            wrapper = make_wrapper(space, self.callable)
            self._wrapper = wrapper
            wrapper.relax_sig_check = True
            if self.c_name is not None:
                wrapper.c_name = cpyext_namespace.uniquename(self.c_name)
        return wrapper

def cpython_api(argtypes, restype, error=_NOT_SPECIFIED, external=True):
    """
    Declares a function to be exported.
    - `argtypes`, `restype` are lltypes and describe the function signature.
    - `error` is the value returned when an applevel exception is raised. The
      special value 'CANNOT_FAIL' (also when restype is Void) turns an eventual
      exception into a wrapped SystemError.  Unwrapped exceptions also cause a
      SytemError.
    - set `external` to False to get a C function pointer, but not exported by
      the API headers.
    """
    if isinstance(restype, lltype.Typedef):
        real_restype = restype.OF
    else:
        real_restype = restype

    if error is _NOT_SPECIFIED:
        if isinstance(real_restype, lltype.Ptr):
            error = lltype.nullptr(real_restype.TO)
        elif real_restype is lltype.Void:
            error = CANNOT_FAIL
    if type(error) is int:
        error = rffi.cast(real_restype, error)
    expect_integer = (isinstance(real_restype, lltype.Primitive) and
                      rffi.cast(restype, 0) == 0)

    def decorate(func):
        func_name = func.func_name
        if external:
            c_name = None
        else:
            c_name = func_name
        api_function = ApiFunction(argtypes, restype, func, error, c_name=c_name)
        func.api_func = api_function

        if external:
            assert func_name not in FUNCTIONS, (
                "%s already registered" % func_name)

        if error is _NOT_SPECIFIED:
            raise ValueError("function %s has no return value for exceptions"
                             % func)
        def make_unwrapper(catch_exception):
            names = api_function.argnames
            types_names_enum_ui = unrolling_iterable(enumerate(
                zip(api_function.argtypes,
                    [tp_name.startswith("w_") for tp_name in names])))

            @specialize.ll()
            def unwrapper(space, *args):
                from pypy.module.cpyext.pyobject import Py_DecRef
                from pypy.module.cpyext.pyobject import make_ref, from_ref
                from pypy.module.cpyext.pyobject import Reference
                newargs = ()
                to_decref = []
                assert len(args) == len(api_function.argtypes)
                for i, (ARG, is_wrapped) in types_names_enum_ui:
                    input_arg = args[i]
                    if is_PyObject(ARG) and not is_wrapped:
                        # build a reference
                        if input_arg is None:
                            arg = lltype.nullptr(PyObject.TO)
                        elif isinstance(input_arg, W_Root):
                            ref = make_ref(space, input_arg)
                            to_decref.append(ref)
                            arg = rffi.cast(ARG, ref)
                        else:
                            arg = input_arg
                    elif is_PyObject(ARG) and is_wrapped:
                        # convert to a wrapped object
                        if input_arg is None:
                            arg = input_arg
                        elif isinstance(input_arg, W_Root):
                            arg = input_arg
                        else:
                            try:
                                arg = from_ref(space,
                                           rffi.cast(PyObject, input_arg))
                            except TypeError, e:
                                err = OperationError(space.w_TypeError,
                                         space.wrap(
                                        "could not cast arg to PyObject"))
                                if not catch_exception:
                                    raise err
                                state = space.fromcache(State)
                                state.set_exception(err)
                                if is_PyObject(restype):
                                    return None
                                else:
                                    return api_function.error_value
                    else:
                        # convert to a wrapped object
                        arg = input_arg
                    newargs += (arg, )
                try:
                    try:
                        res = func(space, *newargs)
                    except OperationError, e:
                        if not catch_exception:
                            raise
                        if not hasattr(api_function, "error_value"):
                            raise
                        state = space.fromcache(State)
                        state.set_exception(e)
                        if is_PyObject(restype):
                            return None
                        else:
                            return api_function.error_value
                    if not we_are_translated():
                        got_integer = isinstance(res, (int, long, float))
                        assert got_integer == expect_integer,'got %r not integer' % res
                    if res is None:
                        return None
                    elif isinstance(res, Reference):
                        return res.get_wrapped(space)
                    else:
                        return res
                finally:
                    for arg in to_decref:
                        Py_DecRef(space, arg)
            unwrapper.func = func
            unwrapper.api_func = api_function
            unwrapper._always_inline_ = 'try'
            return unwrapper

        unwrapper_catch = make_unwrapper(True)
        unwrapper_raise = make_unwrapper(False)
        if external:
            FUNCTIONS[func_name] = api_function
        INTERPLEVEL_API[func_name] = unwrapper_catch # used in tests
        return unwrapper_raise # used in 'normal' RPython code.
    return decorate

def cpython_struct(name, fields, forward=None, level=1):
    configname = name.replace(' ', '__')
    if level == 1:
        config = CConfig
    else:
        config = CConfig2
    setattr(config, configname, rffi_platform.Struct(name, fields))
    if forward is None:
        forward = lltype.ForwardReference()
    TYPES[configname] = forward
    return forward

INTERPLEVEL_API = {}
FUNCTIONS = {}

# These are C symbols which cpyext will export, but which are defined in .c
# files somewhere in the implementation of cpyext (rather than being defined in
# RPython).
SYMBOLS_C = [
    'Py_FatalError', 'PyOS_snprintf', 'PyOS_vsnprintf', 'PyArg_Parse',
    'PyArg_ParseTuple', 'PyArg_UnpackTuple', 'PyArg_ParseTupleAndKeywords',
    'PyArg_VaParse', 'PyArg_VaParseTupleAndKeywords', '_PyArg_NoKeywords',
    'PyString_FromFormat', 'PyString_FromFormatV',
    'PyModule_AddObject', 'PyModule_AddIntConstant', 'PyModule_AddStringConstant',
    'Py_BuildValue', 'Py_VaBuildValue', 'PyTuple_Pack',
    '_PyArg_Parse_SizeT', '_PyArg_ParseTuple_SizeT',
    '_PyArg_ParseTupleAndKeywords_SizeT', '_PyArg_VaParse_SizeT',
    '_PyArg_VaParseTupleAndKeywords_SizeT',
    '_Py_BuildValue_SizeT', '_Py_VaBuildValue_SizeT',

    'PyErr_Format', 'PyErr_NewException', 'PyErr_NewExceptionWithDoc',
    'PySys_WriteStdout', 'PySys_WriteStderr',

    'PyEval_CallFunction', 'PyEval_CallMethod', 'PyObject_CallFunction',
    'PyObject_CallMethod', 'PyObject_CallFunctionObjArgs', 'PyObject_CallMethodObjArgs',
    '_PyObject_CallFunction_SizeT', '_PyObject_CallMethod_SizeT',

    'PyBuffer_FromMemory', 'PyBuffer_FromReadWriteMemory', 'PyBuffer_FromObject',
    'PyBuffer_FromReadWriteObject', 'PyBuffer_New', 'PyBuffer_Type', '_Py_get_buffer_type',

    'PyCObject_FromVoidPtr', 'PyCObject_FromVoidPtrAndDesc', 'PyCObject_AsVoidPtr',
    'PyCObject_GetDesc', 'PyCObject_Import', 'PyCObject_SetVoidPtr',
    'PyCObject_Type', '_Py_get_cobject_type',

    'PyCapsule_New', 'PyCapsule_IsValid', 'PyCapsule_GetPointer',
    'PyCapsule_GetName', 'PyCapsule_GetDestructor', 'PyCapsule_GetContext',
    'PyCapsule_SetPointer', 'PyCapsule_SetName', 'PyCapsule_SetDestructor',
    'PyCapsule_SetContext', 'PyCapsule_Import', 'PyCapsule_Type', '_Py_get_capsule_type',

    'PyObject_AsReadBuffer', 'PyObject_AsWriteBuffer', 'PyObject_CheckReadBuffer',

    'PyOS_getsig', 'PyOS_setsig',
    'PyThread_get_thread_ident', 'PyThread_allocate_lock', 'PyThread_free_lock',
    'PyThread_acquire_lock', 'PyThread_release_lock',
    'PyThread_create_key', 'PyThread_delete_key', 'PyThread_set_key_value',
    'PyThread_get_key_value', 'PyThread_delete_key_value',
    'PyThread_ReInitTLS',

    'PyStructSequence_InitType', 'PyStructSequence_New',

    'PyFunction_Type', 'PyMethod_Type', 'PyRange_Type', 'PyTraceBack_Type',

    'PyArray_Type', '_PyArray_FILLWBYTE', '_PyArray_ZEROS', '_PyArray_CopyInto',

    'Py_DebugFlag', 'Py_VerboseFlag', 'Py_InteractiveFlag', 'Py_InspectFlag',
    'Py_OptimizeFlag', 'Py_NoSiteFlag', 'Py_BytesWarningFlag', 'Py_UseClassExceptionsFlag',
    'Py_FrozenFlag', 'Py_TabcheckFlag', 'Py_UnicodeFlag', 'Py_IgnoreEnvironmentFlag',
    'Py_DivisionWarningFlag', 'Py_DontWriteBytecodeFlag', 'Py_NoUserSiteDirectory',
    '_Py_QnewFlag', 'Py_Py3kWarningFlag', 'Py_HashRandomizationFlag', '_Py_PackageContext',
]
TYPES = {}
GLOBALS = { # this needs to include all prebuilt pto, otherwise segfaults occur
    '_Py_NoneStruct#': ('PyObject*', 'space.w_None'),
    '_Py_TrueStruct#': ('PyObject*', 'space.w_True'),
    '_Py_ZeroStruct#': ('PyObject*', 'space.w_False'),
    '_Py_NotImplementedStruct#': ('PyObject*', 'space.w_NotImplemented'),
    '_Py_EllipsisObject#': ('PyObject*', 'space.w_Ellipsis'),
    'PyDateTimeAPI': ('PyDateTime_CAPI*', 'None'),
    }
FORWARD_DECLS = []
INIT_FUNCTIONS = []
BOOTSTRAP_FUNCTIONS = []

def build_exported_objects():
    # Standard exceptions
    # PyExc_BaseException, PyExc_Exception, PyExc_ValueError, PyExc_KeyError,
    # PyExc_IndexError, PyExc_IOError, PyExc_OSError, PyExc_TypeError,
    # PyExc_AttributeError, PyExc_OverflowError, PyExc_ImportError,
    # PyExc_NameError, PyExc_MemoryError, PyExc_RuntimeError,
    # PyExc_UnicodeEncodeError, PyExc_UnicodeDecodeError, ...
    for exc_name in exceptions.Module.interpleveldefs.keys():
        GLOBALS['PyExc_' + exc_name] = (
            'PyTypeObject*',
            'space.gettypeobject(interp_exceptions.W_%s.typedef)'% (exc_name, ))

    # Common types with their own struct
    for cpyname, pypyexpr in {
        "PyType_Type": "space.w_type",
        "PyString_Type": "space.w_str",
        "PyUnicode_Type": "space.w_unicode",
        "PyBaseString_Type": "space.w_basestring",
        "PyDict_Type": "space.w_dict",
        "PyTuple_Type": "space.w_tuple",
        "PyList_Type": "space.w_list",
        "PySet_Type": "space.w_set",
        "PyFrozenSet_Type": "space.w_frozenset",
        "PyInt_Type": "space.w_int",
        "PyBool_Type": "space.w_bool",
        "PyFloat_Type": "space.w_float",
        "PyLong_Type": "space.w_long",
        "PyComplex_Type": "space.w_complex",
        "PyByteArray_Type": "space.w_bytearray",
        "PyMemoryView_Type": "space.w_memoryview",
        "PyArray_Type": "space.gettypeobject(W_NDimArray.typedef)",
        "PyBaseObject_Type": "space.w_object",
        'PyNone_Type': 'space.type(space.w_None)',
        'PyNotImplemented_Type': 'space.type(space.w_NotImplemented)',
        'PyCell_Type': 'space.gettypeobject(Cell.typedef)',
        'PyModule_Type': 'space.gettypeobject(Module.typedef)',
        'PyProperty_Type': 'space.gettypeobject(W_Property.typedef)',
        'PySlice_Type': 'space.gettypeobject(W_SliceObject.typedef)',
        'PyClass_Type': 'space.gettypeobject(W_ClassObject.typedef)',
        'PyStaticMethod_Type': 'space.gettypeobject(StaticMethod.typedef)',
        'PyCFunction_Type': 'space.gettypeobject(cpyext.methodobject.W_PyCFunctionObject.typedef)',
        'PyWrapperDescr_Type': 'space.gettypeobject(cpyext.methodobject.W_PyCMethodObject.typedef)'
        }.items():
        GLOBALS['%s#' % (cpyname, )] = ('PyTypeObject*', pypyexpr)

    for cpyname in '''PyMethodObject PyListObject PyLongObject
                      PyDictObject PyTupleObject PyClassObject'''.split():
        FORWARD_DECLS.append('typedef struct { PyObject_HEAD } %s'
                             % (cpyname, ))
build_exported_objects()

def get_structtype_for_ctype(ctype):
    from pypy.module.cpyext.typeobjectdefs import PyTypeObjectPtr
    from pypy.module.cpyext.cdatetime import PyDateTime_CAPI
    return {"PyObject*": PyObject, "PyTypeObject*": PyTypeObjectPtr,
            "PyDateTime_CAPI*": lltype.Ptr(PyDateTime_CAPI)}[ctype]

PyTypeObject = lltype.ForwardReference()
PyTypeObjectPtr = lltype.Ptr(PyTypeObject)
# It is important that these PyObjects are allocated in a raw fashion
# Thus we cannot save a forward pointer to the wrapped object
# So we need a forward and backward mapping in our State instance
PyObjectStruct = lltype.ForwardReference()
PyObject = lltype.Ptr(PyObjectStruct)
PyObjectFields = (("ob_refcnt", lltype.Signed), ("ob_type", PyTypeObjectPtr))
PyVarObjectFields = PyObjectFields + (("ob_size", Py_ssize_t), )
cpython_struct('PyObject', PyObjectFields, PyObjectStruct)
PyVarObjectStruct = cpython_struct("PyVarObject", PyVarObjectFields)
PyVarObject = lltype.Ptr(PyVarObjectStruct)

Py_buffer = cpython_struct(
    "Py_buffer", (
        ('buf', rffi.VOIDP),
        ('obj', PyObject),
        ('len', Py_ssize_t),
        ('itemsize', Py_ssize_t),

        ('readonly', lltype.Signed),
        ('ndim', lltype.Signed),
        ('format', rffi.CCHARP),
        ('shape', Py_ssize_tP),
        ('strides', Py_ssize_tP),
        ('suboffsets', Py_ssize_tP),
        #('smalltable', rffi.CFixedArray(Py_ssize_t, 2)),
        ('internal', rffi.VOIDP)
        ))

@specialize.memo()
def is_PyObject(TYPE):
    if not isinstance(TYPE, lltype.Ptr):
        return False
    return hasattr(TYPE.TO, 'c_ob_refcnt') and hasattr(TYPE.TO, 'c_ob_type')

# a pointer to PyObject
PyObjectP = rffi.CArrayPtr(PyObject)

VA_TP_LIST = {}
#{'int': lltype.Signed,
#              'PyObject*': PyObject,
#              'PyObject**': PyObjectP,
#              'int*': rffi.INTP}

def configure_types():
    for config in (CConfig, CConfig2):
        for name, TYPE in rffi_platform.configure(config).iteritems():
            if name in TYPES:
                TYPES[name].become(TYPE)

def build_type_checkers(type_name, cls=None):
    """
    Builds two api functions: Py_XxxCheck() and Py_XxxCheckExact().
    - if `cls` is None, the type is space.w_[type].
    - if `cls` is a string, it is the name of a space attribute, e.g. 'w_str'.
    - else `cls` must be a W_Class with a typedef.
    """
    if cls is None:
        attrname = "w_" + type_name.lower()
        def get_w_type(space):
            return getattr(space, attrname)
    elif isinstance(cls, str):
        def get_w_type(space):
            return getattr(space, cls)
    else:
        def get_w_type(space):
            return space.gettypeobject(cls.typedef)
    check_name = "Py" + type_name + "_Check"

    def check(space, w_obj):
        "Implements the Py_Xxx_Check function"
        w_obj_type = space.type(w_obj)
        w_type = get_w_type(space)
        return (space.is_w(w_obj_type, w_type) or
                space.is_true(space.issubtype(w_obj_type, w_type)))
    def check_exact(space, w_obj):
        "Implements the Py_Xxx_CheckExact function"
        w_obj_type = space.type(w_obj)
        w_type = get_w_type(space)
        return space.is_w(w_obj_type, w_type)

    check = cpython_api([PyObject], rffi.INT_real, error=CANNOT_FAIL)(
        func_with_new_name(check, check_name))
    check_exact = cpython_api([PyObject], rffi.INT_real, error=CANNOT_FAIL)(
        func_with_new_name(check_exact, check_name + "Exact"))
    return check, check_exact

pypy_debug_catch_fatal_exception = rffi.llexternal('pypy_debug_catch_fatal_exception', [], lltype.Void)

# Make the wrapper for the cases (1) and (2)
def make_wrapper(space, callable):
    "NOT_RPYTHON"
    names = callable.api_func.argnames
    argtypes_enum_ui = unrolling_iterable(enumerate(zip(callable.api_func.argtypes,
        [name.startswith("w_") for name in names])))
    fatal_value = callable.api_func.restype._defl()

    @specialize.ll()
    def wrapper(*args):
        from pypy.module.cpyext.pyobject import make_ref, from_ref
        from pypy.module.cpyext.pyobject import Reference
        # we hope that malloc removal removes the newtuple() that is
        # inserted exactly here by the varargs specializer
        rffi.stackcounter.stacks_counter += 1
        llop.gc_stack_bottom(lltype.Void)   # marker for trackgcroot.py
        retval = fatal_value
        boxed_args = ()
        try:
            if not we_are_translated() and DEBUG_WRAPPER:
                print >>sys.stderr, callable,
            assert len(args) == len(callable.api_func.argtypes)
            for i, (typ, is_wrapped) in argtypes_enum_ui:
                arg = args[i]
                if is_PyObject(typ) and is_wrapped:
                    if arg:
                        arg_conv = from_ref(space, rffi.cast(PyObject, arg))
                    else:
                        arg_conv = None
                else:
                    arg_conv = arg
                boxed_args += (arg_conv, )
            state = space.fromcache(State)
            try:
                result = callable(space, *boxed_args)
                if not we_are_translated() and DEBUG_WRAPPER:
                    print >>sys.stderr, " DONE"
            except OperationError, e:
                failed = True
                state.set_exception(e)
            except BaseException, e:
                failed = True
                if not we_are_translated():
                    message = repr(e)
                    import traceback
                    traceback.print_exc()
                else:
                    message = str(e)
                state.set_exception(OperationError(space.w_SystemError,
                                                   space.wrap(message)))
            else:
                failed = False

            if failed:
                error_value = callable.api_func.error_value
                if error_value is CANNOT_FAIL:
                    raise SystemError("The function '%s' was not supposed to fail"
                                      % (callable.__name__,))
                retval = error_value

            elif is_PyObject(callable.api_func.restype):
                if result is None:
                    retval = rffi.cast(callable.api_func.restype,
                                       make_ref(space, None))
                elif isinstance(result, Reference):
                    retval = result.get_ref(space)
                elif not rffi._isllptr(result):
                    retval = rffi.cast(callable.api_func.restype,
                                       make_ref(space, result))
                else:
                    retval = result
            elif callable.api_func.restype is not lltype.Void:
                retval = rffi.cast(callable.api_func.restype, result)
        except Exception, e:
            print 'Fatal error in cpyext, CPython compatibility layer, calling', callable.__name__
            print 'Either report a bug or consider not using this particular extension'
            if not we_are_translated():
                import traceback
                traceback.print_exc()
                print str(e)
                # we can't do much here, since we're in ctypes, swallow
            else:
                print str(e)
                pypy_debug_catch_fatal_exception()
        rffi.stackcounter.stacks_counter -= 1
        return retval
    callable._always_inline_ = 'try'
    wrapper.__name__ = "wrapper for %r" % (callable, )
    return wrapper

def process_va_name(name):
    return name.replace('*', '_star')

def setup_va_functions(eci):
    for name, TP in VA_TP_LIST.iteritems():
        name_no_star = process_va_name(name)
        func = rffi.llexternal('pypy_va_get_%s' % name_no_star, [VA_LIST_P],
                               TP, compilation_info=eci)
        globals()['va_get_%s' % name_no_star] = func

def setup_init_functions(eci, translating):
    if translating:
        prefix = 'PyPy'
    else:
        prefix = 'cpyexttest'
    # jump through hoops to avoid releasing the GIL during initialization
    # of the cpyext module.  The C functions are called with no wrapper,
    # but must not do anything like calling back PyType_Ready().  We
    # use them just to get a pointer to the PyTypeObjects defined in C.
    get_buffer_type = rffi.llexternal('_%s_get_buffer_type' % prefix,
                                      [], PyTypeObjectPtr,
                                      compilation_info=eci, _nowrapper=True)
    get_cobject_type = rffi.llexternal('_%s_get_cobject_type' % prefix,
                                       [], PyTypeObjectPtr,
                                       compilation_info=eci, _nowrapper=True)
    get_capsule_type = rffi.llexternal('_%s_get_capsule_type' % prefix,
                                       [], PyTypeObjectPtr,
                                       compilation_info=eci, _nowrapper=True)
    def init_types(space):
        from pypy.module.cpyext.typeobject import py_type_ready
        py_type_ready(space, get_buffer_type())
        py_type_ready(space, get_cobject_type())
        py_type_ready(space, get_capsule_type())
    INIT_FUNCTIONS.append(init_types)
    from pypy.module.posix.interp_posix import add_fork_hook
    reinit_tls = rffi.llexternal('%sThread_ReInitTLS' % prefix, [], lltype.Void,
                                 compilation_info=eci)
    add_fork_hook('child', reinit_tls)

def init_function(func):
    INIT_FUNCTIONS.append(func)
    return func

def bootstrap_function(func):
    BOOTSTRAP_FUNCTIONS.append(func)
    return func

def run_bootstrap_functions(space):
    for func in BOOTSTRAP_FUNCTIONS:
        func(space)

def c_function_signature(db, func):
    restype = db.gettype(func.restype).replace('@', '').strip()
    args = []
    for i, argtype in enumerate(func.argtypes):
        if argtype is CONST_STRING:
            arg = 'const char *@'
        elif argtype is CONST_WSTRING:
            arg = 'const wchar_t *@'
        else:
            arg = db.gettype(argtype)
        arg = arg.replace('@', 'arg%d' % (i,)).strip()
        args.append(arg)
    args = ', '.join(args) or "void"
    return restype, args

#_____________________________________________________
# Build the bridge DLL, Allow extension DLLs to call
# back into Pypy space functions
# Do not call this more than once per process
def build_bridge(space):
    "NOT_RPYTHON"
    from pypy.module.cpyext.pyobject import make_ref

    export_symbols = list(FUNCTIONS) + SYMBOLS_C + list(GLOBALS)
    from rpython.translator.c.database import LowLevelDatabase
    db = LowLevelDatabase()

    generate_macros(export_symbols, prefix='cpyexttest')

    # Structure declaration code
    members = []
    structindex = {}
    for name, func in sorted(FUNCTIONS.iteritems()):
        restype, args = c_function_signature(db, func)
        members.append('%s (*%s)(%s);' % (restype, name, args))
        structindex[name] = len(structindex)
    structmembers = '\n'.join(members)
    struct_declaration_code = """\
    struct PyPyAPI {
    %(members)s
    } _pypyAPI;
    struct PyPyAPI* pypyAPI = &_pypyAPI;
    """ % dict(members=structmembers)

    functions = generate_decls_and_callbacks(db, export_symbols)

    global_objects = []
    for name, (typ, expr) in GLOBALS.iteritems():
        if "#" in name:
            continue
        if typ == 'PyDateTime_CAPI*':
            continue
        elif name.startswith('PyExc_'):
            global_objects.append('%s _%s;' % (typ[:-1], name))
        else:
            global_objects.append('%s %s = NULL;' % (typ, name))
    global_code = '\n'.join(global_objects)

    prologue = ("#include <Python.h>\n"
                "#include <src/thread.c>\n")
    code = (prologue +
            struct_declaration_code +
            global_code +
            '\n' +
            '\n'.join(functions))

    eci = build_eci(True, export_symbols, code)
    eci = eci.compile_shared_lib(
        outputfilename=str(udir / "module_cache" / "pypyapi"))
    modulename = py.path.local(eci.libraries[-1])

    run_bootstrap_functions(space)

    # load the bridge, and init structure
    import ctypes
    bridge = ctypes.CDLL(str(modulename), mode=ctypes.RTLD_GLOBAL)

    space.fromcache(State).install_dll(eci)

    # populate static data
    for name, (typ, expr) in GLOBALS.iteritems():
        from pypy.module import cpyext
        w_obj = eval(expr)
        if name.endswith('#'):
            name = name[:-1]
            isptr = False
        else:
            isptr = True
        if name.startswith('PyExc_'):
            isptr = False

        INTERPLEVEL_API[name] = w_obj

        name = name.replace('Py', 'cpyexttest')
        if isptr:
            ptr = ctypes.c_void_p.in_dll(bridge, name)
            if typ == 'PyObject*':
                value = make_ref(space, w_obj)
            elif typ == 'PyDateTime_CAPI*':
                value = w_obj
            else:
                assert False, "Unknown static pointer: %s %s" % (typ, name)
            ptr.value = ctypes.cast(ll2ctypes.lltype2ctypes(value),
                                    ctypes.c_void_p).value
        elif typ in ('PyObject*', 'PyTypeObject*'):
            if name.startswith('PyPyExc_') or name.startswith('cpyexttestExc_'):
                # we already have the pointer
                in_dll = ll2ctypes.get_ctypes_type(PyObject).in_dll(bridge, name)
                py_obj = ll2ctypes.ctypes2lltype(PyObject, in_dll)
            else:
                # we have a structure, get its address
                in_dll = ll2ctypes.get_ctypes_type(PyObject.TO).in_dll(bridge, name)
                py_obj = ll2ctypes.ctypes2lltype(PyObject, ctypes.pointer(in_dll))
            from pypy.module.cpyext.pyobject import (
                track_reference, get_typedescr)
            w_type = space.type(w_obj)
            typedescr = get_typedescr(w_type.instancetypedef)
            py_obj.c_ob_refcnt = 1
            py_obj.c_ob_type = rffi.cast(PyTypeObjectPtr,
                                         make_ref(space, w_type))
            typedescr.attach(space, py_obj, w_obj)
            track_reference(space, py_obj, w_obj)
        else:
            assert False, "Unknown static object: %s %s" % (typ, name)

    pypyAPI = ctypes.POINTER(ctypes.c_void_p).in_dll(bridge, 'pypyAPI')

    # implement structure initialization code
    for name, func in FUNCTIONS.iteritems():
        if name.startswith('cpyext_'): # XXX hack
            continue
        pypyAPI[structindex[name]] = ctypes.cast(
            ll2ctypes.lltype2ctypes(func.get_llhelper(space)),
            ctypes.c_void_p)

    setup_va_functions(eci)

    setup_init_functions(eci, translating=False)
    return modulename.new(ext='')

def mangle_name(prefix, name):
    if name.startswith('Py'):
        return prefix + name[2:]
    elif name.startswith('_Py'):
        return '_' + prefix + name[3:]
    else:
        return None

def generate_macros(export_symbols, prefix):
    "NOT_RPYTHON"
    pypy_macros = []
    renamed_symbols = []
    for name in export_symbols:
        name = name.replace("#", "")
        newname = mangle_name(prefix, name)
        assert newname, name
        pypy_macros.append('#define %s %s' % (name, newname))
        if name.startswith("PyExc_"):
            pypy_macros.append('#define _%s _%s' % (name, newname))
        renamed_symbols.append(newname)
    export_symbols[:] = renamed_symbols

    # Generate defines
    for macro_name, size in [
        ("SIZEOF_LONG_LONG", rffi.LONGLONG),
        ("SIZEOF_VOID_P", rffi.VOIDP),
        ("SIZEOF_SIZE_T", rffi.SIZE_T),
        ("SIZEOF_TIME_T", rffi.TIME_T),
        ("SIZEOF_LONG", rffi.LONG),
        ("SIZEOF_SHORT", rffi.SHORT),
        ("SIZEOF_INT", rffi.INT),
        ("SIZEOF_FLOAT", rffi.FLOAT),
        ("SIZEOF_DOUBLE", rffi.DOUBLE),
    ]:
        pypy_macros.append("#define %s %s" % (macro_name, rffi.sizeof(size)))
    pypy_macros.append('')

    pypy_macros_h = udir.join('pypy_macros.h')
    pypy_macros_h.write('\n'.join(pypy_macros))

def generate_decls_and_callbacks(db, export_symbols, api_struct=True):
    "NOT_RPYTHON"
    # implement function callbacks and generate function decls
    functions = []
    pypy_decls = []
    pypy_decls.append("#ifndef _PYPY_PYPY_DECL_H\n")
    pypy_decls.append("#define _PYPY_PYPY_DECL_H\n")
    pypy_decls.append("#ifndef PYPY_STANDALONE\n")
    pypy_decls.append("#ifdef __cplusplus")
    pypy_decls.append("extern \"C\" {")
    pypy_decls.append("#endif\n")
    pypy_decls.append('#define Signed   long           /* xxx temporary fix */\n')
    pypy_decls.append('#define Unsigned unsigned long  /* xxx temporary fix */\n')

    for decl in FORWARD_DECLS:
        pypy_decls.append("%s;" % (decl,))

    for name, func in sorted(FUNCTIONS.iteritems()):
        restype, args = c_function_signature(db, func)
        pypy_decls.append("PyAPI_FUNC(%s) %s(%s);" % (restype, name, args))
        if api_struct:
            callargs = ', '.join('arg%d' % (i,)
                                 for i in range(len(func.argtypes)))
            if func.restype is lltype.Void:
                body = "{ _pypyAPI.%s(%s); }" % (name, callargs)
            else:
                body = "{ return _pypyAPI.%s(%s); }" % (name, callargs)
            functions.append('%s %s(%s)\n%s' % (restype, name, args, body))
    for name in VA_TP_LIST:
        name_no_star = process_va_name(name)
        header = ('%s pypy_va_get_%s(va_list* vp)' %
                  (name, name_no_star))
        pypy_decls.append(header + ';')
        functions.append(header + '\n{return va_arg(*vp, %s);}\n' % name)
        export_symbols.append('pypy_va_get_%s' % (name_no_star,))

    for name, (typ, expr) in GLOBALS.iteritems():
        if name.endswith('#'):
            name = name.replace("#", "")
            typ = typ.replace("*", "")
        elif name.startswith('PyExc_'):
            typ = 'PyObject*'
        pypy_decls.append('PyAPI_DATA(%s) %s;' % (typ, name))

    pypy_decls.append('#undef Signed    /* xxx temporary fix */\n')
    pypy_decls.append('#undef Unsigned  /* xxx temporary fix */\n')
    pypy_decls.append("#ifdef __cplusplus")
    pypy_decls.append("}")
    pypy_decls.append("#endif")
    pypy_decls.append("#endif /*PYPY_STANDALONE*/\n")
    pypy_decls.append("#endif /*_PYPY_PYPY_DECL_H*/\n")

    pypy_decl_h = udir.join('pypy_decl.h')
    pypy_decl_h.write('\n'.join(pypy_decls))
    return functions

def build_eci(building_bridge, export_symbols, code):
    "NOT_RPYTHON"
    # Build code and get pointer to the structure
    kwds = {}
    export_symbols_eci = export_symbols[:]

    compile_extra=['-DPy_BUILD_CORE']

    if building_bridge:
        if sys.platform == "win32":
            # '%s' undefined; assuming extern returning int
            compile_extra.append("/we4013")
            # Sometimes the library is wrapped into another DLL, ensure that
            # the correct bootstrap code is installed
            kwds["link_extra"] = ["msvcrt.lib"]
        elif sys.platform.startswith('linux'):
            compile_extra.append("-Werror=implicit-function-declaration")
            compile_extra.append('-g')
        export_symbols_eci.append('pypyAPI')
    else:
        kwds["includes"] = ['Python.h'] # this is our Python.h

    # Generate definitions for global structures
    structs = ["#include <Python.h>"]
    for name, (typ, expr) in GLOBALS.iteritems():
        if name.endswith('#'):
            structs.append('%s %s;' % (typ[:-1], name[:-1]))
        elif name.startswith('PyExc_'):
            structs.append('extern PyTypeObject _%s;' % (name,))
            structs.append('PyObject* %s = (PyObject*)&_%s;' % (name, name))
        elif typ == 'PyDateTime_CAPI*':
            structs.append('%s %s = NULL;' % (typ, name))
    struct_source = '\n'.join(structs)

    separate_module_sources = [code, struct_source]

    if sys.platform == 'win32':
        get_pythonapi_source = '''
        #include <windows.h>
        HANDLE pypy_get_pythonapi_handle() {
            MEMORY_BASIC_INFORMATION  mi;
            memset(&mi, 0, sizeof(mi));

            if( !VirtualQueryEx(GetCurrentProcess(), &pypy_get_pythonapi_handle,
                                &mi, sizeof(mi)) )
                return 0;

            return (HMODULE)mi.AllocationBase;
        }
        '''
        separate_module_sources.append(get_pythonapi_source)
        export_symbols_eci.append('pypy_get_pythonapi_handle')

    eci = ExternalCompilationInfo(
        include_dirs=include_dirs,
        separate_module_files=[source_dir / "varargwrapper.c",
                               source_dir / "pyerrors.c",
                               source_dir / "modsupport.c",
                               source_dir / "getargs.c",
                               source_dir / "abstract.c",
                               source_dir / "stringobject.c",
                               source_dir / "mysnprintf.c",
                               source_dir / "pythonrun.c",
                               source_dir / "sysmodule.c",
                               source_dir / "bufferobject.c",
                               source_dir / "cobject.c",
                               source_dir / "structseq.c",
                               source_dir / "capsule.c",
                               source_dir / "pysignals.c",
                               source_dir / "pythread.c",
                               source_dir / "ndarrayobject.c",
                               source_dir / "missing.c",
                               ],
        separate_module_sources=separate_module_sources,
        export_symbols=export_symbols_eci,
        compile_extra=compile_extra,
        **kwds
        )

    return eci


def setup_library(space):
    "NOT_RPYTHON"
    from pypy.module.cpyext.pyobject import make_ref

    export_symbols = list(FUNCTIONS) + SYMBOLS_C + list(GLOBALS)
    from rpython.translator.c.database import LowLevelDatabase
    db = LowLevelDatabase()

    generate_macros(export_symbols, prefix='PyPy')

    functions = generate_decls_and_callbacks(db, [], api_struct=False)
    code = "#include <Python.h>\n" + "\n".join(functions)

    eci = build_eci(False, export_symbols, code)

    space.fromcache(State).install_dll(eci)

    run_bootstrap_functions(space)
    setup_va_functions(eci)

    # populate static data
    for name, (typ, expr) in GLOBALS.iteritems():
        name = name.replace("#", "")
        if name.startswith('PyExc_'):
            name = '_' + name
        from pypy.module import cpyext
        w_obj = eval(expr)
        if typ in ('PyObject*', 'PyTypeObject*'):
            struct_ptr = make_ref(space, w_obj)
        elif typ == 'PyDateTime_CAPI*':
            continue
        else:
            assert False, "Unknown static data: %s %s" % (typ, name)
        struct = rffi.cast(get_structtype_for_ctype(typ), struct_ptr)._obj
        struct._compilation_info = eci
        export_struct(name, struct)

    for name, func in FUNCTIONS.iteritems():
        newname = mangle_name('PyPy', name) or name
        deco = entrypoint_lowlevel("cpyext", func.argtypes, newname, relax=True)
        deco(func.get_wrapper(space))

    setup_init_functions(eci, translating=True)
    trunk_include = pypydir.dirpath() / 'include'
    copy_header_files(trunk_include)

initfunctype = lltype.Ptr(lltype.FuncType([], lltype.Void))
@unwrap_spec(path=str, name=str)
def load_extension_module(space, path, name):
    if os.sep not in path:
        path = os.curdir + os.sep + path      # force a '/' in the path
    state = space.fromcache(State)
    if state.find_extension(name, path) is not None:
        return
    old_context = state.package_context
    state.package_context = name, path
    try:
        from rpython.rlib import rdynload
        try:
            ll_libname = rffi.str2charp(path)
            try:
                dll = rdynload.dlopen(ll_libname)
            finally:
                lltype.free(ll_libname, flavor='raw')
        except rdynload.DLOpenError, e:
            raise oefmt(space.w_ImportError,
                        "unable to load extension module '%s': %s",
                        path, e.msg)
        try:
            initptr = rdynload.dlsym(dll, 'init%s' % (name.split('.')[-1],))
        except KeyError:
            raise oefmt(space.w_ImportError,
                        "function init%s not found in library %s", name, path)
        initfunc = rffi.cast(initfunctype, initptr)
        generic_cpy_call(space, initfunc)
        state.check_and_raise_exception()
    finally:
        state.package_context = old_context
    state.fixup_extension(name, path)

@specialize.ll()
def generic_cpy_call(space, func, *args):
    FT = lltype.typeOf(func).TO
    return make_generic_cpy_call(FT, True, False)(space, func, *args)

@specialize.ll()
def generic_cpy_call_dont_decref(space, func, *args):
    FT = lltype.typeOf(func).TO
    return make_generic_cpy_call(FT, False, False)(space, func, *args)

@specialize.ll()
def generic_cpy_call_expect_null(space, func, *args):
    FT = lltype.typeOf(func).TO
    return make_generic_cpy_call(FT, True, True)(space, func, *args)

@specialize.memo()
def make_generic_cpy_call(FT, decref_args, expect_null):
    from pypy.module.cpyext.pyobject import make_ref, from_ref, Py_DecRef
    from pypy.module.cpyext.pyobject import RefcountState
    from pypy.module.cpyext.pyerrors import PyErr_Occurred
    unrolling_arg_types = unrolling_iterable(enumerate(FT.ARGS))
    RESULT_TYPE = FT.RESULT

    # copied and modified from rffi.py
    # We need tons of care to ensure that no GC operation and no
    # exception checking occurs in call_external_function.
    argnames = ', '.join(['a%d' % i for i in range(len(FT.ARGS))])
    source = py.code.Source("""
        def cpy_call_external(funcptr, %(argnames)s):
            # NB. it is essential that no exception checking occurs here!
            res = funcptr(%(argnames)s)
            return res
    """ % locals())
    miniglobals = {'__name__':    __name__, # for module name propagation
                   }
    exec source.compile() in miniglobals
    call_external_function = miniglobals['cpy_call_external']
    call_external_function._dont_inline_ = True
    call_external_function._annspecialcase_ = 'specialize:ll'
    call_external_function._gctransformer_hint_close_stack_ = True
    # don't inline, as a hack to guarantee that no GC pointer is alive
    # anywhere in call_external_function

    @specialize.ll()
    def generic_cpy_call(space, func, *args):
        boxed_args = ()
        to_decref = []
        assert len(args) == len(FT.ARGS)
        for i, ARG in unrolling_arg_types:
            arg = args[i]
            if is_PyObject(ARG):
                if arg is None:
                    boxed_args += (lltype.nullptr(PyObject.TO),)
                elif isinstance(arg, W_Root):
                    ref = make_ref(space, arg)
                    boxed_args += (ref,)
                    if decref_args:
                        to_decref.append(ref)
                else:
                    boxed_args += (arg,)
            else:
                boxed_args += (arg,)

        try:
            # create a new container for borrowed references
            state = space.fromcache(RefcountState)
            old_container = state.swap_borrow_container(None)
            try:
                # Call the function
                result = call_external_function(func, *boxed_args)
            finally:
                state.swap_borrow_container(old_container)

            if is_PyObject(RESULT_TYPE):
                if result is None:
                    ret = result
                elif isinstance(result, W_Root):
                    ret = result
                else:
                    ret = from_ref(space, result)
                    # The object reference returned from a C function
                    # that is called from Python must be an owned reference
                    # - ownership is transferred from the function to its caller.
                    if result:
                        Py_DecRef(space, result)

                # Check for exception consistency
                has_error = PyErr_Occurred(space) is not None
                has_result = ret is not None
                if has_error and has_result:
                    raise OperationError(space.w_SystemError, space.wrap(
                        "An exception was set, but function returned a value"))
                elif not expect_null and not has_error and not has_result:
                    raise OperationError(space.w_SystemError, space.wrap(
                        "Function returned a NULL result without setting an exception"))

                if has_error:
                    state = space.fromcache(State)
                    state.check_and_raise_exception()

                return ret
            return result
        finally:
            if decref_args:
                for ref in to_decref:
                    Py_DecRef(space, ref)
    return generic_cpy_call

