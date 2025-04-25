#/bin/bash
export PREFIX="$HOME/opt64/cross"
export TARGET=x86_64-elf-cpp
export PATH="$PREFIX/bin:$PATH"

mkdir -p ./bin ./build ./build/memory ./build/io ./build/memory/paging ./build/memory/heap ./build/string  
make all
