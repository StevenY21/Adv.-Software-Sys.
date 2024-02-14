        .intel_syntax noprefix  
        .section .data

x:      .quad 100  
y:      .quad 200

        .section .text  
        .global _start

_start:  
        mov rcx, QWORD PTR [x]  
        mov rbx, QWORD PTR [y]  
        add rcx, rbx  
        mov rax, 60  
        mov rdi, 0  
        syscall  