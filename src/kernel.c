#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "memory/heap/kheap.h"
// heap
#include "memory/heap/heap.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "gdt/gdt.h"
#include "keyboard/keyboard.h"
#include "string/string.h"
#include "isr80h/isr80h.h"
#include "task/task.h"
#include "task/process.h"
#include "fs/file.h"
#include "disk/disk.h"
#include "io/io.h"

#include "fs/pparser.h"
#include "disk/streamer.h"
#include "task/tss.h"
#include "config.h"
#include "status.h"

uint16_t *video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

void terminal_backspace()
{
    if (terminal_row == 0 && terminal_col == 0)
    {
        return;
    }

    if (terminal_col == 0)
    {
        terminal_row -= 1;
        terminal_col = VGA_WIDTH;
    }

    terminal_col -= 1;
    terminal_writechar(' ', 15);
    terminal_col -= 1;
}

void terminal_writechar(char c, char colour)
{
    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }

    if (c == 0x08)
    {
        terminal_backspace();
        return;
    }

    terminal_putchar(terminal_col, terminal_row, c, colour);
    terminal_col += 1;
    if (terminal_col >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}
void terminal_initialize()
{
    video_mem = (uint16_t *)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0);
        }
    }
}

void print(const char *str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}
void panic(const char *msg)
{
    print(msg);
    while (1)
    {
    }
}

struct paging_desc *kernel_paging_desc = 0;

void kernel_page()
{
 //   kernel_registers();
    paging_switch(kernel_paging_desc);
}

struct paging_desc *kernel_desc()
{
    return kernel_paging_desc;
}

struct tss tss;

// Will point to the first gdt entry
extern struct gdt_entry gdt[];
void kernel_main()
{

    terminal_initialize();

    print("hello world");

    print("Total memory: \n");
    print(itoa(e820_total_accessible_memory()));
    print("\n");

    // // // Initialize the heap
    kheap_init();

    // Allocate the kernel page table

    kernel_paging_desc = paging_desc_new(PAGING_MAP_LEVEL_4);
    if (!kernel_paging_desc)
    {
        panic("Failed to create kernel paging descriptor\n");
    }

    // Map the kernel memory
    paging_map_e820_memory_regions(kernel_paging_desc);

    // switch to the kernel paging descriptor
    paging_switch(kernel_paging_desc);

    // // Paging is setp tell the heap..
    kheap_post_paging();

    print("test ok");
    // looks valid.

    // Now we need to rewrite the filesystem funcitonalituy soi it works in 64 bit mode
    fs_init();

    // // Search and initialize the disks
    disk_search_and_init();

    ///// TTEMMPORARILY REMOVED FILESYSTEM STUFF UNTIL IG ET IT WORKING

    // Initialize the interrupt descriptor table
    idt_init();

    // Let's allocate a 1 MB stack for the kernel IDT
    // when we switch to ring zero we will use this address
    size_t stack_size = 1024 * 1024;
    // stack grows downwards so the megabyte_stack_tss_end is the last page of the stack
    void *megabyte_stack_tss_end = kzalloc(stack_size);
    void *megabyte_stack_tss_begin = (void *)(((uintptr_t)megabyte_stack_tss_end) + stack_size);

    // For security reasons the first page wont be present, this is so if our stack gets used completley
    // it doesnt start overwriting in other pages of memory
    paging_map(megabyte_stack_tss_end, megabyte_stack_tss_end, megabyte_stack_tss_end, 0);

    // // Setup the TSS
    memset(&tss, 0x00, sizeof(tss));
    tss.rsp0 = (uint64_t)megabyte_stack_tss_begin;

    struct tss_desc_64 *tssdesc = (struct tss_desc_64 *)&gdt[KERNEL_LONG_MODE_TSS_GDT_INDEX];

    gdt_set_tss(tssdesc, &tss,
                sizeof(tss),
                0x89, // TSS type + present
                0x00  // flags
    );
    // Load the TSS
    tss_load(KERNEL_LONG_MODE_TSS_SELECTOR);

    // // Register the kernel commands
    isr80h_register_commands();

    // // Initialize all the system keyboards
    keyboard_init();

    print("NO crash yet\n");

    struct process *process = 0;
    int res = process_load_switch("0:/simple.bin", &process);
    if (res != PEACHOS_ALL_OK)
    {
        panic("Failed to load simple.bin\n");
    }
    

    task_run_first_ever_task();

    while (1)
    {
    }
}