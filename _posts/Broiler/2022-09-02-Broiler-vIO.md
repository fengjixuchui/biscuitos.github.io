---
layout: post
title:  "Broiler IO Virtualization Technology"
date:   2022-09-02 12:00:00 +0800
categories: [MMU]
excerpt: Virtualization.
tags:
  - HypV
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [X86 IO 介绍](#A)
>
> - [X86 IO 虚拟化](#G)
>
> - [同步 IO 端口虚拟化](#B)
>
> - [异步 IO 端口虚拟化](#C)
>
> - [同步 MMIO 虚拟化](#D)
>
> - [异步 MMIO 虚拟化](#E)
>
> - [Broiler IO 虚拟化](#F)
>
> - Broiler IO 虚拟化使用

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### X86 IO 介绍

![](/assets/PDB/HK/TH001481.png)

在 Intel X86 架构中，系统物理地址总线构成的空间称为存储域，其主要由三部分组成，一部分是 DDR 控制器管理的 DDR 域，另外一部分是 PCI 总线域映射到存储域的 PCI 总线地址，最后一部分是设备内部存储空间映射到存储域的区域，这里统一将后者统称为 MMIO。Intel X86 架构也通过 Host PCI 主桥维护一颗 PCI 总线，该 PCI 总线构成的空间称为PCI 总线域。

![](/assets/PDB/HK/TH001695.png)

在 X86 架构中，存储域的空间长度和 PCI 总线域的长度是相同的，并且 DDR 域的地址可以映射到存储域，也可以映射到 PCI 总线域，同理 PCI 域的地址也可以映射到存储域，且该地址在存储域上称为 PCI 总线地址。

![](/assets/PDB/HK/TH001711.png)

在 Broiler 模拟的 X86 架构中，DDR 映射范围被划分做两段，第一段是 [0x00000000, 0xC0000000), 大小为 2Gig 内存, 第二段从 0x100000000 开始. PCI 域映射到存储域的范围是 [0xC0000000, 0x100000000), 该区域包括了 PCI 总线地址，同时也包括了其他外设存储空间映射的区域，例如 Local APIC 映射的 MMIO 区域.

![](/assets/PDB/HK/TH001713.png)

上图是一个 Broiler 运行的 4Gig 虚拟机，通过 “/proc/iomem” 接口可以看到系统存储域的布局，可以看到 System RAM 占用的范围是 [0x00100000, 0xbfffffff], 以及 [0x100000000, 0x13fffffff]. 另外可以看到 PCI 的 MMIO 从 0xC2000000 开始. 这个布局符合 Broiler 的规划.

![](/assets/PDB/HK/TH001715.png)

X86 架构中存在存储域和 IO 地址空间，IO 地址空间用于映射外设的寄存器，并通过 IN/OUT 指令进行访问。在有的架构中 IO 地址空间位于存储域空间，可以使用统一的地址进行访问，但在 X86 架构中两者在不同的空间，需要使用不同的指令进行访问. 本文将 IO 端口和 MMIO 统称为 IO, 因为两者都是用来访问非 DDR 空间地址.

![](/assets/PDB/HK/TH001871.png)

当系统向外设发起 IO 操作(Read/Write) 时，如果 CPU 需要等待 IO 完成之后再继续执行其他指令，那么称这种 IO 为**同步 IO(Synchronous IO)**; 如果 CPU 不需要等待 IO 完成就可以继续执行其他指令，当外设处理完 IO 之后再通过中断告诉 CPU IO 已经处理完成，那么称这种 IO 为**异步 IO(Asynchronous IO)**. 同步 IO 操作譬如对 IO Port 的读写操作，异步 IO 操作譬如 Doorball/Kick Register 等.



