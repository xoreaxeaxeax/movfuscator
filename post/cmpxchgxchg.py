# The CMPXCHGXCHGfuscator post-processor - translates M/o/Vfuscated C into only
# cmpxchg/xchg instructions.

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

# pass 1 - cmpxchg/xchg can't load constants 
# replace all constant references with memory references instead
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

# we need to free up eax, but have no other byte-addressable registers.
# free up ebx first.

# TODO: the register shuffling will break return values from library calls.
asm = [l.replace('%ebx', '%esi') for l in asm]
asm = [l.replace('%eax', '%ebx') for l in asm]
asm = [l.replace('%ax', '%bx') for l in asm]
asm = [l.replace('%al', '%bl') for l in asm]
asm = [l.replace('%ah', '%bh') for l in asm]

with open(sys.argv[1], 'w') as f:
    for l in asm:
        if l.startswith("mov"):
            f.write("#cmpxchgxchg> " + l)

            tok = l.find(",", l.find(")"))
            if tok == -1:
                tok = l.find(",")
            source = l[l.index(" "):tok].strip()
            dest = l[tok+1:].strip()

            if l.startswith("movb"):
                s = "b"
                reg = "%al"
            elif l.startswith("movw"):
                s = "w"
                reg = "%ax"
            elif l.startswith("movl"):
                s = "l"
                reg = "%eax"

            if source.find("(")>=0:
                # source is a memory reference
                f.write("cmpxchg%s %s, %s\n" % (s, reg, source))
                f.write("xchg%s %s, %s\n" % (s, dest, reg))
            else:
                # source is a register
                f.write("cmpxchg%s %s, %s\n" % (s, source, dest))
                f.write("cmpxchg%s %s, %s\n" % (s, source, dest))
        else:
            f.write(l)
