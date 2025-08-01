#ifndef KERNEL_ISR80H_FILE_H
#define KERNEL_ISR80H_FILE_H

struct interrupt_frame;
void* isr80h_command11_fclose(struct interrupt_frame* frame);
void* isr80h_command10_fopen(struct interrupt_frame* frame);
#endif