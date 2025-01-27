section .text


foo:
push rbp
mov rbp, rsp
    ; ...
pop rbp
ret


global _start
_start:
push rbp
mov rbp, rsp

    mov dword [rbp-4], foo
    call [rbp-4]




pop rbp
ret
