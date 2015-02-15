from rpython.rtyper.lltypesystem.lltype import Ptr, Signed
from rpython.rtyper.lltypesystem.rffi import CStruct

# TODO(dss): These definitions can be moved to peer.py.

LOCAL_OBJECT_IMPL = CStruct(
    'LocalObjectImpl',
    ('object_id', Signed),
)
LOCAL_OBJECT_IMPL_P = Ptr(LOCAL_OBJECT_IMPL)
