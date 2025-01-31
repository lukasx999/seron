section .text

global _start
_start:
push rbp
mov rbp, rsp

    mov rax, 15
    mov rdi, 3
    div rdi



    ; exit(0)
    mov rax, 60
    mov rdi, 0
    syscall

pop rbp
ret
