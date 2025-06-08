# The ACX-8 Computer

an 8-bit computer structure designed completely by me using solely transistors and logic gates (and pre-built RAM memory), designed in logisim, and emulated in C++.

In this repository I added 2 schematics options, one fully made by me where even the memory is designed from scratch by me, but it only has 256 bytes of memory and an 8-bit address bus, and another schematic for the finished version with a 16-bit address bus and 64 kilobytes of random access memory that is pre-built in logisim.

the reason for that is that it wouldn't require much archetictural innovation to use the 256 byte memory chips in a copy of their own schematic in place of the registers and have the data bus have both the byte of data and low byte of the address and connect it to the 256 byte memory chips while feeding the highest byte of the address to the 8-bit address bus; And while that is possible, it would just be a whole lot of wiring with no point, and also it would crash logisim and make the circuit file practically unusable, so I made the conscious decision to use pre-built memory that logisim can simulate far easier.

Also there is the emulator source code, written in C++ for windows (while I provided the source code for linux and mac, I have not tested it due to not having a device running linux or mac), and while binaries for windows are included for both 64 and 32 bit systems, I recommend compiling it locally because I have not yet tested it except on my windows 10 64-bit device.

the emulator takes 3 files (in the same folder) to work, they are the RAM file, the ROM file, and the STACK file, if the any of the files is not found the emulator will make it and fill it with binary 0s, however, if they are found yet they are of incompatible size, the emulator will exit with an error; The expected sizes of those files are:
RAM file: 65,536 bytes (exactly 64 kilobytes).
ROM file: 256 bytes.
STACK file: 256 bytes.

Note: the emulator is a terminal/command line program.

Note: the assembler will fill the rest of the ram/rom memory with 0s when compiling code for this archeticture.

for anyone interested in programming for this computer, the instruction set is included in this repository, and the assembler for the acx-8 archeticture's assembly language. However the linker is still in development, come back in a week or so when it's done.