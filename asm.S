.text
.intel_syntax noprefix
.globl memset_loop
.globl memset_fsrm
.globl memset_avx2
.globl memset_avx2_nt
.globl memset_avx2_nt_st
.globl memset_clzero


; RDI = addr
; RSI = value
; RDX = length

.align 4096
memset_loop:
._loop:
        movq [rdi], rsi
        addq rdi, 8
        subq rdx, 8
        jnz ._loop
        ret

.align 4096
memset_fsrm:
        mov rax, rsi
        mov rcx, rdx
        rep stosb
        ret

.align 4096
memset_avx2:
        vpxor xmm0, xmm0, xmm0
        vpinsrb xmm0, xmm0, rsi, 0
        vpbroadcastb ymm0, xmm0
._avx2:
        vmovdqu ymmword ptr [rdi], ymm0
        addq rdi, 32
        subq rdx, 32
        jnz ._avx2
        ret

.align 4096
memset_avx2_nt:
        vpxor xmm0, xmm0, xmm0
        vpinsrb xmm0, xmm0, rsi, 0
        vpbroadcastb ymm0, xmm0
._avx2_nt:
        vmovntdq ymmword ptr [rdi], ymm0
        addq rdi, 32
        subq rdx, 32
        jnz ._avx2_nt
        ret

.align 4096
memset_avx2_nt_st:
        vpxor xmm0, xmm0, xmm0
        vpinsrb xmm0, xmm0, rsi, 0
        vpbroadcastb ymm0, xmm0
        xorq r8, r8
._avx2_nt_st:
        vmovntdq ymmword ptr [rdi], ymm0
        vmovntdq ymmword ptr [rdi + 0x100000], ymm0
        addq r8, 32
        cmp r8, 0x100000
        jne ._avx2_nt_st_skip
        addq rdi, 0x100000
        xorq r8, r8
._avx2_nt_st_skip:
        addq rdi, 32
        subq rdx, 64
        jnz ._avx2_nt_st
        ret

.align 4096
memset_clzero:
        movq rax, rdi
._clzero:
        clzero
        addq rax, 64
        subq rdx, 64
        jnz ._clzero
        ret

.section .note.GNU-stack, "", @progbits
