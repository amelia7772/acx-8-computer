this kernel is based off of wozmon (apple 1's kernel)

it occupies addresses (hex) FE00 through FFFF (last 512 bytes of ram)

when you run the kernel you get a backslash ('\') and a new line, indicating that the program is running successfully

if the program encounters an error it will go back to its start and thus type the same backslash ('\') and new line again, restarting the whole program

this program has 3 operations:
1) read: the user types 4 hexadecimal digits indicating an address in ram to read from, then once a new line is entered, the program will read the content of ram memory at that address and print it as 2 hexadecimal digits, then print a new line character.
2) write: the user types 4 hexadecimal digits indicating an address in ram to write to, then the user inputs a colon character (':') to indicate the write operation, then (warning: no new line character should be entered after it immediately) the user types 2 hexadecimal digits indicating the value of the byte that the user wants to write to the aforementioned address, then after the user types a new line, the program will write that byte to memory and then type another new line character, indicating a successful writing operation.
3) run:  the user types 4 hexadecimal digits indicating an address of the start of a program in ram, and then types R (without a new line character) and the program at that address will start running.