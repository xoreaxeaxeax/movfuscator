# The SBBfuscator post-processor - translates M/o/Vfuscated C into only
# sbb instructions.

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
    #NOTE: The SBBfuscator only works if there are less than 2^32 borrows in the
    # program.  If this is (somehow) a problem, you can add a second carry
    # bucket to consume the next ~2^64.
    f.write(".section .data\n")
    f.write(".carry_collector: .long 0xffffffff\n")
    f.write(".section .text\n")
    for l in asm:
        if l.startswith("mov"):
            f.write("#sbb> " + l)

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

            # ridiculous register shuffling necessary b/c ebx is 
            # our last byte addressable register
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbbl %ebx, %ebx\n")
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbbl %esi, %esi\n")
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbbl %edi, %edi\n")
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbb%s %s, %s\n" % (s, dest, reg))
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbbl %ebx, %esi\n")
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbbl %esi, %edi\n")
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbbl %edi, %ebx\n")
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbbl %edi, %ebx\n")
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbb%s %s, %s\n" % (s, source, reg))
            f.write("sbbl $0, (.carry_collector)\n")
            f.write("sbb%s %s, %s\n" % (s, reg, dest))

        else:
            f.write(l)

