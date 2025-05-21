#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct interrupt_frame;
typedef void*(*ISR80H_COMMAND)(struct interrupt_frame* frame);
typedef void(*INTERRUPT_CALLBACK_FUNCTION)(struct interrupt_frame* frame);


// 64 bit IDT DESCRIPTOR
struct idt_desc
{
    uint16_t offset_1; // Offset bits 0 - 15
    uint16_t selector; // Selector thats in our GDT
    uint8_t ist;       // bits 0-2 holds the interrupt stack table offset
    uint8_t type_attr; // Descriptor type and attributes
    uint16_t offset_2;  // offset bits 16 - 31
    uint32_t offset_3; // Offset bits 32 - 63
    uint32_t reserved;  // reserrved NULL it..
} __attribute__((packed));

struct idtr_desc
{
    uint16_t limit; // Size of descriptor table -1
    uint64_t base; // Base address of the start of the interrupt descriptor table
} __attribute__((packed));

struct interrupt_frame
{
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t reserved;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t ip;
    uint64_t cs;
    uint64_t flags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

void idt_init();
void enable_interrupts();
void disable_interrupts();
void isr80h_register_command(int command_id, ISR80H_COMMAND command);
int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback);

#endif