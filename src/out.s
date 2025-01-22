section .text

; function(start)
; function_prelude(start)
global _start
_start:
push rbp
mov rbp, rsp
; function_prelude(end)

mov dword [rbp-0], 1
mov dword [rbp-4], 2
; addition(start)
mov rax, [rbp-4]
add qword [rbp-0], rax
mov rax, [rbp-0]
mov qword [rbp-8], rax
; addition(end)

; inline(start)
xor rax, rax
; inline(end)


; function_postlude(start)
pop rbp
ret
; function_postlude(end)
; function(end)

