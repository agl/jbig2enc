#!/usr/bin/python
# Copyright 2006 Google Inc.
# Author: agl@imperialviolet.org (Adam Langley)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# JBIG2 Encoder
# https://github.com/agl/jbig2enc

import sys
import re
import struct
import glob
import os
import platform

# This is a very simple script to make a PDF file out of the output of a
# multipage symbol compression.
# Run ./jbig2 -s -p <other options> image1.jpeg image1.jpeg ...
# python pdf.py output > out.pdf

dpi = 72
ispy2 = platform.python_version_tuple()[0]=='2'
ispy3 = platform.python_version_tuple()[0]=='3'

class Ref:
  def __init__(self, x):
    self.x = x

  def __bytes__(self):
    return b"%d 0 R" % self.x

class Dict:
  def __init__(self, values = {}):
    self.d = {}
    self.d.update(values)

  def __bytes__(self):
    s = [b'<< ']
    for (x, y) in self.d.items():
      s.append(b'/%s ' % x.encode())
      s.append(y.encode())
      s.append(b"\n")
    s.append(b">>\n")

    return b''.join(s)

global_next_id = 1

class Obj:
  next_id = 1
  def __init__(self, d = {}, stream = None):
    global global_next_id

    if stream is not None:
      d['Length'] = str(len(stream))
    self.d = Dict(d)
    self.stream = stream
    self.id = global_next_id
    global_next_id += 1

  def __bytes__(self):
    s = []
    s.append(bytes(self.d))
    if self.stream is not None:
      s.append(b'stream\n')
      if ispy3 and isinstance(self.stream, str):
        s.append(self.stream.encode())
      else:
        s.append(self.stream)
      s.append(b'\nendstream\n')
    s.append(b'endobj\n')
    return b''.join(s)

class Doc:
  def __init__(self):
    self.objs = []
    self.pages = []

  def add_object(self, o):
    self.objs.append(o)
    return o

  def add_page(self, o):
    self.pages.append(o)
    return self.add_object(o)

  def __bytes__(self):
    a = []
    j = [0]
    offsets = []

    def add(x):
      a.append(x)
      j[0] += len(x) + 1
    add(b'%PDF-1.4')
    for o in self.objs:
      offsets.append(j[0])
      add(b'%d 0 obj' % o.id)
      add(bytes(o))
    xrefstart = j[0]
    a.append(b'xref')
    a.append(b'0 %d' % (len(offsets) + 1))
    a.append(b'0000000000 65535 f ')
    for o in offsets:
      a.append(b'%010d 00000 n ' % o)
    a.append(b'')
    a.append(b'trailer')
    a.append(b'<< /Size %d\n/Root 1 0 R >>' % (len(offsets) + 1))
    a.append(b'startxref')
    a.append(b'%d' % xrefstart)
    a.append(b'%%EOF')

    # sys.stderr.write(str(offsets) + "\n")
    return b'\n'.join(a)

def ref(x):
  return '%d 0 R' % x

def main(symboltable='symboltable', pagefiles=glob.glob('page-*')):
  doc = Doc()
  doc.add_object(Obj({'Type' : '/Catalog', 'Outlines' : ref(2), 'Pages' : ref(3)}))
  doc.add_object(Obj({'Type' : '/Outlines', 'Count': '0'}))
  pages = Obj({'Type' : '/Pages'})
  doc.add_object(pages)
  if symboltable:
    with open(symboltable, 'rb') as f:
      symd = doc.add_object(Obj({}, f.read()))
  page_objs = []

  pagefiles.sort()
  for p in pagefiles:
    try:
      contents = open(p, "rb").read()
    except IOError:
      sys.stderr.write("error reading page file %s\n"% p)
      continue
    (width, height,xres,yres) = struct.unpack('>IIII', contents[11:27])

    if xres==0:
      xres=dpi
    if yres==0:
      yres=dpi

    lexicon={'Type': '/XObject', 'Subtype': '/Image', 'Width':
        str(width), 'Height': str(height), 'ColorSpace': '/DeviceGray',
        'BitsPerComponent': '1', 'Filter': '/JBIG2Decode'}
    if symboltable:
        lexicon['DecodeParms']=' << /JBIG2Globals %d 0 R >>' % symd.id
    xobj = Obj(lexicon, contents)
    contents = Obj({}, 'q %f 0 0 %f 0 0 cm /Im1 Do Q' % (float(width * 72) / xres, float(height * 72) / yres))
    resources = Obj({'ProcSet': '[/PDF /ImageB]',
        'XObject': '<< /Im1 %d 0 R >>' % xobj.id})
    page = Obj({'Type': '/Page', 'Parent': '3 0 R',
        'MediaBox': '[ 0 0 %f %f ]' % (float(width * 72) / xres, float(height * 72) / yres),
        'Contents': ref(contents.id),
        'Resources': ref(resources.id)})
    [doc.add_object(x) for x in [xobj, contents, resources, page]]
    page_objs.append(page)

    pages.d.d['Count'] = str(len(page_objs))
    pages.d.d['Kids'] = '[' + ' '.join([ref(x.id) for x in page_objs]) + ']'

  if ispy2:
    print(doc)
  elif ispy3:
    sys.stdout.buffer.write(bytes(doc))
  else:
    raise Exception("unexpected python version: %s" % platform.python_version_tuple()[0])


def usage(script, msg):
  if msg:
    sys.stderr.write("%s: %s\n\n" % (script, msg))

  sys.stderr.write("Usage:\n"
                   "  %s [basename] > out.pdf\n"
                   "  %s -s [page.jb2]... > out.pdf\n"
                   "\n"
                   "  Read symbol table from `basename.sym' and pages from `basename.[0-9]*'\n"
                   "    if basename not given: symbol table from `symboltable', pages from `page-*'\n"
                   "\n"
                   "  -s: standalone mode (no global symbol table)\n"
                   % (script, script))
  sys.exit(1)


if __name__ == '__main__':
  if ispy2 and sys.platform == "win32":
    import msvcrt
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)

  if '-s' in sys.argv:
    # "standalone" .jb2 mode
    sym = False
    pages = sys.argv[1:]
    pages.remove('-s')
    if len(pages) == 0:
      usage(sys.argv[0], "no pages found!")
  else:
    if len(sys.argv) == 2:
      sym = sys.argv[1] + '.sym'
      pages = glob.glob(sys.argv[1] + '.[0-9]*')
    elif len(sys.argv) == 1:
      sym = 'symboltable'
      pages = glob.glob('page-*')
    else:
      usage(sys.argv[0], "wrong number of args!")

    if not os.path.exists(sym):
      usage(sys.argv[0], "symbol table %s not found!"% sym)
    elif len(pages) == 0:
      usage(sys.argv[0], "no pages found!")

  main(sym, pages)
