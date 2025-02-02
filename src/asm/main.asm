section .text

global _start
_start:
push rbp
mov rbp, rsp





    ; exit(0)
    mov rax, 60
    mov rdi, 0
    syscall

pop rbp
ret
