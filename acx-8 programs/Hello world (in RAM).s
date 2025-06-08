.org 0xff00
.string text: "Hello, world!"
.byte end_of_text: 0
.org 0x0000

movi r1 [$text:0]
movi r2 [$text:1]
movi r3 1
loop:
ldr r0 r1 r2
iocall r3 r0 r2
add r4 r3 r1
mov r4 r1

cmpz r0
bne loop

halt:
b halt