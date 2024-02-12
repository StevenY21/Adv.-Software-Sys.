.section .data
    hello_msg:
        .ascii "Hello, World!\n"    # Message to be printed

.section .text
    .globl _start

_start:
    # Write the message to stdout
    movl $4, %eax                   # syscall number for sys_write
    movl $1, %ebx                   # file descriptor 1 (stdout)
    movl $hello_msg, %ecx           # pointer to the message
    movl $14, %edx                  # message length
    int $0x80                       # Call kernel

    # Exit the program
    movl $1, %eax                   # syscall number for sys_exit
    xorl %ebx, %ebx                 # Exit code 0
    int $0x80                       # Call kernel