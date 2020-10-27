---
layout: post
title:  ""
date:   2018-12-10 09:13:33 +0800
categories: [Top]
excerpt: Catalogue.
tags:
---

![](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/GIF000204.gif)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

## <span id="Kernel_Establish">Kernel Establish</span>

> - [New!! BiscuitOS-Desktop GOKU](https://biscuitos.github.io/blog/Desktop-GOKU-Usermanual/)
>
> - [New!! Build BiscuitOS-Desktop](https://biscuitos.github.io/blog/BiscuitOS-Desktop-arm32-Usermanual/)
>
> - [Build Upstream Linux from Linus Torvalds](https://biscuitos.github.io/blog/Kernel_Build/#Linux_newest)
>
> - [Build Linux 5.9 - RISCV32/RISCV64/ARM32/ARM64/I386/X86_64](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5Y)
>
> - [Build Linux 0.X](https://biscuitos.github.io/blog/Kernel_Build/#Linux_0X)
>
> - [Build Linux 1.X](https://biscuitos.github.io/blog/Kernel_Build/#Linux_1X)
>
> - [Build Linux 2.X](https://biscuitos.github.io/blog/Kernel_Build/#Linux_2X)
>
> - [Build Linux 3.X](https://biscuitos.github.io/blog/Kernel_Build/#Linux_3X)
>
> - [Build Linux 4.X](https://biscuitos.github.io/blog/Kernel_Build/#Linux_4X)
>
> - [Build Linux 5.X](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)
>
> - [Networking on BiscuitOS](https://biscuitos.github.io/blog/Networking-Usermanual/)
>
> - Other Operation System
>
>   - [Build Debian-0.91 on BiscuitOS](https://biscuitos.github.io/blog/Debian-0.91-Usermanual)
>   
>   - [Build Apollo-11 on BiscuitOS](https://biscuitos.github.io/blog/Apollo-11-Usermanual)
>
>   - [Build MIT XV6 on BiscuitOS](https://biscuitos.github.io/blog/XV6-REV11-Usermanual)
>
> - [BiscuitOS on RaspberryPi](#RaspberryPi)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## [内存管理]()

> - [Linux MMU History Project](https://biscuitos.github.io/blog/HISTORY-MMU/)
>
> - [自己动手设计一个内存管理子系统](https://biscuitos.github.io/blog/Design-MMU/)
>
> - Detecting Memory
>
>   - [Detecting Memory from CMOS \[X86\]](https://biscuitos.github.io/blog/MMU-seaBIOS_E820/#E1)
>
>   - [Detecting Memory from BDA \[X86\]](https://biscuitos.github.io/blog/MMU-seaBIOS_E820/#E2)
>
>   - [Detecting Memory from BIOS IVT \[X86\]](https://biscuitos.github.io/blog/MMU-E820/#F1)
>
>   - [E820 内存管理器 \[X86\]](https://biscuitos.github.io/blog/MMU-E820/)
>
> - Boot-Time Allocator
>
>   - [.bss .data Allocator on Static Compile Stage]()
>
>   - [RESERVE_BRK Allocator \[x86\]](https://biscuitos.github.io/blog/RESERVE_BRK/)
>
>   - [Early_res Allocator \[x86\]]()
>
>   - [Bootmem Allocator](https://biscuitos.github.io/blog/HISTORY-bootmem/)
>
>   - [MEMBLOCK Allocator](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-index/)
>
> - Physical Memory Manager
>
>   - [NUMA]()
>
>   - [UMA]()
>
>   - [ZONE]()
>
>   - [SPARSE Memory Model]()
>
>   - [FLAT Memory Model]()
>
>   - [Physical Page/Frame]()
>
>   - [Buddy Memory Allocator](https://biscuitos.github.io/blog/HISTORY-buddy/)
>
>   - [PCP Memory Allocator](https://biscuitos.github.io/blog/HISTORY-PCP/)
>
> - [PERCPU Memory Allocator](https://biscuitos.github.io/blog/HISTORY-PERCPU/)
>
> - SLAB/KMEM Allocator
>
>   - [SLAB Memory Allocator](https://biscuitos.github.io/blog/HISTORY-SLAB/)
>
>   - [SLUB Memory Allocator]()
>
>   - [SLOB Memory Allocator]()
>
> - [VMALLOC Memory Allocator](https://biscuitos.github.io/blog/HISTORY-VMALLOC/)
>
> - [KMAP Memory Allocator](https://biscuitos.github.io/blog/HISTORY-KMAP/)
>
> - [Fixmap Memory Allcator](https://biscuitos.github.io/blog/HISTORY-FIXMAP/)
>
> - [CMA: Contiguous Memory Allocator](https://biscuitos.github.io/blog/CMA)
>
> - [用户空间实现 Linux 内核分配器](https://biscuitos.github.io/blog/Memory-Userspace/)
>
>   - [MEMBLOCAK Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#B)
>
>   - [PERCPU(UP) Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#C)
>
>   - [PERCPU(SMP) Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#D)
>
>   - [Buddy-Normal Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#E)
>
>   - [Buddy-HighMEM Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#F)
>
>   - [PCP Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#G)
>
>   - [Slub Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#H)
>
>   - [Kmem_cache Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#J)
>
>   - [Kmalloc Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#K)
>
>   - [NAME Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#L)
>
>   - [VMALLOC Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#M)
>
>   - [KMAP Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#N)
>
>   - [Fixmap Memory Allocator](https://biscuitos.github.io/blog/Memory-Userspace/#P)
>
> - Memory Maps
>
>   - [Real Mode Address Space (\< 1 MiB)](https://biscuitos.github.io/blog/MMU-seaBIOS_E820/#B0003)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000108.png)

## [虚拟文件系统]()

> - [系统调用](https://biscuitos.github.io/blog/SYSCALL)
>
>   - [ARM32 架构中添加新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_ARM/)
>
>   - [ARM64 架构中添加新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_ARM64/)
>
>   - [i386 架构中添加新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_I386/)
>
>   - [X86_64 架构中添新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_X86_64/)
>
>   - [RISCV32 架构中添新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_RISCV32/)
>
>   - [RISCV64 架构中添加新的系统调用](https://biscuitos.github.io/blog/SYSCALL_ADD_NEW_RISCV64/)
>
>   - [sys_open](https://biscuitos.github.io/blog/SYSCALL_sys_open/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## [虚拟化]()

> - [KVM on BiscuitOS \[x86_64/i386\]](https://biscuitos.github.io/blog/KVM-on-BiscuitOS/)
>
> - KVM
>
> - 内存虚拟化
>
> - CPU 虚拟化

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## [Basic Research]()

> - [Bit: bitmap/bitops/bitmask/bit find](https://biscuitos.github.io/blog/BITMAP/)
>
> - [GNU 链接脚本](https://biscuitos.github.io/blog/LD/)
>
> - [Device Tree: DTS/DTB/FDT](https://biscuitos.github.io/blog/DTS/)
>
> - [汇编]()
>
>   - [X86/i386/X64 汇编]()
>
>   - [ARM64 汇编]()
>
>    - [ARM Assembly](https://biscuitos.github.io/blog/MMU-ARM32-ASM-index/)
>    <span id="ARM_ASM"></span>
>   - [RISCV 汇编]()
>
>   - [内嵌汇编]()
>
> - 链表
>
>   - Single list
>
>   - [Bidirect list](https://biscuitos.github.io/blog/LIST/)
>
> - 树
>
>   - [二叉树](https://biscuitos.github.io/blog/Tree_binarytree/)
>
>   - [2-3-4 树](https://biscuitos.github.io/blog/Tree_2-3-tree/)
>
>   - [红黑树](https://biscuitos.github.io/blog/Tree_RBTree/)
>
>   - [Radix Tree 基数树](https://biscuitos.github.io/blog/RADIX-TREE/)
>
> - [IDR: ID Radix](https://biscuitos.github.io/blog/IDR/)
>
> - [IDA: ID allocator](https://biscuitos.github.io/blog/IDA/)
>
> - 锁/同步
>
>   - [atomic 原子操作](https://biscuitos.github.io/blog/ATOMIC/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## [Linux Source Code list]()

> - [Linux 5.0 源码](https://biscuitos.github.io/blog/SC-5.0/)
>
> - [Linux 5.x 函数列表](https://biscuitos.github.io/blog/SC-LIST-5.0/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## <span id="Uboot">Bootloader</span>

> - Uboot
>
>   - [Build BiscuitOS uboot-2020.04](https://biscuitos.github.io/blog/uboot-2020.04-Usermanual)
>
>   - [Build BiscuitOS uboot-2019.07](https://biscuitos.github.io/blog/uboot-2019.07-Usermanual)
>
>   - [Build BiscuitOS uboot-2018.03](https://biscuitos.github.io/blog/uboot-2018.03-Usermanual)
>
>   - [Build BiscuitOS uboot-2017.01](https://biscuitos.github.io/blog/uboot-2017.01-Usermanual)
>
>   - [Build BiscuitOS uboot-2016.09](https://biscuitos.github.io/blog/uboot-2016.09-Usermanual)
>
>   - [Build BiscuitOS uboot-2015.07](https://biscuitos.github.io/blog/uboot-2015.07-Usermanual)
>
>   - [Build BiscuitOS uboot-2014.07](https://biscuitos.github.io/blog/uboot-2014.07-Usermanual)
>
>   - [更多 BiscuitOS uboot 版本](https://biscuitos.github.io/blog/Kernel_Build/#BiscuitOS_uboot)
>
> - BIOS/seaBIOS
>
>   - [Build BiscuitOS seaBIOS](https://biscuitos.github.io/blog/seaBIOS-Usermanual)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## <span id="Architecture">Architecture</span>

> - [ARM](https://biscuitos.github.io/blog/ARM-Catalogue-Image/)
>
> - [ARM64]()
>
> - [X86]()
>
> - [RISCV-32]()
>
> - [RISCV-64]()

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## <span id="Enginerring">Enginerring Practice</span>

> - [I2C](https://biscuitos.github.io/blog/I2CBus/)
>
> - [PCIe](https://biscuitos.github.io/blog/PCIe/)
>
> - [CMA](https://biscuitos.github.io/blog/CMA/)
>
> - [DMA](https://biscuitos.github.io/blog/DMA/)
>
> - [MDIO/SMI/MIIM](https://biscuitos.github.io/blog/MDIO/)
>
>   - [MDIO](https://biscuitos.github.io/blog/MDIO/)
>
>   - [SMI](https://biscuitos.github.io/blog/MDIO/)
>
>   - [MIIM](https://biscuitos.github.io/blog/MDIO/)
>
> - [SPI](https://biscuitos.github.io/blog/SPI/)
>
> - [CAN](https://biscuitos.github.io/blog/CAN/)
>
> - [DTS](https://biscuitos.github.io/blog/DTS/)

<span id="RaspberryPi"></span>
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000000.png)

## Hardware Platform Practice

> - [Build BiscuitOS on RaspberryPi 4B](https://biscuitos.github.io/blog/RaspberryPi_4B-Usermanual/)
>
> - [Build BiscuitOS on RaspberryPi 3B](https://biscuitos.github.io/blog/RaspberryPi_3B-Usermanual/)
>
> - I2C
>
>   - [EEPROM AT24C08](https://biscuitos.github.io/blog/LDD_I2C_AT24C08/)
>
>   - [Temperature LM75A](https://biscuitos.github.io/blog/LDD_I2C_LM75A/)
>
>   - [I/O Expander FPC8574](https://biscuitos.github.io/blog/LDD_I2C_PCF8574AT/)
>
>   - [A/D D/A Coverter PCF8591](https://biscuitos.github.io/blog/LDD_I2C_PCF8591/)
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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## <span id="Userland">Userland Develop</span>

> - [应用程序移植](https://biscuitos.github.io/blog/APP-Usermanual/)
>
> - [GNU 项目移植](https://biscuitos.github.io/blog/APP-Usermanual/)
>
> - [动态库移植](https://biscuitos.github.io/blog/APP-Usermanual/)
>
> - [静态块移植](https://biscuitos.github.io/blog/APP-Usermanual/)
>
> - [BiscuitOS 项目移植](https://biscuitos.github.io/blog/APP-Usermanual/)
>
> - [游戏移植](https://biscuitos.github.io/blog/APP-Usermanual/)
>
>   - [Snake 贪吃蛇](https://biscuitos.github.io/blog/USER_snake/)
>
>   - [2048](https://biscuitos.github.io/blog/USER_2048/)
>
>   - [tetris 俄罗斯方块](https://biscuitos.github.io/blog/USER_tetris/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## <span id="Debug">Debug Tools and Usermanual </span>

> - [Linux 5.x ARM32 Debug Usermanual](https://biscuitos.github.io/blog/BOOTASM-debuggingTools/)
>
> - [Linux 0.1x i386 Debug Usermanual](https://biscuitos.github.io/blog/Linux-0.11-Usermanual/#G)
>
> - [Linux 内核开发工具合集](https://biscuitos.github.io/blog/Linux-development-tools/)
>
> - [perf: Linux 性能测试调试工具](https://biscuitos.github.io/blog/TOOLS-perf/)
>
> - [blktrace: Block IO 性能测试调试工具](https://biscuitos.github.io/blog/TOOLS-blktrace/)
>
> - [Linux 内核源码静态分析工具 sparse](https://biscuitos.github.io/blog/SPARSE/#header)
>
> - [系统调用调试建议](https://biscuitos.github.io/blog/SYSCALL_DEBUG/)
>
> - [通过 Kernel 历史树实践内核建议](https://biscuitos.github.io/blog/Kernel_History/#header)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## <span id="OpenProject">BiscuitOS 社区开源项目</span>

> - [Open VnetCard](https://biscuitos.github.io/blog/VnetCard/)
>
> - [BiscuitOS 社区互助计划](https://biscuitos.github.io/blog/BiscuitOS_JD/)
>
> - [BiscuitOS 社区 "一带一路" 计划](https://biscuitos.github.io/blog/Common2Feature/)
>
> - [BiscuitOS 社区 "人类知识共同体" 计划](https://biscuitos.github.io/blog/Human-Knowledge-Common/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

## My Hobbies

> - [ASCII Animation](https://biscuitos.github.io/blog/Animator/)
>
> - [小饼干，你看起来很好吃](https://biscuitos.github.io/blog/Story_SmallBiscuitOS/)

---------------------------------------------

## Donation 🙂

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
