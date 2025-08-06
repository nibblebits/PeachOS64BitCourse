#ifndef USERLAND_FILE_H
#define USERLAND_FILE_H

int fopen(const char* filename, const char* mode);
void fclose(int fd);
int fread(void* buffer, size_t size, size_t count, long fd);

#endif