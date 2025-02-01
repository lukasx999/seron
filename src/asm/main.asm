section .text

global _start
_start:
push rbp
mov rbp, rsp



    mov rax, 1

    cmp rax, 1
    jne .else

    ; write()
    mov byte [rbp-4], 65
    mov rax, 1
    mov rdi, 1
    lea rsi, [rbp-4]
    mov rdx, 1
    syscall

    jmp .end
    .else:

    ; write()
    mov byte [rbp-4], 66
    mov rax, 1
    mov rdi, 1
    lea rsi, [rbp-4]
    mov rdx, 1
    syscall

    .end:


    ; exit(0)
    mov rax, 60
    mov rdi, 0
    syscall

pop rbp
ret
