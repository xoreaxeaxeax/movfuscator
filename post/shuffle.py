# The shuffle post-processor - shuffles the instructions output from the
# M/o/Vfuscator to prevent simple decompilation.

# This shuffler takes a naive approach to instruction shuffling.  It is
# generally overly restrictive (especially w.r.t. memory access ordering), which
# yields less than ideal results.  It is better viewed as a proof of concept or
# useful starting point, rather than actual protection from decompilation.

# This is a post-processing routine that can be run on assembly output from the
# M/o/Vfuscator.

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
import random

SWAP_PASSES = 10

def next_mov(asm, i):
    for k in range(i+1, len(asm)):
        if asm[k].startswith("mov"):
            return (k, asm[k])
    return (-1,"")

# assumes only eax, ebx, ecx, edx are emitted by compiler
def gen_reg(l):
    l=l.replace("eax", "r0")
    l=l.replace("ah", "r0")
    l=l.replace("al", "r0")
    l=l.replace("ax", "r0")

    l=l.replace("ebx", "r1")
    l=l.replace("bh", "r1")
    l=l.replace("bl", "r1")
    l=l.replace("bx", "r1")

    l=l.replace("ecx", "r2")
    l=l.replace("ch", "r2")
    l=l.replace("cl", "r2")
    l=l.replace("cx", "r2")

    l=l.replace("edx", "r3")
    l=l.replace("dh", "r3")
    l=l.replace("dl", "r3")
    l=l.replace("dx", "r3")

    return l

def can_swap(l1, l2):
    l1 = gen_reg(l1)
    l2 = gen_reg(l2)

    if l1.startswith("movb"):
        s1 = "b"
    elif l1.startswith("movw"):
        s1 = "w"
    elif l1.startswith("movl"):
        s1 = "l"

    if l2.startswith("movb"):
        s2 = "b"
    elif l2.startswith("movw"):
        s2 = "w"
    elif l2.startswith("movl"):
        s2 = "l"

    tok1 = l1.find(",", l1.find(")"))
    if tok1 == -1:
        tok1 = l1.find(",")
    source1 = l1[l1.index(" "):tok1].strip()
    dest1 = l1[tok1+1:].strip()
    end = dest1.find(" ")
    if end != -1:
        dest1 = dest1[:end]

    tok2 = l2.find(",", l2.find(")"))
    if tok2 == -1:
        tok2 = l2.find(",")
    source2 = l2[l2.index(" "):tok2].strip()
    dest2 = l2[tok2+1:].strip()
    end = dest2.find(" ")
    if end != -1:
        dest2 = dest2[:end]

    # assumes compiler outputs () around all memory references

    if "(" in dest1 and "(" in source2:
        if "%" in dest1 or "%" in source2:
            return False
        if s1 != s2:
            return False
    if dest1 in source2:
        return False
    if dest1 in dest2:
        return False

    if "(" in dest2 and "(" in source1:
        if "%" in dest2 or "%" in source1:
            return False
        if s1 != s2:
            return False
    if dest2 in source1:
        return False
    if dest2 in dest1:
        return False

    return True

with open(sys.argv[1]) as f:
    asm = f.readlines()

with open(sys.argv[1], 'w') as f:
    swaps = 0

    for k in xrange(SWAP_PASSES):
        for i1, l1 in enumerate(asm):
            sys.stdout.write("shuffling ... %4.2f%% (%d swaps)\r" % \
                    ((k*len(asm)+i1) * 100.0 / (SWAP_PASSES * len(asm)), swaps))

            if l1.startswith("mov"):
                i2, l2 = next_mov(asm, i1)

                if l2 != "":
                    if can_swap(l1, l2):
                        if random.randint(0,1) == 0:
                            asm[i1]=l2
                            asm[i2]=l1
                            swaps = swaps + 1
                            #sys.stdout.write("swap %d: %s <-> %s" % (swaps, l1.strip(), l2.strip()))

    f.write("# (%d swaps)\n" % swaps)

    for l in asm:
        f.write(l)
