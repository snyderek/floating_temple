"""
Unary operations on SomeValues.
"""

from __future__ import absolute_import

from rpython.flowspace.operation import op
from rpython.annotator.model import (SomeObject, SomeInteger, SomeBool,
    SomeString, SomeChar, SomeList, SomeDict, SomeTuple, SomeImpossibleValue,
    SomeUnicodeCodePoint, SomeInstance, SomeBuiltin, SomeBuiltinMethod,
    SomeFloat, SomeIterator, SomePBC, SomeNone, SomeType, s_ImpossibleValue,
    s_Bool, s_None, unionof, add_knowntypedata,
    HarmlesslyBlocked, SomeWeakRef, SomeUnicodeString, SomeByteArray)
from rpython.annotator.bookkeeper import getbookkeeper, immutablevalue
from rpython.annotator import builtin
from rpython.annotator.binaryop import _clone ## XXX where to put this?
from rpython.annotator.model import AnnotatorError
from rpython.annotator.argument import simple_args, complex_args

UNARY_OPERATIONS = set([oper.opname for oper in op.__dict__.values()
                        if oper.dispatch == 1])


class __extend__(SomeObject):

    def type(self, *moreargs):
        if moreargs:
            raise Exception('type() called with more than one argument')
        r = SomeType()
        bk = getbookkeeper()
        op = bk._find_current_op(opname="type", arity=1, pos=0, s_type=self)
        r.is_type_of = [op.args[0]]
        return r

    def issubtype(self, s_cls):
        if hasattr(self, 'is_type_of'):
            vars = self.is_type_of
            annotator = getbookkeeper().annotator
            return builtin.builtin_isinstance(annotator.binding(vars[0]),
                                              s_cls, vars)
        if self.is_constant() and s_cls.is_constant():
            return immutablevalue(issubclass(self.const, s_cls.const))
        return s_Bool

    def len(self):
        return SomeInteger(nonneg=True)

    def bool_behavior(self, s):
        if self.is_immutable_constant():
            s.const = bool(self.const)
        else:
            s_len = self.len()
            if s_len.is_immutable_constant():
                s.const = s_len.const > 0

    def bool(s_obj):
        r = SomeBool()
        s_obj.bool_behavior(r)

        bk = getbookkeeper()
        knowntypedata = {}
        op = bk._find_current_op(opname="bool", arity=1)
        arg = op.args[0]
        s_nonnone_obj = s_obj
        if s_obj.can_be_none():
            s_nonnone_obj = s_obj.nonnoneify()
        add_knowntypedata(knowntypedata, True, [arg], s_nonnone_obj)
        r.set_knowntypedata(knowntypedata)
        return r

    def hash(self):
        raise AnnotatorError("cannot use hash() in RPython")

    def str(self):
        return SomeString()

    def unicode(self):
        return SomeUnicodeString()

    def repr(self):
        return SomeString()

    def hex(self):
        return SomeString()

    def oct(self):
        return SomeString()

    def id(self):
        raise Exception("cannot use id() in RPython; "
                        "see objectmodel.compute_xxx()")

    def int(self):
        return SomeInteger()

    def float(self):
        return SomeFloat()

    def delattr(self, s_attr):
        if self.__class__ != SomeObject or self.knowntype != object:
            getbookkeeper().warning(
                ("delattr on potentally non-SomeObjects is not RPythonic: delattr(%r,%r)" %
                 (self, s_attr)))

    def find_method(self, name):
        "Look for a special-case implementation for the named method."
        try:
            analyser = getattr(self.__class__, 'method_' + name)
        except AttributeError:
            return None
        else:
            return SomeBuiltinMethod(analyser, self, name)

    def getattr(self, s_attr):
        # get a SomeBuiltin if the SomeObject has
        # a corresponding method to handle it
        if not s_attr.is_constant() or not isinstance(s_attr.const, str):
            raise AnnotatorError("getattr(%r, %r) has non-constant argument"
                                 % (self, s_attr))
        attr = s_attr.const
        s_method = self.find_method(attr)
        if s_method is not None:
            return s_method
        # if the SomeObject is itself a constant, allow reading its attrs
        if self.is_immutable_constant() and hasattr(self.const, attr):
            return immutablevalue(getattr(self.const, attr))
        raise AnnotatorError("Cannot find attribute %r on %r" % (attr, self))
    getattr.can_only_throw = []

    def setattr(self, *args):
        return s_ImpossibleValue

    def bind_callables_under(self, classdef, name):
        return self   # default unbound __get__ implementation

    def simple_call(self, *args_s):
        return self.call(simple_args(args_s))

    def call_args(self, *args_s):
        return self.call(complex_args(args_s))

    def call(self, args, implicit_init=False):
        raise AnnotatorError("Cannot prove that the object is callable")

    def op_contains(self, s_element):
        return s_Bool
    op_contains.can_only_throw = []

    def hint(self, *args_s):
        return self

    def getslice(self, *args):
        return s_ImpossibleValue

    def setslice(self, *args):
        return s_ImpossibleValue

    def delslice(self, *args):
        return s_ImpossibleValue

    def pos(self):
        return s_ImpossibleValue
    neg = abs = ord = invert = long = iter = next = pos


class __extend__(SomeFloat):

    def pos(self):
        return self

    def neg(self):
        return SomeFloat()

    abs = neg

    def bool(self):
        if self.is_immutable_constant():
            return getbookkeeper().immutablevalue(bool(self.const))
        return s_Bool

class __extend__(SomeInteger):

    def invert(self):
        return SomeInteger(knowntype=self.knowntype)
    invert.can_only_throw = []

    def pos(self):
        return SomeInteger(knowntype=self.knowntype)

    pos.can_only_throw = []
    int = pos

    # these are the only ones which can overflow:

    def neg(self):
        return SomeInteger(knowntype=self.knowntype)

    neg.can_only_throw = []
    neg_ovf = _clone(neg, [OverflowError])

    def abs(self):
        return SomeInteger(nonneg=True, knowntype=self.knowntype)

    abs.can_only_throw = []
    abs_ovf = _clone(abs, [OverflowError])

class __extend__(SomeBool):
    def bool(self):
        return self

    def invert(self):
        return SomeInteger()

    invert.can_only_throw = []

    def neg(self):
        return SomeInteger()

    neg.can_only_throw = []
    neg_ovf = _clone(neg, [OverflowError])

    def abs(self):
        return SomeInteger(nonneg=True)

    abs.can_only_throw = []
    abs_ovf = _clone(abs, [OverflowError])

    def pos(self):
        return SomeInteger(nonneg=True)

    pos.can_only_throw = []
    int = pos

class __extend__(SomeTuple):

    def len(self):
        return immutablevalue(len(self.items))

    def iter(self):
        return SomeIterator(self)
    iter.can_only_throw = []

    def getanyitem(self):
        return unionof(*self.items)

    def getslice(self, s_start, s_stop):
        assert s_start.is_immutable_constant(),"tuple slicing: needs constants"
        assert s_stop.is_immutable_constant(), "tuple slicing: needs constants"
        items = self.items[s_start.const:s_stop.const]
        return SomeTuple(items)


class __extend__(SomeList):

    def method_append(self, s_value):
        self.listdef.resize()
        self.listdef.generalize(s_value)

    def method_extend(self, s_iterable):
        self.listdef.resize()
        if isinstance(s_iterable, SomeList):   # unify the two lists
            self.listdef.agree(s_iterable.listdef)
        else:
            s_iter = s_iterable.iter()
            self.method_append(s_iter.next())

    def method_reverse(self):
        self.listdef.mutate()

    def method_insert(self, s_index, s_value):
        self.method_append(s_value)

    def method_remove(self, s_value):
        self.listdef.resize()
        self.listdef.generalize(s_value)

    def method_pop(self, s_index=None):
        self.listdef.resize()
        return self.listdef.read_item()
    method_pop.can_only_throw = [IndexError]

    def method_index(self, s_value):
        self.listdef.generalize(s_value)
        return SomeInteger(nonneg=True)

    def len(self):
        s_item = self.listdef.read_item()
        if isinstance(s_item, SomeImpossibleValue):
            return immutablevalue(0)
        return SomeObject.len(self)

    def iter(self):
        return SomeIterator(self)
    iter.can_only_throw = []

    def getanyitem(self):
        return self.listdef.read_item()

    def op_contains(self, s_element):
        self.listdef.generalize(s_element)
        return s_Bool
    op_contains.can_only_throw = []

    def hint(self, *args_s):
        hints = args_s[-1].const
        if 'maxlength' in hints:
            # only for iteration over lists or dicts or strs at the moment,
            # not over an iterator object (because it has no known length)
            s_iterable = args_s[0]
            if isinstance(s_iterable, (SomeList, SomeDict, SomeString)):
                self = SomeList(self.listdef) # create a fresh copy
                self.listdef.resize()
                self.listdef.listitem.hint_maxlength = True
        elif 'fence' in hints:
            self = self.listdef.offspring()
        return self

    def getslice(self, s_start, s_stop):
        check_negative_slice(s_start, s_stop)
        return self.listdef.offspring()

    def setslice(self, s_start, s_stop, s_iterable):
        check_negative_slice(s_start, s_stop)
        if not isinstance(s_iterable, SomeList):
            raise Exception("list[start:stop] = x: x must be a list")
        self.listdef.mutate()
        self.listdef.agree(s_iterable.listdef)
        # note that setslice is not allowed to resize a list in RPython

    def delslice(self, s_start, s_stop):
        check_negative_slice(s_start, s_stop)
        self.listdef.resize()

def check_negative_slice(s_start, s_stop, error="slicing"):
    if isinstance(s_start, SomeInteger) and not s_start.nonneg:
        raise AnnotatorError("%s: not proven to have non-negative start" %
                             error)
    if isinstance(s_stop, SomeInteger) and not s_stop.nonneg and \
           getattr(s_stop, 'const', 0) != -1:
        raise AnnotatorError("%s: not proven to have non-negative stop" % error)


class __extend__(SomeDict):

    def _is_empty(self):
        s_key = self.dictdef.read_key()
        s_value = self.dictdef.read_value()
        return (isinstance(s_key, SomeImpossibleValue) or
                isinstance(s_value, SomeImpossibleValue))

    def len(self):
        if self._is_empty():
            return immutablevalue(0)
        return SomeObject.len(self)

    def iter(self):
        return SomeIterator(self)
    iter.can_only_throw = []

    def getanyitem(self, variant='keys'):
        if variant == 'keys':
            return self.dictdef.read_key()
        elif variant == 'values':
            return self.dictdef.read_value()
        elif variant == 'items':
            s_key   = self.dictdef.read_key()
            s_value = self.dictdef.read_value()
            if (isinstance(s_key, SomeImpossibleValue) or
                isinstance(s_value, SomeImpossibleValue)):
                return s_ImpossibleValue
            else:
                return SomeTuple((s_key, s_value))
        else:
            raise ValueError

    def method_get(self, key, dfl):
        self.dictdef.generalize_key(key)
        self.dictdef.generalize_value(dfl)
        return self.dictdef.read_value()

    method_setdefault = method_get

    def method_copy(self):
        return SomeDict(self.dictdef)

    def method_update(dct1, dct2):
        if s_None.contains(dct2):
            return SomeImpossibleValue()
        dct1.dictdef.union(dct2.dictdef)

    def method__prepare_dict_update(dct, num):
        pass

    def method_keys(self):
        return getbookkeeper().newlist(self.dictdef.read_key())

    def method_values(self):
        return getbookkeeper().newlist(self.dictdef.read_value())

    def method_items(self):
        return getbookkeeper().newlist(self.getanyitem('items'))

    def method_iterkeys(self):
        return SomeIterator(self, 'keys')

    def method_itervalues(self):
        return SomeIterator(self, 'values')

    def method_iteritems(self):
        return SomeIterator(self, 'items')

    def method_clear(self):
        pass

    def method_popitem(self):
        return self.getanyitem('items')

    def method_pop(self, s_key, s_dfl=None):
        self.dictdef.generalize_key(s_key)
        if s_dfl is not None:
            self.dictdef.generalize_value(s_dfl)
        return self.dictdef.read_value()

    def _can_only_throw(self, *ignore):
        if self.dictdef.dictkey.custom_eq_hash:
            return None    # r_dict: can throw anything
        return []          # else: no possible exception

    def op_contains(self, s_element):
        self.dictdef.generalize_key(s_element)
        if self._is_empty():
            s_bool = SomeBool()
            s_bool.const = False
            return s_bool
        return s_Bool
    op_contains.can_only_throw = _can_only_throw


class __extend__(SomeString,
                 SomeUnicodeString):

    def method_startswith(self, frag):
        if self.is_constant() and frag.is_constant():
            return immutablevalue(self.const.startswith(frag.const))
        return s_Bool

    def method_endswith(self, frag):
        if self.is_constant() and frag.is_constant():
            return immutablevalue(self.const.endswith(frag.const))
        return s_Bool

    def method_find(self, frag, start=None, end=None):
        check_negative_slice(start, end, "find")
        return SomeInteger()

    def method_rfind(self, frag, start=None, end=None):
        check_negative_slice(start, end, "rfind")
        return SomeInteger()

    def method_count(self, frag, start=None, end=None):
        check_negative_slice(start, end, "count")
        return SomeInteger(nonneg=True)

    def method_strip(self, chr=None):
        return self.basestringclass(no_nul=self.no_nul)

    def method_lstrip(self, chr=None):
        return self.basestringclass(no_nul=self.no_nul)

    def method_rstrip(self, chr=None):
        return self.basestringclass(no_nul=self.no_nul)

    def method_join(self, s_list):
        if s_None.contains(s_list):
            return SomeImpossibleValue()
        s_item = s_list.listdef.read_item()
        if s_None.contains(s_item):
            if isinstance(self, SomeUnicodeString):
                return immutablevalue(u"")
            return immutablevalue("")
        no_nul = self.no_nul and s_item.no_nul
        return self.basestringclass(no_nul=no_nul)

    def iter(self):
        return SomeIterator(self)
    iter.can_only_throw = []

    def getanyitem(self):
        return self.basecharclass()

    def method_split(self, patt, max=-1):
        if max == -1 and patt.is_constant() and patt.const == "\0":
            no_nul = True
        else:
            no_nul = self.no_nul
        s_item = self.basestringclass(no_nul=no_nul)
        return getbookkeeper().newlist(s_item)

    def method_rsplit(self, patt, max=-1):
        s_item = self.basestringclass(no_nul=self.no_nul)
        return getbookkeeper().newlist(s_item)

    def method_replace(self, s1, s2):
        return self.basestringclass(no_nul=self.no_nul and s2.no_nul)

    def getslice(self, s_start, s_stop):
        check_negative_slice(s_start, s_stop)
        result = self.basestringclass(no_nul=self.no_nul)
        return result

    def op_contains(self, s_element):
        if s_element.is_constant() and s_element.const == "\0":
            r = SomeBool()
            bk = getbookkeeper()
            op = bk._find_current_op(opname="contains", arity=2, pos=0, s_type=self)
            knowntypedata = {}
            add_knowntypedata(knowntypedata, False, [op.args[0]], self.nonnulify())
            r.set_knowntypedata(knowntypedata)
            return r
        else:
            return SomeObject.op_contains(self, s_element)
    op_contains.can_only_throw = []

    def method_format(self, *args):
        raise AnnotatorError("Method format() is not RPython")


class __extend__(SomeByteArray):
    def getslice(ba, s_start, s_stop):
        check_negative_slice(s_start, s_stop)
        return SomeByteArray()

class __extend__(SomeUnicodeString):
    def method_encode(self, s_enc):
        if not s_enc.is_constant():
            raise AnnotatorError("Non-constant encoding not supported")
        enc = s_enc.const
        if enc not in ('ascii', 'latin-1', 'utf-8'):
            raise AnnotatorError("Encoding %s not supported for unicode" % (enc,))
        return SomeString()
    method_encode.can_only_throw = [UnicodeEncodeError]


class __extend__(SomeString):
    def method_isdigit(self):
        return s_Bool

    def method_isalpha(self):
        return s_Bool

    def method_isalnum(self):
        return s_Bool

    def method_upper(self):
        return SomeString()

    def method_lower(self):
        return SomeString()

    def method_splitlines(self, s_keep_newlines=None):
        s_list = getbookkeeper().newlist(self.basestringclass())
        # Force the list to be resizable because ll_splitlines doesn't
        # preallocate the list.
        s_list.listdef.listitem.resize()
        return s_list

    def method_decode(self, s_enc):
        if not s_enc.is_constant():
            raise AnnotatorError("Non-constant encoding not supported")
        enc = s_enc.const
        if enc not in ('ascii', 'latin-1', 'utf-8'):
            raise AnnotatorError("Encoding %s not supported for strings" % (enc,))
        return SomeUnicodeString()
    method_decode.can_only_throw = [UnicodeDecodeError]

class __extend__(SomeChar, SomeUnicodeCodePoint):

    def len(self):
        return immutablevalue(1)

    def ord(self):
        return SomeInteger(nonneg=True)

class __extend__(SomeChar):

    def method_isspace(self):
        return s_Bool

    def method_isalnum(self):
        return s_Bool

    def method_islower(self):
        return s_Bool

    def method_isupper(self):
        return s_Bool

    def method_lower(self):
        return self

    def method_upper(self):
        return self

class __extend__(SomeIterator):

    def iter(self):
        return self
    iter.can_only_throw = []

    def _can_only_throw(self):
        can_throw = [StopIteration]
        if isinstance(self.s_container, SomeDict):
            can_throw.append(RuntimeError)
        return can_throw

    def next(self):
        if s_None.contains(self.s_container):
            return s_ImpossibleValue     # so far
        if self.variant == ("enumerate",):
            s_item = self.s_container.getanyitem()
            return SomeTuple((SomeInteger(nonneg=True), s_item))
        variant = self.variant
        if variant == ("reversed",):
            variant = ()
        return self.s_container.getanyitem(*variant)
    next.can_only_throw = _can_only_throw
    method_next = next


class __extend__(SomeInstance):

    def _true_getattr(self, attr):
        if attr == '__class__':
            return self.classdef.read_attr__class__()
        attrdef = self.classdef.find_attribute(attr)
        position = getbookkeeper().position_key
        attrdef.read_locations[position] = True
        s_result = attrdef.getvalue()
        # hack: if s_result is a set of methods, discard the ones
        #       that can't possibly apply to an instance of self.classdef.
        # XXX do it more nicely
        if isinstance(s_result, SomePBC):
            s_result = self.classdef.lookup_filter(s_result, attr,
                                                  self.flags)
        elif isinstance(s_result, SomeImpossibleValue):
            self.classdef.check_missing_attribute_update(attr)
            # blocking is harmless if the attribute is explicitly listed
            # in the class or a parent class.
            for basedef in self.classdef.getmro():
                if basedef.classdesc.all_enforced_attrs is not None:
                    if attr in basedef.classdesc.all_enforced_attrs:
                        raise HarmlesslyBlocked("get enforced attr")
        elif isinstance(s_result, SomeList):
            s_result = self.classdef.classdesc.maybe_return_immutable_list(
                attr, s_result)
        return s_result

    def getattr(self, s_attr):
        if s_attr.is_constant() and isinstance(s_attr.const, str):
            attr = s_attr.const
            return self._true_getattr(attr)
        raise AnnotatorError("A variable argument to getattr is not RPython")
    getattr.can_only_throw = []

    def setattr(self, s_attr, s_value):
        if s_attr.is_constant() and isinstance(s_attr.const, str):
            attr = s_attr.const
            # find the (possibly parent) class where this attr is defined
            clsdef = self.classdef.locate_attribute(attr)
            attrdef = clsdef.attrs[attr]
            attrdef.modified(clsdef)

            # if the attrdef is new, this must fail
            if attrdef.getvalue().contains(s_value):
                return
            # create or update the attribute in clsdef
            clsdef.generalize_attr(attr, s_value)

            if isinstance(s_value, SomeList):
                clsdef.classdesc.maybe_return_immutable_list(
                    attr, s_value)
        else:
            raise AnnotatorError("setattr(instance, variable_attr, value)")

    def bool_behavior(self, s):
        if not self.can_be_None:
            s.const = True

    def _emulate_call(self, meth_name, *args_s):
        bk = getbookkeeper()
        s_attr = self._true_getattr(meth_name)
        # record for calltables
        bk.emulate_pbc_call(bk.position_key, s_attr, args_s)
        return s_attr.call(simple_args(args_s))

    def iter(self):
        return self._emulate_call('__iter__')

    def next(self):
        return self._emulate_call('next')

    def len(self):
        return self._emulate_call('__len__')

    def getslice(self, s_start, s_stop):
        return self._emulate_call('__getslice__', s_start, s_stop)

    def setslice(self, s_start, s_stop, s_iterable):
        return self._emulate_call('__setslice__', s_start, s_stop, s_iterable)

class __extend__(SomeBuiltin):
    def simple_call(self, *args):
        return self.analyser(*args)

    def call(self, args, implicit_init=False):
        args_s, kwds = args.unpack()
        # prefix keyword arguments with 's_'
        kwds_s = {}
        for key, s_value in kwds.items():
            kwds_s['s_'+key] = s_value
        return self.analyser(*args_s, **kwds_s)


class __extend__(SomeBuiltinMethod):
    def _can_only_throw(self, *args):
        analyser_func = getattr(self.analyser, 'im_func', None)
        can_only_throw = getattr(analyser_func, 'can_only_throw', None)
        if can_only_throw is None or isinstance(can_only_throw, list):
            return can_only_throw
        return can_only_throw(self.s_self, *args)

    def simple_call(self, *args):
        return self.analyser(self.s_self, *args)
    simple_call.can_only_throw = _can_only_throw

    def call(self, args, implicit_init=False):
        args_s, kwds = args.unpack()
        # prefix keyword arguments with 's_'
        kwds_s = {}
        for key, s_value in kwds.items():
            kwds_s['s_'+key] = s_value
        return self.analyser(self.s_self, *args_s, **kwds_s)


class __extend__(SomePBC):

    def getattr(self, s_attr):
        assert s_attr.is_constant()
        if s_attr.const == '__name__':
            from rpython.annotator.description import ClassDesc
            if self.getKind() is ClassDesc:
                return SomeString()
        bookkeeper = getbookkeeper()
        return bookkeeper.pbc_getattr(self, s_attr)
    getattr.can_only_throw = []

    def setattr(self, s_attr, s_value):
        raise AnnotatorError("Cannot modify attribute of a pre-built constant")

    def call(self, args):
        bookkeeper = getbookkeeper()
        return bookkeeper.pbc_call(self, args)

    def bind_callables_under(self, classdef, name):
        d = [desc.bind_under(classdef, name) for desc in self.descriptions]
        return SomePBC(d, can_be_None=self.can_be_None)

    def bool_behavior(self, s):
        if not self.can_be_None:
            s.const = True

    def len(self):
        raise AnnotatorError("Cannot call len on a pbc")

class __extend__(SomeNone):
    def bind_callables_under(self, classdef, name):
        return self

    def getattr(self, s_attr):
        return s_ImpossibleValue
    getattr.can_only_throw = []

    def setattr(self, s_attr, s_value):
        return None

    def call(self, args):
        return s_ImpossibleValue

    def bool_behavior(self, s):
        s.const = False

    def len(self):
        # This None could later be generalized into a list, for example.
        # For now, we give the impossible answer (because len(None) would
        # really crash translated code).  It can be generalized later.
        return SomeImpossibleValue()

#_________________________________________
# weakrefs

class __extend__(SomeWeakRef):
    def simple_call(self):
        if self.classdef is None:
            return s_None   # known to be a dead weakref
        else:
            return SomeInstance(self.classdef, can_be_None=True)
