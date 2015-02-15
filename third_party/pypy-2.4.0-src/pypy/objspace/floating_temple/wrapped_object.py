from pypy.objspace.floating_temple import peer_ffi
from rpython.rtyper.lltypesystem.lltype import _ptr, typeOf

def _wrap_object(peer, obj):
    if isinstance(obj, _ptr) and typeOf(obj) == peer_ffi.PEER_OBJECT_P:
        return WrappedObject(peer, obj)
    else:
        return obj

def _unwrap_object(w_obj):
    if isinstance(w_obj, WrappedObject):
        return w_obj._WrappedObject__peer_object
    else:
        return w_obj

class WrappedObject(object):
    def __init__(self, peer, peer_object):
        assert peer is not None
        assert peer_object is not None

        self.__peer = peer
        self.__peer_object = peer_object

    def __call_method(self, method_name, parameters_w):
        unwrapped_params = [_unwrap_object(param) for param in parameters_w]
        return_obj = self.__peer.call_method(self.__peer_object, method_name,
                                             unwrapped_params)
        return _wrap_object(self.__peer, return_obj)

    def __getattr__(self, name):
        if name == 'w_dict':
            return self.__call_method('getattr_w_dict', [])
        else:
            raise AttributeError()

    def bigint_w(self, space, allow_conversion=True):
        return self.__call_method('bigint_w', [allow_conversion])

    def deldictvalue(self, space, attr):
        return self.__call_method('deldictvalue', [attr])

    def float_w(self, space, allow_conversion=True):
        return self.__call_method('float_w', [allow_conversion])

    def getclass(self, space):
        return self.__call_method('getclass', [])

    def getdict(self, space):
        return self.__call_method('getdict', [])

    def getdictvalue(self, space, attr):
        return self.__call_method('getdictvalue', [attr])

    def install(self):
        return self.__call_method('install', [])

    def int_w(self, space, allow_conversion=True):
        return self.__call_method('int_w', [allow_conversion])

    def is_w(self, space, w_other):
        return self.__peer.objects_are_equivalent(self, w_other)

    def ord(self, space):
        return self.__call_method('ord', [])

    def setclass(self, space, w_subtype):
        return self.__call_method('setclass', [w_subtype])

    def setdict(self, space, w_dict):
        return self.__call_method('setdict', [w_dict])

    def setdictvalue(self, space, attr, w_value):
        return self.__call_method('setdictvalue', [attr, w_value])

    def str_w(self, space):
        return self.__call_method('str_w', [])

    def uint_w(self, space):
        return self.__call_method('uint_w', [])

    def unicode_w(self, space):
        return self.__call_method('unicode_w', [])
