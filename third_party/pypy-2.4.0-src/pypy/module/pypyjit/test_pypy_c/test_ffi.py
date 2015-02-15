import sys, py
from pypy.module.pypyjit.test_pypy_c.test_00_model import BaseTestPyPyC

class Test__ffi(BaseTestPyPyC):

    def test__ffi_call(self):
        from rpython.rlib.test.test_clibffi import get_libm_name
        def main(libm_name):
            try:
                from _rawffi.alt import CDLL, types
            except ImportError:
                sys.stderr.write('SKIP: cannot import _rawffi.alt\n')
                return 0

            libm = CDLL(libm_name)
            pow = libm.getfunc('pow', [types.double, types.double],
                               types.double)
            i = 0
            res = 0
            while i < 300:
                tmp = pow(2, 3)   # ID: fficall
                res += tmp
                i += 1
            return pow.getaddr(), res
        #
        libm_name = get_libm_name(sys.platform)
        log = self.run(main, [libm_name])
        pow_addr, res = log.result
        assert res == 8.0 * 300
        py.test.xfail()     # XXX re-optimize _ffi for the JIT?
        loop, = log.loops_by_filename(self.filepath)
        if 'ConstClass(pow)' in repr(loop):   # e.g. OS/X
            pow_addr = 'ConstClass(pow)'
        assert loop.match_by_id('fficall', """
            guard_not_invalidated(descr=...)
            i17 = force_token()
            setfield_gc(p0, i17, descr=<.* .*PyFrame.vable_token .*>)
            f21 = call_release_gil(%s, 2.000000, 3.000000, descr=<Callf 8 ff EF=6>)
            guard_not_forced(descr=...)
            guard_no_exception(descr=...)
        """ % pow_addr)


    def test__ffi_call_frame_does_not_escape(self):
        from rpython.rlib.test.test_clibffi import get_libm_name
        def main(libm_name):
            try:
                from _rawffi.alt import CDLL, types
            except ImportError:
                sys.stderr.write('SKIP: cannot import _rawffi.alt\n')
                return 0

            libm = CDLL(libm_name)
            pow = libm.getfunc('pow', [types.double, types.double],
                               types.double)

            def mypow(a, b):
                return pow(a, b)

            i = 0
            res = 0
            while i < 300:
                tmp = mypow(2, 3)
                res += tmp
                i += 1
            return pow.getaddr(), res
        #
        libm_name = get_libm_name(sys.platform)
        log = self.run(main, [libm_name])
        pow_addr, res = log.result
        assert res == 8.0 * 300
        loop, = log.loops_by_filename(self.filepath)
        opnames = log.opnames(loop.allops())
        # we only force the virtualref, not its content
        assert opnames.count('new_with_vtable') == 1

    def test__ffi_call_releases_gil(self):
        from rpython.rlib.clibffi import get_libc_name
        def main(libc_name, n):
            import time
            import os
            from threading import Thread
            #
            if os.name == 'nt':
                from _rawffi.alt import WinDLL, types
                libc = WinDLL('Kernel32.dll')
                sleep = libc.getfunc('Sleep', [types.uint], types.uint)
                delays = [0]*n + [1000]
            else:
                from _rawffi.alt import CDLL, types
                libc = CDLL(libc_name)
                sleep = libc.getfunc('sleep', [types.uint], types.uint)
                delays = [0]*n + [1]
            #
            def loop_of_sleeps(i, delays):
                for delay in delays:
                    sleep(delay)    # ID: sleep
            #
            threads = [Thread(target=loop_of_sleeps, args=[i, delays]) for i in range(5)]
            start = time.time()
            for i, thread in enumerate(threads):
                thread.start()
            for thread in threads:
                thread.join()
            end = time.time()
            return end - start
        log = self.run(main, [get_libc_name(), 200], threshold=150,
                       import_site=True)
        assert 1 <= log.result <= 1.5 # at most 0.5 seconds of overhead
        loops = log.loops_by_id('sleep')
        assert len(loops) == 1 # make sure that we actually JITted the loop

    def test_ctypes_call(self):
        from rpython.rlib.test.test_clibffi import get_libm_name
        def main(libm_name):
            import ctypes
            libm = ctypes.CDLL(libm_name)
            fabs = libm.fabs
            fabs.argtypes = [ctypes.c_double]
            fabs.restype = ctypes.c_double
            x = -4
            i = 0
            while i < 300:
                x = fabs(x)
                x = x - 100
                i += 1
            return fabs._ptr.getaddr(), x

        libm_name = get_libm_name(sys.platform)
        log = self.run(main, [libm_name], import_site=True)
        fabs_addr, res = log.result
        assert res == -4.0
        loop, = log.loops_by_filename(self.filepath)
        ops = loop.allops()
        opnames = log.opnames(ops)
        assert opnames.count('new_with_vtable') == 1 # only the virtualref
        py.test.xfail()     # XXX re-optimize _ffi for the JIT?
        assert opnames.count('call_release_gil') == 1
        idx = opnames.index('call_release_gil')
        call = ops[idx]
        assert (call.args[0] == 'ConstClass(fabs)' or    # e.g. OS/X
                int(call.args[0]) == fabs_addr)


    def test__ffi_struct(self):
        def main():
            from _rawffi.alt import _StructDescr, Field, types
            fields = [
                Field('x', types.slong),
                ]
            descr = _StructDescr('foo', fields)
            struct = descr.allocate()
            i = 0
            while i < 300:
                x = struct.getfield('x')   # ID: getfield
                x = x+1
                struct.setfield('x', x)    # ID: setfield
                i += 1
            return struct.getfield('x')
        #
        log = self.run(main, [])
        py.test.xfail()     # XXX re-optimize _ffi for the JIT?
        loop, = log.loops_by_filename(self.filepath)
        assert loop.match_by_id('getfield', """
            guard_not_invalidated(descr=...)
            i57 = getfield_raw(i46, descr=<FieldS dynamic 0>)
        """)
        assert loop.match_by_id('setfield', """
            setfield_raw(i44, i57, descr=<FieldS dynamic 0>)
        """)


    def test__cffi_call(self):
        from rpython.rlib.test.test_clibffi import get_libm_name
        def main(libm_name):
            try:
                import _cffi_backend
            except ImportError:
                sys.stderr.write('SKIP: cannot import _cffi_backend\n')
                return 0

            libm = _cffi_backend.load_library(libm_name)
            BDouble = _cffi_backend.new_primitive_type("double")
            BInt = _cffi_backend.new_primitive_type("int")
            BPow = _cffi_backend.new_function_type([BDouble, BInt], BDouble)
            ldexp = libm.load_function(BPow, 'ldexp')
            i = 0
            res = 0
            while i < 300:
                tmp = ldexp(1, 3)   # ID: cfficall
                res += tmp
                i += 1
            BLong = _cffi_backend.new_primitive_type("long")
            ldexp_addr = int(_cffi_backend.cast(BLong, ldexp))
            return ldexp_addr, res
        #
        libm_name = get_libm_name(sys.platform)
        log = self.run(main, [libm_name])
        ldexp_addr, res = log.result
        assert res == 8.0 * 300
        loop, = log.loops_by_filename(self.filepath)
        if 'ConstClass(ldexp)' in repr(loop):   # e.g. OS/X
            ldexp_addr = 'ConstClass(ldexp)'
        assert loop.match_by_id('cfficall', """
            ...
            f1 = call_release_gil(..., descr=<Callf 8 fi EF=6 OS=62>)
            ...
        """)
        ops = loop.ops_by_id('cfficall')
        for name in ['raw_malloc', 'raw_free']:
            assert name not in str(ops)
        for name in ['raw_load', 'raw_store', 'getarrayitem_raw', 'setarrayitem_raw']:
            assert name not in log.opnames(ops)
        # so far just check that call_release_gil() is produced.
        # later, also check that the arguments to call_release_gil()
        # are constants

    def test_cffi_call_guard_not_forced_fails(self):
        # this is the test_pypy_c equivalent of
        # rpython/jit/metainterp/test/test_fficall::test_guard_not_forced_fails
        #
        # it requires cffi to be installed for pypy in order to run
        def main():
            import sys
            try:
                import cffi
            except ImportError:
                sys.stderr.write('SKIP: cannot import cffi\n')
                return 0

            ffi = cffi.FFI()

            ffi.cdef("""
            typedef void (*functype)(int);
            int foo(int n, functype func);
            """)

            lib = ffi.verify("""
            #include <signal.h>
            typedef void (*functype)(int);

            int foo(int n, functype func) {
                if (n >= 2000) {
                    func(n);
                }
                return n*2;
            }
            """)

            @ffi.callback("functype")
            def mycallback(n):
                if n < 5000:
                    return
                # make sure that guard_not_forced fails
                d = {}
                f = sys._getframe()
                while f:
                    d.update(f.f_locals)
                    f = f.f_back

            n = 0
            while n < 10000:
                res = lib.foo(n, mycallback)  # ID: cfficall
                # this is the real point of the test: before the
                # refactor-call_release_gil branch, the assert failed when
                # res == 5000
                assert res == n*2
                n += 1
            return n

        log = self.run(main, [], import_site=True,
                       discard_stdout_before_last_line=True)  # <- for Win32
        assert log.result == 10000
        loop, = log.loops_by_id('cfficall')
        assert loop.match_by_id('cfficall', """
            ...
            f1 = call_release_gil(..., descr=<Calli 4 ii EF=6 OS=62>)
            ...
        """)

    def test__cffi_bug1(self):
        from rpython.rlib.test.test_clibffi import get_libm_name
        def main(libm_name):
            try:
                import _cffi_backend
            except ImportError:
                sys.stderr.write('SKIP: cannot import _cffi_backend\n')
                return 0

            libm = _cffi_backend.load_library(libm_name)
            BDouble = _cffi_backend.new_primitive_type("double")
            BSin = _cffi_backend.new_function_type([BDouble], BDouble)
            sin = libm.load_function(BSin, 'sin')

            def f(*args):
                for i in range(300):
                    sin(*args)

            f(1.0)
            f(1)
        #
        libm_name = get_libm_name(sys.platform)
        self.run(main, [libm_name])
        # assert did not crash

    def test_cffi_init_struct_with_list(self):
        def main(n):
            import sys
            try:
                import cffi
            except ImportError:
                sys.stderr.write('SKIP: cannot import cffi\n')
                return 0

            ffi = cffi.FFI()
            ffi.cdef("""
            struct s {
                short x;
                short y;
                short z;
            };
            """)

            for i in xrange(n):
                ffi.new("struct s *", [i, i, i])

        log = self.run(main, [300])
        loop, = log.loops_by_filename(self.filepath)
        assert loop.match("""
        i161 = int_lt(i160, i43)
        guard_true(i161, descr=...)
        i162 = int_add(i160, 1)
        setfield_gc(p22, i162, descr=<FieldS pypy.module.__builtin__.functional.W_XRangeIterator.inst_current .>)
        guard_not_invalidated(descr=...)
        p163 = force_token()
        p164 = force_token()
        p165 = getarrayitem_gc(p67, 0, descr=<ArrayP .>)
        guard_value(p165, ConstPtr(ptr70), descr=...)
        p166 = getfield_gc(p165, descr=<FieldP pypy.objspace.std.dictmultiobject.W_DictMultiObject.inst_strategy .+>)
        guard_value(p166, ConstPtr(ptr72), descr=...)
        p167 = call(ConstClass(_ll_0_alloc_with_del___), descr=<Callr . EF=4>)
        guard_no_exception(descr=...)
        i112 = int_sub(i160, -32768)
        setfield_gc(p167, ConstPtr(null), descr=<FieldP pypy.module._cffi_backend.cdataobj.W_CData.inst__lifeline_ .+>)
        setfield_gc(p167, ConstPtr(ptr85), descr=<FieldP pypy.module._cffi_backend.cdataobj.W_CData.inst_ctype .+>)
        i114 = uint_gt(i112, 65535)
        guard_false(i114, descr=...)
        i115 = int_and(i112, 65535)
        i116 = int_add(i115, -32768)
        --TICK--
        i119 = call(ConstClass(_ll_1_raw_malloc_varsize__Signed), 6, descr=<Calli . i EF=4 OS=110>)
        raw_store(i119, 0, i116, descr=<ArrayS 2>)
        raw_store(i119, 2, i116, descr=<ArrayS 2>)
        raw_store(i119, 4, i116, descr=<ArrayS 2>)
        setfield_gc(p167, i119, descr=<FieldU pypy.module._cffi_backend.cdataobj.W_CData.inst__cdata .+>)
        i123 = arraylen_gc(p67, descr=<ArrayP .>)
        jump(..., descr=...)
        """)
