section .text

global _start
_start:
push rbp
mov rbp, rsp

    mov eax, -20
    mov edi, 10
    add eax, edi

    mov dword [rbp-4], -20
    mov dword [rbp-8], 10
    add dword [rbp-4], 10

    mov rax, 60
    mov rdi, 0
    syscall

pop rbp
ret
