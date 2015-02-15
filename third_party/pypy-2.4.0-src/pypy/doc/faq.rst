==========================
Frequently Asked Questions
==========================

.. contents::


General
=======

-------------
What is PyPy?
-------------

PyPy is both:

 - a reimplementation of Python in Python, and

 - a framework for implementing interpreters and virtual machines for
   programming languages, especially dynamic languages.

PyPy tries to find new answers about ease of creation, flexibility,
maintainability and speed trade-offs for language implementations.
For further details see our `goal and architecture document`_ .

.. _`goal and architecture document`: architecture.html


.. _`drop in replacement`:

------------------------------------------
Is PyPy a drop in replacement for CPython?
------------------------------------------

Almost!

The mostly likely stumbling block for any given project is support for
`extension modules`_.  PyPy supports a continually growing
number of extension modules, but so far mostly only those found in the
standard library.

The language features (including builtin types and functions) are very
complete and well tested, so if your project does not use many
extension modules there is a good chance that it will work with PyPy.

We list the differences we know about in `cpython differences`_.

-----------------------------------------------
Module xyz does not work with PyPy: ImportError
-----------------------------------------------

A module installed for CPython is not automatically available for PyPy
--- just like a module installed for CPython 2.6 is not automatically
available for CPython 2.7 if you installed both.  In other words, you
need to install the module xyz specifically for PyPy.

On Linux, this means that you cannot use ``apt-get`` or some similar
package manager: these tools are only meant *for the version of CPython
provided by the same package manager.*  So forget about them for now
and read on.

It is quite common nowadays that xyz is available on PyPI_ and
installable with ``pip install xyz``.  The simplest solution is to `use
virtualenv (as documented here)`_.  Then enter (activate) the virtualenv
and type: ``pip install xyz``.

If you get errors from the C compiler, the module is a CPython C
Extension module using unsupported features.  `See below.`_

Alternatively, if either the module xyz is not available on PyPI or you
don't want to use virtualenv, then download the source code of xyz,
decompress the zip/tarball, and run the standard command: ``pypy
setup.py install``.  (Note: `pypy` here instead of `python`.)  As usual
you may need to run the command with `sudo` for a global installation.
The other commands of ``setup.py`` are available too, like ``build``.

.. _PyPI: https://pypi.python.org/pypi
.. _`use virtualenv (as documented here)`: getting-started.html#installing-using-virtualenv


.. _`See below.`: 

--------------------------------------------
Do CPython Extension modules work with PyPy?
--------------------------------------------

We have experimental support for CPython extension modules, so
they run with minor changes.  This has been a part of PyPy since
the 1.4 release, but support is still in beta phase.  CPython
extension modules in PyPy are often much slower than in CPython due to
the need to emulate refcounting.  It is often faster to take out your
CPython extension and replace it with a pure python version that the
JIT can see.  If trying to install module xyz, and the module has both
a C and a Python version of the same code, try first to disable the C
version; this is usually easily done by changing some line in ``setup.py``.

We fully support ctypes-based extensions. But for best performance, we
recommend that you use the cffi_ module to interface with C code.

For information on which third party extensions work (or do not work) 
with PyPy see the `compatibility wiki`_.


.. _`extension modules`: cpython_differences.html#extension-modules
.. _`cpython differences`: cpython_differences.html
.. _`compatibility wiki`: https://bitbucket.org/pypy/compatibility/wiki/Home
.. _cffi: http://cffi.readthedocs.org/

---------------------------------
On which platforms does PyPy run?
---------------------------------

PyPy is regularly and extensively tested on Linux machines. It mostly
works on Mac and Windows: it is tested there, but most of us are running
Linux so fixes may depend on 3rd-party contributions.  PyPy's JIT
works on x86 (32-bit or 64-bit) and on ARM (ARMv6 or ARMv7).
Support for POWER (64-bit) is stalled at the moment.

To bootstrap from sources, PyPy can use either CPython (2.6 or 2.7) or
another (e.g. older) PyPy.  Cross-translation is not really supported:
e.g. to build a 32-bit PyPy, you need to have a 32-bit environment.
Cross-translation is only explicitly supported between a 32-bit Intel
Linux and ARM Linux (see here__).

.. __: arm.html

------------------------------------------------
Which Python version (2.x?) does PyPy implement?
------------------------------------------------

PyPy currently aims to be fully compatible with Python 2.7. That means that
it contains the standard library of Python 2.7 and that it supports 2.7
features (such as set comprehensions).  

.. _threading:

-------------------------------------------------
Does PyPy have a GIL?  Why?
-------------------------------------------------

Yes, PyPy has a GIL.  Removing the GIL is very hard.  The problems are
essentially the same as with CPython (including the fact that our
garbage collectors are not thread-safe so far).  Fixing it is possible,
as shown by Jython and IronPython, but difficult.  It would require
adapting the whole source code of PyPy, including subtle decisions about
whether some effects are ok or not for the user (i.e. the Python
programmer).

Instead, since 2012, there is work going on on a still very experimental
`Software Transactional Memory`_ (STM) version of PyPy.  This should give
an alternative PyPy which works without a GIL, while at the same time
continuing to give the Python programmer the complete illusion of having
one.

.. _`Software Transactional Memory`: stm.html

--------------------------------------------------
Is PyPy more clever than CPython about Tail Calls?
--------------------------------------------------

No.  PyPy follows the Python language design, including the built-in
debugger features.  This prevents tail calls, as summarized by Guido
van Rossum in two__ blog__ posts.  Moreover, neither the JIT nor
Stackless__ change anything to that.

.. __: http://neopythonic.blogspot.com/2009/04/tail-recursion-elimination.html
.. __: http://neopythonic.blogspot.com/2009/04/final-words-on-tail-calls.html
.. __: stackless.html

------------------------------------------
How do I write extension modules for PyPy?
------------------------------------------

See `Writing extension modules for PyPy`__.

.. __: extending.html

-----------------
How fast is PyPy?
-----------------
This really depends on your code.
For pure Python algorithmic code, it is very fast.  For more typical
Python programs we generally are 3 times the speed of CPython 2.7.
You might be interested in our `benchmarking site`_ and our 
`jit documentation`_.

`Your tests are not a benchmark`_: tests tend to be slow under PyPy
because they run exactly once; if they are good tests, they exercise
various corner cases in your code.  This is a bad case for JIT
compilers.  Note also that our JIT has a very high warm-up cost, meaning
that any program is slow at the beginning.  If you want to compare the
timings with CPython, even relatively simple programs need to run *at
least* one second, preferrably at least a few seconds.  Large,
complicated programs need even more time to warm-up the JIT.

.. _`benchmarking site`: http://speed.pypy.org

.. _`jit documentation`: jit/index.html

.. _`your tests are not a benchmark`: http://alexgaynor.net/2013/jul/15/your-tests-are-not-benchmark/

---------------------------------------------------------------
Couldn't the JIT dump and reload already-compiled machine code?
---------------------------------------------------------------

No, we found no way of doing that.  The JIT generates machine code
containing a large number of constant addresses --- constant at the time
the machine code is generated.  The vast majority is probably not at all
constants that you find in the executable, with a nice link name.  E.g.
the addresses of Python classes are used all the time, but Python
classes don't come statically from the executable; they are created anew
every time you restart your program.  This makes saving and reloading
machine code completely impossible without some very advanced way of
mapping addresses in the old (now-dead) process to addresses in the new
process, including checking that all the previous assumptions about the
(now-dead) object are still true about the new object.


.. _`prolog and javascript`:

--------------------------------------------------------------------------
Can I use PyPy's translation toolchain for other languages besides Python?
--------------------------------------------------------------------------

Yes. The toolsuite that translates the PyPy interpreter is quite
general and can be used to create optimized versions of interpreters
for any language, not just Python.  Of course, these interpreters
can make use of the same features that PyPy brings to Python:
translation to various languages, stackless features,
garbage collection, implementation of various things like arbitrarily long
integers, etc. 

Currently, we have `Topaz`_, a Ruby interpreter; `Hippy`_, a PHP
interpreter; preliminary versions of a `JavaScript interpreter`_
(Leonardo Santagada as his Summer of PyPy project); a `Prolog interpreter`_
(Carl Friedrich Bolz as his Bachelor thesis); and a `SmallTalk interpreter`_
(produced during a sprint).  On the `PyPy bitbucket page`_ there is also a
Scheme and an Io implementation; both of these are unfinished at the moment.

.. _`Topaz`: http://topazruby.com/
.. _`Hippy`: http://morepypy.blogspot.ch/2012/07/hello-everyone.html
.. _`JavaScript interpreter`: https://bitbucket.org/pypy/lang-js/
.. _`Prolog interpreter`: https://bitbucket.org/cfbolz/pyrolog/
.. _`SmallTalk interpreter`: http://dx.doi.org/10.1007/978-3-540-89275-5_7
.. _`PyPy bitbucket page`: https://bitbucket.org/pypy/


Development
===========

-----------------------------------------------------------
How do I get into PyPy development?  Can I come to sprints?
-----------------------------------------------------------

Certainly you can come to sprints! We always welcome newcomers and try
to help them as much as possible to get started with the project.  We
provide tutorials and pair them with experienced PyPy
developers. Newcomers should have some Python experience and read some
of the PyPy documentation before coming to a sprint.

Coming to a sprint is usually the best way to get into PyPy development.
If you get stuck or need advice, `contact us`_. IRC is
the most immediate way to get feedback (at least during some parts of the day;
most PyPy developers are in Europe) and the `mailing list`_ is better for long
discussions.

.. _`contact us`: index.html
.. _`mailing list`: http://mail.python.org/mailman/listinfo/pypy-dev

-------------------------------------------------------------
OSError: ... cannot restore segment prot after reloc... Help?
-------------------------------------------------------------

On Linux, if SELinux is enabled, you may get errors along the lines of
"OSError: externmod.so: cannot restore segment prot after reloc: Permission
denied." This is caused by a slight abuse of the C compiler during
configuration, and can be disabled by running the following command with root
privileges:

.. code-block:: console

    # setenforce 0

This will disable SELinux's protection and allow PyPy to configure correctly.
Be sure to enable it again if you need it!


The RPython translation tool chain
===================================

------------------------------------------------
Can RPython compile normal Python programs to C?
------------------------------------------------

No, RPython is not a Python compiler.

In Python, it is mostly impossible to *prove* anything about the types
that a program will manipulate by doing a static analysis.  It should be
clear if you are familiar with Python, but if in doubt see [BRETT]_.

If you want a fast Python program, please use the PyPy JIT_ instead.

.. _JIT: jit/index.html

.. [BRETT] Brett Cannon,
           Localized Type Inference of Atomic Types in Python,
           http://citeseer.ist.psu.edu/viewdoc/summary?doi=10.1.1.90.3231

.. _`PyPy's RPython`: 

------------------------------
What is this RPython language?
------------------------------

RPython is a restricted subset of the Python language.   It is used for 
implementing dynamic language interpreters within the PyPy toolchain.  The
restrictions ensure that type inference (and so, ultimately, translation
to other languages) of RPython programs is possible. 

The property of "being RPython" always applies to a full program, not to single
functions or modules (the translation toolchain does a full program analysis).
The translation toolchain follows all calls
recursively and discovers what belongs to the program and what does not.

RPython program restrictions mostly limit the ability
to mix types in arbitrary ways. RPython does not allow the binding of two
different types in the same variable. In this respect (and in some others) it
feels a bit like Java. Other features not allowed in RPython are the use of
special methods (``__xxx__``) except ``__init__`` and ``__del__``, and the
use of reflection capabilities (e.g. ``__dict__``).

You cannot use most existing standard library modules from RPython.  The
exceptions are
some functions in ``os``, ``math`` and ``time`` that have native support.
We have our own "RPython standard library" in ``rpython.rlib.*``.

To read more about the RPython limitations read the `RPython description`_.

.. _`RPython description`: coding-guide.html#rpython-definition

---------------------------------------------------------------
Does RPython have anything to do with Zope's Restricted Python?
---------------------------------------------------------------

No.  `Zope's RestrictedPython`_ aims to provide a sandboxed 
execution environment for CPython.   `PyPy's RPython`_ is the implementation
language for dynamic language interpreters.  However, PyPy also provides 
a robust `sandboxed Python Interpreter`_. 

.. _`sandboxed Python Interpreter`: sandbox.html
.. _`Zope's RestrictedPython`: http://pypi.python.org/pypi/RestrictedPython

------------------------------------------------------
What's the ``"NOT_RPYTHON"`` I see in some docstrings?
------------------------------------------------------

If you put "NOT_RPYTHON" into the docstring of a function and that function is
found while trying to translate an RPython program, the translation process
stops and reports this as an error. You can therefore mark functions as
"NOT_RPYTHON" to make sure that they are never analyzed.


-------------------------------------------------------------------
Couldn't we simply take a Python syntax tree and turn it into Lisp?
-------------------------------------------------------------------

It's not necessarily nonsense, but it's not really The PyPy Way.  It's
pretty hard, without some kind of type inference, to translate this
Python::

    a + b

into anything significantly more efficient than this Common Lisp::

    (py:add a b)

And making type inference possible is what RPython is all about.

You could make ``#'py:add`` a generic function and see if a given CLOS
implementation is fast enough to give a useful speed (but I think the
coercion rules would probably drive you insane first).  -- mwh

--------------------------------------------
Do I have to rewrite my programs in RPython?
--------------------------------------------

No, and you shouldn't try.  First and foremost, RPython is a language
designed for writing interpreters. It is a restricted subset of
Python.  If your program is not an interpreter but tries to do "real
things", like use *any* part of the standard Python library or *any*
3rd-party library, then it is not RPython to start with.  You should
only look at RPython if you try to `write your own interpreter`__.

.. __: `how do I compile my own interpreters`_

If your goal is to speed up Python code, then look at the regular PyPy,
which is a full and compliant Python 2.7 interpreter (which happens to
be written in RPython).  Not only is it not necessary for you to rewrite
your code in RPython, it might not give you any speed improvements even
if you manage to.

Yes, it is possible with enough effort to compile small self-contained
pieces of RPython code doing a few performance-sensitive things.  But
this case is not interesting for us.  If you needed to rewrite the code
in RPython, you could as well have rewritten it in C or C++ or Java for
example.  These are much more supported, much more documented languages
`:-)`

  *The above paragraphs are not the whole truth.  It* is *true that there
  are cases where writing a program as RPython gives you substantially
  better speed than running it on top of PyPy.  However, the attitude of
  the core group of people behind PyPy is to answer: "then report it as a
  performance bug against PyPy!".*

  *Here is a more diluted way to put it.  The "No, don't!" above is a
  general warning we give to new people.  They are likely to need a lot
  of help from* some *source, because RPython is not so simple nor
  extensively documented; but at the same time, we, the pypy core group
  of people, are not willing to invest time in supporting 3rd-party
  projects that do very different things than interpreters for dynamic
  languages --- just because we have other interests and there are only
  so many hours a day.  So as a summary I believe it is only fair to
  attempt to point newcomers at existing alternatives, which are more
  mainstream and where they will get help from many people.*

  *If anybody seriously wants to promote RPython anyway, they are welcome
  to: we won't actively resist such a plan.  There are a lot of things
  that could be done to make RPython a better Java-ish language for
  example, starting with supporting non-GIL-based multithreading, but we
  don't implement them because they have little relevance to us.  This
  is open source, which means that anybody is free to promote and
  develop anything; but it also means that you must let us choose* not
  *to go into that direction ourselves.*

---------------------------------------------------
Which backends are there for the RPython toolchain?
---------------------------------------------------

Currently, there only backends is C_.
It can translate the entire PyPy interpreter.
To learn more about backends take a look at the `translation document`_.

.. _C: translation.html#the-c-back-end
.. _`translation document`: translation.html

------------------
Could we use LLVM?
------------------

In theory yes.  But we tried to use it 5 or 6 times already, as a
translation backend or as a JIT backend --- and failed each time.

In more details: using LLVM as a (static) translation backend is
pointless nowadays because you can generate C code and compile it with
clang.  (Note that compiling PyPy with clang gives a result that is not
faster than compiling it with gcc.)  We might in theory get extra
benefits from LLVM's GC integration, but this requires more work on the
LLVM side before it would be remotely useful.  Anyway, it could be
interfaced via a custom primitive in the C code.  (The latest such
experimental backend is in the branch ``llvm-translation-backend``,
which can translate PyPy with or without the JIT on Linux.)

On the other hand, using LLVM as our JIT backend looks interesting as
well --- but again we made an attempt, and it failed: LLVM has no way to
patch the generated machine code.

So the position of the core PyPy developers is that if anyone wants to
make an N+1'th attempt with LLVM, they are welcome, and will be happy to
provide help in the IRC channel, but they are left with the burden of proof
that (a) it works and (b) it gives important benefits.

----------------------
How do I compile PyPy?
----------------------

See the `getting-started`_ guide.

.. _`getting-started`: getting-started-python.html

------------------------------------------
Compiling PyPy swaps or runs out of memory
------------------------------------------

This is documented (here__ and here__).  It needs 4 GB of RAM to run
"rpython targetpypystandalone" on top of PyPy, a bit more when running
on top of CPython.  If you have less than 4 GB free, it will just swap
forever (or fail if you don't have enough swap).  And we mean *free:*
if the machine has 4 GB *in total,* then it will swap.

On 32-bit, divide the numbers by two.  (We didn't try recently, but in
the past it was possible to compile a 32-bit version on a 2 GB Linux
machine with nothing else running: no Gnome/KDE, for example.)

.. __: http://pypy.org/download.html#building-from-source
.. __: https://pypy.readthedocs.org/en/latest/getting-started-python.html#translating-the-pypy-python-interpreter

.. _`how do I compile my own interpreters`:

-------------------------------------
How do I compile my own interpreters?
-------------------------------------
Begin by reading `Andrew Brown's tutorial`_ .

.. _`Andrew Brown's tutorial`: http://morepypy.blogspot.com/2011/04/tutorial-writing-interpreter-with-pypy.html

---------------------------------------------------------
Can RPython modules for PyPy be translated independently?
---------------------------------------------------------

No, you have to rebuild the entire interpreter.  This means two things:

* It is imperative to use test-driven development.  You have to exhaustively
  test your module in pure Python, before even attempting to
  translate it.  Once you translate it, you should have only a few typing
  issues left to fix, but otherwise the result should work out of the box.

* Second, and perhaps most important: do you have a really good reason
  for writing the module in RPython in the first place?  Nowadays you
  should really look at alternatives, like writing it in pure Python,
  using cffi_ if it needs to call C code.

In this context it is not that important to be able to translate
RPython modules independently of translating the complete interpreter.
(It could be done given enough efforts, but it's a really serious
undertaking.  Consider it as quite unlikely for now.)

----------------------------------------------------------
Why does PyPy draw a Mandelbrot fractal while translating?
----------------------------------------------------------

Because it's fun.

.. include:: _ref.txt

