---
layout: post
title:  "内存基础知识"
date:   2022-03-18 12:00:00 +0800
categories: [HW]
excerpt: Memory Hardware.
tags:
  - Memory
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [简介](#M)
>
> - [CPU 架构基础](#A)
>
>   - [逻辑核与物理核](#A0)
>
>   - [CPU Socket](#A1)
>
>   - [CPU Die](#A2)
>
>   - [CPU Package](#A3)
>
>   - [CPU 系统信息](#A4)
>
> - [存储器子系统](#B)
>
>   - [Register](#B5)
>
>   - [CACHE](#B6)
> 
>   - [ROM 和 RAM](#B0)
>
>   - [DIMM 内存条](#B1)
>
>   - [内存颗粒内部结构](#B2)
>
>   - [内存编址](#B3)
>
>   - [内存寻址](#B4)
>
> - [物理地址空间](#C)
>
>   - [DDR 域](#C0)
>
>   - [MMIO and PIO](#C1)
>
>   - [X86 物理地址布局](#C2)
>
>   - [页帧/页号/物理页/Page](#C3)
>
>   - [ZONE/NUMA NODE](#C4)
>
>   - [Memory Model](#C5)
>
> - [虚拟内存](#D)
>
>   - [页表: Page Table](#D0)
>
>   - [缺页: Page Fault](#D1)
>
>   - [用户空间布局](#D2)
>
>   - [内核空间布局](#D3)
>
>   - [内存映射](#D4)
>
> - [X86 架构内存管理](#E)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="M"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### 简介

![](/assets/PDB/HK/TH001438.png)

内存作为计算机架构运行的必要硬件设备之一被开发者熟知，作为软件开发者更多接触的是内存的大小、NUMA NODE、Zone、物理页等概念，而对于硬件开发者来说更多接触的是内存条、DRAM、PMEM 等硬件设备。因此从不同角度对内存都有不同的解读，本文用于帮助软件开发者和硬件开发者打破认知防线，通熟易懂的语言将内存进行讲解，以便开发者在日后的开发对内存有一个整体的认识。

##### 软件角度看内存架构

![](/assets/PDB/HK/TH001434.png)

公元前 5 世纪中国人发明了算盘，用于计算生意贸易的结算，算盘相当于是最早的计算机。随着科技不断的发展，1946 年冯诺依曼提出了计算机的基本原理 **存储程序和程序控制**，以此奠定了现代计算机的基础，人们称这种架构为冯诺依曼架构 (Von Neumann Architecture)。在冯诺依曼架构中提出了由二进制代替十进制的思想，采用存储程序思想，并且将计算机逻辑分为控制器、运算器、存储器、输入设备、输出设备五大部分，其中控制器与运算器组成了大家熟知的 CPU, 存储器一般为内存，磁盘也属于存储器，不过磁盘和内存的存储形式有所不同。

![](/assets/PDB/HK/TH001435.png)

在冯诺依曼架构中，程序在执行前需要将程序和数据放入到存储器中 (PC 上是内存)，当程序执行时把要执行的程序和要处理的数据顺序从存储器中取出指令一条一条的执行，称为顺序执行程序。冯诺依曼架构的核心是运算器为核心，但随着科技不断发展，现在计算机的核心以存储器为核心。由于冯诺依曼架构将数据和指令统一放在存储器中，并且由于顺序执行程序，导致指令的吞吐量遇到瓶颈，因此提出了著名的哈弗架构 (Harvard architecture), 哈弗架构设计的特点是指令存储器和数据存储器是两个独立的存储器，每个存储器独立编址、独立访问. 哈弗结构减轻程序运行时的存放瓶颈。

![](/assets/PDB/HK/TH001436.png)

随着计算不断发展，两种架构相互补齐不足相互发展，最终通用计算机体系结构发展如上图。控制器和计算器组合成 CPU，CPU 中包含了寄存器，寄存器是离 CPU 最近存储器，提供最快速度的数据存储，用于暂时提供 CPU 数据存储能力。CPU 和主内存之间存在 CPU CACAHE Memory 存储器, 用于提供高速小容量数据存储能力以此加快数据访问。主存储器 RAM Main Memory 提供超大容量低速数据存储能力以此存储大部分的数据。外设存储 Storage 可有不同的磁盘设备组成，用于提供持久数据存储能力，磁盘外设包括: floppy 软盘、CD-ROM、SATA 硬盘、SSD 硬盘、ESSD 云盘等设备.

![](/assets/PDB/HK/TH001437.png)

一个程序的生命周期与存储器之间的关系如上图, 源程序经过编译之后生成可执行的二进制文件，二进制文件一般存储到磁盘外设中，当程序在执行的时候，程序的代码段、数据段、BSS 段等数据被拷贝到 Main Memory 主内存中，另外程序运行时候的 heap 堆、Stack 堆栈也会占用主内存的一部分空间。接着 CPU 将要执行的指令和数据准备从主存储中加载到 CPU 中运行，此时 Cache 利用局部行原理将指令和数据相关的一部分内存缓存到 Cache 中，以此加快 CPU 获得下一条指令或数据的速度。Cache 缓存完内容之后，将 CPU 需要执行的指令和数据加载到寄存器中，接下来 CPU 执行指令和处理数据，期间产生的数据和最终产生的数据都存储在寄存器中。待 CPU 执行完指令之后，如果 CPU 需要将计算结果写入到主内存，那么系统首先检查写入的内存是否已经缓存到 Cache 中，如果已经缓存到 Cache 里，那么系统将寄存器中数据拷贝到 Cache 中即可; 反之如果写入的内存没有缓存在 Cache 中，那么系统将寄存器中的内存直接写入到主内存中，然后将主内存中内容缓存中 Cache 中，而 Cache 中原有的内容则提前同步到主内存中。最后如果执行的程序需要将数据写入到磁盘文件上，那么主内存中的数据会在合适的时机同步到磁盘文件上. 待程序执行完毕之后，其 heap/Stack/MMAP 占用的内存将被释放回收。

##### 硬件角度看内存架构

![](/assets/PDB/HK/TH002406.png)

从硬件角度来看，内存架构有各式各样的存储器构成，其目的就是存取系统运行所需的数据和代码。如果按读取数据的速度进行划分，那么存储器会形成上图的金字塔，其中越靠近塔尖其读取数据的速度越快。存储器子系统被划分中三类，第一类是 On-CPU 的存储器，这类存储器离 CPU 最近，因此读取速度最快; 第二类是主存，用于存储正在运行的程序或系统所需的代码和数据; 第三类是辅存，主要用于存储没有运行的程序的代码和数据。CPU 要访问数据，首先在第一类存储器中查找，如果没有找到就到第二类中查找，如果第二类存储器中没有，那么就需要到第三类中获取，但 CPU 不能直接访问第三类，硬件会负责将第三类存储器中的数据搬运到第二类中，然后 CPU 就存取到所需的数据.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

#### CPU 架构基础

![](/assets/PDB/HK/TH002368.png)

**CPU(Central processing unit)** 中央处理器，作为计算机系统运算和控制的核心，是信息处理、程序运行的最后执行单元. 指令在这里处理，信号从这里发出去. CPU 的范围比较大，里面包含了 Core、内存控制器、PCIe 控制器、片外总线等。一个 CPU 中可能包含多个 Core，通常所说的物理核心指的是 Core，每个物理核心都包含各自的电路。

##### <span id="A0">物理核与逻辑核(超线程)</span>

![](/assets/PDB/HK/TH002364.png)

**物理核(Physical Core)** 是一个独立的执行单元，它可以其他物理核**并行**运行一个程序线程。现代 CPU 具有多个物理核，例如上图绿色虚线框内就是一个独立的物理核. 每个物理核上可以用于 2 个**逻辑核(Logical Core)**, 逻辑核与在同一个物理核上运行的其他逻辑核心共享资源，例如上图红色框就是一个逻辑核, 两个逻辑核拥有属于各自的寄存器组，但共用一组 ALU 计算单元，如果每个逻辑 CPU 上运行一个进程，那么两个进程之间的通信完成在物理 CPU 核内部，无需系统总线，但从唯一的 ALU 来看**无法真正意义上同时执行两个进程**。可以将橙色款的部分看做一个没有逻辑核(超线程)的单核物理核. 在 Intel 超线程技术下，逻辑核也称为超线程, 那么在上图中，每个物理核拥有两个逻辑核。

![](/assets/PDB/HK/TH001700.png)

**vCPU(虚拟 CPU)** 等价于逻辑核(超线程)，但存在差异: 虚拟 CPU 更多的是限定在虚拟化语境内. 一个宿主机上的逻辑核可以映射为虚拟机内部的一个虚拟CPU(vCPU), 因此在虚拟化语境中基本是同一个术语.

##### <span id="A1">CPU Socket</span>

![](/assets/PDB/HK/TH002365.png)
![](/assets/PDB/HK/TH002367.png)

多核架构指的是在一颗芯片上放多个处理器(CPU), 正如上图所示。一颗芯片插在主板的一个**插槽(Socket)** 上，一颗芯片上放了多个物理核. 在有的主板上也有多个插槽，那么就可以插多颗芯片，因此会看到 "2 颗 4 核", 其含义的就是主板上有 2 个芯片插槽，每颗芯片上存在 4 个物理 CPU，那么总共 8 个物理 CPU，如果每个物理核存在 2 个超线程，那么总共 16 个超线程或逻辑核.

![](/assets/PDB/HK/TH002366.png)

通常市面上看到的 CPU 一般会标识 X 核 Y 线程，意思就是该 CPU 包含了 X 个物理核，每个物理核上包含 (Y/X) 个逻辑核或超线程. 例如 Intel 酷睿 i7 11700 CPU 就是**八核心十六线程** 指的就是其包含 8 个物理核心且每个物理核包含 2 个超线程(逻辑核); 又如 AMD Ryzen 7000 CPU 就是**96核心192线程**, 指的就是包含了 96 个物理核心，每个物理核上包含了 2 个超线程(逻辑核).

##### <span id="A2">CPU Die</span>

![](/assets/PDB/HK/TH002369.png)

**Die** 或者 **CPU Die** 指的是处理器在生产过程中，从**晶圆(Silicon Wafer)** 上切割下来的一个个小方块，在切割之前需要经过各种加工将电路逻辑刻在 Die 上面. Die 是一块半导体材料(通常是硅)，一个 Die 可以包含任意数量的 Core，Die 是构成 CPU 的晶体管实际所在.

![](/assets/PDB/HK/TH002370.png)

对于主流的 CPU 厂商 Intel 和 AMD，他们会将 1 个或者多个 CPU Die 封装起来形成一个 **CPU Package**, 有时也叫做 **CPU socket(CPU 插槽)**. CPU Die 之间通过**片内总线(Infinity Fabric)** 互联，并且不同 CPU Die 上的 CPU 内核不能共享 CPU 缓存。在 Intel 的 Xeon 处理器里，同一个 CPU Die 上的物理核共享 L3 Cache.

![](/assets/PDB/HK/TH002371.png)

例如 AMD EYPC CPU 而言，它的每个 CPU Socket 由 4 个 CPU Die 组成，每个 CPU Die 中含有 4 个 CPU 物理核，图中四个横的黑色长方体区域就是 CPU Die，每个 Die 中有 4 个物理核. 

##### <span id="A3">CPU Package</span>

![](/assets/PDB/HK/TH002372.png)

**CPU Package** 指的是包含一个或者多个 CPU Die 的塑料/陶瓷外壳和镀金的触电，也就是当你购买单个处理器时所得到的东西. 主板上每个 CPU 插槽(CPU Socket) 只能安装一个 Package, Package 也指是插在插座上的单元. 例如上图看到一个明亮外壳是 Package 的正面，背面全是金属触电，正好与主板的触电贴合在一起.

![](/assets/PDB/HK/TH002373.png)

双核处理器是一个包含两个物理核(Core) 的 Package，可以是一个 CPU Die 也可以是两个 CPU Die。第一代多核处理器通常是在一个 Package 上使用多个 CPU Die，而现代设计将多个 Core 放到同一个 CPU Die 上，带来了一些优势，比如能够共享 On-Die 缓存。上图是一个双核心(Core) 的 CPU，使用了两片 CPU Die，每片上有一个物理核(Core).

##### <span id="A4">CPU 系统信息</span>

![](/assets/PDB/HK/TH002374.png)

在一台拥有 2 个 Socket 的机器上安装上 2 个 CPU Package，每个 Package 拥有 2 个物理核(Core), 每个物理核上拥有两个超线程(逻辑核)。当系统运行之后，通过 "/proc/cpuinfo" 查看 CPU 相关的信息，其各字段含义:

* processor: 指超线程 ID 或者逻辑核 ID.
* apicid: 指逻辑核对应 LAPIC ID.
* core id: 指逻辑核或超线程所属的 Core ID(物理核 ID).
* physical id: 指超线程(逻辑核)对应的物理核插在 CPU Socket ID.
* cpu cores: 指系统包含物理核(Core) 的总数.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### 存储器子系统

![](/assets/PDB/HK/TH002406.png)

存储器子系统指的是计算机中存储程序和数据的各种存储设备，传统的存储器子系统一般包括高速缓存(CACHE)、主存(Main Storage) 和辅存(Secondary Storage). 主存是 CPU 可以直接访问，其存取速度块但容量小，一般用来存储当前正在执行的程序和数据; 辅存位于主机之外，其容量大价格低，但存取速度较慢，一般用来存放暂时不参与运行的程序和数据，CPU 不能直接访问辅存，辅存中的数据只有需要时才会被传送到主存，因此他是主存的补充和后备. 当 CPU 速度很快，但主存存取速度很慢，为了使两者速度匹配，在 CPU 和主存之间添加了高速缓存 CACHE, CACHE 的存取速度比主存快很多，但容量更小，用来存放当前最继续处理的程序和数据，以便快速地向 CPU 提供指令和数据.

![](/assets/PDB/HK/TH002407.png)

上面的表格来自 jboner 测试的各种存储子系统设备存取时延，可以看到离 CPU 最近存取速度越快，那么不同的设备存取速度差异到底有多大，这里通过一个故事进行描述: 假设把 CPU 的一个时钟周期看做 1s，正在图书管理里面查资料，那么从 L1 CACHE 读取信息就好像是拿起桌上的一张草稿纸(3s), 草稿纸上没有那么需要去书架上找，从 L2 Cache 读取信息则是从身边的书架上取出一本书(14s), 从主存中读取信息则相等于走到楼下区买个零食(4 分钟). 如果需要找的资料位于磁盘，那么等待磁盘寻道的时间相当于离开图书馆并开始长达一年零三个月的环球旅行。通过这里例子可以理解不同存储器之间的读取速度差异，以及为什么需要添加 CACHE 了吧.

-----------------------------------

##### <span id="B5">Register</span>

![](/assets/PDB/HK/TH002409.png)

在支持多核多线程的架构里，每个物理核 Core 里面包含了两个逻辑核，每个逻辑核都有各自一套寄存器，这些寄存器包括通用寄存器、EFLAGS 寄存器、段寄存器、EIP 寄存器和 MSR 寄存器等，在软件层次这些寄存器虽然名字相同，软件调用方法也相同，但硬件可以根据程序正在使用的逻辑核使用其私有的寄存器。硬件上同样也存在多个逻辑核共享的寄存器，同样也存在多个物理核共用的寄存器等。由于寄存器离 CPU 最近，因此其存取速度最快，基本和 CPU 速度一致.

-----------------------------------

##### <span id="B6">CACHE</span>

![](/assets/PDB/HK/TH002408.png)

通过上面的介绍可知，CACHE 是为了加速 CPU 对内存的访问，采用局部性原理将即将访问到的数据加载到 CACHE 中，以此保证 CACHE 的命中率，进而提供 CPU 读取数据的速度。上图是一个典型的 X86 多核多线程架构，可以看出同一个物理核的所有逻辑核共享 L1 CACHE 和 L2 CACHE，另外同一个 Socket 上的所有物理核共享 L3 CACHE.

-----------------------------------

##### <span id="B0">ROM 和 RAM</span>

![](/assets/PDB/HK/TH002377.png)

**RAM**: 随机访问存储器(Random Access Memory), 易失性. 是与 CPU 直接交换数据的内部存储器，它可以随机读且速度很快，通常作为操作系统或其他正在运行程序中的临时数据存储媒介。**ROM**: 只读存储器(Read Only Memory), 非易失性. 一般是转入整机前事先写好的，整机工作过程中只能读，而不像随机存储器那样能快速地、方便地加以改写。ROM 数据稳定，断电后所存数据也不会改变。计算机的 ROM 主要用来存储一些系统信息，或者启动程序 BIOS。

![](/assets/PDB/HK/TH002378.png)

随机访问存储器 RAM 分为两类: 静态和动态. 静态的 RAM(SRAM) 比动态 RAM(DRAM) 更块，但也更贵。SRAM 用来做高速缓存存储器，既可以在 CPU 芯片上，也可以在片下. DRAM 用来作为图像系统和文件系统的缓冲区，RAM 断电时将丢失其存储的数据，故主要用于存储短时间使用的程序。RAM 具体差异如下:

* **SRAM**: SRAM 存储器单元具有双稳定态特性，只要有电就可以永远保持它的值(优点类似 ROM 易失性), 即使有干扰电压，待干扰消除之后，电路就会恢复稳定.
* **DRAM**: Dynamic Random Access Memory, 里面所有存储的数据需要周期性地更新。
* **SDRAM**: Synchronous Dynamic RAM, 即有一个和 CPU 同步的时钟信号，使得读写响应域系统总线同步.
* **DDR SDRAM**: Double Data-Rate Synchronous DRAM, 双倍数据速率同步 DRAM.
* **DDR2/DDR3/DDRn**: 不同类型的 DDR SDRAM.

只读存储器 ROM: ROM 里的数据预先被写入，一旦将数据写入 ROM 无法将其删除，只能读取。与 RAM 不同即使计算机关机，ROM 也会保留其内存。ROM 称为非易失性存储器，随着技术的不断发展，也出现了可以重新编写的 ROM，具体分类如下:

* **PROM**: Programmable ROM(可编程 ROM)，只能被编程一次.
* **EPROM**: Erasable Programmable ROM(可擦写可编程 ROM，EPROM)，擦写可达 1000 次
* **EEPROM**: Electrically Erasable Programmable RPM，电子可擦除 EPROM.
* **闪存(Flash memory)**: 基于 EEPROM 的存储技术。固态硬盘(SSD) U 盘等就是一种基于闪存的存储器
* **Nor Flash**: NOR Flash 的读取和 SDRAM 读取一样，可以直接运行装载在 NOR FLASH 里的代码，可以减少 SRAM 容量从而节约成本.
* **Nand Flash**: NAND Flash 没有采用内存的随机读取技术，它的读取是以一次读取一块的形式进行的，通常以此读取 512 字节，采用这种技术的 Flash 比较廉价。不能直接运行 NAND Flash 上的代码，因此很多 NAND Flash 主板使用 NAND Flash 之外还加上一块 NOR Flash 来运行代码.

---------------------------

##### <span id="B1">DIMM 内存条</span>

![](/assets/PDB/HK/TH002379.png)

通过对 CPU 架构基础的学习，CPU 如果要工作起来还需要内存的配合，CPU 通过通过内存控制器可以访问内存硬件(内存颗粒). 在 80286 时代，内存硬件是直插在主板上内存颗粒, 称为 DIP(Dual In-line Package). 到 80386 时代，内存换成了一片焊有内存颗粒的电路板，称为 SIMM(Single-Inline Memory Module), 这样改变带来的好处是: 模块化、便于安装等. 此时 SIMM 的位宽是 32bit，即一个周期读取 4 个字节, 到了奔腾时代，位宽变成 64 位(8 字节)，于是 SIMM 顺势变成了支持 64 位的 DIMM(Double-Inline Memory Module), DIMM 形态一致延续至今，也成了内存硬件的基本形态.

![](/assets/PDB/HK/TH002375.png)

随着科技的发展，DIMM 为了满足不同的场景，分化出不同类型的 DIMM 内存条，大体上包括: RDIMM、UDIMM、SO-DIMM 和 Mini-DIMM.   
* **RDIMM**: 全称 Registered DIMM(寄存型模组), 主要用于服务器上，为了增加内存的容量和稳定性有 ECC 和无 ECC 两种，市面上基本都是带 ECC 的.
* **UDIMM**: 全称 Unbuffered DIMM(无缓冲型模组)，主要用于平时用到的标准台式机 DIMM，也分为有 ECC 和无 ECC 两种，一般是无 ECC 的.
* **SO-DIMM**: 全称 Small Outline DIMM(小外形 DIMM)，用于笔记本电脑，也分 ECC 和无 ECC 两种.
* **Min-DIMM**: RDIMM 的缩小版，用于刀片式服务器等对体积要求苛刻的场景.

![](/assets/PDB/HK/TH002376.png)

一般内存条的长度为 133.35MM, SO-DIMM 为了适应笔记本内狭小的空间，缩短为 67.6mm 而且一般为侧式插入。高度也有一些变种，一般的内存条高度为 30mm, VLP(Very Low Profile) 降低为 18.3mm, 而 ULP(Ultra Low Profile) 更是矮化到 17.8mm，主要为了放入 1U 的刀片服务器中.

----------------------------------

##### <span id="B2">内存颗粒内部结构</span>

![](/assets/PDB/HK/TH002386.png)

从外观来看内存条很多内存颗粒组成，但从内存控制器到内存颗粒的内部逻辑，笼统的从大到小分为: Channel -> DIMM -> RANK -> CHIP -> BANK -> ROW/Column.

###### Memory Channel

![](/assets/PDB/HK/TH002381.png)

**内存通道(Channel)** 是内存控制器和内存之间通信的总线，增加内存通道可以加快数据传输。内存控制器通常有一个通道、两个通道(双通道)、四个通道(四通道)、六通道以及八通道等。从理论上来讲，多通道数据传输速率可以成倍增加(多通道速率 = 单通道速率 * 通道数量). 内存通道一端连接内存控制器，另外一端连接 DIMM 插槽，可以将内存条插入到 DIMM 插槽中.

![](/assets/PDB/HK/TH002389.png)

在主板上会将 DIMM 插槽使用不同的颜色进行标记，相邻且不同颜色的两个 DIMM 插槽属于同一个内存 Channel，例如上图中 DIMM3 黑色和 DIMM4 红色属于 ChannelA; DIMM1 黑色和 DIMM2 红色属于 ChannelB. 如果要提高内存的访问量，那么可以同时使用多个内存控制器访问内存，因此可以将内存条插入到不同的 Channel，这样就可以提高内存总体的访问速度，因此将内存条插入到颜色相同的插槽可以获得更高的内存吞吐量.

![](/assets/PDB/HK/TH002385.png)

**单通道模式**: 指提供单通道带宽运算，并且仅安装一个 DIMM 或所安装的多个 DIMM 各自具有不同的内存容量情况下启用. 例如上图 A 中任何的 DIMM 插槽中插入一个内存条，开机之后启用单通道模式; 例如图 B 中虽然内存条插入到同一颜色不同 Channel 上，但由于两个内存条容量不同，那么系统依旧采用单通道模式，并且使用速度慢的内存条; 例如图 C 中插满了内存条，但内存条的内存容量各部相同，因此也只能开启单通道且采用速度慢的内存条.

**双通道模式**: 指当两个 DIMM 通道的内存容量相等时，系统可以开启双通道模式以提高内存的吞吐量。当内存条容量相同但速度不同时，系统使用最慢的内存时序。例如上图 B 中，DIMMA2 和 DIMMB2 中插入内存容量相同的内存条时，可以开启双通道模式; 又如图 C 中，只需要 DIMMA1 和 DIMMB1 的容量相同，且 DIMMA2 和 DIMMB2 的容量相同，DIMMA1 可以与 DIMMA2 的容量不相等，那么系统同样可以开启双通道模式. 

![](/assets/PDB/HK/TH002382.png)

###### Memory Rank/Chip

![](/assets/PDB/HK/TH002383.png)

**内存颗粒(Chip)** 是真正存储数据的地方，如上图黑色芯片就是一颗独立的内存颗粒。CPU 与内存之间的接口位宽是 64bit，也就意味着 CPU 在一个时钟周期内会向内存发送或从内存读取 64bit 的数据。但是单粒的内存颗粒位宽只有 4bit、8bit、16bit 或者 32bit，因此需要将多颗内存颗粒并联起来，组成一个位宽为 64bit 的数据集合，这样才能与 CPU 交互数据，组成位宽为 64bit 的数据集合称为 **RANK**. 例如上图内存条正面上焊接了很多内存颗粒，然后多个内存颗粒就组成了一个 RANK. 在有的内存条背面同样放置了另外一个 RANK，因此有两个 RANK.

###### Memory Bank/Cell

![](/assets/PDB/HK/TH002387.png)

内存颗粒(Chip) 内部包含了多个 Bank，一个 **Bank** 就是一个存储矩阵，并由 Chip 的 BA 线的位宽决定了 BANK 的个数. 在 BANK 内部通过行列划分成一个个独立的存储单元，这些存储单元称为 **Cell**，可以保存 1Bit 的数据，Bank 就是由很多个 Cell 组成. Cell 由 Row decoder 和 Column decode 进行寻址。

![](/assets/PDB/HK/TH002388.png)

结合 RANK 的概念，内存控制器能够对同一个 RANK 的所有内存颗粒(Chip) 同时进行读写操作，而在同一个 RANK 的 Chip 也共享同样的控制信号。RANK1 和 RANK2 共享同一组 Addr/Command 信号线，利用 CS 片选线选择访问哪组 Rank，之后将存储内存经由 MUX 多路器送出。BANK 再往下就是实际的存储单元，一般来说横向选择排数的线路称为 ROW(Row enable, Row Select, Word line), 纵向负责传送信号线路称为 Column(Bitline), 每组 Bank 的下方还会有个 Row Buffer(Sense amplifer), 负责将读出的 row 内存暂存.

---------------------------------

##### <span id="B3">内存编址</span>

一颗内存颗粒位宽可能是 8 位，也可能是 16 位，容量可能是 1MiB，也可能是 1Gig. 那么内存如何编址呢? 与地址总线如何映射呢? 内存条容量的大小和芯片的扩展方式有关。例如内存模块采用 16MiB\*8bit 的内存颗粒，那么使用 8 颗内存颗粒进行位扩展，称为 16MiB\*64bit，再使用 8 颗内存颗粒进行容量扩展变成 128MiB\*64bit，实际内存大小为 1024MiB. 当对 1024MiB 内存进行编址以便 CPU 能够使用它，通常有多种编址:

* **按字(Word/32bit)编址**: 对 1024MiB 内存来说，它的寻址范围是 256MiB，而且每个内存地址存储 32bit 数据
* **按半字(16bit)编址**: 对于 1024MiB 内存来说，它的寻址范围是 512MiB，而且每个内存地址存储 16bit 数据
* **按字节(Byte)编址**: 对于 1024MiB 内存来说，它的寻址范围是 1024MiB, 而且每个内存地址存储 8 bit 数据

当前计算机体系结构主要采用了**按字节编址**, 所有如果把内存看做一个线性数组的话，每个成员的大小为 8bit，并称为一个存储单元。结合 BANK 和 Cell 的定义，这些存储单元从 0x00000000 开始沿着 BANK 访问编址，每 8 个 Cell 存储单元编号加 1. 超过一个 RANK 之后，从上一个 RANK 最后一个 Cell 的地址加上 1 作为基地址.

![](/assets/PDB/HK/TH002413.png)

每个 Cell 有一个地址的话，按 Channel-RANK-BANK 的顺序进行编址，那么 RANK0 的 BANK0 将按 ROW/Column 的方式为每个 Cell 分配一个地址，因此形成上图的编址编排: 第一个 BANK 的所有 Cell 编址完毕之后从下一个 BANK 继续编址，如果一个 RANK 的所有 BANK 编址完之后，那么继续从下一个 RANK 的第一个 BANK 继续为每个 Cell 编址，同理第一个 Channel 编址完毕完毕之后，从第二个 Channel 的第一个 RANK 的第一个 BANK 继续编址. 直到所有的 Cell 都编址.

![](/assets/PDB/HK/TH002414.png)

系统还会采用称为 **Interleaving** 方式进行编址，其可以有不同粒度的 Interleaving, 例如上图的 BANK 级别的交错编址，编址先给第一个 BANK 的第一个 Cell 编址，然后给第二个 BANK 的第一个 Cell 编址，接着给第三个 BANK 的第一个 Cell 编址，以此类推，所有的 Cell 编址。可见 Cell 的编址是交错进行的，常见的还有 Channel 级、DIMM 级和 RANK 级的。服务器上 Interleaving 更是不可或缺，它的粒度更细。这样的编址的优点是统一的地址空间，带宽与交织的通道数量成正比, 自动负载均衡.

---------------------------------

##### <span id="B4">内存寻址</span>

![](/assets/PDB/HK/TH002362.png)

当 CPU 通过地址总线传送过来一个物理地址时，内存条是如何寻址到一个 Cell 呢? 首先通过上图看看内存条是如何与 CPU 连接的, 当物理地址从 MMU 送出来之后，CPU 会把需要访问的地址当做一个数据给内存控制器，当然这个是会加上一些表示以便识别这个数据需要访问内存地址，内存控制器收到这个地址后，通过集成在控制器中的 "逻辑映射", 内存控制器就知道这个地址对应的 RANK、BANK、ROW 和 Column 地址信息，及确定了哪个 CS# 信号有效(CS# 信号用于选择 RANK)，以及哪组 BANK# 信号有效(BANK# 信号用于选择 BANK)，然后内存控制器遵从内存时序下按照固定的时序发送 CS# 信号和 BANK# 信号，ROW address/Column address 特殊状态下会有 DQM 信号和 BL 信号等。

![](/assets/PDB/HK/TH002390.png)

简单来说内存控制器出来，然后就是选择 DIMM，根据片选到 RANK，再到 BANK，然后在 Chip 的地址解码器里输出行地址和列地址，定位到 Cell。其实也就是物理地址要包含行地址、列地址、BANK 地址、RANK 地址信息等.

###### 读写方式

![](/assets/PDB/HK/TH002391.png)

当需要从内存条读取数据时，内存控制器将 1 组位地址传送到信号线上，内存控制器接着传送控制信号，如果是多 RANK 的情况，CS# 信号送对应的 RANK 上，接着由于每个 RANK 是由多个 Chip 组成，每个 Chip 仅负责部分的数据读取，目标 Chip 收到位地址信号之后，将位地址放入内部的 ROW/Column 解码器找到对应的 BANK 地址，接着开启 ROW 线，同一排 ROW 的内部就会存放到 ROW Buffer 内部，ROW Buffer 判断信息为 0 或者 1 之后就输出存储的内容. 写入是除了地址信息之外，还会传送需要写入的内容到芯片内部的 Input buffer，同样也按照 Row/Column 解码器找到对应的位地址之后写入.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### 物理地址空间

![](/assets/PDB/HK/TH001481.png)

将 CPU 地址总线可以寻址的范围称为**物理地址空间(Physical Address Space)**, 且寻址的地址称为**物理地址(Physical Address)**, 那么物理地址空间将会是一块连续的线性区域. 在 Intel X86 架构中，将物理地址空间也称为**存储域**，其主要由三部分组成:

* 第一部分是 DDR 控制器映射的 **DDR 域**
* 第二部分是 PCI 总线域映射到存储域的 **PCI 总线地址**
* 第三部分是设备内部存储空间映射到存储域的区域

将第二部分和第三部设备 IO 映射到存储域的，统一称为 **MMIO(Memory Mapping IO)**。Intel X86 架构也通过 Host PCI 主桥维护一颗或多颗 PCI 总线，每条 PCI 总线构成独立的空间称为**PCI 总线域**.

![](/assets/PDB/HK/TH001695.png)

在 X86 架构中，**存储域**的空间长度和 **PCI 总线域**的长度是相同的，并且 **DDR 域**的地址可以映射到**存储域**，也可以映射到 **PCI 总线域**，同理 **PCI 域**的地址也可以映射到**存储域**，且该地址在**存储域**上称为 **PCI 总线地址**。

![](/assets/PDB/HK/TH002392.png)

上图是一个典型的 X86 架构物理地址空间分布图，可以看到物理地址空间并不等效于 DDR 域空间，物理空间被划分成了几个大区域，因此并不是物理地址空间都是内存条的内存。

---------------------------------

##### <span id="C0">DDR 域</span>

![](/assets/PDB/HK/TH002394.png)

DDR 内存控制器可以看到完整的 DDR 域空间，并且是一块连续的地址空间, 那么内存条内存是如何映射到物理地址空间呢？ 首先了解一下 DDR 域空间， DDR 域空间一般分作 3 类:

* 第一类是直接映射到物理地址空间的区域
* 第二类是重映射到物理地址空间的区域
* 第三类是系统进入 SMM 模式才可以访问的区域

DDR 域的 \[0x00000000, Reclaim Base] 区域直接映射到了物理地址空间 0x00000000 开始之后的区域，DDR 域 \[0x100000000, MESEG Base] 区域直接映射到物理地址空间的 \[0x100000000, Recliam Base] 区域。DDR 域接着采用 Reclaim 技术将 DDR 域的 \[Reclaim Base, 0x100000000] 映射到物理地址空间 Reclaim Base 之后的区域. 

---------------------------------

##### <span id="C1">MMIO and PIO</span>

![](/assets/PDB/HK/TH001442.png)

在 X86 架构中，不仅存在物理地址空间，还存在 IO 空间，外设可以将其内部寄存器映射到 IO 空间，然后 CPU 可以使用 IN/OUT 指令方式访问外设的寄存器, 那么称 IO 空间的地址为 **IO Port(PIO)**; 外设也可以将其内存寄存器映射到物理地址空间上，然后 CPU 可以像访问内存一样访问外设，那么称这种方式为 **MMIO(Memory Mapping IO)**. X86 架构并没有像其他架构一样将 IO 空间和物理地址空间进行统一编址，而是采用独立编址并需要不同的指令进行访问，但外设可以在硬件映射时选择映射到 IO 空间还是 MMIO.

![](/assets/PDB/HK/TH002395.png)

可以在系统中通过 "/proc/ioports" 节点查看系统 IO 空间的布局，可以看到映射了很多外设的寄存器，包括查看 PCIe 配置空间的 0xCFC 和 0xCF8 端口，又例如映射 DMA 内存寄存器的端口.

![](/assets/PDB/HK/TH002396.png)

可以在系统中通过 "/proc/iomem" 节点查看系统物理地址空间的布局，可以看到系统物理内存和 MMIO 空间，包括系统内存、PCIe 设备的 MMIO、LAPIC 映射的 MMIO 等.

--------------------------------------

##### <span id="C2">X86 物理地址布局</span>

![](/assets/PDB/HK/TH002392.png)

上图是一个典型的 X86 架构物理地址空间分布图，可以看到物理地址空间并不等效于 DDR 域空间，物理空间被划分成了几个大区域，因此并不是物理地址空间都是内存条的内存。

###### DOS Area

![](/assets/PDB/HK/HK000408.png)

最低地址区域称为 DOS RAM，其长度为 1MiB，用于 Legacy BIOS 使用的物理区, 

* \[0x00000, 0xA0000) 最前面的 640KiB 常规内存也是 DDR 内存的一部分，最前 1KiB 用于存储 BIOS 的中断向量表，随后的 1KiB 被用作 BIOS 数据区; 
* \[0xA0000, 0xC0000) 区域映射是显卡的显示 RAM, 也属于 MMIO 不是 DRAM 内存
* \[0xC0000, 0xD0000) 区域映射显卡的 ROM 还有硬盘、网卡的 ROM，也属于 MMIO 不是 DRAM 内存
* \[0xD0000, 0xE0000) 区域可以映射设备的 ROM，如果没有映射就是 DRAM 的内存
* \[0xE0000, 0xF0000) 区域为扩展的 BIOS 区域，输入 DRAM 内存
* \[0xF0000, 0x100000) 区域为常规 BIOS 区域，用于映射 BIOS 芯片上，CPU 的第一句指令 0xFFFF0 就是跳转到该区域.

###### Main Memory

![](/assets/PDB/HK/TH002393.png)

物理地址空间中存在两段物理内存区域，分别是 \[0x00100000, TOLM) 区域和 \[0x100000000, TOUUD) 区域。两段区域都是可用物理内存区域:

* \[0x00F00000, 0x01000000) 区域为传统的 Windows ISA 黑洞(ISA Hole)
* Extended SMRAM Space(TSEG) 扩展 SMM 内存.
* Internal Graphics Memory
* TOLUD 为 DDR 映射到物理地址空间的低端区域最大值，TOUUD 为 DDR 映射到高端区域最大值, 其中也包括 Reclaim 的内存.

###### MMIO Space

![](/assets/PDB/HK/TH002397.png)

物理地址空间中存在两段 MMIO 区域，分别是 \[TOLUD, 4Gig) 和 \[TOUUD, 512Gig) 两片区域，这些区域可以用来映射 PCI/PCIe 外设的 BAR，也可以用来映射 GPU 的 Rangs，具体如下:

* \[0xFFE00000, 0x100000000): BIOS 内容映射的地址，它的大小可调.
* \[0xFEC00000, 0xFED00000): APIC 配置空间，其映射了 Per-CPU 的 Local APIC 和 IOAPIC 内部寄存器
* \[0xFEE00000, 0xFEF00000): PCIe/DMI 设备发送 MSI/MSIX 中断的 Address.
* \[0xE0000000, 0xF0000000): 映射 PCIe/PCI 配置空间
* DMI Interface 为 PCI/PCIe/DMI 设备内部寄存器或 BAR 映射的区域 

-------------------------------------------

##### <span id="C3">页帧/页号/物理页/Page</span>

![](/assets/PDB/HK/TH002402.png)

将物理地址空间切割成 PAGE_SIZE 大小的区域，一个区域称为**物理页(Physical Page)**，物理页内包含了多个物理地址，其中物理页包含地址最低的物理地址称为**物理页起始物理地址**. 按物理页起始物理地址进行排序，那么每个物理页都有一个序号，这个序号称为**页帧号(Page Frame Number, 简称 PFN)**。内核初始化过程中，建立一个 struct page 的指针数组 mem_map[], 该数组的成员依据 PFN 与一个物理页进行绑定，那么系统就可以使用 struct page 数据结构维护一个物理页, mem_map[] 数组就可以维护所有的物理页. 因此可以得到如下的转换关系:

* PFN = PHYS >> PAGE_SHIFT
* PFN = PHYS / PAGE_SIZE
* PFN = page_to_pfn(struct page \*page)
* PFN = PHYS_PFN(phys)
* struct page \*page = pfn_to_page(unsigned long pfn)
* PHYS = page_to_phys(struct page \*page)
* PHYS = PFN_PHYS(pfn)
* PHYS = PFN << PAGE_SHIFT
* PHYS = PFN * PAGE_SIZE

---------------------------------------------

##### <span id="C4">ZONE/NUMA NODE</span>

![](/assets/PDB/HK/TH002403.png)

如果将设备 DMA 时访问物理内存能力对物理地址空间进行划分，那么老式 ISA 设备在做 DMA 时只能访问 1MiB 以下的物理内存，那么将 \[0x00000000, ISA_END_ADDRESS) 区域称为 **ZONE_DMA**; 在新式的设备做 DMA 时已经能访问 32位的物理内存空间，其最大可以访问到 4Gig 处，因此将 \[ISA_END_ADDRESS, 0x100000000) 的物理地址空间称为 **ZONE_DMA32**; 在最新的设备做 DMA 时，其可以访问 64 位的物理内存空间，因此将 \[0x100000000, TOUUD) 区域称为 **ZONE_NORMAL**.

![](/assets/PDB/HK/TH002404.png)

在 IA32 架构或者 ARM 架构中情况可能有所不同，由于两种架构都只有 32 根地址线，那么 ZONE_DMA 区域还是用于老式 ISA 设备 DMA 可以访问的区域; ZONE_NORMAL 区域不仅是新式设备 DMA 可以访问 32 位物理内存空间，另外还表示 ZONE_NORMAL 区域是被内核的线性映射区映射的物理内存区域; 反之 **ZONE_HIGHMEM** 区域表示内核线性映射区没有直接映射的物理内存区域，系统如果要访问这段物理内存需要先建立页表之后才能访问.

![](/assets/PDB/HK/TH002405.png)

在支持 UMA 架构中，所有 CPU 到内存的距离都是相同的，但在 NUMA 架构中，CPU 到所有内存的距离可能不相同，不同的距离就代表不同的时延，因此 CPU 对不同内存区域有不同的亲和性。内核将物理地址空间按 CPU 对内存的亲合性划分成不同的区域，每个区域称为一个 **NUMA NODE**。亲合在 NUMA NODE 上的所有 CPU 访问这段物理内存的时延最小，又称这段物理内存区域为这些 CPU 的**本地内存(Local Memory)**. CPU 访问其他 NUMA NODE 的时延会增加，并称其他 NUMA NODE 的物理内存为**远端内存(Remote Memory)**.

在 X86 架构中，一个 NUMA NODE 可能包含一个或多个 ZONE，同一个 ZONE 也属于不同的 NUMA NODE. 例如上图中 NUMA NODE0 包括 ZONE_DMA 和 ZONE_DMA32; NUMA NODE1 包括 ZONE_DMA32 和 ZONE_NORMAL. 另外 ZONE_DMA32 属于 NUMA NODE0 也属于 NUMA NODE1.

---------------------------------------------------

##### <span id="C5">Memory Model</span>

![](/assets/PDB/HK/TH002410.png)

**FLATMEM Model** 是最内核最早采用的内存模型，由于早期的内存较小，并且没有超过 LOW MMIO 区域，那么内核使用 struct mem_map[\*] 数组管理所有物理页时，数组中的所有物理内存都对应一个真实的物理内存页。但该模型也有一个缺点，当物理较大且 LOW DRAM 区域也不够映射，需要 HIGH DRAM 区域一同映射，那么内核同样会使用 struct mem_map[\*] 数组维护从物理地址 0 到 TOUUD 之间的所有物理页，那么显然 LOW MMIO 区域是没有物理内存页的，那么这样就造成 struct page 数据结构体浪费内存的问题。

![](/assets/PDB/HK/TH002411.png)

**SPARSE Model** 称为稀疏内存模型，其使用将物理地址空间划分成 SECTION_SIZE 大小的区域，并从地址到高地址的顺序给划分之后的区域编号，简称为 SECTION-NR, 内核使用 struct mem_section 数据结构维护该区域，如果 SECTION_SIZE 区域内存在真实的物理内存，那么内核将为 struct mem_section 数据结构的 section_mem_map 分配内存，用于存储 struct page 数组，然后通过 PFN/SECTION-NR 进行映射将每个 struct page 指向了对应的物理页. SPARSE Model 的优点是尽量减少了 struct page 数据结构体浪费的内存。该模型的缺点是并不能完全解决 struct page 的浪费，另外内核在转换 struct page 和 pfn 之间的关系时多了一层 SECTION，那么同样增加了耗时.

![](/assets/PDB/HK/TH002412.png)

为了解决 SPARSE Model 存在的问题，SPARSE 支持了 CONFIG_SPARSEMEM_VMEMMAP 场景中，内核将虚拟内存 vmemmap 用于存储 struct page 数组，内核可以通过 PFN 作为索引直接在 vmemmap 虚拟内存中找打 struct page, 但此时这里只是虚拟内存，没有对应真实的物理内存，内核在初始化阶段，如果 SECTION 里存在真实的物理内存，那么会将 vmemmap 对应的 struct page 对应的虚拟内存建立页表，这样可以最大限度的减少 struct page 的浪费.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000V.jpg)

#### 虚拟内存

![](/assets/PDB/HK/TH001447.png)

计算机硬件架构中存在一种硬件 MMU(Memory Management Unit), 通常称为内存管理单元，有时也称为分页内存管理单元 PMMU(Paged memory management unit), 负责处理 CPU 内存访问请求的计算机硬件。MMU 主要包含: 虚实地址翻译、访问权限控制。另外在计算机中存在实时模式和保护模式，当 CPU 处在实时模式下，CPU 看到的内存就是地址总线上的物理内存，那么 CPU 可以直接访问物理内存; 当计算机开启保护模式，MMU 分页功能便随之启动，此时 CPU 看到的内存是一块连续的线性空间，那么称这块空间为**虚拟内存**, CPU 访问虚拟内存的地址称为**虚拟地址**。虚拟内存的大小与计算机的位宽有关，例如 32 位系统虚拟内存的范围是 \[0, 4G), 而 64 位系统虚拟内存的范围是 \[0, 2^64). 虚拟内存的特点就是连续且范围巨大。

![](/assets/PDB/HK/TH001456.png)

系统将虚拟内存划分成两个区域，用户进程使用的虚拟内存区域称为**用户空间(Userspace)**, 内核使用的虚拟内存区域称为**内核空间(Kernel Space)**, 由于 MMU 的存在，用户进程访问内核空间或者内核访问用户空间都会引起系统错误。在不同架构中两个区域的大小划分有所不同:

![](/assets/PDB/HK/TH001457.png)

在 IA32 架构中，32 位地址总线的寻址能力为 \[0, 4G), 内核将虚拟内存从 PAGE_OFFSET 处分作两部分，\[0, PAGE_OFFSET) 为用户进程所使用的虚拟内存，而 \[PAGE_OFFSET, 4G) 为内核使用的虚拟内存。PAGE_OFFSET 可能是 2G 或者 3G，经典的分割是 \[0，3G) 是用户空间 \[3G, 4G) 为内核空间.

![](/assets/PDB/HK/TH001458.png)

在当前 AMD64/X64 架构中 64 位虚拟地址仅实现 48 位，剩下的高 16 位仅仅是作为符号拓展，组成最终的 64 位虚拟地址。高 16 位称为 Sign Extension 域，而低 48 位称为线性地址域，这样的 64 位虚拟地址称为 canonical-address 地址形式。Sign Extension 域要么全为 1 或者 0，对于 Sign Extension 不全为 0 或 1 的地址称为 Noncanonical-address.

![](/assets/PDB/HK/TH001459.png)

当前 AMD64/X64 架构中 Sign Extension 域为 16 位，那么虚拟内存会分作三部分，当 Sign Extension 全为 0 的区域称为 Canonical "Lower half", 也就是用户空间，128TiB 用户空间 \[0, 0x00007fff ffffffff) 占据了虚拟内存的底部. Sign Extension 全为 1 的区域称为 Canonical "High half", 也就是内核空间，128TiB 内核空间 \[0xffff8000 00000000, 0xffffffff ffffffff) 占据了虚拟内存顶部的位置。Sign Extension 域不全为 0 或 1 的范围 \[0x00008000 0000000, 0xffff8000 00000000) 称为 "Noncanonical Address Space", 落在这个区域的虚拟地址都是非法地址.

![](/assets/PDB/HK/TH001460.png)

随着技术的不断发展，AMD64/X64 64 位虚拟地址可以实现 56 根或者 64 根都能寻址，那么虚拟内存的 Canonical 区域的 "Lower Half" 和 "Higher Half" 将像 IA32 架构一样链接在一起，那么系统的内核空间和用户空间的范围将大大增加.

-------------------------------------

##### <span id="D0">页表</span>

![](/assets/PDB/HK/TH001463.png)

当开启保护模式之后，系统使用的地址从物理地址变成了虚拟地址，但虚拟内存是抽象出来的概念，并不是实际的存储介质，那么当系统访问虚拟内存时，MMU 通过分页机制透明的将虚拟地址自动转换成物理地址，进而访问物理内存。**页表**是一块长度为 PAGE_SIZE 的物理内存, 其被分割为指定长度的 Entry，Entry 内记录了下一级页表的物理地址或者物理页的物理地址。MMU 将虚拟地址划分为多个区域，在不同的架构中每个区域的含义不同。MMU 在转换虚拟地址之前需要建立页表，页表可以通过缺页被动创建，也可以主动创建。在不同的架构中页表的组成有所不同，但其目的都是将虚拟地址通过分层查表的方式找到下一级页表，最终找到最终的物理地址。

![](/assets/PDB/HK/TH001464.png)

不同的架构地址总线数量不同，因此可寻址范围的不同导致页表的级数不同。在 IA32 架构中使用 32 位的地址总线寻址，因此使用两级页表的 32-Bit 分页机制。该机制中存在两种页表: **页目录页表(Page Directory Table)** 和**页表(Page Table)**, 并且使用 CR3 寄存器指向 Page Directory 页表的物理地址. 虚拟地址 \[31, 22] 域称为页目录索引，用于在 Page Directory 页表中索引 PDE(Page Directory Entry), PDE 记录了下一级页表 Page Table 的物理地址。虚拟地址 \[21, 12] 域称为页表索引, 用于在 Page Table 页表中索引 PTE(Page Table Entry), PTE 记录了物理页的物理地址。虚拟地址 \[11, 0] 域称为页内偏移 PAGE_OFFSET，用于在物理页内找到对应的物理地址.

![](/assets/PDB/HK/TH001465.png)

在实现 48 位寻址的 AMD64/X64 架构中，使用 4-level 分页机制，该机制中存在四种页表: **PML4 页表**、**页目录指针页表(Page-Directory Pointer Table)**、**页目录页表(Page-Directory Table)** 和**页表(Page Table)**, 并使用 CR3 寄存器指向 PML4 页表的物理地址。虚拟地址 \[47, 39] 域称为 PML4 索引，用于在 PML4 页表中获得 PML4E(PML4 Entry), PML4E 用于记录下一级 Page-Directory Pointer 页表的物理地址。虚拟地址 \[38, 30] 域称为页目录指针索引, 用于在 Page-Directory Pointer 页表中索引 PDPTE(Page-Directory Pointer Table Entry), PDPTE 记录了下一级页表 Page-Directory 的物理地址。虚拟地址 \[29, 21] 域称为页目录索引，用于在 Page-Directory 页表中索引 PDE(Page Directory Entry), PDE 记录了下一页表 Page Table 物理地址。虚拟地址 \[20, 12] 域称为页表索引，用于在 Page Table 页表中索引 PTE(Page Table Entry), PTE 记录了物理页的物理地址。虚拟地址 \[11, 0] 域称为页内偏移，用于在物理页内索引物理地址.

![](/assets/PDB/HK/TH001466.png)

在实现 56 位寻址的 AMD64/X64 架构中，使用 5-level 分页机制，该机制中存在四种页表: **PML5 页表**、**PML4 页表**、**页目录指针页表(Page-Directory Pointer Table)**、**页目录页表(Page-Directory Table)** 和**页表**(Page Table), 并使用 CR3 寄存器指向 PML5 页表的物理地址。虚拟地址 \[55, 48] 域称为 PML5 索引，用于在 PML5 页表中获得 PML5E(PML5 Entry), PML5E 记录了下一级页表 PML4 的物理地址。虚拟地址 \[47, 39] 域称为 PML4 索引，用于在 PML4 页表中获得 PML4E(PML4 Entry), PML4E 记录下一级 Page-Directory Pointer 页表的物理地址。虚拟地址 \[38, 30] 域称为页目录指针索引, 用于在 Page-Directory Pointer 页表中索引 PDPTE(Page-Directory Pointer Table Entry), PDPTE 记录了下一级页表 Page-Directory 的物理地址。虚拟地址 \[29, 21] 域称为页目录索引，用于在 Page-Directory 页表中索引 PDE(Page Directory Entry), PDE 记录了下一页表 Page Table 物理地址。虚拟地址 \[20, 12] 域称为页表索引，用于在 Page Table 页表中索引 PTE(Page Table Entry), PTE 记录了物理页的物理地址。虚拟地址 \[11, 0] 域称为页内偏移，用于在物理页内索引物理地址.

![](/assets/PDB/HK/HK000996.png)

MMU 通过遍历页表将一个虚拟地址转换成物理地址。例如在 X86 架构中使用 32-Bit 分页机制，系统使用 CR3 寄存器存储 Page Table 页表的起始物理地址，然后将虚拟地址 VA 向右偏移 22 位之后获得了虚拟地址的 Directory 区域, 结合 Page Directory 就可以获得对应的 PDE, 此时 PDE 内记录了 Page Table 页表的起始物理地址。

![](/assets/PDB/HK/TH001448.png)

在获得 PDE 之后，将虚拟地址 VA 向右偏移 12 位并屏蔽 Directory 区域之后获得了虚拟地址的 Table 区域，结合 Page Table 既可以获得对应的 PTE，此时 PTE 内记录了物理页的起始物理地址.

![](/assets/PDB/HK/HK000993.png)

在获得 PTE 之后，将虚拟地址 VA 的低 PAGE_SHIFT 位区域隔离出来就可以获得 Offset 区域，结合物理页的起始物理地址，就可以在物理页内找到一个物理地址，此地址就是虚拟地址映射的物理地址.

--------------------------------------------

##### <span id="D1">缺页</span>

**缺页(page fault)**: 全称缺页异常，当系统访问了一个没有映射物理内存的虚拟地址之后，系统硬件会触发一个缺页异常。在缺页异常处理函数中，内核会为其分配物理内存，并建立虚拟内存到物理内存的页表。缺页异常处理函数处理完毕之后，系统再次再次重试缺页时的指令，也就是对虚拟地址再次访问，此时由于页表已经建立 CPU 可以访问到真正的内存. 缺页是建立页表的一种方式，系统也可以主动建立虚拟内存到物理内存的页表，这样会有效提供内存的访问效率，因为通过缺页建立页表本身就是一件耗时的操作，对内存访问性能要求很高的场景，可以提前建立好页表，减少缺页的发生.

---------------------------------------------

##### <span id="D2">用户空间布局</span>

![](/assets/PDB/HK/TH001461.png)

**用户空间(Userspace)** 是用户进程运行时的虚拟内存，当进程运行时会将虚拟内存划分成不同的区域，每个区域用于存储不同的数据或执行特定的任务。每个区域的具体含义如下:

* **\[0x0000000, \_\_executable_start)** 区域为预留区域
* **\[\_\_executable_start, 0x00007FFFFFFFFFFF)** 用户进程可以使用的虚拟内存
* **.text** 区域存储了进程的代码段
* **.data** 区域存储了进程的数据段
* **.bss** 区域为进程的 .bss 段, 初始化为 0 的数据加载到该段
* **Heap** 进程的堆，向高地址(向上)生长，可以通过 malloc() 和 brk() 为进程小粒度的虚拟内存
* **MMAP** 区域通过 mmap() 和 malloc() 向进程提供大块连续的虚拟内存，其向低地址(向下)生长，分配的虚拟内存可以用来映射共享库或者文件等.
* **Stack** 区域为进程的堆栈，其向下生长，栈底是栈区域的起始地址，栈顶是栈最新扩展的区域，栈利用先进后出的模式存储进程运行时的临时数据
* **argv/environ** 区域用于存储进程运行时的参数和环境变量相关的信息.
* **Kernel Space** 为进程看到的内核空间，不同架构存在位置差异，在 IA32 架构内核空间和用户空间相邻，但在有的 X86 架构两者并不相邻，但 PAGE_OFFSET 之后的虚拟内存就是内核空间.

![](/assets/PDB/HK/TH001462.png)

系统初始化正常运行时，系统存在多个用户进程同时运行的情况，由于进程隔离性的存在，每个进程都有各自的地址空间，并且每个进程只看得到自己的虚拟内存和内核空间，因此进程运行时会认为系统只有自己和内核线程在运行。由于该特性的存在，所有用户进程看到内核空间都是一致的，因此在进程切换的时候内核空间不需要切换，另外对于不同的进程就算用户空间虚拟地址相同，但两个虚拟内存对于的内容完全不是一个东西.

-------------------------------------

##### <span id="D3">内核空间布局</span>

![](/assets/PDB/HK/HK000226.png)

**内核空间(Kernel Space)** 是内核运行时的地址空间，内核空间被换成多个区域, 在不同的架构中，内核空间的布局存在一定的差异，但基本划分为一下几个大的区域, 每个区域都是为指定内存管理器或特殊功能服务。

###### 线性映射区

![](/assets/PDB/HK/TH002398.png)

**线性映射区**: 内核将 PAGE_OFFSET 开始虚拟内存与物理内存建立内核页表, 不同的架构长度不同。在 X86-64 架构上，该区域长度一般为物理内存的长度，那么区域中的虚拟地址只需通过一个线性公式就可以知道其对其的物理内存，而不需要通过遍历页表的方式获得，同理物理内存也只需一个线性公式就可以知道对应的内核空间的虚拟地址. 并且该区域的页表已经建立，那么不会发生缺页. 根据物理地址空间的了解，其不仅包括 DDR 还包含了 MMIO，因此线性映射区的虚拟地址不映射 MMIO。另外如果物理内存已经预留，那么线性区的虚拟内存也不会映射到系统预留物理内存上。最后线性映射区与物理内存可以建立 4KiB 的页表，也可以建立 2MiB 或者 1Gig 的页表，优先采用最大粒度的页表. 另外在 X86-64 架构中，如果按设备 DMA 的能力，将物理地址空间划分了多个 ZONE: 

* **ZONE_DMA** 区域给旧式的 ISA 设备做 DMA 使用，这些设备由于比较古老，其 DMA 访问的范围有限，其范围是 \[0x00000000, 0x00100000)
* **ZONE_DMA32** 是给比较新的设备做 DMA 的区域，这些设备已经可以进行 32bit 的 DMA 操作，因此其范围是 \[0x00100000, 0x100000000)
* **ZONE_NORMAL** 给支持 64bit 的 DMA 设备使用的物理区域，其范围是 \[0x100000000, MAX_PHYS).

![](/assets/PDB/HK/TH002399.png)

在 i386 架构中，由于向前兼容和历史原因，本身其虚拟内存才 4Gig，然后物理地址空间的一部分区域被 MMIO 占用，因此线性映射区域只能是 \[PAGE_OFFSET, high_memory) 区域，超过这个区域的虚拟内存不再是线性映射区域，内核只能通过其他手段间虚拟内存映射到非线性映射的物理内存。在 i386 架构中，内核也按照设备 DMA 能力和物理内存的访问能力，将物理地址空间划分成多个 ZONE:

* **ZONE_DMA** 区域给旧式的 ISA 设备做 DMA 使用，这些设备由于比较古老，其 DMA 访问的范围有限，其范围是 \[0x00000000, 0x00100000)
* **ZONE_NORMAL** 给支持 32bit 的 DMA 设备使用的物理区域, 该区域的物理内存属于线性映射区，其范围是 \[0x00100000, pa(high_memory)).
* **ZONE_HIGHMEM** 区域的物理内存是内核没有直接线性映射的区域，使用时需要建立页表之后才能访问.

###### VMALLOC 区域

![](/assets/PDB/HK/TH002400.png)

**VMALLOC 区域** 是 Vmalloc 分配器从内核空间划处一段虚拟内存，然后独立进行管理，目的是为内核提供虚拟内存是连续的，但映射的物理内存不连续的内存。在不同的架构中，VMALLOC 区域的范围可能有所不同，但内核统一将这个区域描述为 \[VMALLOC_START, VMALLOC_END). VMALLOC 区域的虚拟内存在分配时都是按 2MiB 粒度进行分配的，内核负责从 Buddy 分配器中分配 4KiB 的独立物理页，然后建立 2MiB 虚拟内存到零散 4KiB 的页表，如果将申请的虚拟内存设置的更大，那么就会形成大块连续的虚拟内存，而对应的物理内存则是零散的 4KiB 物理页，因此形成了虚拟地址连续而物理地址不连续的特点.

###### FIXMAP 映射区

![](/assets/PDB/HK/TH001991.png)

**FIXMAP 区域** 可以理解为系统预留虚拟内存区域，内核将 \[FIXADDR_START, FIXADDR_TOP) 的虚拟内存区域进行预留给特定的分配器或任务使用，内核的其他子系统就不能使用这部分虚拟内存。FIXMAP 区域包括了**固定映射分配器**维护的区域，其内部又包括了**Permanent Mapping Allocator(永久映射分配器)** 和 **KMAP Mapping Allocator(临时映射分配器)**， 以及 **Early I/O and Reserved Memory Allocator(早期 IO/预留内存分配器)**。因此可将该区域的虚拟内存已经预留给特定功能使用，这些功能各自维护各自的虚拟内存，然后各自获得物理内存并独立建立页表，以此使用各自的内存，最后还可以根据各自的特点进行内存释放回收.

---------------------------------------

##### <span id="D4">内存映射</span>

![](/assets/PDB/HK/TH002401.png)

**内存映射(Memory Mapping)**: 指将用户进程的虚拟内存或内核的虚拟内存映射到物理内存、文件或者外设上，那么用户进程或内核线程可以像访问普通内存一样访问文件或外设。如果映射的对象进行划分，那么分为文件映射和匿名映射。所谓的**文件映射(File mapping)**指将虚拟内存映射到文件系统的文件里，在虚拟内存和文件中间，内核引入了一段用于加速文件访问的缓存**Page Cache**, 其可以与后端文件进行同步和加速访问，虚拟内存也可以在 Page Cache 上采用回写等策略提供文件的访问效率; 所谓**匿名映射(Anonymous mapping)**是将虚拟内存映射到一块物理内存上，以此供进程使用. 如果按共享方式进行划分，那么分配共享映射和私有映射。所谓**共享映射(Shared mapping)**指的是进程可以和其他进程或子进程一起共享虚拟地址映射的物理内存; **私有映射(Privated mapping)**指的是进程独占虚拟地址映射的物理内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

#### X86 架构内存管理

![](/assets/PDB/HK/HK000427.png)

在 X86 架构的机器上，系统在上电之前将内存条被插入到内存插槽，系统首先启动 BIOS(Basic Input Output System)，BIOS 根据 CMOS/BDA/EBDA 中的信息信息进行硬件初始化，将系统初始化到一个已知状态。CMOS(Complementary Metal Oxide Semiconductor) 是电脑主板上的一块可读写的 ROM 芯片，存储着 BIOS 配置信息，其中也包含着物理内存相关的信息，BIOS 通过外设 IO 地址空间可以访问 BIOS，通过该通路获得了不同物理地址范围内可用物理内存的信息; BDA(BIOS Data Area) 是 RAM 里的一段数据，主要用于 BIOS 管理外设和资源，BIOS 将从 COMS 中获得的内存信息存储到 BDA 指定区域以便后续使用; EBDA(Extended BIOS Data Area) 同 BDA 一样，其也包含了一部分内存信息。BIOS 在硬件初始化完毕之后探测到所有的可用物理内存之后，并构建 BIOS 中断向量表 IVT(Interrupt Vector Table), 并提供多个向量给早期的内核使用。

![](/assets/PDB/HK/TH001450.png)

BIOS 在完成自己的使命之后将控制权移交给早期的内核，此时内核处于实时模式，CPU 直接通过物理地址访问物理内存。当内核初始化到一定阶段进入保护模式，内核临时为部分的内核空间虚拟内存建立页表并映射物理内存，并出现了大块连续的虚拟内存映射到大块连续的物理内存上，因此这部分区域称为线性映射区域，这部分区域只需简单的线性关系就可以获得虚拟内存和物理内存的映射关系，无需通过查页表获得。

![](/assets/PDB/HK/HK000320.png)

内核继续初始化，内核根据中断向量表 IVT 获得了系统可用物理内存的信息，并用这些信息构建了 E820 表，E820 表有多个 Entry 组成，每个 Entry 用于记录一段物理空间的内存信息，如果某段物理区域是内存，那么对应的 E820 Entry 就会将这段区域标记为内存，反之不是物理内存则标记为 Reserved。通过 E820 表可以知道物理内存在物理空间的布局。早期的 E820 表信息来自 BIOS IVT 中断向量表，那么称该表为 BIOS-E820 表。随着内核不断初始化，内核从 CMDLINE 中获得开发者对内存布局的规划, 开发者可以将指定范围的物理内存进行预留，预留之后的物理内存对系统不可见，并将 BIOS-E820 表进行改造。

![](/assets/PDB/HK/TH001451.png)

内核初始化到一定阶段之后将物理内存布局信息 E820 表传递给 MEMBLOCK 物理内存管理器，其作为早期的物理内存管理器，将内存分作两部分，一部分为 memory 即可用内存，另外一部分为 reserved 即为已经分配的内存，MEMBLOCK 使用区域的概念管理两种内存。对于系统预留内存可以是已经在使用的内存以及 CMDLINE 预留的内存，MEMBLOCK 将物理内存管理起来，以便供早期的内存分配需求，由于 MEMBLOCK 管理的物理内存都是已经内核空间虚拟内存建立页表的物理内存，因此 MEMBLOCK 分配器的物理内存内核可以直接使用(无需建立页表)。

![](/assets/PDB/HK/TH001452.png)

Linux 支持 Flat memory model、Discontiguous memory model 和 SPARSE memory model 三种内存模型。在平坦模式(Flat Model)下, 物理内存空间是一块平坦连续的空间，内核使用 struct page 数据结构数组 mem_map[], mem_map[] 数组的每个成员按顺序映射物理页，因此 page、PHY 和 PFN 就建立了独一无二的映射关系. 平坦模型虽然简单高效但存在一个缺点，对于空洞的区域还是照样分配 struct page 进行绑定，这会造成系统内存浪费.

![](/assets/PDB/HK/TH001453.png)

在稀疏模型(SPARSE Model)下，内核采用物理内存拆分为 SECTION_SIZE 大小的 SECTION 区域，那么物理内存被划分成多个 SECTION 区域，每个 SECTION 区域使用一个 struct mem_section 数据结构进行维护，其成员 section_mem_map 指>向一个 struct page 数组，数组中的成员与该 SECTION 内的物理页一一对应。SPARSE 使用一个 struct mem_section 数组 SECTION_ROOT 作为根节点与每个 SECTION 的 struct mem_section 构成树型布局，因此 SPARSE 模型构建了 page、mem_section、PHY 和 PFN 建立了独一无二的映射关系。稀疏模型让只有存在物理内存的区域与 struct page 进行绑定，而对于空洞的区域可以在热插内存时再与 struct page 进行绑定，很好的节省了系统内存开销。Discontiguous memory model 已经过时不进行讲解.

![](/assets/PDB/HK/TH001454.png)

内核根据用途将物理内存细分为不同的区域 (Zone), ZONE_DMA 为适用于 DMA 的内存域，该区域的长度依赖于处理器类型。在 IA32 架构中一般标记为 16MiB，这是由古老的 ISA 设备强加的边界，但现代计算机不受这一限制; ZONE_DMA32 适用于 32 位地址总线寻址和适用于 DMA 的内存域，显然 64 位系统上两种 DMA 才有区别，IA32 架构中 ZONE_DMA32 为空, AMD64 架构上 ZONE_DMA32 区域可能从 0 到 4Gig; ZONE_NORMAL 区域是可以直接映射到内核空间的普通内存区域，架构上保证都会存在 ZONE_NORMAL, 但无法保证该区域对应实际的物理内存。ZONE_HIGHMEM 区域为超出内核空间直接映射区域的物理内存，在 IA32 架构中内核空间可以直接映射的区域只有 896MiB，那么超出 896MiB 的物理内存都属于 ZONE_HIGHMEM，另外对于 AMD64 架构则不需要 ZONE_HIGHMEM; 另外内核还定义了 ZONE_MOVABLE 区域，在防止物理内存碎片机制中需要使用该内存区域。

![](/assets/PDB/RPI/RPI000827.png)

内核继续初始化，MEMBLOCK 内存分配器将可用的物理内存区域传递给 Buddy 内存分配器，Buddy 分配器将接受到的可用物理内存按 PAGE_SIZE 为基础单位进行管理，并与 struct page 进行绑定和初始化。Buddy 分配器在接收物理页的时候>会将连续相连的物理页合并成一个大的复合页，**复合页**是将多个 struct page 组合成一个 struct page 的物理页集合，此时复合页根据其物理页的数量划分为 2 的幂阶，不同阶的复合页维护在不同的 free_area[] 链表上，Buddy 一共维护了 MAX_ORDER 个 free_area 链表。此过程也是 Buddy 内存分配器的初始化过程，当 Buddy 初始化完毕之后会发现高阶的 free_area 链表上维护很多高阶复合页，此时 MEMBLOCK 分配器已经完成了使命将停止使用，接下来系统分配物理内存将由 Buddy 分配器负责。

![](/assets/PDB/RPI/RPI000835.png)
![](/assets/PDB/RPI/RPI000836.png)
![](/assets/PDB/RPI/RPI000837.png)
![](/assets/PDB/RPI/RPI000838.png)

内核在初始化的过程中, 内核为不同的 ZONE 都分配一个 Buddy 分配器，那么每个 Zone 就有各自的 free_area 链表。当系统需要分配物理内存时，Buddy 分配器从指定的 ZONE 的 free_area 链表上查找可用复合页，如果有那么将复合页从链表中移除并返回给调用者; 反之如果 free_area 链表中没有可用的复合页时，Buddy 分配器就从更高阶的 free_area 链表上查找可用的复合页，直到找到一个可用的复合页, 找到之后 Buddy 分配器将复合页一分为二，其中一个加入低阶的 free_area，另外一个如果满足需求就直接返回给调用者，如果大于需求的物理内存，那么 Buddy 分配器继续将复合页一分为二，以此类推直到找到合适的复合页为止。当使用者不再使用物理内存时，将物理内存归还给 Buddy 内存分配器，Buddy 分配器首先在复合页对应阶的 free_area 链表中查看其兄弟复合页是否空闲，如果空闲那么将其兄弟从当前 free_area 链表中移除，然后合并成一个更高阶的复合页, 以此类推直到没有可以合并的兄弟复合页为止。通过 Buddy 算法可以保持 Buddy 分配器中大块连续的物理内存, 从 ZONE_DMA32 和 ZONE_NORMAL 区域的物理页由于已经和内核空间线性映射，因此 Buddy 分配器从这些区域分配的内存存在: 物理地址和虚拟地址连续的特点

![](/assets/PDB/RPI/RPI000908.png)

由于内核高频使用单个物理页，如果单个物理页频繁在 Buddy 分配器进行分配回收势必影响性能，那么内核使用 PCP 分配器用于管理冷热物理链表，每个 ZONE 为每个 CPU 维护了一个 pageset 链表，链表上维护单个物理页且热的物理页位>于链表的前端，而相对冷的物理页位于链表的尾部，PCP 分配器维护一定数量的独立物理页，当 PCP 分配器维护的物理页数量小于某个值时，PCP 分配器从 Buddy 分配器一次性分配多个独立的物理页进行维护; 反之当 PCP 分配器中维护的物理页超过一定数量之后，PCP 分配器将部分冷的物理页归还给 Buddy 分配器。系统中有了 PCP 分配器的存在大大提供了物理内存的分配效率和性能。

![](/assets/PDB/RPI/TB000021.png)

在内核中高频使用小块内存，其粒度远远小于 PAGE_SIZE，有时就几个字节，为了向内核提供小粒度的内存，内核提供了 SLAB/SLOB/SLUB 内存分配器，其从 Buddy 分配器中获得一个或多个物理页，且这些物理页都是直接映射内核空间的，也就是内核空间的虚拟地址已经和这些物理页建立了页表，由于线性映射的缘故 Buddy 分配器分配这种物理页之后可以直接知道其对应的虚拟地址。SLAB 分配器在获得这类物理页之后将其划分成两部分，一部分被划分为同等长度的多个内存区>域称为 object，另外一部分用于管理 object 使用的 slab 区域，slab 区域内使用 bitmap 记录了 object 的使用情况，另外 object 存储着下一个 object 的地址，这样 object 就形成了一个链表，object 的长度就是 slab 提供的长度。当 SLAB 分配器分配一个 object 时，其从 slab 的 s_mem 获得一个可用的 object，然后将其从 object 链表中移除，并将 s_mem 指向下一个空闲的 object，并将获得的 object 返回给调用者; 当调用者使用完这个 object 之后，再次获>得 slab 的 s_mem，并将 object 插入该链表的头部。以上便是 SLAB 分配器使用物理内存的方法.

![](/assets/PDB/HK/HK000289.png)

在有的架构中内核的虚拟空间远远小于物理内存，那么只有一部分物理内存被线性映射，而绝大部分物理内存只有物理地址没有虚拟地址，线性映射区无法满足内核大块的连续虚拟内存需求，于是内核提供了 VMALLOC 内存分配器，其在内核空间划分了从 VMALLOC_START 到 VMALLOC_END 的虚拟区域，但内核有大块连续的虚拟内存需求时，VMALLOC 分配器就从该区域动态分配一段虚拟内存，然后从 Buddy 分配器中分配多个独立的物理页，并建立虚拟内存到这些物理页的页表，最后再把虚拟内存返回给调用者使用; 当调用者不再使用时，VMALLOC 回收这段虚拟内存为可用区域，并将其页表清除，释放物理页回 Buddy 分配器。由于物理页是独立的缘故，所以不能确保物理内存是连续的，因此 VMALLOC 分配器分配的内存特点是: 虚拟内存连续但物理内存不一定连续.

![](/assets/PDB/HK/TH001455.png)

Buddy 分配器能够提供的最大连续物理内存为 8MiB, 但有的需求场景需要大块连续的物理内存，于是内核提供了 CMA/DMA 分配器来实现大块连续内存的分配。CMA 分配器可以在内核初始化过程中，通过 CMDLINE 修改 E820 表将指定的物理内存区域进行预留，预留之后的物理内存系统不会对齐进行初始化，待内核初始化到一定程度，CMA 内存分配器在使用 bitmap 管理预留的内存，当调用者需要大块连续物理内存时，CMA 从 bitmap 中找到空闲的区域并将对应的 bit 置位，然后将连续的物理内存返回给调用者; 当调用者不再使用物理内存时，CMA 回收物理内存并将 bitmap 中对应的 bit 清零。CMA 由会独立维护大段物理内存，但当系统内存吃紧时，CMA 也会将部分物理内存迁移给 Buddy 分配器使用，反之当 CMA 需要更多连续物理内存时，内核也会尽量迁移腾挪出连续物理内存给 CMA 分配器使用.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)
