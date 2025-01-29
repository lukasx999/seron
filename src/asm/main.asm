section .text

global _start
_start:
push rbp
mov rbp, rsp


    mov dword [rbp-4], 45
    mov rax, [rbp-4]


    mov rax, 60
    mov rdi, 0
    syscall

pop rbp
ret
