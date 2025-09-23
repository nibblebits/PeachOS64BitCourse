# PeachOS64 — Build a Modern 64-Bit Operating System From Scratch

This repository is the companion source code for **Part 2 of the PeachOS series**.  
In [Part 1](https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch?coupon=GITHUBKERNELDISCOUNT), we built **PeachOS**, a fully functioning **32-bit multitasking operating system**.  

In **Part 2**, we take the project to the next level:  
🚀 Building a **64-bit multi-threaded kernel** that runs in long mode with a **graphical user interface (GUI)** capable of rendering **interactive, clickable, draggable windows**.

---

[![Get the Course](https://img.shields.io/badge/Get%20the%20Course-Discount%20Link-blue?style=for-the-badge&logo=appveyor)](https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch-part-two-module-one?coupon=JUST99KERNEL)


## What You’ll Learn in This Course
This isn’t just theory—you’ll build a complete, modern OS step by step, starting from the bootloader all the way to a graphical desktop environment.

### 🔹 Module 1 Overview
- **Graphics & Display**
  - Capture and control the UEFI framebuffer (draw pixels directly to screen).
  - Load images & fonts.
  - Build a terminal that renders text using pixel-based fonts.
  - Create the graphical subsystem beneath the terminal:
    - Relative drawing anywhere on screen.  
    - Hierarchical graphics (parent/child offsets).  
    - Composable UI elements.

- **Memory Management**
  - Rebuild the heap allocator using the UEFI/BIOS E820 memory map.  
  - Multi-heap system with intelligent allocation.  
  - Paging-based **memory defragmenter** to solve fragmentation.

- **Disk & Partitions**
  - Extend FAT16 filesystem to support multiple **GPT partitions**.  
  - Mount each partition as a virtual drive.  
  - Abstract the disk system for modern SSD/PCIe storage support.

✅ **By the end of Module 1, you’ll have:**
- A 64-bit kernel bootable on modern UEFI hardware.  
- A pixel-based terminal with a full graphical foundation.  
- Advanced heap allocator + defragmentation support.  
- A multi-partition disk subsystem ready for GPT/SSD storage.  

---

### 🔮 Coming in Module 2 (ETA: Dec 2025)
- Standard C library in userspace (`fopen`, `fread`, etc.) via `isr80h`.  
- Full **windowing system** with interactive GUI elements.  
- NVMe SSD driver (high-speed storage).  
- PCI/PCIe enumeration & bridge support.  
- Run **real user programs** on your OS — contribute to the shared GitHub repo of student applications!  

By the end of Part 2, you’ll have built a **multi-threaded, 64-bit, GUI operating system** from scratch—bootable on real hardware and extensible enough to run user applications.

---

## 📚 Take the Course
This repo contains the **source code**, but the **course** walks you through every line of it step by step.  
👉 [Enroll Here](https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch-part-two-module-one?coupon=JUST99KERNEL) and start building your own OS today.

### 🎉 Special Discount
If you purchase **Module 1** before **Module 2** is released, you’ll get **Module 2 for just €34.99** (or equivalent in your currency).  
Simply email **daniel@dragonzap.com** with proof of purchase. Module 2 is estimated to be released in December 2025

---

## License
This code is provided for **educational purposes** as part of the PeachOS course under GPL license See the license file.
