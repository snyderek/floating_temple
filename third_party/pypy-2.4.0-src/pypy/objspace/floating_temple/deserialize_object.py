from pypy.objspace.floating_temple import peer_ffi
from pypy.objspace.floating_temple.local_object import (BoolLocalObject,
    BytesLocalObject, IntLocalObject, ListLocalObject, NoneLocalObject)
from pypy.objspace.floating_temple.serialization_pb2 import ObjectProto
import functools

def _get_object_by_serialization_index(context, index):
    return peer_ffi.get_peer_object_by_serialization_index(context, index)

def deserialize_object(context, serialized_form):
    get_object_by_index = functools.partial(_get_object_by_serialization_index,
                                            context)

    object_proto = ObjectProto()
    object_proto.ParseFromString(serialized_form)

    if object_proto.HasField('bool_object'):
        return BoolLocalObject(object_proto.bool_object.value)

    if object_proto.HasField('bytearray_object'):
        raise NotImplementedError()

    if object_proto.HasField('complex_object'):
        raise NotImplementedError()

    if object_proto.HasField('dict_object'):
        raise NotImplementedError()

    if object_proto.HasField('float_object'):
        raise NotImplementedError()

    if object_proto.HasField('int_object'):
        return IntLocalObject(object_proto.int_object.value)

    if object_proto.HasField('list_object'):
        lst = []

        for object_index in object_proto.list_object.object_index:
            lst.append(get_object_by_index(object_index))

        return ListLocalObject(lst)

    if object_proto.HasField('long_object'):
        raise NotImplementedError()

    if object_proto.HasField('none_object'):
        return NoneLocalObject()

    if object_proto.HasField('object_object'):
        raise NotImplementedError()

    if object_proto.HasField('slice_object'):
        raise NotImplementedError()

    if object_proto.HasField('small_long_object'):
        raise NotImplementedError()

    if object_proto.HasField('string_object'):
        return BytesLocalObject(object_proto.string_object.value)

    if object_proto.HasField('tuple_object'):
        raise NotImplementedError()

    if object_proto.HasField('unicode_object'):
        raise NotImplementedError()

    if object_proto.HasField('unserializable_object'):
        raise RuntimeError('The object was not serializable.')

    print str(object_proto)
    assert False
    return None
