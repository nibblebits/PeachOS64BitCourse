/** @file
  This sample application bases on HelloWorld PCD setting
  to print "UEFI Hello World!" to the UEFI Console.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Guid/FileInfo.h>
#include "./PeachOS64Bit/src/config.h"
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

EFI_HANDLE imageHandle = NULL;
EFI_SYSTEM_TABLE* systemTable = NULL;

EFI_STATUS ReadFileFromCurrentFilesystem(CHAR16* FileName, VOID** Buffer_Out, UINTN *BufferSize_Out)
{
  EFI_STATUS Status = 0;
  EFI_LOADED_IMAGE_PROTOCOL* LoadedImageProtocol = NULL;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* SimpleFileSystem = NULL;

  EFI_FILE_PROTOCOL* Root = NULL;
  EFI_FILE_PROTOCOL* File = NULL;
  UINTN FileInfoSize = 0;

  *Buffer_Out = NULL;
  *BufferSize_Out = 0;

  Status = gBS->HandleProtocol(
      imageHandle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&LoadedImageProtocol
  );

  if (EFI_ERROR(Status))
  {
     Print(L"Error accessing LoadedImageProtocol: %r\n", Status);
     return Status;
  }

  Status = gBS->HandleProtocol(
      LoadedImageProtocol->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&SimpleFileSystem
  );

  if (EFI_ERROR(Status))
  {
    Print(L"Error accessing SimpleFIleSystem: %r\n", Status);
    return Status;
  }

  Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
  if (EFI_ERROR(Status))
  {
    Print(L"Error opening root directory: %r\n", Status);
    return Status;
  }

  // Open the file in the root directory
  Status = Root->Open(
    Root,
    &File,
    FileName,
    EFI_FILE_MODE_READ,
    0
  );

  if (EFI_ERROR(Status))
  {
    Print(L"Error opening file %s: %r\n", FileName, Status);
    return Status;
  }

  // Retrieve file information to determine file size
  FileInfoSize = OFFSET_OF(EFI_FILE_INFO, FileName) + 256 * sizeof(CHAR16);
  VOID* FileInfoBuffer = AllocatePool(FileInfoSize);
  if (FileInfoBuffer == NULL)
  {
    Print(L"Error allocating buffer for file info\n");
    File->Close(File);
    return EFI_OUT_OF_RESOURCES;
  }

  EFI_FILE_INFO* FileInfo = (EFI_FILE_INFO*)FileInfoBuffer;
  Status = File->GetInfo(
    File, 
    &gEfiFileInfoGuid, 
    &FileInfoSize,
    FileInfo
  );

  if (EFI_ERROR(Status))
  {
    Print(L"Error getting file info for %s %r\n", FileName, Status);
    FreePool(FileInfoBuffer);
    File->Close(File);
    return Status;
  }

  UINTN BufferSize = FileInfo->FileSize;
  FreePool(FileInfoBuffer);
  FileInfoBuffer = NULL;

  // Allocate the memory for the file content
  VOID* Buffer = AllocatePool(BufferSize);
  if(Buffer == NULL)
  {
    Print(L"Error allocating buffer for file %s\n", FileName);
    File->Close(File);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = File->Read(File, &BufferSize, Buffer);
  if (EFI_ERROR(Status))
  {
    Print(L"Error reading file %s %r", FileName, Status);
    FreePool(Buffer);
    File->Close(File);
    return Status;
  }

  *Buffer_Out = Buffer;
  *BufferSize_Out = BufferSize;

   Print(L"Read %d bytes from file %s\n", BufferSize, FileName);

  // Close the file
  File->Close(File);
  return EFI_SUCCESS;
}
/*
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  imageHandle = ImageHandle;
  systemTable = SystemTable;
  EFI_STATUS Status = 0;

  Print(L"Peach OS UEFI bootloader.");

  VOID* KernelBuffer = NULL;
  UINTN KernelBufferSize = 0;
  Status = ReadFileFromCurrentFilesystem(L"kernel.bin", &KernelBuffer, &KernelBufferSize);
  if (EFI_ERROR(Status))
  {
    Print(L"Error reading kernel: %r\n", Status);
    return Status;
  }

  Print(L"Kernel file loaded successfully at: %p\n", KernelBuffer);
  // The kernel must be mapped at 0x100000
  EFI_PHYSICAL_ADDRESS KernelBase = PEACHOS_KERNEL_LOCATION;
  Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, EFI_SIZE_TO_PAGES(KernelBufferSize), &KernelBase);
  if (EFI_ERROR(Status))
  {
    Print(L"Error allocating memory for kernel %r\n", Status);
    return Status;
  }

  // Copy the kernel to the allocated memory
  CopyMem((VOID*)KernelBase, KernelBuffer, KernelBufferSize);
  Print(L"Kernel copied to memory at: %p\n", KernelBase);

  // End the UEFI services and jump to the kernel
  gBS->ExitBootServices(ImageHandle, 0);
  
  __asm__("jmp *%0" : : "r"(KernelBase));

  return EFI_SUCCESS;
}
