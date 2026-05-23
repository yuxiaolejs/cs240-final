.global enable_vfp
enable_vfp:
    push    {r4, r5, r6}
    mov     r6, #0
    // Grant PL1 (and PL0) full access to CP10 and CP11.
    mrc     p15, 0, r4, c1, c0, 2
    orr     r4, r4, #(0xf<<20)
    mcr     p15, 0, r4, c1, c0, 2
    mcr     p15, 0, r6, c7, c5, 4 @ isb
    mov     r4, #(1<<30)
    mcr     p10, 7, r4, c8, c0, 0 @ Enable VFP

    pop     {r4, r5, r6}
    bx      lr

.global irq_timer_interrupt
irq_timer_interrupt:
    sub     lr, lr, #4
    ldr     sp, =0x9100000

    stmfd   sp!, {r0-r12, lr}
    mrs     r3, spsr
    stmfd   sp!, {r3}
    sub     sp, sp, #8
    mov     r0, sp
    stmia   r0, {sp, lr}^
    nop
    mov     r8, sp
    mov     r0, sp

    bl      irq_handler

.global irq_generic_return
irq_generic_return:
    mov     sp, r8
    ldmia   r0, {sp, lr}^
    nop
    add     sp, sp, #8
    ldmfd   sp!, {r3}
    msr     spsr_cxsf, r3
    ldmfd   sp!, {r0-r12, lr}

    subs    pc, lr, #0
