from pypy.objspace.floating_temple.local_object import LocalObject
from pypy.objspace.floating_temple.serialization_pb2 import ObjectProto

class ProgramLocalObject(LocalObject):
    def __init__(self, func, *args, **kwds):
        self.__func = func
        self.__args = args
        self.__kwds = kwds

    def clone(self):
        return ProgramLocalObject(self.__func, *self.__args, **self.__kwds)

    def serialize(self, context):
        object_proto = ObjectProto()
        object_proto.unserializable_object
        return object_proto.SerializeToString()

    def method_run(self, peer):
        return self.__func(peer, *self.__args, **self.__kwds)
