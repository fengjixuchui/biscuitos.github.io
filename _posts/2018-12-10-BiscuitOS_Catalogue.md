---
layout: post
title:  ""
date:   2018-06-30 12:44:17 +0800
categories: [Top]
excerpt: Catalogue.
tags:
---

![](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/GIF000204.gif)

![](/assets/PDB/RPI/RPI100100.png)

## <span id="Kernel_Establish">Kernel Establish</span>

> - [Build Linux 5.14 - RISCV32/RISCV64/ARM32/ARM64/I386/X86_64](/blog/Kernel_Build/#Linux_5Y)
>
> - [Build Upstream Linux from Linus Torvalds](/blog/Kernel_Build/#Linux_newest)
>
> - [Build Linux 0.X](/blog/Kernel_Build/#Linux_0X)
>
> - [Build Linux 1.X](/blog/Kernel_Build/#Linux_1X)
>
> - [Build Linux 2.X](/blog/Kernel_Build/#Linux_2X)
>
> - [Build Linux 3.X](/blog/Kernel_Build/#Linux_3X)
>
> - [Build Linux 4.X](/blog/Kernel_Build/#Linux_4X)
>
> - [Build Linux 5.X](/blog/Kernel_Build/#Linux_5X)
>
> - [BiscuitOS-Desktop GOKU](/blog/Desktop-GOKU-Usermanual/)
>
> - [Build BiscuitOS-Desktop](/blog/BiscuitOS-Desktop-arm32-Usermanual/)
>
> - [Networking on BiscuitOS](/blog/Networking-Usermanual/)
>
> - Other Operation System
>
>   - [Build Debian-0.91 on BiscuitOS](/blog/Debian-0.91-Usermanual)
>   
>   - [Build Apollo-11 on BiscuitOS](/blog/Apollo-11-Usermanual)
>
>   - [Build MIT XV6 on BiscuitOS](/blog/XV6-REV11-Usermanual)
>
>   - [Build SerenityOS on BiscuitOS](/blog/SerenityOS)
>
> - [BiscuitOS on RaspberryPi](#RaspberryPi)
>
> - [Linux Favourable Mechansim](/blog/Human-Knowledge-Common/#H000023)
>
> - [BiscuitOS 社区技术论坛](https://github.com/BiscuitOS/BiscuitOS/discussions)
>
> - [问题反馈及留言](https://github.com/BiscuitOS/BiscuitOS/issues)

![](/assets/PDB/GamePICBF000001.gif)
![](/assets/PDB/BGP/BGP000100.png)

## [内存管理]()

> - [内存百科全书](/blog/Memory-Hardware/)
>
> - [Linux MMU History Project](/blog/HISTORY-MMU/)
>
> - [自己动手设计一个内存管理子系统 (ARM Architecture)](/blog/Design-MMU/)
>
> - [自己动手设计一个内存管理子系统 (X86 Architecture)](/blog/Design-MMU/)
>
> - Detecting Memory
>
>   - [Detecting Memory from CMOS \[X86\]](/blog/MMU-seaBIOS_E820/#E1)
>
>   - [Detecting Memory from BDA \[X86\]](/blog/MMU-seaBIOS_E820/#E2)
>
>   - [Detecting Memory from BIOS IVT \[X86\]](/blog/MMU-E820/#F1)
>
>   - [Detecting Memory from CMDLINE](/blog/MMU-E820/#B0007)
>
>   - [E820 内存管理器 \[X86\]](/blog/MMU-E820/)
>
> - Boot-Time Allocator
>
>   - [Static Compile Memory Allocator]()
>
>   - [RESERVE_BRK Allocator \[x86\]](/blog/RESERVE_BRK/)
>
>   - [Early_Res Allocator \[x86\]](/blog/Early_Res/)
>
>   - [Bootmem Allocator](/blog/HISTORY-bootmem/)
>
>   - [MEMBLOCK Allocator](/blog/MMU-ARM32-MEMBLOCK-index/)
>
> - Memory Model
>
>   - [Flat Memory Model]()
>
>   - [Discontig Memory Model]()
>
>   - [SPARSE Memory Model]()
>
> - Physical Memory Manager
>
>   - [NUMA]()
>
>   - [UMA]()
>
>   - [ZONE]()
>
>   - [Physical Page/Frame]()
>
>   - [Buddy Memory Allocator](/blog/HISTORY-buddy/)
>
>   - [PCP Memory Allocator](/blog/HISTORY-PCP/)
>
> - I/O and Device Memory
>
>   - [PCI and PCIe Address Space](/blog/PCI-Address-Space/)
>
>   - I/O Port Space
>
> - [PERCPU Memory Allocator](/blog/HISTORY-PERCPU/)
>
> - SLAB/KMEM Allocator
>
>   - [SLAB Memory Allocator](/blog/HISTORY-SLAB/)
>
>   - [SLUB Memory Allocator]()
>
>   - [SLOB Memory Allocator]()
>
> - [VMALLOC Memory Allocator](/blog/HISTORY-VMALLOC/)
>
> - [KMAP Memory Allocator](/blog/HISTORY-KMAP/)
>
> - [Fixmap Memory Allcator](/blog/HISTORY-FIXMAP/)
>
> - [CMA: Contiguous Memory Allocator](/blog/CMA)
>
> - Paging Mechanism
>
>   - [Intel 32-Bit Paging Mechanism](/blog/32bit-Paging/)
>
>   - [Intel PAE Paging Mechanism]()
>
>   - [Intel 4-level/5-level Paging Mechanism]()
>
>   - [EPT Paging Mechansim]()
>
> - Page Fault Mechanism
>
>   - [Anonymous Shared/Private Page Fault]()
>
>   - [File Shared/Private Page Fault]()
>
>   - [缺页实践: Page Fault Practice and Trace on 32-Bit Paging](/blog/32bit-Paging/#K0)
>
> - Huge Page Mechanism
>
>   - [Hugetlb and Hugetlbfs Mechanism](/blog/Hugetlbfs)
>
>   - [Transparent HugePage Mechanism]()
>
> - Memory Mmapping Mechanism
>
> - Memory Compact Technology
>
> - Memory Reclaim Mechansim
>
>   - SWAP
>
> - [OOM Mechanism](/blog/OOM/)
>
> - Share Memory Mechanism
>
>   - [SHM: Share Memory on Process]()
>
>   - [KSM: Kernel Same Page Mechanism]()
>
> - Memory Hotplug Technology
>
> - [Memory Virtualize]()
>
>   - [QEMU Memory Manager for Virtual machine](/blog/QEMU-Memory-Manager-VM)
>
> - Memory Principle
>
>   - [PAT: Page Attribute Table]()
>
>   - [MTRR: Memory Type Range Register]()
>
>   - [Register With Memory Mechanism](/blog/Register/)
>
> - Address Space Layout
>
>   - [X86: Real Mode Address Space (\< 1 MiB)](/blog/MMU-seaBIOS_E820/#B0003)
>
>   - [进程地址空间构建研究](/blog/Process-Address-Space)
>
>   - [i386/X86_64 Address Space Layout](/blog/Address-Space-i386)
>
> - Memory Diagnostic/Sanitizers Tools
>
>   - [Live/offline Crash Tools](/blog/CRASH)
>
>   - [Memory Statistical tools](/blog/Memory-Statistic-Tools)
>
>   - [GUN GCC Address/Memory/Leaks Sanitizers (ASAN)](/blog/ASAN)
>
>   - [KASAN/KMEM]()
>
> - Userspace Memory Manager
>
>   - GLIBC/LIBC: malloc/mallopt
>
>   - tcmalloc
>
>   - libhugetlbfs: Hugetlb allocator on Userspace
>
>   - [用户空间实现 Linux 内核分配器](/blog/Memory-Userspace/)
>
> - 内存压测工具
>
>   - Intel MLC
>
>   - memtest
>
> - [Memory ERROR Collection](/blog/Memory-ERROR/)

![](/assets/PDB/BiscuitOS/kernel/IND000108.png)

## [虚拟文件系统]()

> - [系统调用](/blog/SYSCALL)
>
>   - [sys_open](/blog/SYSCALL_sys_open/)
>
>   - sys_mmap/sys_mmap2
>
>   - sys_munmap
>
>   - sys_brk
>
> - SYSCALL on Architecture
>
>   - [ARM32 架构中添加新的系统调用](/blog/SYSCALL_ADD_NEW_ARM/)
>
>   - [ARM64 架构中添加新的系统调用](/blog/SYSCALL_ADD_NEW_ARM64/)
>
>   - [i386 架构中添加新的系统调用](/blog/SYSCALL_ADD_NEW_I386/)
>
>   - [X86_64 架构中添新的系统调用](/blog/SYSCALL_ADD_NEW_X86_64/)
>
>   - [RISCV32 架构中添新的系统调用](/blog/SYSCALL_ADD_NEW_RISCV32/)
>
>   - [RISCV64 架构中添加新的系统调用](/blog/SYSCALL_ADD_NEW_RISCV64/)

![](/assets/PDB/GamePICBS00001.gif)
![](/assets/PDB/BGP/BGP000101.png)

## [虚拟化]()

> - 虚拟化实践
>
>   - [KVM on BiscuitOS (4KiB Page) \[x86_64/i386\]](/blog/KVM-on-BiscuitOS/)
>
>   - [KVM on BiscuitOS (2MiB HugePage) \[x86_64\]](/blog/KVM-2M-on-BiscuitOS/)
>
>   - [KVM on BiscuitOS (1Gig HugePage) \[x86_64\]](/blog/KVM-1G-on-BiscuitOS/)
>
>   - [KVM with Special MMAP on BiscuitOS \[x86_64/i386\]](/blog/KVM-SMAP-on-BiscuitOS/)
>
>   - [QEMU-KVM on BiscuitOS (4KiB Page) \[x86_64\]](/blog/qemu-kvm-on-BiscuitOS/)
>
>   - [QEMU-KVM on BiscuitOS (2MiB HugePage) \[x86_64\]](/blog/qemu-kvm-2M-on-BiscuitOS/)
>
>   - [QEMU-KVM on BiscuitOS (1Gig HugePage) \[x86_64\]](/blog/qemu-kvm-1G-on-BiscuitOS/)
>
>   - [Native Linux KVM tool (kvmtool 4KiB Page) on BiscuitOS \[x86_64\]](/blog/kvmtool-on-BiscuitOS/)
>
>   - [Native Linux KVM tool (kvmtool 2MiB HugePage) on BiscuitOS \[x86_64\]](/blog/kvmtool-2M-on-BiscuitOS/)
>
>   - [Native Linux KVM tool (kvmtool 1Gig HugePage) on BiscuitOS \[x86_64\]](/blog/kvmtool-1G-on-BiscuitOS/)
>
> - KVM
>
> - [内存虚拟化](/blog/Memory-Virtualization)
>
>   - [EPT](/blog/KVM-EPT)
>
> - CPU 虚拟化
>
> - QEMU-KVM
>
>   - [QEMU Memory Manager for Virtual machine](/blog/QEMU-Memory-Manager-VM)
>
> - 设备虚拟化
>
>   - [新增一个 PCIe 设备](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/PCIe/BiscuitOS-pci-device-QEMU-emulate)
>
>   - [新增一个带 DMA 的 PCIe 设备](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/DMA/BiscuitOS-dma-device-QEMU-emulate)

![](/assets/PDB/GamePICBF000002.gif)

## [Basic Research]()

> - [Bit: bitmap/bitops/bitmask/bit find](/blog/BITMAP/)
>
> - [NUMA NODE/NODEMASK](/blog/NODEMASK/)
>
> - [GNU 链接脚本](/blog/LD/)
>
> - [Device Tree: DTS/DTB/FDT](/blog/DTS/)
>
> - [Language Practice on BiscuitOS](/blog/Language/)
>
>   - [C Language: GNU C/C99/C89 on BiscuitOS](/blog/Language/#A)
>  
>   - [C++ Language: GNU CPP/C++ on BiscuitOS](/blog/Language/#B)
>  
>   - [Python: Python2.7 on BiscuitOS](/blog/Language/#C)
>  
>   - [Shell scripts: ash/bash on BiscuitOS](/blog/Language/#D)
>  
>   - [X86-64 AT&T Assembly and inline Assembly on BiscuitOS](/blog/Language/#E)
>  
>   - [i386 AT&T Assembly and inline Assembly on BiscuitOS](/blog/Language/#F)
>  
>   - [ARM AT&T Assembly and inline Assembly on BiscuitOS](/blog/Language/#G)
>  
>   - [ARM64 AT&T Assembly and inline Assembly on BiscuitOS](/blog/Language/#H)
>
> - [汇编]()
>
>   - [X86/i386/X64 汇编]()
>
>   - [ARM64 汇编]()
>
>    - [ARM Assembly](/blog/MMU-ARM32-ASM-index/)
>    <span id="ARM_ASM"></span>
>   - [RISCV 汇编]()
>
>   - [内嵌汇编]()
>
> - 链表
>
>   - Single list
>
>   - [Bidirect list](/blog/LIST/)
>
> - 树
>
>   - [二叉树](/blog/Tree_binarytree/)
>
>   - [2-3-4 树](/blog/Tree_2-3-tree/)
>
>   - [红黑树](/blog/Tree_RBTree/)
>
>   - [Interval Tree](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Algorithem/tree/interval-tree)
>
>   - [Radix Tree 基数树](/blog/RADIX-TREE/)
>
> - [IDR: ID Radix](/blog/IDR/)
>
> - [IDA: ID allocator](/blog/IDA/)
>
> - 锁/同步
>
>   - [atomic 原子操作](/blog/ATOMIC/)

![](/assets/PDB/GamePICBF000000.gif)

## [Linux Source Code list]()

> - [Linux 5.0 源码](/blog/SC-5.0/)
>
> - [Linux 5.x 函数列表](/blog/SC-LIST-5.0/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

## <span id="Uboot">Bootloader</span>

> - Uboot
>
>   - [Build BiscuitOS uboot-2020.04](/blog/uboot-2020.04-Usermanual)
>
>   - [Build BiscuitOS uboot-2019.07](/blog/uboot-2019.07-Usermanual)
>
>   - [Build BiscuitOS uboot-2018.03](/blog/uboot-2018.03-Usermanual)
>
>   - [Build BiscuitOS uboot-2017.01](/blog/uboot-2017.01-Usermanual)
>
>   - [Build BiscuitOS uboot-2016.09](/blog/uboot-2016.09-Usermanual)
>
>   - [Build BiscuitOS uboot-2015.07](/blog/uboot-2015.07-Usermanual)
>
>   - [Build BiscuitOS uboot-2014.07](/blog/uboot-2014.07-Usermanual)
>
>   - [更多 BiscuitOS uboot 版本](/blog/Kernel_Build/#BiscuitOS_uboot)
>
> - BIOS/seaBIOS
>
>   - [Build BiscuitOS seaBIOS](/blog/seaBIOS-Usermanual)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

## <span id="Architecture">Architecture</span>

> - [ARM](/blog/ARM-Catalogue-Image/)
>
> - [ARM64]()
>
> - [X86]()
>
> - [RISCV-32]()
>
> - [RISCV-64]()

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

## <span id="Enginerring">Enginerring Practice</span>

> - [I2C](/blog/I2CBus/)
>
> - [PCIe](/blog/PCIe/)
>
> - [CMA](/blog/CMA/)
>
> - [DMA](/blog/DMA/)
>
> - [MDIO/SMI/MIIM](/blog/MDIO/)
>
>   - [MDIO](/blog/MDIO/)
>
>   - [SMI](/blog/MDIO/)
>
>   - [MIIM](/blog/MDIO/)
>
> - [SPI](/blog/SPI/)
>
> - [CAN](/blog/CAN/)
>
> - [DTS](/blog/DTS/)

<span id="RaspberryPi"></span>
![](/assets/PDB/RPI/RPI000000.png)

## Hardware Platform Practice

> - [Build BiscuitOS on RaspberryPi 4B](/blog/RaspberryPi_4B-Usermanual/)
>
> - [Build BiscuitOS on RaspberryPi 3B](/blog/RaspberryPi_3B-Usermanual/)
>
> - I2C
>
>   - [EEPROM AT24C08](/blog/LDD_I2C_AT24C08/)
>
>   - [Temperature LM75A](/blog/LDD_I2C_LM75A/)
>
>   - [I/O Expander FPC8574](/blog/LDD_I2C_PCF8574AT/)
>
>   - [A/D D/A Coverter PCF8591](/blog/LDD_I2C_PCF8591/)
>
> - SPI
>
> - 1-wire
>
> - CAN
>
> - Input
>
> - Platform
>
> - PWM

![](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/GIF000210.gif)

## <span id="Userland">Userland Develop</span>

> - [应用程序移植](/blog/APP-Usermanual/)
>
> - [GNU 项目移植](/blog/APP-Usermanual/)
>
> - [动态库移植](/blog/APP-Usermanual/)
>
> - [静态块移植](/blog/APP-Usermanual/)
>
> - [BiscuitOS 项目移植](/blog/APP-Usermanual/)
>
> - [游戏移植](/blog/APP-Usermanual/)
>
>   - [Snake 贪吃蛇](/blog/USER_snake/)
>
>   - [2048](/blog/USER_2048/)
>
>   - [tetris 俄罗斯方块](/blog/USER_tetris/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

## <span id="Debug">Debug Tools and Usermanual </span>

> - [Linux 5.x ARM32 Debug Usermanual](/blog/BOOTASM-debuggingTools/)
>
> - [Linux 0.1x i386 Debug Usermanual](/blog/Linux-0.11-Usermanual/#G)
>
> - [Linux 内核开发工具合集](/blog/Linux-development-tools/)
>
> - [perf: Linux 性能测试调试工具](/blog/TOOLS-perf/)
>
> - [blktrace: Block IO 性能测试调试工具](/blog/TOOLS-blktrace/)
>
> - [Linux 内核源码静态分析工具 sparse](/blog/SPARSE/#header)
>
> - [系统调用调试建议](/blog/SYSCALL_DEBUG/)
>
> - [通过 Kernel 历史树实践内核建议](/blog/Kernel_History/#header)
>
> - [内核核心转储: Kdump with kexec and crash](/blog/CRASH)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

## <span id="OpenProject">BiscuitOS 社区开源项目</span>

> - [Open VnetCard](/blog/VnetCard/)
>
> - [BiscuitOS 社区 "一带一路" 计划](/blog/Common2Feature/)
>
> - [BiscuitOS 社区 "人类知识共同体" 计划](/blog/Human-Knowledge-Common/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

## My Hobbies

> - [ASCII Animation](/blog/Animator/)
>
> - [小饼干，你看起来很好吃](/blog/Story_SmallBiscuitOS/)

---------------------------------------------

## Donation 🙂

![](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
