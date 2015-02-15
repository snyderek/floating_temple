from __future__ import with_statement

from pypy.interpreter.error import OperationError

from rpython.rlib import jit
from rpython.rlib.objectmodel import keepalive_until_here, specialize
from rpython.rlib.rarithmetic import r_uint, r_ulonglong
from rpython.rlib.unroll import unrolling_iterable
from rpython.rtyper.lltypesystem import lltype, llmemory, rffi
from rpython.translator.tool.cbuild import ExternalCompilationInfo


# ____________________________________________________________

_prim_signed_types = unrolling_iterable([
    (rffi.SIGNEDCHAR, rffi.SIGNEDCHARP),
    (rffi.SHORT, rffi.SHORTP),
    (rffi.INT, rffi.INTP),
    (rffi.LONG, rffi.LONGP),
    (rffi.LONGLONG, rffi.LONGLONGP)])

_prim_unsigned_types = unrolling_iterable([
    (rffi.UCHAR, rffi.UCHARP),
    (rffi.USHORT, rffi.USHORTP),
    (rffi.UINT, rffi.UINTP),
    (rffi.ULONG, rffi.ULONGP),
    (rffi.ULONGLONG, rffi.ULONGLONGP)])

_prim_float_types = unrolling_iterable([
    (rffi.FLOAT, rffi.FLOATP),
    (rffi.DOUBLE, rffi.DOUBLEP)])

def read_raw_signed_data(target, size):
    for TP, TPP in _prim_signed_types:
        if size == rffi.sizeof(TP):
            return rffi.cast(lltype.SignedLongLong, rffi.cast(TPP, target)[0])
    raise NotImplementedError("bad integer size")

def read_raw_long_data(target, size):
    for TP, TPP in _prim_signed_types:
        if size == rffi.sizeof(TP):
            assert rffi.sizeof(TP) <= rffi.sizeof(lltype.Signed)
            return rffi.cast(lltype.Signed, rffi.cast(TPP, target)[0])
    raise NotImplementedError("bad integer size")

def read_raw_unsigned_data(target, size):
    for TP, TPP in _prim_unsigned_types:
        if size == rffi.sizeof(TP):
            return rffi.cast(lltype.UnsignedLongLong, rffi.cast(TPP, target)[0])
    raise NotImplementedError("bad integer size")

def read_raw_ulong_data(target, size):
    for TP, TPP in _prim_unsigned_types:
        if size == rffi.sizeof(TP):
            assert rffi.sizeof(TP) <= rffi.sizeof(lltype.Unsigned)
            return rffi.cast(lltype.Unsigned, rffi.cast(TPP, target)[0])
    raise NotImplementedError("bad integer size")

@specialize.arg(0)
def _read_raw_float_data_tp(TPP, target):
    # in its own function: FLOAT may make the whole function jit-opaque
    return rffi.cast(lltype.Float, rffi.cast(TPP, target)[0])

def read_raw_float_data(target, size):
    for TP, TPP in _prim_float_types:
        if size == rffi.sizeof(TP):
            return _read_raw_float_data_tp(TPP, target)
    raise NotImplementedError("bad float size")

def read_raw_longdouble_data(target):
    return rffi.cast(rffi.LONGDOUBLEP, target)[0]

@specialize.argtype(1)
def write_raw_unsigned_data(target, source, size):
    for TP, TPP in _prim_unsigned_types:
        if size == rffi.sizeof(TP):
            rffi.cast(TPP, target)[0] = rffi.cast(TP, source)
            return
    raise NotImplementedError("bad integer size")

@specialize.argtype(1)
def write_raw_signed_data(target, source, size):
    for TP, TPP in _prim_signed_types:
        if size == rffi.sizeof(TP):
            rffi.cast(TPP, target)[0] = rffi.cast(TP, source)
            return
    raise NotImplementedError("bad integer size")


@specialize.arg(0, 1)
def _write_raw_float_data_tp(TP, TPP, target, source):
    # in its own function: FLOAT may make the whole function jit-opaque
    rffi.cast(TPP, target)[0] = rffi.cast(TP, source)

def write_raw_float_data(target, source, size):
    for TP, TPP in _prim_float_types:
        if size == rffi.sizeof(TP):
            _write_raw_float_data_tp(TP, TPP, target, source)
            return
    raise NotImplementedError("bad float size")

def write_raw_longdouble_data(target, source):
    rffi.cast(rffi.LONGDOUBLEP, target)[0] = source

# ____________________________________________________________

sprintf_longdouble = rffi.llexternal(
    "sprintf", [rffi.CCHARP, rffi.CCHARP, rffi.LONGDOUBLE], lltype.Void,
    _nowrapper=True, sandboxsafe=True)

FORMAT_LONGDOUBLE = rffi.str2charp("%LE")

def longdouble2str(lvalue):
    with lltype.scoped_alloc(rffi.CCHARP.TO, 128) as p:    # big enough
        sprintf_longdouble(p, FORMAT_LONGDOUBLE, lvalue)
        return rffi.charp2str(p)

# ____________________________________________________________

def _is_a_float(space, w_ob):
    from pypy.module._cffi_backend.cdataobj import W_CData
    from pypy.module._cffi_backend.ctypeprim import W_CTypePrimitiveFloat
    if isinstance(w_ob, W_CData):
        return isinstance(w_ob.ctype, W_CTypePrimitiveFloat)
    return space.isinstance_w(w_ob, space.w_float)

def as_long_long(space, w_ob):
    # (possibly) convert and cast a Python object to a long long.
    # This version accepts a Python int too, and does convertions from
    # other types of objects.  It refuses floats.
    if space.is_w(space.type(w_ob), space.w_int):   # shortcut
        return space.int_w(w_ob)
    try:
        bigint = space.bigint_w(w_ob, allow_conversion=False)
    except OperationError, e:
        if not e.match(space, space.w_TypeError):
            raise
        if _is_a_float(space, w_ob):
            raise
        bigint = space.bigint_w(space.int(w_ob), allow_conversion=False)
    try:
        return bigint.tolonglong()
    except OverflowError:
        raise OperationError(space.w_OverflowError, space.wrap(ovf_msg))

def as_long(space, w_ob):
    # Same as as_long_long(), but returning an int instead.
    if space.is_w(space.type(w_ob), space.w_int):   # shortcut
        return space.int_w(w_ob)
    try:
        bigint = space.bigint_w(w_ob, allow_conversion=False)
    except OperationError, e:
        if not e.match(space, space.w_TypeError):
            raise
        if _is_a_float(space, w_ob):
            raise
        bigint = space.bigint_w(space.int(w_ob), allow_conversion=False)
    try:
        return bigint.toint()
    except OverflowError:
        raise OperationError(space.w_OverflowError, space.wrap(ovf_msg))

def as_unsigned_long_long(space, w_ob, strict):
    # (possibly) convert and cast a Python object to an unsigned long long.
    # This accepts a Python int too, and does convertions from other types of
    # objects.  If 'strict', complains with OverflowError; if 'not strict',
    # mask the result and round floats.
    if space.is_w(space.type(w_ob), space.w_int):   # shortcut
        value = space.int_w(w_ob)
        if strict and value < 0:
            raise OperationError(space.w_OverflowError, space.wrap(neg_msg))
        return r_ulonglong(value)
    try:
        bigint = space.bigint_w(w_ob, allow_conversion=False)
    except OperationError, e:
        if not e.match(space, space.w_TypeError):
            raise
        if strict and _is_a_float(space, w_ob):
            raise
        bigint = space.bigint_w(space.int(w_ob), allow_conversion=False)
    if strict:
        try:
            return bigint.toulonglong()
        except ValueError:
            raise OperationError(space.w_OverflowError, space.wrap(neg_msg))
        except OverflowError:
            raise OperationError(space.w_OverflowError, space.wrap(ovf_msg))
    else:
        return bigint.ulonglongmask()

def as_unsigned_long(space, w_ob, strict):
    # same as as_unsigned_long_long(), but returning just an Unsigned
    if space.is_w(space.type(w_ob), space.w_int):   # shortcut
        value = space.int_w(w_ob)
        if strict and value < 0:
            raise OperationError(space.w_OverflowError, space.wrap(neg_msg))
        return r_uint(value)
    try:
        bigint = space.bigint_w(w_ob, allow_conversion=False)
    except OperationError, e:
        if not e.match(space, space.w_TypeError):
            raise
        if strict and _is_a_float(space, w_ob):
            raise
        bigint = space.bigint_w(space.int(w_ob), allow_conversion=False)
    if strict:
        try:
            return bigint.touint()
        except ValueError:
            raise OperationError(space.w_OverflowError, space.wrap(neg_msg))
        except OverflowError:
            raise OperationError(space.w_OverflowError, space.wrap(ovf_msg))
    else:
        return bigint.uintmask()

neg_msg = "can't convert negative number to unsigned"
ovf_msg = "long too big to convert"

# ____________________________________________________________

class _NotStandardObject(Exception):
    pass

def _standard_object_as_bool(space, w_ob):
    if space.isinstance_w(w_ob, space.w_int):
        return space.int_w(w_ob) != 0
    if space.isinstance_w(w_ob, space.w_long):
        return space.bigint_w(w_ob).tobool()
    if space.isinstance_w(w_ob, space.w_float):
        return space.float_w(w_ob) != 0.0
    raise _NotStandardObject

# hackish, but the most straightforward way to know if a LONGDOUBLE object
# contains the value 0 or not.
eci = ExternalCompilationInfo(post_include_bits=["""
#define pypy__is_nonnull_longdouble(x)  ((x) != 0.0)
"""])
_is_nonnull_longdouble = rffi.llexternal(
    "pypy__is_nonnull_longdouble", [rffi.LONGDOUBLE], lltype.Bool,
    compilation_info=eci, _nowrapper=True, elidable_function=True,
    sandboxsafe=True)

# split here for JIT backends that don't support floats/longlongs/etc.
def is_nonnull_longdouble(cdata):
    return _is_nonnull_longdouble(read_raw_longdouble_data(cdata))
def is_nonnull_float(cdata, size):
    return read_raw_float_data(cdata, size) != 0.0

def object_as_bool(space, w_ob):
    # convert and cast a Python object to a boolean.  Accept an integer
    # or a float object, up to a CData 'long double'.
    try:
        return _standard_object_as_bool(space, w_ob)
    except _NotStandardObject:
        pass
    #
    from pypy.module._cffi_backend.cdataobj import W_CData
    from pypy.module._cffi_backend.ctypeprim import W_CTypePrimitiveFloat
    from pypy.module._cffi_backend.ctypeprim import W_CTypePrimitiveLongDouble
    is_cdata = isinstance(w_ob, W_CData)
    if is_cdata and isinstance(w_ob.ctype, W_CTypePrimitiveFloat):
        if isinstance(w_ob.ctype, W_CTypePrimitiveLongDouble):
            result = is_nonnull_longdouble(w_ob._cdata)
        else:
            result = is_nonnull_float(w_ob._cdata, w_ob.ctype.size)
        keepalive_until_here(w_ob)
        return result
    #
    if not is_cdata and space.lookup(w_ob, '__float__') is not None:
        w_io = space.float(w_ob)
    else:
        w_io = space.int(w_ob)
    try:
        return _standard_object_as_bool(space, w_io)
    except _NotStandardObject:
        raise OperationError(space.w_TypeError,
                             space.wrap("integer/float expected"))

# ____________________________________________________________

def get_new_array_length(space, w_value):
    if (space.isinstance_w(w_value, space.w_list) or
        space.isinstance_w(w_value, space.w_tuple)):
        return (w_value, space.int_w(space.len(w_value)))
    elif space.isinstance_w(w_value, space.w_basestring):
        # from a string, we add the null terminator
        return (w_value, space.int_w(space.len(w_value)) + 1)
    else:
        explicitlength = space.getindex_w(w_value, space.w_OverflowError)
        if explicitlength < 0:
            raise OperationError(space.w_ValueError,
                                 space.wrap("negative array length"))
        return (space.w_None, explicitlength)

# ____________________________________________________________

@specialize.arg(0)
def _raw_memcopy_tp(TPP, source, dest):
    # in its own function: LONGLONG may make the whole function jit-opaque
    rffi.cast(TPP, dest)[0] = rffi.cast(TPP, source)[0]

def _raw_memcopy(source, dest, size):
    if jit.isconstant(size):
        # for the JIT: first handle the case where 'size' is known to be
        # a constant equal to 1, 2, 4, 8
        for TP, TPP in _prim_unsigned_types:
            if size == rffi.sizeof(TP):
                _raw_memcopy_tp(TPP, source, dest)
                return
    _raw_memcopy_opaque(source, dest, size)

@jit.dont_look_inside
def _raw_memcopy_opaque(source, dest, size):
    # push push push at the llmemory interface (with hacks that are all
    # removed after translation)
    zero = llmemory.itemoffsetof(rffi.CCHARP.TO, 0)
    llmemory.raw_memcopy(
        llmemory.cast_ptr_to_adr(source) + zero,
        llmemory.cast_ptr_to_adr(dest) + zero,
        size * llmemory.sizeof(lltype.Char))

@specialize.arg(0, 1)
def _raw_memclear_tp(TP, TPP, dest):
    # in its own function: LONGLONG may make the whole function jit-opaque
    rffi.cast(TPP, dest)[0] = rffi.cast(TP, 0)

def _raw_memclear(dest, size):
    # for now, only supports the cases of size = 1, 2, 4, 8
    for TP, TPP in _prim_unsigned_types:
        if size == rffi.sizeof(TP):
            _raw_memclear_tp(TP, TPP, dest)
            return
    raise NotImplementedError("bad clear size")

# ____________________________________________________________

def pack_list_to_raw_array_bounds(int_list, target, size, vmin, vrangemax):
    for TP, TPP in _prim_signed_types:
        if size == rffi.sizeof(TP):
            ptr = rffi.cast(TPP, target)
            for i in range(len(int_list)):
                x = int_list[i]
                if r_uint(x) - vmin > vrangemax:
                    return x      # overflow
                ptr[i] = rffi.cast(TP, x)
            return 0
    raise NotImplementedError("bad integer size")

@specialize.arg(2)
def pack_float_list_to_raw_array(float_list, target, TP, TPP):
    target = rffi.cast(TPP, target)
    for i in range(len(float_list)):
        x = float_list[i]
        target[i] = rffi.cast(TP, x)

def unpack_list_from_raw_array(int_list, source, size):
    for TP, TPP in _prim_signed_types:
        if size == rffi.sizeof(TP):
            ptr = rffi.cast(TPP, source)
            for i in range(len(int_list)):
                int_list[i] = rffi.cast(lltype.Signed, ptr[i])
            return
    raise NotImplementedError("bad integer size")

def unpack_unsigned_list_from_raw_array(int_list, source, size):
    for TP, TPP in _prim_unsigned_types:
        if size == rffi.sizeof(TP):
            ptr = rffi.cast(TPP, source)
            for i in range(len(int_list)):
                int_list[i] = rffi.cast(lltype.Signed, ptr[i])
            return
    raise NotImplementedError("bad integer size")

def unpack_cfloat_list_from_raw_array(float_list, source):
    ptr = rffi.cast(rffi.FLOATP, source)
    for i in range(len(float_list)):
        float_list[i] = rffi.cast(lltype.Float, ptr[i])
