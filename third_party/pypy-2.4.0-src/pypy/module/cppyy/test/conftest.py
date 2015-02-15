import py

@py.test.mark.tryfirst
def pytest_runtest_setup(item):
    if py.path.local.sysfind('genreflex') is None:
        import pypy.module.cppyy.capi.loadable_capi as lcapi
        if 'dummy' in lcapi.reflection_library:
            # run only tests that are covered by the dummy backend and tests
            # that do not rely on reflex
            import os
            tst = os.path.basename(item.location[0])
            if not tst in ('test_helper.py', 'test_cppyy.py', 'test_pythonify.py',
                           'test_datatypes.py'):
                py.test.skip("genreflex is not installed")
            import re
            if tst == 'test_pythonify.py' and \
                not re.search("AppTestPYTHONIFY.test0[1-6]", item.location[2]):
                py.test.skip("genreflex is not installed")
            elif tst == 'test_datatypes.py' and \
                not re.search("AppTestDATATYPES.test0[1-8]", item.location[2]):
                py.test.skip("genreflex is not installed")

def pytest_ignore_collect(path, config):
    if py.path.local.sysfind('genreflex') is None and config.option.runappdirect:
        return True          # "can't run dummy tests in -A"

def pytest_configure(config):
    if py.path.local.sysfind('genreflex') is None:
        import pypy.module.cppyy.capi.loadable_capi as lcapi
        try:
            import ctypes
            ctypes.CDLL(lcapi.reflection_library)
        except Exception, e:
            if config.option.runappdirect:
                return       # "can't run dummy tests in -A"

            # build dummy backend (which has reflex info and calls hard-wired)
            import os
            from rpython.translator.tool.cbuild import ExternalCompilationInfo
            from rpython.translator.platform import platform

            from rpython.rtyper.lltypesystem import rffi

            pkgpath = py.path.local(__file__).dirpath().join(os.pardir)
            srcpath = pkgpath.join('src')
            incpath = pkgpath.join('include')
            tstpath = pkgpath.join('test')

            eci = ExternalCompilationInfo(
                separate_module_files=[srcpath.join('dummy_backend.cxx')],
                include_dirs=[incpath, tstpath],
                use_cpp_linker=True,
            )

            soname = platform.compile(
                [], eci,
                outputfilename='libcppyy_dummy_backend',
                standalone=False)

            lcapi.reflection_library = str(soname)
