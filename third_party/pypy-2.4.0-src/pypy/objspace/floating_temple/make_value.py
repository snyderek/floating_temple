from pypy.objspace.floating_temple import peer_ffi

LOCAL_TYPE_NONE = 0
LOCAL_TYPE_BOOL = 1
LOCAL_TYPE_INT = 2
LOCAL_TYPE_FLOAT = 3
LOCAL_TYPE_LONG = 4
LOCAL_TYPE_STR = 5
LOCAL_TYPE_UNICODE = 6
LOCAL_TYPE_OBJECT = 7

def make_value(obj, value_ptr):
    if obj is None:
        peer_ffi.set_value_empty(value_ptr, LOCAL_TYPE_NONE)
    elif isinstance(obj, float):
        peer_ffi.set_value_double(value_ptr, LOCAL_TYPE_FLOAT, obj)
    elif isinstance(obj, int):
        if isinstance(obj, bool):
            peer_ffi.set_value_bool(value_ptr, LOCAL_TYPE_BOOL, 1 if obj else 0)
        else:
            peer_ffi.set_value_int64(value_ptr, LOCAL_TYPE_INT, obj)
    elif isinstance(obj, long):
        # TODO(dss): Consider representing a long value as a sequence of bytes
        # in the case where the value is greater than the maximum 64-bit signed
        # integer.
        peer_ffi.set_value_int64(value_ptr, LOCAL_TYPE_LONG, obj)
    elif isinstance(obj, str):
        peer_ffi.set_value_bytes(value_ptr, LOCAL_TYPE_STR, obj, len(obj))
    elif isinstance(obj, unicode):
        encoded = obj.encode('utf_8')
        peer_ffi.set_value_string(value_ptr, LOCAL_TYPE_UNICODE, encoded,
                                  len(encoded))
    else:
        peer_ffi.set_value_peer_object(value_ptr, LOCAL_TYPE_OBJECT, obj)

def extract_value(value_ptr):
    local_type = peer_ffi.get_value_local_type(value_ptr)
    value_type = peer_ffi.get_value_type(value_ptr)

    if local_type == LOCAL_TYPE_NONE:
        assert value_type == 1  # VALUE_TYPE_EMPTY
        return None
    elif local_type == LOCAL_TYPE_BOOL:
        assert value_type == 6  # VALUE_TYPE_BOOL
        return peer_ffi.get_value_bool(value_ptr) != 0
    elif local_type == LOCAL_TYPE_INT:
        assert value_type == 4  # VALUE_TYPE_INT64
        return int(peer_ffi.get_value_int64(value_ptr))
    elif local_type == LOCAL_TYPE_FLOAT:
        assert value_type == 2  # VALUE_TYPE_DOUBLE
        return float(peer_ffi.get_value_double(value_ptr))
    elif local_type == LOCAL_TYPE_LONG:
        assert value_type == 4  # VALUE_TYPE_INT64
        return long(peer_ffi.get_value_int64(value_ptr))
    elif local_type == LOCAL_TYPE_STR:
        assert value_type == 8  # VALUE_TYPE_BYTES
        data = peer_ffi.get_value_bytes_data(value_ptr)
        length = peer_ffi.get_value_bytes_length(value_ptr)
        return ''.join([data[i] for i in range(length)])
    elif local_type == LOCAL_TYPE_UNICODE:
        assert value_type == 7  # VALUE_TYPE_STRING
        data = peer_ffi.get_value_string_data(value_ptr)
        length = peer_ffi.get_value_string_length(value_ptr)
        encoded = ''.join([data[i] for i in range(length)])
        return unicode(encoded.decode('utf_8'))
    elif local_type == LOCAL_TYPE_OBJECT:
        assert value_type == 9  # VALUE_TYPE_PEER_OBJECT
        return peer_ffi.get_value_peer_object(value_ptr)
    else:
        raise ValueError('Unexpected value local type: %d' % (local_type,))
