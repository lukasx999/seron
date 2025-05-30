extern putchar
section .text


global main
main:

push rbp
mov rbp, rsp


sub rsp, 32
mov rax, 65
mov [rbp-40], rax ; array
mov rax, 66
mov [rbp-32], rax ; array
mov rax, 67
mov [rbp-24], rax ; array
lea rax, [rbp-40]
mov [rbp-8], rax ; xs
mov rax, putchar
push rax
mov rax, [rbp-8]
mov rax, [rax]
mov dil, al
pop rax
call rax


mov rsp, rbp
pop rbp

mov eax, 0
ret
