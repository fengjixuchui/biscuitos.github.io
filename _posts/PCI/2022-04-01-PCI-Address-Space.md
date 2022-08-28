---
layout: post
title:  "PCI/PCIe 域地址空间"
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
> - [PCIe 基础知识](#C)
>
> - [PCI 域地址空间](#B)
>
>   - [PCI MM-BAR 存储空间]()
>
>   - [PCI IO-BAR IO 端口]()
>
>   - [CPU 访问 PCI 域空间]()
>
>   - [PCI 访问内存域: DMA]()
>
>   - [PCI Memory Cache 一致性]()
>
> - [PCI 实践攻略](#D)
>
> - [PCI 开源工具](#E)
>
> - [PCI/PCIe 使用攻略](#F)
>
> - PCI/PCIe 与 Linux/BIOS
>
>   - [BIOS PCI/PCIe 研究](/blog/PCI-Address-Space-seaBIOS/)
>
>   - [Linux PCI/PCIe 子系统]()
>
>   - [PCI/PCIe 设备驱动](#G)
>
>   - [DMA 设备驱动]()
>
>   - [PCI IO-Port]()
>
> - [PCI 设备虚拟化研究]()
>
>   - [QEMU PCI 总线/PCI 设备/PCI 桥模拟]()
>
>   - [PCI 半虚拟化之 virtio]()
>
>   - [PCI 虚拟化之设备直通]()
>
>   - [PCI 虚拟化之 vhost]()
>
>   - [PCI 虚拟化之 vhost-user]()
>
>   - [PCI 虚拟化值 Intel SR-IOV]()
>
> - PCI/PCIe 中断研究
>
>   - PCI INTA 中断机制
>
>   - PCIe MSI 中断机制
>
>   - PCIe MSIX 中断机制
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
>
>   - [PCI/PCIe Class Code 表](https://blog.csdn.net/niepangu/article/details/61619990)

###### 捐赠一下吧

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### PCI 基础知识

![](/assets/PDB/HK/TH001468.JPEG)

**PCI**: Peripheral Component Interconnect(外部设备互联标志) 是由 PCISIG(PCI Special Interest Group) 推出的一种局部并行总线标准，其由 **ISA**(Industy Standard Architecture) 总线发展而来，是一种同步的独立于处理器的 32/64 位局部总线。从结构上看，PCI 是在 CPU 原来的系统总线之间插入的一级总线，具体由一个桥接电路实现对该层的管理，并实现上下之间的接口以协调数据的传送。本节对 PCI 总线相关的基础概念进行讲解:

> [PCI 总线](#A0)
>
> [PCI 设备](#A1)
>
> [PCI 桥](#A2)
>
> [HOST 主桥](#A3)

###### <span id="A0">PCI 总线</span>

![](/assets/PDB/HK/TH001481.png)

**PCI 总线**是一种树型结构且独立于 CPU 总线，与 CPU 总线并行操作，PCI 总线上可以挂接 PCI 设备和 PCI 桥，PCI 总线上同一时刻只允许有一个 PCI 主设备，其他均为 PCI 从设备，而且读写操作只能在主从设备之间进行，从设备之间数据交换需要通过主设备中转。PCI 总线由 HOST 主桥或者 PCI 桥管理，用来链接各类设备，如声卡、网卡和 IDE 接口卡等。在一个系统中，可以通过 PCI 桥扩展 PCI 总线，形成具有血缘关系的多级 PCI 总线，从而形成 PCI 总线树型结构。在某些系统中具有几个 HOST 主桥，也就具有几颗 PCI 总线树，而每一课 PCI 总线树都与一个 PCI 总线域对应. 与 HOST 主桥直接连接的 PCI 总线通常命名为 PCI 总线 0.

![](/assets/PDB/HK/TH001467.png)

一个典型的 PCI 总线系统如上图的以 Intel 440FX PMC(PCI and Memory Controller) 为北桥芯片，PIIX (PCI ISA Xcelerator) 为南桥芯片组成的芯片组。北桥芯片 PMC 用于连接主板上的高速设备，向上提供了连接处理器的 Host 总线接口，可以连接多个处理器，向下主要提供了连接内存 DRAM 的接口和连接 PCI 总线系统的 PCI 总线接口，通过该 PCI root port 扩展出整个 PCI 设备树，包括 PIIX 南桥芯片. PIIX 南桥芯片则用于连接主板上的低速设备，主要包括 IDE 控制器、DMA 控制器、硬盘、USB 控制器、SMBus 总线控制器，并且提供了 ISA 总线用于连接更多的低速设备，如键盘、鼠标、BIOS ROM 等.

![](/assets/PDB/HK/TH001695.png)

PCI 总线由 HOST 主桥或者 PCI 桥进行管理，用来连接各类的 PCI Agent 设备和 PCI 桥。在一个处理器中一个 HOST 主桥管理一颗 PCI 总线树，每颗总线树与一个 PCI 总线域对应。PCI 总线域是 PCI 设备可以直接访问的地址，PCI 设备可以使用一段或者多段 PCI 域上的地址，其通过 PCI 设备的 BAR 寄存器进行维护。另外系统主内存的地址空间称为系统内存域空间，在 X86 架构中 PCI 总线域的范围和系统内存域的范围是相等的，X86 架构使用一些透明的硬件机制将 PCI 域上设备占用的区域映射到系统内存域上，从系统内存域角度来看这段地址区域称为 MMIO 地址; 同理 X86 架构也将系统内存域的区域映射到 PCI 域上，PCI 设备只能看到 PCI 域上的地址，当其访问映射系统内存区域的地址时，HOST 主桥会将请求转换为访问系统内存的请求; 同理 CPU 访问系统内存域上的 MMIO 地址时，HOST 主桥自动将请求转换为 PCI 域上的读写事务.

![](/assets/PDB/HK/TH001696.png)

PCI 总线是一种共享总线，总线上的设备分时共享这条总线。PCI 总线带宽为 32 位，总线频率 33MHz，每个时钟传输一组数据，因此 PCI 总线的带宽为 127.2MB/s (1017 Mbps); PCI 2.1 总线带宽是 64 位，总线频率是 66MHz，每个时钟传输一组数据，因此 PCI 2.1 总线的带宽为 508.6MB/s (4068.8 Mbps). PCI 总线上包含了多种信号线，后续章节详细说明.

![](/assets/PDB/HK/TH001692.png)

PCI 插槽是基于 PCI 总线元件扩展接口的扩展插槽，其颜色一般为乳白色，例如上图中 5 位置处的两个 PCI 插槽。可接插声卡、网卡、内置 Modem、内置 ADSL Model、USB2.0 卡、IEEE1394 卡、IDE 接口卡、RAID 卡、电视卡、视频采集卡以及其他种类繁多的扩展卡.

-----------------------------------

###### <span id="A1">PCI 设备</span>

![](/assets/PDB/HK/TH001479.png)

PCI 设备是挂接在 PCI 总线上的设备, PCI 设备主要有三类设备: PCI 主设备、PCI 从设备和 PCI 桥设备。其中 PCI 从设备只能被动的接受来自 HOST 主桥或者其他 PCI 设备的读写请求; 而 PCI 主设备可以通过总线仲裁获得 PCI 总线使用权，并主动向其他 PCI 设备或者存储器发起读写请求。PCI 桥设备主要用于管理下游的 PCI 总线，并转发上下游之间的总线事务。一个 PCI 设备可以既是主设备也可以是从设备，但是同一时刻，这个 PCI 设备或者为主设备或者为从设备。PCI 总线规范将 PCI 主从设备统称为 **PCI Agent 设备**. 在 PCI 总线中，HOST 主桥是一个特殊的 PCI 设备，该设备可以获得 PCI 总线的控制权访问 PCI 设备，可以被 PCI 设备访问。在 PCI 总线中，还有一类特殊的设备，既 PCI 桥设备，它包括了 PCI 桥、PCI-to-ISA 桥、PCI-to-EISA 桥和 PCI-to-Cardbus 桥。

![](/assets/PDB/HK/TH001687.png)

PCI 设备都有独立的配置空间, HOST 主桥通过配置读写总线事务访问这段空间，PCI 总线规定了三种类型的 PCI 配置空间，分别是 PCI Agent 设备使用的配置空间，PCI 桥使用的配置空间和 Cardbus 桥卡使用的配置空间。PCI 设备通常将 PCI 配置信息存放在 EEPROM 中，PCI 设备进行上电初始化时将 EEPROM 中的信息读到 PCI 设备的配置空间作为初始值，这个过程由硬件逻辑完成，绝大多数 PCI 设备使用这种方式初始化其配置空间。PCI 设备可能使用 PCI 总线域上的一段或多段存储器空间，并使用 BAR 寄存器表示这段存储器空间在 PCI 总线域的基地址; 同理 PCI 设备可能使用 PCI 总线域上的一段或多段 I/O 空间，并使用 BAR 寄存器表示这段 I/O 空间在 PCI 总线域的基地址。为了区分 PCI 设备的 I/O 空间和存储器空间的 BAR，使用 MEM-BAR 表示存储器 BAR，而 IO-BAR 表示 I/O 空间 BAR. PCI 内部也存在存储空间 (Intramural memory).

![](/assets/PDB/HK/TH001472.png)

在 X86 处理器中，内核通过 CONFIG_ADDR 和 CONFIG_DATA 寄存器读取 PCI Agent 设备的配置空间。在 PCI Agent 设备配置空间包含上图中的寄存器，详细寄存器介绍可以参考《[PCI Agent 设备配置空间 TYPE0](#D0)》, 当 PCI 设备的配置空间初始化之后，该 PCI 设备在当前的 PCI 总线树上拥有一个独立的 PCI 总线地址空间，即 BAR 寄存器所描述的空间。在配置空间中含有该设备在 PCI 总线中使用的基地址，系统软件可以动态配置这个地址，从而保证每个 PCI 设备使用的物理地址并不相同。常见的 PCI 设备包括下面的: PCI 网卡、PCI 声卡、PCI 磁盘以及 PCI 显卡.

![PCI 网卡](/assets/PDB/HK/TH001689.png)
![PCI 声卡](/assets/PDB/HK/TH001690.png)
![PCI 磁盘](/assets/PDB/HK/TH001691.png)
![PCI 显卡](/assets/PDB/HK/TH001693.png)

------------------------------------------

###### <span id="A2">PCI 桥</span>

![](/assets/PDB/HK/TH001479.png)

在 PCI 总线中，还有一类特殊的设备，既 PCI 桥设备，它包括了 PCI 桥、PCI-to-ISA 桥、PCI-to-EISA 桥和 PCI-to-Cardbus 桥。PCI 桥的引入使 PCI 总线具有扩展性，也极大增加了 PCI 总线的复杂度。PCI 总线的电气特性决定了一条 PCI 总线上挂接的负载有限，当 PCI 总线需要连接多个 PCI 设备时，需要使用 PCI 桥进行总线扩展，扩展出的 PCI 总线可以连接其他 PCI 设备，包括 PCI 桥。在一颗 PCI 总线树上，最多可以挂接 256 个 PCI 设备，包括 PCI 桥。PCI 桥作为一个特殊的 PCI 设备，具有独立的配置空间，但是其配置空间与 PCI Agent 设备有所不同。PCI 桥的配置空间可以管理其下 PCI 总线子树的 PCI 设备. 使用 PCI 桥可以扩展出新的 PCI 总线，在这条 PCI 总线上可以继续挂接多个 PCI 设备。

![](/assets/PDB/HK/TH001688.png)

PCI 桥跨接在两个 PCI 总线之间, 其中距离 HOST 主桥较近的 PCI 总线被称为该桥片的上游总线(Primary Bus)，距离 HOST 主桥较远的 PCI 总线称为该桥片的下游总线(Secondary Bus). 如图 PCI 桥1 的上游总线为 PCI Bus 0，而 PCI 桥1 的下游总线为 PCI Bus1, 同理 PCI 桥3 的上游总线为 PCI Bus 2, 而下游总线为 PCI Bus 3. 上下游总线间的数据通过 PCI 桥进行处理和转发. 每一个 PCI 总线的下方都可以挂接一个或多个 PCI 桥，每个 PCI 桥都可以连接一条新的 PCI 总线。在同一条 PCI 总线上的设备之间的数据交换不会影响其他 PCI 总线。PCI 桥可以扩展一条新的 PCI 总线，但是不能扩展新的 PCI 总线域，例如当前系统使用 32 位的 PCI 总线地址，那么这个系统的 PCI 总线域的地址空间为 4GB，在这个总线域上的所有设备将共享这个 4GB 大小的空间。

![](/assets/PDB/HK/TH001473.png)

在 X86 处理器中，内核通过 CONFIG_ADDR 和 CONFIG_DATA 寄存器读取 PCI 桥的配置空间。在 PCI 桥配置空间包含上图中的寄存器，与 PCI Agent 设备的配置空间类似，详细寄存器介绍可以参考《[PCI 桥配置空间 TYPE1](#D1)》。不同的是 PCI 桥的配置空间值包括两组 BAR 寄存器，其含义与 PCI Agent 设备的 BAR 寄存器一致。PCI 桥除了作为 PCI 设备之外，还需要管理旗下连接的 PCI 总线字树使用的各类资源。在 PCI 桥中，与 Secondary Bus 相关的寄存器分做两大类，一类寄存器管理 Secondary Bus 之下 PCI 子树的总线号，如 Secondary 寄存器存放 Secondary Bus 号，这个 Bus 号也是该 PCI 桥管理的 PCI 子树中编号最小的 PCI 总线号，同理 Subordinate Bus Number 寄存器存放当前 PCI 桥管理的 PCI 子树中，编号最大的 PCI 总线号.

----------------------------------

###### <span id="A3">HOST 主桥</span>

![](/assets/PDB/HK/TH001481.png)

HOST 主桥是一个很特别的 PCI 桥，其主要功能是隔离处理器系统的存储域和 PCI 总线域，管理 PCI 总线域，并完成处理器与 PCI 设备间的数据交换。处理器与 PCI 设备间的数据交换主要由**处理器访问 PCI 设备的地址空间**和**PCI 设备使用 DMA 机制访问主存储器**两部分组成. 不同架构的处理器中 HOST 主桥的位置不同，在 x86 架构中使用南北桥结构，处理器内核在一个芯片中，而 HOST 主桥在北桥中。HOST 主桥是联系处理器与 PCI 设备的桥梁，在一个处理器系统中，每一个 HOST 主桥都管理一棵 PCI 总线树，在同一棵 PCI 总线树上的所有 PCI 设备都属于同一个 PCI 总线域。

![](/assets/PDB/HK/TH001694.png)

如上拓扑图有一个 CPU、一个 DRAM 控制器和两个 HOST 主桥组成，在这个处理器系统中，包含了 CPU 域、存储域和 PCI 总线域。其中 HOST 主桥A 和 HOST 主桥 B 分别管理 PCI 总线 A 域和 PCI 总线 B 域。PCI 设备访问存储器域时，也需要通过 HOST 主桥，并由 HOST 主桥进行 PCI 总线域到存储器域地址转换; 同理 CPU 访问 PCI 设备时，同样需要通过 HOST 主桥进行存储器与到 PCI 总线域地址转换。如果 HOST 主桥支持 Peer-to-Peer 传送机制，那么 PCI 总线 A 域上的设备可以与 PCI 总线 B 域上的设备直接通信，如 PCI 设备0 可以直接与 PCI 设备4 直接通信.

![](/assets/PDB/HK/TH001467.png)

x86 处理器使用南北桥连接 CPU 和 PCI 设备，其中北桥 (North Bridge) 连接快速设备，如显卡和内存条，并连接 PCI 总线，HOST 主桥包含在北桥中。而南桥 (South Bridge) 连接慢速设备，上图典型的 PCI 总线系统 Intel 440FX, 其中 PMC 为北桥芯片，PIIX 为南桥芯片组成的芯片组. 不同处理器系统继承这些部件的方式并不相同，在许多嵌入式系统中，既含有 PCI 设备也含有 PCIe 设备等。Intel 使用南北桥概念统一 PC 架构，但从本质来看，南北桥架构并不重要，北桥中存放的主要部件不过是存储器控制器、显卡控制器和 HOST 主桥而已，而南桥存放的是一些慢速设备，如 ISA 总线和中断控制器等.

![](/assets/PDB/HK/TH001475.png)

X86 架构定义了两个 IO 端口寄存器 CONFIG_ADDRESS 和 CONFIG_DATA 寄存器，其地址为 0xCF8 和 0xCFC. x86 架构使用这两个 IO 端口寄存器访问 PCI 设备的配置空间。当 X86 架构处理器对 CONFIG_DATA 寄存器进行 I/O 读写访问，且 CONFIG_ADDR 寄存器的 Enable 为 1 时，HOST 主桥将这个 I/O 读写访问转换为 PCI 配置读写总线事务，然后发送到 PCI 总线上，PCI 总线根据保存在 CONFIG_ADDR 寄存器中的 ID 号，将 PCI 配置读写请求发送到指定的 PCI 设备的指定配置寄存器中.

-------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### PCIe 基础知识


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

> [PCIE_Base_Specification_Revision_4_0_Version 1](https://whycan.com/files/members/3423/PCIE_Base_Specification_Revision_4_0_Version%201_0.pdf)

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

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001664.png)
![](/assets/PDB/HK/TH001665.png)
![](/assets/PDB/HK/TH001666.png)

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

与 QEMU PCI 设备部署一致，借助 BiscuitOS 编译平台通过简单的几个命令就可以进行 PCI 设备驱动的部署，使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PCI: Peripheral Component Interconnect --->
          [*] PCI Common Device Driver Module --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-pci-device-driver-default/
# 源码下载
make download
# 源码编译
make 
# 驱动安装
make install
# rootfs 打包
make pack
# 运行 BiscuitOS
make run
{% endhighlight %}

> [BiscuitOS PCI Device Driver Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/PCIe/BiscuitOS-pci-device-driver)
>
> [BiscuitOS 独立应用程序实践攻略](/blog/Human-Knowledge-Common/#C2)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001664.png)
![](/assets/PDB/HK/TH001667.png)
![](/assets/PDB/HK/TH001668.png)
![](/assets/PDB/HK/TH001499.png)

当驱动部署成功在 BiscuitOS 上运行之后，注册一个 PCI 设备，PCI 设备的其中一块 BAR 映射到系统地址空间，而另外一个 BAR 映射到系统的 IO 空间，驱动程序分别对两块 BAR 空间进行了读写操作. 

![](/assets/PDB/HK/TH001500.png)

查看系统的地址空间布局可以看到 PCI 设备的内存 BAR 映射到了系统地址空间的 \[0xFEA00000, 0xFEB00000), 这段区域在系统地址空间称为 "BiscuitOS-PCIe-MMIO".

![](/assets/PDB/HK/TH001501.png)

查看系统的 IO 空间布局可以看到 PCI 设备的 IO BAR 映射到了系统 IO空间的 \[0xC000, 0xC07F], 这段区域在系统 IO 空间称为 "BiscuitOS-PCIe-IO"。至此实践到此为止.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### PCI/PCIe 设备驱动

PCI(Peripheral Component Interconnect) 是外围设备互连的简称，作为一种通用的总线接口标准，它在目前的计算机系统中得到了非常广泛的应用。PCI 提供了一组完整的总线接口规范，其目的是描述如何将计算机系统中的外围设备以一种结构化和可控化的方式连接在一起，同时它还刻画了外围设备在连接时的电气特性和行为规约，并且详细定义了计算机系统中的各个不同部件之间应该如何正确地进行交互。无论是在基于 Intel 芯片的 PC 机中，或是在基于 Alpha 芯片的工作站上，PCI 毫无疑问都是目前使用最广泛的一种总线接口标准。同旧式的 ISA 总线不同，PCI 将计算机系统中的总线子系统与存储子系统完全地分开，CPU 通过一块称为 PCI 桥(PCI-Bridge)的设备来完成同总线子系统的交互，如图

![](/assets/PDB/HK/TH001481.png)

由于使用了更高的时钟频率，因此 PCI 总线能够获得比 ISA 总线更好的整体性能。PCI 总线的时钟频率一般在 25MHz 到 33MHz 范围内，有些甚至能够达到 66MHz 或者 133MHz，而在 64 位系统中则最高能达到 266MHz。尽管目前 PCI 设备大多采用 32 位数据总线，但 PCI 规范中已经给出了 64 位的扩展实现，从而使 PCI 总线能够更好地实现平台无关性，现在 PCI 总线已经能够用于 IA-32、Alpha、PowerPC、SPARC64 和 IA-64 等体系结构中。

![](/assets/PDB/HK/TH001467.png)

一个典型的 PCI 总线系统如上图的以 Intel 440FX PMC(PCI and Memory Controller) 为北桥芯片，PIIX (PCI ISA Xcelerator) 为南桥芯片组成的芯片组。北桥芯片 PMC 用于连接主板上的高速设备，向上提供了连接处理器的 Host 总线接口，可以连接多个处理器，向下主要提供了连接内存 DRAM 的接口和连接 PCI 总线系统的 PCI 总线接口，通过该 PCI root port 扩展出整个 PCI 设备树，包括 PIIX 南桥芯片. PIIX 南桥芯片则用于连接主板上的低速设备，主要包括 IDE 控制器、DMA 控制器、硬盘、USB 控制器、SMBus 总线控制器，并且提供了 ISA 总线用于连接更多的低速设备，如键盘、鼠标、BIOS ROM 等. 如果想更深入的了解 PCI/PCIe 知识，可以参考如下文档:

> [https://tldp.org/LDP/tlk/dd/pci.html](https://tldp.org/LDP/tlk/dd/pci.html)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

#### PCI 驱动部署实践

BiscuitOS 为了让开发者能够在不使用真实硬件的情况下感受 Linux PCI 驱动， BiscuitOS 基于 QEMU 模拟了一个 PCI 设备 "BiscuitOS-pci", 感兴趣的童鞋可以参考下面内容进行部署实践:

> - [PCI/PCIe 实践攻略(QEMU PCI 设备模拟/Linux PCI 驱动)](#D)

另外借助 BiscuitOS 编译平台通过简单的几个命令就可以进行 PCI 设备驱动的部署，开发者可以使用如下命令进行部署: (以 X86_64 Linux 5.0 为例进行讲解)

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
tree
{% endhighlight %}

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001503.png)
![](/assets/PDB/HK/TH001504.png)
![](/assets/PDB/HK/TH001505.png)

部署完毕之后可以在 BiscuitOS-pci-device-driver-default 目录下获得多个文件，其中 main.c 为 Linux PCI 设备驱动源文件、Makefile 是 BiscuitOS 编译驱动使用的 make 文件、Makefile.host 是本机编译驱动使用的 make 脚本文件、Kconfig 是 KBuild 的宏定义文件(可忽略)、README.md 是驱动简要说明文件. 有了以上文件就可以在 BiscuitOS 编译并运行(前提是已经部署了 QEMU PCI 设备模拟组件), 开发者只需参考如下命令就可以实践驱动:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-pci-device-driver-default/
make
make install
make pack
make run
{% endhighlight %}

> [BiscuitOS 独立应用程序实践攻略](/blog/Human-Knowledge-Common/#C2)

![](/assets/PDB/HK/TH001499.png)

当驱动部署成功在 BiscuitOS 上运行之后，注册一个 PCI 设备，PCI 设备的其中一块 BAR 映射到系统地址空间，而另外一个 BAR 映射到系统的 IO 空间，驱动程序分别对两块 BAR 空间进行了读写操作.

![](/assets/PDB/HK/TH001500.png)

查看系统的地址空间布局可以看到 PCI 设备的内存 BAR 映射到了系统地址空间的 \[0xFEA00000, 0xFEB00000), 这段区域在系统地址空间称为 "BiscuitOS-PCIe-MMIO".

![](/assets/PDB/HK/TH001501.png)

查看系统的 IO 空间布局可以看到 PCI 设备的 IO BAR 映射到了系统 IO空间的 \[0xC000, 0xC07F], 这段区域在系统 IO 空间称为 "BiscuitOS-PCIe-IO"。至此实践到此为止.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------

#### 驱动源码

> [BiscuitOS PCI Device Driver Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/PCIe/BiscuitOS-pci-device-driver)

{% highlight c %}
/*
 * BiscuitOS PCIe QEMU Device Driver
 *
 * (C) 2022.03.26 BuddyZhang1 <buddy.zhang@aliyun.com>
 * (C) 2022.04.02 BiscuitOS <https://biscuitos.github.io/blog/BiscuitOS_Catalogue/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>

#define DEV_NAME		"BiscuitOS-PCIe"
#define IO_BAR			0
#define MMIO_BAR		1
#define BAR_ADDR_REG		0x04
#define BAR_VER_REG		0x08

struct BiscuitOS_PCIe_state {
	struct pci_dev *pdev;
	/* IRQ */
	int irq;
	/* IO BAR */
	void __iomem *io;
	/* MMIO BAR */
	void __iomem *mmio;
};
static struct BiscuitOS_PCIe_state *bds;

/* Interrup handler */
static irqreturn_t BiscuitOS_PCIe_irq_handler(int irq, void *dev)
{
	/* TODO */
	return IRQ_HANDLED;
}

/* Device Probe interface */
static int BiscuitOS_PCIe_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret = 0;

	/* DMA status */
	bds = kzalloc(sizeof(*bds), GFP_KERNEL);
	if (!bds) {
		ret = -ENOMEM;
		printk("%s ERROR: DMA Status allocate failed.\n", DEV_NAME);
		goto err_alloc;
	}
	bds->pdev = pdev;

	/* Enable PCI device */
	ret = pci_enable_device(pdev);
	if (ret < 0) {
		printk("%s ERROR: PCI Device Enable failed.\n", DEV_NAME);
		goto err_enable_pci;
	}

	/* Request IO BAR resource */
	ret = pci_request_region(pdev, IO_BAR, "BiscuitOS-PCIe-IO");
	if (ret < 0) {
		printk("%s ERROR: Request IO BAR Failed.\n", DEV_NAME);
		goto err_request_io;
	}

	/* Mapping IO BAR */
	bds->io = pci_iomap(pdev, IO_BAR, pci_resource_len(pdev, IO_BAR));
	if (!bds->io) {
		ret = -EBUSY;
		printk("%s ERROR: Mapping IO Failedn\n", DEV_NAME);
		goto err_io;
	}

	/* Request MMIO BAR resource */
	ret = pci_request_region(pdev, MMIO_BAR, "BiscuitOS-PCIe-MMIO");
	if (ret < 0) {
		printk("%s ERROR: Request MMIO BAR Failed.\n", DEV_NAME);
		goto err_request_mmio;
	}

	/* Mapping MMIO BAR */
	bds->mmio = pci_iomap(pdev, MMIO_BAR, pci_resource_len(pdev, MMIO_BAR));
	if (!bds->mmio) {
		ret = -EBUSY;
		printk("%s ERROR: Mapping MMIO Failedn\n", DEV_NAME);
		goto err_mmio;
	}

	/* Request MSI IRQ */
	ret = pci_enable_msi(pdev);
	if (ret < 0) {
		printk("%s ERROR: Enable MSI Interrupt failed.\n", DEV_NAME);
		goto err_msi;
	}
	bds->irq = pdev->irq;

	/* Register Interrupt */
	ret = request_irq(bds->irq, BiscuitOS_PCIe_irq_handler,
					IRQF_SHARED, DEV_NAME, (void *)bds);
	if (ret < 0) {
		printk("%s ERROR: register irq failed.\n", DEV_NAME);
		goto err_irq;
	}

	/* Set master */
	pci_set_master(pdev);

	printk("%s Success Register PCIe Device.\n", DEV_NAME);

	/* Read Data From IO BAR */
	iowrite8(0x12, bds->io + BAR_ADDR_REG);
	printk("[IO-BAR] Address-Reg: %#hhx\n", ioread8(bds->io + BAR_ADDR_REG));

	/* Read Data From MMIO BAR */
	printk("[MMIO-BAR] VerionReg: %c\n", *(uint8_t *)(bds->mmio + BAR_VER_REG));

	return 0;

err_irq:
	pci_disable_msi(pdev);
err_msi:
	pci_iounmap(pdev, bds->io);
err_mmio:
	pci_release_region(pdev, MMIO_BAR);
err_request_mmio:
	pci_iounmap(pdev, bds->mmio);
err_io:
	pci_release_region(pdev, IO_BAR);
err_request_io:
	pci_disable_device(pdev);
err_enable_pci:
	kfree(bds);
	bds = NULL;
err_alloc:
	return ret;
}

static void BiscuitOS_PCIe_remove(struct pci_dev *pdev)
{

	free_irq(bds->irq, (void *)bds);
	pci_disable_msi(pdev);
	pci_iounmap(pdev, bds->io);
	pci_release_region(pdev, IO_BAR);
	pci_iounmap(pdev, bds->mmio);
	pci_release_region(pdev, MMIO_BAR);
	pci_disable_device(pdev);
	kfree(bds);
	bds = NULL;
}

static const struct pci_device_id BiscuitOS_PCIe_ids[] = {
	{ PCI_DEVICE(0x1016, 0x1413), },
};

static struct pci_driver BiscuitOS_PCIe_driver = {
	.name		= DEV_NAME,
	.id_table	= BiscuitOS_PCIe_ids,
	.probe		= BiscuitOS_PCIe_probe,
	.remove		= BiscuitOS_PCIe_remove,
};

/* Module initialize entry */
static int __init BiscuitOS_init(void)
{
	/* Register pci device */
	return pci_register_driver(&BiscuitOS_PCIe_driver);
}

/* Module exit entry */
static void __exit BiscuitOS_exit(void)
{
	/* Un-Register pci device */
	pci_unregister_driver(&BiscuitOS_PCIe_driver);
}

module_init(BiscuitOS_init);
module_exit(BiscuitOS_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BiscuitOS <buddy.zhang@aliyun.com>");
MODULE_DESCRIPTION("BiscuitOS PCIe Device Driver");
{% endhighlight %}

###### MAkefile

{% highlight bash %}
obj-m += pcie.o

pcie-objs := main.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

ROOT := $(dir $(M))
DEMOINCLUDE := -I$(ROOT)../include -I$(ROOT)/include

GCCVERSION = $(shell gcc -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')

GCC49 := $(shell expr $(GCCVERSION) \>= 40900)

all:
        $(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install: all
        $(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
        depmod -a

clean:
        rm -rf *.o *.o.d *~ core .depend .*.cmd *.ko *.ko.unsigned *.mod.c .tmp_versions *.symvers \
                *.save *.bak Modules.* modules.order Module.markers *.bin

CFLAGS_pcie_demo.o := -Wall $(DEMOINCLUDE)

ifeq ($(GCC49),1)
        CFLAGS_pcie_demo.o += -Wno-error=date-time
endif

CFLAGS_main.o := $(DEMOINCLUDE)
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------

#### 驱动讲解

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### PCI/PCIe 开源工具

开源世界存在多个工具用于操作 PCI，本节用于介绍各种与 PCI 相关的开源工具，需要的童鞋可以按目录进行使用:

> - [pcituils](#E0)
>
>   - [pciutils 编译部署](#E01)
>
>   - [lspci](#E02)
>
>   - [setpci](#E03)
>
>   - [update-pciids](#E04)
>
>   - [pcimodules](#E05)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="E0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

#### pciuitls

pciutils 包含了一个用于 PCI 总线配置寄存器的可移植库及基于这个库开发的几个实用程序. pciutils 支持的平台非常多，包括 Linux/Windows/FreeBSD/Darwin 等平台，且这些不同平台访问 pci bus 总线的方法也有所区别，pciutils 完成兼容了这些平台，具有很好的移植性. pciutils 支持如下几个实用程序以及部署:

> - [lspci - 显示所有 pci 总线与设备的详细信息](#E02)
>
> - [setpci - 运行用户从 pci 配置空间寄存器读取写入数据](#E03)
>
> - [update-pciids - 从网络上下载 pci.ids 文件的当前版本](#E04)
>
> - [pcimodules - 列出当前系统上所有插入的 pci 设备可用的内核驱动](#E05)
>
> - [pciuitls 源码编译/跨平台安装](#E01)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------

#### <span id="E01">pciutils 源码编译/跨平台安装</span>

pciuitls 支持的平台非常多，本节用于介绍如何通过源码编译安装 pciuitls 工具。BiscuitOS 目前已经支持 pciutils 的部署，本节从三个维度进行讲解:

> [pciutils 源码网站 http://mj.ucw.cz/pciutils.shtml ](http://mj.ucw.cz/pciutils.shtml )

下载所需版本并进行解压编译，使用如下命令:

{% highlight bash %}
wget http://mj.ucw.cz/download/linux/pci/pciutils-3.7.0.tar.gz
tar xf pciutils-3.7.0.tar.gz
cd pciutils-3.7.0
make
{% endhighlight %}

![](/assets/PDB/HK/TH001506.png)

编译成功之后可以获得 lspci/setpci/update-pciids 等工具，直接运行即可. 如果开发者不想通过源码编译的方式安装 pciuitls 工具，那么开发者可以在不同发行版下使用包安装工具直接进行安装, 例如:

{% highlight bash %}
# Ubuntu/Debian
sudo apt install -y pciutils

# CentOS
sudo yum install -y pciutils
{% endhighlight %}

最后 BiscuitOS 上部署 pciuitls 工具可以参考如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PCI: Peripheral Component Interconnect --->
          [*] PCI Utilities: lspci/setpci/update-pciids --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/pciutils-3.7.0/
make download
make tar
tree -L 1
{% endhighlight %}

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001503.png)
![](/assets/PDB/HK/TH001507.png)
![](/assets/PDB/HK/TH001508.png)

在 BiscuitOS 使用简单几个命令就可以部署 pciutils，接下来进行源码编译安装并在 BiscuitOS 使用，参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/pciutils-3.7.0/
make
make install
make pack
make run
{% endhighlight %}

![](/assets/PDB/HK/TH001509.png)

由于 BUSYBOX 默认带有 lspci，为了区分两个工具，那么将 pciutils 生成的 lscpi 命名为 lspci-common, 可以看出该工具可以在 BiscuitOS 正常运行.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------

#### <span id="E02">lspci</span>

![](/assets/PDB/HK/TH001509.png)

lscpi 是 pciuitls 工具包合集中一个用于显示 PCI 总线与设备信息的工具，其提供了丰富的参数以此获得更多有效的信息，其参数列表如下:

![](/assets/PDB/HK/TH001510.png)

> - [lspci](#E0200)
>
> - [lspci -s](#E0201)
>
> - [lspci -v\[vv\]](#E0202)
>
> - [lspci -n\[n\]](#E0203)
>
> - [lspci -d](#E0204)

----------------------------------

###### <span id="E0200">lspci</span>

![](/assets/PDB/HK/TH001511.png)

lspci 命令不带任何参数时将显示处目前所有 PCI 总线上的设备，其输出的格式如下:

{% highlight bash %}
# lspci information format
# 00:04.0 Class 00ff: Device 1016:1413 (rev 10)
00            : 00        : 04       . 0           Class 00ff      : Device 1016      : 1413              ( rev 10        )
<DomainNumber>:<BusNumber>:<DeviceID>.<FunctionID> Class <ClassNum>: Device <VendorID>:<Product-DeviceID> (<ReviewVersion>)
{% endhighlight %}

* DomainNumer: 域编号，一般为 0 不显示
* BusNumer: BDF 中 PCI 设备所在 PCI Bus 号
* DeviceID: BDF 中 PCI 设备的设备号
* FunctionID: BDF 中 PCI 设备的功能号
* ClassNum: BDF 中 PCI 设备的功能类别信息 
* VendorID: 厂商 ID
* Product-DeviceID: 厂商设备 ID
* ReviewVersion: 

----------------------------------

###### <span id="E0201">lspci -s</span>

![](/assets/PDB/HK/TH001512.png)

lspci "-s" 通过 BDF 找到指定 PCI 设备，该参数与 "Domain"、"BusID"、"SlotID" 和 "FunctionID" 任意组合都可以查找不同需求的 PCI 设备，以便简介显示指定 PCI 设备的信息. 其使用格式以及输出如下:

{% highlight bash %}
# Show only devices in selected slots
lspci -s [[[[<domain>]:]<bus>]:][<slot>][.[<func>]]

# Show Domain all Device
lspci -s Domain::.

# Show Bus all Device
lspci -s :Bus:.

# Show match slot all Device
lspci -s ::slot.

# show match function all Device
lspci -s ::.func

# Show all Device
lspci -s ::.
{% endhighlight %}

* domain: 域 ID
* bus: BDF 中 PCI 设备所在 PCI Bus ID
* slot: BDF 中 PCI 设备所在 Slot ID，或者是 DeviceID
* func: BDF 中 PCI 设备的 Function Number

----------------------------------

###### <span id="E0202">lspci -v[vv]</span>

lspci "-v" 参数以冗余模式显示所有设备的详细信息. 其可以和 "-s" 参数联合使用，以此获得指定 PCI 设备详细信息. 其使用如下:

![](/assets/PDB/HK/TH001513.png)

上面的命令用于显示 00:04.0 设备的详细信息，"00:04.0" 字段表示设备的 BDF 号，"Class 00ff" 为设备的 ClassNumber，"Device 1016:1413" 字段是设备的厂商 ID 和厂商设备 ID. "Subsystem: Device" 字段表示. "IRQ 11" 字段表示 PCI 使用的中断 11. "I/O ports at c000 [size=128]" 表示 PCI 设备的第一个 BAR 是一个 IO，其映射到系统 IO 空间 0xC000, 另外 BAR 长度为 128 字节. "Memory at fea00000 (32-bit, non-prefetchable) [size=1M]" 表示 PCI 设备的第二个 BAR 是一个 MEM，其映射到系统地址空间 0xFEA00000, 长度为 1M 且为非预读 MEM. 

![](/assets/PDB/HK/TH001552.png)

lspci "-vv" 参数比 "-v" 参数以更冗余模式显示所有设备的信息。相比 "-v" 参数输出的信息，其包含了 Control 寄存器的信息、Status Cap 寄存器的信息.

![](/assets/PDB/HK/TH001553.png)

lspci "-vvv" 参数比 "-vv" 参数以更冗余模式显示所有设备的信息。大部分情况下两者输出信息一致.

--------------------------------------------

###### <span id="E0203">lspci -n[n]</span>

![](/assets/PDB/HK/TH001554.png)

lspci "-n" 参数相比单纯的 "lspci" 命令，其不显示 ID 的名字只显示 ID 的值，其格式如下:

{% highlight bash %}
# lspci -n information format
# 00:04.0 00ff: 1016:1413 (rev 10)
00            : 00        : 04       . 0           00ff      : 1016      : 1413              ( rev 10        )
<DomainNumber>:<BusNumber>:<DeviceID>.<FunctionID> <ClassNum>: <VendorID>:<Product-DeviceID> (<ReviewVersion>)
{% endhighlight %}

* DomainNumer: 域编号，一般为 0 不显示
* BusNumer: BDF 中 PCI 设备所在 PCI Bus 号
* DeviceID: BDF 中 PCI 设备的设备号
* FunctionID: BDF 中 PCI 设备的功能号
* ClassNum: BDF 中 PCI 设备的功能类别信息 
* VendorID: 厂商 ID
* Product-DeviceID: 厂商设备 ID
* ReviewVersion: 

![](/assets/PDB/HK/TH001555.png)

相比 lspci "-n" 参数，lspci "-nn" 参数命令不止会输出 ID 的名字还会将 ID 的值通过中括号圈起来，其格式如下:

{% highlight bash %}
# lspci -nn information format
# 00:04.0 Class [00ff]: Device [1016:1413] (rev 10)
00            : 00        : 04       . 0           Class [00ff]      : Device [1016      : 1413]              ( rev 10        )
<DomainNumber>:<BusNumber>:<DeviceID>.<FunctionID> Class [<ClassNum>]: Device [<VendorID>:<Product-DeviceID>] (<ReviewVersion>)
{% endhighlight %}

* DomainNumer: 域编号，一般为 0 不显示
* BusNumer: BDF 中 PCI 设备所在 PCI Bus 号
* DeviceID: BDF 中 PCI 设备的设备号
* FunctionID: BDF 中 PCI 设备的功能号
* ClassNum: BDF 中 PCI 设备的功能类别信息
* VendorID: 厂商 ID
* Product-DeviceID: 厂商设备 ID
* ReviewVersion:

----------------------------------

###### <span id="E0206">lspci -d</span>

![](/assets/PDB/HK/TH001556.png)

lspci "-d" 参数可以根据 VendorID、DeviceID 和 ClassID 任意组合在 PCI 总线上查找指定的 PCI 设备，其使用格式如下:

{% highlight bash %}
# lspci -d information format
# Show only devices with specified ID's
lspci -d [<vendor>]:[<device>][:<class>]

# vendor and device and class
lspci -d vendor:device:class

# Only vendor and device
lspci -d vendor:device

# Only vendor and class
lspci -d vendor::class

# Only device and class
lspci -d :device:class

# Only vendor
lspci -d vendor::

# Only device
lspci -d :device:

# Only class
lspci -d ::class

# Nobody
lspci -d ::
{% endhighlight %}

-------------------------------------------

<span id="F"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000W.jpg)

#### PCI/PCIe 使用攻略

PCI/PCIe 使用攻略章节基于 [PCI 实践攻略](#D) 章节提供了 PCI 设备的多种应用场景，皆在最大程度上让开发者从实践中感受 PCI 设备的用途。BiscuitOS 通过 QEMU 模拟了适应不同场景的 PCI/PCIe 设备，并提供配套的驱动和应用程序，以此提供一个完整的实践环境与使用说明，开发者可以参考如下文档:

> - [PCI Agent 设备使用攻略](#F0)
>
> - [PCI Agent DMA 使用攻略](#F1)
>
> - [PCI Bridge 使用攻略](#F2)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="F0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### PCI Agent 设备使用攻略

![](/assets/PDB/HK/TH001678.png)

市面上常见的 PCI Agent 设备包括网卡、显卡、加速卡等，BiscuitOS 通过 QEMU 模拟了一个简单的 PCI 设备，并在 BiscuitOS 内提供了对应的 PCI 驱动，通过 PCI 驱动和用户空间开源工具描述 PCI Agent 的使用方法. QEMU 模拟了的 PCI Agent 设备 VendorID:DeviceID 为 1016:1413，其包含了一个 MM-BAR 和一个 IO-BAR，MM-BAR 对应的存储空间长度为 1MiB，IO-BAR 的 IO 端口长度为 128 字节.

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

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001664.png)
![](/assets/PDB/HK/TH001665.png)
![](/assets/PDB/HK/TH001666.png)

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

与 QEMU PCI 设备部署一致，借助 BiscuitOS 编译平台通过简单的几个命令就可以进行 PCI 设备驱动的部署，使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PCI: Peripheral Component Interconnect --->
          [*] PCI Common Device Driver Module --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-pci-device-driver-default/
# 源码下载
make download
# 源码编译
make 
# 模块安装
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run
{% endhighlight %}

> [BiscuitOS PCI Device Driver Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/PCIe/BiscuitOS-pci-device-driver)
>
> [BiscuitOS 独立应用程序实践攻略](/blog/Human-Knowledge-Common/#C2)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001664.png)
![](/assets/PDB/HK/TH001667.png)
![](/assets/PDB/HK/TH001668.png)
![](/assets/PDB/HK/TH001499.png)

当驱动部署成功在 BiscuitOS 上运行之后，注册一个 PCI 设备，PCI 设备的其中一块 BAR 映射到系统地址空间，而另外一个 BAR 映射到系统的 IO 空间，驱动程序分别对两块 BAR 空间进行了读写操作. 通过 lspci 工具可以看出 PCI Agent 设备的配置空间信息.

![](/assets/PDB/HK/TH001500.png)

查看系统的地址空间布局可以看到 PCI 设备的内存 BAR 映射到了系统地址空间的 \[0xFEA00000, 0xFEB00000), 这段区域在系统地址空间称为 "BiscuitOS-PCIe-MMIO".

![](/assets/PDB/HK/TH001501.png)

查看系统的 IO 空间布局可以看到 PCI 设备的 IO BAR 映射到了系统 IO空间的 \[0xC000, 0xC07F], 这段区域在系统 IO 空间称为 "BiscuitOS-PCIe-IO"。至此实践到此为止.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="F1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

#### PCI Agent DMA 使用攻略

![](/assets/PDB/HK/TH001678.png)

市面上常见的 PCI Agent 设备包括网卡、显卡、加速卡等，这些 PCI 设备都会将其内部数据拷贝到系统内存里，也会将系统内存的数据拷贝到设备内部，这样的数据交互称为 DMA。BiscuitOS 通过 QEMU 模拟了一个简单的 PCI-DMA 设备，并在 BiscuitOS 内提供了对应的 DMA 驱动和应用程序，通过 DMA 驱动和用户空间开源工具描述 PCI Agent 的使用方法. QEMU 模拟了的 PCI-DMA 设备 VendorID:DeviceID 为 1016:1314，其包含了一个 MM-BAR 和一块 2MiB 的内部存储空间，MM-BAR 对应的存储空间长度为 1MiB，MM-BAR 的 bitmap 如下图:

![](/assets/PDB/HK/TH001669.png)

###### QEMU 部署 PCI-DMA 模拟设备

QEMU 可以模拟 PCI 设备，BiscuitOS 提供了一个可实践的 PCI 设备 "BiscuitOS-DMA", 其是一个简单的 PCI 设备，设备挂载 PCI Bus 0 总线上。首先在 QEMU 源码目录下执行如下命令:

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
          [*] QEMU emulate DMA Device (BiscuitOS-DMA) --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-dma-device-QEMU-emulate-default/
make download
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/qemu-system/hw/BiscuitOS/
cp BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-dma-device-QEMU-emulate-default/BiscuitOS-dma-device-QEMU-emulate-default ./ -rf
{% endhighlight %}

> [BiscuitOS QEMU DMA Device Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/DMA/BiscuitOS-dma-device-QEMU-emulate)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001664.png)
![](/assets/PDB/HK/TH001670.png)
![](/assets/PDB/HK/TH001671.png)

在 BiscuitOS 的顶层目录执行 make menuconfig 之后，选择 QEMU PCI 设备源码和内核 PCI 驱动源码，然后执行 make 进行部署。接着进入 BiscuitOS-dma-device-QEMU-emulate-default 目录执行 make download 下载 QEMU DMA 设备源码。待源码下载完毕之后将其拷贝到 QEMU 源码 hw/BiscuitOS/ 目录下.

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/qemu-system/hw/BiscuitOS/
vi Makefile.objs

# Add context
common-obj-$(CONFIG_BISCUITOS_DMA) += BiscuitOS-dma-device-QEMU-emulate-default/

vi Kconfig

# Add context
source BiscuitOS-dma-device-QEMU-emulate-default/Kconfig
{% endhighlight %}

源码拷贝完毕之后，修改 hw/BiscuitOS 目录下 Makefile.objs 和 Kconfig，是 QEMU PCI-DMA 设备源码进入 QEMU 源码的编译体系中.

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/qemu-system/
vi default-configs/i386-softmmu.mak

# Add context
CONFIG_BISCUITOS_DMA=y

vi config-all-devices.mak

# Add context
CONFIG_BISCUITOS_DMA:=$(findstring y,$(CONFIG_BISCUITOS_DMA)y)
{% endhighlight %}

接着修改 QEMU 源码目录下的 "default-configs/i386-softmmu.mak", 启用 CONFIG_BISCUITOS_DMA 宏，然后修改 config-all-devices.mak 文件，使 CONFIG_BISCUITOS_DMA 宏始终为 Y，至此 QEMU PCI-DMA 设备源码已经添加到 QEMU 编译体系，接着只要重新编译源码设备就存在 QEMU 中，但还没能让 QEMU 启用该设备，需要修改 QEMU Command Line:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/
vi RunBiscuitOS.sh

# Add context
        -hda ${ROOT}/BiscuitOS.img \
+       -device BiscuitOS-DMA \
        -drive file=${ROOT}/Freeze.img,if=virtio \
{% endhighlight %}

在 RunBiscuitOS.sh 中添加 "-device BiscuitOS-DMA" 字段之后，系统启动之后就可以看到该设备。设备的 VendorID 和 DeviceID 分别是 0x1016:0x1314. 最后使用如下命令重新编译 QEMU 并在 BiscuitOS 中检查 QEMU 模拟的 PCI 设备:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/qemu-system/
./RunQEMU.sh -b
{% endhighlight %}

![](/assets/PDB/HK/TH001672.png)

从内核的 Dmesg 中可以看出内核在枚举 PCI 总线上的 PCI 设备时，已经探测到 QEMU 模拟的 PCI-DMA 设备 0x1016:0x1314. 那么接下来部署 PCI 设备对应的 Linux 驱动:

---------------------------------------

###### 部署 PCI-DMA 设备驱动和应用程序

与 QEMU PCI 设备部署一致，借助 BiscuitOS 编译平台通过简单的几个命令就可以进行 PCI-DMA 设备驱动的部署，使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PCI: Peripheral Component Interconnect --->
          [*] DMA Common Device Driver Module (PCIe) --->
          [*] DMA userland Application (BiscuitOS-DMA module) --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-dma-device-driver-default/
# 源码下载
make download
# 源码编译
make 
# 模块安装
make install
# Rootfs 打包
make pack
# 部署应用程序
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-dma-userland-default/
# 源码下载
make download
# 源码编译
make 
# 模块安装
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run
{% endhighlight %}

> [BiscuitOS DMA Device Driver Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/DMA/BiscuitOS-dma-device-driver)
>
> [BiscuitOS DMA Userland application Soruce Code on Gitee](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Device-Driver/DMA/BiscuitOS-dma-userland)
>
> [BiscuitOS 独立应用程序实践攻略](/blog/Human-Knowledge-Common/#C2)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001664.png)
![](/assets/PDB/HK/TH001667.png)
![](/assets/PDB/HK/TH001673.png)
![](/assets/PDB/HK/TH001675.png)
![](/assets/PDB/HK/TH001674.png)
![](/assets/PDB/HK/TH001676.png)

当驱动部署成功在 BiscuitOS 上运行之后，注册一个 PCI-DMA 设备，PCI 设备的其中一块 BAR 映射到系统地址空间，接着运行程序 BiscuitOS-dma-userland-default, 其功能是在主内存和 PCI-DMA 设备之间通过 DMA 传输数据，可以通过结果看到从设备读取了字符串 "This is BiscuitOS DMA module, welcome :)"，驱动程序分别对两块 BAR 空间进行了读写操作. 

![](/assets/PDB/HK/TH001677.png)

查看系统的地址空间布局可以看到 PCI-DMA 设备的内存 BAR 映射到了系统地址空间的 \[0xFEA00000, 0xFEB00000), 这段区域在系统地址空间称为 "BiscuitOS-DMA-MMIO".

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)







