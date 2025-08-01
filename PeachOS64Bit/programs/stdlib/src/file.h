#ifndef USERLAND_FILE_H
#define USERLAND_FILE_H

int fopen(const char* filename, const char* mode);
void fclose(int fd);
#endif