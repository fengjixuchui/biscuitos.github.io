---
layout: post
title:  "PCI/PCIe Address Space"
date:   2022-04-01 12:00:00 +0800
categories: [HW]
excerpt: PCI Memory Sapce.
tags:
  - Memory
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [PCI 基础知识](#A)
>
>   - [PCI Type0 Header Mapping](#D0)
>
>   - [PCI Type1 Header Mapping](#D1)
>
> - [PCIe 基础知识](#A)
>
>   - [PCIe Type0 Header Mapping](#D0)
>
>   - [PCIe Type1 Header Mapping](#D1)
>
> - [PCI 地址空间](#B)


######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### PCI 基础知识

![](/assets/PDB/HK/TH001468.JPEG)

**PCI**: Peripheral Component Interconnect(外部设备互联标志) 是由 PCISIG(PCI Special Interest Group) 推出的一种局部并行总线标准，其由 **ISA**(Industy Standard Architecture) 总线发展而来，是一种同步的独立于处理器的 32/64 位局部总线。从结构上看，PCI 是在 CPU 原来的系统总线之间插入的一级总线，具体由一个桥接电路实现对该层的管理，并实现上下之间的接口以协调数据的传送。PCI 总线是一种树型结构且独立于 CPU 总线，与 CPU 总线并行操作，PCI 总线上可以挂接 PCI 设备和 PCI 桥，PCI 总线上同一时刻只允许有一个 PCI 主设备，其他均为 PCI 从设备，而且读写操作只能在主从设备之间进行，从设备之间数据交换需要通过主设备中转。

![](/assets/PDB/HK/TH001467.png)

一个典型的 PCI 总线系统如上图的以 Intel 440FX PMC(PCI and Memory Controller) 为北桥芯片，PIIX (PCI ISA Xcelerator) 为南桥芯片组成的芯片组。北桥芯片 PMC 用于连接主板上的高速设备，向上提供了连接处理器的 Host 总线接口，可以连接多个处理器，向下主要提供了连接内存 DRAM 的接口和连接 PCI 总线系统的 PCI 总线接口，通过该 PCI root port 扩展出整个 PCI 设备树，包括 PIIX 南桥芯片. PIIX 南桥芯片则用于连接主板上的低速设备，主要包括 IDE 控制器、DMA 控制器、硬盘、USB 控制器、SMBus 总线控制器，并且提供了 ISA 总线用于连接更多的低速设备，如键盘、鼠标、BIOS ROM 等.

![](/assets/PDB/HK/TH001471.png)

设备标识符可以看做 PCI 设备在 PCI 总线上的地址，其结构如上图所示。其 MSB 8bit 的 Bus 域表示设备所在的总线号，系统最多支持 256 条总线。5bit 的 Device 域表示设备号，代表所在 Bus 总线的一个设备。3bit 的 Function 域表示功能号，标识具体设备上的某个功能单元(逻辑设备)，一个独立的 PCI 设备最多有 8 个功能单元。Device 和 Function 一般联合起来使用，表示一条总线上一个最多 256 个设备。通常 PCI 设备的标识符三个字段的缩写为 BDF.

![](/assets/PDB/HK/TH001470.png)

PCI 总线采用的是一种深度优先(Depth First Search) 的拓扑算法，且 Bus0 总分配给 Root Complex. Root 中包含有集成的 Endpoint 和多个端口(Port), 每个端口内部都有一个虚拟的 PCI-to-PCI(P2P) 桥, 并且该桥也有设备号和功能号. 需要注意的是每个设备必须要有 Function0, 其他的 7 个功能可选. 每个 PCI 功能 (Function) 都包含 256 字节的配置空间 (Configuration Space), 其中前 64 字节称为 Header, 剩余 192 字节用于一些可选功能。PCI Spec 规定了两种类型的 Header: Type0 和 Type1，其中 Type0 Header 表示 PCI 设备功能不是桥, Type1 Header 表示 PCI 设备功能是桥。

<span id="D0"></span>
![](/assets/PDB/HK/TH001472.png)

**Base Address Register**(基地址寄存器): 也就是通常所说的 PCI BAR，它用于报告寄存器或者设备 RAM 在 I/O 端口地址空间或者物理地址空间的地址。地址是由 BIOS 或操作系统动态配置。Type0 中包含了 6 个 BAR。

**Interrupt Pin**(中断引脚): PCI 中断线的标准设计是 4 条: INTA/INTB/INTC/INTD. 

**Interrupt Line**(设备中断线): 该寄存器只起一个保护作用.

<span id="D1"></span>
![](/assets/PDB/HK/TH001473.png)

**Base Address Register**(基地址寄存器) 也就是通常所说的 PCI BAR，它用于报告寄存器或者设备 RAM 在 I/O 端口地址空间或者物理地址空间的地址。地址是由 BIOS 或操作系统动态配置。Type0 中包含了 2 个 BAR。

**Interrupt Pin**(中断引脚): PCI 中断线的标准设计是 4 条: INTA/INTB/INTC/INTD. 

**Interrupt Line**(设备中断线): 该寄存器只起一个保护作用.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### PCI 地址空间

![](/assets/PDB/HK/TH001474.png)

每一个 PCI 设备功能包含 256 字节的配置空间，如果将所有的 PCI 设备功能的配置空间集合在一起，那么称为 **PCI 配置空间**(**PCI Configuration Space**), 并使用 BDF 进行寻址。由于每个 PCI 设备最多支持 8 中功能 (Function), 每一条 PCI 总线最多支持 32 个设备，每个 PCI 总线系统最多支持 256 个子总线，那么 PCI Configuration Space 的大小为: 256 (Bytes/Function) * 8 (Functioins/device) * 32 (device/Bus) * 256 (buses/system) = 16MiB。

![](/assets/PDB/HK/TH001475.png)

在 X86 架构中，地址空间被划分为内存空间和 I/O 空间，在 PCI 总线上，X86 只能通过 IO 端口方式才能访问 PCI Configuration Space。由于 X86 I/O 地址空间有限 (64KiB), 所以一般在 I/O Space 中都包含两个寄存器，一个指向要操作的内部地址，第二个存放读或写的数据。因此对于 PCI 周期来说，包含了两个步骤: 首先 CPU 对 PCI Address Port 的 \[0xCF8, 0xCFB] 写入要操作的的寄存器地址，其中包括了总线号(Bus Numer)、设备号(Device Number)、功能号(Function Number) 和寄存器指针; 接着 CPU 向 PCI Data Port 的 \[0xCFC, 0xCFF] 中写入读或写的数据.

![](/assets/PDB/HK/TH001468.png)

PCI 总线具有 32/64 位数据/地址复用总线，因此 PCI 地址空间可寻址 4GiB/16EiB 的空间，也就是 PCI 上所有设备共同映射到这 4GiB/16EiB 上. 每个 PCI 设备内部可能包含 Memory 和 IO，因此每个 PCI 设备占用 PCI 总线唯一一地址，以便 PCI 总线统一寻址。每个 PCI 设备通过 PCI 寄存器中的基地址寄存器 (Base Address Register) 来指定映射的首地址。

![](/assets/PDB/HK/TH001476.png)

在早期 PC 中，除内存之外的设备都称为 IO 设备或者外设，IO 设备内部也会包含存储或者寄存器，其只能通过 IO 地址空间进行访问。X86 架构中 I/O 地址空间非常有限只有 16KiB，通过 IO 空间访问外设的方式局限性太大，且效率低，于是提出了一个新的技术 MMIO. **MMIO**(Memory Mapped IO): 将 IO 设备内部的存储和寄存器统一映射到系统的地址空间。PCI 规范也支持将 PCI 设备的存储或寄存器映射到系统 IO 空间。从处理器角度来看，IO 设备内部空间射到系统地址空间之后，CPU 就可以像访问内存一样访问 IO 设备内部空间。MMIO 虽好但会消耗系统地址空间的，导致内存的可用空间变小。

![](/assets/PDB/HK/TH001469.png)

在支持 PCI 总线的架构中，一共存在四种地址空间: Memory Address Space(系统内存地址空间)、I/O Address Space(I/O 端口空间)、PCI Address Space(PCI 设备地址空间) 和 PCI Configuration Address Space(PCI 配置空间). 在 X86 架构中可以直接访问的只有 Memory Address Space 和 I/O Address Space. 

