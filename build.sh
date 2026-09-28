#!/bin/sh

set -v

# grab the frontend
[ ! -d "lcc" ] && git clone https://github.com/drh/lcc
cd lcc && git reset --hard 3b3f01b4103cd7b519ae84bd1122c9b03233e687 && cd -

# create a build directory
export BUILDDIR=`pwd`/build
mkdir -p "$BUILDDIR"

# configure the build includes
mkdir -p "$BUILDDIR/include"
cp -p -R lcc/include/x86/linux/* "$BUILDDIR/include"

# Link to gcc's library directory
GCCLN=$(gcc --print-search-dirs | grep install | head -1 | cut -d " " -f 2-)
ln -sfn "$GCCLN" "$BUILDDIR/gcc"

# Bind to the backend
patch -N -r - lcc/src/bind.c movfuscator/bind.patch

# Add the new backend to the frontend
patch -N -r - lcc/makefile movfuscator/makefile.patch

# Make LCC more lenient with pointer and const violations that pervade everything
patch -N -r - lcc/src/enode.c movfuscator/enode.patch

# Fix bug in LCC register allocation
patch -N -r - lcc/src/gen.c movfuscator/gen.patch

# Fix bug in LCC unary - promotion
patch -N -r - lcc/src/expr.c movfuscator/expr.patch

# Rename LCC constexpr helper to avoid modern GCC keyword conflicts
# (I am too dumb to coax the patch command to do this, so I use perl instead)
perl -0pi -e 's/extern Tree constexpr\(int\);/extern Tree constexp(int);/g; s/Tree constexpr\(int tok\) \{/Tree constexp(int tok) {/g; s/Tree p = constexpr\(tok\);/Tree p = constexp(tok);/g; s/p = constexpr\(0\);/p = constexp(0);/g' lcc/src/c.h lcc/src/simp.c lcc/src/stmt.c

# Add additional flag passing for lcc linker
patch -N -r - lcc/etc/lcc.c movfuscator/lcc.patch

# Build the compiler driver
make -C lcc HOSTFILE=../movfuscator/host.c CFLAGS='-g -DLCCDIR=\"$(BUILDDIR)/\" -Wno-implicit-int -Wno-implicit-function-declaration' lcc

# Build lcc with the M/o/Vfuscator backend
make -C lcc CFLAGS='-Wno-implicit-int -Wno-implicit-function-declaration' all

# Create movcc
ln -sfn "$BUILDDIR/lcc" "$BUILDDIR/movcc"

# Build the M/o/Vfuscator crt libraries
"$BUILDDIR/movcc" movfuscator/crt0.c -o "$BUILDDIR/crt0.o" -c -Wf--crt0 -Wf--q
"$BUILDDIR/movcc" movfuscator/crtf.c -o "$BUILDDIR/crtf.o" -c -Wf--crtf -Wf--q
"$BUILDDIR/movcc" movfuscator/crtd.c -o "$BUILDDIR/crtd.o" -c -Wf--crtd -Wf--q

# Build the M/o/Vfuscator crt libraries with unobfuscated control flow
"$BUILDDIR/movcc" movfuscator/crt0.c -o "$BUILDDIR/crt0_cf.o" -c -Wf--crt0 -Wf--q -Wf--no-mov-flow
"$BUILDDIR/movcc" movfuscator/crtf.c -o "$BUILDDIR/crtf_cf.o" -c -Wf--crtf -Wf--q -Wf--no-mov-flow
"$BUILDDIR/movcc" movfuscator/crtd.c -o "$BUILDDIR/crtd_cf.o" -c -Wf--crtd -Wf--q -Wf--no-mov-flow

# Build the M/o/Vfuscator soft float library
# These may give warnings about overflows, they are (mostly) safe to ignore
make -C softfloat clean && make -C softfloat CC="$BUILDDIR/movcc"
mkdir -p movfuscator/lib
cp softfloat/softfloat32.o movfuscator/lib/softfloat32.o
cp softfloat/softfloat64.o movfuscator/lib/softfloat64.o
cp softfloat/softfloatfull.o movfuscator/lib/softfloatfull.o

# Build the M/o/Vfuscator soft float library with unobfuscated control flow
# These may give warnings about overflows, they are (mostly) safe to ignore
make -C softfloat clean && make -C softfloat CC="$BUILDDIR/movcc -Wf--no-mov-flow"
mkdir -p movfuscator/lib
cp softfloat/softfloat32.o movfuscator/lib/softfloat32_cf.o
cp softfloat/softfloat64.o movfuscator/lib/softfloat64_cf.o
cp softfloat/softfloatfull.o movfuscator/lib/softfloatfull_cf.o

make -C softfloat clean
