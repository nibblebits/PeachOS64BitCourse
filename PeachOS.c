/*
 * PeachOS 64-Bit Kernel Project
 * Copyright (C) 2026 Daniel McCarthy <daniel@dragonzap.com>
 *
 * This file is part of the PeachOS 64-Bit Kernel.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * For full source code, documentation, and structured learning,
 * see the official kernel development course part one:
 * https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch

 * Get part one and part two module one, module two all peachos courses (69 hours of content): https://dragonzap.com/offer/kernel-development-from-scratch-69-hours

 * Get the part two, module one and two modules: https://dragonzap.com/offer/developing-a-multithreaded-kernel-from-scratch-part-two-full-series
 */

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

typedef struct __attribute__((packed)) E820Entry
{
  UINT64 base_addr;
  UINT64 length;
  UINT32 type;
  UINT32 extended_attr;
} E820Entry;

typedef struct __attribute__((packed)) E820Entries
{
  UINT64 count;
  E820Entry entries[];
} E820Entries;

EFI_HANDLE imageHandle = NULL;
EFI_SYSTEM_TABLE* systemTable = NULL;

EFI_STATUS SetupMemoryMaps()
{
   EFI_STATUS status;
   UINTN memoryMapSize = 0;
   EFI_MEMORY_DESCRIPTOR* memoryMap = NULL;
   UINTN mapKey;
   UINTN descriptorSize;
   UINT32 descriptorVersion;

   status = gBS->GetMemoryMap(
     &memoryMapSize,
     NULL, 
     &mapKey,
     &descriptorSize,
     &descriptorVersion
   );

   if (status != EFI_BUFFER_TOO_SMALL && EFI_ERROR(status))
   {
     Print(L"Error retreving initial memory map size: %r\n", status);
     return status;
   }

   // Calculate memory map size 
   memoryMapSize += descriptorSize * 10;

   // Allocate a buffer for the memory map
   memoryMap = AllocatePool(memoryMapSize);
   if (memoryMap == NULL)
   {
      Print(L"Error allocating memory for memory map\n");
      return EFI_OUT_OF_RESOURCES;
   }

   status = gBS->GetMemoryMap(
     &memoryMapSize,
     memoryMap,
     &mapKey,
     &descriptorSize,
     &descriptorVersion
   );

   if (EFI_ERROR(status))
   {
      Print(L"Error getting memory map: %r\n", status);
      FreePool(memoryMap);
      return status;
   }

  UINTN descriptorCount = memoryMapSize / descriptorSize;
  EFI_MEMORY_DESCRIPTOR* desc = memoryMap;
  UINTN totalConventionalDescriptors = 0;
  for (UINTN i = 0; i < descriptorCount; ++i)
  {
    if (desc->Type == EfiConventionalMemory)
    {
      totalConventionalDescriptors++;
    }
    desc = (EFI_MEMORY_DESCRIPTOR*)((UINT8*) desc+descriptorSize);
  }

  EFI_PHYSICAL_ADDRESS MemoryMapLocationE820 = PEACHOS_MEMORY_MAP_TOTAL_ENTRIES_LOCATION;
  UINTN MemoryMapSizeE820 = (sizeof(E820Entry) * totalConventionalDescriptors) + sizeof(UINT64);
  status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, EFI_SIZE_TO_PAGES(MemoryMapSizeE820), &MemoryMapLocationE820);
  if (EFI_ERROR(status))
  {
    Print(L"Error allocating memory for the E820 Entries: %r\n", status);
    return status;
  }

  E820Entries* e820Entries = (E820Entries*) MemoryMapLocationE820;
  UINTN ConventionalMemoryIndex = 0;
  desc = memoryMap;
  for (UINTN i = 0; i < descriptorCount; ++i)
  {
    if (desc->Type == EfiConventionalMemory)
    {
      E820Entry* e820Entry = &e820Entries->entries[ConventionalMemoryIndex];
      e820Entry->base_addr = desc->PhysicalStart;
      e820Entry->length = desc->NumberOfPages * 4096;
      e820Entry->type = 1; // Usuable memory
      e820Entry->extended_attr = 0;
      Print(L"e820Entry=%p\n", e820Entry);
      ConventionalMemoryIndex++;
    }
    desc = (EFI_MEMORY_DESCRIPTOR*) ((UINT8*) desc+descriptorSize);
  }

  e820Entries->count = totalConventionalDescriptors;
  FreePool(memoryMap);

  return EFI_SUCCESS;
}

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

EFI_STATUS GetFrameBufferInfo(EFI_GRAPHICS_OUTPUT_PROTOCOL** GraphicsOutput)
{
  EFI_STATUS Status;
  // Locate the graphics output protocol
  Status = gBS->LocateProtocol(
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    (VOID**) GraphicsOutput
  );

  if (EFI_ERROR(Status))
  {
     Print(L"Error: unable to locate GOP: %r", Status);
     return Status;
  }

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
  UINTN SizeOfInfo;

  // Get the current mode
  Status = (*GraphicsOutput)->QueryMode(*GraphicsOutput, (*GraphicsOutput)->Mode->Mode, &SizeOfInfo, &Info);
  if (EFI_ERROR(Status))
  {
    Print(L"Unable to query mode %r\n", Status);
    return Status;
  }

  Print(L"Framebuffer base address: %p", (*GraphicsOutput)->Mode->FrameBufferBase);
  Print(L"Framebuffer size: %lu bytes\n", (*GraphicsOutput)->Mode->FrameBufferSize);
  Print(L"Screen resolution: %u x %u\n", Info->HorizontalResolution, Info->VerticalResolution);
  Print(L"Pixels per scan line: %u\n", Info->PixelsPerScanLine);
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
  // Setup and load E820 Entries
  SetupMemoryMaps();
  
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

  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput = NULL;
  // Lets get the frame buffers
  Status = GetFrameBufferInfo(&GraphicsOutput);
  if (EFI_ERROR(Status))
  {
    Print(L"Error getting frame buffer info: %r\n", Status);
    return Status;
  }
  
  // Draw the entire screen green
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL* FrameBuffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*) GraphicsOutput->Mode->FrameBufferBase;
  // There can be padding so we must use pixels per scan line
  UINTN PixelsPerScanLine = GraphicsOutput->Mode->Info->PixelsPerScanLine;
  UINTN HoriziontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
  UINTN VerticalResoltuion = GraphicsOutput->Mode->Info->VerticalResolution;
  for (UINTN y = 0; y < VerticalResoltuion; y++)
  {
    for (UINTN x = 0; x < HoriziontalResolution; x++)
    {
      FrameBuffer[y * PixelsPerScanLine + x].Red = 0x00;
      FrameBuffer[y * PixelsPerScanLine + x].Green = 0xff;
      FrameBuffer[y * PixelsPerScanLine + x].Blue = 0x00;
      FrameBuffer[y * PixelsPerScanLine + x].Reserved = 0x00;
    }
  }
  // End the UEFI services and jump to the kernel
  gBS->ExitBootServices(ImageHandle, 0);

  __asm__ __volatile__(
      "movq %0, %%rdi\n\t"  // Frame buffer base
      "movq %1, %%rsi\n\t"  // pixels per scan line
      "movq %2, %%rdx\n\t"  // horiziontal resolution
      "movq %3, %%rcx\n\t"  // vertical resolution
      :
      :"r"((UINT64) FrameBuffer),
       "r"((UINT64) PixelsPerScanLine),
       "r"((UINT64) HoriziontalResolution),
       "r"((UINT64) VerticalResoltuion)
      : "rdi", "rsi", "rdx", "rcx");

   __asm__("jmp *%0" : : "r"(KernelBase));

  // Will never get run because of the jump above.
  return EFI_SUCCESS;
}
