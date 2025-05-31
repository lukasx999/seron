extern glfwInit
extern putchar

section .text

global main
main:

push rbp
mov rbp, rsp

sub rsp, 64

; mov edi, 65
call glfwInit


mov eax, 0
mov rsp, rbp
pop rbp
ret
