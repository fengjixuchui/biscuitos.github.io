---
layout: post
title:  "PCI/PCIe Address Space on seaBIOS"
date:   2022-04-01 12:00:00 +0800
categories: [HW]
excerpt: PCI Memory Sapce.
tags:
  - Memory
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [BIOS PCI 基础知识](#A)
>
> - [BIOS PCI 软件架构](#B)
>
> - [BIOS PCI 代码逻辑](#C)
>
> - [BIOS PCI 实践与调试](#D)
>
> - [BIOS PCI 使用攻略](#F)
>
> - BIOS PCI 进阶研究

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

#### BIOS PCI 基础知识

![](/assets/PDB/HK/TH001481.png)

在 X86 架构中，BIOS 负责在内核运行之前为必备硬件设备进行初始化，其中包括对 PCI-Host、PCI-Bridge 以及 PCI 设备的初始化，其中包括 BDF 号的分配，以及设备配置空间的初始化，其中一个重要的任务是为 PCI 设备的 MEM-IO BAR 分配系统地址空间的 MMIO 地址和系统 IO 空间的端口号.

![](/assets/PDB/HK/TH001514.png)

通过 PCI 基础知识的了解，在 PCI 设备内部存在三种存储空间，第一种是以内存形式的寄存器空间(MEM-BAR)，第二种是以 IO 端口形式的寄存器空间(IO-BAR)，第三种则是设备内部的存储空间. 在 X86 架构的 PCI 总线架构中，由于系统地址空间与 PCI 总线空间是 1:1 的映射关系，那么系统可以通过 MMIO 机制将 PCI 地址空间的区域映射到系统地址空间，那么 CPU 就可以像访问内存一样访问 PCI 空间; 同理在 X86 架构的 PCI 总线架构中，由于系统 IO 空间与 PCI IO 空间也存在 1:1 映射的关系，那么 PCI IO 空间的端口可以映射到系统 IO 空间，那么 CPU 可以向访问普通 IO 端口一样访问 PCI IO 空间。有了上面的基础，BIOS 的任务就是为每个 PCI 设备的 MEM-IO BAR 分配系统空间地址(同时等于 PCI 空间地址)，或着分配系统 IO 端口(同时等于 PCI IO 端口).

![](/assets/PDB/HK/TH001486.png)

BAR 寄存器有些只读的 bit 是出厂前厂商固定好的 bit，固定 bit 包括 [3:0], 其中 Bit0 为 0 时表示对应的空间是一块内存，为 1 时表示对应的空间是一块 IO 空间. Bit2:1 构成的域用于表示对应的空间宽度，如果是 00，那么对应的空间宽度为 32 位，如果是 10，那么空间宽度为 64 位. Bit3 表示空间是否支持预读，当为 0 时表示不预读，而为 1 时表示为预读.

![](/assets/PDB/HK/TH001487.png)

系统上电之后，BAR 寄存器的低 [11:4] 位全为 0 而高位未知，此时 BIOS 向 BAR 寄存器写入全 1，然后读取 BAR 寄存器会得到 BAR 对应空间的长度，从 Bit4 到 Bit31 域的最低置位位表示 BAR 的大小，例如上图中域中最低置位位是 Bit20, 那么可操作的最低位为 20，则 BAR 可申请的地址空间为 1MiB(2^20).

![](/assets/PDB/HK/TH001488.png)

在获得 BAR 对应空间的长度之后，BIOS 将通过 MMIO 映射之后的地址对应的页帧号写入到 BAR 寄存器的高 20 位，例如上图 BAR 对应的内存通过 MMIO 映射到了系统地址空间的 0xFE000000, 那么将 0xFE000 写入到 BAR 寄存器高 20 位. 至此 PCI 设备 32 位内存映射到了系统地址空间. 对于 IO-BAR 和 64-Bit BAR 的初始化，这里不做细节介绍，另外对于 PCI 设备/PCI 桥 BDF 号分配的原理，请参考:

> [PCI BAR 初始化研究](#D4)
>
> [PCI 总线枚举分配 BDF 原理](#D3)

![](/assets/PDB/HK/TH001474.png)

每一个 PCI 设备功能包含 256 字节的配置空间，如果将所有的 PCI 设备功能的配置空间集合在一起，那么称为 PCI 配置空间(PCI Configuration Space), 并使用 BDF 进行寻址。由于每个 PCI 设备最多支持 8 中功能 (Function), 每一条 PCI 总线最多支持 32 个设备，每个 PCI 总线系统最多支持 256 个子总线，那么 PCI Configuration Space 的大小为: 256 (Bytes/Function) * 8 (Functioins/device) * 32 (device/Bus) * 256 (buses/system) = 16MiB。

![](/assets/PDB/HK/TH001475.png)

在 X86 架构中，只能通过 IO 端口方式才能访问 PCI Configuration Space。由于 X86 I/O 地址空间有限 (64KiB), 所以一般在 I/O Space 中都包含两个寄存器，一个指向要操作的内部地址，第二个存放读或写的数据。因此对于 PCI 周期来说，包含了两个步骤: 首先 CPU 对 PCI Address Port 的 [0xCF8, 0xCFB] 写入要操作的的寄存器地址，其中包括了总线号(Bus Numer)、设备号(Device Number)、功能号(Function Number) 和寄存器指针; 接着 CPU 向 PCI Data Port 的 [0xCFC, 0xCFF] 中写入读或写的数据.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### BIOS PCI 软件架构

![](/assets/PDB/HK/TH001517.png)

在 BIOS PCI 架构中，使用 struct pci_bus 数据结构用于描述一条 PCI BUS，另外使用 struct pci_device 数据结构用于描述一个 PCI 设备或者 PCI 桥. BIOS 在初始化之前只知道 PCI BUS 0 的存在，那么其从 PCI BUS 0 开始枚举，并按深度优先(DFS) 算法去探测 PCI 设备和其他 PCI 子总线。BIOS 在某条 PCI BUS 上探测到一个 PCI 设备或者 PCI 桥时，BIOS 会未其分配 BDF 号，并分配一个 struct pci_device 描述该设备，最后将新分配的 struct pci_device 添加到 PCIDevices hlist 链表里. 通过 PCI 总线的枚举，BIOS 知道其包含了几条 PCI 子总线和 PCI 设备或者 PCI 桥。

![](/assets/PDB/HK/TH001516.png)

BIOS 枚举完毕之后知道了 PCI 子总线的数量，那么动态分配一个 struct pci_bus 数据 busses[]. 接下来 BIOS 遍历 PCIDevices 链表上的 PCI 设备/PCI 桥，每当遍历一个 PCI 设备或 PCI 桥的时候，由于此时已经为 PCI 设备分配了 BDF 号，那么通过 BDF 获得 busses[] 数组中对应 PCI 总线的 struct pci_bus 数据结构，接着依次读取 PCI 设备的 BAR 寄存器，此时获得了 BAR 的类型和长度，那么接下来 BIOS 为每个 BAR 创建了一个数据结构 struct pci_region_entry, 并根据 BAR 的类型将 struct pci_region_entry 添加到对应 struct pci_bus 的 r[] 数组里，r[] 数组总共包含三个成员，分别是 r[PCI_REGION_TYPE_IO]、r[PCI_REGION_TYPE_MEM] 以及 r[PCI_REGION_TYPE_PREFMEM], 因此 struct pci_region_entry 会被添加到对应类型的 r[].list 链表里。

![](/assets/PDB/HK/TH001518.png)

BIOS 接下来先为所有的 busses[] 中的 r[PCI_REGION_TYPE_IO] 对应的 IO-BAR 分配 IO 端口，在 seaBIOS 中系统 IO 空间的分配布局如上图，\[0x0000, 0x1000] 区域分配给旧版本的 ISA、PCI config 端口以及 PCI root bus 使用, \[0x1000, 0xa000] 区域是可分配的区域. \[0xa000, 0xb000] 区域分配给热插拔设备的 IO 使用，例如热插拔 CPU 以及通过 ACPI 热插的 PCI 设备，最后该区域还给了 i440fx 的南北桥使用. \[0xb000, 0xc000] 区域则分配给电源管理模块的 IO 使用。最后 \[0xc000, 0x10000] 区域是可分配区域，通常分配给 PCI 设备的的 IO 使用. BIOS 首先遍历 busses.r[PCI_REGION_TYPE_IO] 链表，每当遍历到一个 struct pci_region_entry, 那么可以知道该 IO-BAR 的长度，那么 BIOS 从 0xC000 开始圈一块与 IO-BAR 长度相等的区域，然后将这个区域的首地址写入到 PCI 设备对应的 BAR 寄存器里; 当遍历下一个 struct pci_region_entry 时，BIOS 从上一次分配区域的结尾开始找一块当前 IO-BAR 大小的区域，同样将区域的首地址写入到当前 PCI 设备对应的 BAR 寄存器里，遍历完毕之后，所有 PCI 设备的 IO-BAR 都被设置新的地址，那么 PCI IO Space 已经映射到系统的 IO Space.

![](/assets/PDB/HK/TH001521.png)

当 BiscuitOS 运行之后，可以通过 /proc/ioports 命令可以查看系统 IO 空间，可以看到 PCI root bus 占用了 \[0x0000, 0x0cf7] 的区域, 

![](/assets/PDB/HK/TH001519.png)

BIOS 接着为所有 busses[] 中的 r[PCI_REGION_TYPE_MEM] 对应的 MEM-BAR 分配 MMIO 地址，在 seaBIOS 中系统地址总线的 PCI Memory 空间分配 32 位低地址部分和 64 位高地址部分。如果 MEM-BAR 是 32 位区域，那么其可以在 pcimem_start 到 pcimem_end 对应的区域进行分配，其具体范围是 \[0xE0000000, 0xFEC00000], BIOS 首先遍历 busses.r[PCI_REGION_TYPE_MEM] 链表，并计算出其链表上所有 PCI 设备 MEM-BAR 占用的总内存，然后从 PCI 区域的顶端向下分配出 PCI 占用的总内存，并将该区域的起始地址作为 PCI 映射的基地址。接着从链表第一个 PCI 设备开始分配指定长度的区域，并将该区域的起始地址写入 MEM-BAR 寄存器，接着下一个 PCI 设备从该区域的结束地址开始分配. 遍历完毕之后，所有的 PCI 设备的 MEM-BAR 都被设置了新的 MMIO 地址，那么 PCI MEM Space 已经被映射到系统地址空间.

![](/assets/PDB/HK/TH001520.png)

当 BiscuitOS 运行之后，可以通过 "cat /proc/iomem" 查看系统地址空间的布局，可以看到 \[0x2000000, 0xFEC00000] 区域没有物理内存，并且 \[0xE0000000, 0xFEC00000] 区域用于映射 PCI MEM-BAR 的 MMIO，因此可以看出 PCI 设备的 MEM-BAR 占据了 \[0xE0000000, 0xFEC00000] 区域的顶部，并向 0xE0000000 扩张. 

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

#### BIOS PCI 代码逻辑

![](/assets/PDB/HK/TH001522.png)

seaBIOS 处理 PCI 架构的核心代码流程如上图，其使用 struct pci_bus 数据结构描述一条 PCI Bus，使用 struct pci_device 数据结构描述一个 PCI 设备或者 PCI 桥，使用 struct pci_region_entry 数据结构描述 PCI 设备的一个 BAR, 最后使用 struct pci_region 数据结构维护所有 PCI 设备类型相同的 BAR. 具体数据结构和函数分析如下:

> - [struct pci_bus](#C0)
>
> - [struct pci_device](#C1)
>
> - [struct pci_region_entry](#C2)
>
> - [struct pci_region](#C3)

> - [pci_setup](#C4)
>
> - [pci_probe_host](#C5)
>
> - [pci_bios_init_bus](#C6)
>
> - [pci_probe_devices](#C7)
>
> - [pci_bios_init_platform](#C8)
>
> - [pci_bios_check_devices](#C9)
>
> - [pci_bios_map_device](#CA)
>
>   - [pci_bios_init_root_regions_io](#CB)
>
>   - [pci_bios_init_root_regions_mem](#CC)
>
>   - [pci_region_map_entry](#CD)
>
> - [pci_bios_init_devices](#CE)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="C0">struct pci_bus</span>

![](/assets/PDB/HK/TH001523.png)

**r**: struct pci_bus 数据结构用于描述一条 PCI Bus，其 r[] 成员是一个 struct pci_region 数据结构数组, 且 r[] 数组包含 PCI_REGION_TYPE_COUNT 个成员，分别是:

* r[PCI_REGION_TYPE_IO] 维护总线上所有设备的 IO-BAR
* r[PCI_REGION_TYPE_MEM] 维护总线上所有设备的 MEM-BAR
* r[PCI_REGION_TYPE_PREFMEM] 维护总线上所有设备的 PREFMEM-BAR

![](/assets/PDB/HK/TH001516.png)

BIOS 中使用 struct pci_region_entry 数据结构描述一个 BAR，那么 r[] 数据的成员将相同属性的 BAR 对应的 struct pci_region_entry 维护在 r[].list 链表上，BIOS 通过 r[] 成员可以遍历同一条总线上相同属性的 BAR，并可获得总线上所有该属性 BAR 占用内存的大小，最后通过 struct pci_region_entry 数据结构可以找到 BAR 对应的 PCI 设备，此时 struct pci_device 描述一个 PCI 设备或者 PCI 桥.

**bus_dev**: struct pci_bus 数据结构用于描述一条 PCI Bus, 每条 PCI Bus 都用自己的 PCI 桥设备，对于 PCI Bus 0 则是对应的 PCIHost，那么 bus_dev 成员指向 PCI 总线对应的 PCI 桥设备. BIOS 使用 struct pci_device 数据结构描述一个 PCI 设备或者 PCI 桥，那么 bus_dev 成员是一个 struct pci_device 指针.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="C1">struct pci_device</span>

![](/assets/PDB/HK/TH001524.png)

BIOS 使用 struct pci_device 数据结构描述一个 PCI 设备或者一个 PCI 桥，PCI 设备作为 PCI 总线架构中的核心构成，其挂接在 PCI 总线上，PCI 总线由 PCI 桥进行管理，因此形成树形拓扑结构. PCI 设备或者 PCI 桥在 PCI 总线拓扑结构中具有唯一的标识 BDF。

**bdf** 成员用于指明 PCI 设备或者 PCI 桥的 BDF 号，其由 BIOS 在 PCI 总线枚举阶段分配. **rootbus** 成员用于记录上一级 PCI Bus 的编号. **vendor** 成员用于记录 PCI 设备或者 PCI 桥的产商 VendorID，而 **device** 成员用于记录 PCI 设备或者 PCI 桥的产商 DeviceID. **class** 成员则记录 PCI 设备或者 PCI 桥设备配置空间 Class 信息，Class 信息的另外两个字段由 **prog_if** 和 **revision** 成员维护. **header_type** 成员用于记录 PCI 设备或者 PCI 桥的 Header 信息. **secondar_bus** 成员在 PCI 桥中用于记录其下一级 PCI Bus 号。

![](/assets/PDB/HK/TH001517.png)

所有的 struct pci_device 数据结构通过 **node** 成员链接到 PCIDevices 链表上，因此 BIOS 使用 PCIDevices 链表维护了所有 PCI 设备或 PCI 桥. **parent** 成员指向了上一级 PCI Bus 的 PCI 桥.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="C2">struct pci_region_entry</span>

![](/assets/PDB/HK/TH001525.png)

BIOS 使用 struct pci_region_entry 数据结构描述 PCI 设备或 PCI 桥的一个 BAR 空间. **dev** 成员用于指向其所属的 PCI 设备，在 BIOS 中 PCI 设备或 PCI 桥通过 struct pci_device 数据结构进行描述，因此 dev 成员是一个 struct pci_device 指针. **bar** 成员用于指明其是 PCI 设备或者 PCI 桥的第几个 BAR 寄存器. **size** 成员用于指明 BAR 空间的长度. **align** 成员指明了 BAR 空间的起始地址必须对齐的方式. **isa64** 成员用于指明 BAR 是否用于描述 64 位的 BAR 空间。

![](/assets/PDB/HK/TH001516.png)

**type** 成员用于指明 BAR 空间的类型，PCI 设备或者 PCI 桥支持的 BAR 空间类型有 IO/MEM/PREFMEM 三种，BIOS 使用 struct pci_bus 数据结构描述一条 PCI Bus，并将总线上所有 PCI 设备或者 PCI 桥的 BAR 按类型进行分类，并将同类型的 BAR 对应的数据结构 struct pci_region_entry 维护在其 r[] 数组成员里， 因此 **node** 成员用于将其加入到对应类型的链表里. 

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="C3">struct pci_region</span>

![](/assets/PDB/HK/TH001526.png)

BIOS 使用 struct pci_region 数据结构描述一类 BAR 空间集合，目前 BIOS 支持三种类型的 BAR，分别是 IO/MEM/PREFMEM, BIOS 然后使用 struct pci_bus 数据结构描述一条 PCI Bus，并维护了 r[] 数组，数组的成员就是 struct pci_region, 那么从 PCI Bus 角度来看其将维护的所有 PCI 设备或者 PCI 桥的 BAR 按类型维护在 r[] 数组里。struct pci_region 数据结构只包含了两个成员，**base** 成员用于指明在该总线上相同类型的 BAR 集合映射到系统总线的基地址.

![](/assets/PDB/HK/TH001516.png)

BIOS 使用 struct pci_region_entry 数据结构描述PCI 设备或者 PCI 桥的一个 BAR，那么 **list** 成员作为链表头，将总线上所有相同类型的 BAR 的 struct pci_region_entry 维护在同一个链表里.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="C4">pci_setup</span>

![](/assets/PDB/HK/TH001527.png)

pci_setup() 函数的主要作用是枚举 PCI 总线，找到 PCIHOST 和所有的 PCI Bus，以及 PCI Bus 上挂载的所有 PCI 设备和 PCI 桥，然后为每个 PCI 设备和 PCI 桥分配 BDF 号，最后将 PCI 设备或 PCI 桥的 BAR 空间映射到系统地址空间或系统 IO 空间. 其实现逻辑如下:

<span id="C5"></span>
![](/assets/PDB/HK/TH001540.png)

在枚举 PCI 总线之前，CPU 只知道 PCIHOST 和 PCI Bus 0 的存在，其他一概不止，那么此时 PCIHOST 的 BDF 默认为 0，那么 BIOS 在 117 行向 PORT_PCI_CMD 端口写入 "0x80000000", 并根据 118 行从 PORT_PCI_CMD 寄存器读入的值进行判断，如果此时寄存器的值依旧为 0x80000000, 那么证明 PCIHOST 存在; 反之不存在报 "Detected non-PCI system" 信息并退出.

<span id="C6"></span>
![](/assets/PDB/HK/TH001541.png)

pci_bios_init_bus() 函数的主要作用是初始化 PCIHOST 主桥，BIOS 首先在 52 行调用 romfile_loadint() 函数加载 "etc/extra-pci-roots" 文件，然后调用 pci_bios_init_bus_rec() 函数初始化 PCIHOST 主桥。初始化完毕之后如果 extraroots 存在，那么函数将 pci_bus 数加一之后再次调用 pci_bios_init_bus_rec() 函数初始化 PCIHOST.

<span id="C7"></span>
![](/assets/PDB/HK/TH001542.png)

pci_probe_devices() 函数的主要作用是探测 PCI 总线所有的 PCI 设备和 PCI 桥。BIOS 首先在 24 行创建了一个 struct pci_device 的指针数组 busdevs[], 接着在 26 行将 pprev 指向了 PCIDevices 链表头 first 成员，接着 BIOS 在 29 行使用 while 信息准备遍历所有的 PCI Bus，并用 MaxPCIBus 变量存储系统 PCI Bus 的数量。在 while 循环里，BIOS 在 32 行使用 foreachbdf() 函数遍历每一条 PCI BUS 上存在 PCI 设备或者 PCI 桥的 BDF。每当遍历到一个有效的 BDF，那么其 PCI 设备或 PCI 桥是存在的，那么 BIOS 在 34 行新分配一个 struct pci_device 数据结构用于描述找到的 PCI 涉笔或者 PCI 桥。接下来 BIOS 在 40 行将 struct pci_device 数据结构添加到 PCIDevices 链表上:

![](/assets/PDB/HK/TH001517.png)

遍历完毕之后，所有的 PCI 设备或者 PCI 桥对应的 struct pci_device 数据结构维护在 PCIDevices 链表上. BIOS 接着在 45-56 行为 struct pci_device 指定 parent，此处 parent 即是上一层 PCI Bus 号。接下来 BIOS 在 59-81 行通过从 PCI 设备或 PCI 桥的配置空间读取内容并存储到 struct pci_device 指定的成员里。另外在 71-77 行如果探测到一个 PCI 桥，那么需要配置子 PCI Bus 对应成员的信息。函数执行完毕之后，struct pci_device 就能维护一个 PCI 设备或 PCI 桥.

<span id="C8"></span>
![](/assets/PDB/HK/TH001543.png)

pci_bios_init_platform() 函数的作用是为没有设置 VendorID 的 PCI 设备或 PCI 桥进行设置。BIOS 在 21 行调用 foreachpci() 函数遍历所有的 PCI 设备或者 PCI 桥，如果其对应的 struct pci_device 数据结构 vendor 成员没有设置，那么其根据 pci_platform_tbl[] 数组提供的方法进行设置.

![](/assets/PDB/HK/TH001544.png)

回到 pci_setup() 函数，BIOS 使用 struct pci_bus 数据结构描述一条 PCI Bus，并且 MaxPCIBus 变量通过枚举之后知道了系统 PCI Bus 的数量，那么 BIOS 在 70 行为每条 PCI Bus 分配一个 struct pci_bus 数据结构，并使用 busses 局部变量进行维护。

<span id="C9"></span>
![](/assets/PDB/HK/TH001545.png)

BIOS 使用 struct pci_region_entry 数据结构描述一个 BAR 空间，pci_bios_check_devices() 函数的主要作用是探测每个 PCI 设备 BAR 空间，并将 BAR 与一个 struct pci_region_entry 进行挂钩。BIOS 首先在 59 行调用 foreachpci() 函数遍历所有的 PCI 设备或 PCI 桥，每遍历到一个 PCI 设备或 PCI 桥时，BIOS 在 63 行通过pci_bdf_to_bus() 函数获得 struct pci_device 对应的 struct pci_bus, 接着 BIOS 在 70 行依次遍历 PCI 设备或者 PCI 桥的 BAR 空间，每遍历一个 BAR 时调用 pci_bios_get_bar() 函数获得 BAR 的类型、长度和位宽信息，接着 BIOS 在 82 行调用 pci_region_create_entry() 函数为 BAR 绑定一个 struct pci_region_entry，另外 struct pci_bus 根据 BAR 的类型将 struct pci_region_entry 维护在其 r[] 数组里，数组的成语是相同类型 BAR 的集合. 最后构成如下架构:

![](/assets/PDB/HK/TH001516.png)

每条 PCI Bus 对应的 struct pci_bus 的 r[] 数组将相同类型的 struct pci_region_entry 维护在同一个链表上，因此 BIOS 可以快速遍历指定 PCI Bus 某种类型的 BAR 信息.

<span id="CA"></span>
![](/assets/PDB/HK/TH001546.png)

pci_bios_map_devices() 函数的主要作用是为每个 MEM-BAR/PREFMEM-BAR 空间分配系统地址空间地址，为 IO-BAR 空间分配系统 IO 端口. BIOS 首先在 95 行调用 pci_bios_init_root_regions_io() 函数统计 IO-BAR 总长，并为整个 IO-BAR 分配基地址。BIOS 在 99 行调用 pci_bios_init_root_regions_mem() 函数统计 MEM-BAR 和 PREFMEM-BAR 的总长，并为整个 MEM-BAR 和 PREFMEM-BAR 分配各自区域的基地址。最后 BIOS 在 140 行调用 pci_region_map_entries() 函数为每个 BAR 分配基地址.

<span id="CB"></span>
![](/assets/PDB/HK/TH001547.png)

pci_bios_init_root_regions_io() 函数首先在 1000 行获得 PCI 总线所有 IO-BAR 集合，然后在 1001 行调用 pci_region_sum() 函数计算 IO-BAR 集合的总长，接着在系统 IO 空间的找到一块合适的区域用于映射 PCI IO-BAR，从 990-998 行的注释可以知道系统 IO 地址空间的大概布局，其中 \[0xC000, 0x10000] 区域可用于映射 PCI IO-BAR，如果集合总长小于 0x4000, 那么 BIOS 将 IO-BAR 集合的基地址设置为 0xC000; 如果集合总长小于 "pci_io_low_end - 0x1000", 那么 BAR-IO 集合的基地址设置为 0x1000; 如果 IO-BAR 集合总长不符合以上要求，那么直接返回 -1. 最后函数打印了 IO-BAR 集合可以映射的区域.

<span id="CC"></span>
![](/assets/PDB/HK/TH001548.png)

pci_bios_init_root_regions_mem() 函数同时计算 MEM-BAR 和 PREFMEM-BAR 集合的长度和基地址，函数首先在 18-19 行将 r_end 指向 PREFMEM-BAR 集合，r_start 指向 MEM-BAR 集合。函数接着在 21 行进行对齐判断，默认情况下 MEM-BAR 的对齐粒度不小于 PREFMEM-BAR。如果 MEM-BAR 的对齐粒度小于 PREFMEM-BAR, 那么交换一下 r_start 和 r_end，以此确保小粒度对齐占用了系统 PCI Memory 空间的顶部。假设 MEM-BAR 的对齐粒度小于 PREFMEM-BAR, 那么 r_end 指向 MEM-BAR, r_start 则指向 PREFMEM-BAR. 函数接着在 26 行获得 MEM-BAR 集合的总长，然后将集合基地址设置为 "pcimem_end - sum" 区域的起始地址，这样 MEM-BAR 占据了系统地址空间 PCI Memory 的顶部; 同理函数在 29 行计算 PREFMEM-BAR 集合的长度，然后将基地址设置为 "MEM-BAR 基地址 - sum" 区域的起始地址. 函数最后在 33 行对区域覆盖进行检测，如果发现两个区域覆盖了，那么直接返回 -1.

<span id="CD"></span>
![](/assets/PDB/HK/TH001549.png)

pci_region_map_entries() 函数的作用是为 BAR 分配映射地址。参数 busses 表示 BAR 所属的 PCI Bus，参数 r 表示 BAR 所属的类型集合。BIOS 使用 struct pci_bus 描述一条 PCI Bus，然后将所有类型相同的 BAR 都维护在 struct pci_bus 的 r[] 数组里，函数在 1081 行遍历某种类型的 BAR 集合时，由于 pci_bios_init_root_regions_mem/io() 函数已经分别计算出 IO-BAR、MEM-BAR 和 PREFMEM-BAR 集合的起始地址，那么 BIOS 在 1082 行将 r->base 作为遍历到 BAR 的基地址，然后在 1082 行将 r->base 增加遍历到 BAR 的长度，那么下一个 BAR 的基地址就确认了，接着 BIOS 在 1087 行调用 pci_region_map_one_entry() 函数配置 BAR 的地址，其函数实现可以看出 1047-1053 行用于设置一个 BAR 的地址，而 1057-1073 行则设置非透明桥的 BASE_ADDRESS 寄存器和 Limit 寄存器.

<span id="CE"></span>
![](/assets/PDB/HK/TH001550.png)

pci_setup() 函数最后调用 pci_bios_init_devices() 函数初始化每个 PCI 设备或 PCI 桥，每遍历一个 PCI 设备时，其调用 pci_bios_init_devices() 函数对 PCI 设备或 PCI 桥进行初始化，BIOS 在 408 行读取设备的 PCI_INTERRUPT_PIN 寄存器的值，如果值不为 0，那么向寄存器写入 pci_slot_get_irq() 值，并在 412 行调用 pci_init_device() 函数调用再次设置 VendorID 和 DeviceID 寄存器。BIOS 接着在 415 行向配置空间的 PCI_COMMAND 寄存器写入 PCI_COMMAND_IO、PCI_COMMAND_MEMORY 和 PCI_COMMAND_SERR 三个标志。最后如果是 PCI 桥，那么向其配置空间 PCI_BRIDGE_CONTROL 寄存器写入 PCI_BRIDGE_CTL_SERR 标志.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

#### BIOS PCI 实践与调试

市面上常见的 PCI/PCIe 设备包括网卡、显卡、加速卡等，如果想实践 PCI 设备，那么需要购买相应的设备和主机，这样花费昂贵且困难程度很大。BiscuitOS 平台是一个基于 QEMU 的虚拟化平台，可以利用 BiscuitOS 构建不同的架构的虚拟机，其底层基于 QEMU 和 KVM。为了让广大开发者能否从实践中体会 BIOS 阶段 PCI 构建逻辑，BiscuitOS 通过 QEMU 模拟了一个带 PCI 的 seaBIOS，并提供了丰富的调试手段，开发者可以通过实践亲自感受 BIOS 初始化 PCI 的架构逻辑. 另外由于 BIOS 存在与 IA32 和 AMD64 平台，因此开发者在实践之前需要部署其中一个平台的开发环境，本文以 Linux 5.0 内核和 x86_64 平台进行案例讲解，其他版本可以举一反三。开发者首先部署开发环境，可以参考如下:

> [BiscuitOS X86_64 Linux 5.0 开发环境搭建](/blog/Linux-5.0-x86_64-Usermanual/)

![](/assets/PDB/HK/TH001467.png)

BiscuitOS 运行在 QEMU 模拟的 X86_64 平台，该平台硬件架构采用了 Intel 440FX，一个典型的 PCI 总线系统如上图的以 Intel 440FX PMC(PCI and Memory Controller) 为北桥芯片，PIIX (PCI ISA Xcelerator) 为南桥芯片组成的芯片组。北桥芯片 PMC 用于连接主板上的高速设备，向上提供了连接处理器的 Host 总线接口，可以连接多个处理器，向下主要提供了连接内存 DRAM 的接口和连接 PCI 总线系统的 PCI 总线接口，通过该 PCI root port 扩展出整个 PCI 设备树，包括 PIIX 南桥芯片. PIIX 南桥芯片则用于连接主板上的低速设备，主要包括 IDE 控制器、DMA 控制器、硬盘、USB 控制器、SMBus 总线控制器，并且提供了 ISA 总线用于连接更多的低速设备，如键盘、鼠标、BIOS ROM 等. 接下来通过 BiscuitOS 部署 HOST-SeaBIOS 组件，其在 BiscuitOS 平台上部署逻辑如下:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] KVM --->
          [*] Host seaBIOS for BiscuitOS --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-seaBIOS-host-default/
make download
tree
{% endhighlight %}

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001528.png)
![](/assets/PDB/HK/TH001529.png)
![](/assets/PDB/HK/TH001530.png)

代码部署完毕之后，可以在 BiscuitOS-seaBIOS-host-default 目录下看到一个软连接目录 BiscuitOS-seaBIOS-host-default，其指向了 seaBIOS 源码的真实目录，可以看到其位于 qemu-system/qemu-system/roms/seabios 目录. 接下来可以参考如下命令进行 seaBIOS 实践:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-seaBIOS-host-default/
## 编译源码
make
## 安装 seaBIOS 并重新生成 rootfs
make install
make pack
## 运行最新的 seaBIOS
make run
## 清除编译
make clean
{% endhighlight %}

![](/assets/PDB/HK/TH001531.png)

seaBIOS 源码编译成功之后，接下来就是运行 seaBIOS，由于 BiscuitOS 启动过程中，BIOS 基本不输出信息到串口，为了调试方便，开发者可以参考下面章节进行 seaBIOS 调试:

#### seaBIOS 调试

BiscuitOS 已经集成了 seaBIOS 调试工具，开发者只需简单的几个命令就可以实现 seaBIOS 的调试，无需复杂低效的部署，本节先介绍调试方法，最后再介绍调试原理。开发者在 BiscuitOS-seaBIOS-host-default 目录下需要准备两个终端，第一个终端输入 "make debug-client" 命令用于获得 seaBIOS 中打印的 log，另外一个终端则负责编译和运行带调试信息的 seaBIOS，其命令是: "make debug". 运行如下图:

![](/assets/PDB/HK/TH001532.png)

第一个终端输入 "make debug" 之后，BiscuitOS 将编译 seaBIOS 源码并运行，由于 seaBIOS 运行完毕之后就直接运行内核，因此该终端基本看不到 seaBIOS 打印的消息。接着第二个终端在第一个终端输入命令之前先输入命令 "make debug-client", 那么可以看到第二个终端中打印了 seaBIOS 运行过程中的信息:

![](/assets/PDB/HK/TH001533.png)

可以从第二个终端看到很多 seaBIOS 启动信息，那么接下来就是在 seaBIOS 源码中，使用 dprintf() 函数进行信息到处，其使用格式如下:

{% highlight bash %}
## 使用格式
dprintf(debug_level, ...)

## e.g.
dprintf(1, "Hello BiscuitOS %d\n", 1024);
{% endhighlight %}

dprintf() 支持不同 debug_level 的信息输出，debug_level 的取值范围从 1 到 9，BiscuitOS 支持的 debug_level 为 1，因此只有 "dprintf(1, ...)" 的代码语句才会输出信息到第二个端口. seaBIOS 的调试到此为止，接下来分析一下 BiscuitOS seaBIOS 调试实现原理。

![](/assets/PDB/HK/TH001534.png)

在 BiscuitOS-seaBIOS-host-default 目录下输入 "make debug" 之后，RunBiscuitOS.sh 脚本中会向 QEMU 启动命令中添加 "-bios" 命令，该命令后面跟着该编译 seaBIOS 源码生成的 bios.bin 位置，另外 "-chardev pipe" 命令指向了 seaBIOS 源码目录下的一个管道文件 BiscuitOS-pipe, 该管道文件会在执行 "nmake debug-client" 命令的同时通过命令 "mkfifo" 命令创建，最后 "-device isa-debugcon" 命令新建立了一个设备 isa-debugcon, 该端口提供了一些端口供 Debug 信息的输出。另外第二个端口执行了 "make debug-client" 命令之后，其会在 seaBIOS 目录下通过 mkfifo 命令创建一个管道文件 "BiscuitOS-pipe", 然后运行 seaBIOS 源码目录下的脚本:

{% highlight bash %}
## Debug-Stub
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-seaBIOS-host-default/
python BiscuitOS-seaBIOS-host-default/scripts/readserial.py -nf BiscuitOS-seaBIOS-host-default/BiscuitOS-pipe
{% endhighlight %}

第二个端口通过 "Ctrl-C" 进行退出，如果不退出其会一直监听 seaBIOS 打印的 debug 信息. 至此 seaBIOS 调试到此为止.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="F"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### BIOS PCI 使用攻略

> - [PCI-CMD/PCI-DATA IO-Port 使用](#F0)
>
> - [PCI 设备配置空间操作](#F1)
>
> - [PCI 桥配置空间操作](#F2)
>
> - [遍历所有的 PCI 设备/PCI 桥](#F3)
>
> - [遍历指定 PCI bus 上存在的 BDF](#F4)
>
> - [读取 PCI 设备/PCI 桥的 BAR 寄存器](#F5)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="F0">PCI-CMD/PCI-DATA IO-Port 使用</span>

![](/assets/PDB/HK/TH001535.png)

在 X86 架构中系统 IO 空间是独立与系统地址空间的，因此 CPU 访问 IO 空间就需要特殊的指令 "in/out"，另外 0x0CF8 端口作为读取 PCI Configuration Space 的 CMD 端口，而 0x0CFC 端口作为读取 PCI Configuration Space 的 Data 端口，两个端口的格式定义如下:

![](/assets/PDB/HK/TH001475.png)

因此在使用 PCI 端口时，函数通过 bus、device、function 和 reg 构成了 PCI CMD 写入的值，如果只是为了 PCI Data 读取某个寄存器的值，那么 0xCFBH 的最高位应该保持清零态; 反之如果是为了配置某个寄存器，那么 0xCFBH 的最高位应该保持置位. 当通过 outl() 向 PCI CMD 写入值之后，可以通过 inl() 函数从 PCI DATA 读入指定寄存器的值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="F1">PCI 设备配置空间操作</span>

![](/assets/PDB/HK/TH001472.png)

每个 PCI 设备都有一个 BDF 号，通过 BDF 号可以在 PCI configuration Space 中找到一端配置空间，上图为 PCI 设备的配置空间寄存器布局，seaBIOS 提供 pci_config_readX 和 pci_config_writeX 函数对配置空间进行读写操作，X 代表不同的位宽，可以是单字节、双字节和四字节.

![](/assets/PDB/HK/TH001536.png)

pci_config_write() 函数接收三个参数，第一个参数是 PCI 设备的 BDF 号，第二个参数是配置空间里寄存器的地址，第三个值为寄存器需要写入的值. pci_config_read() 函数则接收两个参数，第一个参数是 PCI 设备的 BDF 号，而第二个参数是配置空间里寄存器的地址. 两个函数都提供了不同位宽操作，开发者可以根据需求自行选择.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="F2">PCI 桥配置空间操作</span>

![](/assets/PDB/HK/TH001473.png)

每个 PCI 桥都有一个 BDF 号，通过 BDF 号可以在 PCI configuration Space 中找到一端配置空间，上图为 PCI 桥的配置空间寄存器布局，seaBIOS 提供 pci_config_readX 和 pci_config_writeX 函数对配置空间进行读写操作，X 代表不同的位宽，可以是单字节、双字节和四字节.

![](/assets/PDB/HK/TH001536.png)

pci_config_write() 函数接收三个参数，第一个参数是 PCI 桥的 BDF 号，第二个参数是配置空间里寄存器的地址，第三个值为寄存器需要写入的值. pci_config_read() 函数则接收两个参数，第一个参数是 PCI 桥的 BDF 号，而第二个参数是配置空间里寄存器的地址. 两个函数都提供了不同位宽操作，开发者可以根据需求自行选择.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="F3">遍历所有的 PCI 设备/PCI 桥</span>

![](/assets/PDB/HK/TH001517.png)

BIOS 使用 struct pci_device 数据结构描述一个 PCI 设备或者一个 PCI 桥，并将所有的 struct pci_device 数据结构维护在 PCIDevices 链表里，因此可以通过 PCIDevice 链表遍历所有的 PCI 设备和桥.

![](/assets/PDB/HK/TH001537.png)

BIOS 提供了 foreachpci() 函数实现遍历所有的 PCI 设备或 PCI 桥的功能，其使用如上图，只需传入一个 struct pci_device 指针即可.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="F4">遍历指定 PCI bus 上存在的 BDF</span>

![](/assets/PDB/HK/TH001470.png)

在 PCI 架构中，CPU 与 PCIHOST 主桥相连，PCIHOST 主桥之下是 PCI BUS 0，PCI BUS 0 上挂载不定数量的 PCI 设备和 PCI 桥，PCI 桥的下边是一条 PCI Bus，PCI Bus 上又挂载着不定数量的 PCI 设备和 PCI 桥，以此类推，那么 PCI 总线的树形结构由此构成。在 PCI 总线架构里，每个 PCI 设备或 PCI 桥都有唯一的 BDF 号，并且每个 BDF 对应 PCI Configuration Space 中一段配置空间，PCI 总线通过 BDF 号寻址 PCI 设备或者 PCI 桥。PCI 总线探测一个 PCI 设备或者 PCI 桥是否存在的原理是向 BDF 对应的配置空间 VENDOR ID 寄存器写入值，如果返回非 0xFFFF，那么设备存在, 反之不存在.

![](/assets/PDB/HK/TH001538.png)

BIOS 提供了 foreachbdf() 函数实现遍历指定 PCI Bus 上可用的 BDF 号，其接收两个参数，参数一用于记录 PCI Bus 存在的 BDF 号，参数二用于指定 PCI Bus 号。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="F5">PCI-CMD/PCI-DATA IO-Port 使用</span>

![](/assets/PDB/HK/TH001486.png)

PCI 设备或 PCI 桥都有唯一的 BDF 号，BDF 在 PCI configuration Space 中对应一段配置空间，其配置空间存在 BAR 寄存器，用于指明 PCI 设备或 PCI 桥的 MEM-IO 映射到系统地址空间的地址以及系统 IO 空间的端口号。BAR 寄存器的内部布局如上图，可以从中获得 BAR 的类型: MEM-BAR、IO-BAR、PREFMEM-BAR, 以及判断 BAR 是 32Bit-BAR 还是 64Bit-BAR。最后对未初始化的 BAR 可以向其写入全 1，可以获得 BAR 空间的长度.

![](/assets/PDB/HK/TH001539.png)

BIOS 提供了 pci_bios_get_bar() 函数可以获得指定 PCI 设备或者 PCI 桥的 BAR 信息，其接收五个参数，第一个参数用于指明 PCI 设备/PCI 桥，第二个参数用于指明 BAR 的索引，第三个参数用于存储 BAR 的类型，第四个参数用于存储 BAR 的长度，最后一个参数用于存储 BAR 是否为 64Bit-BAR。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

