# The mov32 post-processor - translates M/o/Vfuscated C into only 32 bit mem2reg
# or reg2mem MOV instructions.  This is primarily useful as an intermediate step
# towards translating code into instructions that have restrictive memory access
# sizes.

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
                f.write("movl (.C%s), %%edi\n" % c)
                f.write("movl %%edi, %s\n" % dest)
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

