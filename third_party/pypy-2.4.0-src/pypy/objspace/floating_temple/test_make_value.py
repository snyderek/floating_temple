from pypy.objspace.floating_temple import peer_ffi
from pypy.objspace.floating_temple.make_value import extract_value, make_value
from rpython.rtyper.lltypesystem.lltype import free, malloc
import sys, unittest

class TestMakeValue(unittest.TestCase):
    def setUp(self):
        peer_ffi.init_logging(sys.argv[0])
        self.value_ptr = malloc(peer_ffi.VALUE, flavor='raw')
        peer_ffi.init_value(self.value_ptr)

    def tearDown(self):
        peer_ffi.destroy_value(self.value_ptr)
        free(self.value_ptr, flavor='raw')
        peer_ffi.shut_down_logging()

    def test_none(self):
        make_value(None, self.value_ptr)
        self.assertIsNone(extract_value(self.value_ptr))

    def test_float(self):
        make_value(2.54, self.value_ptr)
        v = extract_value(self.value_ptr)
        self.assertEqual(type(v), float)
        self.assertAlmostEqual(v, 2.54)

    def test_int(self):
        make_value(25, self.value_ptr)
        v = extract_value(self.value_ptr)
        self.assertEqual(type(v), int)
        self.assertEqual(v, 25)

    def test_long(self):
        make_value(25L, self.value_ptr)
        v = extract_value(self.value_ptr)
        self.assertEqual(type(v), long)
        self.assertEqual(v, 25)

    def test_false(self):
        make_value(False, self.value_ptr)
        v = extract_value(self.value_ptr)
        self.assertIs(v, False)

    def test_true(self):
        make_value(True, self.value_ptr)
        v = extract_value(self.value_ptr)
        self.assertIs(v, True)

    def test_str(self):
        make_value("This is a test string.", self.value_ptr)
        v = extract_value(self.value_ptr)
        self.assertEqual(type(v), str)
        self.assertEqual(v, u"This is a test string.")

    def test_unicode(self):
        make_value(u"This is a test unicode string.", self.value_ptr)
        v = extract_value(self.value_ptr)
        self.assertEqual(type(v), unicode)
        self.assertEqual(v, u"This is a test unicode string.")

    # TODO(dss): Add a test that calls make_value with an instance of
    # PEER_OBJECT_P.

if __name__ == '__main__':
    unittest.main()
