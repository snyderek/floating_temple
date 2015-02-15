import string
from pypy.interpreter.argument import Arguments
from pypy.interpreter.baseobjspace import W_Root
from pypy.interpreter.error import OperationError, oefmt
from pypy.interpreter.gateway import interp2app, unwrap_spec
from pypy.interpreter.typedef import (TypeDef, GetSetProperty,
                                      interp_attrproperty, interp_attrproperty_w)
from rpython.rlib import jit
from rpython.rlib.objectmodel import specialize, compute_hash
from rpython.rlib.rarithmetic import r_longlong, r_ulonglong
from pypy.module.micronumpy import types, boxes, base, support, constants as NPY
from pypy.module.micronumpy.appbridge import get_appbridge_cache
from pypy.module.micronumpy.converters import byteorder_converter


def decode_w_dtype(space, w_dtype):
    if space.is_none(w_dtype):
        return None
    return space.interp_w(
        W_Dtype, space.call_function(space.gettypefor(W_Dtype), w_dtype))


@jit.unroll_safe
def dtype_agreement(space, w_arr_list, shape, out=None):
    """ agree on dtype from a list of arrays. if out is allocated,
    use it's dtype, otherwise allocate a new one with agreed dtype
    """
    from pypy.module.micronumpy.ufuncs import find_binop_result_dtype

    if not space.is_none(out):
        return out
    dtype = None
    for w_arr in w_arr_list:
        if not space.is_none(w_arr):
            dtype = find_binop_result_dtype(space, dtype, w_arr.get_dtype())
    assert dtype is not None
    out = base.W_NDimArray.from_shape(space, shape, dtype)
    return out


class W_Dtype(W_Root):
    _immutable_fields_ = [
        "itemtype?", "num", "kind", "char", "w_box_type",
        "byteorder?", "names?", "fields?", "elsize?", "alignment?",
        "shape?", "subdtype?", "base?",
    ]

    def __init__(self, itemtype, num, kind, char, w_box_type,
                 byteorder=None, names=[], fields={},
                 elsize=None, shape=[], subdtype=None):
        self.itemtype = itemtype
        self.num = num
        self.kind = kind
        self.char = char
        self.w_box_type = w_box_type
        if byteorder is None:
            if itemtype.get_element_size() == 1:
                byteorder = NPY.IGNORE
            else:
                byteorder = NPY.NATIVE
        self.byteorder = byteorder
        self.names = names
        self.fields = fields
        if elsize is None:
            elsize = itemtype.get_element_size()
        self.elsize = elsize
        self.alignment = itemtype.alignment
        self.shape = shape
        self.subdtype = subdtype
        if not subdtype:
            self.base = self
        else:
            self.base = subdtype.base

    def __repr__(self):
        if self.fields:
            return '<DType %r>' % self.fields
        return '<DType %r>' % self.itemtype

    @specialize.argtype(1)
    def box(self, value):
        return self.itemtype.box(value)

    @specialize.argtype(1, 2)
    def box_complex(self, real, imag):
        return self.itemtype.box_complex(real, imag)

    def coerce(self, space, w_item):
        return self.itemtype.coerce(space, self, w_item)

    def is_bool(self):
        return self.kind == NPY.GENBOOLLTR

    def is_signed(self):
        return self.kind == NPY.SIGNEDLTR

    def is_unsigned(self):
        return self.kind == NPY.UNSIGNEDLTR

    def is_int(self):
        return (self.kind == NPY.SIGNEDLTR or self.kind == NPY.UNSIGNEDLTR or
                self.kind == NPY.GENBOOLLTR)

    def is_float(self):
        return self.kind == NPY.FLOATINGLTR

    def is_complex(self):
        return self.kind == NPY.COMPLEXLTR

    def is_str(self):
        return self.num == NPY.STRING

    def is_str_or_unicode(self):
        return self.num == NPY.STRING or self.num == NPY.UNICODE

    def is_flexible(self):
        return self.is_str_or_unicode() or self.num == NPY.VOID

    def is_record(self):
        return bool(self.fields)

    def is_native(self):
        return self.byteorder in (NPY.NATIVE, NPY.NATBYTE)

    def get_float_dtype(self, space):
        assert self.is_complex()
        dtype = get_dtype_cache(space).component_dtypes[self.num]
        if self.byteorder == NPY.OPPBYTE:
            dtype = dtype.descr_newbyteorder(space)
        assert dtype.is_float()
        return dtype

    def get_name(self):
        name = self.w_box_type.name
        if name.startswith('numpy.'):
            name = name[6:]
        if name.endswith('_'):
            name = name[:-1]
        return name

    def descr_get_name(self, space):
        name = self.get_name()
        if self.is_flexible() and self.elsize != 0:
            return space.wrap(name + str(self.elsize * 8))
        return space.wrap(name)

    def descr_get_str(self, space):
        basic = self.kind
        endian = self.byteorder
        size = self.elsize
        if endian == NPY.NATIVE:
            endian = NPY.NATBYTE
        if self.num == NPY.UNICODE:
            size >>= 2
        return space.wrap("%s%s%s" % (endian, basic, size))

    def descr_get_descr(self, space):
        if not self.is_record():
            return space.newlist([space.newtuple([space.wrap(""),
                                                  self.descr_get_str(space)])])
        else:
            descr = []
            for name in self.names:
                subdtype = self.fields[name][1]
                subdescr = [space.wrap(name)]
                if subdtype.is_record():
                    subdescr.append(subdtype.descr_get_descr(space))
                elif subdtype.subdtype is not None:
                    subdescr.append(subdtype.subdtype.descr_get_str(space))
                else:
                    subdescr.append(subdtype.descr_get_str(space))
                if subdtype.shape != []:
                    subdescr.append(subdtype.descr_get_shape(space))
                descr.append(space.newtuple(subdescr[:]))
            return space.newlist(descr)

    def descr_get_hasobject(self, space):
        return space.w_False

    def descr_get_isbuiltin(self, space):
        if self.fields is None:
            return space.wrap(1)
        return space.wrap(0)

    def descr_get_isnative(self, space):
        return space.wrap(self.is_native())

    def descr_get_base(self, space):
        return space.wrap(self.base)

    def descr_get_subdtype(self, space):
        if self.subdtype is None:
            return space.w_None
        return space.newtuple([space.wrap(self.subdtype),
                               self.descr_get_shape(space)])

    def descr_get_shape(self, space):
        return space.newtuple([space.wrap(dim) for dim in self.shape])

    def descr_get_fields(self, space):
        if not self.fields:
            return space.w_None
        w_fields = space.newdict()
        for name, (offset, subdtype) in self.fields.iteritems():
            space.setitem(w_fields, space.wrap(name),
                          space.newtuple([subdtype, space.wrap(offset)]))
        return w_fields

    def descr_get_names(self, space):
        if not self.fields:
            return space.w_None
        return space.newtuple([space.wrap(name) for name in self.names])

    def descr_set_names(self, space, w_names):
        if not self.fields:
            raise oefmt(space.w_ValueError, "there are no fields defined")
        if not space.issequence_w(w_names) or \
                space.len_w(w_names) != len(self.names):
            raise oefmt(space.w_ValueError,
                        "must replace all names at once "
                        "with a sequence of length %d",
                        len(self.names))
        names = []
        for w_name in space.fixedview(w_names):
            if not space.isinstance_w(w_name, space.w_str):
                raise oefmt(space.w_ValueError,
                            "item #%d of names is of type %T and not string",
                            len(names), w_name)
            names.append(space.str_w(w_name))
        fields = {}
        for i in range(len(self.names)):
            if names[i] in fields:
                raise oefmt(space.w_ValueError, "Duplicate field names given.")
            fields[names[i]] = self.fields[self.names[i]]
        self.fields = fields
        self.names = names

    def descr_del_names(self, space):
        raise OperationError(space.w_AttributeError, space.wrap(
            "Cannot delete dtype names attribute"))

    def eq(self, space, w_other):
        w_other = space.call_function(space.gettypefor(W_Dtype), w_other)
        if space.is_w(self, w_other):
            return True
        if isinstance(w_other, W_Dtype):
            return space.eq_w(self.descr_reduce(space),
                              w_other.descr_reduce(space))
        return False

    def descr_eq(self, space, w_other):
        return space.wrap(self.eq(space, w_other))

    def descr_ne(self, space, w_other):
        return space.wrap(not self.eq(space, w_other))

    def _compute_hash(self, space, x):
        from rpython.rlib.rarithmetic import intmask
        if not self.fields and self.subdtype is None:
            endian = self.byteorder
            if endian == NPY.NATIVE:
                endian = NPY.NATBYTE
            flags = 0
            y = 0x345678
            y = intmask((1000003 * y) ^ ord(self.kind[0]))
            y = intmask((1000003 * y) ^ ord(endian[0]))
            y = intmask((1000003 * y) ^ flags)
            y = intmask((1000003 * y) ^ self.elsize)
            if self.is_flexible():
                y = intmask((1000003 * y) ^ self.alignment)
            return intmask((1000003 * x) ^ y)
        if self.fields:
            for name, (offset, subdtype) in self.fields.iteritems():
                assert isinstance(subdtype, W_Dtype)
                y = intmask(1000003 * (0x345678 ^ compute_hash(name)))
                y = intmask(1000003 * (y ^ compute_hash(offset)))
                y = intmask(1000003 * (y ^ subdtype._compute_hash(space,
                                                                 0x345678)))
                x = intmask(x ^ y)
        if self.subdtype is not None:
            for s in self.shape:
                x = intmask((1000003 * x) ^ compute_hash(s))
            x = self.base._compute_hash(space, x)
        return x

    def descr_hash(self, space):
        return space.wrap(self._compute_hash(space, 0x345678))


    def descr_str(self, space):
        if self.fields:
            return space.str(self.descr_get_descr(space))
        elif self.subdtype is not None:
            return space.str(space.newtuple([
                self.subdtype.descr_get_str(space),
                self.descr_get_shape(space)]))
        else:
            if self.is_flexible():
                return self.descr_get_str(space)
            else:
                return self.descr_get_name(space)

    def descr_repr(self, space):
        if self.fields:
            r = self.descr_get_descr(space)
        elif self.subdtype is not None:
            r = space.newtuple([self.subdtype.descr_get_str(space),
                                self.descr_get_shape(space)])
        else:
            if self.is_flexible():
                if self.byteorder != NPY.IGNORE:
                    byteorder = NPY.NATBYTE if self.is_native() else NPY.OPPBYTE
                else:
                    byteorder = ''
                size = self.elsize
                if self.num == NPY.UNICODE:
                    size >>= 2
                r = space.wrap(byteorder + self.char + str(size))
            else:
                r = self.descr_get_name(space)
        return space.wrap("dtype(%s)" % space.str_w(space.repr(r)))

    def descr_getitem(self, space, w_item):
        if not self.fields:
            raise oefmt(space.w_KeyError, "There are no fields in dtype %s.",
                        self.get_name())
        if space.isinstance_w(w_item, space.w_basestring):
            item = space.str_w(w_item)
        elif space.isinstance_w(w_item, space.w_int):
            indx = space.int_w(w_item)
            try:
                item = self.names[indx]
            except IndexError:
                raise oefmt(space.w_IndexError,
                    "Field index %d out of range.", indx)
        else:
            raise oefmt(space.w_ValueError,
                "Field key must be an integer, string, or unicode.")
        try:
            return self.fields[item][1]
        except KeyError:
            raise oefmt(space.w_KeyError,
                "Field named '%s' not found.", item)

    def descr_len(self, space):
        if not self.fields:
            return space.wrap(0)
        return space.wrap(len(self.fields))

    def descr_reduce(self, space):
        w_class = space.type(self)
        builder_args = space.newtuple([
            space.wrap("%s%d" % (self.kind, self.elsize)),
            space.wrap(0), space.wrap(1)])

        version = space.wrap(3)
        endian = self.byteorder
        if endian == NPY.NATIVE:
            endian = NPY.NATBYTE
        subdescr = self.descr_get_subdtype(space)
        names = self.descr_get_names(space)
        values = self.descr_get_fields(space)
        if self.is_flexible():
            w_size = space.wrap(self.elsize)
            alignment = space.wrap(self.alignment)
        else:
            w_size = space.wrap(-1)
            alignment = space.wrap(-1)
        flags = space.wrap(0)

        data = space.newtuple([version, space.wrap(endian), subdescr,
                               names, values, w_size, alignment, flags])
        return space.newtuple([w_class, builder_args, data])

    def descr_setstate(self, space, w_data):
        if self.fields is None:  # if builtin dtype
            return space.w_None

        version = space.int_w(space.getitem(w_data, space.wrap(0)))
        if version != 3:
            raise oefmt(space.w_ValueError,
                        "can't handle version %d of numpy.dtype pickle",
                        version)

        endian = space.str_w(space.getitem(w_data, space.wrap(1)))
        if endian == NPY.NATBYTE:
            endian = NPY.NATIVE

        w_subarray = space.getitem(w_data, space.wrap(2))
        w_names = space.getitem(w_data, space.wrap(3))
        w_fields = space.getitem(w_data, space.wrap(4))
        size = space.int_w(space.getitem(w_data, space.wrap(5)))
        alignment = space.int_w(space.getitem(w_data, space.wrap(6)))

        if (w_names == space.w_None) != (w_fields == space.w_None):
            raise oefmt(space.w_ValueError, "inconsistent fields and names")

        self.byteorder = endian
        self.shape = []
        self.subdtype = None
        self.base = self

        if w_subarray != space.w_None:
            if not space.isinstance_w(w_subarray, space.w_tuple) or \
                    space.len_w(w_subarray) != 2:
                raise oefmt(space.w_ValueError,
                            "incorrect subarray in __setstate__")
            subdtype, w_shape = space.fixedview(w_subarray)
            assert isinstance(subdtype, W_Dtype)
            if not support.issequence_w(space, w_shape):
                self.shape = [space.int_w(w_shape)]
            else:
                self.shape = [space.int_w(w_s) for w_s in space.fixedview(w_shape)]
            self.subdtype = subdtype
            self.base = subdtype.base

        if w_names != space.w_None:
            self.names = []
            self.fields = {}
            for w_name in space.fixedview(w_names):
                name = space.str_w(w_name)
                value = space.getitem(w_fields, w_name)

                dtype = space.getitem(value, space.wrap(0))
                assert isinstance(dtype, W_Dtype)
                offset = space.int_w(space.getitem(value, space.wrap(1)))

                self.names.append(name)
                self.fields[name] = offset, dtype
            self.itemtype = types.RecordType()

        if self.is_flexible():
            self.elsize = size
            self.alignment = alignment

    @unwrap_spec(new_order=str)
    def descr_newbyteorder(self, space, new_order=NPY.SWAP):
        newendian = byteorder_converter(space, new_order)
        endian = self.byteorder
        if endian != NPY.IGNORE:
            if newendian == NPY.SWAP:
                endian = NPY.OPPBYTE if self.is_native() else NPY.NATBYTE
            elif newendian != NPY.IGNORE:
                endian = newendian
        itemtype = self.itemtype.__class__(endian in (NPY.NATIVE, NPY.NATBYTE))
        fields = self.fields
        if fields is None:
            fields = {}
        return W_Dtype(itemtype, self.num, self.kind, self.char,
                       self.w_box_type, byteorder=endian, elsize=self.elsize,
                       names=self.names, fields=fields,
                       shape=self.shape, subdtype=self.subdtype)


@specialize.arg(2)
def dtype_from_list(space, w_lst, simple):
    lst_w = space.listview(w_lst)
    fields = {}
    offset = 0
    names = []
    for i in range(len(lst_w)):
        w_elem = lst_w[i]
        if simple:
            subdtype = descr__new__(space, space.gettypefor(W_Dtype), w_elem)
            fldname = 'f%d' % i
        else:
            w_shape = space.newtuple([])
            if space.len_w(w_elem) == 3:
                w_fldname, w_flddesc, w_shape = space.fixedview(w_elem)
                if not support.issequence_w(space, w_shape):
                    w_shape = space.newtuple([w_shape])
            else:
                w_fldname, w_flddesc = space.fixedview(w_elem, 2)
            subdtype = descr__new__(space, space.gettypefor(W_Dtype), w_flddesc, w_shape=w_shape)
            fldname = space.str_w(w_fldname)
            if fldname == '':
                fldname = 'f%d' % i
            if fldname in fields:
                raise oefmt(space.w_ValueError, "two fields with the same name")
        assert isinstance(subdtype, W_Dtype)
        fields[fldname] = (offset, subdtype)
        offset += subdtype.elsize
        names.append(fldname)
    return W_Dtype(types.RecordType(), NPY.VOID, NPY.VOIDLTR, NPY.VOIDLTR,
                   space.gettypefor(boxes.W_VoidBox),
                   names=names, fields=fields, elsize=offset)


def dtype_from_dict(space, w_dict):
    raise OperationError(space.w_NotImplementedError, space.wrap(
        "dtype from dict"))


def dtype_from_spec(space, w_spec):
    w_lst = get_appbridge_cache(space).call_method(space,
        'numpy.core._internal', '_commastring', Arguments(space, [w_spec]))
    if not space.isinstance_w(w_lst, space.w_list) or space.len_w(w_lst) < 1:
        raise oefmt(space.w_RuntimeError,
                    "_commastring is not returning a list with len >= 1")
    if space.len_w(w_lst) == 1:
        return descr__new__(space, space.gettypefor(W_Dtype),
                            space.getitem(w_lst, space.wrap(0)))
    else:
        return dtype_from_list(space, w_lst, True)


def _check_for_commastring(s):
    if s[0] in string.digits or s[0] in '<>=|' and s[1] in string.digits:
        return True
    if s[0] == '(' and s[1] == ')' or s[0] in '<>=|' and s[1] == '(' and s[2] == ')':
        return True
    sqbracket = 0
    for c in s:
        if c == ',':
            if sqbracket == 0:
                return True
        elif c == '[':
            sqbracket += 1
        elif c == ']':
            sqbracket -= 1
    return False


def descr__new__(space, w_subtype, w_dtype, w_align=None, w_copy=None, w_shape=None):
    # w_align and w_copy are necessary for pickling
    cache = get_dtype_cache(space)

    if w_shape is not None and (space.isinstance_w(w_shape, space.w_int) or
                                space.len_w(w_shape) > 0):
        subdtype = descr__new__(space, w_subtype, w_dtype, w_align, w_copy)
        assert isinstance(subdtype, W_Dtype)
        size = 1
        if space.isinstance_w(w_shape, space.w_int):
            w_shape = space.newtuple([w_shape])
        shape = []
        for w_dim in space.fixedview(w_shape):
            dim = space.int_w(w_dim)
            shape.append(dim)
            size *= dim
        if size == 1:
            return subdtype
        size *= subdtype.elsize
        return W_Dtype(types.VoidType(), NPY.VOID, NPY.VOIDLTR, NPY.VOIDLTR,
                       space.gettypefor(boxes.W_VoidBox),
                       shape=shape, subdtype=subdtype, elsize=size)

    if space.is_none(w_dtype):
        return cache.w_float64dtype
    elif space.isinstance_w(w_dtype, w_subtype):
        return w_dtype
    elif space.isinstance_w(w_dtype, space.w_str):
        name = space.str_w(w_dtype)
        if _check_for_commastring(name):
            return dtype_from_spec(space, w_dtype)
        cname = name[1:] if name[0] == NPY.OPPBYTE else name
        try:
            dtype = cache.dtypes_by_name[cname]
        except KeyError:
            pass
        else:
            if name[0] == NPY.OPPBYTE:
                dtype = dtype.descr_newbyteorder(space)
            return dtype
        if name[0] in 'VSUca' or name[0] in '<>=|' and name[1] in 'VSUca':
            return variable_dtype(space, name)
        raise oefmt(space.w_TypeError, 'data type "%s" not understood', name)
    elif space.isinstance_w(w_dtype, space.w_list):
        return dtype_from_list(space, w_dtype, False)
    elif space.isinstance_w(w_dtype, space.w_tuple):
        w_dtype0 = space.getitem(w_dtype, space.wrap(0))
        w_dtype1 = space.getitem(w_dtype, space.wrap(1))
        subdtype = descr__new__(space, w_subtype, w_dtype0, w_align, w_copy)
        assert isinstance(subdtype, W_Dtype)
        if subdtype.elsize == 0:
            name = "%s%d" % (subdtype.kind, space.int_w(w_dtype1))
            return descr__new__(space, w_subtype, space.wrap(name), w_align, w_copy)
        return descr__new__(space, w_subtype, w_dtype0, w_align, w_copy, w_shape=w_dtype1)
    elif space.isinstance_w(w_dtype, space.w_dict):
        return dtype_from_dict(space, w_dtype)
    for dtype in cache.builtin_dtypes:
        if dtype.num in cache.alternate_constructors and \
                w_dtype in cache.alternate_constructors[dtype.num]:
            return dtype
        if w_dtype is dtype.w_box_type:
            return dtype
    if space.isinstance_w(w_dtype, space.w_type):
        raise oefmt(space.w_NotImplementedError,
            "cannot create dtype with type '%N'", w_dtype)
    raise oefmt(space.w_TypeError, "data type not understood")


W_Dtype.typedef = TypeDef("numpy.dtype",
    __new__ = interp2app(descr__new__),

    type = interp_attrproperty_w("w_box_type", cls=W_Dtype),
    kind = interp_attrproperty("kind", cls=W_Dtype),
    char = interp_attrproperty("char", cls=W_Dtype),
    num = interp_attrproperty("num", cls=W_Dtype),
    byteorder = interp_attrproperty("byteorder", cls=W_Dtype),
    itemsize = interp_attrproperty("elsize", cls=W_Dtype),
    alignment = interp_attrproperty("alignment", cls=W_Dtype),

    name = GetSetProperty(W_Dtype.descr_get_name),
    str = GetSetProperty(W_Dtype.descr_get_str),
    descr = GetSetProperty(W_Dtype.descr_get_descr),
    hasobject = GetSetProperty(W_Dtype.descr_get_hasobject),
    isbuiltin = GetSetProperty(W_Dtype.descr_get_isbuiltin),
    isnative = GetSetProperty(W_Dtype.descr_get_isnative),
    base = GetSetProperty(W_Dtype.descr_get_base),
    subdtype = GetSetProperty(W_Dtype.descr_get_subdtype),
    shape = GetSetProperty(W_Dtype.descr_get_shape),
    fields = GetSetProperty(W_Dtype.descr_get_fields),
    names = GetSetProperty(W_Dtype.descr_get_names,
                           W_Dtype.descr_set_names,
                           W_Dtype.descr_del_names),

    __eq__ = interp2app(W_Dtype.descr_eq),
    __ne__ = interp2app(W_Dtype.descr_ne),
    __hash__ = interp2app(W_Dtype.descr_hash),
    __str__= interp2app(W_Dtype.descr_str),
    __repr__ = interp2app(W_Dtype.descr_repr),
    __getitem__ = interp2app(W_Dtype.descr_getitem),
    __len__ = interp2app(W_Dtype.descr_len),
    __reduce__ = interp2app(W_Dtype.descr_reduce),
    __setstate__ = interp2app(W_Dtype.descr_setstate),
    newbyteorder = interp2app(W_Dtype.descr_newbyteorder),
)
W_Dtype.typedef.acceptable_as_base_class = False


def variable_dtype(space, name):
    if name[0] in '<>=|':
        name = name[1:]
    char = name[0]
    if len(name) == 1:
        size = 0
    else:
        try:
            size = int(name[1:])
        except ValueError:
            raise oefmt(space.w_TypeError, "data type not understood")
    if char == NPY.CHARLTR:
        return new_string_dtype(space, 1, NPY.CHARLTR)
    elif char == NPY.STRINGLTR or char == NPY.STRINGLTR2:
        return new_string_dtype(space, size)
    elif char == NPY.UNICODELTR:
        return new_unicode_dtype(space, size)
    elif char == NPY.VOIDLTR:
        return new_void_dtype(space, size)
    assert False


def new_string_dtype(space, size, char=NPY.STRINGLTR):
    return W_Dtype(
        types.StringType(),
        elsize=size,
        num=NPY.STRING,
        kind=NPY.STRINGLTR,
        char=char,
        w_box_type=space.gettypefor(boxes.W_StringBox),
    )


def new_unicode_dtype(space, size):
    itemtype = types.UnicodeType()
    return W_Dtype(
        itemtype,
        elsize=size * itemtype.get_element_size(),
        num=NPY.UNICODE,
        kind=NPY.UNICODELTR,
        char=NPY.UNICODELTR,
        w_box_type=space.gettypefor(boxes.W_UnicodeBox),
    )


def new_void_dtype(space, size):
    return W_Dtype(
        types.VoidType(),
        elsize=size,
        num=NPY.VOID,
        kind=NPY.VOIDLTR,
        char=NPY.VOIDLTR,
        w_box_type=space.gettypefor(boxes.W_VoidBox),
    )


class DtypeCache(object):
    def __init__(self, space):
        self.w_booldtype = W_Dtype(
            types.Bool(),
            num=NPY.BOOL,
            kind=NPY.GENBOOLLTR,
            char=NPY.BOOLLTR,
            w_box_type=space.gettypefor(boxes.W_BoolBox),
        )
        self.w_int8dtype = W_Dtype(
            types.Int8(),
            num=NPY.BYTE,
            kind=NPY.SIGNEDLTR,
            char=NPY.BYTELTR,
            w_box_type=space.gettypefor(boxes.W_Int8Box),
        )
        self.w_uint8dtype = W_Dtype(
            types.UInt8(),
            num=NPY.UBYTE,
            kind=NPY.UNSIGNEDLTR,
            char=NPY.UBYTELTR,
            w_box_type=space.gettypefor(boxes.W_UInt8Box),
        )
        self.w_int16dtype = W_Dtype(
            types.Int16(),
            num=NPY.SHORT,
            kind=NPY.SIGNEDLTR,
            char=NPY.SHORTLTR,
            w_box_type=space.gettypefor(boxes.W_Int16Box),
        )
        self.w_uint16dtype = W_Dtype(
            types.UInt16(),
            num=NPY.USHORT,
            kind=NPY.UNSIGNEDLTR,
            char=NPY.USHORTLTR,
            w_box_type=space.gettypefor(boxes.W_UInt16Box),
        )
        self.w_int32dtype = W_Dtype(
            types.Int32(),
            num=NPY.INT,
            kind=NPY.SIGNEDLTR,
            char=NPY.INTLTR,
            w_box_type=space.gettypefor(boxes.W_Int32Box),
        )
        self.w_uint32dtype = W_Dtype(
            types.UInt32(),
            num=NPY.UINT,
            kind=NPY.UNSIGNEDLTR,
            char=NPY.UINTLTR,
            w_box_type=space.gettypefor(boxes.W_UInt32Box),
        )
        self.w_longdtype = W_Dtype(
            types.Long(),
            num=NPY.LONG,
            kind=NPY.SIGNEDLTR,
            char=NPY.LONGLTR,
            w_box_type=space.gettypefor(boxes.W_LongBox),
        )
        self.w_ulongdtype = W_Dtype(
            types.ULong(),
            num=NPY.ULONG,
            kind=NPY.UNSIGNEDLTR,
            char=NPY.ULONGLTR,
            w_box_type=space.gettypefor(boxes.W_ULongBox),
        )
        self.w_int64dtype = W_Dtype(
            types.Int64(),
            num=NPY.LONGLONG,
            kind=NPY.SIGNEDLTR,
            char=NPY.LONGLONGLTR,
            w_box_type=space.gettypefor(boxes.W_Int64Box),
        )
        self.w_uint64dtype = W_Dtype(
            types.UInt64(),
            num=NPY.ULONGLONG,
            kind=NPY.UNSIGNEDLTR,
            char=NPY.ULONGLONGLTR,
            w_box_type=space.gettypefor(boxes.W_UInt64Box),
        )
        self.w_float32dtype = W_Dtype(
            types.Float32(),
            num=NPY.FLOAT,
            kind=NPY.FLOATINGLTR,
            char=NPY.FLOATLTR,
            w_box_type=space.gettypefor(boxes.W_Float32Box),
        )
        self.w_float64dtype = W_Dtype(
            types.Float64(),
            num=NPY.DOUBLE,
            kind=NPY.FLOATINGLTR,
            char=NPY.DOUBLELTR,
            w_box_type=space.gettypefor(boxes.W_Float64Box),
        )
        self.w_floatlongdtype = W_Dtype(
            types.FloatLong(),
            num=NPY.LONGDOUBLE,
            kind=NPY.FLOATINGLTR,
            char=NPY.LONGDOUBLELTR,
            w_box_type=space.gettypefor(boxes.W_FloatLongBox),
        )
        self.w_complex64dtype = W_Dtype(
            types.Complex64(),
            num=NPY.CFLOAT,
            kind=NPY.COMPLEXLTR,
            char=NPY.CFLOATLTR,
            w_box_type=space.gettypefor(boxes.W_Complex64Box),
        )
        self.w_complex128dtype = W_Dtype(
            types.Complex128(),
            num=NPY.CDOUBLE,
            kind=NPY.COMPLEXLTR,
            char=NPY.CDOUBLELTR,
            w_box_type=space.gettypefor(boxes.W_Complex128Box),
        )
        self.w_complexlongdtype = W_Dtype(
            types.ComplexLong(),
            num=NPY.CLONGDOUBLE,
            kind=NPY.COMPLEXLTR,
            char=NPY.CLONGDOUBLELTR,
            w_box_type=space.gettypefor(boxes.W_ComplexLongBox),
        )
        self.w_stringdtype = W_Dtype(
            types.StringType(),
            elsize=0,
            num=NPY.STRING,
            kind=NPY.STRINGLTR,
            char=NPY.STRINGLTR,
            w_box_type=space.gettypefor(boxes.W_StringBox),
        )
        self.w_unicodedtype = W_Dtype(
            types.UnicodeType(),
            elsize=0,
            num=NPY.UNICODE,
            kind=NPY.UNICODELTR,
            char=NPY.UNICODELTR,
            w_box_type=space.gettypefor(boxes.W_UnicodeBox),
        )
        self.w_voiddtype = W_Dtype(
            types.VoidType(),
            elsize=0,
            num=NPY.VOID,
            kind=NPY.VOIDLTR,
            char=NPY.VOIDLTR,
            w_box_type=space.gettypefor(boxes.W_VoidBox),
        )
        self.w_float16dtype = W_Dtype(
            types.Float16(),
            num=NPY.HALF,
            kind=NPY.FLOATINGLTR,
            char=NPY.HALFLTR,
            w_box_type=space.gettypefor(boxes.W_Float16Box),
        )
        self.w_intpdtype = W_Dtype(
            types.Long(),
            num=NPY.LONG,
            kind=NPY.SIGNEDLTR,
            char=NPY.INTPLTR,
            w_box_type=space.gettypefor(boxes.W_LongBox),
        )
        self.w_uintpdtype = W_Dtype(
            types.ULong(),
            num=NPY.ULONG,
            kind=NPY.UNSIGNEDLTR,
            char=NPY.UINTPLTR,
            w_box_type=space.gettypefor(boxes.W_ULongBox),
        )
        aliases = {
            NPY.BOOL:        ['bool_', 'bool8'],
            NPY.BYTE:        ['byte'],
            NPY.UBYTE:       ['ubyte'],
            NPY.SHORT:       ['short'],
            NPY.USHORT:      ['ushort'],
            NPY.LONG:        ['int', 'intp', 'p'],
            NPY.ULONG:       ['uint', 'uintp', 'P'],
            NPY.LONGLONG:    ['longlong'],
            NPY.ULONGLONG:   ['ulonglong'],
            NPY.FLOAT:       ['single'],
            NPY.DOUBLE:      ['float', 'double'],
            NPY.LONGDOUBLE:  ['longdouble', 'longfloat'],
            NPY.CFLOAT:      ['csingle'],
            NPY.CDOUBLE:     ['complex', 'cfloat', 'cdouble'],
            NPY.CLONGDOUBLE: ['clongdouble', 'clongfloat'],
            NPY.STRING:      ['string_', 'str'],
            NPY.UNICODE:     ['unicode_'],
        }
        self.alternate_constructors = {
            NPY.BOOL:     [space.w_bool],
            NPY.LONG:     [space.w_int,
                           space.gettypefor(boxes.W_IntegerBox),
                           space.gettypefor(boxes.W_SignedIntegerBox)],
            NPY.ULONG:    [space.gettypefor(boxes.W_UnsignedIntegerBox)],
            NPY.LONGLONG: [space.w_long],
            NPY.DOUBLE:   [space.w_float,
                           space.gettypefor(boxes.W_NumberBox),
                           space.gettypefor(boxes.W_FloatingBox)],
            NPY.CDOUBLE:  [space.w_complex,
                           space.gettypefor(boxes.W_ComplexFloatingBox)],
            NPY.STRING:   [space.w_str,
                           space.gettypefor(boxes.W_CharacterBox)],
            NPY.UNICODE:  [space.w_unicode],
            NPY.VOID:     [space.gettypefor(boxes.W_GenericBox)],
                           #space.w_buffer,  # XXX no buffer in space
        }
        float_dtypes = [self.w_float16dtype, self.w_float32dtype,
                        self.w_float64dtype, self.w_floatlongdtype]
        complex_dtypes = [self.w_complex64dtype, self.w_complex128dtype,
                          self.w_complexlongdtype]
        self.component_dtypes = {
            NPY.CFLOAT:      self.w_float32dtype,
            NPY.CDOUBLE:     self.w_float64dtype,
            NPY.CLONGDOUBLE: self.w_floatlongdtype,
        }
        self.builtin_dtypes = [
            self.w_booldtype,
            self.w_int8dtype, self.w_uint8dtype,
            self.w_int16dtype, self.w_uint16dtype,
            self.w_longdtype, self.w_ulongdtype,
            self.w_int32dtype, self.w_uint32dtype,
            self.w_int64dtype, self.w_uint64dtype,
            ] + float_dtypes + complex_dtypes + [
            self.w_stringdtype, self.w_unicodedtype, self.w_voiddtype,
            self.w_intpdtype, self.w_uintpdtype,
        ]
        self.float_dtypes_by_num_bytes = sorted(
            (dtype.elsize, dtype)
            for dtype in float_dtypes
        )
        self.dtypes_by_num = {}
        self.dtypes_by_name = {}
        # we reverse, so the stuff with lower numbers override stuff with
        # higher numbers
        for dtype in reversed(self.builtin_dtypes):
            dtype.fields = None  # mark these as builtin
            self.dtypes_by_num[dtype.num] = dtype
            self.dtypes_by_name[dtype.get_name()] = dtype
            for can_name in [dtype.kind + str(dtype.elsize),
                             dtype.char]:
                self.dtypes_by_name[can_name] = dtype
                self.dtypes_by_name[NPY.NATBYTE + can_name] = dtype
                self.dtypes_by_name[NPY.NATIVE + can_name] = dtype
                self.dtypes_by_name[NPY.IGNORE + can_name] = dtype
            if dtype.num in aliases:
                for alias in aliases[dtype.num]:
                    self.dtypes_by_name[alias] = dtype

        typeinfo_full = {
            'LONGLONG': self.w_int64dtype,
            'SHORT': self.w_int16dtype,
            'VOID': self.w_voiddtype,
            'UBYTE': self.w_uint8dtype,
            'UINTP': self.w_ulongdtype,
            'ULONG': self.w_ulongdtype,
            'LONG': self.w_longdtype,
            'UNICODE': self.w_unicodedtype,
            #'OBJECT',
            'ULONGLONG': self.w_uint64dtype,
            'STRING': self.w_stringdtype,
            'CFLOAT': self.w_complex64dtype,
            'CDOUBLE': self.w_complex128dtype,
            'CLONGDOUBLE': self.w_complexlongdtype,
            #'DATETIME',
            'UINT': self.w_uint32dtype,
            'INTP': self.w_intpdtype,
            'UINTP': self.w_uintpdtype,
            'HALF': self.w_float16dtype,
            'BYTE': self.w_int8dtype,
            #'TIMEDELTA',
            'INT': self.w_int32dtype,
            'DOUBLE': self.w_float64dtype,
            'LONGDOUBLE': self.w_floatlongdtype,
            'USHORT': self.w_uint16dtype,
            'FLOAT': self.w_float32dtype,
            'BOOL': self.w_booldtype,
        }

        typeinfo_partial = {
            'Generic': boxes.W_GenericBox,
            'Character': boxes.W_CharacterBox,
            'Flexible': boxes.W_FlexibleBox,
            'Inexact': boxes.W_InexactBox,
            'Integer': boxes.W_IntegerBox,
            'SignedInteger': boxes.W_SignedIntegerBox,
            'UnsignedInteger': boxes.W_UnsignedIntegerBox,
            'ComplexFloating': boxes.W_ComplexFloatingBox,
            'Number': boxes.W_NumberBox,
            'Floating': boxes.W_FloatingBox
        }
        w_typeinfo = space.newdict()
        for k, v in typeinfo_partial.iteritems():
            space.setitem(w_typeinfo, space.wrap(k), space.gettypefor(v))
        for k, dtype in typeinfo_full.iteritems():
            itembits = dtype.elsize * 8
            items_w = [space.wrap(dtype.char),
                       space.wrap(dtype.num),
                       space.wrap(itembits),
                       space.wrap(dtype.itemtype.get_element_size())]
            if dtype.is_int():
                if dtype.is_bool():
                    w_maxobj = space.wrap(1)
                    w_minobj = space.wrap(0)
                elif dtype.is_signed():
                    w_maxobj = space.wrap(r_longlong((1 << (itembits - 1))
                                          - 1))
                    w_minobj = space.wrap(r_longlong(-1) << (itembits - 1))
                else:
                    w_maxobj = space.wrap(r_ulonglong(1 << itembits) - 1)
                    w_minobj = space.wrap(0)
                items_w = items_w + [w_maxobj, w_minobj]
            items_w = items_w + [dtype.w_box_type]
            space.setitem(w_typeinfo, space.wrap(k), space.newtuple(items_w))
        self.w_typeinfo = w_typeinfo


def get_dtype_cache(space):
    return space.fromcache(DtypeCache)
