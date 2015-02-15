from pypy.objspace.floating_temple import peer_ffi
from pypy.objspace.floating_temple.deserialize_object import deserialize_object
from pypy.objspace.floating_temple.make_value import extract_value, make_value
from pypy.objspace.floating_temple.local_object_impl import (LOCAL_OBJECT_IMPL,
    LOCAL_OBJECT_IMPL_P)
from pypy.objspace.floating_temple.string_util import CStringToPythonString
from rpython.rlib.objectmodel import compute_unique_id
from rpython.rtyper.annlowlevel import llhelper
from rpython.rtyper.lltypesystem.lltype import free, malloc
from rpython.rtyper.lltypesystem.rffi import CArray, SIZE_T, cast
import sys, threading, time, traceback

class PeerConflictException(Exception):
    pass

# TODO(dss): Make this class thread-safe.
class Peer(object):
    def __init__(self, peer_struct):
        self.__peer_struct = peer_struct

        self.__shutting_down = False
        self.__thread_local = threading.local()

        # TODO(dss): Find a way to make LOCAL_OBJECT_IMPL reference the garbage-
        # collected object directly, instead of using this map.
        self.__local_objects = {}

        self.__interpreter = malloc(peer_ffi.INTERPRETER, flavor='raw')

        self.__interpreter.c_clone_local_object = llhelper(
            peer_ffi.clone_local_object_callback, self.__clone_local_object)
        self.__interpreter.c_serialize_local_object = llhelper(
            peer_ffi.serialize_local_object_callback,
            self.__serialize_local_object)
        self.__interpreter.c_deserialize_object = llhelper(
            peer_ffi.deserialize_object_callback, self.__deserialize_object)
        self.__interpreter.c_free_local_object = llhelper(
            peer_ffi.free_local_object_callback, self.__free_local_object)
        self.__interpreter.c_invoke_method = llhelper(
            peer_ffi.invoke_method_callback, self.__invoke_method)

        self.__poll_thread = threading.Thread(target=self.__poll_for_callbacks,
                                              name='poll_thread')
        self.__poll_thread.start()

    def shut_down(self):
        peer_ffi.stop_peer(self.__peer_struct)
        self.__shutting_down = True
        self.__poll_thread.join()
        free(self.__interpreter, flavor='raw')

    def __get_peer_thread(self):
        return self.__thread_local.__dict__.get('peer_thread')

    def __set_peer_thread(self, peer_thread):
        if peer_thread is None:
            try:
                del self.__thread_local.__dict__['peer_thread']
            except KeyError:
                pass
        else:
            self.__thread_local.__dict__['peer_thread'] = peer_thread

    def __substitute_peer_thread(self, new_peer_thread):
        class ThreadSubstitution(object):
            def __init__(self, peer, new_peer_thread):
                self.__peer = peer
                self.__new_peer_thread = new_peer_thread

            def __enter__(self):
                self.__old_peer_thread = self.__peer._Peer__get_peer_thread()
                self.__peer._Peer__set_peer_thread(self.__new_peer_thread)
                return None

            def __exit__(self, type, value, tv):
                self.__peer._Peer__set_peer_thread(self.__old_peer_thread)

        return ThreadSubstitution(self, new_peer_thread)

    def __make_local_object(self, obj):
        object_id = compute_unique_id(obj)

        assert self.__local_objects.setdefault(object_id, obj) is obj

        local_object = malloc(LOCAL_OBJECT_IMPL, flavor='raw')
        local_object.c_object_id = object_id

        return cast(peer_ffi.LOCAL_OBJECT_P, local_object)

    def __get_object(self, local_object_raw):
        local_object = cast(LOCAL_OBJECT_IMPL_P, local_object_raw)
        return self.__local_objects[local_object.c_object_id]

    def __poll_for_callbacks(self):
        while not self.__shutting_down:
            # TODO(dss): If self.__shutting_down is set to True, signal
            # peer_ffi.poll_for_callback to return immediately.
            if peer_ffi.poll_for_callback(self.__peer_struct,
                                          self.__interpreter) == 0:
                time.sleep(0.01)

    def __clone_local_object(self, local_object_raw):
        obj = self.__get_object(local_object_raw)
        clone_obj = obj.clone()
        return self.__make_local_object(clone_obj)

    def __serialize_local_object(self, local_object_raw, buf, buf_size,
                                 context):
        obj = self.__get_object(local_object_raw)
        serialized_form = obj.serialize(context)
        length = len(serialized_form)

        if length <= buf_size:
            for i in range(length):
                buf[i] = serialized_form[i]

        return cast(SIZE_T, length)

    def __deserialize_object(self, buf, buf_size, context):
        serialized_form = ''.join([buf[i] for i in range(buf_size)])
        return deserialize_object(context, serialized_form)

    def __free_local_object(self, local_object_raw):
        local_object = cast(LOCAL_OBJECT_IMPL_P, local_object_raw)
        del self.__local_objects[local_object.c_object_id]
        free(local_object, flavor='raw')

    def __invoke_method(self, local_object_raw, thread, peer_object,
                        method_name, parameter_count, parameter_values,
                        return_value):
        obj = self.__get_object(local_object_raw)
        method_name_str = CStringToPythonString(method_name)
        params = [extract_value(parameter_values[i])
                  for i in range(parameter_count)]

        with self.__substitute_peer_thread(thread):
            try:
                return_obj = obj.invoke_method(self, method_name_str, params)
            except:
                traceback.print_exc()
                print 'Unexpected error:', sys.exc_info()[0]
                raise

        make_value(return_obj, return_value)

    def run_program(self, program_object):
        return_value = malloc(peer_ffi.VALUE, flavor='raw')
        peer_ffi.init_value(return_value)

        peer_ffi.run_program(self.__interpreter, self.__peer_struct,
                             self.__make_local_object(program_object), 'run',
                             return_value)

        return_obj = extract_value(return_value)

        peer_ffi.destroy_value(return_value)
        free(return_value, flavor='raw')

        return return_obj

    def objects_are_equivalent(self, peer_object1, peer_object2):
        return peer_ffi.objects_are_equivalent(self.__get_peer_thread(),
                                               peer_object1, peer_object2) != 0

    def begin_transaction(self):
        # TODO(dss): Raise PeerConflictException if peer_ffi.begin_transaction
        # returns 0.
        return peer_ffi.begin_transaction(self.__get_peer_thread()) != 0

    def end_transaction(self):
        return peer_ffi.end_transaction(self.__get_peer_thread()) != 0

    def create_peer_object(self, initial_version):
        local_object = self.__make_local_object(initial_version)

        return peer_ffi.create_peer_object(self.__get_peer_thread(),
                                           local_object)

    def get_or_create_named_object(self, name, initial_version):
        candidate_local_object = self.__make_local_object(initial_version)

        return peer_ffi.get_or_create_named_object(self.__get_peer_thread(),
                                                   name, candidate_local_object)

    # TODO(dss): Rename this method to distinguish it from the unrelated method
    # StdObjSpace.call_method.
    def call_method(self, peer_object, method_name, parameters):
        parameter_count = len(parameters)

        param_values = malloc(CArray(peer_ffi.VALUE), parameter_count,
                              flavor='raw')
        peer_ffi.init_value_array(param_values, parameter_count)
        for param, value_ptr in zip(parameters, param_values):
            make_value(param, value_ptr)

        return_value = malloc(peer_ffi.VALUE, flavor='raw')
        peer_ffi.init_value(return_value)

        if peer_ffi.call_method(self.__interpreter, self.__get_peer_thread(),
                                peer_object, method_name, parameter_count,
                                param_values, return_value) == 0:
            raise PeerConflictException()

        return_obj = extract_value(return_value)

        peer_ffi.destroy_value(return_value)
        free(return_value, flavor='raw')

        for value_ptr in param_values:
            peer_ffi.destroy_value(value_ptr)
        free(param_values, flavor='raw')

        return return_obj
