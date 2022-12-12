/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 */
#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/asm-offsets.h>
#include <asm/loongarch.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/thread_info.h>

/*
 * task_struct *__switch_to(task_struct *prev, task_struct *next,
 *			    struct thread_info *next_ti)
 */
	.align	5 
SYM_FUNC_START(__switch_to)
	csrrd	t1, LOONGARCH_CSR_PRMD // save prmd
	stptr.d	t1, a0, THREAD_CSRPRMD // save prmd

	cpu_save_nonscratch a0 // save a0-a1
	stptr.d	ra, a0, THREAD_REG01 // save ra
	move	tp, a2 // save tp
	cpu_restore_nonscratch a1 // restore a0-a1

	li.w	t0, _THREAD_SIZE - 32 // t0 = THREAD_SIZE - 32
	PTR_ADDU	t0, t0, tp // t0 = tp + THREAD_SIZE - 32
	set_saved_sp	t0, t1, t2 // t1 = t0 + 32

	ldptr.d	t1, a1, THREAD_CSRPRMD // load prmd
	csrwr	t1, LOONGARCH_CSR_PRMD // load prmd

	jr	ra
SYM_FUNC_END(__switch_to)