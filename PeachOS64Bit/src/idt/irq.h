/*
 * PeachOS 64-Bit Kernel Project
 * Copyright (C) 2026 Daniel McCarthy <daniel@dragonzap.com>
 *
 * This file is part of the PeachOS 64-Bit Kernel.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * For full source code, documentation, and structured learning,
 * see the official kernel development course part one:
 * https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch

 * Get part one and part two module one, module two all peachos courses (69 hours of content): https://dragonzap.com/offer/kernel-development-from-scratch-69-hours

 * Get the part two, module one and two modules: https://dragonzap.com/offer/developing-a-multithreaded-kernel-from-scratch-part-two-full-series
 */

#ifndef KERNEL_IRQ
#define KERNEL_IRQ
enum
{
    IRQ_TIMER,
    IRQ_KEYBOARD,
    IRQ_CASCADE,
    IRQ_COM2,
    IRQ_COM1,
    IRQ_LPT2,
    IRQ_FLOPPY_DISK,
    IRQ_LPT1,
    IRQ_CMOS,
    IRQ_LEGACY_SCSI,
    IRQ_SCSI,
    IRQ_PS2_MOUSE,
    IRQ_FPU_COPROCESSOR,
    IRQ_PRIMARY_ATA_HDD,
    IRQ_SECONDARY_ATA_HDD
};

#define PIC_MASTER_ENDING_IRQ 7
#define PIC_SLAVE_STARTING_IRQ 8
#define PIC_SLAVE_ENDING_IRQ 15
#define IRQ_MASTER_PORT 0x21
#define IRQ_SLAVE_PORT 0xA1

typedef int IRQ;
void IRQ_disable(IRQ irq);
void IRQ_enable(IRQ irq);
#endif