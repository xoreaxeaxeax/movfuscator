# The rrrrrfuscator post-processor - translates M/o/Vfuscated C into only
# single-bit shift and rotate instructions.

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

with open(sys.argv[1]) as f:
    asm = f.readlines()

with open(sys.argv[1], 'w') as f:
    for l in asm:
        if l.startswith("mov"):
            f.write("#constant> " + l)

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


with open(sys.argv[1]) as f:
    asm = f.readlines()

asm = [l.replace('%ebx', '%esi') for l in asm]

with open(sys.argv[1], 'w') as f:
    f.write(".section .data\n")
    f.write("rrr8: .byte 0\n")
    f.write("rrr16: .word 0\n")
    f.write("rrr32: .long 0\n")

    for l in asm:
        if l.startswith("mov"):
            f.write("#rrr> " + l)

            tok = l.find(",", l.find(")"))
            if tok == -1:
                tok = l.find(",")
            source = l[l.index(" "):tok].strip()
            dest = l[tok+1:].strip()
            end = dest.find(" ")
            if end != -1:
                dest = dest[:end]


            if l.startswith("movb"):
                b = 8
                s = "b"
                reg = "%bl"
                t = "(rrr8)"
            elif l.startswith("movw"):
                b = 16
                s = "w"
                reg = "%bx"
                t = "(rrr16)"
            elif l.startswith("movl"):
                b = 32
                s = "l"
                reg = "%ebx"
                t = "(rrr32)"

            f.write("rcr%s %s\n" % (s, source))

            for i in range(0,b):
                f.write("rcr%s %s\n" % (s, reg))
                f.write("sar%s %s\n" % (s, reg))
                f.write("rcl%s %s\n" % (s, reg))
                f.write("rcr%s %s\n" % (s, t))
                f.write("rcl%s %s\n" % (s, reg))
                f.write("rcr%s %s\n" % (s, source))

            for i in range(0,b):
                f.write("rcr%s %s\n" % (s, t))
                f.write("rcr%s %s\n" % (s, dest))

        else:
            f.write(l)
