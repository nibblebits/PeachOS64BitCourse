qemu-system-x86_64 \
  -machine pc \
  -drive file=./bin/os.img,format=raw,if=ide \
  -m 512M \
  -cpu qemu64 \
  -bios /usr/share/ovmf/OVMF.fd