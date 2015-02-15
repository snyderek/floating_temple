from pypy.objspace.floating_temple.local_object import (BoolLocalObject,
    BytesLocalObject, DictLocalObject, EllipsisLocalObject, IntLocalObject,
    ListLocalObject, ModuleLocalObject, NoneLocalObject,
    NotImplementedLocalObject)
from pypy.objspace.floating_temple.wrapped_object import WrappedObject

# TODO(dss): Make the code thread-safe.

class FloatingTempleObjSpace(object):
    def __init__(self, peer):
        assert peer is not None
        self.__peer = peer

        self.w_None = self.__create_named_object('None', NoneLocalObject())
        self.w_True = self.__create_named_object('True', BoolLocalObject(True))
        self.w_False = self.__create_named_object('False',
                                                  BoolLocalObject(False))
        self.w_Ellipsis = self.__create_named_object('Ellipsis',
                                                     EllipsisLocalObject())
        self.w_NotImplemented = self.__create_named_object(
            'NotImplemented', NotImplementedLocalObject())

        self.sys = self.__create_named_object('sys', ModuleLocalObject())

    def __create_unnamed_object(self, local_object):
        return WrappedObject(self.__peer,
                             self.__peer.create_peer_object(local_object))

    def __create_named_object(self, name, local_object):
        return WrappedObject(
            self.__peer,
            self.__peer.get_or_create_named_object(name, local_object))

    def __call_method(self, w_obj, method_name, parameters):
        return w_obj._WrappedObject__call_method(method_name, parameters)

    def call_args(self, w_callable, args):
        return self.__call_method(w_callable, 'call_args', args)

    def delattr(self, w_obj, w_name):
        return self.__call_method(w_obj, 'delattr', [w_name])

    def delete(self, w_descr, w_obj):
        return self.__call_method(w_descr, 'delete', [w_obj])

    def delitem(self, w_obj, w_key):
        return self.__call_method(w_obj, 'delitem', [w_key])

    def get(self, w_descr, w_obj, w_type=None):
        return self.__call_method(w_descr, 'get', [w_obj, w_type])

    def getattr(self, w_obj, w_name):
        return self.__call_method(w_obj, 'getattr', [w_name])

    def getindex_w(self, w_obj, w_exception, objdescr=None):
        return self.__call_method(w_obj, 'getindex_w',
                                  [w_exception, objdescr])

    def getitem(self, w_obj, w_key):
        return self.__call_method(w_obj, 'getitem', [w_key])

    def is_true(self, w_x):
        return self.__call_method(w_x, 'is_true', [])

    def iter(self, w_obj):
        return self.__call_method(w_obj, 'iter', [])

    def len(self, w_obj):
        return self.__call_method(w_obj, 'len', [])

    def newdict(self, module=False, instance=False, kwargs=False,
                strdict=False):
        return self.__create_unnamed_object(DictLocalObject())

    def newint(self, intval):
        return self.__create_unnamed_object(IntLocalObject(intval))

    def newlist(self, list_w, sizehint=-1):
        return self.__create_unnamed_object(ListLocalObject(list_w))

    def newslice(self, w_start, w_end, w_step):
        # TODO(dss): Implement this.
        raise NotImplementedError()

    def newtuple(self, list_w):
        # TODO(dss): Implement this.
        raise NotImplementedError()

    def next(self, w_obj):
        return self.__call_method(w_obj, 'next', [])

    def nonzero(self, w_obj):
        return self.__call_method(w_obj, 'nonzero', [])

    def set(self, w_descr, w_obj, w_val):
        return self.__call_method(w_descr, 'set', [w_obj, w_val])

    def setattr(self, w_obj, w_name, w_val):
        return self.__call_method(w_obj, 'setattr', [w_name, w_val])

    def setitem(self, w_obj, w_key, w_val):
        return self.__call_method(w_obj, 'setitem', [w_key, w_val])

    def wrap(self, x):
        if x is None:
            return self.w_None

        if isinstance(x, int):
            if isinstance(x, bool):
                return self.newbool(x)
            else:
                return self.newint(x)

        if isinstance(x, long):
            raise NotImplementedError()

        if isinstance(x, float):
            raise NotImplementedError()

        if isinstance(x, str):
            return self.__create_unnamed_object(BytesLocalObject(x))

        if isinstance(x, unicode):
            raise NotImplementedError()

        if isinstance(x, WrappedObject):
            return x

        raise TypeError('FloatingTempleObjSpace.wrap was called with an object '
                        'of an invalid type: %s' % (str(type(x)),))
