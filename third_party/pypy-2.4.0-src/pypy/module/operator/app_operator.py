'''NOT_RPYTHON: because of attrgetter and itemgetter
Operator interface.

This module exports a set of operators as functions. E.g. operator.add(x,y) is
equivalent to x+y.
'''

import types


def countOf(a,b):
    'countOf(a, b) -- Return the number of times b occurs in a.'
    count = 0
    for x in a:
        if x == b:
            count += 1
    return count

def delslice(obj, start, end):
    'delslice(a, b, c) -- Same as del a[b:c].'
    if not isinstance(start, int) or not isinstance(end, int):
        raise TypeError("an integer is expected")
    del obj[start:end]
__delslice__ = delslice

def getslice(a, start, end):
    'getslice(a, b, c) -- Same as a[b:c].'
    if not isinstance(start, int) or not isinstance(end, int):
        raise TypeError("an integer is expected")
    return a[start:end]
__getslice__ = getslice

def indexOf(a, b):
    'indexOf(a, b) -- Return the first index of b in a.'
    index = 0
    for x in a:
        if x == b:
            return index
        index += 1
    raise ValueError('sequence.index(x): x not in sequence')

def isMappingType(obj,):
    'isMappingType(a) -- Return True if a has a mapping type, False otherwise.'
    if isinstance(obj, types.InstanceType):
        return hasattr(obj, '__getitem__')
    return hasattr(obj, '__getitem__') and not hasattr(obj, '__getslice__')

def isNumberType(obj,):
    'isNumberType(a) -- Return True if a has a numeric type, False otherwise.'
    return hasattr(obj, '__int__') or hasattr(obj, '__float__')

def isSequenceType(obj,):
    'isSequenceType(a) -- Return True if a has a sequence type, False otherwise.'
    if isinstance(obj, dict):
        return False
    return hasattr(obj, '__getitem__')

def repeat(obj, num):
    'repeat(a, b) -- Return a * b, where a is a sequence, and b is an integer.'
    if not isinstance(num, (int, long)):
        raise TypeError('an integer is required')
    if not isSequenceType(obj):
        raise TypeError("non-sequence object can't be repeated")

    return obj * num

__repeat__ = repeat

def setslice(a, b, c, d):
    'setslice(a, b, c, d) -- Same as a[b:c] = d.'
    a[b:c] = d
__setslice__ = setslice


def _resolve_attr_chain(chain, obj, idx=0):
    obj = getattr(obj, chain[idx])
    if idx + 1 == len(chain):
        return obj
    else:
        return _resolve_attr_chain(chain, obj, idx + 1)


class _simple_attrgetter(object):
    def __init__(self, attr):
        self._attr = attr

    def __call__(self, obj):
        return getattr(obj, self._attr)


class _single_attrgetter(object):
    def __init__(self, attrs):
        self._attrs = attrs

    def __call__(self, obj):
        return _resolve_attr_chain(self._attrs, obj)


class _multi_attrgetter(object):
    def __init__(self, attrs):
        self._attrs = attrs

    def __call__(self, obj):
        return tuple([
            _resolve_attr_chain(attrs, obj)
            for attrs in self._attrs
        ])


def attrgetter(attr, *attrs):
    if (
        not isinstance(attr, basestring) or
        not all(isinstance(a, basestring) for a in attrs)
    ):
        def _raise_typeerror(obj):
            raise TypeError(
                "argument must be a string, not %r" % type(attr).__name__
            )
        return _raise_typeerror
    if attrs:
        return _multi_attrgetter([
            a.split(".") for a in [attr] + list(attrs)
        ])
    elif "." not in attr:
        return _simple_attrgetter(attr)
    else:
        return _single_attrgetter(attr.split("."))


class itemgetter(object):
    def __init__(self, item, *items):
        self._single = not bool(items)
        if self._single:
            self._idx = item
        else:
            self._idx = [item] + list(items)

    def __call__(self, obj):
        if self._single:
            return obj[self._idx]
        else:
            return tuple([obj[i] for i in self._idx])


class methodcaller(object):
    def __init__(self, method_name, *args, **kwargs):
        self._method_name = method_name
        self._args = args
        self._kwargs = kwargs

    def __call__(self, obj):
        return getattr(obj, self._method_name)(*self._args, **self._kwargs)
