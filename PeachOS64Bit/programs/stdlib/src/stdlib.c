#include "stdlib.h"
#include "peachos.h"
#include "memory.h"

char* itoa(int i)
{
    static char text[12];
    int loc = 11;
    text[11] = 0;
    char neg = 1;
    if (i >= 0)
    {
        neg = 0;
        i = -i;
    }

    while(i)
    {
        text[--loc] = '0' - (i % 10);
        i /= 10;
    }

    if (loc == 11)
        text[--loc] = '0';
    
    if (neg)
        text[--loc] = '-';

    return &text[loc];
}

void* malloc(size_t size)
{
    return peachos_malloc(size);
}
void* calloc(size_t n_memb, size_t size)
{
   size_t b_size = n_memb * size;
   void* ptr = malloc(b_size);
   if (!ptr)
   {
        return NULL;
   }   

   memset(ptr, 0, b_size);
   return ptr;
}

void* realloc(void* ptr, size_t new_size)
{
    return peachos_realloc(ptr, new_size);
}

void free(void* ptr)
{
    peachos_free(ptr);
}