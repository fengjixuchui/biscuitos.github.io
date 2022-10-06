---
layout: post
title:  "Broiler PIO/MMIO Virtualization Technology"
date:   2022-09-02 12:00:00 +0800
categories: [MMU]
excerpt: Virtualization.
tags:
  - HypV
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [X86 PIO/MMIO 介绍](#A)
>
> - [X86 PIO 虚拟化](#G)
>
>   - [In-Broiler 设备 PIO 虚拟化](#G1)
>
>   - [In-Kernel 设备 PIO 虚拟化](#G2)
>
> - [X86 MMIO 虚拟化](#H)
>
>   - [In-Broiler 设备 MMIO 虚拟化](#H8)
>
>   - [In-Kernel 设备 MMIO 虚拟化](#H9)
>
> - [同步 PIO 端口虚拟化](#B)
>
>   - [In-Broiler 设备同步 PIO 读模拟流程](#B5)
>
>   - [In-Broiler 设备同步 PIO 写模拟流程](#B6)
>
>   - [In-Kernel 设备同步 PIO 读模拟流程](#B7)
>
>   - [In-Kernel 设备同步 PIO 写模拟流程](#B8)
>
>   - [In-Broiler 设备使用同步 PIO](#B2)
>
>   - [In-Kernel 设备使用同步 PIO](#B3)
>
>   - [PCI 设备使用同步 PIO](#B4)
>
> - [异步 PIO 端口虚拟化](#C)
>
>   - [In-Broiler 设备异步 PIO 写流程](#C2)
>
>   - [In-Broiler 设备使用异步 PIO](#C3)
>
>   - [PCI 设备使用异步 PIO](#C4)
>
> - [同步 MMIO 虚拟化](#D)
>
>   - [In-Broiler 设备同步 MMIO 读模拟流程](#D1)
>
>   - [In-Broiler 设备同步 MMIO 写模拟流程](#D2)
>
>   - [In-Kernel 设备同步 MMIO 读模拟流程](#D3)
>
>   - [In-Kernel 设备同步 MMIO 写模拟流程](#D4)
>
>   - [In-Broiler 设备使用同步 MMIO](#D5)  
>
>   - [In-Kernel 设备使用同步 MMIO](#D6)
>
>   - [PCI 设备使用同步 MMIO](#D7)
>
> - [异步 MMIO 虚拟化](#E)
>
>   - [In-Broiler 设备异步 MMIO 写流程](#E2)
>
>   - [In-Broiler 设备使用异步 MMIO](#E3)
>
>   - [PCI 设备使用异步 MMIO](#E4)
>
> - X86 PIO/MMIO 虚拟化进阶研究
>
>   - [KVM 与 Broiler 之间 PIO/MMIO 数据桥梁](#P1)

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### X86 PIO/MMIO 介绍

![](/assets/PDB/HK/TH001481.png)

在 Intel X86 架构中，系统物理地址总线可寻址的空间称为**存储域**，其主要由两部分组成，一部分是 DDR 控制器管理的 **DDR 域**，另外一部分是由外设的寄存器映射到存储域的 **MMIO (Memory-Mapped IO)**。其中 PCI/PCIe 设备的寄存器映射到存储域的 MMIO 称为 **PCI 总线地址**, 另外 Intel X86 架构也通过 Host PCI 主桥/Root Port(RC) 维护一颗 PCI 总线，该 PCI 总线可寻址的空间称为**PCI 总线域**.

![](/assets/PDB/HK/TH001695.png)

在 X86 架构中，存储域空间长度和 PCI 总线域长度是相同的，并且 DDR 域的地址可以映射到存储域，也可以映射到 PCI 总线域，同理 PCI 域的地址也可以映射到存储域，且该地址在存储域上称为 PCI 总线地址。

![](/assets/PDB/HK/TH001711.png)

在 Broiler 模拟的 X86 架构中，DDR 映射范围被划分做两段，第一段是 \[0x00000000, 0xC0000000), 大小为 3Gig 内存, 第二段从 0x100000000 开始. 外设寄存器映射到存储域 MMIO 的范围是 \[0xC0000000, 0x100000000), 该区域包括了 PCI 总线地址，同时也包括了其他外设存储空间映射的区域，例如 Local APIC 映射的 MMIO 区域.

![](/assets/PDB/HK/TH001713.png)

上图是一个 Broiler 运行的 4Gig 虚拟机，通过 “/proc/iomem” 接口可以看到系统存储域的布局，可以看到 System RAM 占用的范围是 [0x00100000, 0xbfffffff], 以及 [0x100000000, 0x13fffffff]. 另外可以看到 PCI 的 MMIO 从 0xC2000000 开始. 

![](/assets/PDB/HK/TH001715.png)

在 X86 架构中除了可以通过存储域与外设或者内存交换数据，X86 还提供了 I/O Port 与外设进行数据交互. X86 将通过 I/O Port 进行寻址的空间称为 **I/O 空间**. IO 空间与存储域是相互独立的寻址空间，外设既可以将寄存器映射到 IO 空间，也可以映射到存储域. 系统需要使用独立的指令才能访问 IO 空间，而可以像访问普通内存一样访问 MMIO.

![](/assets/PDB/HK/TH001871.png)

无论使用 IO Port 访问外设的寄存器，还是使用 MMIO 访问外设的寄存器，本文统一称为对外设 IO 的访问。当系统向外设发起 IO 操作(Read/Write) 时，如果 CPU 需要等待 IO 完成之后再继续执行其他指令，那么称这种 IO 为**同步 IO(Synchronous IO)**; 如果 CPU 不需要等待 IO 完成就可以继续执行其他指令，当外设处理完 IO 之后再通过中断告诉 CPU IO 已经处理完成，那么称这种 IO 为**异步 IO(Asynchronous IO)**. 同步 IO 操作譬如对 IO Port/MMIO 的读写操作，异步 IO 操作譬如 Doorball/Kick Register 等.

> [Linux 使用 PIO]()
>
> [Linux 使用 MMIO]()

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### X86 PIO 虚拟化

![](/assets/PDB/HK/TH001442.png)

在 X86 架构下，CPU 与外设沟通的方法包括: 存储域、IO Space 和中断。外设可以通过将其内部的寄存器硬件上映射到 IO Space 或者存储域，然后 CPU 通过访问 IO Space 或者存储域实现与外设的数据交互。根据前文了解到外设将其内部寄存器映射到 IO Space 的方式称为 PIO，CPU 可以通过访问 IO Space 的 IO Port 访问外设的寄存器.

![](/assets/PDB/HK/TH001715.png)

在 Linux 里可以通过 cat /proc/ioport 查看系统的 IO Space 布局，其有多个 IO Port 构成，例如用于访问 PCI 配置空间的 0xCFC/0xCF8. Intel 提供了 IN/OUT 等指令可以直接访问 IO Space.

![](/assets/PDB/HK/TH001924.png)

Intel 支持两种方式访问 PIO 产生 VM-EXIT，在 VMCS 的 Primary Processor-Based VM-Execution Controls 寄存器中存在两个相关标志位. 当 BIT24 置位，那么当虚拟机内部使用 IN/INS/INSB/INSW/INSD 以及 OUT/OUTS/OUTSB/OUTSW/OUTSD 时虚拟机会发生 VM-EXIT; 当 BIT25 置位，KVM 会维护一个 IO Bitmap, 每个 bit 对应一个 IO Port，当某个 bit 置位，那么虚拟机访问对应的 IO Port 会发生 VM-EXIT，反之如果某个 bit 清零，那么虚拟机访问对应的 IO Port 不会导致 VM-EXIT. 两种方法只能是能其中的一种，通常使用 BIT24 模式.

![](/assets/PDB/HK/TH001873.png)

当虚拟机因为访问 PIO 导致 VM-EXIT 时，可以从 VMCS 的 Exit Qualification 寄存器中获得虚拟机访问 PIO 的行为。Exit Qualification 寄存器的布局如上图，其高 64 位预留而低 6-15 位也同样预留. 其布局含义如下:

* BIT0-2: Size of Access 用于描述 PIO 的长度
* BIT3: Direction 用于描述 PIO 操作是读还是写，置位则为读，清零则为写
* BIT4: String Instruction 指明 PIO 指令是否为串行指令，即一个 PIO 包括多个重复指令
* BIT16-31: Port Number 指明 IO Port 的端口号.

![](/assets/PDB/HK/TH001872.png)

在 PIO 虚拟化中, 任何 PIO 都不能独立存在，而是要基于模拟设备，模拟设备可以位于内核空间，那么称这类模拟设备为 In-Kernel 设备，而有的设备模拟位于用户空间的 Broiler，那么称这类设备为 In-Broiler 设备. 无论是哪类设备都可以对 PIO 进行模拟. 虚拟机内部通过 IO Port 访问 PIO，那么会导致 VM-EXIT。KVM 捕获到因为 PIO 导致的 VM-EXIT 之后，获得虚拟机访问 PIO 的信息。如果模拟设备位于 Kernel，那么直接让设备进行 PIO 模拟，模拟完毕之后再恢复虚拟机运行，虚拟机从指定的寄存器获得模拟的值; 如果虚拟设备位于用户空间，那么 KVM 进行 KVM_RUN ioctl 返回之后，Broiler 捕获到虚拟机因为 PIO 退出，那么 Broiler 找到对应的设备进行 PIO 模拟，模拟完毕之后 Broiler 再次让虚拟机运行，虚拟机从指定的寄存器获得模拟的值。

![](/assets/PDB/HK/TH001934.png)

Broiler 通过一棵红黑树实现了对 IO Space 的模拟，每个红黑树的叶子维护某个 In-Broiler 设备的 IO Port，由于红黑树的特点，左子树维护的 IO Port 比右子树维护的 IO Port 小。因此 Broiler 可以根据 IO Port 快速找到叶子，然后从叶子中获得对应的 In-Broiler 设备.

> [In-Broiler 设备 PIO 虚拟化](#G1)

![](/assets/PDB/HK/TH001896.png)

在 KVM 中每个虚拟机维护了多条虚拟总线，其中包括一条 KVM_PIO_BUS 虚拟 IO Space 总线，总线上挂载多个 struct kvm_io_range 节点，节点描述了 IO PORT 的范围以及指向了 In-Kernel 设备。因此 KVM 可以根据 PIO 找到对应的 In-Kernel，然后进行 PIO 模拟.

> [In-Kernel 设备 PIO 虚拟化](#G2)

![](/assets/PDB/HK/TH001871.png)

**同步 PIO(Synchronous IO)** 指的是 CPU 对 IO Port 发起读写操作之后，需要等待外设处理完毕并通知 CPU 之后，CPU 才可以继续执行, 一般情况下同步 PIO 包括读写操作，需要在指令周期内完成. 相对于同步 PIO，**异步 PIO(Asynchronous PIO)** 在发起写操作之后，无需等待外设处理完毕就可以直接运行，外设处理完毕之后可以中断告诉 CPU，CPU 也可以通过轮询的方式查看异步 PIO 的完成情况，一般情况下，异步 PIO 操作为写操作.

> [同步 PIO 端口虚拟化](#B)
>
> [异步 PIO 端口虚拟化](#C)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------

##### <span id="G1">In-Broiler 设备 PIO 虚拟化</span>

![](/assets/PDB/HK/TH001925.png)

1. 虚拟机通过 IN 指令访问 IO Port 导致虚拟机 VM-EXIT
2. KVM 捕获到虚拟机退出的原因是 EXIT_REASON_IO_INSTRUCTION
3. KVM 调用 handle_io 获得访问 PIO 相关信息
4. KVM 将模拟 PIO 相关信息存储在 struct kvm_run 中
5. KVM 将 vcpu->arch.complete_userspace_io 设置为 complete_fast_pio_in
6. KVM 退出到 Broiler 且退出原因是 KVM_EXIT_IO/KVM_EXIT_IO_IN
7. Broiler 收到退出的原因是 KVM_EXIT_IO，接着找到对应的设备
8. 指定的设备进行读写模拟，模拟完毕之后 Broiler 通过 KVM_RUN 恢复虚拟机运行
9. KVM 收到 Broielr 恢复虚拟机运行的 KVM_RUN IOCTL
10. KVM 检查 vcpu->arch.complete_userspace_io 是否需要模拟 PIO/MMIO
11. KVM 确认模拟 PIO，并调用 complete_fast_pio_in
12. KVM 将 Broiler 设备模拟的值通过 kvm_rax_write 写入 RAX 寄存器
13. 由于 IN/OUT 指令使用固定寄存器，因此不需要指令模拟
14. 虚拟机恢复运行，继续执行 IN 指令并从 RAX 寄存器获得模拟的值

![](/assets/PDB/HK/TH001926.png)

KVM 捕获到 VM-EXIT 退出的原因是 EXIT_REASON_IO_INSTRUCTION 之后调用 handle_i-o() 函数，函数可以从 VMCS 的 Exit Qualification 寄存器中获得需要模拟 PIO 的端口号、数据长度、PIO 方向等信息。函数最后调用 kvm_fast_pio() 函数进行 PIO 模拟.

![](/assets/PDB/HK/TH001927.png)

kvm_fast_pio_in() 函数通过调用 emulator_pio_in() 函数进行 IO 模拟，如果 PIO 对应对应的设备不是 In-Kernel 设备，那么其将继续执行 71 行分支，并将 vcpu->arch.complete_userspace_-io 设置为 complete_fast_pio_in，这个设置用于虚拟机恢复运行之前更新访问 PIO 相关的寄存器.

![](/assets/PDB/HK/TH001928.png)

emulator_pio_in_out() 函数首先记录 PIO 模拟的基础信息，接着处理 PIO 模拟时检查到 PIO 对应的设备是 In-Kernel 设备，那么直接调用 kernel_pio() 函数直接进行 PIO 模拟, 模拟完毕之后进入虚拟机恢复运行流程; 反之 PIO 对应设备来自 Broiler，那么需要将 PIO 模拟的相关信息存储在 struct kvm_run 数据结构中，并将退出原因标记为 KVM_EXIT_IO 和 KVM_EXIT_IO_IN，并退通过 KVM_RUN IOCTL 退出到 Broiler.

![](/assets/PDB/HK/TH001934.png)

Broiler 通过一棵红黑树实现了对 IO Space 的模拟，每个红黑树的叶子维护某个 In-Broiler 设备的 IO Port，由于红黑树的特点，左子树维护的 IO Port 比右子树维护的 IO Port 小。因此 Broiler 可以根据 IO Port 快速找到叶子，然后从叶子中获得对应的 In-Broiler 设备.

![](/assets/PDB/HK/TH001929.png)

Broiler 从 KVM_RUN IOCTL 返回之后，检查到退出的原因是 KVM_EXIT_IO，那么调用 bro-iler_cpu_emulate_io() 函数找到 PIO 对应的设备，然后进行 PIO 模拟，模拟完毕之后 Broiler 再次调用 broiler_cpu_run() 函数告诉 KVM 恢复虚拟机的运行.

![](/assets/PDB/HK/TH001930.png)

KVM 收到 KVM_RUN IOCTL 之后调用 kvm_arch_vcpu_ioctl_run() 函数进行虚拟机的恢复运行流程，其中包含了上图的逻辑，其会检查虚拟机恢复之前是否需要处理 PIO 或者 MMIO，函数在 33 行通过 vcpu->arch.complete_userspace_io 变量进行判断，通过上文的分析可以知道此时指向了 complete_fast_pio_in，那么 KVM 执行 34 行分支，并运行 complete_fast_pio_in.

![](/assets/PDB/HK/TH001931.png)

complete_fast_pio_in() 函数通过在 45 行调用 emulator_pio_in() 获得设备模拟的 PIO 值，然后调用 kvm_rax_write() 函数将模拟的值写入到 RAX 寄存器中，虚拟机恢复运行之前 VMCS Guest 区域的 RAX 会更新成 PIO 模拟的值。由于虚拟机内部对 PIO 访问的指令使用固定的寄存器，因此不需要进行指令模拟，函数直接调用 kvm_skip_emulated_instruction(). 最后虚拟机恢复运行之后，继续执行中断的 PIO 访问指令，并从指定寄存器中获得模拟的值。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------

##### <span id="G2">In-Kernel 设备 PIO 虚拟化</span>

![](/assets/PDB/HK/TH001932.png)

1. 虚拟机通过 IN 指令访问 IO Port 导致虚拟机 VM-EXIT
2. KVM 捕获到虚拟机退出的原因是 EXIT_REASON_IO_INSTRUCTION
3. KVM 调用 handle_io 获得访问 PIO 相关信息
4. KVM 检查到 PIO 属于 In-Kernel 设备，直接进行 PIO 模拟
5. KVM 将 PIO 模拟完毕的值更新到 VMCS Guest RAX 区域
6. 由于 IN/OUT 指令使用固定寄存器，因此不需要指令模拟
7. 虚拟机恢复运行，继续执行 IN 指令并从 RAX 寄存器获得模拟的值

![](/assets/PDB/HK/TH001926.png)

KVM 捕获到 VM-EXIT 退出的原因是 EXIT_REASON_IO_INSTRUCTION 之后调用 handle_i-o() 函数，函数可以从 VMCS 的 Exit Qualification 寄存器中获得需要模拟 PIO 的端口号、数据长度、PIO 方向等信息。函数最后调用 kvm_fast_pio() 函数进行 PIO 模拟.

![](/assets/PDB/HK/TH001927.png)

kvm_fast_pio_in() 函数通过调用 emulator_pio_in() 函数进行 IO 模拟，由于 PIO 所属的设备是 In-Kernel 设备，那么函数在 65 行调用 emulator_pio_in() 函数直接进行 PIO 模拟，模拟完毕之后进入 67 行分支，并调用 kvm_rax_write() 函数将 PIO 模拟的结果填入 RAX 寄存器.

![](/assets/PDB/HK/TH001928.png)

emulator_pio_in_out() 函数首先记录 PIO 模拟的基础信息，接着处理 PIO 模拟时检查到 PIO 对应的设备是 In-Kernel 设备，那么直接调用 kernel_pio() 函数直接进行 PIO 模拟, 模拟完毕之后进入虚拟机恢复运行流程, 那么 62 行分支不会被执行.

![](/assets/PDB/HK/TH001896.png)

KVM 维护了多条虚拟总线，其中一条为 IO Space 虚拟总线，所有 In-Kernel 设备使用的 IO Port 区域都维护在 KVM_PIO_BUS 虚拟总线上。KVM 在进行 In-kernel 设备 PIO 模拟时通过 PIO 地址在 KVM_PIO_BUS 虚拟总线上找到对应的设备，接着 In-Kernel 设备进行 PIO 模拟.

![](/assets/PDB/HK/TH001933.png)

KVM 调用 \_\_kvm_io_bus_read() 函数进行 In-Kernel 设备的 PIO 模拟，其首先调用 kvm_io_bus_get_first_dev() 函数获得 KVM_PIO_BUS 虚拟总线，然后在 61 分支中根据 PIO 地址找到 KVM_PIO_BUS 对应的 In-Kernel 设备，但找到对应的 In-Kernel 设备就进行 PIO 模拟. PIO 模拟完毕之后 KVM 直接进行虚拟机恢复运行流程，最后虚拟机恢复运行并继续执行 PIO 访问指令，并从指令对应的寄存器中获得 PIO 模拟的值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="H"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

#### X86 MMIO 虚拟化

![](/assets/PDB/HK/TH001712.png)

在 Intel X86 架构下，CPU 与外接沟通的方式包括: 存储域、IO Space 和中断. 外设可以通过将其内部寄存器映射到 IO Space 或者物理空间，然后 CPU 通过访问访问存储域或者 IO Space 实现与外设的交互。根据前文了解到外设将其内部寄存器映射到存储域的方式称为 **MMIO(Memory Mapped IO)**，CPU 可以使用访问存储域的指令去访问外设的寄存器。因此存储域不仅由 DDR 构成还包括 MMIO 区域.

![](/assets/PDB/HK/TH001905.png)

既然 CPU 可以向访问普通内存一样访问 MMIO，那么需要先为 MMIO 建立页表，与普通物理内存页表不同的是，MMIO 的页表需要在使用之前建立，缺页不会为 MMIO 区域建立页表. 另外一点是普通内存的带缓存的，而 MMIO 是不带缓存(NO-CACHE) 的，因此需要在页表中标明，其余域普通物理内存的页表无异。

![](/assets/PDB/HK/HK000732.png)

在 MMIO 虚拟化中，MMIO 的虚拟也和普通物理内存虚拟化一样，系统会为 Guset OS 的 MMIO 建立一个 EPT 页表，用于将 GMMIO(Guest MMIO) 映射到 HMMIO(Host MMIO), 但由于 HMMIO 的实现不同，可能 HMMIO 也需要 KVM、QEMU 或者 Broiler 进行模拟. 一旦 EPT 页表建立之后虚拟机再次访问 GPA 将不会导致虚拟机 VM-EXIT，但对于 GPA 为 MMIO 的场景，需要虚拟机每访问一次 MMIO 就 VM-EXIT，并对 MMIO 进行读写模拟。为了解决这个问题需要将 MMIO 的虚拟化分作两个阶段，第一阶段就是虚拟机第一次访问 MMIO-GPA，此时会导致 VM-EXIT 并建立 EPT 页表，但对于 MMIO 最后一级 EPT 页表要进行特殊处理，将最后一级页表配置为 "只写" 和 "可执行"，这是一个错误的页表属性组合(MisConfigure); 第二阶段是虚拟机非首次访问 MMIO，由于 MMIO-GPA 的 EPT 页表已经存在，但硬件会检查到其最后一级的 EPT 页表是错误配置，那么同样会导致虚拟机 VM-EXIT.

![](/assets/PDB/HK/TH001935.png)

Broiler 通过一棵红黑树实现了对存储域的 MMIO 区域进行管理，每个红黑树的叶子维护某个 In-Broiler 设备的映射的一段 MMIO 区域，由于红黑树的特点，左子树维护的 MMIO 区域比右子树维护的 MMIO 区域小。因此 Broiler 可以根据 MMIO 地址快速找到叶子，然后从叶子中获得对应的 In-Broiler 设备.

> [In-Broiler 设备 MMIO 虚拟化](#H8)

![](/assets/PDB/HK/TH001896.png)

在 KVM 中每个虚拟机维护了多条虚拟总线，其中包括一条 KVM_MMIO_BUS 虚拟 MMIO 总线，总线上挂载多个 struct kvm_io_range 节点，节点描述了 MMIO 的范围以及指向了 In-Kernel 设备。因此 KVM 可以根据 MMIO 地址找到对应的 In-Kernel，然后进行 MMIO 模拟.

> [In-Kernel 设备 MMIO 虚拟化](#H9)

![](/assets/PDB/HK/TH001904.png)

**同步 MMIO(Synchronous Memory Mapped IO)** 指的是 CPU 对 MMIO 发起读写操作之后，需要等待外设处理完毕并通知 CPU 之后，CPU 才可以继续执行, 一般情况下同步 MMIO 包括读写操作，需要在指令周期内完成. 相对于同步 MMIO，**异步 MMIO(Asynchronous Memory Mapped IO)** 在发起写操作之后，无需等待外设处理完毕就可以直接运行，外设处理完毕之后可以中断告诉 CPU，CPU 也可以通过轮询的方式查看异步 MMIO 的完成情况，一般情况下，异步 MMIO 操作为写操作. 

> [同步 MMIO 端口虚拟化](#D)
>
> [异步 MMIO 端口虚拟化](#E)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

##### <span id="H8">In-Broiler 设备 MMIO 虚拟化</span>

![](/assets/PDB/HK/TH001936.png)

在 MMIO 虚拟化中，MMIO 的虚拟也和普通物理内存虚拟化一样，系统会为 Guset OS 的 MMIO 建立一个 EPT 页表，用于将 GMMIO(Guest MMIO) 映射到 HMMIO(Host MMIO). 一旦 EPT 页表建立之后虚拟机再次访问 GPA 将不会导致虚拟机 VM-EXIT，但对于 GPA 为 MMIO 的场景，需要虚拟机每访问一次 MMIO 就 VM-EXIT，并对 MMIO 进行读写模拟。为了解决这个问题需要将 MMIO 的虚拟化分作两个阶段，第一阶段就是虚拟机第一次访问 MMIO-GPA，此时会导致 VM-EXIT 并建立 EPT 页表，但对于 MMIO 最后一级 EPT 页表要进行特殊处理，将最后一级页表配置为 "只写" 和 "可执行"，这是一个错误的页表属性组合(MisConfigure); 第二阶段是虚拟机非首次访问 MMIO，由于 MMIO-GPA 的 EPT 页表已经存在，但硬件会检查到其最后一级的 EPT 页表是错误配置，那么同样会导致虚拟机 VM-EXIT. 对于 In-Broiler 设备，KVM 在捕获到虚拟机因为访问 MMIO VM-EXIT 之后，会将 MMIO 模拟的任务交给用户空间的 Broiler，待 MMIO 模拟完毕之后 Broiler 通过 KVM 再次恢复执行，具体细节如下:

> [虚拟机首次访问 In-Broiler 设备的 MMIO](#H1)
>
> [虚拟机非首次访问 In-Broiler 设备的 MMIO](#H2)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="H1">虚拟机首次访问 In-Broiler 设备的 MMIO</span>

![](/assets/PDB/HK/TH001906.png)

1. 虚拟机首次访问 MMIO 导致 VM-EXIT，退出原因是 EXIT_REASON_EPT_VIOLATION
2. KVM 为 MMIO 建立 EPT 页表，但其属性配置为专用属性(异常属性 Misconfig)
3. KVM 对访问 MMIO 的指令进行解码，以此获得指令使用的寄存器等信息
4. KVM 对访问 MMIO 的指令进行模拟，并对指令模拟标记为 complete_emulated_mmio
5. KVM 发现 MMIO 对应的设备来自 Broiler
6. KVM 将 MMIO 模拟的信息存储在 struct kvm_run, 并以 KVM_EXIT_MMIO 退出到 Broiler
7. Broiler 收到退出的原因是 KVM_EXIT_MMIO, 然后找到对应设备进行读写模拟
8. MMIO 模拟完毕之后，Broiler 调用 KVM_RUN 再次运行虚拟机
9. KVM 收到 KVM_RUN IOCTL 之后，并检查到 complete_emulated_mmio
10. KVM 再次模拟 MMIO 访问的指令，将模拟的值存储到指定寄存器
11. KVM 在将入虚拟机之前将 Dirty 寄存器更新到 VMCS 的 Guest 区域
12. 虚拟机恢复运行之后，从访问 MMIO 的寄存器获得所需的值

![](/assets/PDB/HK/TH001907.png)

当虚拟机第一次访问 MMIO 时，虚拟机会因为 EXIT_REASON_EPT_VIOLATION 退出，可以 VMCS Exit Qualification 中获得具体的 MMIO 信息，并从 VMCS 的 GUEST_PHYSICAL_A-DDRESS 域中获得 MMIO 的地址。KVM 首先为 MMIO 建立 EPT 页表，除最后一级 EPT 页表其余级页表都与普通内存 EPT 页表一致，因此 KVM 会进入 kvm_tdp_page_fault() 函数为 MMIO 建立非最后一级 EPT 页表.

![](/assets/PDB/HK/TH001908.png)

对于 MMIO EPT 的最后一级页表，因为其不是映射普通内存，而且需要每次访问 MMIO 时让 VM-EXIT，并进行 MMIO 读写模拟，因此 KVM 调用 set_spte() 函数为 MMIO 建立最后一级页表时，调用 set_mmio_spte() 函数将 MMIO 最后一级 EPT 页表标记为一种错误(MissConfig)的属性，这样虚拟机再次访问 MMIO 时因为 EPT 页表属性错误会导致 VM-EXIT.

![](/assets/PDB/HK/TH001909.png)

KVM 通过 ept_set_mmio_spte_mask() 函数生成 MMIO EPT 最后一级页表的内容，其通过将页表设置为"只写"和"可执行"，正常页表逻辑中"可执行"必须与"可读"配合使用, 此时 MMIO 最后一级 EPT 页表这么配置就是一种错误的配置，虚拟机再次访问 MMIO 会因为页表配置错误导致 VM-EXIT.

![](/assets/PDB/HK/TH001910.png)

当 KVM 为 MMIO 建立好 EPT 页表之后，KVM 对访问 MMIO 的指令进行模拟，那么为什么要对指令进行模拟呢? 首先从虚拟机内部访问 MMIO 的代码说起, 一般在内核中使用 ioremap() C 函数分配一段虚拟地址映射到 MMIO(建立页表), 然后通过访问虚拟地址来访问 MMIO. 将 C 代码通过反汇编转换为汇编可以看到其对应的机器码和汇编代码，可以看到 CPU 可以像访问普通内存一样使用 MOV 指令访问 MMIO. 但有点不同的是 CPU 在使用 MOV 访问 MMIO 是可以使用不同类型的寄存器存储 MMIO 的值，或者有多种方式寻址 MMIO 地址, 由于这个原因的存在 KVM 需要知道访问 MMIO 使用了那些寄存器，以便 MMIO 模拟完毕之后将结果存储到指定寄存器，当虚拟机再次恢复运行之后就可以从指定寄存器获得访问 MMIO 的结果, 因此 KVM 需要对访问 MMIO 的指令进行解析和模拟.

![](/assets/PDB/HK/TH001911.png)

在讲解 KVM 指令模拟之前，需要对 X86 指令结构有一定的研究，细节可以参考 SDM 手册卷二. X86 指令的一部分是指令前缀(Instruction Prefixes), 例如 REP 和 Lock 前缀。第二部分是操作码(Opcode), 其是一个指令索引并指定指令的特定变体; 第三部分 ModR/M 字段指定寻址和操作数，符号 "R/M(Register/Memory)" 代表的是寄存器或内存; 第四部分 SIB 字段称为伸缩索引字段(Scale index byteundefined SIB), 用于计算数组索引偏移量; 第五部分表示地址位移字段，其保存了操作数的偏移量; 第六部分表示立即数字段，用于保存常量操作数. 具体细节参考下文:

![](/assets/PDB/HK/TH001920.png)

> [X86 指令编码简述](https://blog.csdn.net/xiao__1bai/article/details/126584837)
>
> [X86 汇编助记符指令机器码对应表](https://blog.csdn.net/weixin_43708844/article/details/103211703)

KVM 调用 x86_emulate_instruction() 函数对访问 MMIO 的指令进行模拟，首先是 x86_decode_insn() 函数将指令进行解码，然后 x86_emulate_insn() 函数进行实际的模拟，KVM 从指令解码中获得此次是 MOV 指令，并且是对一个地址的访问，并将访问的结果存储在 EDI 寄存中，那么其调用 segmented_read() 函数模拟 MOV 指令

![](/assets/PDB/HK/TH001912.png)

emulator_read_write() 函数是对 MMIO 读写进行模拟，函数调用 emulator_read_write_onepage() 判断 MMIO 是否属于 In-Kernel 设备，如果属于则找到对应的 In-Kernel 设备进行 MMIO 读写模拟，如果不是则执行 304 行的分支，此时 KVM 将 MMIO 需要模拟的信息存储在 struct kvm_run 数据结构中，并将退出到 Broiler 的原因标记为 KVM_EXIT_MMIO, 这里还有一个细节，KVM 将 vcpu 的 mmio_enabled 设置为 1, 函数执行完毕之后就进行返回.

![](/assets/PDB/HK/TH001913.png)

KVM 返回到 x86_emulate_instruction() 函数中执行时，如果检查到虚拟机再次恢复运行时需要处理 MMIO，那么函数执行 429 行分支，将 vcpu->arch.complete_userspace_io 设置为 complete_emulated_mmio。最后函数基于 KVM_RUN IOCTL 返回到 Broiler.

![](/assets/PDB/HK/TH001914.png)

Broiler 从 KVM_RUN 返回中得知是因为 KVM_EXIT_MMIO 退出的，那么 Broiler 调用 broiler_cpu_emulate_mmio() 函数找到 MMIO 对应的设备，然后进行读写模拟。当设备读写模拟完成之后，Broiler 再次调用 37 行 broiler_cpu_run() 函数告诉 KVM 恢复虚拟机的运行.

![](/assets/PDB/HK/TH001915.png)

KVM 接收到 KVM_RUN IOCTL 之后调用 kvm_arch_vcpu_ioctl_run() 函数恢复虚拟机的运行，在该函数中存在上图的判断，如果 vcpu->arch.complete_userspace_io 不空，那么说明虚拟机恢复之后要处理 PIO 或者 MMIO，通过之前的分析此时 vcpu->arch.complete_userspace_io 指向 complete_emulated_mmio() 函数，那么函数进入 20 行分支执行， 函数此时调用 complete_e-mulated_mmio() 函数.

![](/assets/PDB/HK/TH001916.png)

complete_emulated_mmio() 函数的主要作用是在虚拟机恢复之前，再次解析虚拟机 VM-EXIT 时的指令，然后将 MMIO 模拟的值存储到指定寄存器中，最后 KVM 更新到虚拟机 VMCS Guest 域指定寄存器中，待虚拟机恢复运行之后，可以从正在执行指令对应的寄存器中获得所需的值。188 行将设备模拟的 MMIO 值存储到 frag->data 中，然后函数进入 202 行分支，并继续调用 complete_emulate_io() 函数对指令进行模拟.

![](/assets/PDB/HK/TH001917.png)

complete_emulated_io() 函数再次调用 kvm_emulate_instruction() 函数对指令进行模拟，但是此时通过 EMULTYPE_NO_DECODE 指明不需要解析指令，直接进行指令模拟。与之前的指令模拟有所不同的是，KVM 已经知道模拟的指令以及使用的寄存器了，现在只需将模拟的值写入到指定寄存器即可

![](/assets/PDB/HK/TH001918.png)

KVM 找到需要更新的寄存器，调用 kvm_register_write() 函数对需要更新的寄存器写入新值，函数通过将 MMIO 模拟的值写入到 vcpu->arch.regs[] 指定的寄存器中，然后调用 kvm_register_mark_dirty() 函数将寄存器标记为 Dirty，那么虚拟机在恢复运行时，KVM 就会更新 VMCS Guest 中指定的域。

![](/assets/PDB/HK/TH001919.png)

最后虚拟机恢复执行，虚拟机继续执行 VM-EXIT 的指令，例如本案例中，访问 MMIO 使用的是 EDI 寄存器，那么 KVM 在虚拟机运行之前，已经将设备模拟的值填入到 VMCS Guest 区域内，此时虚拟机从 EDI 寄存器获得访问 MMIO 的值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="H1">虚拟机非首次访问 In-Broiler 设备的 MMIO</span>

![](/assets/PDB/HK/TH001921.png)

1. 虚拟机再次访问 MMIO 因页表属性异常导致 VM-EXIT (EXIT_REASON_EPT_MISCONFIG)
2. KVM 通过 MMIO 的 EPT 页表发现其已经建立 MMIO 页表那么直接 RET_PF_EMULATE
3. KVM 对访问 MMIO 的指令进行解码，以此获得指令使用的寄存器等信息
4. KVM 对访问 MMIO 的指令进行模拟，并对指令模拟标记为 complete_emulated_mmio
5. KVM 发现 MMIO 对应的设备来自 Broiler
6. KVM 将 MMIO 模拟的信息存储在 struct kvm_run, 并以 KVM_EXIT_MMIO 退出到 Broiler
7. Broiler 收到退出的原因是 KVM_EXIT_MMIO, 然后找到对应设备进行读写模拟
8. Broiler 模拟完毕之后调用 KVM_RUN 再次运行虚拟机
9. KVM 收到 KVM_RUN IOCTL 之后，并检查到 complete_emulated_mmio
10. KVM 再次模拟 MMIO 访问的指令，将模拟的值存储到指定寄存器
11. KVM 在将入虚拟机之前将 Dirty 寄存器更新到 VMCS 的 Guest 区域
12. 虚拟机恢复运行之后，从访问 MMIO 的寄存器获得所需的值

![](/assets/PDB/HK/TH001922.png)

当虚拟机再次访问 MMIO 时，由于 EPT 页表已经建立，那么硬件自动查询 EPT 页表，但硬件发现最后一级 EPT 页表的属性有问题，其配置为"只写"和"可执行"，那么硬件认为这是一种差异就导致虚拟机 VM-EXIT。退出的原因定义为 EXIT_REASON_EPT_MISCONFIG，KVM 调用 handle_ept_misconfig() 函数进行处理. 函数首先调用 vmcs_read64() 函数获得 VM-EXIT 退出时的物理地址，由于 MMIO 非第一次访问且 EPT 页表已经建立，那么函数直接调用 kvm_mmu_do_page_fault() 函数查询 MMIO EPT 页表.
 
![](/assets/PDB/HK/TH001923.png)

kvm_mmu_page_fault() 函数在处理 MMIO 的 EPT 页表时，与 52 行条件匹配，直接调用 handle_mmio_page_fault() 函数处理 MMIO EPT 页表，KVM 发现 MMIO 地址之前已经处理过，那么直接将函数的返回值设置为 RET_PF_EMULATE，并跳转到 emulate 处直接对指令进行模拟，不再需要修改或处理 MMIO EPT 页表. 接下来 KVM 或者 Broiler 处理的流程与首次访问 MMIO 的场景一致，具体流程可以参考:

![](/assets/PDB/HK/TH001911.png)

在讲解 KVM 指令模拟之前，需要对 X86 指令结构有一定的研究，细节可以参考 SDM 手册卷二. X86 指令的一部分是指令前缀(Instruction Prefixes), 例如 REP 和 Lock 前缀。第二部分是操作码(Opcode), 其是一个指令索引并指定指令的特定变体; 第三部分 ModR/M 字段指定寻址和操作数，符号 "R/M(Register/Memory)" 代表的是寄存器或内存; 第四部分 SIB 字段称为伸缩索引字段(Scale index byteundefined SIB), 用于计算数组索引偏移量; 第五部分表示地址位移字段，其保存了操作数的偏移量; 第六部分表示立即数字段，用于保存常量操作数. 具体细节参考下文:

![](/assets/PDB/HK/TH001920.png)

> [X86 指令编码简述](https://blog.csdn.net/xiao__1bai/article/details/126584837)
>
> [X86 汇编助记符指令机器码对应表](https://blog.csdn.net/weixin_43708844/article/details/103211703)

KVM 调用 x86_emulate_instruction() 函数对访问 MMIO 的指令进行模拟，首先是 x86_decode_insn() 函数将指令进行解码，然后 x86_emulate_insn() 函数进行实际的模拟，KVM 从指令解码中获得此次是 MOV 指令，并且是对一个地址的访问，并将访问的结果存储在 EDI 寄存中，那么其调用 segmented_read() 函数模拟 MOV 指令

![](/assets/PDB/HK/TH001912.png)

emulator_read_write() 函数是对 MMIO 读写进行模拟，函数调用 emulator_read_write_onepage() 判断 MMIO 是否属于 In-Kernel 设备，如果属于则找到对应的 In-Kernel 设备进行 MMIO 读写模拟，如果不是则执行 304 行的分支，此时 KVM 将 MMIO 需要模拟的信息存储在 struct kvm_run 数据结构中，并将退出到 Broiler 的原因标记为 KVM_EXIT_MMIO, 这里还有一个细节，KVM 将 vcpu 的 mmio_enabled 设置为 1, 函数执行完毕之后就进行返回.

![](/assets/PDB/HK/TH001913.png)

KVM 返回到 x86_emulate_instruction() 函数中执行时，如果检查到虚拟机再次恢复运行时需要处理 MMIO，那么函数执行 429 行分支，将 vcpu->arch.complete_userspace_io 设置为 complete_emulated_mmio。最后函数基于 KVM_RUN IOCTL 返回到 Broiler.

![](/assets/PDB/HK/TH001914.png)

Broiler 从 KVM_RUN 返回中得知是因为 KVM_EXIT_MMIO 退出的，那么 Broiler 调用 broiler_cpu_emulate_mmio() 函数找到 MMIO 对应的设备，然后进行读写模拟。当设备读写模拟完成之后，Broiler 再次调用 37 行 broiler_cpu_run() 函数告诉 KVM 恢复虚拟机的运行.

![](/assets/PDB/HK/TH001915.png)

KVM 接收到 KVM_RUN IOCTL 之后调用 kvm_arch_vcpu_ioctl_run() 函数恢复虚拟机的运行，在该函数中存在上图的判断，如果 vcpu->arch.complete_userspace_io 不空，那么说明虚拟机恢复之后要处理 PIO 或者 MMIO，通过之前的分析此时 vcpu->arch.complete_userspace_io 指向 complete_emulated_mmio() 函数，那么函数进入 20 行分支执行， 函数此时调用 complete_e-mulated_mmio() 函数.

![](/assets/PDB/HK/TH001916.png)

complete_emulated_mmio() 函数的主要作用是在虚拟机恢复之前，再次解析虚拟机 VM-EXIT 时的指令，然后将 MMIO 模拟的值存储到指定寄存器中，最后 KVM 更新到虚拟机 VMCS Guest 域指定寄存器中，待虚拟机恢复运行之后，可以从正在执行指令对应的寄存器中获得所需的值。188 行将设备模拟的 MMIO 值存储到 frag->data 中，然后函数进入 202 行分支，并继续调用 complete_emulate_io() 函数对指令进行模拟.

![](/assets/PDB/HK/TH001917.png)

complete_emulated_io() 函数再次调用 kvm_emulate_instruction() 函数对指令进行模拟，但是此时通过 EMULTYPE_NO_DECODE 指明不需要解析指令，直接进行指令模拟。与之前的指令模拟有所不同的是，KVM 已经知道模拟的指令以及使用的寄存器了，现在只需将模拟的值写入到指定寄存器即可

![](/assets/PDB/HK/TH001918.png)

KVM 找到需要更新的寄存器，调用 kvm_register_write() 函数对需要更新的寄存器写入新值，函数通过将 MMIO 模拟的值写入到 vcpu->arch.regs[] 指定的寄存器中，然后调用 kvm_register_mark_dirty() 函数将寄存器标记为 Dirty，那么虚拟机在恢复运行时，KVM 就会更新 VMCS Guest 中指定的域。

![](/assets/PDB/HK/TH001919.png)

最后虚拟机恢复执行，虚拟机继续执行 VM-EXIT 的指令，例如本案例中，访问 MMIO 使用的是 EDI 寄存器，那么 KVM 在虚拟机运行之前，已经将设备模拟的值填入到 VMCS Guest 区域内，此时虚拟机从 EDI 寄存器获得访问 MMIO 的值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

##### <span id="H9">In-Kernel 设备 MMIO 虚拟化</span>

![](/assets/PDB/HK/TH001937.png)

在 MMIO 虚拟化中，MMIO 的虚拟也和普通物理内存虚拟化一样，系统会为 Guset OS 的 MMIO 建立一个 EPT 页表，用于将 GMMIO(Guest MMIO) 映射到 HMMIO(Host MMIO). 一旦 EPT 页表建立之后虚拟机再次访问 GPA 将不会导致虚拟机 VM-EXIT，但对于 GPA 为 MMIO 的场景，需要虚拟机每访问一次 MMIO 就 VM-EXIT，并对 MMIO 进行读写模拟。为了解决这个问题需要将 MMIO 的虚拟化分作两个阶段，第一阶段就是虚拟机第一次访问 MMIO-GPA，此时会导致 VM-EXIT 并建立 EPT 页表，但对于 MMIO 最后一级 EPT 页表要进行特殊处理，将最后一级页表配置为 "只写" 和 "可执行"，这是一个错误的页表属性组合(MisConfigure); 第二阶段是虚拟机非首次访问 MMIO，由于 MMIO-GPA 的 EPT 页表已经存在，但硬件会检查到其最后一级的 EPT 页表是错误配置，那么同样会导致虚拟机 VM-EXIT. 对于 In-Kernel 设备，KVM 在捕获到虚拟机因为访问 MMIO VM-EXIT 之后，会将 MMIO 模拟的任务交给内核空间 KVM 模拟的设备，待 MMIO 模拟完毕之后 KVM 再次恢复执行，具体细节如下:

> [虚拟机首次访问 In-Kernel 设备的 MMIO](#H5)
>
> [虚拟机非首次访问 In-Kernel 设备的 MMIO](#H6)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="H5">虚拟机首次访问 In-Kernel 设备的 MMIO</span>

![](/assets/PDB/HK/TH001938.png)

1. 虚拟机首次访问 MMIO 导致 VM-EXIT，退出原因是 EXIT_REASON_EPT_VIOLATION
2. KVM 为 MMIO 建立 EPT 页表，但其属性配置为专用属性(异常属性 Misconfig)
3. KVM 对访问 MMIO 的指令进行解码，以此获得指令使用的寄存器等信息
5. KVM 发现 MMIO 对应的设备来自 Broiler
5. KVM 在 KVM_MMIO_BUS 中找到对应的 In-Kernel 设备，并进行 MMIO 模拟
7. In-Kernel 模拟完之后调用 em_mov 模拟访问 MMIO 的指令
8. KVM 在将入虚拟机之前将 Dirty 寄存器更新到 VMCS 的 Guest 区域
9. 虚拟机恢复运行之后，从访问 MMIO 的寄存器获得所需的值

![](/assets/PDB/HK/TH001907.png)

当虚拟机第一次访问 MMIO 时，虚拟机会因为 EXIT_REASON_EPT_VIOLATION 退出，可以 VMCS Exit Qualification 中获得具体的 MMIO 信息，并从 VMCS 的 GUEST_PHYSICAL_A-DDRESS 域中获得 MMIO 的地址。KVM 首先为 MMIO 建立 EPT 页表，除最后一级 EPT 页表其余级页表都与普通内存 EPT 页表一致，因此 KVM 会进入 kvm_tdp_page_fault() 函数为 MMIO 建立非最后一级 EPT 页表.

![](/assets/PDB/HK/TH001908.png)

对于 MMIO EPT 的最后一级页表，因为其不是映射普通内存，而且需要每次访问 MMIO 时让 VM-EXIT，并进行 MMIO 读写模拟，因此 KVM 调用 set_spte() 函数为 MMIO 建立最后一级页表时，调用 set_mmio_spte() 函数将 MMIO 最后一级 EPT 页表标记为一种错误(MissConfig)的属性，这样虚拟机再次访问 MMIO 时因为 EPT 页表属性错误会导致 VM-EXIT.

![](/assets/PDB/HK/TH001909.png)

KVM 通过 ept_set_mmio_spte_mask() 函数生成 MMIO EPT 最后一级页表的内容，其通过将页表设置为"只写"和"可执行"，正常页表逻辑中"可执行"必须与"可读"配合使用, 此时 MMIO 最后一级 EPT 页表这么配置就是一种错误的配置，虚拟机再次访问 MMIO 会因为页表配置错误导致 VM-EXIT.

![](/assets/PDB/HK/TH001910.png)

当 KVM 为 MMIO 建立好 EPT 页表之后，KVM 对访问 MMIO 的指令进行模拟，那么为什么要对指令进行模拟呢? 首先从虚拟机内部访问 MMIO 的代码说起, 一般在内核中使用 ioremap() C 函数分配一段虚拟地址映射到 MMIO(建立页表), 然后通过访问虚拟地址来访问 MMIO. 将 C 代码通过反汇编转换为汇编可以看到其对应的机器码和汇编代码，可以看到 CPU 可以像访问普通内存一样使用 MOV 指令访问 MMIO. 但有点不同的是 CPU 在使用 MOV 访问 MMIO 是可以使用不同类型的寄存器存储 MMIO 的值，或者有多种方式寻址 MMIO 地址, 由于这个原因的存在 KVM 需要知道访问 MMIO 使用了那些寄存器，以便 MMIO 模拟完毕之后将结果存储到指定寄存器，当虚拟机再次恢复运行之后就可以从指定寄存器获得访问 MMIO 的结果, 因此 KVM 需要对访问 MMIO 的指令进行解析和模拟.

![](/assets/PDB/HK/TH001911.png)

在讲解 KVM 指令模拟之前，需要对 X86 指令结构有一定的研究，细节可以参考 SDM 手册卷二. X86 指令的一部分是指令前缀(Instruction Prefixes), 例如 REP 和 Lock 前缀。第二部分是操作码(Opcode), 其是一个指令索引并指定指令的特定变体; 第三部分 ModR/M 字段指定寻址和操作数，符号 "R/M(Register/Memory)" 代表的是寄存器或内存; 第四部分 SIB 字段称为伸缩索引字段(Scale index byteundefined SIB), 用于计算数组索引偏移量; 第五部分表示地址位移字段，其保存了操作数的偏移量; 第六部分表示立即数字段，用于保存常量操作数. 具体细节参考下文:

![](/assets/PDB/HK/TH001920.png)

> [X86 指令编码简述](https://blog.csdn.net/xiao__1bai/article/details/126584837)
>
> [X86 汇编助记符指令机器码对应表](https://blog.csdn.net/weixin_43708844/article/details/103211703)

KVM 调用 x86_emulate_instruction() 函数对访问 MMIO 的指令进行模拟，首先是 x86_decode_insn() 函数将指令进行解码，然后 x86_emulate_insn() 函数进行实际的模拟，KVM 从指令解码中获得此次是 MOV 指令，并且是对一个地址的访问，并将访问的结果存储在 EDI 寄存中，那么其调用 segmented_read() 函数模拟 MOV 指令

![](/assets/PDB/HK/TH001912.png)

emulator_read_write() 函数是对 MMIO 读写进行模拟，函数调用 emulator_read_write_onepage() 检查到 MMIO 属于 In-Kernel 设备，则调用 emulator_read_write_onepage() 函数直接在内核空间进行 MMIO 模拟，模拟完毕之后函数直接从 297 行返回.

![](/assets/PDB/HK/TH001939.png)

vcpu_mmio_read() 函数通过调用 kvm_io_bus_read() 函数在 KVM 维护的 KVM_MMIO_BUS 中找到 MMIO 地址对应的 In-Kernel 设备，当找到之后就进行 MMIO 读写模拟。

![](/assets/PDB/HK/TH001940.png)

KVM 返回到 x86_emulate_instruction() 函数中执行，如果检查到在 ctxt->execute 非空，那么 KVM 通过 ctxt->execute(ctxt) 调用 em_mov() 函数进行 MOV 指令的模拟，模拟完毕之后跳转到 writeback 出继续执行.

![](/assets/PDB/HK/TH001918.png)

KVM 找到需要更新的寄存器，调用 kvm_register_write() 函数对需要更新的寄存器写入新值，函数通过将 MMIO 模拟的值写入到 vcpu->arch.regs[] 指定的寄存器中，然后调用 kvm_register_mark_dirty() 函数将寄存器标记为 Dirty，那么虚拟机在恢复运行时，KVM 就会更新 VMCS Guest 中指定的域。

![](/assets/PDB/HK/TH001919.png)

最后虚拟机恢复执行，虚拟机继续执行 VM-EXIT 的指令，例如本案例中，访问 MMIO 使用的是 EDI 寄存器，那么 KVM 在虚拟机运行之前，已经将设备模拟的值填入到 VMCS Guest 区域内，此时虚拟机从 EDI 寄存器获得访问 MMIO 的值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="H6">虚拟机非首次访问 In-Kernel 设备的 MMIO</span>

![](/assets/PDB/HK/TH001941.png)

1. 虚拟机再次访问 MMIO 因页表属性异常导致 VM-EXIT (EXIT_REASON_EPT_MISCONFIG)
2. KVM 通过 MMIO 的 EPT 页表发现其已经建立 MMIO 页表那么直接 RET_PF_EMULATE
3. KVM 对访问 MMIO 的指令进行解码，以此获得指令使用的寄存器等信息
5. KVM 发现 MMIO 对应的设备来自 Broiler
5. KVM 在 KVM_MMIO_BUS 中找到对应的 In-Kernel 设备，并进行 MMIO 模拟
7. In-Kernel 模拟完之后调用 em_mov 模拟访问 MMIO 的指令
8. KVM 在将入虚拟机之前将 Dirty 寄存器更新到 VMCS 的 Guest 区域
9. 虚拟机恢复运行之后，从访问 MMIO 的寄存器获得所需的值

![](/assets/PDB/HK/TH001922.png)

当虚拟机再次访问 MMIO 时，由于 EPT 页表已经建立，那么硬件自动查询 EPT 页表，但硬件发现最后一级 EPT 页表的属性有问题，其配置为"只写"和"可执行"，那么硬件认为这是一种差异就导致虚拟机 VM-EXIT。退出的原因定义为 EXIT_REASON_EPT_MISCONFIG，KVM 调用 handle_ept_misconfig() 函数进行处理. 函数首先调用 vmcs_read64() 函数获得 VM-EXIT 退出时的物理地址，由于 MMIO 非第一次访问且 EPT 页表已经建立，那么函数直接调用 kvm_mmu_do_page_fault() 函数查询 MMIO EPT 页表.
 
![](/assets/PDB/HK/TH001923.png)

kvm_mmu_page_fault() 函数在处理 MMIO 的 EPT 页表时，与 52 行条件匹配，直接调用 handle_mmio_page_fault() 函数处理 MMIO EPT 页表，KVM 发现 MMIO 地址之前已经处理过，那么直接将函数的返回值设置为 RET_PF_EMULATE，并跳转到 emulate 处直接对指令进行模拟，不再需要修改或处理 MMIO EPT 页表. 接下来 KVM 或者 Broiler 处理的流程与首次访问 MMIO 的场景一致，具体流程可以参考:

![](/assets/PDB/HK/TH001911.png)

在讲解 KVM 指令模拟之前，需要对 X86 指令结构有一定的研究，细节可以参考 SDM 手册卷二. X86 指令的一部分是指令前缀(Instruction Prefixes), 例如 REP 和 Lock 前缀。第二部分是操作码(Opcode), 其是一个指令索引并指定指令的特定变体; 第三部分 ModR/M 字段指定寻址和操作数，符号 "R/M(Register/Memory)" 代表的是寄存器或内存; 第四部分 SIB 字段称为伸缩索引字段(Scale index byteundefined SIB), 用于计算数组索引偏移量; 第五部分表示地址位移字段，其保存了操作数的偏移量; 第六部分表示立即数字段，用于保存常量操作数. 具体细节参考下文:

![](/assets/PDB/HK/TH001920.png)

> [X86 指令编码简述](https://blog.csdn.net/xiao__1bai/article/details/126584837)
>
> [X86 汇编助记符指令机器码对应表](https://blog.csdn.net/weixin_43708844/article/details/103211703)

KVM 调用 x86_emulate_instruction() 函数对访问 MMIO 的指令进行模拟，首先是 x86_decode_insn() 函数将指令进行解码，然后 x86_emulate_insn() 函数进行实际的模拟，KVM 从指令解码中获得此次是 MOV 指令，并且是对一个地址的访问，并将访问的结果存储在 EDI 寄存中，那么其调用 segmented_read() 函数模拟 MOV 指令

![](/assets/PDB/HK/TH001912.png)

emulator_read_write() 函数是对 MMIO 读写进行模拟，函数调用 emulator_read_write_onepage() 检查到 MMIO 属于 In-Kernel 设备，则调用 emulator_read_write_onepage() 函数直接在内核空间进行 MMIO 模拟，模拟完毕之后函数直接从 297 行返回.

![](/assets/PDB/HK/TH001939.png)

vcpu_mmio_read() 函数通过调用 kvm_io_bus_read() 函数在 KVM 维护的 KVM_MMIO_BUS 中找到 MMIO 地址对应的 In-Kernel 设备，当找到之后就进行 MMIO 读写模拟。

![](/assets/PDB/HK/TH001940.png)

KVM 返回到 x86_emulate_instruction() 函数中执行，如果检查到在 ctxt->execute 非空，那么 KVM 通过 ctxt->execute(ctxt) 调用 em_mov() 函数进行 MOV 指令的模拟，模拟完毕之后跳转到 writeback 出继续执行.

![](/assets/PDB/HK/TH001918.png)

KVM 找到需要更新的寄存器，调用 kvm_register_write() 函数对需要更新的寄存器写入新值，函数通过将 MMIO 模拟的值写入到 vcpu->arch.regs[] 指定的寄存器中，然后调用 kvm_register_mark_dirty() 函数将寄存器标记为 Dirty，那么虚拟机在恢复运行时，KVM 就会更新 VMCS Guest 中指定的域。

![](/assets/PDB/HK/TH001919.png)

最后虚拟机恢复执行，虚拟机继续执行 VM-EXIT 的指令，例如本案例中，访问 MMIO 使用的是 EDI 寄存器，那么 KVM 在虚拟机运行之前，已经将设备模拟的值填入到 VMCS Guest 区域内，此时虚拟机从 EDI 寄存器获得访问 MMIO 的值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### 同步 PIO 端口虚拟化

![](/assets/PDB/HK/TH001715.png)

在外设内部存在一些列的寄存器，这些寄存器有的用来控制外设的行为、有的用来存储外设产生的数据。CPU 通过与外设的寄存器交互实现对外设的控制。在 X86 架构中存在 IO Space，IO Space 可以通过 IO Port 方式寻址，外设可以将其寄存器映射到 IO Space，然后 CPU 可以直接通过 IO Port 访问外设。**PIO(Programm-ing Input/Output Model)**, PIO 模式是一种通过 CPU 执行 I/O 端口指令来进行数据读写的交换模式, 在虚拟化中将 PIO 泛指 IO 空间的 IO 端口.

![](/assets/PDB/HK/TH001871.png)

**同步 PIO(Synchronous IO)** 指的是 CPU 对 IO Port 发起读写操作之后，需要等待外设处理完毕并通知 CPU 之后，CPU 才可以继续执行, 一般情况下同步 PIO 包括读写操作，需要在指令周期内完成. 相对于同步 PIO，异步 PIO 在发起写操作之后，无需等待外设处理完毕就可以直接运行，外设处理完毕之后可以中断告诉 CPU，CPU 也可以通过轮询的方式查看异步 PIO 的完成情况，一般情况下，异步 PIO 操作为写操作.

![](/assets/PDB/HK/TH001872.png)

在 IO 端口虚拟化中，无论是同步 PIO 还是异步 PIO 的虚拟化，都不能独立存在，而是要基于模拟设备，模拟设备可以位于内核空间，那么称这类模拟设备为 In-Kernel 设备，而有的设备模拟位于用户空间的 Broiler，那么称这类设备为 In-Broiler 设备. 无论是哪类设备都可以对 PIO 进行模拟. Guest OS 内部通过 IO Port 访问同步 PIO，那么会导致 VM-EXIT。KVM 捕获到因为 PIO 导致的 VM-EXIT 之后，获得 Guest OS 访问 PIO 的信息。如果模拟设备位于 Kernel，那么直接让设备进行 PIO 模拟，模拟完毕之后再恢复虚拟机运行; 如果虚拟设备位于用户空间，那么 KVM 进行 KVM_RUN ioctl 返回之后，Broiler 捕获到虚拟机因为 PIO 退出，那么 Broiler 找到对应的设备进行 PIO 模拟，模拟完毕之后 Broiler 再次让虚拟机运行。通过以上的操作，虚拟机再次 VM-ENTRY 继续支持同步 PIO 指令.

![](/assets/PDB/HK/TH001873.png)

当 Guest OS 因为访问 PIO 导致 VM-EXIT 时，可以从 VMCS 的 Exit Qualification 寄存器中获得 Guest OS 访问 PIO 的行为。Exit Qualification 寄存器的布局如上图，其高 64 位预留而低 6-15 位也同样预留. 其布局含义如下:

* BIT0-2: Size of Access 用于描述 PIO 的长度
* BIT3: Direction 用于描述 PIO 操作是读还是写，置位则为读，清零则为写
* BIT4: String Instruction 指明 PIO 指令是否为串行指令，即一个 PIO 包括多个重复指令
* BIT16-31: Port Number 指明 IO Port 的端口号.

-------------------------------------

###### <span id="B5">In-Broiler 设备同步 PIO 读模拟流程</span>

![](/assets/PDB/HK/TH001925.png)

-------------------------------------

###### <span id="B6">In-Broiler 设备同步 PIO 写模拟流程</span>

![](/assets/PDB/HK/TH001962.png)

-------------------------------------

###### <span id="B7">In-Kernel 设备同步 PIO 读模拟流程</span>

![](/assets/PDB/HK/TH001932.png)

-------------------------------------

###### <span id="B8">In-Kernel 设备同步 PIO 写模拟流程</span>

![](/assets/PDB/HK/TH001963.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="B2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

##### In-Broiler 设备使用同步 PIO

![](/assets/PDB/HK/TH001883.png)

Broiler 模拟的设备可以将其寄存器映射到 X86 的 IO Space，然后通过 IO Port 访问外设的寄存器。Linux 提供了一系列的机制访问 IO Port，可以使用 IN/OUT 指令访问，也可以向访问存储域一样访问。本节通过 Broiler 模拟一个普通的外设，外设内部存在几个寄存器，这些寄存器映射到 X86 IO Space。当虚拟机因为访问这些 IO Port 导致 VM-EXIT 之后，Broiler 模拟的设备对 IO Port 的读写进行模拟(源码位置 footstuff/Broiler-Synchronous-pio-base.c).

![](/assets/PDB/HK/TH001878.png)

Broiler 模拟了一个设备，设备内包含四个寄存器，分别为 SLOT_NUM_REG、SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器，Broiler 在 71 行调用 broiler_register_pio() 函数将其映射到 X86 IO Space \[0x6060, 0x6070]，四个寄存器的读写模拟函数为 Broiler_pio_callback(). 当 Guest OS 内对这四个寄存器进行读写操作时会导致 VM-EXIT, KVM 捕获到之后发现 IO Port 对应的设备位于 Broiler，那么通过 vcpu_run 数据结构将需要模拟的 IO Port 传递给 Broiler 进行模拟。Broiler 捕获到虚拟机因为这些 IO Port VM-EXIT，那么 Broiler 找到对应的模拟设备，接着调用 IO Port 读写模拟函数 Broiler_pio_callback() 进行 IO Port 读写模拟。当读写模拟完毕之后，虚拟机再次恢复运行，并继续执行导致 VM-EXIT 的指令. 接下来就是 Guest OS 内部驱动访问这些 IO Port 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PIO: Programming Input/Output Model --->
          [*] Sychronous PIO Driver for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Synchronous-pio-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001879.png)
![](/assets/PDB/HK/TH001880.png)
![](/assets/PDB/HK/TH001881.png)

> [Broiler-Synchronous-pio-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/PIO/Broiler-Synchronous-pio)

Guest OS 内操作 IO Port 的驱动源码比较简单，首先通过 request_resource() 向系统 IO Space 树注册了设备的 IO Port，其 IO 端口为 \[0x6060, 0x6070], 驱动接着调用 ioport_map() 函数将 IO 端口映射到虚拟内存，接着在 47 行读取 SLOT_NUM_REG 寄存器的值，然后向 SLOT_S-EL_REG 寄存器写入 0x02, 最后将 SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器的值读取出来. 接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001882.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Synchronous-pio-default.ko, 可以看到驱动加载成功，首先读取 SLOT_NUM_REG 寄存器的值为 32，接着向 SLOT_SEL_REG 写入 2 之后读出的值也是 2，最后 Freq 的范围在 \[0x010, 0x40]. 至此 Broiler 设备同步 PIO 使用讲解完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="B3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

##### In-Kernel 设备使用同步 PIO

![](/assets/PDB/HK/TH001884.png)

In-Kernel 模拟的设备可以将其寄存器映射到 X86 的 IO Space，然后通过 IO Port 访问外设的寄存器。Linux 提供了一系列的机制访问 IO Port，可以使用 IN/OUT 指令访问，也可以向访问存储域一样访问。本节通过 In-Kernel 模拟一个普通的外设，外设内部存在几个寄存器，这些寄存器映射到 X86 IO Space。当虚拟机因为访问这些 IO Port 导致 VM-EXIT 之后，Broiler 模拟的设备对 IO Port 的读写进行模拟(源码位置 footstuff/Broiler-Synchronous-pio-In-Kernel.c).

![](/assets/PDB/HK/TH001885.png)

由于 In-Kernel 设备在 KVM 里实现，因此 Broiler 只提供了简单的代码用于检查 KVM 是否支持创建采用同步 IO 的 In-Kernel 设备，如果支持那么通过 IOCTL KVM_CREATE_SYN-C_PIO_DEV 创建一个 In-Kernel 设备。对于位于 KVM 的代码如下(footstuff/Broiler-Synchronous-pio-In-Kernel.patch):

![](/assets/PDB/HK/TH001886.png)

上图提供了一个 KVM 实现 In-Kernel 的 patch，由于 KVM 版本不同实际代码可能存在位置的差异，开发者可以参考该 patch 进行 In-Kernel 设备的创建。KVM 模拟了一个设备，设备内包含四个寄存器，分别为 SLOT_NUM_REG、SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器，KVM 在 107 行调用 kvm_iodevice_init() 函数创建一个 In-Kernel 设备，并在 109 行调用 kvm_io_bus_register_dev() 函数将设备的四个寄存器映射到 X86 IO Space \[0x6080, 0x6090]，四个寄存器的读模拟函数为 broiler_synchronous_pio_read(), 以及写模拟函数为 broiler_synchronous_pio_write(). 当 Guest OS 内对这四个寄存器进行读写操作时会导致 VM-EXIT, KVM 捕获到之后发现 IO Port 对应的设备位于 KVM 内，那么直接找到这个 In-Kernel 设备进行读写模拟。当 In-Kernel 设备读写模拟完毕之后，KVM 再次恢复虚拟机的执行，虚拟机恢复之后继续将访问 IO Port 的指令执行完毕. 接下来就是 Guest OS 内部驱动访问这些 IO Port 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻>辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PIO: Programming Input/Output Model --->
          [*] Sychronous PIO Driver In Kernel (Broiler)

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Synchronous-pio-In-Kernel-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001879.png)
![](/assets/PDB/HK/TH001887.png)
![](/assets/PDB/HK/TH001888.png)

> [Broiler-Synchronous-pio-In-Kernel-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/PIO/Broiler-Synchronous-pio-In-Kernel)

Guest OS 内操作 IO Port 的驱动源码比较简单，首先通过 request_resource() 向系统 IO Space 树注册了设备的 IO Port，其 IO 端口为 \[0x6080, 0x6090], 驱动接着调用 ioport_map() 函数将 IO 端口映射到虚拟内存，接着在 47 行读取 SLOT_NUM_REG 寄存器的值，然后向 SLOT_S-EL_REG 寄存器写入 0x02, 最后将 SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器的值读取出来. 接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001889.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Synchronous-pio-In-Kernel-default.ko, 可以看到驱动加载成功，首先读取 SLOT_NUM_REG 寄存器的值为 32，接着向 SLOT_SEL_REG 写入 2 之后读出的值也是 2，最后 Freq 的范围在 \[0x010, 0x40]. 至此 In-Kernel 设备同步 PIO 使用讲解完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="B4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

##### PCI 设备使用同步 PIO

![](/assets/PDB/HK/TH001890.png)

PCI 模拟的设备同样可以将其 BAR 空间映射到 X86 的 IO Space，然后通过 IO Port 访问 PCI 的 BAR 空间，这里暂称为 IO-BAR。Linux 提供了一系列的机制访问 IO Port，可以使用 IN/OUT 指令访问，也可以像访问存储域一样访问。本节通过 Broiler 模拟一个 PCI 设备，PCI 设备包含了一个 IO-BAR 空间，并将 IO-BAR 映射到 X86 IO Space。当虚拟机因为访问这些 IO Port 导致 VM-EXIT 之后，Broiler 模拟的 PCI 设备对 IO-BAR 的读写进行模拟(源码位置 footstuff/Broiler-Synchronous-pio-PCI.c).

![](/assets/PDB/HK/TH001891.png)

Broiler 模拟了 PCI 一个设备，PCI 设备内包一个 IO-BAR，IO-BAR 内部含四个寄存器，分别为 SLOT_NUM_REG、SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器，这里对 PCI 设备的模拟不做细节讲解，只对 PCI 如何使用同步 IO 进行讲解。Broiler 首先在 63 行调用 pci_alloc_io_port_block() 函数在 PCI 设备中创建一个 IO-BAR, 接着在 113 行指明 BAR 映射到 X86 的 IO Space，114 行指明 IO-BAR 的长度为 PCI_IO_SIZE. 最后 Broiler 在 119 行调用 pci_register_bar_regions() 注册了 IO-BAR 的 Enable/Disable 函数。在 PCI BAR Enable 接口 Broiler_pci_bar_active() 函数中，Broiler 在 81 行调用 broiler_register_pio() 函数最终将 IO-BAR 映射到 IO Space，并提供了读写模拟函数 Broiler_pci_bar_callback(). 在 Broiler_pci_bar_callback() 函数中实现了对 IO-BAR 里的四个寄存器的读写模拟. 接下来就是 Guest OS 内部驱动访问这些 IO Port 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PIO: Programming Input/Output Model --->
          [*] Sychronous PIO PCI Driver for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Synchronous-pio-PCI-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001879.png)
![](/assets/PDB/HK/TH001892.png)
![](/assets/PDB/HK/TH001893.png)

> [Broiler-Synchronous-pio-PCI-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/PIO/Broiler-Synchronous-pio-PCI)

Guest OS 内操作 IO Port 基于一个 PCI 驱动，抛开 PCI 设备驱动的代码，驱动首先在 60 行调用 pci_request_region() 函数将 IO-BAR 注册到内核的 IO Space 资源树，然后在 67 行 pci_iomap() 将 IO-BAR 对应的 IO Port 映射到虚拟内存，接下来就是 80-87 行使用 IO Port 操作函数对 IO-BAR 的寄存器进行读写操作. 接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001894.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Synchronous-pio-PCI-default.ko, 可以看到驱动加载成功，首先读取 SLOT_NUM_REG 寄存器的值为 0x20，接着向 SLOT_SEL_REG 写入 2 之后读出的值也是 2，最后 Freq 的范围在 \[0x010, 0x40]. 至此 Broiler PCI 设备使用同步 PIO 讲解完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

#### 异步 PIO 端口虚拟化

![](/assets/PDB/HK/TH001715.png)

在外设内部存在一些列的寄存器，这些寄存器有的用来控制外设的行为、有的用来存储外设产生的数据。CPU 通过与外设的寄存器交互实现对外设的控制。在 X86 架构中存在 IO Space，IO Space 可以通过 IO Port 方式寻址，外设可以将其寄存器映射到 IO Space，然后 CPU 可以直接通过 IO Port 访问外设。**PIO(Programm-ing Input/Output Model)**, PIO 模式是一种通过 CPU 执行 I/O 端口指令来进行数据读写的交换模式, 在虚拟化中将 PIO 泛指 IO 空间的 IO 端口.

![](/assets/PDB/HK/TH001871.png)

**异步 PIO(Asynchronous IO)** 指的是 CPU 对 IO Port 发起写操作之后，不需要等待外设处理完毕 CPU 直接运行下一条指令。相对与同步 PIO，异步 PIO 只包括写操作，同步 PIO 发起读写操作之后，CPU 需要外设完成读写请求并返回之后，CPU 才能继续执行下一条指令。异步 PIO 一般用于触发外设执行异步操作，待外设异步操作执行完毕之后，再通过中断通知 CPU 异步请求已经完毕，CPU 被中断之后对异步操作收尾。常见异步 PIO 比如 Kick 寄存器和 Doorball 寄存器等. 

![](/assets/PDB/HK/TH001964.png)

在 IO 端口虚拟化中，无论是同步 PIO 还是异步 PIO 的虚拟化，都不能独立存在，而是要基于模拟设备，模拟设备可以位于内核空间，那么称这类模拟设备为 In-Kernel 设备，而有的设备模拟位于用户空间的 Broiler，那么称这类设备为 In-Broiler 设备. 无论是哪类设备都可以对 PIO 进行模拟. Guest OS 内部通过 IO Port 访问异步 PIO，那么会导致 VM-EXIT。KVM 捕获到因为 PIO 导致的 VM-EXIT 之后，获得 Guest OS 访问 PIO 的信息。如果模拟设备位于 Kernel，那么直接通知设备进行 PIO 模拟，通知完毕之后直接恢复虚拟机运行，In-Kernel 设备完成异步 PIO 请求之后注入中断告诉虚拟机，虚拟机收到中断之后对异步 PIO 进行收尾; 如果虚拟设备位于用户空间，那么 KVM 直接通过 eventfd 通知 Broiler，通知完毕之后虚拟机恢复运行，Broiler 在处理完异步 PIO 请求之后，同样注入一个中断告诉虚拟机异步 PIO 已经处理完毕了，虚拟机收到中断之后对异步 PIO 进行收尾.

![](/assets/PDB/HK/TH001873.png)

当 Guest OS 因为访问 PIO 导致 VM-EXIT 时，可以从 VMCS 的 Exit Qualification 寄存器中获得 Guest OS 访问 PIO 的行为。Exit Qualification 寄存器的布局如上图，其高 64 位预留而低 6-15 位也同样预留. 其布局含义如下:

* BIT0-2: Size of Access 用于描述 PIO 的长度
* BIT3: Direction 用于描述 PIO 操作是读还是写，置位则为读，清零则为写
* BIT4: String Instruction 指明 PIO 指令是否为串行指令，即一个 PIO 包括多个重复指令
* BIT16-31: Port Number 指明 IO Port 的端口号.

-------------------------------------

###### <span id="C1">Broiler 设备创建异步 PIO</span>

![](/assets/PDB/HK/TH001895.png)

与同步 PIO 相比，异步 IO 不仅要在 Broiler 设备中申请一段 PIO 进行写模拟，而且还需要在 KVM 中注册一个异步写通知机制，大体流程如上: Broiler 设备申请了一段 PIO 之后，然后为这段 PIO 申请一个 eventfd 的管道，接着调用 ioctl KVM_IOEVENTFD 将 eventfd 告诉 KVM。KVM 在收到 ioctl KVM_IOEVENTFD 之后调用 kvm_ioeventfd() 函数进行异步通知链，调用的核心位于 kvm_assign_ioeventfd_idx() 函数，其首先调用 eventfd_ctx_fdget() 函数与用户空间的 eventfd 对接作为通知方，接着调用 kvm_iodevice_init() 函数为通知链提供了通知方法 ioeventfd_ops，即有异步通知的时候就会调用 ioeventfd_ops 提供的方法进行通知。函数接着调用 kvm_io_bus_register_dev() 函数在 KVM 维护的 IO BUS 上注册异步 PIO 对应的区域，只要虚拟机访问了这段 PIO，那么 KVM 会直接调用 ioeventfd_ops 提供的方法进行通知，以上便是异步 PIO 注册的流程.

![](/assets/PDB/HK/TH001896.png)

在 KVM 中，每个虚拟机对应的 struct kvm 会维护 KVM_NR_BUSES 条虚拟总线，分别是 KVM_MMIO_BUS、KVM_PIO_BUS、KVM_VIRTIO_CCW_NOTIFY_BUS 和 KVM_FAST_MMIO_BUS，KVM 使用 struct kvm_io_range 描述虚拟总线上的一段 range，每条虚拟总线上维护了多个 range，通过 struct kvm_io_range 可以知道这段 range 是哪个设备的，但虚拟机访问的地址落在了 KVM 虚拟总线上某段 range 里，那么 KVM 会找到这段 range 对应的设备，然后调用设备提供的异步通知方法，并进行异步通知.

------------------------------

###### <span id="C2">In-Broiler 设备异步 PIO 写流程</span>

![](/assets/PDB/HK/TH001897.png)

对于异步 PIO 操作只存在写的场景，Broiler 虚拟设备的异步写流程如上图，当虚拟机对异步 IO 进行写操作时，虚拟机会触发 VM-EXIT，KVM 捕获到 VM 退出的原因是 EXIT_REASON_I-O_INSTRUCTION，于是调用 handle_io() 函数处理 IO 模拟。在流程的 kernel_pio() 函数中，发现 PIO 是 KVM 维护的 PIO 虚拟总线上的一端区域，于是通过 kvm_io_bus_write() 函数找到对应的设备进行写操作，这时找到 Broiler 虚拟设备初始化注册的异步接口，于是函数调用 kvm_iodevice_write() 函数通过 eventfd 通知用户空间，当 KVM 通知完毕之后就直接恢复了虚拟机的执行，以此同时 Broiler 的设备通过 eventfd 管道收到了通知，那么虚拟设备就进行异步的写模拟，这里很好的体现了异步的概念，虚拟机继续运行且模拟设备也在模拟 PIO 写。当设备模拟完 PIO 写之后，Broiler 可以通过 broiler_irq_line() 函数向虚拟机注入一个中断，以此告诉虚拟机异步写 PIO 已经完成。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="C3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000N.jpg)

##### In-Broiler 设备使用异步 PIO

![](/assets/PDB/HK/TH001898.png)

Broiler 模拟的设备可以将其寄存器映射到 X86 的 IO Space，然后通过 IO Port 访问外设的寄存器。Linux 提供了一系列的机制访问 IO Port，可以使用 IN/OUT 指令访问，也可以向访问存储域一样访问。本节通过 Broiler 模拟一个普通的外设，外设内部存在几个寄存器，这些寄存器映射到 X86 IO Space。当虚拟机因为访问异步 PIO 会导致 VM-EXIT 之后，Broiler 模拟的设备对 IO Port 的写进行模拟之后。注入一个中断告诉设备异步 PIO 写已经完成(源码位置 footstuff/Broiler-Asynchronous-pio-base.c).

![](/assets/PDB/HK/TH001899.png)

Broiler 模拟的设备分作三部分，第一部分是 PIO 写模拟部分，第二个部分是异步 IO 注册部分，第三部分是中断注入部分。设备在 75 行调用 broiler_register_pio() 函数为设备注册 BROILER_PIO_PORT 出的 PIO，并提供 PIO 的模拟函数 Broiler_pio_callback, 函数 Broiler_pio_callback() 的用途是模拟 IRQ_NUM_REG 寄存器的读操作，IRQ_NUM_REG 寄存器里存储设备的中断号; 设备在 81 行调用 eventfd() 函数添加一个通知管道，接着在 87 行创建一个线程 irq_thread(), 在线程 irq_thread() 设备只要被 eventfd 管道通知，那么唤醒异步 PIO 模拟，模拟完毕之后向虚拟机注入一个中断，设备在 93-98 行通过 struct kvm_ioeventfd 数据结构构建了异步通道的信息，然后在 100 行通过 ioctl 向 KVM 传入 KVM_IOEVENTFD, 以此告诉 KVM 为 PIO 注册一个异步通知接口; 设备在 71 行调用 irq_alloc_from_irqchip() 函数为设备申请了一个中断，中断号存储在 irq 变量中，另外对 IRQ_NUM_REG 寄存器读模拟时读取的中断号来自 irq 变量，当异步 PIO 处理完毕之后，设备可以在 46 行调用 broiler_irq_line() 函数进行中断注入，以此告诉虚拟机异步 PIO 处理完毕. 接下来就是 Guest OS 内部驱动访问这些 IO Port 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PIO: Programming Input/Output Model --->
          [*] Asychronous PIO Driver for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Synchronous-pio-PCI-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001879.png)
![](/assets/PDB/HK/TH001900.png)
![](/assets/PDB/HK/TH001901.png)

> [Broiler-Asynchronous-pio-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/PIO/Broiler-Asynchronous-pio)

Guest OS 内操作异步 IO Port 的驱动源码比较简单，首先通过 request_resource() 向系统 IO Space 树注册了设备的 IO Port，其 IO 端口为 \[0x60A0, 0x60B0], 驱动接着调用 ioport_map() 函数将 IO 端口映射到虚拟内存，接着在驱动的 53 行从 IRQ_NUM_REG 寄存器中获得中断号，并调用 request_irq() 函数为中断注册中断处理函数 Broiler_irq_handler(). 驱动最后在 65 行向寄存器 DOORBALL_REG 写，这就是对异步 PIO 写操作. 接下来在 BiscuitOS 实践该案例: 

![](/assets/PDB/HK/TH001902.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Asynchronous-pio-default.ko, 可以看到驱动加载成功，驱动使用的中断 3 并注册成功，待 3s 之后驱动收到了设备发来的驱动，因此异步 PIO 的验证到此完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="C4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

##### PCI 设备使用异步 PIO

![](/assets/PDB/HK/TH001978.png)

Broiler 模拟的 PCI 设备可以将其寄存器映射到 X86 的 IO Space，然后通过 IO Port 访问外设的寄存器。Linux 提供了一系列的机制访问 IO Port，可以使用 IN/OUT 指令访问，也可以向访问存储域一样访问。本节通过 Broiler 模拟一个 PCI 设备，PCI 设备内部存在几个寄存器，这些寄存器映射到 X86 IO Space。当虚拟机因为访问异步 PIO 会导致 VM-EXIT 之后，Broiler 模拟的 PCI 设备对 IO Port 的写进行模拟之后。注入一个中断告诉设备异步 PIO 写已经完成(PCI 设备源码位置 footstuff/Broiler-Asynchronous-pio-PCI.c).

![](/assets/PDB/HK/TH001899.png)

Broiler 模拟的设备分作三部分，第一部分是 PIO 写模拟部分，第二个部分是异步 IO 注册部分，第三部分是中断注入部分。设备在 75 行调用 broiler_register_pio() 函数为设备注册 BROILER_PIO_PORT 出的 PIO，并提供 PIO 的模拟函数 Broiler_pio_callback, 函数 Broiler_pio_callback() 的用途是模拟 IRQ_NUM_REG 寄存器的读操作，IRQ_NUM_REG 寄存器里存储设备的中断号; 设备在 81 行调用 eventfd() 函数添加一个通知管道，接着在 87 行创建一个线程 irq_thread(), 在线程 irq_thread() 设备只要被 eventfd 管道通知，那么唤醒异步 PIO 模拟，模拟完毕之后向虚拟机注入一个中断，设备在 93-98 行通过 struct kvm_ioeventfd 数据结构构建了异步通道的信息，然后在 100 行通过 ioctl 向 KVM 传入 KVM_IOEVENTFD, 以此告诉 KVM 为 PIO 注册一个异步通知接口; 设备在 71 行调用 irq_alloc_from_irqchip() 函数为设备申请了一个中断，中断号存储在 PCI 设备的 PCI Configuration Space 中，当异步 PIO 处理完毕之后，设备可以在 46 行调用 broiler_irq_line() 函数进行中断注入，以此告诉虚拟机异步 PIO 处理完毕. 接下来就是 Guest OS 内部驱动访问这些 IO Port 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] PIO: Programming Input/Output Model --->
          [*] Asychronous PIO PCI Driver for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Asynchronous-pio-PCI-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001879.png)
![](/assets/PDB/HK/TH001979.png)
![](/assets/PDB/HK/TH001980.png)

> [Broiler-Asynchronous-pio-PCI-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/PIO/Broiler-Asynchronous-pio-PCI)

Guest OS 内操作异步 IO Port 的 PCI 驱动源码比较简单，抛开 PCI 设备驱动本身，首先通过 request_resource() 向系统 IO Space 树注册了设备的 IO Port, 驱动接着调用 pci_iomap() 函数将 IO 端口映射到虚拟内存，接着在驱动的 77 行通过 pci_intx() 函数获得中断号，并调用 request_irq() 函数为中断注册中断处理函数 Broiler_irq_handler(). 驱动最后在 91 行向寄存器 DOORBALL_REG 写，这就是对异步 PIO 写操作. 接下来在 BiscuitOS 实践该案例: 

![](/assets/PDB/HK/TH001981.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Asynchronous-pio-PCI-default.ko, 可以看到驱动加载成功，驱动使用的中断 8 并注册成功，待 3s 之后驱动收到了设备发来的驱动，因此异步 PIO 的验证到此完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000D.jpg)

#### 同步 MMIO 虚拟化

![](/assets/PDB/HK/TH001903.png)

在外设内部存在一些列的寄存器，这些寄存器有的用来控制外设的行为、有的用来存储外设产生的数据。CPU 通过与外设的寄存器交互实现对外设的控制和数据传输。在 X86 架构中存在 IO Space，IO Space 可以通过 IO Port 方式寻址，外设可以将其寄存器映射到 IO Space，然后 CPU 可以直接通过 IO Port 访问外设; 另外外设也可以将寄存器映射存储域，并像普通内存一样访问这些寄存器，那么称这种外设寄存器的映射方式为 **MMIO(Memory mapped IO)**. 常见的 MMIO 包括 PCI 设备的 BAR 空间和 LAPIC 内部寄存器等。

![](/assets/PDB/HK/TH001904.png)

**同步 MMIO(Synchronous MMIO)** 指的是 CPU 对 MMIO 发起读写操作之后，需要等待外设处理完毕并通知 CPU 之后，CPU 才可以继续执行, 一般情况下同步 MMIO 包括读写操作，需要在指令周期内完成. 相对于同步 MMIO，异步 MMIO 在发起写操作之后，无需等待外设处理完毕就可以直接运行，外设处理完毕之后可以中断告诉 CPU，CPU 也可以通过轮询的方式查看异步 MMIO 的完成情况，一般情况下，异步 MMIO 操作为写操作.

![](/assets/PDB/HK/TH001872.png)

在 MMIO 虚拟化中，MMIO 的虚拟也和普通物理内存虚拟化一样，系统会为 Guset OS 的 MMIO 建立一个 EPT 页表，用于将 GMMIO(Guest MMIO) 映射到 HMMIO(Host MMIO), 但由于 HMMIO 的实现不同，可能 HMMIO 也需要 KVM、QEMU 或者 Broiler 进行模拟. 一旦 EPT 页表建立之后虚拟机再次访问 GPA 将不会导致虚拟机 VM-EXIT，但对于 GPA 为 MMIO 的场景，需要虚拟机每访问一次 MMIO 就 VM-EXIT，并对 MMIO 进行读写模拟。为了解决这个问题需要将 MMIO 的虚拟化分作两个阶段，第一阶段就是虚拟机第一次访问 MMIO-GPA，此时会导致 VM-EXIT 并建立 EPT 页表，但对于 MMIO 最后一级 EPT 页表要进行特殊处理，将最后一级页表配置为 “只写” 和 “可执行”，这是一个错误的页表属性组合(MisConfigure); 第二阶段是虚拟机非首次访问 MMIO，由于 MMIO-GPA 的 EPT 页表已经存在，但硬件会检查到其最后一级的 EPT 页表是错误配置，那么同样会导致虚拟机 VM-EXIT.

-----------------------------------------

###### <span id="D1">In-Broiler 设备同步 MMIO 读模拟流程</span>

![](/assets/PDB/HK/TH001906.png)
![](/assets/PDB/HK/TH001921.png)

第一张图为虚拟机对同步 In-Broiler 设备 MMIO 第一次读操作，第二张图为虚拟机对同步 In-Broiler 设备 MMIO 非首次读操作.

-----------------------------------------

###### <span id="D2">In-Broiler 设备同步 MMIO 写模拟流程</span>

![](/assets/PDB/HK/TH001942.png)
![](/assets/PDB/HK/TH001943.png)

第一张图为虚拟机对同步 In-Broiler 设备 MMIO 第一次写操作，第二张图为虚拟机对同步 In-Broiler 设备 MMIO 非首次写操作.

-----------------------------------------

###### <span id="D3">In-Kernel 设备同步 MMIO 读模拟流程</span>

![](/assets/PDB/HK/TH001938.png)
![](/assets/PDB/HK/TH001941.png)

第一张图为虚拟机对同步 In-Kernel 设备 MMIO 第一次读操作，第二张图为虚拟机对同步 In-Kernel 设备 MMIO 非首次读操作.

-----------------------------------------

###### <span id="D4">In-Kernel 设备同步 MMIO 写模拟流程</span>

![](/assets/PDB/HK/TH001946.png)
![](/assets/PDB/HK/TH001945.png)

第一张图为虚拟机对同步 In-Kernel 设备 MMIO 第一次写操作，第二张图为虚拟机对同步 In-Kernel 设备 MMIO 非首次写操作.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="D5"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

##### In-Broiler 设备使用同步 MMIO

![](/assets/PDB/HK/TH001936.png)

Broiler 模拟的设备可以将其寄存器映射到 X86 的存储域，然后像普通内存一样使用 MOV 指令访问外设的寄存器。Linux 提供了一系列的机制访问 MMIO，可以通过建立虚拟地址到 MMIO 的页表访问。本节通过 Broiler 模拟一个普通 In-Broiler 外设，外设内部存在几个寄存器，这些寄存器映射到存储域作为 MMIO。当虚拟机因为访问这些 MMIO 导致 VM-EXIT 之后，Broiler 模拟的设备对 MMIO 的读写进行模拟(In-Broiler 设备源码位置 footstuff/Broiler-Synchronous-mmio-base.c).

![](/assets/PDB/HK/TH001947.png)

Broiler 模拟了一个设备，设备内包含四个寄存器，分别为 SLOT_NUM_REG、SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器，Broiler 在 71 行调用 broiler_ioport_register() 函数将其映射到 存储域 \[0xD0000000, 0xD0000010]，四个寄存器的读写模拟函数为 Broiler_mmio_callback(). 当 Guest OS 内对这四个寄存器进行读写操作时会导致 VM-EXIT, KVM 捕获到之后发现 MMIO 对应的设备位于 Broiler，那么通过 vcpu_run 数据结构将需要模拟的 MMIO 传递给 Broiler 进行模拟。Broiler 捕获到虚拟机因为这些 MMIO VM-EXIT，那么 Broiler 找到对应的模拟设备，接着调用 MMIO 读写模拟函数 Broiler_mmio_callback() 进行 MMIO 读写模拟。当读写模拟完毕之后，虚拟机再次恢复运行，并继续执行导致 VM-EXIT 的指令. 接下来就是 Guest OS 内部驱动访问这些 MMIO 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] MMIO: Memory Mapped IO --->
          [*] Sychronous MMIO Driver for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Synchronous-mmio-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001948.png)
![](/assets/PDB/HK/TH001949.png)
![](/assets/PDB/HK/TH001950.png)

> [Broiler-Synchronous-mmio-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMIO/Broiler-Synchronous-mmio)

Guest OS 内操作 MMIO 的驱动源码比较简单，首先通过 request_resource() 向系统存储域树注册了设备的 MMIO，其 MMIO 范围为 \[0xD0000000, 0xD0000010], 驱动接着调用 ioremap() 函数将 MMIO 区域映射到虚拟内存，接着在 47 行读取 SLOT_NUM_REG 寄存器的值，然后向 SLOT_SEL_REG 寄存器写入 0x02, 最后将 SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ\_-REG 寄存器的值读取出来. 接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001951.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Synchronous-mmio-default.ko, 可以看到驱动加载成功，首先读取 SLOT_NUM_REG 寄存器的值为 32，接着向 SLOT_SEL_REG 写入 2 之后读出的值也是 2，最后 Freq 的范围在 \[0x010, 0x40]. 至此 Broiler 设备同步 MMIO 使用讲解完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="D6"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

##### In-Kernel 设备使用同步 MMIO

![](/assets/PDB/HK/TH001937.png)

In-Kernel 模拟的设备可以将其寄存器映射到 X86 的存储域，然后像访问普通内存一样访问外设的寄存器。Linux 提供了一系列的机制访问 MMIO，可以使用 MOV 指令访问，也可以通过建立虚拟地址到 MMIO 的页表访问。本节通过 In-Kernel 模拟一个普通的外设，外设内部存在几个寄存器，这些寄存器映射到存储。当虚拟机因为访问这些 MMIO 导致 VM-EXIT 之后，Broiler 模拟的设备对 MMIO 的读写进行模拟(In-Kernel 设备源码位置 footstuff/Broiler-Synchronous-pio-In-Kernel.c).

![](/assets/PDB/HK/TH001952.png)

由于 In-Kernel 设备在 KVM 里实现，因此 Broiler 只提供了简单的代码用于检查 KVM 是否支持创建采用同步 MMIO 的 In-Kernel 设备，如果支持那么通过 IOCTL KVM_CREATE_SYN-C_MMIO_DEV 创建一个 In-Kernel 设备。对于位于 KVM 的代码如下(footstuff/Broiler-Synchro-nous-pio-In-Kernel.patch):

![](/assets/PDB/HK/TH001953.png)

上图提供了一个 KVM 实现 In-Kernel 的 patch，由于 KVM 版本不同实际代码可能存在位置的差异，开发者可以参考该 patch 进行 In-Kernel 设备的创建。KVM 模拟了一个设备，设备内包含四个寄存器，分别为 SLOT_NUM_REG、SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器，KVM 在 107 行调用 kvm_iodevice_init() 函数创建一个 In-Kernel 设备，并在 109 行调用 kvm_io_bus_register_dev() 函数将设备的四个寄存器映射到 X86 存储域 \[0xD0000020, 0xD0000030]，四个寄存器的读模拟函数为 broiler_synchronous_mmio_read(), 以及写模拟函数为 broiler_synchronous_mmio_write(). 当 Guest OS 内对这四个寄存器进行读写操作时会导致 VM-EXIT, KVM 捕获到之后发现 MMIO 对应的设备位于 KVM 内，那么直接找到这个 In-Kernel 设备进行读写模拟。当 In-Kernel 设备读写模拟完毕之后，KVM 再次恢复虚拟机的执行，虚拟机恢复之后继续将访问 MMIO 的指令执行完毕. 接下来就是 Guest OS 内部驱动访问这些 MMIO 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻>辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] MMIO: Memory Mapped IO --->
          [*] Sychronous MMIO Driver In-Kernel for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Synchronous-mmio-In-Kernel-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001948.png)
![](/assets/PDB/HK/TH001954.png)
![](/assets/PDB/HK/TH001955.png)

> [Broiler-Synchronous-mmio-In-Kernel-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMIO/Broiler-Synchronous-mmio-In-Kernel)

Guest OS 内操作 MMIO 的驱动源码比较简单，首先通过 request_resource() 向系统存储域树注册了设备的 MMIO，其 MMIO 范围为 \[0xD0000020, 0xD0000030], 驱动接着调用 ioremap() 函数将 MMIO 区域映射到虚拟内存，接着在 47 行读取 SLOT_NUM_REG 寄存器的值，然后向 SLOT_S-EL_REG 寄存器写入 0x02, 最后将 SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器的值读取出来. 接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001956.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Synchronous-mmio-In-Kernel-default.ko, 可以看到驱动加载成功，首先读取 SLOT_NUM_REG 寄存器的值为 32，接着向 SLOT_SEL_REG 写入 2 之后读出的值也是 2，最后 Freq 的范围在 \[0x010, 0x40]. 至此 In-Kernel 设备同步 MMIO 使用讲解完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="D7"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

##### PCI 设备使用同步 MMIO

![](/assets/PDB/HK/TH001957.png)

PCI 模拟的设备同样可以将其 BAR 空间映射到 X86 的存储域，然后像访问普通内存一样访问 PCI 的 BAR 空间，这里暂称为 MEM-BAR。Linux 提供了一系列的机制访问 PCI MEM-BAR，可以使用 MOV 指令访问，也将一段虚拟内存映射到 MMIO 后直接访问。本节通过 Broiler 模拟一个 PCI 设备，PCI 设备包含了一个 MEM-BAR 空间，并将 MEM-BAR 映射到 X86 存储域。当虚拟机因为访问 MMIO 导致 VM-EXIT 之后，Broiler 模拟的 PCI 设备对 MEM-BAR 的读写进行模拟(PCI 设备源码位置 footstuff/Broiler-Synchronous-mmio-PCI.c).

![](/assets/PDB/HK/TH001958.png)

Broiler 模拟了 PCI 一个设备，PCI 设备内包一个 MEM-BAR，MMIO-BAR 内部含四个寄存器，分别为 SLOT_NUM_REG、SLOT_SEL_REG、MIN_FREQ_REG 和 MAX_FREQ_REG 寄存器，这里对 PCI 设备的模拟不做细节讲解，只对 PCI 如何使用同步 MMIO 进行讲解。Broiler 首先在 103 行调用 pci_alloc_mmio_port_block() 函数在 PCI 设备中创建一个 MEM-BAR, 接着在 113 行指明 BAR 映射到 X86 的存储域，114 行指明 MEM-BAR 的长度为 PCI_IO_SIZE. 最后 Broiler 在 119 行调用 pci_register_bar_regions() 注册了 MEM-BAR 的 Enable/Disable 函数。在 PCI BAR Enable 接口 Broiler_pci_bar_active() 函数中，Broiler 在 80 行调用 broiler_ioport_register() 函数最终将 MEM-BAR 映射到存储域，并提供了读写模拟函数 Broiler_pci_bar_callback(). 在 Broiler_pci_bar_callback() 函数中实现了对 MEM-BAR 里的四个寄存器的读写模拟. 接下来就是 Guest OS 内部驱动访问这些 MMIO 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] MMIO: Memory Mapped IO --->
          [*] Sychronous MMIO Driver In-Kernel for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Synchronous-mmio-PCI-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001948.png)
![](/assets/PDB/HK/TH001959.png)
![](/assets/PDB/HK/TH001960.png)

> [Broiler-Synchronous-mmio-PCI-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMIO/Broiler-Synchronous-mmio-PCI)

Guest OS 内操作 MMIO 基于一个 PCI 驱动，抛开 PCI 设备驱动的代码，驱动首先在 60 行调用 pci_request_region() 函数将 MEM-BAR 注册到内核的存储域资源树，然后在 67 行 pci_iomap() 将 MEM-BAR 对应的 MMIO 映射到虚拟内存，接下来就是 80-87 行使用 MMIO 操作函数对 MEM-BAR 的寄存器进行读写操作. 接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001961.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Synchronous-mmio-PCI-default.ko, 可以看到驱动加载成功，首先读取 SLOT_NUM_REG 寄存器的值为 0x20，接着向 SLOT_SEL_REG 写入 2 之后读出的值也是 2，最后 Freq 的范围在 \[0x010, 0x40]. 至此 Broiler PCI 设备使用同步 MMIO 讲解完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Y.jpg)

#### 异步 MMIO 虚拟化

![](/assets/PDB/HK/TH001903.png)

在外设内部存在一些列的寄存器，这些寄存器有的用来控制外设的行为、有的用来存储外设产生的数据。CPU 通过与外设的寄存器交互实现对外设的控制和数据传输。在 X86 架构中存在 IO Space，IO Space 可以通过 IO Port 方式寻址，外设可以将其寄存器映射到 IO Space，然后 CPU 可以直接通过 IO Port 访问外设; 另外外设也可以将寄存器映射存储域，并像普通内存一样访问这些寄存器，那么称这种外设寄存器的映射方式为 MMIO(Memory mapped IO). 常见的 MMIO 包括 PCI 设备的 BAR 空间和 LAPIC 内部寄存器等。

![](/assets/PDB/HK/TH001904.png)

**异步 MMIO(Asynchronous Memory Mapped IO)** 指的是 CPU 对 MMIO 发起写操作之后，不需要等待外设处理完毕 CPU 直接运行下一条指令。相对与同步 MMIO，异步 MMIO 只包括写操作，同步 MMIO 发起读写操作之后，CPU 需要外设完成读写请求并返回之后，CPU 才能继续执行下一条指令。异步 MMIO 一般用于触发外设执行异步操作，待外设异步操作执行完毕之后，再通过中断通知 CPU 异步请求已经完毕，CPU 被中断之后对异步操作收尾。常见异步 MMIO 比如 Kick 寄存器和 Doorball 寄存器等. 

![](/assets/PDB/HK/TH001977.png)

在 MMIO 虚拟化中，无论是同步 MMIO 还是异步 MMIO 的虚拟化，都不能独立存在，而是要基于模拟设备，模拟设备可以位于内核空间，那么称这类模拟设备为 In-Kernel 设备，而有的设备模拟位于用户空间的 Broiler，那么称这类设备为 In-Broiler 设备. 无论是哪类设备都可以对 MMIO 进行模拟. Guest OS 内部通过 MMIO 访问异步 MMIO，那么会导致 VM-EXIT。KVM 捕获到因为 MMIO 导致的 VM-EXIT 之后，获得 Guest OS 访问 MMIO 的信息。如果模拟设备位于 Kernel，那么直接通知设备进行 MMIO 模拟，通知完毕之后直接恢复虚拟机运行，In-Kernel 设备完成异步 MMIO 请求之后注入中断告诉虚拟机，虚拟机收到中断之后对异步 MMIO 进行收尾; 如果虚拟设备位于用户空间，那么 KVM 直接通过 eventfd 通知 Broiler，通知完毕之后虚拟机恢复运行，Broiler 在处理完异步 MMIO 请求之后，同样注入一个中断告诉虚拟机异步 MMIO 已经处理完毕了，虚拟机收到中断之后对异步 MMIO 进行收尾.

-------------------------------------

###### <span id="E1">In-Broiler 设备创建异步 MMIO</span>

![](/assets/PDB/HK/TH001895.png)

与同步 MMIO 相比，异步 MMIO 不仅要在 Broiler 设备中申请一段 MMIO 进行写模拟，而且还需要在 KVM 中注册一个异步写通知机制，大体流程如上: Broiler 设备申请了一段 MMIO 之后，然后为这段 MMIO 申请一个 eventfd 的管道，接着调用 ioctl KVM_IOEVENTFD 将 eventfd 告诉 KVM。KVM 在收到 ioctl KVM_IOEVENTFD 之后调用 kvm_ioeventfd() 函数进行异步通知链，调用的核心位于 kvm_assign_ioeventfd_idx() 函数，其首先调用 eventfd_ctx_fdget() 函数与用户空间的 eventfd 对接作为通知方，接着调用 kvm_iodevice_init() 函数为通知链提供了通知方法 ioeventfd_ops，即有异步通知的时候就会调用 ioeventfd_ops 提供的方法进行通知。函数接着调用 kvm_io_bus_register_dev() 函数在 KVM 维护的 MMIO BUS 上注册异步 MMIO 对应的区域，只要虚拟机访问了这段 MMIO，那么 KVM 会直接调用 ioeventfd_ops 提供的方法进行通知，以上便是异步 MMIO 注册的流程.

![](/assets/PDB/HK/TH001896.png)

在 KVM 中，每个虚拟机对应的 struct kvm 会维护 KVM_NR_BUSES 条虚拟总线，分别是 KVM_MMIO_BUS、KVM_PIO_BUS、KVM_VIRTIO_CCW_NOTIFY_BUS 和 KVM_FAST_MMIO_BUS，KVM 使用 struct kvm_io_range 描述虚拟总线上的一段 range，每条虚拟总线上维护了多个 range，通过 struct kvm_io_range 可以知道这段 range 是哪个设备的，但虚拟机访问的地址落在了 KVM 虚拟总线上某段 range 里，那么 KVM 会找到这段 range 对应的设备，然后调用设备提供的异步通知方法，并进行异步通知.

------------------------------

###### <span id="E2">In-Broiler 设备异步 MMIO 写流程</span>

![](/assets/PDB/HK/TH001966.png)
![](/assets/PDB/HK/TH001965.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="E3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

##### In-Broiler 设备使用异步 MMIO

![](/assets/PDB/HK/TH001967.png)

Broiler 模拟的设备可以将其寄存器映射到 X86 的存储域，然后向普通内存一样访问外设的寄存器。Linux 提供了一系列的机制访问 MMIO，可以使用 MOV 指令访问，也可以向访问存储域一样访问。本节通过 Broiler 模拟一个普通的外设，外设内部存在几个寄存器，这些寄存器映射到 X86 存储域 MMIO。当虚拟机因为访问异步 MMIO 会导致 VM-EXIT 之后，Broiler 模拟的设备对 MMIO 的写进行模拟之后。注入一个中断告诉设备异步 MMIO 写已经完成(In-Broiler 设备源码位置 footstuff/Broiler-Asynchronous-mmio-base.c).

![](/assets/PDB/HK/TH001968.png)

Broiler 模拟的设备分作三部分，第一部分是 MMIO 写模拟部分，第二个部分是异步 MMIO 注册部分，第三部分是中断注入部分。设备在 75 行调用 broiler_ioport_register() 函数为设备注册 BROILER_MMIO_BASE 处的 MMIO，并提供 MMIO 的读写模拟函数 Broiler_mmio_callback, 函数 Broiler_mmio_callback() 的用途是模拟 IRQ_NUM_REG 寄存器的读操作，IRQ_NUM_REG 寄存器里存储设备的中断号; 设备在 82 行调用 eventfd() 函数添加一个通知管道，接着在 88 行创建一个线程 irq_thread(), 在线程 irq_thread() 设备只要被 eventfd 管道通知，那么唤醒异步 MMIO 模拟，模拟完毕之后向虚拟机注入一个中断，设备在 94-98 行通过 struct kvm_ioeventfd 数据结构构建了异步通道的信息，然后在 101 行通过 ioctl 向 KVM 传入 KVM_IOEVENTFD, 以此告诉 KVM 为 MMIO 注册一个异步通知接口; 设备在 71 行调用 irq_alloc_from_irqchip() 函数为设备申请了一个中断，中断号存储在 irq 变量中，另外对 IRQ_NUM_REG 寄存器读模拟时读取的中断号来自 irq 变量，当异步 MMIO 处理完毕之后，设备可以在 45 行调用 broiler_irq_line() 函数进行中断注入，以此告诉虚拟机异步 MMIO 处理完毕. 接下来就是 Guest OS 内部驱动访问这些 MMIO 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] MMIO: Memory Mapped IO --->
          [*] Asychronous MMIO Driver for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Asynchronous-mmio-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001948.png)
![](/assets/PDB/HK/TH001969.png)
![](/assets/PDB/HK/TH001970.png)

> [Broiler-Asynchronous-mmio-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMIO/Broiler-Asynchronous-mmio)

Guest OS 内操作异步 MMIO 的驱动源码比较简单，首先通过 request_resource() 向系统存储域树注册了设备的 MMIO，其 MMIO 区域为 \[0xD0000040, 0xD0000050], 驱动接着调用 ioremap() 函数将 MMIO 区域映射到虚拟内存，接着在驱动的 53 行从 IRQ_NUM_REG 寄存器中获得中断号，并调用 request_irq() 函数为中断注册中断处理函数 Broiler_irq_handler(). 驱动最后在 65 行向寄存器 DOORBALL_REG 写，这就是对异步 MMIO 写操作. 接下来在 BiscuitOS 实践该案例: 

![](/assets/PDB/HK/TH001971.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Asynchronous-mmio-default.ko, 可以看到驱动加载成功，驱动使用的中断 5 并注册成功，待 3s 之后驱动收到了设备发来的驱动，因此异步 MMIO 的验证到此完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

<span id="E4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

##### PCI 设备使用异步 MMIO

![](/assets/PDB/HK/TH001972.png)

Broiler 模拟的设备可以将其寄存器映射到 X86 的存储域，然后像访问普通内存一样访问外设的寄存器。Linux 提供了一系列的机制访问 MMIO，可以使用 MOV 指令访问，也可以向访问存储域一样访问。本节通过 Broiler 模拟一个 PCI 设备, PCI 设备内部存在几个寄存器，这些寄存器映射到 X86 存储域。当虚拟机因为访问异步 MMIO 会导致 VM-EXIT 之后，Broiler 模拟的设备对 MMIO 的写进行模拟之后。注入一个中断告诉设备异步 MMIO 写已经完成(PCI 设备源码位置 footstuff/Broiler-Asynchronous-mmio-PCI.c).

![](/assets/PDB/HK/TH001973.png)

Broiler 模拟的 PCI 设备分作四部分，第一部分是 PCI 设备本身的虚拟，第二部分是 MMIO 写模拟部分，第三个部分是异步 MMIO 注册部分，第四部分是中断注入部分。由于本文重点介绍 MMIO，因此 PCI 设备模拟不做介绍. 设备在 160-161 行设置 PCI 设备 BAR0 长度为 PCI_IO_SIZE，并在 166 行通过 pci_register_bar_regions() 函数将 BAR0 注册到存储域空间，另外提供 MMIO 的模拟函数 Broiler_pci_bar_callback; 设备在 65 行调用 eventfd() 函数添加一个通知管道，接着在 72 行创建一个线程 irq_thread(), 在线程 irq_thread() 设备只要被 eventfd 管道通知，那么唤醒异步 MMIO 模拟，模拟完毕之后向虚拟机注入一个中断，设备在 79-84 行通过 struct kvm_ioeventfd 数据结构构建了异步通道的信息，然后在 86 行通过 ioctl 向 KVM 传入 KVM_IOEVENTFD, 以此告诉 KVM 为 MMIO 注册一个异步通知接口; 设备在 174 行调用 pci_assign_irq() 函数为设备申请了一个中断，中断号存储在 intx_irq 变量中，当异步 MMIO 处理完毕之后，设备可以在 52 行调用 broiler_irq_line() 函数进行中断注入，以此告诉虚拟机异步 MMIO 处理完毕. 接下来就是 Guest OS 内部驱动访问这些 MMIO 的分析，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] MMIO: Memory Mapped IO  --->
          [*] Asychronous MMIO Driver with PCI for Broiler

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-Asynchronous-mmio-PCI-default/
# 下载源码
make download
# 编译并运行源码
make
make install
make pack
# Broiler Rootfs 打包
cd output/linux-5.10-x86_64/package/BiscuitOS-Broiler-default/
make build
{% endhighlight %}

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH001948.png)
![](/assets/PDB/HK/TH001974.png)
![](/assets/PDB/HK/TH001975.png)

> [Broiler-Asynchronous-mmio-PCI-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMIO/Broiler-Asynchronous-mmio-PCI)

Guest OS 内操作异步 MMIO 的驱动源码比较简单，首先通过 request_resource() 向系统存储域树注册了设备的 MMIO，其 MMIO 区域为 \[0xc2000c00, 0xc2000d00], 驱动接着调用 pci_iomap() 函数将 MMIO 映射到虚拟内存，并调用 request_irq() 函数为中断注册中断处理函数 Broiler_irq_handler(). 驱动最后在 91 行向寄存器 DOORBALL_REG 写，这就是对异步 MMIO 写操作. 接下来在 BiscuitOS 实践该案例: 

![](/assets/PDB/HK/TH001976.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-Asynchronous-mmio-PCI-default.ko, 可以看到驱动加载成功，驱动使用的中断 8 并注册成功，待 3s 之后驱动收到了设备发来的驱动，因此异步 MMIO 的验证到此完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### X86 PIO/MMIO 虚拟化进阶研究 

-------------------------------------------

<span id="P1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

##### KVM 与 Broiler 之间 PIO/MMIO 数据桥梁

![](/assets/PDB/HK/TH001872.png)

在虚拟机访问 PIO 或者 MMIO 导致 VM-EXIT 时，KVM 发现 PIO/MMIO 对应的设备位于用户空间的 Broiler，那么需要将 PIO/MMIO 需要模拟的数据传递给 Broiler，然后 Broiler 接受到需要模拟的 PIO/MMIO 请求之后，Broiler 找打对应的模拟设备进行 PIO/MMIO 读写模拟。待 Broiler 模拟完毕之后需要再将模拟好的数据传递给 KVM，最后虚拟机恢复之前 KVM 将模拟的结果填充到 VMCS Host 区域指定的寄存器中，虚拟机恢复运行之后就可以获得所需的值. 以上便是 In-Broiler 设备模拟的流程，那么 KVM 和 Broiler 之间如何传递 PIO/MMIO 请求和应答?

![](/assets/PDB/HK/TH001982.png)

Broiler 在创建虚拟机时，KVM 先在内核空间申请了一段内存空间，然后 Broiler 通过 mmap 映射到这段存储空间，那么 KVM 和 Broiler 可以共享这块内存。KVM 使用 struct kvm_run 数据结构维护这段内存，并用以实现 KVM 与 Broiler 之间的数据交互。其中包含了两个区域 struct io 和 struct mmio，struct io 承载了 PIO 的模拟而 struct struct mmio 则承载了 MMIO 的模拟. 对于 PIO 的模拟通过 direction 指明模拟 PIO 指令的方向，如果是读 PIO 则是 KVM_EXIT_IO_IN，反之如果写 PIO 则 KVM_EXIT_IO_OUT; size 成员指明了指令读写 PIO 的宽度，可以是 1、2、4、8 字节; port 成员则指明 PIO 指明操作的 IO Port 编号; count 成员则指明指令执行的次数; data_offset 成员则指明了 PIO 模拟数据存储的位置. 对于 MMIO 的模拟 struct mmio 通过 phys_addr 指明了 MMIO 的地址; data 成员指向要模拟的数据; len 成员则表明 MMIO 模拟的长度; is_write 成员则指明了 MMIO 指令是读还是写. 接下来通过 PIO/MMIO 读两个理解进行介绍具体细节:

----------------------------------------------------

###### In-Broiler 设备 PIO 读模拟

![](/assets/PDB/HK/TH001925.png)

> [In-Broiler 设备使用同步 PIO](#B2)

上图是 In-Broiler 设备 PIO 读模拟流程和具体实践链接，开发者可以边看边实践。在 PIO 读模拟流程中，VM-EXIT 退出之后 KVM 捕获到退出原因是 EXIT_REASON_IO_INSTRUCTION，那么调用 handle_io() 函数进行处理，handle_io() 从 VMCS 的 Exit Qualification 寄存器获得需要模拟 PIO 读信息.

![](/assets/PDB/HK/TH001928.png)

KVM 发现 PIO 对应的设备位于 Broiler 中，因此需要将 PIO 模拟的信息通过 struct kvm_run 传递给用户空间的 Broiler. 具体实现位于 emulator_pio_in_out(). 42 行 KVM 将需要模拟的 PIO IO Port 信息存储在 struct kvm_cpu 的 arch.pio.port 里，然后将访问 PIO 方向信息存储在 struct kvm_cpu 的 arch.pio.in 里，同理将长度和指令次数信息存储在 arch.pio.count 和 arch.pio.size 里，存储在 struct kvm_cpu 的 arch.pio 信息供 KVM 和 X86 架构使用; 函数从 52 行开始将 VM-EXIT 原因 KVM_EXIT_IO 存储在 kvm_run 的 exit_reason, 接着对于 PIO 读将 KVM_EXIT_IO_IN 存储在 kvm_run 的 io.direction 中，另外还有需要模拟的 PIO IO Port 和长度、次数信息也存储到了 kvm_run 的 io 里. 最后将 kvm_run 的 io.data_offset 指向了 (KVM_PIO_PAGE_OFFSET * PAGE_SIZE) 处，以此存储 PIO 读模拟的数据.

![](/assets/PDB/HK/TH001929.png)

KVM 将 PIO 模拟的数据存储到 kvm_run.io 之后退出到 Broiler，Broiler 检查到虚拟机退出的原因是 KVM_EXIT_IO, 那么调用 broiler_cpu_emulate_io() 进行 PIO 模拟，可以看出 PIO 模拟所需的信息都来自 kvm_run->io 中.

![](/assets/PDB/HK/TH001983.png)

在 In-Broiler 设备内部，对于 PIO 读模拟在位于 50-62 行，参数 data 对应 kvm_run + kvm_run->io.data_offset, 参数 addr 对应 kvm_run->io.port, 参数 len 对应 kvm_run->io.size. Broiler_pio_callback() 函数通过调用 ioport_write32() 函数将 PIO 读模拟的值写入到 kvm_run + kvm_run->io.data_offset 处. In-Broiler 模拟完毕之后，Broiler 通过 KVM_RUN IOCTL 告诉 KVM 虚拟机恢复运行.

![](/assets/PDB/HK/TH001984.png)

KVM 在恢复虚拟机运行之前，通过 complete_fast_io() 函数将 PIO 读模拟的数据写入到 VMCS Guest 指定的寄存器中。在 emulator_pio_in() 函数中，程序直接跳转到 84 行处执行，可以看到 85 行处调用 memcpy() 函数将 kvm_vcpu->arch.pio_data 数据拷贝到 val 变量，但这里为什么不是 kvm_run + kvm_run->io.data_offset 模拟好的数据呢?

![](/assets/PDB/HK/TH001986.png)
![](/assets/PDB/HK/TH001985.png)

具体原因是在虚拟机创建过程中，KVM 在 kvm_arch_vcpu_create() 函数中调用 alloc_page() 函数分配一个物理页，然后将 vcpu->arch.pio_data 指向物理页对应的虚拟地址; 接着 KVM 在 kvm_vcpu_fault() 函数的 18-21 行将 VCPU \[KVM_PIO_PAGE_OFFSET * PAGE_SIZE, KVM_PIO_PAGE_OFFSET * PAGE_SIZE + PAGE_SIZE) 虚拟内存映射到 vcpu->arch.pio_data 对应的物理内存，也就是说 \[KVM_PIO_PAGE_OFFSET * PAGE_SIZE, KVM_PIO_PAGE_OFFSET * PAGE_SIZE + PAGE_SIZE) 与 \[vcpu->arch.pio_data, vcpu->arch.pio_data + PAGE_SIZE) 对应同一个区域. 那么之前的逻辑就说的通了，KVM 在刚开始模拟 PIO 之间将存储数据地址 kvm_run->io.data_offset 设置为 (KVM_PIO_PAGE_OFFSET * PAGE_SIZE), 然后 Broiler 将模拟完的数据存储在 (kvm_run + kvm_run->io.data_offset) 处，最后 KVM 在恢复虚拟机运行之前将 kvm_vcpu->arch.pio_data 数据拷贝到 val 变量，因为 kvm_vcpu->arch.pio_data 指向的就是 (kvm_run + kvm_run->io.data_offset)，因此 val 变量里存储的就是 PIO 读模拟的结果.

![](/assets/PDB/HK/TH001931.png)

val 变量获得 PIO 读模拟的值之后，调用 kvm_rax_write() 函数将 val 的值写入到 RAX/AX 中，因为 IN 指令最终要从 RAX/AX 中读出 IO Port 的值，最后虚拟机恢复运行之前 RAX/AX 的值会被更新到 VMCS GUEST 指定的区域，虚拟机恢复运行之后 RAX/AX 寄存器里就是 PIO 读模拟之后的值.

----------------------------------------------------

###### In-Broiler 设备 MMIO 读模拟

![](/assets/PDB/HK/TH001906.png)

> [In-Broiler 设备使用同步 MMIO](#D5)

上图是 In-Broiler 设备 MMIO 读模拟流程和具体实践链接，开发者可以边看边实践。在 MMIO 读模拟流程中，VM-EXIT 退出之后 KVM 捕获到退出原因是 EXIT_REASON_EPT_VIOLA-TION，那么调用 handle_ept_violation() 函数进行处理，handle_ept_violation() 从 VMCS 的 Exit Qualification 寄存器获得需要模拟 MMIO 读信息.

![](/assets/PDB/HK/TH001912.png)

由于虚拟机可以向访问普通内存一样访问 MMIO，因此可以使用不同类型的指令来访问 MMIO，那么 KVM 需要对访问 MMIO 的指令进行解码和模拟，以此获得 MMIO 模拟的相关的信息。emulator_read_write() 函数在发现 MMIO 对应的设备位于用户空间的 Broiler，那么其执行 6313 行分支. kvm_run->exit_reason 设置为 KVM_EXIT_MMIO，MMIO 地址存储在 kvm_run->mmio.phys_addr, 访问 MMIO 方向信息存储在 kvm_run->mmio.is_write 中，最后访问 MMIO 长度信息存储在 kvm_run->mmio.len 中. 接下来 KVM 通过 KVM_RUN IOCTL 退出到 Broiler.

![](/assets/PDB/HK/TH001914.png)

Broiler 检查到虚拟机退出的原因是 KVM_EXIT_MMIO, 那么调用 broiler_cpu_emula-te_mmio() 进行 MMIO 模拟，可以看出 MMIO 模拟的信息都来自 kvm_run->mmio 中.

![](/assets/PDB/HK/TH001987.png)

在 In-Broiler 设备内部，对于 MMIO 读模拟在位于 50-62 行，参数 data 对应 kvm_ru-n->mmio.data, 参数 addr 对应 kvm_run->mmio.phys_addr, 参数 len 对应 kvm_run->mmio.len. Broiler_mmio_callback() 函数通过调用 ioport_write32() 函数将 MMIO 读模拟的值写入到 kvm_run.mmio.data 处. In-Broiler 模拟完毕之后，Broiler 通过 KVM_RUN IOCTL 告诉 KVM 虚拟机恢复运行.

![](/assets/PDB/HK/TH001988.png)

KVM 在恢复虚拟机之前会调用 complete_emulated_mmio(), 其在 07 行判断到 MMIO 读操作，那么调用 memcpy() 函数将 kvm_run->mmio.data 模拟好的数据存储到 frag->data 中. frag 来自 vpcu->mmio_frqgment[] 数组，其通过 vcpu->mmio_cur_frqgment 指定.

![](/assets/PDB/HK/TH001989.png)

当 KVM 调用到 read_emulated() 函数时，其首先获得 ctxt 的 mem_read 成员，该成员内部维护 frag->data 的数据，也就是 kvm_run->mmio.data 中的数据，由于满足 54 行条件，那么直接跳转到 68 行将 MMIO 读模拟的数据拷贝到 dest 变量里，有函数调用关系可以知道 dest 指向 ctxt->src.valptr. KVM 最后将 MMIO 读模拟的值写入到了 VMCS Guest 指定区域内. 当虚拟机恢复运行之后，虚拟机从指定寄存器中读取了 MMIO 的值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)
