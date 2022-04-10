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

> - [PCI/PCIe 基础知识](#A)
>
> - [PCI/PCIe 地址空间](#B)
>
> - [PCI/PCIe 实践攻略](#D)
>
> - [PCI/PCIe 工具手册](#E)
>
> - [PCI/PCIe 使用攻略](#F)
>
> - [PCI/PCIe 设备驱动](#G)
>
> - [PCI/PCIe DMA](#H)
>
> - PCI/PCIe 进阶研究
>
>   - [PCI Type0 Header Mapping](#D0)
>
>   - [PCI Type1 Header Mapping](#D1)
>
>   - [PCI 总线枚举及 BDF 分配](#D3)
>
>   - [PCI BAR 初始化](#D4)
>
>   - [PCI 总线地址与系统总线映射](#D5)
>
>   - BIOS 枚举 PCI 总线研究
>
>   - QEMU 与 PCI 内存研究
>
>   - QEMU PCI 总线设备模拟研究
>
>   - QEMU DMA 内存搬运研究

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### PCI/PCIe 基础知识

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

#### PCI/PCIe 地址空间

![](/assets/PDB/HK/TH001474.png)

每一个 PCI 设备功能包含 256 字节的配置空间，如果将所有的 PCI 设备功能的配置空间集合在一起，那么称为 **PCI 配置空间**(**PCI Configuration Space**), 并使用 BDF 进行寻址。由于每个 PCI 设备最多支持 8 中功能 (Function), 每一条 PCI 总线最多支持 32 个设备，每个 PCI 总线系统最多支持 256 个子总线，那么 PCI Configuration Space 的大小为: 256 (Bytes/Function) * 8 (Functioins/device) * 32 (device/Bus) * 256 (buses/system) = 16MiB。

![](/assets/PDB/HK/TH001475.png)

在 X86 架构中，地址空间被划分为内存空间和 I/O 空间，在 PCI 总线上，X86 只能通过 IO 端口方式才能访问 PCI Configuration Space。由于 X86 I/O 地址空间有限 (64KiB), 所以一般在 I/O Space 中都包含两个寄存器，一个指向要操作的内部地址，第二个存放读或写的数据。因此对于 PCI 周期来说，包含了两个步骤: 首先 CPU 对 PCI Address Port 的 \[0xCF8, 0xCFB] 写入要操作的的寄存器地址，其中包括了总线号(Bus Numer)、设备号(Device Number)、功能号(Function Number) 和寄存器指针; 接着 CPU 向 PCI Data Port 的 \[0xCFC, 0xCFF] 中写入读或写的数据.

![](/assets/PDB/HK/TH001468.png)

PCI 总线具有 32/64 位数据/地址复用总线，因此 PCI 地址空间可寻址 4GiB/16EiB 的空间，也就是 PCI 上所有设备共同映射到这 4GiB/16EiB 上. 每个 PCI 设备内部可能包含 Memory 和 IO，因此每个 PCI 设备占用 PCI 总线唯一一地址，以便 PCI 总线统一寻址。每个 PCI 设备通过 PCI 寄存器中的基地址寄存器 (Base Address Register) 来指定映射的首地址。

![](/assets/PDB/HK/TH001476.png)

在早期 PC 中，除内存之外的设备都称为 IO 设备或者外设，IO 设备内部也会包含存储或者寄存器，其只能通过 IO 地址空间进行访问。X86 架构中 I/O 地址空间非常有限只有 16KiB，通过 IO 空间访问外设的方式局限性太大，且效率低，于是提出了一个新的技术 MMIO. **MMIO**(Memory Mapped IO): 将 IO 设备内部的存储和寄存器统一映射到系统的地址空间。PCI 规范也支持将 PCI 设备的存储或寄存器映射到系统 IO 空间。从处理器角度来看，IO 设备内部空间射到系统地址空间之后，CPU 就可以像访问内存一样访问 IO 设备内部空间。MMIO 虽好但会消耗系统地址空间的，导致内存的可用空间变小。

![](/assets/PDB/HK/TH001477.png)

当 PCI 设备通过 MMIO 机制将内部的存储和 IO 映射到系统地址总线之后，CPU 是不能直接访问这些地址的，CPU 只能直接访问物理内存和 IO 空间，那么此时需要 Root Complex(RC) 的协助。当 CPU 需要读取 PCI 设备的 MMIO 地址时，CPU 就会让 RC 通过 TLP 把数据从 PCI 外设读到内存，然后 CPU 从内存读取数据; 如果 CPU 需要向 PCI 设备的 MMIO 地址写入数据，则先将数据在内存中准备好，然后让 RC 通过 TLP 写入到 PCI 设备. 另外 RC 还会判断 CPU 发出的物理地址是内存地址还是 PCI 映射的 MMIO 地址，如果发现物理地址是物理内存，那么直接访问物理内存, 反之如果检测到物理地址是 PCI 设备的 MMIO 地址，那么 RC 就会触发生产 TLP 去访问对应的 PCIe 设备，读取或写入 PCIe 设备. 

![](/assets/PDB/HK/TH001469.png)

通过上面的讨论，在支持 PCI/PCIe 总线的架构中，一共存在四种地址空间: Memory Address Space(系统内存地址空间)、I/O Address Space(I/O 端口空间)、PCI Address Space(PCI 设备地址空间) 和 PCI Configuration Address Space(PCI 配置空间). 在 X86 架构中可以直接访问的只有 Memory Address Space 和 I/O Address Space. PCI Configuration Address Space 需要系统通过指定的 IO port 才能访问，而 PCI Address Space 需要通过 MMIO 机制映射到系统地址空间才能进行访问.

![](/assets/PDB/HK/TH001478.png)

系统地址空间被划分成很多过区域，其中包括内存的 Extend Memory, 以及 PCI 设备映射的 PCI Memory 区域，PCI 地址空间上的 PCI 设备通过 MMIO 机制，将其内部存储和 IO 映射到系统地址空间的 PCI Memory 区域或者 I/O 空间。那么 PCI 设备的 MMIO 地址是如何分配呢?

<span id="D3"></span>
###### PCI 总线枚举及 BDF 分配

![](/assets/PDB/HK/TH001481.png)

系统上电之后对于 CPU 来说，最开始仅仅知道 PCI Bus 0 的存在，至于 PCI Bus 0 上有什么设备一改不知，此时需要对 PCI 总线进行枚举，以此探明每条总线的情况. PCI 总线架构最多支持 256 根总线，每根总线上可以挂 32 个设备，每个设备内部可以支持 8 种功能。PCI 标准为每个 PCI 设备或着 PCI 桥使用一个 BDF 号进行标识，其 Device 和 Function 号由硬件产商和硬件拓扑结构确定, 因此 PCI 总线的枚举实际是分配总线号.

![](/assets/PDB/HK/TH001479.png)

Function number 用于多 Function PCI 设备用于区分设备中具体访问哪个 Function，对于大部分单 Function 的设备来说，Function Number 固定为 0. 对于多 Function 的设备或者支持虚拟 Function 的设备来说，Function Number 是由设备内部来进行管理和区分的，不需要通过总线枚举。上图是一个未进行枚举的 PCI 总线拓扑架构，可以看出 **PCI 设备 7** 和 **PCI 设备 2** 内部的 Function Number 已经由硬件厂商定义号了, 因此每个 PCI 设备的 Function Number 号如下:

![](/assets/PDB/HK/TH001480.png)

对于单 Function 的 PCI 设备和 PCI 桥的 Function Number 都是 0，而对于多 Function 的 PCI 设备，其每个 Function 都有一个 Function Number，并且 Function Number 已经由厂商定义好，例如 **PCI 设备 7** 包含了 3 个 Function，其 Function Number 分别是 0/4/5. 同理 **PCI 设备 2**包含了 2 个 Function，其 Function Number 分别是 0/1.

![](/assets/PDB/HK/TH001483.png)

为了支持 PCI ID 路由，每个 PCI 设备(EP或者Switch) 中有存储设备总线号和设备号的寄存器，复位时，该寄存器请 0. 上电复位之后，PCI 总线枚举一条总线上的 \[Device:Function] 时，RC 会对该 \[Device:Function] 发起一个配置访问事务, 设备在它的原级链路上检测到一个 Type0 的 config transaction 事务包时，它从该 TLP 头标中的 \[8:9] 字节获得总线号和设备号，以此作为自己的 BUS ID 和 Device ID 存储在设备的总线号寄存器和设备号寄存器中。

![](/assets/PDB/HK/TH001482.png)

对于 CPU 来说，最开始只知道 PCI Bus 0 的存在，那么首先读 \[Bus0, Dev0, Fun0] 的 DID&VID,此时 RC 会对其发起一个配置访问事务，如果返回值为 0xFFFF 表示设备不存在，那么接着读 \[Bus0, Dev1, Fun0] 的 DID&VID; 反之如果返回值不为 0xFFFF 表示设备存在，那么接下来继续读 \[Bus0, Dev0, Fun1] 的 DID&VID，以此类推。读取一个设备时必须按 Fun0 到 Fun7 的顺序读取，如果其中某个 Fun 不存在，那么就调到下一个 Device，这样就可以遍历该总线上的所有 \[Device,Function]。设备在原级链路上收到配置访问事务对应的 TLP 时，将获得 Bus ID 和 Device ID，并存储在设备的总线号寄存器和设备号寄存器，那么 PCI 节点的 BUS ID 和 Device ID 已经分配完毕，例如在上图的案例中，**PCI 桥1** 的 BUSID 为 0 且 DeviceID 也为 0，**PCI 设备1** 的 BUSID 为 0 且 DeviceID 为 3，**PCI 桥4** 的 BUSID 为 0 且 Device ID 为 6.

![](/assets/PDB/HK/TH001484.png)

\[Bus0, Dev0, Fun0] 存在，那么继续读其 Header 寄存器的值为 1，那么该节点是一个 PCI to PCI Bridge(PCI 桥)，在 PCI 规范中每个 PCI Bridge 对应一个 Bus，另外由于 PCI 总线枚举遵循深度优先法则，那么该 PCI Bridge 对应的 Bus 就是 PCI Bus 1，那么接下来就像 PCI Bus 0 一样遍历 PCI Bus 1 总线上所有的 \[Device,Function], 对于设备存在的时候，读取设备的 Header 寄存器以此判断是否为 Bridge，如果是 Bridge 就按深度优先的策略为其分配 BUS ID 并进入下一条总线进行扫描; 反之如果设备存在且为 PCI 端节点，那么只需配置 BusID 寄存器和 DeviceID 寄存器即可。对于 Bridge 需要配置其总线寄存器:

{% highlight bash %}
# PCI 桥1
Primary Bus Number = 0
Secondary Bus Number = 1
Subordinate Bus Number = 1
{% endhighlight %}

更新完该总线寄存器之后，每检查到一条新的 PCI Bus 之后，需要将上游所有的 Bridge 的 Subordinate Bus Number 加 1.

![](/assets/PDB/HK/TH001485.png) 

PCI 总线枚举的时候遵循深度优先的策略，分配完 **PCI 桥1** 下的所有子总线之后才会为 **PCI 桥4** 下的子总线进行分配 BUS ID。通过 PCI 总线枚举，每个存在的 PCI 设备或者 PCI 桥都分配了一个 BDF，那么可以通过该 BDF 找 PCI Configuration Space 中找 PCI 设备/桥对应的配置空间.

<span id="D4"></span>
###### PCI BAR 初始化

![](/assets/PDB/HK/TH001472.png)

每个 PCI 功能都包含 256 字节的配置空间(Configuration Space), 其前 64 字节称为 Header，剩余的 192 字节用于一些可选功能。PCI Spec 规定了两种类型的 Header: Type0 和 Type1, 其中 Type0 Header 表示 PCI 设备功能不是桥，Type1 Header 表示 PCI 功能是桥. 每个 PCI 设备或者桥内部都存在一定数量的存储空间或者 IO 空间，这部分空间通过 MMIO 映射到系统地址空间或者 IO 空间, Header 中存在数量不同的 Base Address 寄存器，该寄存器用于记录 PCI 设备/桥内部存储映射到系统地址空间的地址或者 IO 空间的地址。

![](/assets/PDB/HK/TH001486.png)

BAR 寄存器有些只读的 bit 是出厂前厂商固定好的 bit，固定 bit 包括 \[3:0], 其中 Bit0 为 0 时表示对应的空间是一块内存，为 1 时表示对应的空间是一块 IO 空间. Bit2:1 构成的域用于表示对应的空间宽度，如果是 00，那么对应的空间宽度为 32 位，如果是 10，那么空间宽度为 64 位. Bit3 表示空间是否支持预读，当为 0 时表示不预读，而为 1 时表示为预读. 

![](/assets/PDB/HK/TH001487.png)

系统上电之后，BAR 寄存器的低 \[11:4] 位全为 0 而高位未知，此时系统软件(BIOS) 向 BAR 寄存器写入全 1，然后读取 BAR 寄存器会得到 BAR 对应空间的长度，从 Bit4 到 Bit31 域的最低置位位表示 BAR 的大小，例如上图中域中最低置位位是 Bit20, 那么可操作的最低位为 20，则 BAR 可申请的地址空间为 1MiB(2^20).

![](/assets/PDB/HK/TH001488.png)

在获得 BAR 对应空间的长度之后，系统软件(BIOS) 将通过 MMIO 映射之后的地址对应的页帧号写入到 BAR 寄存器的高 20 位，例如上图 BAR 对应的内存通过 MMIO 映射到了系统地址空间的 0xFE000000, 那么将 0xFE000 写入到 BAR 寄存器高 20 位. 至此 PCI 设备 32 位内存映射到了系统地址空间.

![](/assets/PDB/HK/TH001489.png)

对于 BAR 指向 64 位的空间，需要使用两个 BAR 进行描述，BARn 用于描述 64 位的低 32 位，BAR(n+1) 则描述 64 位的高 32 位。系统上电之后，BAR 寄存器的低 \[25:4] 位全为 0 而高位未知，[2:1] 为 10. 此时系统软件(BIOS) 向两个 BAR 寄存器写入全 1，然后读取两个 BAR 寄存器会得到 BAR 对应空间的长度，从 Bit26 到下一个 BAR 的所有 bit 最低置位位表示 BAR 的大小，例如上图中域中最低置位位是 Bit27, 那么可操作的最低位为 27，则 BAR 可申请的地址空间为 128MiB(2^27). 在获得 BAR 对应空间的长度之后，系统软件(BIOS) 将分配的物理地址的高 36bit 写入到两个 BAR 寄存器器中，其中 BAR(n+1) 写入高 36bit 的高 32 位，BARn 则写入高 36bit 的低 4bit，例如上图 BAR 对应空间映射的物理地址是: 0x0000000F00000000.

<span id="D5"></span>
###### PCI 总线地址与系统总线映射

![](/assets/PDB/HK/TH001476.png)

PCI 总线枚举完所有的节点之后，为每个 PCI 节点分配了一个 BDF，那么可以通过 BDF 为每个 PCI Function 在 PCI Configuration Space 中找到一个配置空间。配置空间中包含了最多 6 个基地址寄存器(BAR), BAR 用于记录 PCI 节点内部的存储空间或者 IO 空间映射到系统地址总线或者 IO 空间的地址。另外上电复位之后，其配置空间的 BAR 是未初始化的，那么接下来系统软件 BIOS 为每个 PCI 节点的 BAR 分配物理地址。系统初始化阶段预留一段系统地址空间作为 PCI 映射区域，那么 BIOS 会根据一定的逻辑为每个 PCI 节点分配一段 PCI 映射区域。

![](/assets/PDB/HK/TH001491.png)

接下来通过一个例子进行讲解: 假设系统将 \[0xFE000000, 0x100000000) 开始的 32MiB 区域用于映射 PCI 内存，当 CPU 访问这段物理地址时，Host 主桥会认领这段地址，并将物理地址访问的空间由物理内存空间转换成 PCI 地址空间，并与 \[0x80000000, 0x82000000] 这段 PCI 总线空间对应. 为了简化起见，假设每个 PCI 设备的每个 Function 有且仅有一个存储 BAR0，其长度为 2MiB. 另外 PCI 桥不包含任何 BAR，其拓扑结构如下:

![](/assets/PDB/HK/TH001490.png)

BIOS 根据深度优先查找算法(DFS) 首先找到第一组 PCI 设备: **PCI 设备2** 和 **PCI 设备3**, 其中 **PCI 设备2** 包含了两个 Function，每个 Function 包含一个 BAR，且长度为 2MiB，那么 BIOS 为 **PCI 设备2** Function0 的 BAR0 分配映射地址为 0xFE000000, Function1 的 BAR0 分配映射地址为 0xFE200000. **PCI 设备3** 是单 Function 设备，其只只包含一个 BAR 且长度为 2MiB，那么 BIOS 为其 Function0 的 BAR0 分配映射地址为 0xFE400000. 在配置完 PCI 设备之后，BIOS 将初始化 **PCI 桥3**，其 Memory Base 寄存器保存其下所有 PCI 设备使用的 "PCI 总线域地址空间的基地址"，而 Memory Limit 寄存器保存了其下 PCI 设备使用的 "PCI 总线域地址空间大小"。BIOS 将 **PCI 桥3** 的 Memory Base 寄存器设置为 0x80000000, 而 Memory Limit 寄存器设置为 0x600000.

![](/assets/PDB/HK/TH001492.png)

BIOS 回溯到 PCI Bus 2，并找到 **PCI 设备4**，并为其 Function0 的 BAR0 分配映射地址 0xFE600000. 完成 PCI Bus 2 的遍历之后，BIOS 初始化 **PCI 桥2** 的配置寄存器，将 Memory Base 寄存器设置为 0x80000000, Memory Limit 寄存器设置为 0x800000.

![](/assets/PDB/HK/TH001493.png)

BIOS 回溯到 PCI Bus 1，并找到 **PCI 设备5**，并为其 Function0 的 BAR0 分配映射地址 0xFE800000. 完成 PCI Bus 2 的遍历之后，BIOS 初始化 **PCI 桥1** 的配置寄存器，将 Memory Base 寄存器设置为 0x80000000, Memory Limit 寄存器设置为 0xa00000.

![](/assets/PDB/HK/TH001494.png)

BIOS 回溯到 PCI Bus 0, 并在总线上发现了另外一个 PCI 桥，即 **PCI 桥4**. 那么 BIOS 使用 DFS 算法继续遍历 PCI Bus 4, 并未在 PCI Bus 4 上发现任何桥，只发现 **PCI 设备6** 和 **PCI 设备7**. BIOS 为 **PCI 设备6** Function0 的 BAR0 分配地址为 0xFEA00000, 由于 **PCI 设备7** 中包含了 Function0、Function4、Function5，那么 BIOS 按顺序为 **PCI 设备7** Function0 的 BAR0 分配 0xFEC00000, Function4 的 BAR0 分配 0xFEE00000, Function5 的 BAR0 分配 0xFF000000. BIOS 接着初始化 **PCI 桥4**，将其 Memory Base 寄存器设置为 0x80A00000, Memory Limit 寄存器设置为 0x800000. 

![](/assets/PDB/HK/TH001495.png)

BIOS 回溯到 PCI Bus 0，此时总线上已经没有任何 PCI 桥，那么只有 **PCI 设备1** 没有初始化，BIOS 为 **PCI 设备1** Function0 的 BAR0 分配地址为 0xFF200000. 最后结束整个 DFS 遍历过程.

![](/assets/PDB/HK/TH001496.png)

经过 DFS 的遍历，BIOS 已经为每个 PCI 设备的 BAR 分配了映射到系统地址空间的物理地址，当 CPU 访问这些地址时，RC 会将访问的地址由系统地址空间地址转换 PCI 地址空间的地址，这样就完成了对 PCI 设备内部存储或者 IO 空间的访问.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### PCI/PCIe 实践攻略

市面上常见的 PCI/PCIe 设备包括网卡、显卡、加速卡等，如果想实践 PCI 设备，那么需要购买相应的设备和主机，这样花费昂贵且困难程度很大。BiscuitOS 平台是一个基于 QEMU 的虚拟化平台，可以利用 BiscuitOS 构建不同的架构的虚拟机，其底层基于 QEMU 和 KVM。为了让广大开发者能否从实践中体会 PCI 设备的使用，BiscuitOS 通过 QEMU 模拟了一个 PCI 设备，并为模拟的 PCI 设备提供了内核驱动，开发者可以完成的实践一个 PCI 设备。另外感兴趣的童鞋可以对 QEMU 模拟的设备进行定制化开发，并对 BIOS 和 Kernel 阶段对 PCI 设备进行不同程度的实践。说这么多不如动手做做一次，下面以 AMD64 Intel 为硬件架构并配合 Linux 5.0 内核进行实践讲解，开发者可以举一反三在其他平台实践。在实践之前开发者需要提前部署 BiscuitOS X86_64 Linux 5.0 的开发环境，具体请参考:

> [BiscuitOS X86_64 Linux 5.0 开发环境搭建](/blog/Linux-5.0-x86_64-Usermanual/)

![](/assets/PDB/HK/TH001497.png)

###### QEMU 部署 PCI 模拟设备

QEMU 可以模拟 PCI 设备，BiscuitOS 提供了一个可实践的 PCI 设备 "BiscuitOS-pci", 其是一个简单的 PCI 设备，其内部包含了一块存储空间和一块 IO 空间，设备挂载 PCI Bus 0 总线上。首先在 QEMU 源码目录下执行如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/qemu-system/hw/
mkdir BiscuitOS
vi Makefile.objs

# Add context
devices-dirs-$(CONFIG_SOFTMMU) += BiscuitOS/

vi Kconfig

# Add context
source BiscuitOS/Kconfig
{% endhighlight %}

安装 QEMU 源码的逻辑，在 hw 目录下创建 BiscuitOS 目录，用于存放 PCI 设备的源码。接下来是通过 BiscuitOS 编译平台下载 PCI 设备源码，参考如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig 

  [*] Package --->
      [*] PCI: Peripheral Component Interconnect --->
          [*] QEMU emulate PCIe Device (BiscuitOS-pcie) --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-pci-device-QEMU-emulate-default/
make download
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/qemu-system/hw/BiscuitOS/
cp BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-pci-device-QEMU-emulate-default/BiscuitOS-pci-device-QEMU-emulate-default ./ -rf
{% endhighlight %}

> [BiscuitOS QEMU PCI Device Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/PCIe/BiscuitOS-pci-device-QEMU-emulate)

在 BiscuitOS 的顶层目录执行 make menuconfig 之后，选择 QEMU PCI 设备源码和内核 PCI 驱动源码，然后执行 make 进行部署。接着进入 BiscuitOS-pci-device-QEMU-emulate-default 目录执行 make download 下载 QEMU PCI 设备源码。待源码下载完毕之后将其拷贝到 QEMU 源码 hw/BiscuitOS/ 目录下.

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/qemu-system/hw/BiscuitOS/
vi Makefile.objs

# Add context
common-obj-$(CONFIG_BISCUITOS_PCI) += BiscuitOS-pci-device-QEMU-emulate-default/

vi Kconfig

# Add context
source BiscuitOS-pci-device-QEMU-emulate-default/Kconfig
{% endhighlight %}

源码拷贝完毕之后，修改 hw/BiscuitOS 目录下 Makefile.objs 和 Kconfig，是 QEMU PCI 设备源码进入 QEMU 源码的编译体系中.

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/qemu-system/
vi default-configs/i386-softmmu.mak

# Add context
CONFIG_BISCUITOS_PCI=y

vi config-all-devices.mak

# Add context
CONFIG_BISCUITOS_PCI:=$(findstring y,$(CONFIG_BISCUITOS_PCI)y)
{% endhighlight %}

接着修改 QEMU 源码目录下的 "default-configs/i386-softmmu.mak", 启用 CONFIG_BISCUITOS_PCI 宏，然后修改 config-all-devices.mak 文件，使 CONFIG_BISCUITOS_PCI 宏始终为 Y，至此 QEMU PCI 设备源码已经添加到 QEMU 编译体系，接着只要重新编译源码设备就存在 QEMU 中，但还没能让 QEMU 启用该设备，需要修改 QEMU Command Line:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/
vi RunBiscuitOS.sh

# Add context
        -hda ${ROOT}/BiscuitOS.img \
+       -device BiscuitOS-pci \
        -drive file=${ROOT}/Freeze.img,if=virtio \
{% endhighlight %}

在 RunBiscuitOS.sh 中添加 "-device BiscuitOS-pci" 字段之后，系统启动之后就可以看到该设备。设备的 VendorID 和 DeviceID 分别是 0x1016:0x1413. 最后使用如下命令重新编译 QEMU 并在 BiscuitOS 中检查 QEMU 模拟的 PCI 设备:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/
./RunQEMU.sh -b
{% endhighlight %}

![](/assets/PDB/HK/TH001498.png)

从内核的 Dmesg 中可以看出内核在枚举 PCI 总线上的 PCI 设备时，已经探测到 QEMU 模拟的 PCI 设备 0x1016:0x1413. 那么接下来部署 PCI 设备对应的 Linux 驱动:

---------------------------------------

###### 部署 PCI 设备驱动

与 QEMU PCI 设备部署一致，借助 BiscuitOS 编译平台进行 PCI 设备驱动的部署，使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PCI: Peripheral Component Interconnect --->
          [*] PCI Common Device Driver Module --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-pci-device-driver-default/
make download
make 
make install
make pack
make run
{% endhighlight %}

> [BiscuitOS PCI Device Driver Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/PCIe/BiscuitOS-pci-device-driver)

![](/assets/PDB/HK/TH001499.png)

当驱动部署成功在 BiscuitOS 上运行之后，注册一个 PCI 设备，PCI 设备的其中一块 BAR 映射到系统地址空间，而另外一个 BAR 映射到系统的 IO 空间，驱动程序分别对两块 BAR 空间进行了读写操作. 

![](/assets/PDB/HK/TH001500.png)

查看系统的地址空间布局可以看到 PCI 设备的内存 BAR 映射到了系统地址空间的 \[0xFEA00000, 0xFEB00000), 这段区域在系统地址空间称为 "BiscuitOS-PCIe-MMIO".

![](/assets/PDB/HK/TH001501.png)

查看系统的 IO 空间布局可以看到 PCI 设备的 IO BAR 映射到了系统 IO空间的 \[0xC000, 0xC07F], 这段区域在系统 IO 空间称为 "BiscuitOS-PCIe-IO"。至此实践到此为止.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)




