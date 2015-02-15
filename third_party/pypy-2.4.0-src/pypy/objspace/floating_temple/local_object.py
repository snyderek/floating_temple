from pypy.objspace.floating_temple import peer_ffi
from pypy.objspace.floating_temple.serialization_pb2 import ObjectProto
from rpython.rlib.objectmodel import compute_unique_id

def _get_serialization_index_for_peer_object(context, peer_object):
    return peer_ffi.get_serialization_index_for_peer_object(context,
                                                            peer_object)

class LocalObject(object):
    def clone(self):
        raise NotImplementedError('Abstract method')

    def serialize(self, context):
        raise NotImplementedError('Abstract method')

    def invoke_method(self, peer, method_name, params):
        f = getattr(self, 'method_' + method_name)
        return f(peer, *params)

class BoolLocalObject(LocalObject):
    def __init__(self, bool_value):
        self.__bool_value = bool_value

    def clone(self):
        return BoolLocalObject(self.__bool_value)

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.bool_object.value = self.__bool_value
        return object_proto.SerializeToString()

    def method_is_true(self, peer):
        return self.__bool_value

class BytesLocalObject(LocalObject):
    def __init__(self, bytes_value):
        self.__bytes_value = bytes_value

    def clone(self):
        return BytesLocalObject(self.__bytes_value)

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.string_object.value = self.__bytes_value
        return object_proto.SerializeToString()

    def method_is_true(self, peer):
        return not not self.__bytes_value

    def method_str_w(self, peer):
        return self.__bytes_value

class DictLocalObject(LocalObject):
    def __init__(self):
        self.__dct = {}

    def clone(self):
        new_obj = DictLocalObject()
        new_obj.__dct = dict(self.__dct)
        return new_obj

    def serialize(self, context):
        object_proto = ObjectProto()
        dict_object = object_proto.dict_object

        for key_peer_object, value_peer_object in self.__dct.iteritems():
            dict_item = dict_object.item.append()
            dict_item.key_object_index = (
                _get_serialization_index_for_peer_object(context,
                                                         key_peer_object))
            dict_item.value_object_index = (
                _get_serialization_index_for_peer_object(context,
                                                         value_peer_object))

        return object_proto.SerializeToString()

    def method_getitem(self, peer, w_key):
        self.__dct.get(compute_unique_id(w_key))

    def method_is_true(self, peer):
        return not not self.__dct

    def method_setitem(self, peer, w_key, w_value):
        self.__dct[compute_unique_id(w_key)] = w_value

class EllipsisLocalObject(LocalObject):
    def clone(self):
        return EllipsisLocalObject()

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.ellipsis_object
        return object_proto.SerializeToString()

    def method_is_true(self, peer):
        return True

class FunctionLocalObject(LocalObject):
    def __init__(self, func):
        self.__func = func

    def clone(self):
        return FunctionLocalObject(self.__func)

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.unserializable_object
        return object_proto.SerializeToString()

    def method_call(self, peer, args, kwds):
        return self.__func(peer, *args, **kwds)

    def method_is_true(self, peer):
        return True

class IntLocalObject(LocalObject):
    def __init__(self, int_value):
        self.__int_value = int_value

    def clone(self):
        return IntLocalObject(self.__int_value)

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.int_object.value = self.__int_value
        return object_proto.SerializeToString()

    def method_is_true(self, peer):
        return not not self.__int_value

class ListLocalObject(LocalObject):
    def __init__(self, peer_objects):
        self.__peer_objects = peer_objects[:]

    def clone(self):
        return ListLocalObject(self.__peer_objects)

    def serialize(self, context):
        object_proto = ObjectProto()
        list_object = object_proto.list_object

        for peer_object in self.__peer_objects:
            list_object.object_index.append(
                _get_serialization_index_for_peer_object(context, peer_object))

        return object_proto.SerializeToString()

    def method_is_true(self, peer):
        return not not self.__peer_objects

class ModuleLocalObject(LocalObject):
    def __init__(self):
        self.__w_dict = None

    def clone(self):
        return ModuleLocalObject()

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.unserializable_object
        return object_proto.SerializeToString()

    def method_getattr_w_dict(self, peer):
        if self.__w_dict is None:
            self.__w_dict = peer.create_peer_object(DictLocalObject())

        return self.__w_dict

    def method_is_true(self, peer):
        return True

class NoneLocalObject(LocalObject):
    def clone(self):
        return NoneLocalObject()

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.none_object
        return object_proto.SerializeToString()

    def method_is_true(self, peer):
        return False

class NotImplementedLocalObject(LocalObject):
    def clone(self):
        return NotImplementedLocalObject()

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.not_implemented_object
        return object_proto.SerializeToString()

    def method_is_true(self, peer):
        return True
