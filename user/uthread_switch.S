	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	    /* 保存当前线程的寄存器到 old context */
        sd ra, 0(a0)      /* 保存返回地址 */
        sd sp, 8(a0)      /* 保存栈指针 */
        sd s0, 16(a0)     /* 保存 s0 */
        sd s1, 24(a0)     /* 保存 s1 */
        sd s2, 32(a0)     /* 保存 s2 */
        sd s3, 40(a0)     /* 保存 s3 */
        sd s4, 48(a0)     /* 保存 s4 */
        sd s5, 56(a0)     /* 保存 s5 */
        sd s6, 64(a0)     /* 保存 s6 */
        sd s7, 72(a0)     /* 保存 s7 */
        sd s8, 80(a0)     /* 保存 s8 */
        sd s9, 88(a0)     /* 保存 s9 */
        sd s10, 96(a0)    /* 保存 s10 */
        sd s11, 104(a0)   /* 保存 s11 */

        /* 从 new context 恢复新线程的寄存器 */
        ld ra, 0(a1)      /* 恢复返回地址 */
        ld sp, 8(a1)      /* 恢复栈指针 */
        ld s0, 16(a1)     /* 恢复 s0 */
        ld s1, 24(a1)     /* 恢复 s1 */
        ld s2, 32(a1)     /* 恢复 s2 */
        ld s3, 40(a1)     /* 恢复 s3 */
        ld s4, 48(a1)     /* 恢复 s4 */
        ld s5, 56(a1)     /* 恢复 s5 */
        ld s6, 64(a1)     /* 恢复 s6 */
        ld s7, 72(a1)     /* 恢复 s7 */
        ld s8, 80(a1)     /* 恢复 s8 */
        ld s9, 88(a1)     /* 恢复 s9 */
        ld s10, 96(a1)    /* 恢复 s10 */
        ld s11, 104(a1)   /* 恢复 s11 */
        
        ret               /* 跳转到新线程 (通过 ra 寄存器) */
