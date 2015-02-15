
# patch sys.path so that this script can be executed standalone
import sys
from os.path import abspath, dirname
# this script is assumed to be at pypy/doc/tool/makeref.py
path = dirname(abspath(__file__))
path = dirname(dirname(dirname(path)))
sys.path.insert(0, path)


import py
import pypy
pypydir = py.path.local(pypy.__file__).join('..')
distdir = pypydir.dirpath()
issue_url = 'http://bugs.pypy.org/issue/pypy-dev/' 
bitbucket_url = 'https://bitbucket.org/pypy/pypy/src/default/'

import urllib2, posixpath


def makeref(docdir):
    reffile = docdir.join('_ref.txt') 

    linkrex = py.std.re.compile('`(\S+)`_')

    name2target = {}
    def addlink(linkname, linktarget): 
        assert linkname and linkname != '/'
        if linktarget in name2target: 
            if linkname in name2target[linktarget]: 
                return
        name2target.setdefault(linktarget, []).append(linkname)

    for textfile in sorted(docdir.listdir()):  # for subdirs, see below
        if textfile.ext != '.rst':
            continue
        content = textfile.read()
        found = False
        for linkname in linkrex.findall(content): 
            if '/' in linkname:
                found = True
                assert distdir.join(linkname).check(), "link %s in %s is dead" % (linkname, textfile)
                url = bitbucket_url + linkname
                if not linkname.endswith("/") and distdir.join(linkname).check(dir=1):
                    url += "/"
                addlink(linkname, url)
            elif linkname.startswith('issue'): 
                found = True
                addlink(linkname, issue_url+linkname)
        if found:
            assert ".. include:: _ref.txt" in content, "you need to include _ref.txt in %s" % (textfile, )

    items = name2target.items() 
    items.sort() 

    lines = []
    for linktarget, linknamelist in items: 
        linknamelist.sort()
        for linkname in linknamelist[:-1]: 
            lines.append(".. _`%s`:" % linkname)
        lines.append(".. _`%s`: %s" %(linknamelist[-1], linktarget))

    content  = ".. This file is generated automatically by makeref.py script,\n"
    content += "   which in turn is run manually.\n\n"
    content += "\n".join(lines) + "\n"
    reffile.write(content, mode="wb")

    print "wrote %d references to %r" %(len(lines), reffile)
    #print "last ten lines"
    #for x in lines[-10:]: print x


# We need to build a new _ref.txt for each directory that uses it, because
# they differ in the number of "../" that they need in the link targets...
makeref(pypydir.join('doc'))
makeref(pypydir.join('doc').join('jit'))
