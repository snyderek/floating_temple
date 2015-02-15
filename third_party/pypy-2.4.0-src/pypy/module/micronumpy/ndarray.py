from pypy.interpreter.error import OperationError, oefmt
from pypy.interpreter.gateway import interp2app, unwrap_spec, applevel, \
    WrappedDefault
from pypy.interpreter.typedef import TypeDef, GetSetProperty, \
    make_weakref_descr
from rpython.rlib import jit
from rpython.rlib.rstring import StringBuilder
from rpython.rlib.rawstorage import RAW_STORAGE_PTR
from rpython.rtyper.lltypesystem import rffi
from rpython.tool.sourcetools import func_with_new_name
from pypy.module.micronumpy import descriptor, ufuncs, boxes, arrayops, loop, \
    support, constants as NPY
from pypy.module.micronumpy.appbridge import get_appbridge_cache
from pypy.module.micronumpy.arrayops import repeat, choose, put
from pypy.module.micronumpy.base import W_NDimArray, convert_to_array, \
    ArrayArgumentException, wrap_impl
from pypy.module.micronumpy.concrete import BaseConcreteArray
from pypy.module.micronumpy.converters import multi_axis_converter, \
    order_converter, shape_converter
from pypy.module.micronumpy.flagsobj import W_FlagsObject
from pypy.module.micronumpy.flatiter import W_FlatIterator
from pypy.module.micronumpy.strides import get_shape_from_iterable, \
    shape_agreement, shape_agreement_multiple


def _match_dot_shapes(space, left, right):
    left_shape = left.get_shape()
    right_shape = right.get_shape()
    my_critical_dim_size = left_shape[-1]
    right_critical_dim_size = right_shape[0]
    right_critical_dim = 0
    out_shape = []
    if len(right_shape) > 1:
        right_critical_dim = len(right_shape) - 2
        right_critical_dim_size = right_shape[right_critical_dim]
        assert right_critical_dim >= 0
        out_shape = (out_shape + left_shape[:-1] +
                     right_shape[0:right_critical_dim] +
                     right_shape[right_critical_dim + 1:])
    elif len(right_shape) > 0:
        #dot does not reduce for scalars
        out_shape = out_shape + left_shape[:-1]
    if my_critical_dim_size != right_critical_dim_size:
        raise oefmt(space.w_ValueError, "objects are not aligned")
    return out_shape, right_critical_dim


class __extend__(W_NDimArray):
    @jit.unroll_safe
    def descr_get_shape(self, space):
        shape = self.get_shape()
        return space.newtuple([space.wrap(i) for i in shape])

    def get_shape(self):
        return self.implementation.get_shape()

    def descr_set_shape(self, space, w_new_shape):
        shape = get_shape_from_iterable(space, self.get_size(), w_new_shape)
        self.implementation = self.implementation.set_shape(space, self, shape)

    def descr_get_strides(self, space):
        strides = self.implementation.get_strides()
        return space.newtuple([space.wrap(i) for i in strides])

    def get_dtype(self):
        return self.implementation.dtype

    def get_order(self):
        return self.implementation.order

    def descr_get_dtype(self, space):
        return self.implementation.dtype

    def descr_set_dtype(self, space, w_dtype):
        dtype = space.interp_w(descriptor.W_Dtype, space.call_function(
            space.gettypefor(descriptor.W_Dtype), w_dtype))
        if (dtype.elsize != self.get_dtype().elsize or
                dtype.is_flexible() or self.get_dtype().is_flexible()):
            raise OperationError(space.w_ValueError, space.wrap(
                "new type not compatible with array."))
        self.implementation.set_dtype(space, dtype)

    def descr_del_dtype(self, space):
        raise OperationError(space.w_AttributeError, space.wrap(
            "Cannot delete array dtype"))

    def descr_get_ndim(self, space):
        return space.wrap(len(self.get_shape()))

    def descr_get_itemsize(self, space):
        return space.wrap(self.get_dtype().elsize)

    def descr_get_nbytes(self, space):
        return space.wrap(self.get_size() * self.get_dtype().elsize)

    def descr_fill(self, space, w_value):
        self.fill(space, self.get_dtype().coerce(space, w_value))

    def descr_tostring(self, space, w_order=None):
        order = order_converter(space, w_order, NPY.CORDER)
        if order == NPY.FORTRANORDER:
            raise OperationError(space.w_NotImplementedError, space.wrap(
                "unsupported value for order"))
        return space.wrap(loop.tostring(space, self))

    def getitem_filter(self, space, arr):
        if len(arr.get_shape()) > 1 and arr.get_shape() != self.get_shape():
            raise OperationError(space.w_ValueError, space.wrap(
                "boolean index array should have 1 dimension"))
        if arr.get_size() > self.get_size():
            raise OperationError(space.w_ValueError, space.wrap(
                "index out of range for array"))
        size = loop.count_all_true(arr)
        if len(arr.get_shape()) == 1:
            res_shape = [size] + self.get_shape()[1:]
        else:
            res_shape = [size]
        w_res = W_NDimArray.from_shape(space, res_shape, self.get_dtype(),
                                       w_instance=self)
        return loop.getitem_filter(w_res, self, arr)

    def setitem_filter(self, space, idx, val):
        if len(idx.get_shape()) > 1 and idx.get_shape() != self.get_shape():
            raise OperationError(space.w_ValueError, space.wrap(
                "boolean index array should have 1 dimension"))
        if idx.get_size() > self.get_size():
            raise OperationError(space.w_ValueError, space.wrap(
                "index out of range for array"))
        size = loop.count_all_true(idx)
        if size > val.get_size() and val.get_size() != 1:
            raise oefmt(space.w_ValueError,
                        "NumPy boolean array indexing assignment "
                        "cannot assign %d input values to "
                        "the %d output values where the mask is true",
                        val.get_size(), size)
        loop.setitem_filter(space, self, idx, val)

    def _prepare_array_index(self, space, w_index):
        if isinstance(w_index, W_NDimArray):
            return [], w_index.get_shape(), w_index.get_shape(), [w_index]
        w_lst = space.listview(w_index)
        for w_item in w_lst:
            if not space.isinstance_w(w_item, space.w_int):
                break
        else:
            arr = convert_to_array(space, w_index)
            return [], arr.get_shape(), arr.get_shape(), [arr]
        shape = None
        indexes_w = [None] * len(w_lst)
        res_shape = []
        arr_index_in_shape = False
        prefix = []
        for i, w_item in enumerate(w_lst):
            if (isinstance(w_item, W_NDimArray) or
                    space.isinstance_w(w_item, space.w_list)):
                w_item = convert_to_array(space, w_item)
                if shape is None:
                    shape = w_item.get_shape()
                else:
                    shape = shape_agreement(space, shape, w_item)
                indexes_w[i] = w_item
                if not arr_index_in_shape:
                    res_shape.append(-1)
                    arr_index_in_shape = True
            else:
                if space.isinstance_w(w_item, space.w_slice):
                    lgt = space.decode_index4(w_item, self.get_shape()[i])[3]
                    if not arr_index_in_shape:
                        prefix.append(w_item)
                    res_shape.append(lgt)
                indexes_w[i] = w_item
        real_shape = []
        for i in res_shape:
            if i == -1:
                real_shape += shape
            else:
                real_shape.append(i)
        return prefix, real_shape[:], shape, indexes_w

    def getitem_array_int(self, space, w_index):
        prefix, res_shape, iter_shape, indexes = \
            self._prepare_array_index(space, w_index)
        if iter_shape is None:
            # w_index is a list of slices, return a view
            chunks = self.implementation._prepare_slice_args(space, w_index)
            return chunks.apply(space, self)
        shape = res_shape + self.get_shape()[len(indexes):]
        w_res = W_NDimArray.from_shape(space, shape, self.get_dtype(),
                                       self.get_order(), w_instance=self)
        if not w_res.get_size():
            return w_res
        return loop.getitem_array_int(space, self, w_res, iter_shape, indexes,
                                      prefix)

    def setitem_array_int(self, space, w_index, w_value):
        val_arr = convert_to_array(space, w_value)
        prefix, _, iter_shape, indexes = \
            self._prepare_array_index(space, w_index)
        if iter_shape is None:
            # w_index is a list of slices
            chunks = self.implementation._prepare_slice_args(space, w_index)
            view = chunks.apply(space, self)
            view.implementation.setslice(space, val_arr)
            return
        if support.product(iter_shape) == 0:
            return
        loop.setitem_array_int(space, self, iter_shape, indexes, val_arr,
                               prefix)

    def descr_getitem(self, space, w_idx):
        if space.is_w(w_idx, space.w_Ellipsis):
            return self
        elif isinstance(w_idx, W_NDimArray) and w_idx.get_dtype().is_bool() \
                and len(w_idx.get_shape()) > 0:
            return self.getitem_filter(space, w_idx)
        try:
            return self.implementation.descr_getitem(space, self, w_idx)
        except ArrayArgumentException:
            return self.getitem_array_int(space, w_idx)

    def getitem(self, space, index_list):
        return self.implementation.getitem_index(space, index_list)

    def setitem(self, space, index_list, w_value):
        self.implementation.setitem_index(space, index_list, w_value)

    def descr_setitem(self, space, w_idx, w_value):
        if space.is_w(w_idx, space.w_Ellipsis):
            self.implementation.setslice(space, convert_to_array(space, w_value))
            return
        elif isinstance(w_idx, W_NDimArray) and w_idx.get_dtype().is_bool() \
                and len(w_idx.get_shape()) > 0:
            self.setitem_filter(space, w_idx, convert_to_array(space, w_value))
            return
        try:
            self.implementation.descr_setitem(space, self, w_idx, w_value)
        except ArrayArgumentException:
            self.setitem_array_int(space, w_idx, w_value)

    def descr_delitem(self, space, w_idx):
        raise OperationError(space.w_ValueError, space.wrap(
            "cannot delete array elements"))

    def descr_len(self, space):
        shape = self.get_shape()
        if len(shape):
            return space.wrap(shape[0])
        raise OperationError(space.w_TypeError, space.wrap(
            "len() of unsized object"))

    def descr_repr(self, space):
        cache = get_appbridge_cache(space)
        if cache.w_array_repr is None:
            return space.wrap(self.dump_data())
        return space.call_function(cache.w_array_repr, self)

    def descr_str(self, space):
        cache = get_appbridge_cache(space)
        if cache.w_array_str is None:
            return space.wrap(self.dump_data(prefix='', separator='', suffix=''))
        return space.call_function(cache.w_array_str, self)

    def dump_data(self, prefix='array(', separator=',', suffix=')'):
        i, state = self.create_iter()
        first = True
        dtype = self.get_dtype()
        s = StringBuilder()
        s.append(prefix)
        if not self.is_scalar():
            s.append('[')
        while not i.done(state):
            if first:
                first = False
            else:
                s.append(separator)
                s.append(' ')
            if self.is_scalar() and dtype.is_str():
                s.append(dtype.itemtype.to_str(i.getitem(state)))
            else:
                s.append(dtype.itemtype.str_format(i.getitem(state)))
            state = i.next(state)
        if not self.is_scalar():
            s.append(']')
        s.append(suffix)
        return s.build()

    def create_iter(self, shape=None, backward_broadcast=False):
        assert isinstance(self.implementation, BaseConcreteArray)
        return self.implementation.create_iter(
            shape=shape, backward_broadcast=backward_broadcast)

    def is_scalar(self):
        return len(self.get_shape()) == 0

    def set_scalar_value(self, w_val):
        return self.implementation.setitem(self.implementation.start, w_val)

    def fill(self, space, box):
        self.implementation.fill(space, box)

    def descr_get_size(self, space):
        return space.wrap(self.get_size())

    def get_size(self):
        return self.implementation.get_size()

    def get_scalar_value(self):
        assert self.get_size() == 1
        return self.implementation.getitem(self.implementation.start)

    def descr_copy(self, space, w_order=None):
        order = order_converter(space, w_order, NPY.KEEPORDER)
        if order == NPY.FORTRANORDER:
            raise OperationError(space.w_NotImplementedError, space.wrap(
                "unsupported value for order"))
        copy = self.implementation.copy(space)
        w_subtype = space.type(self)
        return wrap_impl(space, w_subtype, self, copy)

    def descr_get_real(self, space):
        ret = self.implementation.get_real(space, self)
        return wrap_impl(space, space.type(self), self, ret)

    def descr_get_imag(self, space):
        ret = self.implementation.get_imag(space, self)
        return wrap_impl(space, space.type(self), self, ret)

    def descr_set_real(self, space, w_value):
        # copy (broadcast) values into self
        self.implementation.set_real(space, self, w_value)

    def descr_set_imag(self, space, w_value):
        # if possible, copy (broadcast) values into self
        if not self.get_dtype().is_complex():
            raise oefmt(space.w_TypeError,
                        'array does not have imaginary part to set')
        self.implementation.set_imag(space, self, w_value)

    def reshape(self, space, w_shape):
        new_shape = get_shape_from_iterable(space, self.get_size(), w_shape)
        new_impl = self.implementation.reshape(self, new_shape)
        if new_impl is not None:
            return wrap_impl(space, space.type(self), self, new_impl)
        # Create copy with contiguous data
        arr = self.descr_copy(space)
        if arr.get_size() > 0:
            arr.implementation = arr.implementation.reshape(self, new_shape)
            assert arr.implementation
        else:
            arr.implementation.shape = new_shape
        return arr

    def descr_reshape(self, space, __args__):
        """reshape(...)
        a.reshape(shape)

        Returns an array containing the same data with a new shape.

        Refer to `numpy.reshape` for full documentation.

        See Also
        --------
        numpy.reshape : equivalent function
        """
        args_w, kw_w = __args__.unpack()
        order = NPY.CORDER
        if kw_w:
            if "order" in kw_w:
                order = order_converter(space, kw_w["order"], order)
                del kw_w["order"]
            if kw_w:
                raise OperationError(space.w_TypeError, space.wrap(
                    "reshape() got unexpected keyword argument(s)"))
        if order == NPY.KEEPORDER:
            raise OperationError(space.w_ValueError, space.wrap(
                "order 'K' is not permitted for reshaping"))
        if order != NPY.CORDER and order != NPY.ANYORDER:
            raise OperationError(space.w_NotImplementedError, space.wrap(
                "unsupported value for order"))
        if len(args_w) == 1:
            if space.is_none(args_w[0]):
                return self.descr_view(space)
            w_shape = args_w[0]
        else:
            w_shape = space.newtuple(args_w)
        return self.reshape(space, w_shape)

    def descr_get_transpose(self, space):
        return W_NDimArray(self.implementation.transpose(self))

    def descr_transpose(self, space, args_w):
        if not (len(args_w) == 0 or
                len(args_w) == 1 and space.is_none(args_w[0])):
            raise OperationError(space.w_NotImplementedError, space.wrap(
                "axes unsupported for transpose"))
        return self.descr_get_transpose(space)

    @unwrap_spec(axis1=int, axis2=int)
    def descr_swapaxes(self, space, axis1, axis2):
        """a.swapaxes(axis1, axis2)

        Return a view of the array with `axis1` and `axis2` interchanged.

        Refer to `numpy.swapaxes` for full documentation.

        See Also
        --------
        numpy.swapaxes : equivalent function
        """
        if self.is_scalar():
            return self
        return self.implementation.swapaxes(space, self, axis1, axis2)

    def descr_nonzero(self, space):
        index_type = descriptor.get_dtype_cache(space).w_int64dtype
        return self.implementation.nonzero(space, index_type)

    def descr_tolist(self, space):
        if len(self.get_shape()) == 0:
            return self.get_scalar_value().item(space)
        l_w = []
        for i in range(self.get_shape()[0]):
            l_w.append(space.call_method(self.descr_getitem(space,
                                         space.wrap(i)), "tolist"))
        return space.newlist(l_w)

    def descr_ravel(self, space, w_order=None):
        if space.is_none(w_order):
            order = 'C'
        else:
            order = space.str_w(w_order)
        if order != 'C':
            raise OperationError(space.w_NotImplementedError, space.wrap(
                "order not implemented"))
        return self.reshape(space, space.wrap(-1))

    @unwrap_spec(w_axis=WrappedDefault(None),
                 w_out=WrappedDefault(None),
                 w_mode=WrappedDefault('raise'))
    def descr_take(self, space, w_obj, w_axis=None, w_out=None, w_mode=None):
        return app_take(space, self, w_obj, w_axis, w_out, w_mode)

    def descr_compress(self, space, w_obj, w_axis=None):
        if not space.is_none(w_axis):
            raise OperationError(space.w_NotImplementedError,
                                 space.wrap("axis unsupported for compress"))
            arr = self
        else:
            arr = self.reshape(space, space.wrap(-1))
        index = convert_to_array(space, w_obj)
        return arr.getitem_filter(space, index)

    def descr_flatten(self, space, w_order=None):
        if self.is_scalar():
            # scalars have no storage
            return self.reshape(space, space.wrap(1))
        w_res = self.descr_ravel(space, w_order)
        if w_res.implementation.storage == self.implementation.storage:
            return w_res.descr_copy(space)
        return w_res

    @unwrap_spec(repeats=int)
    def descr_repeat(self, space, repeats, w_axis=None):
        return repeat(space, self, repeats, w_axis)

    def descr_set_flatiter(self, space, w_obj):
        arr = convert_to_array(space, w_obj)
        loop.flatiter_setitem(space, self, arr, 0, 1, self.get_size())

    def descr_get_flatiter(self, space):
        return space.wrap(W_FlatIterator(self))

    def descr_item(self, space, __args__):
        args_w, kw_w = __args__.unpack()
        if len(args_w) == 1 and space.isinstance_w(args_w[0], space.w_tuple):
            args_w = space.fixedview(args_w[0])
        shape = self.get_shape()
        coords = [0] * len(shape)
        if len(args_w) == 0:
            if self.get_size() == 1:
                w_obj = self.get_scalar_value()
                assert isinstance(w_obj, boxes.W_GenericBox)
                return w_obj.item(space)
            raise oefmt(space.w_ValueError,
                        "can only convert an array of size 1 to a Python scalar")
        elif len(args_w) == 1 and len(shape) != 1:
            value = support.index_w(space, args_w[0])
            value = support.check_and_adjust_index(space, value, self.get_size(), -1)
            for idim in range(len(shape) - 1, -1, -1):
                coords[idim] = value % shape[idim]
                value //= shape[idim]
        elif len(args_w) == len(shape):
            for idim in range(len(shape)):
                coords[idim] = support.index_w(space, args_w[idim])
        else:
            raise oefmt(space.w_ValueError, "incorrect number of indices for array")
        item = self.getitem(space, coords)
        assert isinstance(item, boxes.W_GenericBox)
        return item.item(space)

    def descr_itemset(self, space, args_w):
        if len(args_w) == 0:
            raise OperationError(space.w_ValueError, space.wrap(
                "itemset must have at least one argument"))
        if len(args_w) != len(self.get_shape()) + 1:
            raise OperationError(space.w_ValueError, space.wrap(
                "incorrect number of indices for array"))
        self.descr_setitem(space, space.newtuple(args_w[:-1]), args_w[-1])

    def descr___array__(self, space, w_dtype=None):
        if not space.is_none(w_dtype):
            raise OperationError(space.w_NotImplementedError, space.wrap(
                "__array__(dtype) not implemented"))
        if type(self) is W_NDimArray:
            return self
        return W_NDimArray.from_shape_and_storage(
            space, self.get_shape(), self.implementation.storage,
            self.get_dtype(), w_base=self)

    def descr_array_iface(self, space):
        addr = self.implementation.get_storage_as_int(space)
        # will explode if it can't
        w_d = space.newdict()
        space.setitem_str(w_d, 'data',
                          space.newtuple([space.wrap(addr), space.w_False]))
        space.setitem_str(w_d, 'shape', self.descr_get_shape(space))
        space.setitem_str(w_d, 'typestr', self.get_dtype().descr_get_str(space))
        if self.implementation.order == 'C':
            # Array is contiguous, no strides in the interface.
            strides = space.w_None
        else:
            strides = self.descr_get_strides(space)
        space.setitem_str(w_d, 'strides', strides)
        return w_d

    w_pypy_data = None

    def fget___pypy_data__(self, space):
        return self.w_pypy_data

    def fset___pypy_data__(self, space, w_data):
        self.w_pypy_data = w_data

    def fdel___pypy_data__(self, space):
        self.w_pypy_data = None

    def descr_argsort(self, space, w_axis=None, w_kind=None, w_order=None):
        # happily ignore the kind
        # create a contiguous copy of the array
        # we must do that, because we need a working set. otherwise
        # we would modify the array in-place. Use this to our advantage
        # by converting nonnative byte order.
        if self.is_scalar():
            return space.wrap(0)
        dtype = self.get_dtype().descr_newbyteorder(space, NPY.NATIVE)
        contig = self.implementation.astype(space, dtype)
        return contig.argsort(space, w_axis)

    def descr_astype(self, space, w_dtype):
        cur_dtype = self.get_dtype()
        new_dtype = space.interp_w(descriptor.W_Dtype, space.call_function(
            space.gettypefor(descriptor.W_Dtype), w_dtype))
        if new_dtype.num == NPY.VOID:
            raise oefmt(space.w_NotImplementedError,
                        "astype(%s) not implemented yet",
                        new_dtype.get_name())
        if new_dtype.num == NPY.STRING and new_dtype.elsize == 0:
            if cur_dtype.num == NPY.STRING:
                new_dtype = descriptor.variable_dtype(
                    space, 'S' + str(cur_dtype.elsize))
        impl = self.implementation
        new_impl = impl.astype(space, new_dtype)
        return wrap_impl(space, space.type(self), self, new_impl)

    def descr_get_base(self, space):
        impl = self.implementation
        ret = impl.base()
        if ret is None:
            return space.w_None
        return ret

    @unwrap_spec(inplace=bool)
    def descr_byteswap(self, space, inplace=False):
        if inplace:
            loop.byteswap(self.implementation, self.implementation)
            return self
        else:
            w_res = W_NDimArray.from_shape(space, self.get_shape(),
                                           self.get_dtype(), w_instance=self)
            loop.byteswap(self.implementation, w_res.implementation)
            return w_res

    def descr_choose(self, space, w_choices, w_out=None, w_mode=None):
        return choose(space, self, w_choices, w_out, w_mode)

    def descr_clip(self, space, w_min=None, w_max=None, w_out=None):
        if space.is_none(w_min):
            w_min = None
        else:
            w_min = convert_to_array(space, w_min)
        if space.is_none(w_max):
            w_max = None
        else:
            w_max = convert_to_array(space, w_max)
        if space.is_none(w_out):
            w_out = None
        elif not isinstance(w_out, W_NDimArray):
            raise OperationError(space.w_TypeError, space.wrap(
                "return arrays must be of ArrayType"))
        if not w_min and not w_max:
            raise oefmt(space.w_ValueError, "One of max or min must be given.")
        shape = shape_agreement_multiple(space, [self, w_min, w_max, w_out])
        out = descriptor.dtype_agreement(space, [self, w_min, w_max], shape, w_out)
        loop.clip(space, self, shape, w_min, w_max, out)
        return out

    def descr_get_ctypes(self, space):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "ctypes not implemented yet"))

    def buffer_w(self, space, flags):
        return self.implementation.get_buffer(space, True)

    def readbuf_w(self, space):
        return self.implementation.get_buffer(space, True)

    def writebuf_w(self, space):
        return self.implementation.get_buffer(space, False)

    def charbuf_w(self, space):
        return self.implementation.get_buffer(space, True).as_str()

    def descr_get_data(self, space):
        return space.newbuffer(self.implementation.get_buffer(space, False))

    @unwrap_spec(offset=int, axis1=int, axis2=int)
    def descr_diagonal(self, space, offset=0, axis1=0, axis2=1):
        if len(self.get_shape()) < 2:
            raise OperationError(space.w_ValueError, space.wrap(
                "need at least 2 dimensions for diagonal"))
        if (axis1 < 0 or axis2 < 0 or axis1 >= len(self.get_shape()) or
                axis2 >= len(self.get_shape())):
            raise oefmt(space.w_ValueError,
                        "axis1(=%d) and axis2(=%d) must be withing range "
                        "(ndim=%d)", axis1, axis2, len(self.get_shape()))
        if axis1 == axis2:
            raise OperationError(space.w_ValueError, space.wrap(
                "axis1 and axis2 cannot be the same"))
        return arrayops.diagonal(space, self.implementation, offset, axis1, axis2)

    @unwrap_spec(offset=int, axis1=int, axis2=int)
    def descr_trace(self, space, offset=0, axis1=0, axis2=1,
                    w_dtype=None, w_out=None):
        diag = self.descr_diagonal(space, offset, axis1, axis2)
        return diag.descr_sum(space, w_axis=space.wrap(-1), w_dtype=w_dtype, w_out=w_out)

    def descr_dump(self, space, w_file):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "dump not implemented yet"))

    def descr_dumps(self, space):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "dumps not implemented yet"))

    w_flags = None

    def descr_get_flags(self, space):
        if self.w_flags is None:
            self.w_flags = W_FlagsObject(self)
        return self.w_flags

    @unwrap_spec(offset=int)
    def descr_getfield(self, space, w_dtype, offset):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "getfield not implemented yet"))

    @unwrap_spec(new_order=str)
    def descr_newbyteorder(self, space, new_order=NPY.SWAP):
        return self.descr_view(
            space, self.get_dtype().descr_newbyteorder(space, new_order))

    @unwrap_spec(w_axis=WrappedDefault(None),
                 w_out=WrappedDefault(None))
    def descr_ptp(self, space, w_axis=None, w_out=None):
        return app_ptp(space, self, w_axis, w_out)

    def descr_put(self, space, w_indices, w_values, w_mode=None):
        put(space, self, w_indices, w_values, w_mode)

    @unwrap_spec(w_refcheck=WrappedDefault(True))
    def descr_resize(self, space, w_new_shape, w_refcheck=None):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "resize not implemented yet"))

    @unwrap_spec(decimals=int)
    def descr_round(self, space, decimals=0, w_out=None):
        if space.is_none(w_out):
            if self.get_dtype().is_bool():
                # numpy promotes bool.round() to float16. Go figure.
                w_out = W_NDimArray.from_shape(space, self.get_shape(),
                    descriptor.get_dtype_cache(space).w_float16dtype)
            else:
                w_out = None
        elif not isinstance(w_out, W_NDimArray):
            raise OperationError(space.w_TypeError, space.wrap(
                "return arrays must be of ArrayType"))
        out = descriptor.dtype_agreement(space, [self], self.get_shape(), w_out)
        if out.get_dtype().is_bool() and self.get_dtype().is_bool():
            calc_dtype = descriptor.get_dtype_cache(space).w_longdtype
        else:
            calc_dtype = out.get_dtype()

        if decimals == 0:
            out = out.descr_view(space, space.type(self))
        loop.round(space, self, calc_dtype, self.get_shape(), decimals, out)
        return out

    @unwrap_spec(side=str, w_sorter=WrappedDefault(None))
    def descr_searchsorted(self, space, w_v, side='left', w_sorter=None):
        if not space.is_none(w_sorter):
            raise OperationError(space.w_NotImplementedError, space.wrap(
                'sorter not supported in searchsort'))
        if not side or len(side) < 1:
            raise OperationError(space.w_ValueError, space.wrap(
                "expected nonempty string for keyword 'side'"))
        elif side[0] == 'l' or side[0] == 'L':
            side = 'l'
        elif side[0] == 'r' or side[0] == 'R':
            side = 'r'
        else:
            raise oefmt(space.w_ValueError,
                        "'%s' is an invalid value for keyword 'side'", side)
        if len(self.get_shape()) > 1:
            raise oefmt(space.w_ValueError, "a must be a 1-d array")
        v = convert_to_array(space, w_v)
        if len(v.get_shape()) > 1:
            raise oefmt(space.w_ValueError, "v must be a 1-d array-like")
        ret = W_NDimArray.from_shape(
            space, v.get_shape(), descriptor.get_dtype_cache(space).w_longdtype)
        app_searchsort(space, self, v, space.wrap(side), ret)
        if ret.is_scalar():
            return ret.get_scalar_value()
        return ret

    def descr_setasflat(self, space, w_v):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "setasflat not implemented yet"))

    def descr_setfield(self, space, w_val, w_dtype, w_offset=0):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "setfield not implemented yet"))

    def descr_setflags(self, space, w_write=None, w_align=None, w_uic=None):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "setflags not implemented yet"))

    @unwrap_spec(kind=str)
    def descr_sort(self, space, w_axis=None, kind='quicksort', w_order=None):
        # happily ignore the kind
        # modify the array in-place
        if self.is_scalar():
            return
        return self.implementation.sort(space, w_axis, w_order)

    def descr_squeeze(self, space, w_axis=None):
        cur_shape = self.get_shape()
        if not space.is_none(w_axis):
            axes = multi_axis_converter(space, w_axis, len(cur_shape))
            new_shape = []
            for i in range(len(cur_shape)):
                if axes[i]:
                    if cur_shape[i] != 1:
                        raise OperationError(space.w_ValueError, space.wrap(
                            "cannot select an axis to squeeze out "
                            "which has size greater than one"))
                else:
                    new_shape.append(cur_shape[i])
        else:
            new_shape = [s for s in cur_shape if s != 1]
        if len(cur_shape) == len(new_shape):
            return self
        return wrap_impl(space, space.type(self), self,
                         self.implementation.get_view(
                             space, self, self.get_dtype(), new_shape))

    def descr_strides(self, space):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "strides not implemented yet"))

    def descr_tofile(self, space, w_fid, w_sep="", w_format="%s"):
        raise OperationError(space.w_NotImplementedError, space.wrap(
            "tofile not implemented yet"))

    def descr_view(self, space, w_dtype=None, w_type=None):
        if not w_type and w_dtype:
            try:
                if space.is_true(space.issubtype(
                        w_dtype, space.gettypefor(W_NDimArray))):
                    w_type = w_dtype
                    w_dtype = None
            except OperationError, e:
                if e.match(space, space.w_TypeError):
                    pass
                else:
                    raise
        if w_dtype:
            dtype = space.interp_w(descriptor.W_Dtype, space.call_function(
                space.gettypefor(descriptor.W_Dtype), w_dtype))
        else:
            dtype = self.get_dtype()
        old_itemsize = self.get_dtype().elsize
        new_itemsize = dtype.elsize
        impl = self.implementation
        if new_itemsize == 0:
            raise OperationError(space.w_TypeError, space.wrap(
                "data-type must not be 0-sized"))
        if dtype.subdtype is None:
            new_shape = self.get_shape()[:]
            dims = len(new_shape)
        else:
            new_shape = self.get_shape() + dtype.shape
            dtype = dtype.subdtype
            dims = 0
        if dims == 0:
            # Cannot resize scalars
            if old_itemsize != new_itemsize:
                raise OperationError(space.w_ValueError, space.wrap(
                    "new type not compatible with array."))
        else:
            if dims == 1 or impl.get_strides()[0] < impl.get_strides()[-1]:
                # Column-major, resize first dimension
                if new_shape[0] * old_itemsize % new_itemsize != 0:
                    raise OperationError(space.w_ValueError, space.wrap(
                        "new type not compatible with array."))
                new_shape[0] = new_shape[0] * old_itemsize / new_itemsize
            else:
                # Row-major, resize last dimension
                if new_shape[-1] * old_itemsize % new_itemsize != 0:
                    raise OperationError(space.w_ValueError, space.wrap(
                        "new type not compatible with array."))
                new_shape[-1] = new_shape[-1] * old_itemsize / new_itemsize
        v = impl.get_view(space, self, dtype, new_shape)
        w_ret = wrap_impl(space, w_type, self, v)
        return w_ret

    # --------------------- operations ----------------------------

    def _unaryop_impl(ufunc_name):
        def impl(self, space, w_out=None):
            return getattr(ufuncs.get(space), ufunc_name).call(
                space, [self, w_out])
        return func_with_new_name(impl, "unaryop_%s_impl" % ufunc_name)

    descr_pos = _unaryop_impl("positive")
    descr_neg = _unaryop_impl("negative")
    descr_abs = _unaryop_impl("absolute")
    descr_invert = _unaryop_impl("invert")

    descr_conj = _unaryop_impl('conjugate')

    def descr___nonzero__(self, space):
        if self.get_size() > 1:
            raise OperationError(space.w_ValueError, space.wrap(
                "The truth value of an array with more than one element "
                "is ambiguous. Use a.any() or a.all()"))
        iter, state = self.create_iter()
        return space.wrap(space.is_true(iter.getitem(state)))

    def _binop_impl(ufunc_name):
        def impl(self, space, w_other, w_out=None):
            return getattr(ufuncs.get(space), ufunc_name).call(
                space, [self, w_other, w_out])
        return func_with_new_name(impl, "binop_%s_impl" % ufunc_name)

    descr_add = _binop_impl("add")
    descr_sub = _binop_impl("subtract")
    descr_mul = _binop_impl("multiply")
    descr_div = _binop_impl("divide")
    descr_truediv = _binop_impl("true_divide")
    descr_floordiv = _binop_impl("floor_divide")
    descr_mod = _binop_impl("mod")
    descr_pow = _binop_impl("power")
    descr_lshift = _binop_impl("left_shift")
    descr_rshift = _binop_impl("right_shift")
    descr_and = _binop_impl("bitwise_and")
    descr_or = _binop_impl("bitwise_or")
    descr_xor = _binop_impl("bitwise_xor")

    def descr_divmod(self, space, w_other):
        w_quotient = self.descr_div(space, w_other)
        w_remainder = self.descr_mod(space, w_other)
        return space.newtuple([w_quotient, w_remainder])

    def _binop_comp_impl(ufunc):
        def impl(self, space, w_other, w_out=None):
            try:
                return ufunc(self, space, w_other, w_out)
            except OperationError, e:
                if e.match(space, space.w_ValueError):
                    return space.w_False
                raise e

        return func_with_new_name(impl, ufunc.func_name)

    descr_eq = _binop_comp_impl(_binop_impl("equal"))
    descr_ne = _binop_comp_impl(_binop_impl("not_equal"))
    descr_lt = _binop_comp_impl(_binop_impl("less"))
    descr_le = _binop_comp_impl(_binop_impl("less_equal"))
    descr_gt = _binop_comp_impl(_binop_impl("greater"))
    descr_ge = _binop_comp_impl(_binop_impl("greater_equal"))

    def _binop_inplace_impl(ufunc_name):
        def impl(self, space, w_other):
            w_out = self
            ufunc = getattr(ufuncs.get(space), ufunc_name)
            return ufunc.call(space, [self, w_other, w_out])
        return func_with_new_name(impl, "binop_inplace_%s_impl" % ufunc_name)

    descr_iadd = _binop_inplace_impl("add")
    descr_isub = _binop_inplace_impl("subtract")
    descr_imul = _binop_inplace_impl("multiply")
    descr_idiv = _binop_inplace_impl("divide")
    descr_itruediv = _binop_inplace_impl("true_divide")
    descr_ifloordiv = _binop_inplace_impl("floor_divide")
    descr_imod = _binop_inplace_impl("mod")
    descr_ipow = _binop_inplace_impl("power")
    descr_ilshift = _binop_inplace_impl("left_shift")
    descr_irshift = _binop_inplace_impl("right_shift")
    descr_iand = _binop_inplace_impl("bitwise_and")
    descr_ior = _binop_inplace_impl("bitwise_or")
    descr_ixor = _binop_inplace_impl("bitwise_xor")

    def _binop_right_impl(ufunc_name):
        def impl(self, space, w_other, w_out=None):
            w_other = convert_to_array(space, w_other)
            return getattr(ufuncs.get(space), ufunc_name).call(
                space, [w_other, self, w_out])
        return func_with_new_name(impl, "binop_right_%s_impl" % ufunc_name)

    descr_radd = _binop_right_impl("add")
    descr_rsub = _binop_right_impl("subtract")
    descr_rmul = _binop_right_impl("multiply")
    descr_rdiv = _binop_right_impl("divide")
    descr_rtruediv = _binop_right_impl("true_divide")
    descr_rfloordiv = _binop_right_impl("floor_divide")
    descr_rmod = _binop_right_impl("mod")
    descr_rpow = _binop_right_impl("power")
    descr_rlshift = _binop_right_impl("left_shift")
    descr_rrshift = _binop_right_impl("right_shift")
    descr_rand = _binop_right_impl("bitwise_and")
    descr_ror = _binop_right_impl("bitwise_or")
    descr_rxor = _binop_right_impl("bitwise_xor")

    def descr_rdivmod(self, space, w_other):
        w_quotient = self.descr_rdiv(space, w_other)
        w_remainder = self.descr_rmod(space, w_other)
        return space.newtuple([w_quotient, w_remainder])

    def descr_dot(self, space, w_other, w_out=None):
        if space.is_none(w_out):
            out = None
        elif not isinstance(w_out, W_NDimArray):
            raise oefmt(space.w_TypeError, 'output must be an array')
        else:
            out = w_out
        other = convert_to_array(space, w_other)
        if other.is_scalar():
            #Note: w_out is not modified, this is numpy compliant.
            return self.descr_mul(space, other)
        elif len(self.get_shape()) < 2 and len(other.get_shape()) < 2:
            w_res = self.descr_mul(space, other)
            assert isinstance(w_res, W_NDimArray)
            return w_res.descr_sum(space, space.wrap(-1), out)
        dtype = ufuncs.find_binop_result_dtype(space, self.get_dtype(),
                                               other.get_dtype())
        if self.get_size() < 1 and other.get_size() < 1:
            # numpy compatability
            return W_NDimArray.new_scalar(space, dtype, space.wrap(0))
        # Do the dims match?
        out_shape, other_critical_dim = _match_dot_shapes(space, self, other)
        if out:
            matches = True
            if dtype != out.get_dtype():
                matches = False
            elif not out.implementation.order == "C":
                matches = False
            elif len(out.get_shape()) != len(out_shape):
                matches = False
            else:
                for i in range(len(out_shape)):
                    if out.get_shape()[i] != out_shape[i]:
                        matches = False
                        break
            if not matches:
                raise OperationError(space.w_ValueError, space.wrap(
                    'output array is not acceptable (must have the right type, '
                    'nr dimensions, and be a C-Array)'))
            w_res = out
            w_res.fill(space, self.get_dtype().coerce(space, None))
        else:
            w_res = W_NDimArray.from_shape(space, out_shape, dtype, w_instance=self)
        # This is the place to add fpypy and blas
        return loop.multidim_dot(space, self, other, w_res, dtype,
                                 other_critical_dim)

    def descr_mean(self, space, __args__):
        return get_appbridge_cache(space).call_method(
            space, 'numpy.core._methods', '_mean', __args__.prepend(self))

    def descr_var(self, space, __args__):
        return get_appbridge_cache(space).call_method(
            space, 'numpy.core._methods', '_var', __args__.prepend(self))

    def descr_std(self, space, __args__):
        return get_appbridge_cache(space).call_method(
            space, 'numpy.core._methods', '_std', __args__.prepend(self))

    # ----------------------- reduce -------------------------------

    def _reduce_ufunc_impl(ufunc_name, cumulative=False):
        @unwrap_spec(keepdims=bool)
        def impl(self, space, w_axis=None, w_dtype=None, w_out=None, keepdims=False):
            if space.is_none(w_out):
                out = None
            elif not isinstance(w_out, W_NDimArray):
                raise oefmt(space.w_TypeError, 'output must be an array')
            else:
                out = w_out
            return getattr(ufuncs.get(space), ufunc_name).reduce(
                space, self, w_axis, keepdims, out, w_dtype, cumulative=cumulative)
        return func_with_new_name(impl, "reduce_%s_impl_%d" % (ufunc_name, cumulative))

    descr_sum = _reduce_ufunc_impl("add")
    descr_prod = _reduce_ufunc_impl("multiply")
    descr_max = _reduce_ufunc_impl("maximum")
    descr_min = _reduce_ufunc_impl("minimum")
    descr_all = _reduce_ufunc_impl('logical_and')
    descr_any = _reduce_ufunc_impl('logical_or')

    descr_cumsum = _reduce_ufunc_impl('add', cumulative=True)
    descr_cumprod = _reduce_ufunc_impl('multiply', cumulative=True)

    def _reduce_argmax_argmin_impl(raw_name):
        op_name = "arg%s" % raw_name
        def impl(self, space, w_axis=None, w_out=None):
            if not space.is_none(w_axis):
                raise oefmt(space.w_NotImplementedError,
                            "axis unsupported for %s", op_name)
            if not space.is_none(w_out):
                raise oefmt(space.w_NotImplementedError,
                            "out unsupported for %s", op_name)
            if self.get_size() == 0:
                raise oefmt(space.w_ValueError,
                            "Can't call %s on zero-size arrays", op_name)
            try:
                getattr(self.get_dtype().itemtype, raw_name)
            except AttributeError:
                raise oefmt(space.w_NotImplementedError,
                            '%s not implemented for %s',
                            op_name, self.get_dtype().get_name())
            return space.wrap(getattr(loop, op_name)(self))
        return func_with_new_name(impl, "reduce_%s_impl" % op_name)

    descr_argmax = _reduce_argmax_argmin_impl("max")
    descr_argmin = _reduce_argmax_argmin_impl("min")

    def descr_int(self, space):
        if self.get_size() != 1:
            raise OperationError(space.w_TypeError, space.wrap(
                "only length-1 arrays can be converted to Python scalars"))
        if self.get_dtype().is_str_or_unicode():
            raise OperationError(space.w_TypeError, space.wrap(
                "don't know how to convert scalar number to int"))
        value = self.get_scalar_value()
        return space.int(value)

    def descr_long(self, space):
        if self.get_size() != 1:
            raise OperationError(space.w_TypeError, space.wrap(
                "only length-1 arrays can be converted to Python scalars"))
        if self.get_dtype().is_str_or_unicode():
            raise OperationError(space.w_TypeError, space.wrap(
                "don't know how to convert scalar number to long"))
        value = self.get_scalar_value()
        return space.long(value)

    def descr_float(self, space):
        if self.get_size() != 1:
            raise OperationError(space.w_TypeError, space.wrap(
                "only length-1 arrays can be converted to Python scalars"))
        if self.get_dtype().is_str_or_unicode():
            raise OperationError(space.w_TypeError, space.wrap(
                "don't know how to convert scalar number to float"))
        value = self.get_scalar_value()
        return space.float(value)

    def descr_hex(self, space):
        if self.get_size() != 1:
            raise oefmt(space.w_TypeError,
                        "only length-1 arrays can be converted to Python scalars")
        if not self.get_dtype().is_int():
            raise oefmt(space.w_TypeError,
                        "don't know how to convert scalar number to hex")
        value = self.get_scalar_value()
        return space.hex(value)

    def descr_oct(self, space):
        if self.get_size() != 1:
            raise oefmt(space.w_TypeError,
                        "only length-1 arrays can be converted to Python scalars")
        if not self.get_dtype().is_int():
            raise oefmt(space.w_TypeError,
                        "don't know how to convert scalar number to oct")
        value = self.get_scalar_value()
        return space.oct(value)

    def descr_index(self, space):
        if self.get_size() != 1 or \
                not self.get_dtype().is_int() or self.get_dtype().is_bool():
            raise OperationError(space.w_TypeError, space.wrap(
                "only integer arrays with one element "
                "can be converted to an index"))
        value = self.get_scalar_value()
        assert isinstance(value, boxes.W_GenericBox)
        return value.item(space)

    def descr_reduce(self, space):
        from rpython.rlib.rstring import StringBuilder
        from pypy.interpreter.mixedmodule import MixedModule
        from pypy.module.micronumpy.concrete import SliceArray

        numpypy = space.getbuiltinmodule("_numpypy")
        assert isinstance(numpypy, MixedModule)
        multiarray = numpypy.get("multiarray")
        assert isinstance(multiarray, MixedModule)
        reconstruct = multiarray.get("_reconstruct")
        parameters = space.newtuple([self.getclass(space), space.newtuple(
            [space.wrap(0)]), space.wrap("b")])

        builder = StringBuilder()
        if isinstance(self.implementation, SliceArray):
            iter, state = self.implementation.create_iter()
            while not iter.done(state):
                box = iter.getitem(state)
                builder.append(box.raw_str())
                state = iter.next(state)
        else:
            builder.append_charpsize(self.implementation.get_storage(),
                                     self.implementation.get_storage_size())

        state = space.newtuple([
            space.wrap(1),      # version
            self.descr_get_shape(space),
            self.get_dtype(),
            space.wrap(False),  # is_fortran
            space.wrap(builder.build()),
        ])

        return space.newtuple([reconstruct, parameters, state])

    def descr_setstate(self, space, w_state):
        lens = space.len_w(w_state)
        # numpy compatability, see multiarray/methods.c
        if lens == 5:
            base_index = 1
        elif lens == 4:
            base_index = 0
        else:
            raise oefmt(space.w_ValueError,
                        "__setstate__ called with len(args[1])==%d, not 5 or 4",
                        lens)
        shape = space.getitem(w_state, space.wrap(base_index))
        dtype = space.getitem(w_state, space.wrap(base_index+1))
        #isfortran = space.getitem(w_state, space.wrap(base_index+2))
        storage = space.getitem(w_state, space.wrap(base_index+3))
        if not isinstance(dtype, descriptor.W_Dtype):
            raise oefmt(space.w_ValueError,
                        "__setstate__(self, (shape, dtype, .. called with "
                        "improper dtype '%R'", dtype)
        self.implementation = W_NDimArray.from_shape_and_storage(
            space, [space.int_w(i) for i in space.listview(shape)],
            rffi.str2charp(space.str_w(storage), track_allocation=False),
            dtype, owning=True).implementation

    def descr___array_finalize__(self, space, w_obj):
        pass

    def descr___array_wrap__(self, space, w_obj, w_context=None):
        return w_obj

    def descr___array_prepare__(self, space, w_obj, w_context=None):
        return w_obj
        pass


@unwrap_spec(offset=int)
def descr_new_array(space, w_subtype, w_shape, w_dtype=None, w_buffer=None,
                    offset=0, w_strides=None, w_order=None):
    from pypy.module.micronumpy.concrete import ConcreteArray
    from pypy.module.micronumpy.strides import calc_strides
    dtype = space.interp_w(descriptor.W_Dtype, space.call_function(
        space.gettypefor(descriptor.W_Dtype), w_dtype))
    shape = shape_converter(space, w_shape, dtype)

    if not space.is_none(w_buffer):
        if (not space.is_none(w_strides)):
            raise OperationError(space.w_NotImplementedError,
                                 space.wrap("unsupported param"))

        try:
            buf = space.writebuf_w(w_buffer)
        except OperationError:
            buf = space.readbuf_w(w_buffer)
        try:
            raw_ptr = buf.get_raw_address()
        except ValueError:
            raise OperationError(space.w_TypeError, space.wrap(
                "Only raw buffers are supported"))
        if not shape:
            raise OperationError(space.w_TypeError, space.wrap(
                "numpy scalars from buffers not supported yet"))
        totalsize = support.product(shape) * dtype.elsize
        if totalsize + offset > buf.getlength():
            raise OperationError(space.w_TypeError, space.wrap(
                "buffer is too small for requested array"))
        storage = rffi.cast(RAW_STORAGE_PTR, raw_ptr)
        storage = rffi.ptradd(storage, offset)
        return W_NDimArray.from_shape_and_storage(space, shape, storage, dtype,
                                                  w_subtype=w_subtype,
                                                  w_base=w_buffer,
                                                  writable=not buf.readonly)

    order = order_converter(space, w_order, NPY.CORDER)
    if order == NPY.CORDER:
        order = 'C'
    else:
        order = 'F'
    if space.is_w(w_subtype, space.gettypefor(W_NDimArray)):
        return W_NDimArray.from_shape(space, shape, dtype, order)
    strides, backstrides = calc_strides(shape, dtype.base, order)
    impl = ConcreteArray(shape, dtype.base, order, strides, backstrides)
    w_ret = space.allocate_instance(W_NDimArray, w_subtype)
    W_NDimArray.__init__(w_ret, impl)
    space.call_function(space.getattr(w_ret,
                        space.wrap('__array_finalize__')), w_subtype)
    return w_ret


@unwrap_spec(addr=int)
def descr__from_shape_and_storage(space, w_cls, w_shape, addr, w_dtype, w_subtype=None):
    """
    Create an array from an existing buffer, given its address as int.
    PyPy-only implementation detail.
    """
    storage = rffi.cast(RAW_STORAGE_PTR, addr)
    dtype = space.interp_w(descriptor.W_Dtype, space.call_function(
        space.gettypefor(descriptor.W_Dtype), w_dtype))
    shape = shape_converter(space, w_shape, dtype)
    if w_subtype:
        if not space.isinstance_w(w_subtype, space.w_type):
            raise OperationError(space.w_ValueError, space.wrap(
                "subtype must be a subtype of ndarray, not a class instance"))
        return W_NDimArray.from_shape_and_storage(space, shape, storage, dtype,
                                                  'C', False, w_subtype)
    else:
        return W_NDimArray.from_shape_and_storage(space, shape, storage, dtype)

app_take = applevel(r"""
    def take(a, indices, axis, out, mode):
        if mode != 'raise':
            raise NotImplementedError("mode != raise not implemented")
        if axis is None:
            from numpy import array
            indices = array(indices)
            res = a.ravel()[indices.ravel()].reshape(indices.shape)
        else:
            from operator import mul
            if axis < 0: axis += len(a.shape)
            s0, s1 = a.shape[:axis], a.shape[axis+1:]
            l0 = reduce(mul, s0) if s0 else 1
            l1 = reduce(mul, s1) if s1 else 1
            res = a.reshape((l0, -1, l1))[:,indices,:].reshape(s0 + (-1,) + s1)
        if out is not None:
            out[:] = res
            return out
        return res
""", filename=__file__).interphook('take')

app_ptp = applevel(r"""
    def ptp(a, axis, out):
        res = a.max(axis) - a.min(axis)
        if out is not None:
            out[:] = res
            return out
        return res
""", filename=__file__).interphook('ptp')

app_searchsort = applevel(r"""
    def searchsort(arr, v, side, result):
        import operator
        def func(a, op, val):
            imin = 0
            imax = a.size
            while imin < imax:
                imid = imin + ((imax - imin) >> 1)
                if op(a[imid], val):
                    imin = imid +1
                else:
                    imax = imid
            return imin
        if side == 'l':
            op = operator.lt
        else:
            op = operator.le
        if v.size < 2:
            result[...] = func(arr, op, v)
        else:
            for i in range(v.size):
                result[i] = func(arr, op, v[i])
        return result
""", filename=__file__).interphook('searchsort')

W_NDimArray.typedef = TypeDef("numpy.ndarray",
    __new__ = interp2app(descr_new_array),

    __len__ = interp2app(W_NDimArray.descr_len),
    __getitem__ = interp2app(W_NDimArray.descr_getitem),
    __setitem__ = interp2app(W_NDimArray.descr_setitem),
    __delitem__ = interp2app(W_NDimArray.descr_delitem),

    __repr__ = interp2app(W_NDimArray.descr_repr),
    __str__ = interp2app(W_NDimArray.descr_str),
    __int__ = interp2app(W_NDimArray.descr_int),
    __long__ = interp2app(W_NDimArray.descr_long),
    __float__ = interp2app(W_NDimArray.descr_float),
    __hex__ = interp2app(W_NDimArray.descr_hex),
    __oct__ = interp2app(W_NDimArray.descr_oct),
    __index__ = interp2app(W_NDimArray.descr_index),

    __pos__ = interp2app(W_NDimArray.descr_pos),
    __neg__ = interp2app(W_NDimArray.descr_neg),
    __abs__ = interp2app(W_NDimArray.descr_abs),
    __invert__ = interp2app(W_NDimArray.descr_invert),
    __nonzero__ = interp2app(W_NDimArray.descr___nonzero__),

    __add__ = interp2app(W_NDimArray.descr_add),
    __sub__ = interp2app(W_NDimArray.descr_sub),
    __mul__ = interp2app(W_NDimArray.descr_mul),
    __div__ = interp2app(W_NDimArray.descr_div),
    __truediv__ = interp2app(W_NDimArray.descr_truediv),
    __floordiv__ = interp2app(W_NDimArray.descr_floordiv),
    __mod__ = interp2app(W_NDimArray.descr_mod),
    __divmod__ = interp2app(W_NDimArray.descr_divmod),
    __pow__ = interp2app(W_NDimArray.descr_pow),
    __lshift__ = interp2app(W_NDimArray.descr_lshift),
    __rshift__ = interp2app(W_NDimArray.descr_rshift),
    __and__ = interp2app(W_NDimArray.descr_and),
    __or__ = interp2app(W_NDimArray.descr_or),
    __xor__ = interp2app(W_NDimArray.descr_xor),

    __radd__ = interp2app(W_NDimArray.descr_radd),
    __rsub__ = interp2app(W_NDimArray.descr_rsub),
    __rmul__ = interp2app(W_NDimArray.descr_rmul),
    __rdiv__ = interp2app(W_NDimArray.descr_rdiv),
    __rtruediv__ = interp2app(W_NDimArray.descr_rtruediv),
    __rfloordiv__ = interp2app(W_NDimArray.descr_rfloordiv),
    __rmod__ = interp2app(W_NDimArray.descr_rmod),
    __rdivmod__ = interp2app(W_NDimArray.descr_rdivmod),
    __rpow__ = interp2app(W_NDimArray.descr_rpow),
    __rlshift__ = interp2app(W_NDimArray.descr_rlshift),
    __rrshift__ = interp2app(W_NDimArray.descr_rrshift),
    __rand__ = interp2app(W_NDimArray.descr_rand),
    __ror__ = interp2app(W_NDimArray.descr_ror),
    __rxor__ = interp2app(W_NDimArray.descr_rxor),

    __iadd__ = interp2app(W_NDimArray.descr_iadd),
    __isub__ = interp2app(W_NDimArray.descr_isub),
    __imul__ = interp2app(W_NDimArray.descr_imul),
    __idiv__ = interp2app(W_NDimArray.descr_idiv),
    __itruediv__ = interp2app(W_NDimArray.descr_itruediv),
    __ifloordiv__ = interp2app(W_NDimArray.descr_ifloordiv),
    __imod__ = interp2app(W_NDimArray.descr_imod),
    __ipow__ = interp2app(W_NDimArray.descr_ipow),
    __ilshift__ = interp2app(W_NDimArray.descr_ilshift),
    __irshift__ = interp2app(W_NDimArray.descr_irshift),
    __iand__ = interp2app(W_NDimArray.descr_iand),
    __ior__ = interp2app(W_NDimArray.descr_ior),
    __ixor__ = interp2app(W_NDimArray.descr_ixor),

    __eq__ = interp2app(W_NDimArray.descr_eq),
    __ne__ = interp2app(W_NDimArray.descr_ne),
    __lt__ = interp2app(W_NDimArray.descr_lt),
    __le__ = interp2app(W_NDimArray.descr_le),
    __gt__ = interp2app(W_NDimArray.descr_gt),
    __ge__ = interp2app(W_NDimArray.descr_ge),

    dtype = GetSetProperty(W_NDimArray.descr_get_dtype,
                           W_NDimArray.descr_set_dtype,
                           W_NDimArray.descr_del_dtype),
    shape = GetSetProperty(W_NDimArray.descr_get_shape,
                           W_NDimArray.descr_set_shape),
    strides = GetSetProperty(W_NDimArray.descr_get_strides),
    ndim = GetSetProperty(W_NDimArray.descr_get_ndim),
    size = GetSetProperty(W_NDimArray.descr_get_size),
    itemsize = GetSetProperty(W_NDimArray.descr_get_itemsize),
    nbytes = GetSetProperty(W_NDimArray.descr_get_nbytes),
    flags = GetSetProperty(W_NDimArray.descr_get_flags),

    fill = interp2app(W_NDimArray.descr_fill),
    tostring = interp2app(W_NDimArray.descr_tostring),

    mean = interp2app(W_NDimArray.descr_mean),
    sum = interp2app(W_NDimArray.descr_sum),
    prod = interp2app(W_NDimArray.descr_prod),
    max = interp2app(W_NDimArray.descr_max),
    min = interp2app(W_NDimArray.descr_min),
    put = interp2app(W_NDimArray.descr_put),
    argmax = interp2app(W_NDimArray.descr_argmax),
    argmin = interp2app(W_NDimArray.descr_argmin),
    all = interp2app(W_NDimArray.descr_all),
    any = interp2app(W_NDimArray.descr_any),
    dot = interp2app(W_NDimArray.descr_dot),
    var = interp2app(W_NDimArray.descr_var),
    std = interp2app(W_NDimArray.descr_std),
    searchsorted = interp2app(W_NDimArray.descr_searchsorted),

    cumsum = interp2app(W_NDimArray.descr_cumsum),
    cumprod = interp2app(W_NDimArray.descr_cumprod),

    copy = interp2app(W_NDimArray.descr_copy),
    reshape = interp2app(W_NDimArray.descr_reshape),
    resize = interp2app(W_NDimArray.descr_resize),
    squeeze = interp2app(W_NDimArray.descr_squeeze),
    T = GetSetProperty(W_NDimArray.descr_get_transpose),
    transpose = interp2app(W_NDimArray.descr_transpose),
    tolist = interp2app(W_NDimArray.descr_tolist),
    flatten = interp2app(W_NDimArray.descr_flatten),
    ravel = interp2app(W_NDimArray.descr_ravel),
    take = interp2app(W_NDimArray.descr_take),
    ptp = interp2app(W_NDimArray.descr_ptp),
    compress = interp2app(W_NDimArray.descr_compress),
    repeat = interp2app(W_NDimArray.descr_repeat),
    swapaxes = interp2app(W_NDimArray.descr_swapaxes),
    nonzero = interp2app(W_NDimArray.descr_nonzero),
    flat = GetSetProperty(W_NDimArray.descr_get_flatiter,
                          W_NDimArray.descr_set_flatiter),
    item = interp2app(W_NDimArray.descr_item),
    itemset = interp2app(W_NDimArray.descr_itemset),
    real = GetSetProperty(W_NDimArray.descr_get_real,
                          W_NDimArray.descr_set_real),
    imag = GetSetProperty(W_NDimArray.descr_get_imag,
                          W_NDimArray.descr_set_imag),
    conj = interp2app(W_NDimArray.descr_conj),

    argsort  = interp2app(W_NDimArray.descr_argsort),
    sort  = interp2app(W_NDimArray.descr_sort),
    astype   = interp2app(W_NDimArray.descr_astype),
    base     = GetSetProperty(W_NDimArray.descr_get_base),
    byteswap = interp2app(W_NDimArray.descr_byteswap),
    choose   = interp2app(W_NDimArray.descr_choose),
    clip     = interp2app(W_NDimArray.descr_clip),
    round    = interp2app(W_NDimArray.descr_round),
    data     = GetSetProperty(W_NDimArray.descr_get_data),
    diagonal = interp2app(W_NDimArray.descr_diagonal),
    trace = interp2app(W_NDimArray.descr_trace),
    view = interp2app(W_NDimArray.descr_view),
    newbyteorder = interp2app(W_NDimArray.descr_newbyteorder),

    ctypes = GetSetProperty(W_NDimArray.descr_get_ctypes), # XXX unimplemented
    __array_interface__ = GetSetProperty(W_NDimArray.descr_array_iface),
    __weakref__ = make_weakref_descr(W_NDimArray),
    _from_shape_and_storage = interp2app(descr__from_shape_and_storage,
                                         as_classmethod=True),
    __pypy_data__ = GetSetProperty(W_NDimArray.fget___pypy_data__,
                                   W_NDimArray.fset___pypy_data__,
                                   W_NDimArray.fdel___pypy_data__),
    __reduce__ = interp2app(W_NDimArray.descr_reduce),
    __setstate__ = interp2app(W_NDimArray.descr_setstate),
    __array_finalize__ = interp2app(W_NDimArray.descr___array_finalize__),
    __array_prepare__ = interp2app(W_NDimArray.descr___array_prepare__),
    __array_wrap__ = interp2app(W_NDimArray.descr___array_wrap__),
    __array__         = interp2app(W_NDimArray.descr___array__),
)


def _reconstruct(space, w_subtype, w_shape, w_dtype):
    return descr_new_array(space, w_subtype, w_shape, w_dtype)


W_FlatIterator.typedef = TypeDef("numpy.flatiter",
    __iter__ = interp2app(W_FlatIterator.descr_iter),
    __getitem__ = interp2app(W_FlatIterator.descr_getitem),
    __setitem__ = interp2app(W_FlatIterator.descr_setitem),
    __len__ = interp2app(W_FlatIterator.descr_len),

    __eq__ = interp2app(W_FlatIterator.descr_eq),
    __ne__ = interp2app(W_FlatIterator.descr_ne),
    __lt__ = interp2app(W_FlatIterator.descr_lt),
    __le__ = interp2app(W_FlatIterator.descr_le),
    __gt__ = interp2app(W_FlatIterator.descr_gt),
    __ge__ = interp2app(W_FlatIterator.descr_ge),

    next = interp2app(W_FlatIterator.descr_next),
    base = GetSetProperty(W_FlatIterator.descr_base),
    index = GetSetProperty(W_FlatIterator.descr_index),
    coords = GetSetProperty(W_FlatIterator.descr_coords),
)
