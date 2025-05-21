#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/heap/heap.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "keyboard/keyboard.h"
#include "string/string.h"
#include "isr80h/isr80h.h"
#include "task/task.h"
#include "task/process.h"
#include "fs/file.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "disk/streamer.h"
#include "task/tss.h"
#include "gdt/gdt.h"

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

// static struct paging_4gb_chunk* kernel_chunk = 0;

// void kernel_page()
// {
//     kernel_registers();
//     paging_switch(kernel_chunk);
// }

// struct tss tss;
// struct gdt gdt_real[PEACHOS_TOTAL_GDT_SEGMENTS];
// struct gdt_structured gdt_structured[PEACHOS_TOTAL_GDT_SEGMENTS] = {
//     {.base = 0x00, .limit = 0x00, .type = 0x00},                // NULL Segment
//     {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
//     {.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
//     {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
//     {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
//     {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}      // TSS Segment
// };

struct tss tss;

extern struct gdt_entry gdt[];

// page descriptor
struct paging_desc *kernel_paging_desc = 0;

void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_paging_desc);
}


struct paging_desc* kernel_desc()
{
    return kernel_paging_desc;
}

void kernel_main()
{
    terminal_initialize();
    print("Hello 64-bit!\n");

    print("Total memory\n");
    print(itoa(e820_total_accessible_memory()));
    print("\n");

    kheap_init();

    char *data = kmalloc(50);
    data[0] = 'A';
    data[1] = 'B';
    data[2] = 'C';
    data[3] = 0x00;
    print(data);
    kernel_paging_desc = paging_desc_new(PAGING_MAP_LEVEL_4);
    if (!kernel_paging_desc)
    {
        panic("Failed to create kernel paging descriptor\n");
    }
    paging_map_e820_memory_regions(kernel_paging_desc);

    paging_switch(kernel_paging_desc);

    // The multi-heap is ready
    kheap_post_paging();

    // Enable interrupt descriptor table
    idt_init();

    // Enable fs functionality
    fs_init();

    // Enable the disks
    disk_search_and_init();

    // Allocate a 1 MB stack for the kernel IDT 
    size_t stack_size = 1024*1024;
    void* megabyte_stack_tss_end = kzalloc(stack_size);
    void* megabyte_stack_tss_begin = (void*)(((uintptr_t) megabyte_stack_tss_end) + stack_size);
    
    // Block the first page
    paging_map(kernel_desc(), megabyte_stack_tss_end, megabyte_stack_tss_end, 0);

    // Setup the TSS
    memset(&tss, 0x00, sizeof(tss));
    tss.rsp0 = (uint64_t) megabyte_stack_tss_begin;
    tss.iopb_offset = sizeof(tss); // No I/O permissions are used

    struct tss_desc_64* tssdesc = (struct tss_desc_64*)&gdt[KERNEL_LONG_MODE_TSS_GDT_INDEX];
    gdt_set_tss(tssdesc, &tss, sizeof(tss)-1, TSS_DESCRIPTOR_TYPE, 0x00);
    
    // load the tss
    tss_load(KERNEL_LONG_MODE_TSS_SELECTOR);

    print("tss load was fine\n");
    // Register isr80h commands
    isr80h_register_commands();
    print("register isr80h\n");

    // Initialize the keyboard
    keyboard_init();

    print("Loading program...\n");
    struct process* process = 0;
    int res = process_load_switch("0:/simple.bin", &process);
    if (res != PEACHOS_ALL_OK)
    {
        panic("Failed to load user program\n");
    }

    // Drop to user land
    task_run_first_ever_task();

    //  data[0] = 'M';
    //  print(data);

    //  struct heap* kernel_heap = kheap_get();
    //  size_t total = heap_total_size(kernel_heap);
    //  size_t used = heap_total_used(kernel_heap);
    //  size_t avail = heap_total_available(kernel_heap);

    //  print("Total heap size: ");
    //  print(itoa(total));
    //  print("\n");

    //  print("Total heap used: ");
    //  print(itoa(used));
    //  print("\n");

    //  print("Total heap available: ");
    //  print(itoa(avail));
    //  print("\n");

    // OLD CODE BELOW
    // ----------------------------------

    // memset(gdt_real, 0x00, sizeof(gdt_real));
    // gdt_structured_to_gdt(gdt_real, gdt_structured, PEACHOS_TOTAL_GDT_SEGMENTS);

    // // Load the gdt
    // gdt_load(gdt_real, sizeof(gdt_real)-1);

    // // Initialize the heap
    // kheap_init();

    // // Initialize filesystems
    // fs_init();

    // // Search and initialize the disks
    // disk_search_and_init();

    // // Initialize the interrupt descriptor table
    // idt_init();

    // // Setup the TSS
    // memset(&tss, 0x00, sizeof(tss));
    // tss.esp0 = 0x600000;
    // tss.ss0 = KERNEL_DATA_SELECTOR;

    // // Load the TSS
    // tss_load(0x28);

    // // Setup paging
    // kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    // // Switch to kernel paging chunk
    // paging_switch(kernel_chunk);

    // // Enable paging
    // enable_paging();

    // // Register the kernel commands
    // isr80h_register_commands();

    // // Initialize all the system keyboards
    // keyboard_init();

    // struct process* process = 0;
    // int res = process_load_switch("0:/blank.elf", &process);
    // if (res != PEACHOS_ALL_OK)
    // {
    //     panic("Failed to load blank.elf\n");
    // }

    // struct command_argument argument;
    // strcpy(argument.argument, "Testing!");
    // argument.next = 0x00;

    // process_inject_arguments(process, &argument);

    // res = process_load_switch("0:/blank.elf", &process);
    // if (res != PEACHOS_ALL_OK)
    // {
    //     panic("Failed to load blank.elf\n");
    // }

    // strcpy(argument.argument, "Abc!");
    // argument.next = 0x00;
    // process_inject_arguments(process, &argument);

    // task_run_first_ever_task();

    while (1)
    {
    }
}