start:
movi r0 0b00000010
movi r1 0x00
movi r2 0xfe
iocall r0 r1 r2
b start