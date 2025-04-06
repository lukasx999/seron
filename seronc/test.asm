section .text

global _start
_start:

push rbp
mov rbp, rsp

mov rax, 10
mov rdi, 2
imul rax, rdi

mov rsp, rbp
pop rbp


mov rax, 60
mov rdi, 0
syscall
