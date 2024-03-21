.text
.intel_syntax noprefix
.globl memset_fsrm
.globl memset_loop

; RDI = addr
; RSI = value
; RDX = length

memset_fsrm:
        mov rax, rsi
        mov rcx, rdx
        rep stosb
        ret

memset_avx2:
        vpbroadcastb ymm0, rsi
        ; vmovdqu ymm1, ymmword ptr [rdx]
        ret

memset_loop:
.loop_start:
        movq [rdi], rsi
        addq rdi, 8
        subq rdx, 8
        jnz .loop_start
        ret

.section .note.GNU-stack, "", @progbits
