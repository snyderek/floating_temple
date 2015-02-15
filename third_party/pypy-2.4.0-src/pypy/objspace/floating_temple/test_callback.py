from pypy.objspace.floating_temple import peer_ffi
from rpython.rtyper.annlowlevel import llhelper
import sys, unittest

def my_callback(n):
    print n
    if n > 0:
        callback = llhelper(peer_ffi.test_callback, my_callback)
        peer_ffi.test_function(n - 1, callback)

class TestCallback(unittest.TestCase):
    def setUp(self):
        peer_ffi.init_logging(sys.argv[0])

    def tearDown(self):
        peer_ffi.shut_down_logging()

    def test_simple(self):
        print
        callback = llhelper(peer_ffi.test_callback, my_callback)
        peer_ffi.test_function(20, callback)

if __name__ == '__main__':
    unittest.main()
