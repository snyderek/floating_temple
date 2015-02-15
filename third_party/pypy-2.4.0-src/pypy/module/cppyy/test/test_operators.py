import py, os, sys


currpath = py.path.local(__file__).dirpath()
test_dct = str(currpath.join("operatorsDict.so"))

def setup_module(mod):
    if sys.platform == 'win32':
        py.test.skip("win32 not supported so far")
    err = os.system("cd '%s' && make operatorsDict.so" % currpath)
    if err:
        raise OSError("'make' failed (see stderr)")

class AppTestOPERATORS:
    spaceconfig = dict(usemodules=['cppyy', '_rawffi', 'itertools'])

    def setup_class(cls):
        cls.w_N = cls.space.wrap(5)  # should be imported from the dictionary
        cls.w_test_dct  = cls.space.wrap(test_dct)
        cls.w_operators = cls.space.appexec([], """():
            import cppyy
            return cppyy.load_reflection_info(%r)""" % (test_dct, ))

    def teardown_method(self, meth):
        import gc
        gc.collect()

    def test01_math_operators(self):
        """Test overloading of math operators"""

        import cppyy
        number = cppyy.gbl.number

        assert (number(20) + number(10)) == number(30)
        assert (number(20) + 10        ) == number(30)
        assert (number(20) - number(10)) == number(10)
        assert (number(20) - 10        ) == number(10)
        assert (number(20) / number(10)) == number(2)
        assert (number(20) / 10        ) == number(2)
        assert (number(20) * number(10)) == number(200)
        assert (number(20) * 10        ) == number(200)
        assert (number(20) % 10        ) == number(0)
        assert (number(20) % number(10)) == number(0)
        assert (number(5)  & number(14)) == number(4)
        assert (number(5)  | number(14)) == number(15)
        assert (number(5)  ^ number(14)) == number(11)
        assert (number(5)  << 2) == number(20)
        assert (number(20) >> 2) == number(5)

    def test02_unary_math_operators(self):
        """Test overloading of unary math operators"""

        import cppyy
        number = cppyy.gbl.number

        n  = number(20)
        n += number(10)
        n -= number(10)
        n *= number(10)
        n /= number(2)
        assert n == number(100)

        nn = -n;
        assert nn == number(-100)

    def test03_comparison_operators(self):
        """Test overloading of comparison operators"""

        import cppyy
        number = cppyy.gbl.number

        assert (number(20) >  number(10)) == True
        assert (number(20) <  number(10)) == False
        assert (number(20) >= number(20)) == True
        assert (number(20) <= number(10)) == False
        assert (number(20) != number(10)) == True
        assert (number(20) == number(10)) == False

    def test04_boolean_operator(self):
        """Test implementation of operator bool"""

        import cppyy
        number = cppyy.gbl.number

        n = number(20)
        assert n

        n = number(0)
        assert not n

    def test05_exact_types(self):
        """Test converter operators of exact types"""

        import cppyy
        gbl = cppyy.gbl

        o = gbl.operator_char_star()
        assert o.m_str == 'operator_char_star'
        assert str(o)  == 'operator_char_star'

        o = gbl.operator_const_char_star()
        assert o.m_str == 'operator_const_char_star'
        assert str(o)  == 'operator_const_char_star'

        o = gbl.operator_int(); o.m_int = -13
        assert o.m_int == -13
        assert int(o)  == -13

        o = gbl.operator_long(); o.m_long = 42
        assert o.m_long == 42
        assert long(o)  == 42

        o = gbl.operator_double(); o.m_double = 3.1415
        assert o.m_double == 3.1415
        assert float(o)   == 3.1415

    def test06_approximate_types(self):
        """Test converter operators of approximate types"""

        import cppyy, sys
        gbl = cppyy.gbl

        o = gbl.operator_short(); o.m_short = 256
        assert o.m_short == 256
        assert int(o)    == 256

        o = gbl.operator_unsigned_int(); o.m_uint = 2147483647 + 32
        assert o.m_uint == 2147483647 + 32
        assert long(o)  == 2147483647 + 32

        o = gbl.operator_unsigned_long();
        o.m_ulong = sys.maxint + 128
        assert o.m_ulong == sys.maxint + 128
        assert long(o)   == sys.maxint + 128

        o = gbl.operator_float(); o.m_float = 3.14
        assert round(o.m_float - 3.14, 5) == 0.
        assert round(float(o) - 3.14, 5)  == 0.

    def test07_virtual_operator_eq(self):
        """Test use of virtual bool operator=="""

        import cppyy

        b1  = cppyy.gbl.v_opeq_base(1)
        b1a = cppyy.gbl.v_opeq_base(1)
        b2  = cppyy.gbl.v_opeq_base(2)
        b2a = cppyy.gbl.v_opeq_base(2)

        assert b1 == b1
        assert b1 == b1a
        assert not b1 == b2
        assert not b1 == b2a
        assert b2 == b2
        assert b2 == b2a

        d1  = cppyy.gbl.v_opeq_derived(1)
        d1a = cppyy.gbl.v_opeq_derived(1)
        d2  = cppyy.gbl.v_opeq_derived(2)
        d2a = cppyy.gbl.v_opeq_derived(2)

        # derived operator== returns opposite
        assert not d1 == d1
        assert not d1 == d1a
        assert d1 == d2
        assert d1 == d2a
        assert not d2 == d2
        assert not d2 == d2a

        # the following is a wee bit interesting due to python resolution
        # rules on the one hand, and C++ inheritance on the other: python
        # will never select the derived comparison b/c the call will fail
        # to pass a base through a const derived&
        assert b1 == d1
        assert d1 == b1
        assert not b1 == d2
        assert not d2 == b1
        
