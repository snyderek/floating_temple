from rpython.rtyper.lltypesystem.lltype import malloc
from rpython.rtyper.lltypesystem.rffi import CArray, CHAR

def CStringToPythonString(buf):
    lst = []
    i = 0
    while buf[i] != '\0':
        lst.append(buf[i])
        i += 1
    return ''.join(lst)

# The caller must free the returned buffer.
def PythonStringToCString(s):
    length = len(s)
    buf = malloc(CArray(CHAR), length + 1, flavor='raw')
    for i in range(length):
        buf[i] = s[i]
    buf[length] = '\0'
    return buf
