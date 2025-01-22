    extern puts

    section .text

    global _start
_start:
    push rbp
    mov rbp, rsp

    mov rdi, msg
    call puts

    ; 1+2
    mov dword [rbp], 1
    mov dword [rbp-4], 2

    mov rax, [rbp]
    add qword [rbp-4], rax

    mov rax, [rbp-4]
    mov qword [rbp-8], rax



    ; mov byte [rbp-4], 65
    ; mov byte [rbp-3], 10

    ; mov rax, 1
    ; mov rdi, 1
    ; lea rsi, [rbp-4]
    ; mov rdx, 2
    ; syscall

    pop rbp

    mov rax, 60
    mov rdi, 45
    syscall

    section .data
    msg: db "Greetings", 0
