CURRENT_DIR=$(pwd)

cd ../../../
set -e

export EDK_TOOLS_PATH=$HOME/edk2/BaseTools
. edksetup.sh BaseTools
build

cd "$CURRENT_DIR"

mkdir -p ./bin /mnt/d

dd if=/dev/zero bs=1048576 count=700 of=./bin/os.img
LOOPDEV=$(sudo losetup --find --show --partscan ./bin/os.img)
echo "Loop Device $LOOPDEV"

# Create a GPT partition
sudo parted "$LOOPDEV" --script mklabel gpt

# Create an EFI system parition
sudo parted "$LOOPDEV" --script mkpart ESP fat16 1Mib 50%

# Create another parition
sudo parted "$LOOPDEV" --script mkpart ESP fat16 50% 100%

# Mark the partition as bootable
sudo parted "$LOOPDEV" --script set 1 esp on
sudo partprobe "$LOOPDEV"
sleep 2
lsblk "$LOOPDEV"

sudo mkfs.vfat -n ABC "${LOOPDEV}p1"
sudo mkfs.vfat -n PEACH "${LOOPDEV}p2"

sudo mount -t vfat "${LOOPDEV}p2" /mnt/d

# Build the 64 bit peachos kernel
cd ./PeachOS64Bit
make clean
./build.sh
# end build

# GO back to the peachos directory
cd "$CURRENT_DIR"

# COpy the UEFI bootloader
cp ../../../Build/MdeModule/DEBUG_GCC/X64/PeachOS.efi ./bin/PeachOS.efi

# COpy the EFI file into the filesystem of partition two
sudo mkdir -p /mnt/d/EFI/BOOT
sudo cp ./bin/PeachOS.efi /mnt/d/EFI/Boot/BOOTX64.efi
sudo umount /mnt/d
sudo losetup -d "$LOOPDEV"

echo "Build completed"