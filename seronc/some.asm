section .text

global _start
_start:

push rbp
mov rbp, rsp

mov rax, [rbp-4]


mov rsp, rbp
pop rbp
