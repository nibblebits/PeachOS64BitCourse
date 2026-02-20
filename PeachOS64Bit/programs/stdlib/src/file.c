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

#include "file.h"
#include "peachos.h"
#include <stdint.h>
#include <stddef.h>

int fopen(const char* filename, const char* mode)
{
    return (int) peachos_fopen(filename, mode);
}

void fclose(int fd)
{
    peachos_fclose((size_t) fd);
}

int fread(void* buffer, size_t size, size_t count, long fd)
{
    return peachos_fread(buffer, size, count, fd);
}

int fseek(int fd, int offset, int whence)
{
    return (int) peachos_fseek(fd, offset, whence);
}

int fstat(int fd, struct file_stat* file_stat_out)
{
    return (int) peachos_fstat(fd, file_stat_out);
}