from pypy.interpreter.error import OperationError, oefmt
from rpython.rlib import jit


def issequence_w(space, w_obj):
    from pypy.module.micronumpy.base import W_NDimArray
    return (space.isinstance_w(w_obj, space.w_tuple) or
            space.isinstance_w(w_obj, space.w_list) or
            isinstance(w_obj, W_NDimArray))


def index_w(space, w_obj):
    try:
        return space.int_w(space.index(w_obj))
    except OperationError:
        try:
            return space.int_w(space.int(w_obj))
        except OperationError:
            raise oefmt(space.w_IndexError, "cannot convert index to integer")


@jit.unroll_safe
def product(s):
    i = 1
    for x in s:
        i *= x
    return i


def check_and_adjust_index(space, index, size, axis):
    if index < -size or index >= size:
        if axis >= 0:
            raise oefmt(space.w_IndexError,
                        "index %d is out of bounds for axis %d with size %d",
                        index, axis, size)
        else:
            raise oefmt(space.w_IndexError,
                        "index %d is out of bounds for size %d",
                        index, size)
    if index < 0:
        index += size
    return index
