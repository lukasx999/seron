section .text

global _start
_start:
push rbp
mov rbp, rsp



    mov rax, 0

    jmp .cond
    .while:

    ; write()
    mov byte [rbp-4], 65
    mov rax, 1
    mov rdi, 1
    lea rsi, [rbp-4]
    mov rdx, 1
    syscall

    .cond:
    cmp rax, 0
    jne .while






    ; exit(0)
    mov rax, 60
    mov rdi, 0
    syscall

pop rbp
ret
