#ifndef KERNEL_H
#define KERNEL_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 20

#define PEACHOS_MAX_PATH 108

void kernel_main();
void print(const char* str);
void terminal_writechar(char c, char colour);

void panic(const char* msg);
void kernel_page();
void kernel_registers();


struct paging_desc;
struct paging_desc* kernel_desc();

#define ERROR(value) (void*)((intptr_t)value)
#define ERROR_I(value) (int)((intptr_t)(value))
#define ISERR(value) ((int)((int64_t)value) < 0)
#endif