section .text


global _start
_start:

push rbp
mov rbp, rsp

mov dword [rbp-4], 45
lea rax, [rbp-4]
mov rdi, [rax]

mov rsp, rbp
pop rbp
