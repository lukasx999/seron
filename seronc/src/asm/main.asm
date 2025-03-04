
section .data

section .text

foo:
push rbp
mov rbp, rsp

    mov eax, 45

pop rbp
ret


global _start
_start:
push rbp
mov rbp, rsp


    call foo
    sub rsp, 4
    mov [rbp-4], eax



    ; exit(0)
    mov rax, 60
    mov rdi, 0
    syscall

pop rbp
ret

