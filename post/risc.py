# The risc post-processor - translates M/o/Vfuscated C into only 32 bit
# mem2reg or reg2mem MOV instructions, using only base+offset addressing.  This
# is primarily useful as an intermediate step towards translating code into
# instructions that have restrictive memory access sizes.

# This is a post-processing routine that can be run on assembly output from the
# M/o/Vfuscator.  It converts the M/o/Vfuscator's mov instructions into an
# alternative set of minimalistic instructions.

# To use:
#   movcc example.c -S
#   python this.py example.s
#   as --32 example.s -o example.o
#   movcc example.o

# Note that the final linking step above will add the M/o/Vfuscator crt
# libraries; you may wish to run these libraries through this same script, to
# ensure the entire executable is converted.  See the M/o/Vfuscator build script
# for an example of building the crt libraries.

import sys
import hashlib
import re
from math import log

def ismem(term):
    return "(" in term or "%" not in term

def compose(offset, base, index, scale):
    if not base and not index and not scale:
        return "(%s)" % offset
    elif not index and not scale:
        return "%s(%s)" % (offset, base)
    elif not scale:
        return "%s(%s,%s,1)" % (offset, base, index)
    else:
        return "%s(%s,%s,%s)" % (offset, base, index, scale)

def decompose(term):
    offset = base = index = scale = ""
    r = re.compile(r'(.*)\((%.*),(%.*),(.*)\)')
    if r.match(term):
        (offset,base,index,scale) = r.search(term).groups()
    else:
        r = re.compile(r'(.*)\(,(%.*),(.*)\)')
        if r.match(term):
            (offset,index,scale) = r.search(term).groups()
        else:
            r = re.compile(r'(.*)\((%.*),(%.*)\)') 
            if r.match(term):
                (offset,base,index) = r.search(term).groups()
            else:
                r = re.compile(r'(.*)\((%.*)\)') 
                if r.match(term):
                    (offset,base) = r.search(term).groups()
                else:
                    r = re.compile(r'\((.*)\)') 
                    if r.match(term):
                        (offset,) = r.search(term).groups()
                    else:
                        r = re.compile(r'(.*)') 
                        if r.match(term):
                            (offset,) = r.search(term).groups()
                        else:
                            raise Exception 
    return (offset, base, index, scale)

XFER_SIZE = 4

with open(sys.argv[1]) as f:
    asm = f.readlines()

# pass 1:
# replace all constant references with memory references instead
with open(sys.argv[1], 'w') as f:
    for l in asm:
        if l.startswith("mov"):
            tok = l.find(",", l.find(")"))
            if tok == -1:
                tok = l.find(",")
            source = l[l.index(" "):tok].strip()
            dest = l[tok+1:].strip()
            end = dest.find(" ")
            if end != -1:
                dest = dest[:end]

            # NOTE: requires M/o/Vfuscator to only produce dword constants
            if source.startswith("$"):
                f.write("#constant> " + l)

                # have to jump through some hoops due to as and ld limitations
                # on absolutes 
                c = hashlib.md5(source[1:]).hexdigest()
                f.write(".section .data\n")
                f.write(".ifndef .C%s\n" % c)
                f.write(".C%s: .long %s\n" % (c, source[1:]))
                f.write(".endif\n")
                f.write(".section .text\n")
                f.write("movl (.C%s), %%ebp\n" % c)
                f.write("movl %%ebp, %s\n" % dest)
            else:
                f.write(l)

        else:
            f.write(l)

# pass 2:
# replace all register to register transfers
with open(sys.argv[1]) as f:
    asm = f.readlines()

with open(sys.argv[1], 'w') as f:
    f.write(".section .data\n")
    f.write(".r2r: .long 0\n")
    f.write(".section .text\n")

    for l in asm:
        if l.startswith("mov"):
            f.write("#mov32> " + l)

            tok = l.find(",", l.find(")"))
            if tok == -1:
                tok = l.find(",")
            source = l[l.index(" "):tok].strip()
            dest = l[tok+1:].strip()
            end = dest.find(" ")
            if end != -1:
                dest = dest[:end]


            if l.startswith("movb"):
                s = "b"
            elif l.startswith("movw"):
                s = "w"
            elif l.startswith("movl"):
                s = "l"

            if source.startswith("%") and dest.startswith("%"):
                f.write("mov%s %s, (.r2r)\n" % (s, source))
                f.write("mov%s (.r2r), %s\n" % (s, dest))
            else:
                f.write(l)

        else:
            f.write(l)

# pass 3:
# pad .data and .bss sections to allow data accesses to extend past boundaries
with open(sys.argv[1]) as f:
    asm = f.readlines()

with open(sys.argv[1], 'w') as f:
    f.write("# section padding\n")
    f.write(".section .data\n")
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".long 0\n")
    f.write(".section .bss\n")
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".long 0\n")
    f.write("# end padding\n")
    f.write("# mov32 shuffle space\n")
    f.write(".section .data\n")
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".s_a%d: .byte 0\n" % i)
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".s_b%d: .byte 0\n" % i)
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".s_c%d: .byte 0\n" % i)
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".r_a%d: .byte 0\n" % i)
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".r_b%d: .byte 0\n" % i)
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".r_c%d: .byte 0\n" % i)
    f.write("# end shuffle space\n")
    for l in asm:
        f.write(l)
    f.write("# section padding\n")
    f.write(".section .data\n")
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".long 0\n")
    f.write(".section .bss\n")
    for i in xrange(0,(XFER_SIZE+1)/4):
        f.write(".long 0\n")
    f.write("# end padding\n")

# pass 4:
# convert all transfers to 32 bits
with open(sys.argv[1]) as f:
    asm = f.readlines()

with open(sys.argv[1], 'w') as f:
    for l in asm:
        if l.startswith("mov"):
            f.write("#mov32> " + l)

            tok = l.find(",", l.find(")"))
            if tok == -1:
                tok = l.find(",")
            source = l[l.index(" "):tok].strip()
            dest = l[tok+1:].strip()
            end = dest.find(" ")
            if end != -1:
                dest = dest[:end]


            # warning: ebp used in a previous pass to load immediates.  it's
            # okay since it was loading 32 bit values, and won't be translated
            # here.
            sr = "%ebp"

            if l.startswith("movb"):
                if source.startswith("%"):
                    # r8 -> m8
                    r32 = "%%e%cx" % source[1]
                    m = dest[dest.index("(")+1:dest.index(")")]
                    if not "%" in m:
                        # "(b)" format
                        b = m
                        si = ""
                    else:
                        # "b(si)" format
                        b = dest[:dest.index("(")]
                        si = "(" + m + ")"

                    f.write("movl %s, (.s_b0)\n" % r32)

                    f.write("movl %s%+d%s, %s\n" % (b, -XFER_SIZE, si, sr))

                    if source[2] == 'h':
                        f.write("movl %s, (.s_b0%+d)\n" % (sr, -XFER_SIZE+1))
                        f.write("movl (.s_b0%+d), %s\n" % (-XFER_SIZE+2, sr))
                    elif source[2] == 'l':
                        f.write("movl %s, (.s_b0%+d)\n" % (sr, -XFER_SIZE))
                        f.write("movl (.s_b0%+d), %s\n" % (-XFER_SIZE+1, sr))
                    else:
                        raise Exception

                    f.write("movl %s, %s%+d%s\n" % (sr, b, -XFER_SIZE+1, si))

                else:
                    # m8 -> r8
                    r32 = "%%e%cx" % dest[1]
                    m = source[source.index("(")+1:source.index(")")]
                    if not "%" in m:
                        # "(b)" format
                        b = m
                        si = ""
                    else:
                        # "b(si)" format
                        b = source[:source.index("(")]
                        si = "(" + m + ")"

                    f.write("movl %s, (.r_b0)\n" % r32)

                    if dest[2] == 'h':
                        f.write("movl %s%s, %s\n" % (b, si, sr))
                        f.write("movl %s, (.s_b0+1)\n" % (sr))
                        f.write("movl (.r_b0+2), %s\n" % (sr))
                        f.write("movl %s, (.s_b0+2)\n" % (sr))
                        f.write("movl (.r_b0%+d), %s\n" % (sr, -XFER_SIZE+1))
                        f.write("movl %s, (.s_b0%+d)\n" % (sr, -XFER_SIZE+1))
                    elif dest[2] == 'l':
                        f.write("movl %s%s, %s\n" % (b, si, sr))
                        f.write("movl %s, (.s_b0)\n" % (sr))
                        f.write("movl (.r_b0+1), %s\n" % (sr))
                        f.write("movl %s, (.s_b0+1)\n" % (sr))
                    else:
                        raise Exception

                    f.write("movl (.s_b0), %s\n" % r32)

            elif l.startswith("movw"):
                if source.startswith("%"):
                    # r16 -> m16
                    r32 = "%%e%cx" % source[1]
                    m = dest[dest.index("(")+1:dest.index(")")]
                    if not "%" in m:
                        # "(b)" format
                        b = m
                        si = ""
                    else:
                        # "b(si)" format
                        b = dest[:dest.index("(")]
                        si = "(" + m + ")"

                    f.write("movl %s, (.s_b0)\n" % r32)

                    f.write("movl %s%+d%s, %s\n" % (b, -XFER_SIZE, si, sr))

                    f.write("movl %s, (.s_b0%+d)\n" % (sr, -XFER_SIZE))
                    f.write("movl (.s_b0%+d), %s\n" % (-XFER_SIZE+2, sr))

                    f.write("movl %s, %s%+d%s\n" % (sr, b, -XFER_SIZE+2, si))

                else:
                    # m16 -> r16
                    r32 = "%%e%cx" % dest[1]
                    m = source[source.index("(")+1:source.index(")")]
                    if not "%" in m:
                        # "(b)" format
                        b = m
                        si = ""
                    else:
                        # "b(si)" format
                        b = source[:source.index("(")]
                        si = "(" + m + ")"

                    f.write("movl %s, (.r_b0)\n" % r32)

                    f.write("movl %s%s, %s\n" % (b, si, sr))
                    f.write("movl %s, (.s_b0)\n" % (sr))
                    f.write("movl (.r_b0+2), %s\n" % (sr))
                    f.write("movl %s, (.s_b0+2)\n" % (sr))

                    f.write("movl (.s_b0), %s\n" % r32)

            elif l.startswith("movl"):
                f.write(l)

        else:
            f.write(l)


# pass 5: convert all addressing to base + offset addressing
# e.g.: [eax+4*ebx+12345678] -> [ecx+12345678]
# This may not be the actual desired addressing mode, but it is much easier to
# translate to other modes after initially converted to base/offset

with open(sys.argv[1]) as f:
    asm = f.readlines()

with open(sys.argv[1], 'w') as f:
    f.write(".section .data\n")
    f.write(".eax: .long 0\n")
    f.write(".ebx: .long 0\n")
    f.write(".ecx: .long 0\n")
    f.write(".edx: .long 0\n")
    f.write(".esi: .long 0\n")
    f.write(".edi: .long 0\n")
    f.write(".ebp: .long 0\n")
    f.write(".esp: .long 0\n")
    f.write(".zero: .long 0\n")
    f.write(".long 0\n")
    f.write(".chop: .long 0\n")
    f.write(".long 0\n")
    f.write(".sum_x: .long 0\n")

    for i in (0,1,2,3):
        f.write(".ind%dl: .long 0\n" % i)
        f.write(".ind%dh: .long 0\n" % i)

    f.write(".long 0\n")
    f.write(".oraddr: .long 0\n")
    f.write(".long 0\n")

    f.write(".long 0\n")
    f.write(".orresult: .long 0\n")
    f.write(".long 0\n")

    for k in (0,1,2,3):
        f.write(".scale%dl:\n" % (2**k))
        for i in xrange(0,256):
            f.write(".byte 0x%02x\n" % ((i<<k)&0xff))
        f.write(".scale%dh:\n" % (2**k))
        for i in xrange(0,256):
            f.write(".byte 0x%02x\n" % (((i<<k)&0xff00)>>8))

    f.write(".riscor:\n")
    for i in xrange(0,0x10000):
        f.write(".byte 0x%02x\n" % ((i&0xff)|((i&0xff00)>>8)))

    f.write(".long 0\n")
    f.write(".sumaddr: .long 0\n")
    f.write(".long 0\n")

    f.write(".long 0\n")
    f.write(".sumcarry: .long 0\n")
    f.write(".long 0\n")

    f.write(".long 0\n")
    f.write(".sumresult: .long 0\n")
    f.write(".long 0\n")

    for i in (0,1,2,3):
        f.write(".sum%dl: .long 0, 0\n" % i)
        f.write(".sum%dh: .long 0, 0\n" % i)

    f.write(".riscaddl:\n")
    for i in xrange(0,0x20000):
        f.write(".byte 0x%02x\n" % \
                (((i&0xff)+((i&0xff00)>>8)+((i&0x10000)>>16))&0xff))
    f.write(".riscaddh:\n")
    for i in xrange(0,0x20000):
        f.write(".byte 0x%02x\n" % \
                (((i&0xff)+((i&0xff00)>>8)+((i&0x10000)>>16))>>8))

    f.write(".section .text\n")

    # edi used as an always 0 register, to convert [offset] addressing to
    # [base+offset] addressing
    f.write("movl .zero, %edi\n")

    for l in asm:
        if l.startswith("mov"):
            f.write("#risc> " + l)

            match = re.search(r'^mov([bwl])\s+(.*)\s*,\s*([^#]*).*\n$', l)

            (size, source, dest) = match.groups()

            if ismem(source):
                # mov mem, reg
                (offset, base, index, scale) = decompose(source)
            else:
                # mov reg, mem
                (offset, base, index, scale) = decompose(dest)

            f.write("movl .zero(%edi), %esi\n")

            if index and scale:
                for b in ('l','h'):
                    for i in (0,1,2,3):
                        # get byte
                        f.write("movl .%s(%%edi), %%esi\n" % index[1:])
                        f.write("movl %esi, .chop(%edi)\n")
                        #f.write("movl %s, .chop(%%edi)\n" % index)
                        f.write("movl .zero(%edi), %esi\n")
                        f.write("movl %%esi, .chop+%d(%%edi)\n" % (i+1))
                        f.write("movl .chop+%d(%%edi), %%esi\n" % (i))

                        # shift
                        f.write("movl .scale%s%c(%%esi), %%esi\n" % (scale, b))
                        f.write("movl %esi, .chop(%edi)\n")
                        f.write("movl .zero(%edi), %esi\n")
                        f.write("movl %esi, .chop+1(%edi)\n")
                        f.write("movl .chop(%edi), %esi\n")

                        # save
                        f.write("movl %%esi, .ind%d%c(%%edi)\n" % (i, b))

                f.write("movl .ind0l(%edi), %esi\n")
                f.write("movl %esi, .orresult(%edi)\n")
                for i in (1,2,3):
                    f.write("movl .ind%dh(%%edi), %%esi\n" % (i-1))
                    f.write("movl %esi, .oraddr(%edi)\n")
                    f.write("movl .ind%dl(%%edi), %%esi\n" % (i))
                    f.write("movl %esi, .oraddr+1(%edi)\n")
                    f.write("movl .oraddr(%edi), %esi\n")
                    f.write("movl .riscor(%esi), %esi\n")
                    f.write("movl %%esi, .orresult+%d(%%edi)\n" % i)

                f.write("movl .orresult(%edi), %esi\n")

            elif index:
                #f.write("movl %s, %s\n" % (index, "%esi"))
                f.write("movl .%s(%%edi), %s\n" % (index[1:], "%esi"))

            if base and index:
                f.write("# sum base\n")
                f.write("movl %esi, .sum_x(%edi)\n")

                f.write("movl .zero(%edi), %esi\n")
                f.write("movl %esi, .sumcarry(%edi)\n")

                for i in (0,1,2,3):
                    # merge
                    f.write("movl .%s(%%edi), %%esi\n" % base[1:])
                    f.write("movl %esi, .chop(%edi)\n")
                    #f.write("movl %s, .chop(%%edi)\n" % base)
                    f.write("movl .chop+%d(%%edi), %%esi\n" % (i))
                    f.write("movl %esi, .sumaddr(%edi)\n")

                    f.write("movl .sum_x+%d(%%edi), %%esi\n" % (i))
                    f.write("movl %esi, .sumaddr+1(%edi)\n")

                    f.write("movl .sumcarry(%edi), %esi\n")
                    f.write("movl %esi, .sumaddr+2(%edi)\n")

                    # sum
                    f.write("movl .sumaddr(%edi), %esi\n")
                    f.write("movl .riscaddl(%esi), %esi\n")
                    f.write("movl %%esi, .sumresult+%d(%%edi)\n" % i)

                    f.write("movl .sumaddr(%edi), %esi\n")
                    f.write("movl .riscaddh(%esi), %esi\n")
                    f.write("movl %esi, .sumcarry(%edi)\n")
                    f.write("movl .zero(%edi), %esi\n")
                    f.write("movl %esi, .sumcarry+1(%edi)\n")

                f.write("movl .sumresult(%edi), %esi\n")

            elif base:
                f.write("movl .%s(%%edi), %%esi\n" % base[1:])

            if ismem(source):
                #f.write("movl.d32 %s(%s), %s\n" % (offset, "%esi", dest))

                f.write("movl.d32 %s(%s), %s\n" % (offset, "%esi", "%esi"))
                f.write("movl %%esi, .%s(%%edi)\n" % dest[1:])

                if "<REQ>" in l:
                    f.write("movl .%s(%%edi), %s\n" % (dest[1:], dest))

            else:
                #f.write("movl.d32 %s, %s(%s)\n" % (source, offset, "%esi"))

                if "<REQ>" in l:
                    f.write("movl %s, .%s(%%edi)\n" % (source, source[1:]))

		# TODO: ideally, stick to just esi/edi ... but it's tough when
		#       edi is reserved for 0
                f.write("movl .%s(%%edi), %%eax\n" % (source[1:]))
                f.write("movl.d32 %s, %s(%s)\n" % ("%eax", offset, "%esi"))

        else:
            f.write(l)

