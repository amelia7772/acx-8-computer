.org 0xfe00
start:
movi r0 1
movi r1 '\\'
iocall r0 r1 r2
restart:
movi r0 1
movi r1 '\n'
iocall r0 r1 r2
movi r1 '\r'
iocall r0 r1 r2

movi r0 0x00
movi r1 0x00

movi r2 0b10000001 ;keyboard
movi r4 0
get_address_from_user:
iocall r2 r3 r5
cmpi r3 0
beq get_address_from_user
movi r2 1 ; display
iocall r2 r3 r5
movi r2 0b10000001 ;keyboard
cmpi r3 '\b'
beq backspace_hit
cmpi r3 '\n'
beq finished_getting_address_from_user
cmpi r3 ':'
beq set_write_mode
cmpi r3 'R'
beq run_program_at_current_address
cmpi r4 4
beq start
cmpi r3 'F'
bgr start
cmpi r3 '0'
ble start
cmpi r3 'A'
ble is_address_letter_a_number
movi r5 55 ; 'A' - 10
sub r6 r3 r5
mov r6 r3
b move_on_to_read_next_letter
is_address_letter_a_number:
cmpi r3 '9'
bgr start
movi r5 '0'
sub r6 r3 r5
mov r6 r3
move_on_to_read_next_letter:
push r3
movi r6 1
add r5 r4 r6
mov r5 r4
b get_address_from_user

finished_getting_address_from_user:
iocall r2 r3 r5
movi r2 1 ; display
iocall r2 r3 r5

cmpz r0
beq convert_address_to_bytes
movi r4 0
pull r2
add r3 r2 r4
mov r3 r4
pull r2
shfl r3 r2
shfl r2 r3
shfl r3 r2
shfl r2 r3
add r3 r2 r4
mov r3 r4
movi r5 1
movi r0 0
convert_address_to_bytes:
pull r2
add r3 r2 r0
mov r3 r0
pull r2
shfl r3 r2
shfl r2 r3
shfl r3 r2
shfl r2 r3
add r3 r2 r0
mov r3 r0
pull r2
add r3 r2 r1
mov r3 r1
pull r2
shfl r3 r2
shfl r2 r3
shfl r3 r2
shfl r2 r3
add r3 r2 r1
mov r3 r1
cmpi r5 1
beq write_contents_to_ram_address
cmpi r4 1
beq run_contents_at_ram_address
read_ram_contents_from_address:
ldr r2 r0 r1
movi r3 0xf0
and r4 r2 r3
shfr r5 r4
shfr r4 r5
shfr r5 r4
shfr r4 r5
cmpi r4 9
bgr first_nip_is_letter
movi r5 '0'
add r6 r4 r5
movi r3 1
iocall r3 r6 r2
b second_nip
first_nip_is_letter:
movi r5 55 ; 'A' - 10
add r6 r4 r5
movi r3 1
iocall r3 r6 r2
second_nip:
movi r3 0x0f
and r4 r2 r3
cmpi r4 9
bgr second_nip_is_letter
movi r5 '0'
add r6 r4 r5
movi r3 1
iocall r3 r6 r2
b restart
second_nip_is_letter:
movi r5 55 ; 'A' - 10
add r6 r4 r5
movi r3 1
iocall r3 r6 r2
b restart

backspace_hit:
iocall r2 r3 r4
movi r2 1 ; display
iocall r2 r3 r4
movi r2 0b10000001 ;keyboard
iocall r2 r3 r4
movi r2 1 ; display
iocall r2 r3 r4
movi r2 0b10000001 ;keyboard
cmpz r5
beq get_address_from_user
pull r6
movi r6 1
sub r5 r4 r6
mov r5 r4
b get_address_from_user

set_write_mode:
cmpi r4 3
ble start
movi r4 1
movi r0 1
b get_address_from_user

write_contents_to_ram_address:
str r4 r0 r1
b restart

run_program_at_current_address:
iocall r2 r3 r5
movi r2 1 ; display
iocall r2 r3 r5
cmpi r4 3
ble start
movi r4 1
b convert_address_to_bytes

run_contents_at_ram_address:
b16 r0 r1
b start