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

#include "idt/irq.h"
#include "io/io.h"

void IRQ_enable(IRQ irq)
{
    int port = IRQ_MASTER_PORT;
    // The IRQ_line relative to the PIC
    IRQ relative_irq = irq;

    // If the IRQ belongs to slave pig
    // then update
    if (irq >= PIC_SLAVE_STARTING_IRQ)
    {
        port = IRQ_SLAVE_PORT;
        relative_irq = irq - PIC_SLAVE_STARTING_IRQ;
    }

    // Read the current value from the master or slave port
    int pic_value = insb(port);
    // Set the bit corrosponding to the IRQ to mask (disable) it
    pic_value  &= ~(1 << relative_irq);
    // Write the updated mask back to the PIC
    // disabling that IRQ functionality
    outb(port, pic_value);
}

void IRQ_disable(IRQ irq)
{
    int port = IRQ_MASTER_PORT;
    // The IRQ_line relative to the PIC
    IRQ relative_irq = irq;

    // If the IRQ belongs to slave pig
    // then update
    if (irq >= PIC_SLAVE_STARTING_IRQ)
    {
        port = IRQ_SLAVE_PORT;
        relative_irq = irq - PIC_SLAVE_STARTING_IRQ;
    }

    // Read the current value from the master or slave port
    int pic_value = insb(port);
    // Set the bit corrosponding to the IRQ to mask (disable) it
    pic_value  |= (1 << relative_irq);
    // Write the updated mask back to the PIC
    // disabling that IRQ functionality
    outb(port, pic_value);
}