from pypy.conftest import option
from pypy.module.micronumpy.test.test_base import BaseNumpyAppTest
from pypy.interpreter.gateway import interp2app

class BaseAppTestDtypes(BaseNumpyAppTest):
    def setup_class(cls):
        BaseNumpyAppTest.setup_class.im_func(cls)
        if option.runappdirect:
            import platform
            bits, linkage = platform.architecture()
            ptr_size = int(bits[:-3]) // 8
        else:
            from rpython.rtyper.lltypesystem import rffi
            ptr_size = rffi.sizeof(rffi.CCHARP)
        cls.w_ptr_size = cls.space.wrap(ptr_size)

class AppTestDtypes(BaseAppTestDtypes):
    spaceconfig = dict(usemodules=["micronumpy", "struct", "binascii"])

    def test_typeinfo(self):
        import numpy as np
        try:
            from numpy.core.multiarray import typeinfo
        except ImportError:
            # running on dummy module
            from numpypy import typeinfo
        assert typeinfo['Number'] == np.number
        assert typeinfo['LONGLONG'] == ('q', 9, 64, 8, 9223372036854775807L,
                                        -9223372036854775808L, np.longlong)
        assert typeinfo['VOID'] == ('V', 20, 0, 1, np.void)
        assert typeinfo['BOOL'] == ('?', 0, 8, 1, 1, 0, np.bool_)
        assert typeinfo['CFLOAT'] == ('F', 14, 64, 8, np.complex64)
        assert typeinfo['CDOUBLE'] == ('D', 15, 128, 16, np.complex128)
        assert typeinfo['HALF'] == ('e', 23, 16, 2, np.float16)
        assert typeinfo['INTP'] == ('p', np.dtype('int').num,
                                    self.ptr_size*8, self.ptr_size,
                                    2**(self.ptr_size*8 - 1) - 1,
                                    -2**(self.ptr_size*8 - 1),
                                    np.dtype('int').type)

    def test_dtype_basic(self):
        from numpypy import dtype
        import sys

        d = dtype('?')
        assert d.num == 0
        assert d.kind == 'b'
        assert dtype(d) is d
        assert dtype('bool') is d
        assert dtype('bool_') is d
        assert dtype('|b1') is d
        b = '>' if sys.byteorder == 'little' else '<'
        assert dtype(b + 'i4') is not dtype(b + 'i4')
        assert repr(type(d)) == "<type 'numpy.dtype'>"
        exc = raises(ValueError, "d.names = []")
        assert exc.value[0] == "there are no fields defined"
        exc = raises(ValueError, "d.names = None")
        assert exc.value[0] == "there are no fields defined"

        assert dtype('int8').num == 1
        assert dtype('int8').name == 'int8'
        assert dtype('void').name == 'void'
        assert dtype(int).fields is None
        assert dtype(int).names is None
        assert dtype(int).hasobject is False
        assert dtype(int).subdtype is None
        assert dtype(str) is dtype('string') is dtype('string_')
        assert dtype(unicode) is dtype('unicode') is dtype('unicode_')

        assert dtype(None) is dtype(float)

        for d in [dtype('<c8'), dtype('>i4'), dtype('bool')]:
            for key in ["d[2]", "d['z']", "d[None]"]:
                exc = raises(KeyError, key)
                assert exc.value[0] == "There are no fields in dtype %s." % str(d)

        exc = raises(TypeError, dtype, (1, 2))
        assert exc.value[0] == 'data type not understood'
        exc = raises(TypeError, dtype, lambda: 42)
        assert exc.value[0] == 'data type not understood'
        exc = raises(TypeError, dtype, 'oooo')
        assert exc.value[0] == 'data type "oooo" not understood'
        raises(KeyError, 'dtype(int)["asdasd"]')

    def test_dtype_from_tuple(self):
        import numpy as np
        d = np.dtype((np.int64, 0))
        assert d == np.dtype(('i8', 0,))
        assert d.shape == (0,)
        d = np.dtype((np.int64, 1))
        assert d == np.dtype('i8')
        assert d.shape == ()
        d = np.dtype((np.int64, 1,))
        assert d == np.dtype('i8')
        assert d.shape == ()
        d = np.dtype((np.int64, 4))
        assert d == np.dtype(('i8', (4,)))
        assert d.shape == (4,)
        d = np.dtype((np.string_, 4))
        assert d == np.dtype('S4')
        assert d.shape == ()
        d = np.dtype(('S', 4))
        assert d == np.dtype('S4')
        assert d.shape == ()

    def test_dtype_eq(self):
        from numpypy import dtype

        assert dtype("int8") == "int8"
        assert "int8" == dtype("int8")
        raises(TypeError, lambda: dtype("int8") == 3)
        assert dtype(bool) == bool

    def test_dtype_aliases(self):
        from numpypy import dtype
        assert dtype('bool8') is dtype('bool')
        assert dtype('byte') is dtype('int8')
        assert dtype('ubyte') is dtype('uint8')
        assert dtype('short') is dtype('int16')
        assert dtype('ushort') is dtype('uint16')
        assert dtype('longlong') is dtype('q')
        assert dtype('ulonglong') is dtype('Q')
        assert dtype("float") is dtype(float)
        assert dtype('single') is dtype('float32')
        assert dtype('double') is dtype('float64')
        assert dtype('longfloat').num in (12, 13)
        assert dtype('longdouble').num in (12, 13)
        assert dtype('csingle') is dtype('complex64')
        assert dtype('cfloat') is dtype('complex128')
        assert dtype('cdouble') is dtype('complex128')
        assert dtype('clongfloat').num in (15, 16)
        assert dtype('clongdouble').num in (15, 16)

    def test_dtype_with_types(self):
        from numpypy import dtype

        assert dtype(bool).num == 0
        if self.ptr_size == 4:
            assert dtype('int32').num == 7
            assert dtype('uint32').num == 8
            assert dtype('int64').num == 9
            assert dtype('uint64').num == 10
        else:
            assert dtype('int32').num == 5
            assert dtype('uint32').num == 6
            assert dtype('int64').num == 7
            assert dtype('uint64').num == 8
        assert dtype(int).num == 7
        assert dtype('int').num == 7
        assert dtype('uint').num == 8
        assert dtype('intp').num == 7
        assert dtype('uintp').num == 8
        assert dtype(long).num == 9
        assert dtype(float).num == 12
        assert dtype('float').num == 12
        assert dtype('complex').num == 15

    def test_array_dtype_attr(self):
        from numpypy import array, dtype

        a = array(range(5), long)
        assert a.dtype is dtype(long)

    def test_isbuiltin(self):
        import numpy as np
        import sys
        assert np.dtype('?').isbuiltin == 1
        assert np.dtype(int).newbyteorder().isbuiltin == 0
        assert np.dtype(np.dtype(int)).isbuiltin == 1
        assert np.dtype('=i4').isbuiltin == 1
        b = '>' if sys.byteorder == 'little' else '<'
        assert np.dtype(b + 'i4').isbuiltin == 0
        assert np.dtype(b + 'i4').newbyteorder().isbuiltin == 0
        b = '<' if sys.byteorder == 'little' else '>'
        assert np.dtype(b + 'i4').isbuiltin == 1
        assert np.dtype(b + 'i4').newbyteorder().isbuiltin == 0
        assert np.dtype((int, 2)).isbuiltin == 0
        assert np.dtype([('', int), ('', float)]).isbuiltin == 0
        assert np.dtype('void').isbuiltin == 1
        assert np.dtype(str).isbuiltin == 1
        assert np.dtype('S0').isbuiltin == 1
        assert np.dtype('S5').isbuiltin == 0

    def test_repr_str(self):
        from numpypy import dtype
        b = dtype(int).newbyteorder().newbyteorder().byteorder
        assert '.dtype' in repr(dtype)
        d = dtype('?')
        assert repr(d) == "dtype('bool')"
        assert str(d) == "bool"
        d = dtype([('', '<f8')])
        assert repr(d) == "dtype([('f0', '<f8')])"
        assert str(d) == "[('f0', '<f8')]"
        d = dtype('S5')
        assert repr(d) == "dtype('S5')"
        assert str(d) == "|S5"
        d = dtype('a5')
        assert repr(d) == "dtype('S5')"
        assert str(d) == "|S5"
        d = dtype('U5')
        assert repr(d) == "dtype('%sU5')" % b
        assert str(d) == "%sU5" % b
        d = dtype(('<f8', 2))
        assert repr(d) == "dtype(('<f8', (2,)))"
        assert str(d) == "('<f8', (2,))"
        d = dtype('V16')
        assert repr(d) == "dtype('V16')"
        assert str(d) == "|V16"

    def test_bool_array(self):
        from numpypy import array, False_, True_

        a = array([0, 1, 2, 2.5], dtype='?')
        assert a[0] is False_
        for i in xrange(1, 4):
            assert a[i] is True_

    def test_copy_array_with_dtype(self):
        from numpypy import array, longlong, False_

        a = array([0, 1, 2, 3], dtype=long)
        # int on 64-bit, long in 32-bit
        assert isinstance(a[0], longlong)
        b = a.copy()
        assert isinstance(b[0], longlong)

        a = array([0, 1, 2, 3], dtype=bool)
        assert a[0] is False_
        b = a.copy()
        assert b[0] is False_

    def test_zeros_bool(self):
        from numpypy import zeros, False_

        a = zeros(10, dtype=bool)
        for i in range(10):
            assert a[i] is False_

    def test_ones_bool(self):
        from numpypy import ones, True_

        a = ones(10, dtype=bool)
        for i in range(10):
            assert a[i] is True_

    def test_zeros_long(self):
        from numpypy import zeros, longlong
        a = zeros(10, dtype=long)
        for i in range(10):
            assert isinstance(a[i], longlong)
            assert a[1] == 0

    def test_ones_long(self):
        from numpypy import ones, longlong
        a = ones(10, dtype=long)
        for i in range(10):
            assert isinstance(a[i], longlong)
            assert a[1] == 1

    def test_overflow(self):
        from numpypy import array, dtype
        assert array([128], 'b')[0] == -128
        assert array([256], 'B')[0] == 0
        assert array([32768], 'h')[0] == -32768
        assert array([65536], 'H')[0] == 0
        a = array([65520], dtype='float64')
        b = array(a, dtype='float16')
        assert b == float('inf')
        if dtype('l').itemsize == 4: # 32-bit
            raises(OverflowError, "array([2**32/2], 'i')")
            raises(OverflowError, "array([2**32], 'I')")
        raises(OverflowError, "array([2**64/2], 'q')")
        raises(OverflowError, "array([2**64], 'Q')")

    def test_bool_binop_types(self):
        from numpypy import array, dtype
        types = [
            '?', 'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'q', 'Q', 'f', 'd',
            'e'
        ]
        if array([0], dtype='longdouble').itemsize > 8:
            types += ['g', 'G']
        a = array([True], '?')
        for t in types:
            assert (a + array([0], t)).dtype is dtype(t)

    def test_binop_types(self):
        from numpypy import array, dtype
        tests = [('b','B','h'), ('b','h','h'), ('b','H','i'), ('b','i','i'),
                 ('b','l','l'), ('b','q','q'), ('b','Q','d'), ('B','h','h'),
                 ('B','H','H'), ('B','i','i'), ('B','I','I'), ('B','l','l'),
                 ('B','L','L'), ('B','q','q'), ('B','Q','Q'), ('h','H','i'),
                 ('h','i','i'), ('h','l','l'), ('h','q','q'), ('h','Q','d'),
                 ('H','i','i'), ('H','I','I'), ('H','l','l'), ('H','L','L'),
                 ('H','q','q'), ('H','Q','Q'), ('i','l','l'), ('i','q','q'),
                 ('i','Q','d'), ('I','L','L'), ('I','q','q'), ('I','Q','Q'),
                 ('q','Q','d'), ('b','f','f'), ('B','f','f'), ('h','f','f'),
                 ('H','f','f'), ('i','f','d'), ('I','f','d'), ('l','f','d'),
                 ('L','f','d'), ('q','f','d'), ('Q','f','d'), ('q','d','d')]
        if dtype('i').itemsize == dtype('l').itemsize: # 32-bit
            tests.extend([('b','I','q'), ('b','L','q'), ('h','I','q'),
                          ('h','L','q'), ('i','I','q'), ('i','L','q')])
        else:
            tests.extend([('b','I','l'), ('b','L','d'), ('h','I','l'),
                          ('h','L','d'), ('i','I','l'), ('i','L','d')])
        for d1, d2, dout in tests:
            # make a failed test print helpful info
            d3 = (array([1], d1) + array([1], d2)).dtype
            assert (d1, d2) == (d1, d2) and d3 is dtype(dout)

    def test_add(self):
        import numpypy as np
        for dtype in ["int8", "int16", "I"]:
            a = np.array(range(5), dtype=dtype)
            b = a + a
            assert b.dtype is np.dtype(dtype)
            for i in range(5):
                assert b[i] == i * 2

    def test_len(self):
        import numpy as np
        d = np.dtype('int32')
        assert len(d) == 0
        d = np.dtype([('x', 'i4'), ('y', 'i4')])
        assert len(d) == 2

    def test_shape(self):
        from numpypy import dtype
        assert dtype(long).shape == ()

    def test_cant_subclass(self):
        from numpypy import dtype
        # You can't subclass dtype
        raises(TypeError, type, "Foo", (dtype,), {})

    def test_can_subclass(self):
        import numpypy
        class xyz(numpypy.void):
            pass
        assert True

    def test_index(self):
        import numpypy as np
        for dtype in [np.int8, np.int16, np.int32, np.int64]:
            a = np.array(range(10), dtype=dtype)
            b = np.array([0] * 10, dtype=dtype)
            for idx in b:
                a[idx] += 1

    def test_hash(self):
        import numpypy as numpy
        for tp, value in [
            (numpy.int8, 4),
            (numpy.int16, 5),
            (numpy.uint32, 7),
            (numpy.int64, 3),
            (numpy.float16, 10.),
            (numpy.float32, 2.0),
            (numpy.float64, 4.32),
            (numpy.longdouble, 4.32),
        ]:
            assert hash(tp(value)) == hash(value)

        d1 = numpy.dtype([('f0', 'i4'), ('f1', 'i4')])
        d2 = numpy.dtype([('f0', 'i4'), ('f1', 'i4')])
        d3 = numpy.dtype([('f0', 'i4'), ('f2', 'i4')])
        d4 = numpy.dtype([('f0', 'i4'), ('f1', d1)])
        d5 = numpy.dtype([('f0', 'i4'), ('f1', d2)])
        d6 = numpy.dtype([('f0', 'i4'), ('f1', d3)])
        import sys
        assert hash(d1) == hash(d2)
        assert hash(d1) != hash(d3)
        assert hash(d4) == hash(d5)
        assert hash(d4) != hash(d6)

    def test_record_hash(self):
        from numpy import dtype
        # make sure the fields hash return different value
        # for different order of field in a structure

        # swap names
        t1 = dtype([('x', '<f4'), ('y', '<i4')])
        t2 = dtype([('y', '<f4'), ('x', '<i4')])
        assert hash(t1) != hash(t2)

        # swap types
        t3 = dtype([('x', '<f4'), ('y', '<i4')])
        t4 = dtype([('x', '<i4'), ('y', '<f4')])
        assert hash(t3) != hash(t4)

        # swap offsets
        t5 = dtype([('x', '<f4'), ('y', '<i4')])
        t6 = dtype([('y', '<i4'), ('x', '<f4')])
        assert hash(t5) != hash(t6)
    def test_pickle(self):
        import numpy as np
        from numpypy import array, dtype
        from cPickle import loads, dumps
        a = array([1,2,3])
        if self.ptr_size == 8:
            assert a.dtype.__reduce__() == (dtype, ('i8', 0, 1), (3, '<', None, None, None, -1, -1, 0))
        else:
            assert a.dtype.__reduce__() == (dtype, ('i4', 0, 1), (3, '<', None, None, None, -1, -1, 0))
        assert loads(dumps(a.dtype)) == a.dtype
        assert np.dtype('bool').__reduce__() == (dtype, ('b1', 0, 1), (3, '|', None, None, None, -1, -1, 0))
        assert np.dtype('|V16').__reduce__() == (dtype, ('V16', 0, 1), (3, '|', None, None, None, 16, 1, 0))
        assert np.dtype(('<f8', 2)).__reduce__() == (dtype, ('V16', 0, 1), (3, '|', (dtype('float64'), (2,)), None, None, 16, 1, 0))

    def test_newbyteorder(self):
        import numpypy as np
        import sys
        sys_is_le = sys.byteorder == 'little'
        native_code = sys_is_le and '<' or '>'
        swapped_code = sys_is_le and '>' or '<'
        native_dt = np.dtype(native_code+'i2')
        swapped_dt = np.dtype(swapped_code+'i2')
        assert native_dt.newbyteorder('S') == swapped_dt
        assert native_dt.newbyteorder() == swapped_dt
        assert native_dt == swapped_dt.newbyteorder('S')
        assert native_dt == swapped_dt.newbyteorder('=')
        assert native_dt == swapped_dt.newbyteorder('N')
        assert native_dt == native_dt.newbyteorder('|')
        assert np.dtype('<i2') == native_dt.newbyteorder('<')
        assert np.dtype('<i2') == native_dt.newbyteorder('L')
        assert np.dtype('>i2') == native_dt.newbyteorder('>')
        assert np.dtype('>i2') == native_dt.newbyteorder('B')

        for t in [np.int_, np.float_]:
            dt = np.dtype(t)
            dt1 = dt.newbyteorder().newbyteorder()
            assert dt.isbuiltin
            assert not dt1.isbuiltin
            dt2 = dt.newbyteorder("<")
            dt3 = dt.newbyteorder(">")
            assert dt.byteorder != dt1.byteorder
            assert hash(dt) == hash(dt1)
            if dt == dt2:
                assert dt.byteorder != dt2.byteorder
                assert hash(dt) == hash(dt2)
            else:
                assert dt.byteorder != dt3.byteorder
                assert hash(dt) == hash(dt3)

            exc = raises(ValueError, dt.newbyteorder, 'XX')
            assert exc.value[0] == 'XX is an unrecognized byteorder'

        for t in [np.int_, np.float_]:
            dt1 = np.dtype(t)
            dt2 = dt1.newbyteorder()
            s1 = np.array(123, dtype=dt1).tostring()
            s2 = np.array(123, dtype=dt2).byteswap().tostring()
            assert s1 == s2

        d = np.dtype([('', '<i8')]).newbyteorder()
        assert d.shape == ()
        assert d.names == ('f0',)
        #assert d.fields['f0'] == ('>i8', 0)
        assert d.subdtype is None
        #assert d.descr == [('f0', '>i8')]
        #assert str(d) == "[('f0', '>i8')]"
        d = np.dtype(('<i8', 2)).newbyteorder()
        assert d.shape == (2,)
        assert d.names is None
        assert d.fields is None
        assert d.descr == [('', '|V16')]
        #assert str(d) == "('>i8', (2,))"

    def test_object(self):
        import numpy as np
        import sys
        class O(object):
            pass
        for o in [object, O]:
            if '__pypy__' not in sys.builtin_module_names:
                assert np.dtype(o).str == '|O8'
            else:
                exc = raises(NotImplementedError, "np.dtype(o)")
                assert exc.value[0] == "cannot create dtype with type '%s'" % o.__name__

class AppTestTypes(BaseAppTestDtypes):
    def test_abstract_types(self):
        import numpypy as numpy

        raises(TypeError, numpy.generic, 0)
        raises(TypeError, numpy.number, 0)
        raises(TypeError, numpy.integer, 0)
        exc = raises(TypeError, numpy.signedinteger, 0)
        assert 'cannot create' in str(exc.value)
        assert 'signedinteger' in str(exc.value)
        exc = raises(TypeError, numpy.unsignedinteger, 0)
        assert 'cannot create' in str(exc.value)
        assert 'unsignedinteger' in str(exc.value)
        raises(TypeError, numpy.floating, 0)
        raises(TypeError, numpy.inexact, 0)

        # numpy allows abstract types in array creation
        a_n = numpy.array([4,4], numpy.number)
        a_f = numpy.array([4,4], numpy.floating)
        a_c = numpy.array([4,4], numpy.complexfloating)
        a_i = numpy.array([4,4], numpy.integer)
        a_s = numpy.array([4,4], numpy.signedinteger)
        a_u = numpy.array([4,4], numpy.unsignedinteger)

        assert a_n.dtype.num == 12
        assert a_f.dtype.num == 12
        assert a_c.dtype.num == 15
        assert a_i.dtype.num == 7
        assert a_s.dtype.num == 7
        assert a_u.dtype.num == 8

        assert a_n.dtype is numpy.dtype('float64')
        assert a_f.dtype is numpy.dtype('float64')
        assert a_c.dtype is numpy.dtype('complex128')
        if self.ptr_size == 4:
            assert a_i.dtype is numpy.dtype('int32')
            assert a_s.dtype is numpy.dtype('int32')
            assert a_u.dtype is numpy.dtype('uint32')
        else:
            assert a_i.dtype is numpy.dtype('int64')
            assert a_s.dtype is numpy.dtype('int64')
            assert a_u.dtype is numpy.dtype('uint64')

        # too ambitious for now
        #a = numpy.array('xxxx', numpy.generic)
        #assert a.dtype is numpy.dtype('|V4')

    def test_new(self):
        import numpypy as np
        assert np.int_(4) == 4
        assert np.float_(3.4) == 3.4

    def test_pow(self):
        from numpypy import int_
        assert int_(4) ** 2 == 16

    def test_bool(self):
        import numpypy as numpy

        assert numpy.bool_.mro() == [numpy.bool_, numpy.generic, object]
        assert numpy.bool_(3) is numpy.True_
        assert numpy.bool_("") is numpy.False_
        assert type(numpy.True_) is type(numpy.False_) is numpy.bool_

        class X(numpy.bool_):
            pass

        assert type(X(True)) is numpy.bool_
        assert X(True) is numpy.True_
        assert numpy.bool_("False") is numpy.True_

    def test_int8(self):
        import numpypy as numpy

        assert numpy.int8.mro() == [numpy.int8, numpy.signedinteger,
                                    numpy.integer, numpy.number,
                                    numpy.generic, object]

        a = numpy.array([1, 2, 3], numpy.int8)
        assert type(a[1]) is numpy.int8
        assert numpy.dtype("int8").type is numpy.int8

        x = numpy.int8(128)
        assert x == -128
        assert x != 128
        assert type(x) is numpy.int8
        assert repr(x) == "-128"

        assert type(int(x)) is int
        assert int(x) == -128
        assert numpy.int8('50') == numpy.int8(50)
        raises(ValueError, numpy.int8, '50.2')
        assert numpy.int8('127') == 127
        assert numpy.int8('128') == -128

    def test_uint8(self):
        import numpypy as numpy

        assert numpy.uint8.mro() == [numpy.uint8, numpy.unsignedinteger,
                                     numpy.integer, numpy.number,
                                     numpy.generic, object]

        a = numpy.array([1, 2, 3], numpy.uint8)
        assert type(a[1]) is numpy.uint8
        assert numpy.dtype("uint8").type is numpy.uint8

        x = numpy.uint8(128)
        assert x == 128
        assert x != -128
        assert type(x) is numpy.uint8
        assert repr(x) == "128"

        assert type(int(x)) is int
        assert int(x) == 128

        assert numpy.uint8(255) == 255
        assert numpy.uint8(256) == 0
        assert numpy.uint8('255') == 255
        assert numpy.uint8('256') == 0

    def test_int16(self):
        import numpypy as numpy

        x = numpy.int16(3)
        assert x == 3
        assert numpy.int16(32767) == 32767
        assert numpy.int16(32768) == -32768
        assert numpy.int16('32767') == 32767
        assert numpy.int16('32768') == -32768

    def test_uint16(self):
        import numpy
        assert numpy.uint16(65535) == 65535
        assert numpy.uint16(65536) == 0
        assert numpy.uint16('65535') == 65535
        assert numpy.uint16('65536') == 0

    def test_int32(self):
        import sys
        import numpy
        x = numpy.int32(23)
        assert x == 23
        assert numpy.int32(2147483647) == 2147483647
        assert numpy.int32('2147483647') == 2147483647
        if sys.maxint > 2 ** 31 - 1:
            assert numpy.int32(2147483648) == -2147483648
            assert numpy.int32('2147483648') == -2147483648
        else:
            raises(OverflowError, numpy.int32, 2147483648)
            raises(OverflowError, numpy.int32, '2147483648')
        assert numpy.dtype('int32') is numpy.dtype(numpy.int32)

    def test_uint32(self):
        import sys
        import numpy
        assert numpy.uint32(10) == 10
        if sys.maxint > 2 ** 31 - 1:
            assert numpy.uint32(4294967295) == 4294967295
            assert numpy.uint32(4294967296) == 0
            assert numpy.uint32('4294967295') == 4294967295
            assert numpy.uint32('4294967296') == 0

    def test_int_(self):
        import numpypy as numpy

        assert numpy.int_ is numpy.dtype(int).type
        assert numpy.int_.mro() == [numpy.int_, numpy.signedinteger,
                                    numpy.integer, numpy.number,
                                    numpy.generic, int, object]

    def test_int64(self):
        import sys
        import numpy
        if sys.maxint == 2 ** 63 -1:
            assert numpy.int64.mro() == [numpy.int64, numpy.signedinteger,
                                         numpy.integer, numpy.number,
                                         numpy.generic, int, object]
        else:
            assert numpy.int64.mro() == [numpy.int64, numpy.signedinteger,
                                         numpy.integer, numpy.number,
                                         numpy.generic, object]

        assert numpy.dtype(numpy.int64).type is numpy.int64
        assert numpy.int64(3) == 3

        assert numpy.int64(9223372036854775807) == 9223372036854775807
        assert numpy.int64(9223372036854775807) == 9223372036854775807
        raises(OverflowError, numpy.int64, 9223372036854775808)
        raises(OverflowError, numpy.int64, 9223372036854775808L)

    def test_uint64(self):
        import numpy
        assert numpy.dtype(numpy.uint64).type is numpy.uint64
        assert numpy.uint64.mro() == [numpy.uint64, numpy.unsignedinteger,
                                      numpy.integer, numpy.number,
                                      numpy.generic, object]
        import sys
        if '__pypy__' not in sys.builtin_module_names:
            # These tests pass "by chance" on numpy, things that are larger than
            # platform long (i.e. a python int), don't get put in a normal box,
            # instead they become an object array containing a long, we don't have
            # yet, so these can't pass.
            assert numpy.uint64(9223372036854775808) == 9223372036854775808
            assert numpy.uint64(18446744073709551615) == 18446744073709551615
        else:
            raises(OverflowError, numpy.int64, 9223372036854775808)
            raises(OverflowError, numpy.int64, 18446744073709551615)
        raises(OverflowError, numpy.uint64, 18446744073709551616)
        assert numpy.uint64((2<<63) - 1) == (2<<63) - 1

    def test_float16(self):
        import numpy
        assert numpy.float16.mro() == [numpy.float16, numpy.floating,
                                       numpy.inexact, numpy.number,
                                       numpy.generic, object]

        assert numpy.float16(12) == numpy.float64(12)
        assert numpy.float16('23.4') == numpy.float16(23.4)
        raises(ValueError, numpy.float16, '23.2df')


    def test_float32(self):
        import numpy
        assert numpy.float32.mro() == [numpy.float32, numpy.floating,
                                       numpy.inexact, numpy.number,
                                       numpy.generic, object]

        assert numpy.float32(12) == numpy.float64(12)
        assert numpy.float32('23.4') == numpy.float32(23.4)
        raises(ValueError, numpy.float32, '23.2df')

    def test_float64(self):
        import numpy
        assert numpy.float64.mro() == [numpy.float64, numpy.floating,
                                       numpy.inexact, numpy.number,
                                       numpy.generic, float, object]

        a = numpy.array([1, 2, 3], numpy.float64)
        assert type(a[1]) is numpy.float64
        assert numpy.dtype(float).type is numpy.float64

        assert "{:3f}".format(numpy.float64(3)) == "3.000000"

        assert numpy.float64(2.0) == 2.0
        assert numpy.float64('23.4') == numpy.float64(23.4)
        raises(ValueError, numpy.float64, '23.2df')

    def test_float_None(self):
        import numpy
        from math import isnan
        assert isnan(numpy.float32(None))
        assert isnan(numpy.float64(None))
        assert isnan(numpy.longdouble(None))

    def test_longfloat(self):
        import numpy
        # it can be float96 or float128
        if numpy.longfloat != numpy.float64:
            assert numpy.longfloat.mro()[1:] == [numpy.floating,
                                       numpy.inexact, numpy.number,
                                       numpy.generic, object]
        a = numpy.array([1, 2, 3], numpy.longdouble)
        assert type(a[1]) is numpy.longdouble
        assert numpy.float64(12) == numpy.longdouble(12)
        assert numpy.float64(12) == numpy.longfloat(12)
        raises(ValueError, numpy.longfloat, '23.2df')

    def test_complex_floating(self):
        import numpy
        assert numpy.complexfloating.__mro__ == (numpy.complexfloating,
            numpy.inexact, numpy.number, numpy.generic, object)

    def test_complex_format(self):
        import sys
        import numpypy as numpy

        for complex_ in (numpy.complex128, numpy.complex64,):
            for real, imag, should in [
                (1, 2, '(1+2j)'),
                (0, 1, '1j'),
                (1, 0, '(1+0j)'),
                (-1, -2, '(-1-2j)'),
                (0.5, -0.75, '(0.5-0.75j)'),
                #xxx
                #(numpy.inf, numpy.inf, '(inf+inf*j)'),
                ]:

                c = complex_(complex(real, imag))
                assert c == complex(real, imag)
                assert c.real == real
                assert c.imag == imag
                assert repr(c) == should

        real, imag, should = (1e100, 3e66, '(1e+100+3e+66j)')
        c128 = numpy.complex128(complex(real, imag))
        assert type(c128.real) is type(c128.imag) is numpy.float64
        assert c128.real == real
        assert c128.imag == imag
        assert repr(c128) == should

        c64 = numpy.complex64(complex(real, imag))
        assert repr(c64.real) == 'inf'
        assert type(c64.real) is type(c64.imag) is numpy.float32
        assert repr(c64.imag).startswith('inf')
        assert repr(c64) in ('(inf+inf*j)', '(inf+infj)')


        assert numpy.complex128(1.2) == numpy.complex128(complex(1.2, 0))
        assert numpy.complex64(1.2) == numpy.complex64(complex(1.2, 0))
        raises((ValueError, TypeError), numpy.array, [3+4j], dtype=float)
        if sys.version_info >= (2, 7):
            assert "{:g}".format(numpy.complex_(0.5+1.5j)) == '{:g}'.format(0.5+1.5j)

    def test_complex(self):
        import numpypy as numpy

        assert numpy.complex_ is numpy.complex128
        assert numpy.csingle is numpy.complex64
        assert numpy.cfloat is numpy.complex128
        assert numpy.complex64.__mro__ == (numpy.complex64,
            numpy.complexfloating, numpy.inexact, numpy.number, numpy.generic,
            object)
        assert numpy.complex128.__mro__ == (numpy.complex128,
            numpy.complexfloating, numpy.inexact, numpy.number, numpy.generic,
            complex, object)

        assert numpy.dtype(complex).type is numpy.complex128
        assert numpy.dtype("complex").type is numpy.complex128
        d = numpy.dtype('complex64')
        assert d.kind == 'c'
        assert d.num == 14
        assert d.char == 'F'

    def test_subclass_type(self):
        import numpypy as numpy

        class X(numpy.float64):
            def m(self):
                return self + 2

        b = X(10)
        assert type(b) is X
        assert b.m() == 12
        b = X(numpy.array([1, 2, 3]))
        assert type(b) is numpy.ndarray
        assert b.dtype.type is numpy.float64

    def test_long_as_index(self):
        from numpypy import int_, float64
        assert (1, 2, 3)[int_(1)] == 2
        raises(TypeError, lambda: (1, 2, 3)[float64(1)])

    def test_int(self):
        from numpypy import int32, int64, int_
        import sys
        assert issubclass(int_, int)
        if sys.maxint == (1<<31) - 1:
            assert issubclass(int32, int)
            assert int_ is int32
        else:
            assert issubclass(int64, int)
            assert int_ is int64

    def test_various_types(self):
        import numpypy as numpy

        assert numpy.int16 is numpy.short
        assert numpy.int8 is numpy.byte
        assert numpy.bool_ is numpy.bool8
        assert numpy.intp().dtype.num == 7
        assert numpy.intp().dtype.char == 'l'
        if self.ptr_size == 4:
            assert numpy.intp().dtype.name == 'int32'
            assert numpy.intp is numpy.int32
            assert numpy.uintp is numpy.uint32
        elif self.ptr_size == 8:
            assert numpy.intp().dtype.name == 'int64'
            assert numpy.intp is numpy.int64
            assert numpy.uintp is numpy.uint64

        assert issubclass(numpy.float64, numpy.floating)
        assert issubclass(numpy.longfloat, numpy.floating)
        assert not issubclass(numpy.float64, numpy.longfloat)
        assert not issubclass(numpy.longfloat, numpy.float64)

    def test_mro(self):
        import numpypy as numpy

        assert numpy.int16.__mro__ == (numpy.int16, numpy.signedinteger,
                                       numpy.integer, numpy.number,
                                       numpy.generic, object)
        assert numpy.bool_.__mro__ == (numpy.bool_, numpy.generic, object)

    def test_operators(self):
        from operator import truediv
        from numpypy import float64, int_, True_, False_
        assert 5 / int_(2) == int_(2)
        assert truediv(int_(3), int_(2)) == float64(1.5)
        assert truediv(3, int_(2)) == float64(1.5)
        assert int_(8) % int_(3) == int_(2)
        assert 8 % int_(3) == int_(2)
        assert divmod(int_(8), int_(3)) == (int_(2), int_(2))
        assert divmod(8, int_(3)) == (int_(2), int_(2))
        assert 2 ** int_(3) == int_(8)
        assert int_(3) << int_(2) == int_(12)
        assert 3 << int_(2) == int_(12)
        assert int_(8) >> int_(2) == int_(2)
        assert 8 >> int_(2) == int_(2)
        assert int_(3) & int_(1) == int_(1)
        assert 2 & int_(3) == int_(2)
        assert int_(2) | int_(1) == int_(3)
        assert 2 | int_(1) == int_(3)
        assert int_(3) ^ int_(5) == int_(6)
        assert True_ ^ False_ is True_
        assert 5 ^ int_(3) == int_(6)
        assert +int_(3) == int_(3)
        assert ~int_(3) == int_(-4)
        raises(TypeError, lambda: float64(3) & 1)

    def test_alternate_constructs(self):
        import numpy as np
        from numpypy import dtype
        nnp = self.non_native_prefix
        byteorder = self.native_prefix
        assert dtype('i8') == dtype(byteorder + 'i8') == dtype('=i8') == dtype(long)
        assert dtype(nnp + 'i8') != dtype('i8')
        assert dtype(nnp + 'i8').byteorder == nnp
        assert dtype('=i8').byteorder == '='
        assert dtype(byteorder + 'i8').byteorder == '='
        assert dtype(str).byteorder == '|'
        assert dtype('S5').byteorder == '|'
        assert dtype('>S5').byteorder == '|'
        assert dtype('<S5').byteorder == '|'
        assert dtype('<S5').newbyteorder('=').byteorder == '|'
        assert dtype('void').byteorder == '|'
        assert dtype((int, 2)).byteorder == '|'
        assert dtype(np.generic).str == '|V0'
        d = dtype(np.character)
        assert d.num == 18
        assert d.char == 'S'
        assert d.kind == 'S'
        assert d.str == '|S0'

    def test_dtype_str(self):
        from numpypy import dtype
        byteorder = self.native_prefix
        assert dtype('i8').str == byteorder + 'i8'
        assert dtype('<i8').str == '<i8'
        assert dtype('>i8').str == '>i8'
        assert dtype('int8').str == '|i1'
        assert dtype('float').str == byteorder + 'f8'
        assert dtype('f').str == byteorder + 'f4'
        assert dtype('=f').str == byteorder + 'f4'
        assert dtype('|f').str == byteorder + 'f4'
        assert dtype('>f').str == '>f4'
        assert dtype('<f').str == '<f4'
        assert dtype('d').str == byteorder + 'f8'
        assert dtype('=d').str == byteorder + 'f8'
        assert dtype('|d').str == byteorder + 'f8'
        assert dtype('>d').str == '>f8'
        assert dtype('<d').str == '<f8'
        # strange
        assert dtype('string').str == '|S0'
        assert dtype('unicode').str == byteorder + 'U0'
        assert dtype(('string', 7)).str == '|S7'
        assert dtype('=S5').str == '|S5'
        assert dtype(('unicode', 7)).str == '<U7'
        assert dtype([('', 'f8')]).str == "|V8"
        assert dtype(('f8', 2)).str == "|V16"

    def test_intp(self):
        from numpypy import dtype
        for s in ['p', 'int']:
            assert dtype(s) is dtype('intp')
        for s in ['P', 'uint']:
            assert dtype(s) is dtype('uintp')
        assert dtype('p').num == 7
        assert dtype('P').num == 8
        assert dtype('p').char == 'l'
        assert dtype('P').char == 'L'
        assert dtype('p').kind == 'i'
        assert dtype('P').kind == 'u'
        if self.ptr_size == 4:
            assert dtype('p').name == 'int32'
            assert dtype('P').name == 'uint32'
        else:
            assert dtype('p').name == 'int64'
            assert dtype('P').name == 'uint64'

    def test_alignment(self):
        from numpypy import dtype
        assert dtype('i4').alignment == 4

    def test_isnative(self):
        from numpypy import dtype
        assert dtype('i4').isnative == True
        assert dtype('>i8').isnative == False

    def test_any_all_nonzero(self):
        import numpypy as numpy
        x = numpy.bool_(True)
        assert x.any() is numpy.True_
        assert x.all() is numpy.True_
        assert x.__nonzero__() is True
        x = numpy.bool_(False)
        assert x.any() is numpy.False_
        assert x.all() is numpy.False_
        assert x.__nonzero__() is False
        x = numpy.float64(0)
        assert x.any() is numpy.False_
        assert x.all() is numpy.False_
        assert x.__nonzero__() is False
        x = numpy.complex128(0)
        assert x.any() is numpy.False_
        assert x.all() is numpy.False_
        assert x.__nonzero__() is False
        x = numpy.complex128(0+1j)
        assert x.any() is numpy.True_
        assert x.all() is numpy.True_
        assert x.__nonzero__() is True

    def test_ravel(self):
        from numpypy import float64, int8, array
        x = float64(42.5).ravel()
        assert x.dtype == float64
        assert (x == array([42.5])).all()
        #
        x = int8(42).ravel()
        assert x.dtype == int8
        assert (x == array(42)).all()

    def test_descr(self):
        import numpy as np
        assert np.dtype('<i8').descr == [('', '<i8')]
        assert np.dtype('|S4').descr == [('', '|S4')]
        assert np.dtype(('<i8', (5,))).descr == [('', '|V40')]
        d = [('test', '<i8'), ('blah', '<i2', (2, 3))]
        assert np.dtype(d).descr == d
        a = [('x', '<i8'), ('y', '<f8')]
        b = [('x', '<i4'), ('y', a)]
        assert np.dtype(b).descr == b
        assert np.dtype(('<f8', 2)).descr == [('', '|V16')]

class AppTestStrUnicodeDtypes(BaseNumpyAppTest):
    def test_mro(self):
        from numpypy import str_, unicode_, character, flexible, generic
        import sys
        if '__pypy__' in sys.builtin_module_names:
            assert str_.mro() == [str_, character, flexible, generic,
                                  str, basestring, object]
            assert unicode_.mro() == [unicode_, character, flexible, generic,
                                      unicode, basestring, object]
        else:
            assert str_.mro() == [str_, str, basestring, character, flexible,
                                  generic, object]
            assert unicode_.mro() == [unicode_, unicode, basestring, character,
                                      flexible, generic, object]

    def test_str_dtype(self):
        from numpypy import dtype, str_

        raises(TypeError, "dtype('Sx')")
        for t in ['S8', '|S8', '=S8']:
            d = dtype(t)
            assert d.itemsize == 8
            assert dtype(str) == dtype('S')
            assert d.kind == 'S'
            assert d.type is str_
            assert d.name == "string64"
            assert d.num == 18
        for i in [1, 2, 3]:
            d = dtype('c%d' % i)
            assert d.itemsize == 1
            assert d.kind == 'S'
            assert d.type is str_
            assert d.name == 'string8'
            assert d.num == 18
            assert d.str == '|S1'

    def test_unicode_dtype(self):
        from numpypy import dtype, unicode_

        raises(TypeError, "dtype('Ux')")
        d = dtype('U8')
        assert d.itemsize == 8 * 4
        assert dtype(unicode) == dtype('U')
        assert d.kind == 'U'
        assert d.type is unicode_
        assert d.name == "unicode256"
        assert d.num == 19

    def test_string_boxes(self):
        from numpypy import str_
        assert isinstance(str_(3), str_)

    def test_unicode_boxes(self):
        from numpypy import unicode_
        import sys
        if '__pypy__' in sys.builtin_module_names:
            exc = raises(NotImplementedError, unicode_, 3)
            assert exc.value.message.find('not supported yet') >= 0
        else:
            u = unicode_(3)
            assert isinstance(u, unicode)

    def test_character_dtype(self):
        import numpy as np
        from numpypy import array, character
        x = array([["A", "B"], ["C", "D"]], character)
        assert (x == [["A", "B"], ["C", "D"]]).all()
        d = np.dtype('c')
        assert d.num == 18
        assert d.char == 'c'
        assert d.kind == 'S'
        assert d.str == '|S1'

class AppTestRecordDtypes(BaseNumpyAppTest):
    spaceconfig = dict(usemodules=["micronumpy", "struct", "binascii"])

    def test_create(self):
        from numpypy import dtype, void

        raises(ValueError, "dtype([('x', int), ('x', float)])")
        d = dtype([("x", "<i4"), ("y", "<f4"), ("z", "<u2"), ("v", "<f8")])
        assert d.fields['x'] == (dtype('<i4'), 0)
        assert d.fields['v'] == (dtype('<f8'), 10)
        assert d['x'] == dtype('<i4')
        assert d.name == "void144"
        assert d.num == 20
        assert d.itemsize == 18
        assert d.kind == 'V'
        assert d.base == d
        assert d.type is void
        assert d.char == 'V'
        exc = raises(ValueError, "d.names = None")
        assert exc.value[0] == 'must replace all names at once with a sequence of length 4'
        exc = raises(ValueError, "d.names = (a for a in 'xyzv')")
        assert exc.value[0] == 'must replace all names at once with a sequence of length 4'
        exc = raises(ValueError, "d.names = ('a', 'b', 'c', 4)")
        assert exc.value[0] == 'item #3 of names is of type int and not string'
        exc = raises(ValueError, "d.names = ('a', 'b', 'c', u'd')")
        assert exc.value[0] == 'item #3 of names is of type unicode and not string'
        assert d.names == ("x", "y", "z", "v")
        d.names = ('x', '', 'v', 'z')
        assert d.names == ('x', '', 'v', 'z')
        assert d.fields['v'] == (dtype('<u2'), 8)
        assert d.fields['z'] == (dtype('<f8'), 10)
        assert [a[0] for a in d.descr] == ['x', '', 'v', 'z']
        exc = raises(ValueError, "d.names = ('a', 'b', 'c')")
        assert exc.value[0] == 'must replace all names at once with a sequence of length 4'
        d.names = ['a', 'b', 'c', 'd']
        assert d.names == ('a', 'b', 'c', 'd')
        exc = raises(ValueError, "d.names = ('a', 'b', 'c', 'c')")
        assert exc.value[0] == "Duplicate field names given."
        exc = raises(AttributeError, 'del d.names')
        assert exc.value[0] == "Cannot delete dtype names attribute"
        assert d.names == ('a', 'b', 'c', 'd')
        raises(KeyError, 'd["xyz"]')
        raises(KeyError, 'd.fields["xyz"]')
        d = dtype([('', '<i8'), ('', '<f8')])
        assert d.descr == [('f0', '<i8'), ('f1', '<f8')]
        d = dtype([('', '<i8'), ('b', '<f8')])
        assert d.descr == [('f0', '<i8'), ('b', '<f8')]
        d = dtype([('a', '<i8'), ('', '<f8')])
        assert d.descr == [('a', '<i8'), ('f1', '<f8')]
        exc = raises(ValueError, "dtype([('a', '<i8'), ('a', '<f8')])")
        assert exc.value[0] == 'two fields with the same name'

    def test_create_from_dict(self):
        import numpy as np
        import sys
        d = {'names': ['r','g','b','a'],
             'formats': [np.uint8, np.uint8, np.uint8, np.uint8]}
        if '__pypy__' not in sys.builtin_module_names:
            dt = np.dtype(d)
        else:
            raises(NotImplementedError, np.dtype, d)

    def test_create_subarrays(self):
        from numpypy import dtype
        d = dtype([("x", "float", (2,)), ("y", "int64", (2,))])
        assert d.itemsize == 32
        assert d.name == "void256"
        keys = d.fields.keys()
        assert "x" in keys
        assert "y" in keys
        assert d["x"].shape == (2,)
        assert d["x"].itemsize == 16
        e = dtype([("x", "float", 2), ("y", "int", 2)])
        assert e.fields.keys() == keys
        for v in ['x', u'x', 0, -2]:
            assert e[v] == (dtype('float'), (2,))
        for v in ['y', u'y', 1, -1]:
            assert e[v] == (dtype('int'), (2,))
        for v in [-3, 2]:
            exc = raises(IndexError, "e[%d]" % v)
            assert exc.value.message == "Field index %d out of range." % v
        exc = raises(KeyError, "e['z']")
        assert exc.value.message == "Field named 'z' not found."
        exc = raises(ValueError, "e[None]")
        assert exc.value.message == 'Field key must be an integer, string, or unicode.'

        dt = dtype((float, 10))
        assert dt.shape == (10,)
        assert dt.kind == 'V'
        assert dt.fields == None
        assert dt.subdtype == (dtype(float), (10,))
        assert dt.base == dtype(float)

    def test_setstate(self):
        import numpy as np
        import sys
        d = np.dtype('f8')
        d.__setstate__((3, '|', (np.dtype('float64'), (2,)), None, None, 20, 1, 0))
        assert d.str == ('<' if sys.byteorder == 'little' else '>') + 'f8'
        assert d.fields is None
        assert d.shape == ()
        assert d.itemsize == 8
        assert d.subdtype is None
        assert repr(d) == "dtype('float64')"

        d = np.dtype(('>' if sys.byteorder == 'little' else '<') + 'f8')
        d.__setstate__((3, '|', (np.dtype('float64'), (2,)), None, None, 20, 1, 0))
        assert d.str == '|f8'
        assert d.fields is None
        assert d.shape == (2,)
        assert d.itemsize == 8
        assert d.subdtype is not None
        assert repr(d) == "dtype(('<f8', (2,)))"

        d = np.dtype(('<f8', 2))
        assert d.fields is None
        assert d.shape == (2,)
        assert d.itemsize == 16
        assert d.subdtype is not None
        assert repr(d) == "dtype(('<f8', (2,)))"

        d = np.dtype(('<f8', 2))
        d.__setstate__((3, '|', (np.dtype('float64'), (2,)), None, None, 20, 1, 0))
        assert d.fields is None
        assert d.shape == (2,)
        assert d.itemsize == 20
        assert d.subdtype is not None
        assert repr(d) == "dtype(('<f8', (2,)))"

        d = np.dtype(('<f8', 2))
        d.__setstate__((3, '|', (np.dtype('float64'), 2), None, None, 20, 1, 0))
        assert d.fields is None
        assert d.shape == (2,)
        assert d.itemsize == 20
        assert d.subdtype is not None
        assert repr(d) == "dtype(('<f8', (2,)))"

        d = np.dtype(('<f8', 2))
        exc = raises(ValueError, "d.__setstate__((3, '|', None, ('f0', 'f1'), None, 16, 1, 0))")
        assert exc.value[0] == 'inconsistent fields and names'
        assert d.fields is None
        assert d.shape == (2,)
        assert d.subdtype is not None
        assert repr(d) == "dtype(('<f8', (2,)))"

        d = np.dtype(('<f8', 2))
        exc = raises(ValueError, "d.__setstate__((3, '|', None, None, {'f0': (np.dtype('float64'), 0), 'f1': (np.dtype('float64'), 8)}, 16, 1, 0))")
        assert exc.value[0] == 'inconsistent fields and names'
        assert d.fields is None
        assert d.shape == (2,)
        assert d.subdtype is not None
        assert repr(d) == "dtype(('<f8', (2,)))"

        d = np.dtype(('<f8', 2))
        exc = raises(ValueError, "d.__setstate__((3, '|', (np.dtype('float64'), (2,), 3), ('f0', 'f1'), {'f0': (np.dtype('float64'), 0), 'f1': (np.dtype('float64'), 8)}, 16, 1, 0))")
        assert exc.value[0] == 'incorrect subarray in __setstate__'
        assert d.fields is None
        assert d.shape == ()
        assert d.subdtype is None
        assert repr(d) == "dtype('V16')"

        d = np.dtype(('<f8', 2))
        d.__setstate__((3, '|', (np.dtype('float64'), (2,)), ('f0', 'f1'), {'f0': (np.dtype('float64'), 0), 'f1': (np.dtype('float64'), 8)}, 16, 1, 0))
        assert d.fields is not None
        assert d.shape == (2,)
        assert d.subdtype is not None
        assert repr(d) == "dtype([('f0', '<f8'), ('f1', '<f8')])"

        d = np.dtype(('<f8', 2))
        d.__setstate__((3, '|', None, ('f0', 'f1'), {'f0': (np.dtype('float64'), 0), 'f1': (np.dtype('float64'), 8)}, 16, 1, 0))
        assert d.fields is not None
        assert d.shape == ()
        assert d.subdtype is None
        assert repr(d) == "dtype([('f0', '<f8'), ('f1', '<f8')])"

        d = np.dtype(('<f8', 2))
        d.__setstate__((3, '|', None, ('f0', 'f1'), {'f0': (np.dtype('float64'), 0), 'f1': (np.dtype('float64'), 8)}, 16, 1, 0))
        d.__setstate__((3, '|', (np.dtype('float64'), (2,)), None, None, 16, 1, 0))
        assert d.fields is not None
        assert d.shape == (2,)
        assert d.subdtype is not None
        assert repr(d) == "dtype([('f0', '<f8'), ('f1', '<f8')])"

    def test_pickle_record(self):
        from numpypy import array, dtype
        from cPickle import loads, dumps

        d = dtype([("x", "int32"), ("y", "int32"), ("z", "int32"), ("value", float)])
        assert d.__reduce__() == (dtype, ('V20', 0, 1), (3, '|', None, ('x', 'y', 'z', 'value'), {'y': (dtype('int32'), 4), 'x': (dtype('int32'), 0), 'z': (dtype('int32'), 8), 'value': (dtype('float64'), 12)}, 20, 1, 0))

        new_d = loads(dumps(d))

        assert new_d.__reduce__() == d.__reduce__()

    def test_pickle_record_subarrays(self):
        from numpypy import array, dtype
        from cPickle import loads, dumps

        d = dtype([("x", "int32", (3,)), ("y", "int32", (2,)), ("z", "int32", (4,)), ("value", float, (5,))])
        new_d = loads(dumps(d))

        keys = d.fields.keys()
        keys.sort()
        assert keys == ["value", "x", "y", "z"]

        assert new_d.itemsize == d.itemsize == 76

class AppTestNotDirect(BaseNumpyAppTest):
    def setup_class(cls):
        BaseNumpyAppTest.setup_class.im_func(cls)
        def check_non_native(w_obj, w_obj2):
            stor1 = w_obj.implementation.storage
            stor2 = w_obj2.implementation.storage
            assert stor1[0] == stor2[1]
            assert stor1[1] == stor2[0]
            if stor1[0] == '\x00':
                assert stor2[1] == '\x00'
                assert stor2[0] == '\x01'
            else:
                assert stor2[1] == '\x01'
                assert stor2[0] == '\x00'
        if option.runappdirect:
            cls.w_check_non_native = lambda *args : None
        else:
            cls.w_check_non_native = cls.space.wrap(interp2app(check_non_native))

    def test_non_native(self):
        from numpypy import array
        a = array([1, 2, 3], dtype=self.non_native_prefix + 'i2')
        assert a[0] == 1
        assert (a + a)[1] == 4
        self.check_non_native(a, array([1, 2, 3], 'i2'))
        a = array([1, 2, 3], dtype=self.non_native_prefix + 'f8')
        assert a[0] == 1
        assert (a + a)[1] == 4
        a = array([1, 2, 3], dtype=self.non_native_prefix + 'f4')
        assert a[0] == 1
        assert (a + a)[1] == 4
        a = array([1, 2, 3], dtype=self.non_native_prefix + 'f2')
        assert a[0] == 1
        assert (a + a)[1] == 4
        a = array([1, 2, 3], dtype=self.non_native_prefix + 'g') # longdouble
        assert a[0] == 1
        assert (a + a)[1] == 4
        a = array([1, 2, 3], dtype=self.non_native_prefix + 'G') # clongdouble
        assert a[0] == 1
        assert (a + a)[1] == 4

class AppTestObjectDtypes(BaseNumpyAppTest):
    def test_scalar_from_object(self):
        from numpypy import array
        import sys
        class Polynomial(object):
            pass
        if '__pypy__' in sys.builtin_module_names:
            exc = raises(NotImplementedError, array, Polynomial())
            assert exc.value.message.find('unable to create dtype from objects') >= 0
        else:
            a = array(Polynomial())
            assert a.shape == ()
