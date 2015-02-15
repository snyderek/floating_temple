from pypy.objspace.floating_temple import peer_ffi
from pypy.objspace.floating_temple.objspace import FloatingTempleObjSpace
from pypy.objspace.floating_temple.peer import Peer
from pypy.objspace.floating_temple.program_local_object import (
    ProgramLocalObject)
from rpython.rtyper.lltypesystem.lltype import free, malloc
from rpython.rtyper.lltypesystem.rffi import CArray, CArrayPtr, CCHARP, cast
import sys, unittest

class TestIntegration(unittest.TestCase):
    def setUp(self):
        peer_ffi.init_logging(sys.argv[0])

    def tearDown(self):
        peer_ffi.shut_down_logging()

    def test_simple(self):
        def main(peer):
            floating_space = FloatingTempleObjSpace(peer)

            peer.begin_transaction()
            obj = floating_space.wrap('Hello')
            peer.end_transaction()

            peer.begin_transaction()
            success = floating_space.is_true(obj)
            peer.end_transaction()

            assert success

        known_peer_ids = malloc(CArray(CCHARP), 0, flavor='raw')
        # TODO(dss): Randomly select an unused port instead of hard-coding the
        # port number.
        peer_struct = peer_ffi.create_network_peer(
            'pypy', 1025, 0, cast(CArrayPtr(CCHARP), known_peer_ids), 1)
        free(known_peer_ids, flavor='raw')

        peer = Peer(peer_struct)

        peer.run_program(ProgramLocalObject(main))

        peer.shut_down()
        peer_ffi.free_peer(peer_struct)

if __name__ == '__main__':
    unittest.main()
