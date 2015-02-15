===============
PyPy on Windows
===============

PyPy is supported on Windows platforms, starting with Windows 2000.
The following text gives some hints about how to translate the PyPy
interpreter.

PyPy supports only being translated as a 32bit program, even on
64bit Windows.  See at the end of this page for what is missing
for a full 64bit translation.

To build pypy-c you need a working python environment, and a C compiler.
It is possible to translate with a CPython 2.6 or later, but this is not
the preferred way, because it will take a lot longer to run – depending
on your architecture, between two and three times as long. So head to 
`our downloads`_ and get the latest stable version.

Microsoft Visual Studio is preferred as a compiler, but there are reports 
of success with the mingw32 port of gcc.


Translating PyPy with Visual Studio
-----------------------------------

We routinely test the `RPython translation toolchain`_ using 
Visual Studio 2008, Express
Edition.  Other configurations may work as well.

The translation scripts will set up the appropriate environment variables
for the compiler, so you do not need to run vcvars before translation.  
They will attempt to locate the same compiler version that
was used to build the Python interpreter doing the
translation.  Failing that, they will pick the most recent Visual Studio
compiler they can find.  In addition, the target architecture
(32 bits, 64 bits) is automatically selected.  A 32 bit build can only be built
using a 32 bit Python and vice versa. By default pypy is built using the 
Multi-threaded DLL (/MD) runtime environment.

**Note:** PyPy is currently not supported for 64 bit Python, and translation
will fail in this case.

Python and a C compiler are all you need to build pypy, but it will miss some
modules that relies on third-party libraries.  See below how to get
and build them.

Please see the `non-windows instructions`_ for more information, especially note
that translation is RAM-hungry. A standard translation requires around 4GB, so
special preparations are necessary, or you may want to use the method in the
notes of the `build instructions`_ to reduce memory usage at the price of a
slower translation::

    set PYPY_GC_MAX_DELTA=200MB
    pypy --jit loop_longevity=300 ../../rpython/bin/rpython -Ojit targetpypystandalone
    set PYPY_GC_MAX_DELTA=

Preping Windows for the Large Build
-----------------------------------

Normally 32bit programs are limited to 2GB of memory on Windows. It is
possible to raise this limit, to 3GB on Windows 32bit, and almost 4GB
on Windows 64bit.

On Windows 32bit, it is necessary to modify the system: follow
http://usa.autodesk.com/adsk/servlet/ps/dl/item?siteID=123112&id=9583842&linkID=9240617
to enable the "3GB" feature, and reboot. This step is not necessary on
Windows 64bit.

Then you need to execute::

    editbin /largeaddressaware translator.exe

where ``translator.exe`` is the pypy.exe or cpython.exe you will use to 
translate with. 

Installing external packages
----------------------------

On Windows, there is no standard place where to download, build and
install third-party libraries.  We recommend installing them in the parent
directory of the pypy checkout.  For example, if you installed pypy in
``d:\pypy\trunk\`` (This directory contains a README file), the base
directory is ``d:\pypy``. You must then set the
INCLUDE, LIB and PATH (for DLLs) environment variables appropriately.

Abridged method (for -Ojit builds using Visual Studio 2008)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Download the versions of all the external packages from 
https://bitbucket.org/pypy/pypy/downloads/local_2.4.zip
(for 2.4 release and later) or
https://bitbucket.org/pypy/pypy/downloads/local.zip
(for pre-2.4 versions)
Then expand it into the base directory (base_dir) and modify your environment
to reflect this::

    set PATH=<base_dir>\bin;<base_dir>\tcltk\bin;%PATH%
    set INCLUDE=<base_dir>\include;<base_dir>\tcltk\include;%INCLUDE%
    set LIB=<base_dir>\lib;<base_dir>\tcltk\lib;%LIB%

Now you should be good to go. Read on for more information. 

The Boehm garbage collector
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This library is needed if you plan to use the ``--gc=boehm`` translation
option (this is the default at some optimization levels like ``-O1``,
but unneeded for high-performance translations like ``-O2``).
You may get it at
http://hboehm.info/gc/gc_source/gc-7.1.tar.gz

Versions 7.0 and 7.1 are known to work; the 6.x series won't work with
pypy. Unpack this folder in the base directory. 
The default GC_abort(...) function in misc.c will try to open a MessageBox.
You may want to disable this with the following patch::

    --- a/misc.c    Sun Apr 20 14:08:27 2014 +0300
    +++ b/misc.c    Sun Apr 20 14:08:37 2014 +0300
    @@ -1058,7 +1058,7 @@
     #ifndef PCR
      void GC_abort(const char *msg)
       {
       -#   if defined(MSWIN32)
       +#   if 0 && defined(MSWIN32)
              (void) MessageBoxA(NULL, msg, "Fatal error in gc", MB_ICONERROR|MB_OK);
               #   else
                      GC_err_printf("%s\n", msg);
    
Then open a command prompt::

    cd gc-7.1
    nmake -f NT_THREADS_MAKEFILE
    copy Release\gc.dll <somewhere in the PATH>

The zlib compression library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Download http://www.gzip.org/zlib/zlib-1.2.3.tar.gz and extract it in
the base directory.  Then compile as a static library::

    cd zlib-1.2.3
    nmake -f win32\Makefile.msc
    copy zlib.lib <somewhere in LIB>
    copy zlib.h zconf.h <somewhere in INCLUDE>

The bz2 compression library
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Get the same version of bz2 used by python and compile as a static library::

    svn export http://svn.python.org/projects/external/bzip2-1.0.6
    cd bzip2-1.0.6
    nmake -f makefile.msc
    copy libbz2.lib <somewhere in LIB>
    copy bzlib.h <somewhere in INCLUDE>

    
The sqlite3 database library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PyPy uses cffi to interact with sqlite3.dll. Only the dll is needed, the cffi
wrapper is compiled when the module is imported for the first time.
The sqlite3.dll should be version 3.6.21 for CPython2.7 compatablility.

The expat XML parser
~~~~~~~~~~~~~~~~~~~~

Download the source code of expat on sourceforge:
http://sourceforge.net/projects/expat/ and extract it in the base
directory.  Version 2.1.0 is known to pass tests. Then open the project 
file ``expat.dsw`` with Visual
Studio; follow the instruction for converting the project files,
switch to the "Release" configuration, use the ``expat_static`` project,
reconfigure the runtime for Multi-threaded DLL (/MD) and build.

Then, copy the file ``win32\bin\release\libexpat.lib`` somewhere in
somewhere in LIB, and both ``lib\expat.h`` and ``lib\expat_external.h``
somewhere in INCLUDE.

The OpenSSL library
~~~~~~~~~~~~~~~~~~~

OpenSSL needs a Perl interpreter to configure its makefile.  You may
use the one distributed by ActiveState, or the one from cygwin.::

    svn export http://svn.python.org/projects/external/openssl-1.0.1i
    cd openssl-1.0.1i
    perl Configure VC-WIN32 no-idea no-mdc2
    ms\do_ms.bat
    nmake -f ms\nt.mak install

Then, copy the files ``out32\*.lib`` somewhere in
somewhere in LIB, and the entire ``include\openssl`` directory as-is somewhere
in INCLUDE.
    
TkInter module support
~~~~~~~~~~~~~~~~~~~~~~

Note that much of this is taken from the cpython build process.
Tkinter is imported via cffi, so the module is optional. To recreate the tcltk
directory found for the release script, create the dlls, libs, headers and
runtime by running::

	svn export http://svn.python.org/projects/external/tcl-8.5.2.1 tcl85 
	svn export http://svn.python.org/projects/external/tk-8.5.2.0 tk85
	cd tcl85\win 
	nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=0 INSTALLDIR=..\..\tcltk clean all 
	nmake -f makefile.vc DEBUG=0 INSTALLDIR=..\..\tcltk install
	cd ..\..\tk85\win 
	nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl85 clean all 
	nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl85 install

Now you should have a tcktk\bin, tcltk\lib, and tcltk\include directory ready
for use. The release packaging script will pick up the tcltk runtime in the lib
directory and put it in the archive.

Using the mingw compiler
------------------------

You can compile pypy with the mingw compiler, using the --cc=mingw32 option;
gcc.exe must be on the PATH. If the -cc flag does not begin with "ming", it should be
the name of a valid gcc-derivative compiler, i.e. x86_64-w64-mingw32-gcc for the 64 bit
compiler creating a 64 bit target.

You probably want to set the CPATH, LIBRARY_PATH, and PATH environment variable to
the header files, lib or dlls, and dlls respectively of the locally installed packages 
if they are not in the mingw directory heirarchy. 

libffi for the mingw compiler
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable the _rawffi (and ctypes) module, you need to compile a mingw
version of libffi.  Here is one way to do this, wich should allow you to try
to build for win64 or win32:

#. Download and unzip a `mingw32 build`_ or `mingw64 build`_, say into c:\mingw
#. If you do not use cygwin, you will need msys to provide make, 
   autoconf tools and other goodies.

    #. Download and unzip a `msys for mingw`_, say into c:\msys
    #. Edit the c:\msys\etc\fstab file to mount c:\mingw

#. Download and unzip the `libffi source files`_, and extract
   them in the base directory.  
#. Run c:\msys\msys.bat or a cygwin shell which should make you
   feel better since it is a shell prompt with shell tools.
#. From inside the shell, cd to the libffi directory and do::

    sh ./configure
    make
    cp .libs/libffi-5.dll <somewhere on the PATH>

If you can't find the dll, and the libtool issued a warning about 
"undefined symbols not allowed", you will need to edit the libffi
Makefile in the toplevel directory. Add the flag -no-undefined to
the definition of libffi_la_LDFLAGS

If you wish to experiment with win64, you must run configure with flags::

    sh ./configure --build=x86_64-w64-mingw32 --host=x86_64-w64-mingw32

or such, depending on your mingw64 download.

hacking on PyPy with the mingw compiler
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Since hacking on PyPy means running tests, you will need a way to specify
the mingw compiler when hacking (as opposed to translating). As of
March 2012, --cc is not a valid option for pytest.py. However if you set an
environment variable CC to the compliter exe, testing will use it.

.. _`mingw32 build`: http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Automated%20Builds
.. _`mingw64 build`: http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Automated%20Builds
.. _`msys for mingw`: http://sourceforge.net/projects/mingw-w64/files/External%20binary%20packages%20%28Win64%20hosted%29/MSYS%20%2832-bit%29   
.. _`libffi source files`: http://sourceware.org/libffi/
.. _`RPython translation toolchain`: translation.html
.. _`our downloads`: http://pypy.org/download.html   
.. _`non-windows instructions`: getting-started-python.html#translating-the-pypy-python-interpreter
.. _`build instructions`: http://pypy.org/download.html#building-from-source

What is missing for a full 64-bit translation
---------------------------------------------

The main blocker is that we assume that the integer type of RPython is
large enough to (occasionally) contain a pointer value cast to an
integer.  The simplest fix is to make sure that it is so, but it will
give the following incompatibility between CPython and PyPy on Win64:
  
CPython: ``sys.maxint == 2**32-1, sys.maxsize == 2**64-1``

PyPy: ``sys.maxint == sys.maxsize == 2**64-1``

...and, correspondingly, PyPy supports ints up to the larger value of
sys.maxint before they are converted to ``long``.  The first decision
that someone needs to make is if this incompatibility is reasonable.

Assuming that it is, the first thing to do is probably to hack *CPython*
until it fits this model: replace the field in PyIntObject with a ``long
long`` field, and change the value of ``sys.maxint``.  This might just
work, even if half-brokenly: I'm sure you can crash it because of the
precision loss that undoubtedly occurs everywhere, but try not to. :-)

Such a hacked CPython is what you'll use in the next steps.  We'll call
it CPython64/64.

It is probably not too much work if the goal is only to get a translated
PyPy executable, and to run all tests before translation.  But you need
to start somewhere, and you should start with some tests in
rpython/translator/c/test/, like ``test_standalone.py`` and
``test_newgc.py``: try to have them pass on top of CPython64/64.

Keep in mind that this runs small translations, and some details may go
wrong.  The most obvious one is to check that it produces C files that
use the integer type ``Signed`` --- but what is ``Signed`` defined to?
It should be equal to ``long`` on every other platform, but on Win64 it
should be something like ``long long``.

What is more generally needed is to review all the C files in
rpython/translator/c/src for the word ``long``, because this means a
32-bit integer even on Win64.  Replace it with ``Signed`` most of the
times.  You can replace one with the other without breaking anything on
any other platform, so feel free to.

Then, these two C types have corresponding RPython types: ``rffi.LONG``
and ``lltype.Signed`` respectively.  The first should really correspond
to the C ``long``.  Add tests that check that integers cast to one
type or the other really have 32 and 64 bits respectively, on Win64.

Once these basic tests work, you need to review ``rpython/rlib/`` for
uses of ``rffi.LONG`` versus ``lltype.Signed``.  The goal would be to
fix some more ``LONG-versus-Signed`` issues, by fixing the tests --- as
always run on top of CPython64/64.  Note that there was some early work
done in ``rpython/rlib/rarithmetic`` with the goal of running all the
tests on Win64 on the regular CPython, but I think by now that it's a
bad idea.  Look only at CPython64/64.

The major intermediate goal is to get a translation of PyPy with ``-O2``
with a minimal set of modules, starting with ``--no-allworkingmodules``;
you need to use CPython64/64 to run this translation too.  Check
carefully the warnings of the C compiler at the end. By default, MSVC
reports a lot of mismatches of integer sizes as warnings instead of
errors.

Then you need to review ``pypy/module/*/`` for ``LONG-versus-Signed``
issues.  At some time during this review, we get a working translated
PyPy on Windows 64 that includes all ``--translationmodules``, i.e.
everything needed to run translations.  Once we have that, the hacked
CPython64/64 becomes much less important, because we can run future
translations on top of this translated PyPy.  As soon as we get there,
please *distribute* the translated PyPy.  It's an essential component
for anyone else that wants to work on Win64!  We end up with a strange
kind of dependency --- we need a translated PyPy in order to translate a
PyPy ---, but I believe it's ok here, as Windows executables are
supposed to never be broken by newer versions of Windows.

Happy hacking :-)
