# The randomfuscator post-processor - translates M/o/Vfuscated C into some
# random combination of the other post-processors.

# TODO: This needs to be updated for code reuse

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
import random
import hashlib

def o_adc(f, l):
    f.write("#adc> " + l)

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
        f.write("adcl $0, (.carry_collector)\n")
        f.write("adc%s %s, %s\n" % (s, reg, reg))

    for i in xrange(0,b):
        f.write("adcl $0, (.carry_collector)\n")
        f.write("adcl %esi, %esi\n")

    f.write("adcl $0, (.carry_collector)\n")
    f.write("adc%s %s, %s\n" % (s, source, reg))
    f.write("adcl $0, (.carry_collector)\n")
    f.write("adcl %ebx, %esi\n")

    for i in xrange(0,b):
        f.write("adcl $0, (.carry_collector)\n")
        f.write("adc%s %s, %s\n" % (s, reg, reg))

    f.write("adcl $0, (.carry_collector)\n")
    f.write("adc%s %s, %s\n" % (s, dest, reg))

    for i in xrange(0,b):
        f.write("adcl $0, (.carry_collector)\n")
        f.write("adc%s %s, %s\n" % (s, reg, dest))
        f.write("adcl $0, (.carry_collector)\n")
        f.write("adc%s %s, %s\n" % (s, reg, reg))

    f.write("adcl $0, (.carry_collector)\n")
    f.write("adcl %esi, %ebx\n")
    f.write("adcl $0, (.carry_collector)\n")
    f.write("adc%s %s, %s\n" % (s, reg, dest))

def o_add(f, l):
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

def o_andor(f, l):
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

def o_rrrrr(f, l):
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

def o_sbb(f, l):
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
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbbl %ebx, %ebx\n")
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbbl %esi, %esi\n")
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbbl %edi, %edi\n")
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbb%s %s, %s\n" % (s, dest, reg))
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbbl %ebx, %esi\n")
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbbl %esi, %edi\n")
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbbl %edi, %ebx\n")
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbbl %edi, %ebx\n")
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbb%s %s, %s\n" % (s, source, reg))
    f.write("sbbl $0, (.borrow_collector)\n")
    f.write("sbb%s %s, %s\n" % (s, reg, dest))

def o_sub(f, l):
    f.write("#sub> " + l)

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
    f.write("subl %ebx, %ebx\n")
    f.write("subl %esi, %esi\n")
    f.write("subl %edi, %edi\n")
    f.write("sub%s %s, %s\n" % (s, dest, reg))
    f.write("subl %ebx, %esi\n")
    f.write("subl %esi, %edi\n")
    f.write("subl %edi, %ebx\n")
    f.write("subl %edi, %ebx\n")
    f.write("sub%s %s, %s\n" % (s, source, reg))
    f.write("sub%s %s, %s\n" % (s, reg, dest))

def o_xadd(f, l):
    f.write("#xadd> " + l)

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
        f.write("xadd%s %s, %s\n" % (s, reg, reg))

    for i in xrange(0,b):
        f.write("xaddl %esi, %esi\n")

    f.write("xadd%s %s, %s\n" % (s, reg, source))
    f.write("xaddl %ebx, %esi\n")

    f.write("xadd%s %s, %s\n" % (s, reg, dest))

    for i in xrange(0,b):
        f.write("xadd%s %s, %s\n" % (s, reg, dest))
        f.write("xadd%s %s, %s\n" % (s, reg, reg))

    f.write("xaddl %ebx, %esi\n")
    f.write("xadd%s %s, %s\n" % (s, reg, dest))

o = [o_adc, o_add, o_andor, o_rrrrr, o_sub, o_sbb, o_xadd]

# pass 1: remove immediate references from code
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

                # hacky
                # some of the obfuscators rely on edi
                # prevent these lines from being translated by adding a space to
                # the beginning
                f.write(" movl (.C%s), %%edi\n" % c)
                f.write(" movl %%edi, %s\n" % dest)
            else:
                f.write(l)

        else:
            f.write(l)

# pass 2: randomly obfuscate
with open(sys.argv[1]) as f:
    asm = f.readlines()

asm = [l.replace('ebx', 'ebp') for l in asm]

with open(sys.argv[1], 'w') as f:
    f.write(".section .data\n")
    f.write(".carry_collector: .long 0\n")
    f.write(".borrow_collector: .long 0xffffffff\n")
    f.write(".section .text\n")

    f.write(".section .data\n")
    f.write("rrr8: .byte 0\n")
    f.write("rrr16: .word 0\n")
    f.write("rrr32: .long 0\n")
    f.write(".section .text\n")

    for l in asm:
        if l.startswith("mov"):
            random.choice(o)(f,l)
        else:
            f.write(l)

