# The ADDfuscator post-processor - translates M/o/Vfuscated C into only
# add instructions.

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

asm = [l.replace('ebx', 'ebp') for l in asm]

with open(sys.argv[1], 'w') as f:
    for l in asm:
        if l.startswith("mov"):
            f.write("#add> " + l)

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
            elif l.startswith("movw"):
                b = 16
                s = "w"
                reg = "%bx"
            elif l.startswith("movl"):
                b = 32
                s = "l"
                reg = "%ebx"

            # ridiculous register shuffling necessary b/c ebx is 
            # our last byte addressable register

            for i in xrange(0,b):
                f.write("add%s %s, %s\n" % (s, reg, reg))

            for i in xrange(0,b):
                f.write("addl %esi, %esi\n")

            f.write("add%s %s, %s\n" % (s, source, reg))
            f.write("addl %ebx, %esi\n")

            for i in xrange(0,b):
                f.write("add%s %s, %s\n" % (s, reg, reg))

            f.write("add%s %s, %s\n" % (s, dest, reg))

            for i in xrange(0,b):
                f.write("add%s %s, %s\n" % (s, reg, dest))
                f.write("add%s %s, %s\n" % (s, reg, reg))

            f.write("addl %esi, %ebx\n")
            f.write("add%s %s, %s\n" % (s, reg, dest))

        else:
            f.write(l)

