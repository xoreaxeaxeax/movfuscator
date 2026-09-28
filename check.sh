#!/bin/sh

# To verify that the M/o/Vfuscator is working correctly, this script will pull
# down an open source AES implementation, compile it, objdump it, and run it.
# The objdump will take some time, be patient.

MOVCC=${MOVCC:-./build/movcc}

echo

# Grab some example source
echo "Downloading..."
echo
sleep 2
[ ! -d "validation/aes" ] && git clone https://github.com/kokke/tiny-AES128-C validation/aes
cd validation/aes && git reset --hard 7e42e693288bdf22d8e677da94248115168211b9 && cd -
sleep 2
echo
echo

# M/o/Vfuscate the program
echo "M/o/Vfuscating..."
echo
sleep 2
"$MOVCC" validation/aes/aes.c validation/aes/test.c -o validation/aes/aes -s
sleep 2
echo
echo

# Check the assembly
echo "Dumping..."
echo
sleep 2
objdump -d -Mintel --insn-width=15 validation/aes/aes
sleep 2
echo
echo

# Check execution
echo "Running..."
echo
sleep 2
./validation/aes/aes
sleep 2
echo "M/o/Vfuscator check complete."
echo
