from rpython.rtyper.lltypesystem.lltype import Ptr, Void
from rpython.rtyper.lltypesystem.rffi import (CArrayPtr, CCHARP, CCallback,
    CFixedArray, CHAR, COpaquePtr, CStruct, DOUBLE, FLOAT, INT, LONGLONG,
    SIZE_T, ULONGLONG, VOIDP, llexternal)
from rpython.translator.tool.cbuild import ExternalCompilationInfo
import os

eci = ExternalCompilationInfo(
    includes=[
        'interpreter.h',
        'logging.h',
        'peer.h',
        'value.h',
    ],
    libraries=[
        'floating_temple',
    ],
    include_dirs=[os.path.abspath(os.environ['FLOATING_TEMPLE_INCLUDE_DIR'])],
    library_dirs=[os.path.abspath(os.environ['FLOATING_TEMPLE_LIB_DIR'])],
)

DESERIALIZATION_CONTEXT_P = COpaquePtr('floatingtemple_DeserializationContext',
                                       compilation_info=eci)
PEER_OBJECT_P = COpaquePtr('floatingtemple_PeerObject', compilation_info=eci)
PEER_P = COpaquePtr('floatingtemple_Peer', compilation_info=eci)
SERIALIZATION_CONTEXT_P = COpaquePtr('floatingtemple_SerializationContext',
                                     compilation_info=eci)
THREAD_P = COpaquePtr('floatingtemple_Thread', compilation_info=eci)

LOCAL_OBJECT = CStruct('floatingtemple_LocalObject')
LOCAL_OBJECT_P = Ptr(LOCAL_OBJECT)

VALUE = CStruct('floatingtemple_Value', ('padding', CFixedArray(CHAR, 24)))
VALUE_P = Ptr(VALUE)

clone_local_object_callback = CCallback([LOCAL_OBJECT_P], LOCAL_OBJECT_P)
serialize_local_object_callback = CCallback([LOCAL_OBJECT_P, VOIDP, SIZE_T,
                                             SERIALIZATION_CONTEXT_P],
                                            SIZE_T)
deserialize_object_callback = CCallback([LOCAL_OBJECT_P, VOIDP, SIZE_T,
                                         DESERIALIZATION_CONTEXT_P],
                                        LOCAL_OBJECT_P)
free_local_object_callback = CCallback([LOCAL_OBJECT_P], Void)
invoke_method_callback = CCallback([LOCAL_OBJECT_P, THREAD_P, PEER_OBJECT_P,
                                    CCHARP, INT, CArrayPtr(VALUE), VALUE_P],
                                   Void)

test_callback = CCallback([INT], Void)

INTERPRETER = CStruct(
    'floatingtemple_Interpreter',
    ('clone_local_object', clone_local_object_callback),
    ('serialize_local_object', serialize_local_object_callback),
    ('deserialize_object', deserialize_object_callback),
    ('free_local_object', free_local_object_callback),
    ('invoke_method', invoke_method_callback),
)
INTERPRETER_P = Ptr(INTERPRETER)

init_logging = llexternal(
    'floatingtemple_InitLogging',
    [CCHARP],
    Void,
    compilation_info=eci,
)

shut_down_logging = llexternal(
    'floatingtemple_ShutDownLogging',
    [],
    Void,
    compilation_info=eci,
)

create_network_peer = llexternal(
    'floatingtemple_CreateNetworkPeer',
    [CCHARP, INT, INT, CArrayPtr(CCHARP), INT],
    PEER_P,
    compilation_info=eci,
)

create_standalone_peer = llexternal(
    'floatingtemple_CreateStandalonePeer',
    [],
    PEER_P,
    compilation_info=eci,
)

run_program = llexternal(
    'floatingtemple_RunProgram',
    [INTERPRETER_P, PEER_P, LOCAL_OBJECT_P, CCHARP, VALUE_P],
    Void,
    compilation_info=eci,
)

stop_peer = llexternal(
    'floatingtemple_StopPeer',
    [PEER_P],
    Void,
    compilation_info=eci,
)

free_peer = llexternal(
    'floatingtemple_FreePeer',
    [PEER_P],
    Void,
    compilation_info=eci,
)

begin_transaction = llexternal(
    'floatingtemple_BeginTransaction',
    [THREAD_P],
    INT,
    compilation_info=eci,
)

end_transaction = llexternal(
    'floatingtemple_EndTransaction',
    [THREAD_P],
    INT,
    compilation_info=eci,
)

create_peer_object = llexternal(
    'floatingtemple_CreatePeerObject',
    [THREAD_P, LOCAL_OBJECT_P],
    PEER_OBJECT_P,
    compilation_info=eci,
)

get_or_create_named_object = llexternal(
    'floatingtemple_GetOrCreateNamedObject',
    [THREAD_P, CCHARP, LOCAL_OBJECT_P],
    PEER_OBJECT_P,
    compilation_info=eci,
)

call_method = llexternal(
    'floatingtemple_CallMethod',
    [INTERPRETER_P, THREAD_P, PEER_OBJECT_P, CCHARP, INT, CArrayPtr(VALUE),
     VALUE_P],
    INT,
    compilation_info=eci,
)

objects_are_equivalent = llexternal(
    'floatingtemple_ObjectsAreEquivalent',
    [THREAD_P, PEER_OBJECT_P, PEER_OBJECT_P],
    INT,
    compilation_info=eci,
)

get_serialization_index_for_peer_object = llexternal(
    'floatingtemple_GetSerializationIndexForPeerObject',
    [SERIALIZATION_CONTEXT_P, PEER_OBJECT_P],
    INT,
    compilation_info=eci,
)

get_peer_object_by_serialization_index = llexternal(
    'floatingtemple_GetPeerObjectBySerializationIndex',
    [DESERIALIZATION_CONTEXT_P, INT],
    PEER_OBJECT_P,
    compilation_info=eci,
)

poll_for_callback = llexternal(
    'floatingtemple_PollForCallback',
    [PEER_P, INTERPRETER_P],
    INT,
    compilation_info=eci,
)

init_value = llexternal(
    'floatingtemple_InitValue',
    [VALUE_P],
    Void,
    compilation_info=eci,
)

destroy_value = llexternal(
    'floatingtemple_DestroyValue',
    [VALUE_P],
    Void,
    compilation_info=eci,
)

init_value_array = llexternal(
    'floatingtemple_InitValueArray',
    [CArrayPtr(VALUE), INT],
    Void,
    compilation_info=eci,
)

get_value_local_type = llexternal(
    'floatingtemple_GetValueLocalType',
    [VALUE_P],
    INT,
    compilation_info=eci,
)

get_value_type = llexternal(
    'floatingtemple_GetValueType',
    [VALUE_P],
    INT,
    compilation_info=eci,
)

get_value_double = llexternal(
    'floatingtemple_GetValueDouble',
    [VALUE_P],
    DOUBLE,
    compilation_info=eci,
)

get_value_float = llexternal(
    'floatingtemple_GetValueFloat',
    [VALUE_P],
    FLOAT,
    compilation_info=eci,
)

get_value_int64 = llexternal(
    'floatingtemple_GetValueInt64',
    [VALUE_P],
    LONGLONG,
    compilation_info=eci,
)

get_value_uint64 = llexternal(
    'floatingtemple_GetValueUint64',
    [VALUE_P],
    ULONGLONG,
    compilation_info=eci,
)

get_value_bool = llexternal(
    'floatingtemple_GetValueBool',
    [VALUE_P],
    INT,
    compilation_info=eci,
)

get_value_string_data = llexternal(
    'floatingtemple_GetValueStringData',
    [VALUE_P],
    CCHARP,
    compilation_info=eci,
)

get_value_string_length = llexternal(
    'floatingtemple_GetValueStringLength',
    [VALUE_P],
    SIZE_T,
    compilation_info=eci,
)

get_value_bytes_data = llexternal(
    'floatingtemple_GetValueBytesData',
    [VALUE_P],
    CCHARP,
    compilation_info=eci,
)

get_value_bytes_length = llexternal(
    'floatingtemple_GetValueBytesLength',
    [VALUE_P],
    SIZE_T,
    compilation_info=eci,
)

get_value_peer_object = llexternal(
    'floatingtemple_GetValuePeerObject',
    [VALUE_P],
    PEER_OBJECT_P,
    compilation_info=eci,
)

set_value_empty = llexternal(
    'floatingtemple_SetValueEmpty',
    [VALUE_P, INT],
    Void,
    compilation_info=eci,
)

set_value_double = llexternal(
    'floatingtemple_SetValueDouble',
    [VALUE_P, INT, DOUBLE],
    Void,
    compilation_info=eci,
)

set_value_float = llexternal(
    'floatingtemple_SetValueFloat',
    [VALUE_P, INT, FLOAT],
    Void,
    compilation_info=eci,
)

set_value_int64 = llexternal(
    'floatingtemple_SetValueInt64',
    [VALUE_P, INT, LONGLONG],
    Void,
    compilation_info=eci,
)

set_value_uint64 = llexternal(
    'floatingtemple_SetValueUint64',
    [VALUE_P, INT, ULONGLONG],
    Void,
    compilation_info=eci,
)

set_value_bool = llexternal(
    'floatingtemple_SetValueBool',
    [VALUE_P, INT, INT],
    Void,
    compilation_info=eci,
)

set_value_string = llexternal(
    'floatingtemple_SetValueString',
    [VALUE_P, INT, CCHARP, SIZE_T],
    Void,
    compilation_info=eci,
)

set_value_bytes = llexternal(
    'floatingtemple_SetValueBytes',
    [VALUE_P, INT, CCHARP, SIZE_T],
    Void,
    compilation_info=eci,
)

set_value_peer_object = llexternal(
    'floatingtemple_SetValuePeerObject',
    [VALUE_P, INT, PEER_OBJECT_P],
    Void,
    compilation_info=eci,
)

assign_value = llexternal(
    'floatingtemple_AssignValue',
    [VALUE_P, VALUE_P],
    Void,
    compilation_info=eci,
)

test_function = llexternal(
    'floatingtemple_TestFunction',
    [INT, test_callback],
    Void,
    compilation_info=eci,
)
