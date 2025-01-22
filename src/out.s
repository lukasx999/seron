section .text

; function(start)
; function_prelude(start)
global print_bytes
print_bytes:
push rbp
mov rbp, rsp
; function_prelude(end)

; inline(start)

    mov rdx, rsi ; size
    mov rsi, rdi ; buf
    mov rax, 1
    mov rdi, 1
    syscall
    
; inline(end)

; function_postlude(start)
pop rbp
ret
; function_postlude(end)
; function(end)

; function(start)
; function_prelude(start)
global sys_exit
sys_exit:
push rbp
mov rbp, rsp
; function_prelude(end)

; inline(start)

    mov rax, 60
    mov rdi, 0
    syscall
    
; inline(end)

; function_postlude(start)
pop rbp
ret
; function_postlude(end)
; function(end)

; function(start)
; function_prelude(start)
global _start
_start:
push rbp
mov rbp, rsp
; function_prelude(end)

; store(start)
sub rsp, 4
mov dword [rbp-4], 10
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-8], 33
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-12], 100
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-16], 108
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-20], 114
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-24], 111
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-28], 87
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-32], 32
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-36], 44
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-40], 111
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-44], 108
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-48], 108
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-52], 101
; store(end)

; store(start)
sub rsp, 4
mov dword [rbp-56], 72
; store(end)

; inline(start)

    mov rdi, rsp
    mov rsi, 56
    call print_bytes
    
; inline(end)

; inline(start)

    call sys_exit
    
; inline(end)

; function_postlude(start)
pop rbp
ret
; function_postlude(end)
; function(end)

