# The ANDORfuscator post-processor - translates M/o/Vfuscated C into only
# and/or instructions.

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

with open(sys.argv[1]) as f:
    asm = f.readlines()

asm = [l.replace('ebx', 'esi') for l in asm]

with open(sys.argv[1], 'w') as f:
    for l in asm:
        if l.startswith("mov"):
            f.write("#andor> " + l)

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
                reg = "%bl"
            elif l.startswith("movw"):
                s = "w"
                reg = "%bx"
            elif l.startswith("movl"):
                s = "l"
                reg = "%ebx"

            f.write("and%s %s, %s\n" % (s, "$0", reg))
            f.write("or%s  %s, %s\n" % (s, source, reg))
            f.write("and%s %s, %s\n" % (s, "$0", dest))
            f.write("or%s  %s, %s\n" % (s, reg, dest))

        else:
            f.write(l)

