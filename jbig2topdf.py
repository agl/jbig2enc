#!/usr/bin/env python3
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

import glob
import struct
import sys
from pathlib import Path

# This is a very simple script to make a PDF file out of the output of a
# multipage symbol compression.
# Run ./jbig2 -s -p <other options> image1.jpeg image1.jpeg ...
# python jbig2topdf.py output > out.pdf


dpi = 72  # Default DPI value


class Ref:
    def __init__(self, x: int):
        self.x = x

    def __str__(self) -> str:
        return f"{self.x} 0 R"


class Dict:
    def __init__(self, values: dict = None):
        if values is None:
            values = {}
        self.d = values.copy()

    def __str__(self) -> str:
        entries = [f"/{key} {value}" for key, value in self.d.items()]
        return f"<< {' '.join(entries)} >>\n"


class Obj:
    next_id = 1

    def __init__(self, d: dict = None, stream: str = None):
        if d is None:
            d = {}
        if stream is not None:
            d["Length"] = str(len(stream))
        self.d = Dict(d)
        self.stream = stream
        self.id = Obj.next_id
        Obj.next_id += 1

    def __str__(self) -> str:
        result = [str(self.d)]
        if self.stream is not None:
            result.append(f"stream\n{self.stream}\nendstream\n")
        result.append("endobj\n")
        return "".join(result)


class Doc:
    def __init__(self):
        self.objs = []
        self.pages = []

    def add_object(self, obj: Obj) -> Obj:
        """Adds an object to the document."""
        self.objs.append(obj)
        return obj

    def add_page(self, page: Obj) -> Obj:
        """Adds a page to the document and the list of objects."""
        self.pages.append(page)
        return self.add_object(page)

    def __str__(self) -> str:
        output = []
        offsets = []
        current_offset = 0

        def add_line(line: str):
            nonlocal current_offset
            output.append(line)
            current_offset += len(line) + 1  # Adding 1 for the newline character

        # PDF header
        add_line("%PDF-1.4")

        # Add each object and track its byte offset
        for obj in self.objs:
            offsets.append(current_offset)
            add_line(f"{obj.id} 0 obj")
            add_line(str(obj))

        # Cross-reference table
        xref_start = current_offset
        add_line("xref")
        add_line(f"0 {len(offsets) + 1}")
        add_line("0000000000 65535 f ")
        for offset in offsets:
            add_line(f"{offset:010} 00000 n ")

        # Trailer and EOF
        add_line("trailer")
        add_line(f"<< /Size {len(offsets) + 1}\n/Root 1 0 R >>")
        add_line("startxref")
        add_line(str(xref_start))
        add_line("%%EOF")

        return "\n".join(output)


def ref(x: int) -> str:
    """Creates a PDF reference string."""
    return f"{x} 0 R"


def create_pdf(symboltable: str = "symboltable", pagefiles: list = None):
    """Creates a PDF document from a symbol table and a list of page files."""
    if pagefiles is None:
        pagefiles = glob.glob("page-*")

    doc = Doc()

    # Add catalog and outlines objects
    catalog_obj = Obj({"Type": "/Catalog", "Outlines": ref(2), "Pages": ref(3)})
    outlines_obj = Obj({"Type": "/Outlines", "Count": "0"})
    pages_obj = Obj({"Type": "/Pages"})

    doc.add_object(catalog_obj)
    doc.add_object(outlines_obj)
    doc.add_object(pages_obj)

    # Read the symbol table
    try:
        with open(symboltable, "rb") as sym_file:
            symd = doc.add_object(Obj({}, sym_file.read().decode("latin1")))
    except IOError:
        sys.stderr.write(f"Error reading symbol table: {symboltable}\n")
        return

    page_objs = []
    pagefiles.sort()

    for p in pagefiles:
        try:
            with open(p, mode="rb") as page_file:
                contents = page_file.read()
        except IOError:
            sys.stderr.write(f"Error reading page file: {p}\n")
            continue

        try:
            width, height, xres, yres = struct.unpack(">IIII", contents[11:27])
        except struct.error:
            sys.stderr.write(f"Error unpacking page file: {p}\n")
            continue

        # Set default resolution if missing
        xres = xres or dpi
        yres = yres or dpi

        # Create XObject (image) for the page
        xobj = Obj(
            {
                "Type": "/XObject",
                "Subtype": "/Image",
                "Width": str(width),
                "Height": str(height),
                "ColorSpace": "/DeviceGray",
                "BitsPerComponent": "1",
                "Filter": "/JBIG2Decode",
                "DecodeParms": f"<< /JBIG2Globals {symd.id} 0 R >>",
            },
            contents.decode("latin1"),
        )

        # Create content stream for the page
        contents_obj = Obj(
            {},
            f"q {float(width * 72) / xres} 0 0 {float(height * 72) / yres} 0 0 cm /Im1 Do Q",
        )

        # Create resource dictionary for the page
        resources_obj = Obj(
            {"ProcSet": "[/PDF /ImageB]", "XObject": f"<< /Im1 {xobj.id} 0 R >>"}
        )

        # Create the page object
        page_obj = Obj(
            {
                "Type": "/Page",
                "Parent": "3 0 R",
                "MediaBox": f"[ 0 0 {float(width * 72) / xres} {float(height * 72) / yres} ]",
                "Contents": ref(contents_obj.id),
                "Resources": ref(resources_obj.id),
            }
        )

        # Add objects to the document
        for obj in (xobj, contents_obj, resources_obj, page_obj):
            doc.add_object(obj)

        page_objs.append(page_obj)

        # Update pages object
        pages_obj.d.d["Count"] = str(len(page_objs))
        pages_obj.d.d["Kids"] = "[" + " ".join([ref(x.id) for x in page_objs]) + "]"

    # Output the final PDF document to stdout
    sys.stdout.buffer.write(str(doc).encode("latin1"))


def usage(script, msg):
    """Display usage information and an optional error message."""
    if msg:
        sys.stderr.write("%s: %s\n" % (script, msg))
    sys.stderr.write("Usage: %s [file_basename] > out.pdf\n" % script)
    sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) == 2:
        base_name = sys.argv[1]
        sym = f"{base_name}.sym"
        pages = glob.glob(f"{base_name}.[0-9]*")
    elif len(sys.argv) == 1:
        sym = "symboltable"
        pages = glob.glob("page-*")
    else:
        usage(sys.argv[0], "wrong number of arguments!")

    # Validate that the symbol table exists
    sym_path = Path(sym)
    if not sym_path.exists():
        usage(sys.argv[0], f"symbol table '{sym}' not found!")

    # Validate that pages were found
    if not pages:
        usage(sys.argv[0], "no pages found!")

    create_pdf(sym, pages)
