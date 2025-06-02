#/bin/bash
export PREFIX="$HOME/opt64/cross"
export TARGET=x86_64-elf-cpp
export PATH="$PREFIX/bin:$PATH"

mkdir -p ./bin ./build ./build/loader ./build/loader/formats ./build/isr80h ./build/keyboard ./build/gdt ./build/disk ./build/task ./build/fs ./build/fs/fat ./build/memory ./build/io ./build/memory/paging ./build/memory/heap ./build/string ./build/idt  
make all
