#/bin/bash
export PREFIX="$HOME/opt64/cross"
export TARGET=x86_64-elf-cpp
export PATH="$PREFIX/bin:$PATH"
make all
