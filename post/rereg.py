# The re-reg post-processor - reallocates registers to diversify programs and
# prevent simple decompilation.

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
import re

r0 = 0
r1 = 1
r2 = 2
r3 = 3
n = 4

# assumes compiler only emits eax, ebx, ecx, edx
free_regs = ['A', 'B', 'C', 'D']

alloc = dict()

def getfree():
    random.shuffle(free_regs)
    return free_regs.pop()

def free(r):
    if not r in free_regs:
        free_regs.append(r)

def getnext():
    global n
    nr = n
    n = n + 1
    return nr

def genreg(l):
    global r0, r1, r2, r3

    if l.startswith("movb"):
        s = "b"
    elif l.startswith("movw"):
        s = "w"
    elif l.startswith("movl"):
        s = "l"

    tok = l.find(",", l.find(")"))
    if tok == -1:
        tok = l.find(",")
    source = l[l.index(" "):tok].strip()
    dest = l[tok+1:].strip()
    end = dest.find(" ")
    if end != -1:
        dest = dest[:end]

    if "TR0" in source:
        source = source.replace("TR0", "r%d" % r0)
    if "TR1" in source:
        source = source.replace("TR1", "r%d" % r1)
    if "TR2" in source:
        source = source.replace("TR2", "r%d" % r2)
    if "TR3" in source:
        source = source.replace("TR3", "r%d" % r3)

    if s == "l":
        if dest.startswith("%TR0"):
            r0 = getnext()
            dest = dest.replace("TR0", "r%d" % r0)
        if dest.startswith("%TR1"):
            r1 = getnext()
            dest = dest.replace("TR1", "r%d" % r1)
        if dest.startswith("%TR2"):
            r2 = getnext()
            dest = dest.replace("TR2", "r%d" % r2)
        if dest.startswith("%TR3"):
            r3 = getnext()
            dest = dest.replace("TR3", "r%d" % r3)
    else:
        if dest.startswith("%TR0"):
            dest = dest.replace("TR0", "r%d" % r0)
        if dest.startswith("%TR1"):
            dest = dest.replace("TR1", "r%d" % r1)
        if dest.startswith("%TR2"):
            dest = dest.replace("TR2", "r%d" % r2)
        if dest.startswith("%TR3"):
            dest = dest.replace("TR3", "r%d" % r3)

    if "TR0" in dest:
        dest = dest.replace("TR0", "r%d" % r0)
    if "TR1" in dest:
        dest = dest.replace("TR1", "r%d" % r1)
    if "TR2" in dest:
        dest = dest.replace("TR2", "r%d" % r2)
    if "TR3" in dest:
        dest = dest.replace("TR3", "r%d" % r3)

    return "mov%s %s, %s" % (s, source, dest)

def is_used(r, asm, i):
    for k in range(i+1, len(asm)):
        if r in asm[k]:
            return True
    return False

def rereg(l, asm, i):
    global alloc

    if l.startswith("movb"):
        s = "b"
    elif l.startswith("movw"):
        s = "w"
    elif l.startswith("movl"):
        s = "l"

    tok = l.find(",", l.find(")"))
    if tok == -1:
        tok = l.find(",")
    source = l[l.index(" "):tok].strip()
    dest = l[tok+1:].strip()
    end = dest.find(" ")
    if end != -1:
        dest = dest[:end]

    source_regs = re.findall(r'r\d+_', source)
    for r in source_regs:
        if not r in alloc:
            alloc[r] = getfree()
        source = source.replace(r, alloc[r] + "_")

    for r in source_regs:
        if not r in dest:
            if not is_used(r, asm, i+1):
                free(alloc[r])

    dest_regs = re.findall(r'r\d+_', dest)
    for r in dest_regs:
        if not r in alloc:
            alloc[r] = getfree()
        dest = dest.replace(r, alloc[r] + "_")

    for r in dest_regs:
        if not is_used(r, asm, i+1):
            free(alloc[r])

    return "mov%s %s, %s" % (s, source, dest)

with open(sys.argv[1]) as f:
    asm = f.readlines()

for i, l in enumerate(asm):
    if l.startswith("mov"):
        l = l.replace("%eax", "%TR0_l")
        l = l.replace("%ax", "%TR0_w")
        l = l.replace("%al", "%TR0_b")
        l = l.replace("%ah", "%TR0_h")

        l = l.replace("%ebx", "%TR1_l")
        l = l.replace("%bx", "%TR1_w")
        l = l.replace("%bl", "%TR1_b")
        l = l.replace("%bh", "%TR1_h")

        l = l.replace("%ecx", "%TR2_l")
        l = l.replace("%cx", "%TR2_w")
        l = l.replace("%cl", "%TR2_b")
        l = l.replace("%ch", "%TR2_h")

        l = l.replace("%edx", "%TR3_l")
        l = l.replace("%dx", "%TR3_w")
        l = l.replace("%dl", "%TR3_b")
        l = l.replace("%dh", "%TR3_h")

        asm[i]=l

for i, l in enumerate(asm):
    if l.startswith("mov"):
        asm[i]=genreg(l)

for i, l in enumerate(asm):
    if l.startswith("mov"):
        asm[i]=rereg(l, asm, i)

for i, l in enumerate(asm):
    if l.startswith("mov"):
        l = l.replace("A_l", "eax")
        l = l.replace("A_w", "ax")
        l = l.replace("A_b", "al")
        l = l.replace("A_h", "ah")

        l = l.replace("B_l", "ebx")
        l = l.replace("B_w", "bx")
        l = l.replace("B_b", "bl")
        l = l.replace("B_h", "bh")

        l = l.replace("C_l", "ecx")
        l = l.replace("C_w", "cx")
        l = l.replace("C_b", "cl")
        l = l.replace("C_h", "ch")

        l = l.replace("D_l", "edx")
        l = l.replace("D_w", "dx")
        l = l.replace("D_b", "dl")
        l = l.replace("D_h", "dh")

        asm[i] = l

for l in asm:
    print l.strip()
