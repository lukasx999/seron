    section .text

    global _start
_start:
    push rbp
    mov rbp, rsp

    mov byte [rbp-4], 65
    mov byte [rbp-3], 10

    mov rax, 1
    mov rdi, 1
    lea rsi, [rbp-4]
    mov rdx, 2
    syscall

    pop rbp

    mov rax, 60
    mov rdi, 45
    syscall
