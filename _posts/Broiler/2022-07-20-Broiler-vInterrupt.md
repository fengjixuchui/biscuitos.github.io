---
layout: post
title:  "Broiler Interrupt Virtualization Technology"
date:   2022-07-20 12:00:00 +0800
categories: [MMU]
excerpt: Virtualization.
tags:
  - HypV
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [X86 中断机制](#E)
>
> - [中断重要概念](#E2)
>
> - [X86 中断虚拟化](#E3)
>
> - [PIC 8259A 虚拟化](#E5)
>
>   - [8259A 中断控制器编程](#E55)
>
>   - [vPIC 创建](#E50)
>
>   - [Broiler vPIC 中断配置](#E51)
>
>   - [Broiler 设备使用 vPIC 中断](#E52)
>
>   - [vPIC 中断注入](#E53)
>
> - [IOAPIC 虚拟化](#E6)
>
>   - [IOAPIC 中断控制器编程](#E65)
>  
>   - [vIOAPIC 创建](#E60)
>  
>   - [Broiler vIOAPIC 中断配置](#E61)
>  
>   - [Broiler 设备使用 vIOAPIC 中断](#E62)
>  
>   - [vIOAPIC 中断注入](#E63)
>
> - [MSI 中断虚拟化](#E7)
>
>   - [MSI 中断编程]()
> 
>   - [Broiler 设备使用 MSI 中断]()
> 
>   - [MSI 中断注入]()
>
> - [设备中断虚拟化](#E4)

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### X86 中断机制

![](/assets/PDB/HK/TH001754.png)

在计算机架构中，CPU 运行的速度远远大于外设运行的速度，在早期程序设计时，计算机如果要获得外部设备完成 IO 情况，计算机就不得不通过轮询来查询外设完成情况，因此往往做了很多无用的外设，从而导致计算机性能低下。为了解决这个问题引入了中断机制，**中断** 是为了解决外部设备完成某些工作之后通知 CPU 的一种机制，中断大大解放了 CPU, IO 操作效率大增. 在 X86 架构中，中断是一种电信号，由硬件外设产生，并直接送入中断控制器的输入引脚，然后由中断控制器下处理器发送相应的信号。处理器一旦检查到该信号，便会中断当前处理的工作转而处理中断。

![](/assets/PDB/HK/TH001755.png)

X86 架构的 CPU 为中断提供了两条外接引脚: **NMI** 和 **INTR**. 其中 NMI 是不可屏蔽中断，通常用于电源掉电和物理存储器奇偶校验; INTR 是可屏蔽中断，可以通过设置中断屏蔽位来进行中断屏蔽，主要用于接受外部硬件的中断信号，这些信号由中断控制器传递给 CPU。常见的中断控制器由两种: 8259A PIC 和 APIC.

###### <span id="E0">PIC 8259A</span>

![](/assets/PDB/HK/TH001756.png)

X86 架构中，传统的 PIC(Programmable Interrupt Controller) 可编程中断控制器由两片 8259A 外部芯片以级联的方式连接在一起，每个芯片可处理多达 8 个不同的 IRQ，Slave PIC 的 INT 输出线连接到 Master PIC 的 IRQ2 引脚，所以可用的 IRQ 线的个数达到 15 个. 中断引脚具有优先级，其中 IR0 优先级最高，IR7 优先级最低。PIC 内部具有三个重要的寄存器:

![](/assets/PDB/HK/TH001757.png)

**IRR**(Interrupt Request Register) 中断请求寄存器, 一共 8 bit 对应 IR0 ~ IR7 8 个中断引脚. 某位置位代表收到对应引脚收到中断但还没有提交给 CPU. **ISR**(In Service Register) 服务中寄存器，一共 8 bit，某位置位代表对应引脚的中断已提交给 CPU 处理，但 CPU 还没有处理完. **IMR**(Interrupt Mask Register) 中断屏蔽寄存器，一个 8 bit，某位置位代表对应的引脚被屏蔽. 除此之外，PIC 还有一个 EOI 位，当 CPU 处理完一个中断时，通过写该 bit 告知 PIC 中断处理完毕。PIC 向 CPU 递交中断的流程如下:

1. 一个或多个 IR 引脚产生电平信号，若中断对应的 IRR bit 没有置位.
2. PIC 拉高 INT 引脚通知 CPU 中断发生
3. CPU 通过 INTA 引脚应答 PIC 表示中断请求收到
4. PIC 收到 INTA 应答之后，将 IRR 中具有高优先级的位清零，并置位 ISR 对应位
5. CPU 通过 INTA 引脚第二次发出脉冲，PIC 将最高优先级 Vector 送到数据线上.
6. 等待 CPU 写 EOI.
7. 收到 EIO 后，ISR 中最高优先级位清零.

---------------------------------------------------

###### <span id="E1">APIC: Local APIC and I/O APIC</span>

![](/assets/PDB/HK/TH001758.png)

PIC 可以在 UP(单处理器)平台上工作，但无法用于 MP(多处理)平台，为此 **APIC**(Advanced Programmable Interrupt Controller) 因运而生。APIC 由位于 CPU 中的本地高级可编程中断控制器 **LAPIC**(Local Advanced Programmable Interrupt Controller) 和位于主板南桥中 I/O 高级可编程中断控制器 **I/O APIC**(I/O Advanced Programmable Interrupt Controller) 两部分构成，他们的关系如上图.

![](/assets/PDB/HK/TH001759.png)

每个 Logical Processor 逻辑处理器都有自己的 Local APIC，每个 local APIC 包含了一组 Local APIC 寄存器，用于控制 kicak 和 external 中断的产生、发送和接受等，也用于产生和发送 IPI。Local APIC 寄存器组以 MMIO 形式映射到系统的存储域空间，因此可以像操作物理内存一样访问。Local APIC 寄存器在存储域的起始物理地址为 0xFEE0000; 在 x2APIC 模式的 Local APIC 寄存器映射到 MSR 寄存器组来代替，因此可以使用 RDMSR 和 WRMSR 指令来访问 Local APIC 寄存器。

![](/assets/PDB/HK/TH001760.png)

Local APIC 由一组 LVT(Local vector table) 寄存器用来产生和接口 Local interrupt source. 由 LVT 的 LINT0 和 LINT1 寄存器对应着处理器 LINT0 和 LINT1 Pin, 它们可以直接接受外部 I/O 设备或连接 8259A 兼容类的外部中断控制器. 典型的 LINT0 作为处理器的 INTR Pin 接着外部的 8259 类的中断控制器的 INTR 输出端，LINT1 作为处理器的 NMI Pin 接外部设备的 NMI 请求.

![](/assets/PDB/HK/TH001761.png)

IO APIC 通常有 24 个不具有优先级的引脚用于连接外部设备，当收到某个引脚的中断信号之后，IO APIC 根据软件设定的 PRT(Programmable Redirection Table) 表查找对应引脚的 RTE(Redirection Table Entry). 通过 PTE 的各个字段，格式化出一条包含该中断所有信息的中断信息，再由系统总线发送给特定的 CPU 的 Local APIC，Local APIC 收到该信息之后择机将中断递交给 CPU 处理. IO APIC 也有自己的寄存器，同样也是通过 MMIO 映射到存储域空间。在 APIC 系统中，中断发起大致流程如下:

1. IO APIC 收到某个引脚产生的中断信号
2. 查找 PRT 表获得引脚对应的 RTE
3. 根据 RTE 个字段格式化出一条中断信息，并确定发送给哪个 CPU 的 LAPIC
4. 通过系统总线发送中断信息
5. Local APIC 收到中断信息，判断是否由自己接受
6. Local APIC 确认接受，将 IRR 中对应的位置位，同时确认是否由 CPU 处理
7. 确认由 CPU 处理中断，从 IRR 中获得最高优先级中断，将 ISR 中对应的位置位，提交中断。对于边缘触发的中断，IRR 中对应的位此时清零.
8. CPU 处理完中断，软件写 EOI 寄存器告知中断处理完成. 对于电平触发中断，IRR 中对应的位清零. Local APIC 提交下一个中断.

在 MP(多处理器) 平台上，多个 CPU 要协同工作，处理器间中断 (Inter-processor Interrupt, **IPI**) 提供 CPU 之间相互通信的手段。CPU 可以通过 Local APIC 的 ICR(Interrupt Command Register, 中断命令寄存器) 向指定的一个/多个 CPU 发送中断. OS 通常使用 IPI 来完成诸如进程转移、中断平衡和 TLB 刷新等任务.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------- 

<span id="E2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### 中断重要概念

![](/assets/PDB/HK/TH001763.png)

中断可分为**同步中断**(Synchronous Interrupt) 和**异步中断**(Asynchronous Interrupt)。同步中断是当指令执行时由 CPU 控制单元产生，之所以称为同步，是因为只有在一条指令执行完毕后 CPU 才会发出中断，发生中断之后 CPU 立即处理，而不是发生在代码指令执行期间，比如系统调用; 异步中断是指由其他硬件设备依照 CPU 时钟信号随机产生，即意味着中断能够在指令之间发生，中断产生之后不能立即被 CPU 执行.

![](/assets/PDB/HK/TH001762.png)

在 Intel X86 架构中，同步中断称为**异常**(exception), 异步中断称为**中断**(Interrupt), 中断可以分为**可屏蔽中断**(Maskable Interrupt) 和**不可屏蔽中断**(Nomaskable Interrupt). 异常可分为: **故障**(Fault)、**陷阱**(Trap)和**终止**(abort)三类. 在 X86 架构中每个中断被赋予一个唯一的编号或者向量**Vector**(8 位无符号整数).

![](/assets/PDB/HK/TH001764.png)

在 X86 架构保护模式下，系统使用**中断描述表**(Interrupt Descriptor Table, IDT) 表示中断向量表，总共 256 个描述符，IDT 的索引称为中断向量 Vector. IDT 表实际就是一个大数组，IDTR 寄存器指明了 IDT 在物理内存的位置以及长度，用于存放各种门(中断门、陷阱门、任务门)，这些门是中断和异常通往各自处理函数的入口。

----------------------------------

###### PIN/IRQ/GSI/VECTOR

PIN、IRQ、GSI 和 Vector 这几个概念容易搅浑，**IRQ** 是 PIC 时代的产物，由于 ISA 设备通常是连接到固定的 PIC 引脚，所以说一个设备的 IRQ 实际就是指它连接的 PIC 引脚号。IRQ 暗示着中断优先级，例如 IRQ0 比 IRQ1 有着更高的优先级。当进入到 APIC 时代，为了向前兼容，习惯用 IRQ 表示一个设备的中断号，但对于 16 以下的 IRQ，可能不再与 IOAPIC 的引脚对应. **Pin** 是引脚号，表示 IOAPIC 的引脚，PIC 时代类似的是 IRQ。Pin 的最大值受 IOAPIC 的引脚数限制，目前取值范围是 \[0, 23]. **GSI**(Global System Interrupt) 是 APIC 时代引入的概念，它为系统中的每个中断源指定唯一的中断号.

![](/assets/PDB/HK/TH001765.png)

上图中有 3 个 I/O APIC, IO-APIC0 具有 24 个引脚，其中 GSI Base 为 0, 每个 Pin 的 **GSI=GSI_Base + Pin** , 故 IO-APIC0 的 GSI 范围为 \[0: 23]. IO-APIC1 具有 16 个引脚，GSI base 为 24，GSI 范围为 \[24, 39], 以此类推。APIC 要求 ISA 的 16 个 IRQ 应该被 Identify map 到 GSI \[0, 15]. **Vector** 是中断中 IDT 表中的索引，是一个 CPU 概念，每个 IRQ (或 GSI) 都对应一个 Vector。在 PIC 模式下，IRQ 对应的 **vector = Start_Vector + IRQ**; 在 APIC 模式下, IRQ/GSI 的 vector 由操作系统分配.

##### 操作系统对中断/异常的处理流程

虽然各种操作系统对中断/异常处理的实现不同，但基本流程遵循如下顺序:
一个中断或异常发生，打断当前正在执行的任务

1. CPU 通过 vector 索引 IDT 表得到对应的门，并获得其处理函数的入口地址
2. 保存被打断任务的上下文，并跳转到中断处理函数进行执行
3. 如果是中断，处理完成后需要写 EOI 寄存器应答，异常不需要
4. 恢复被打断任务的上下文，准备返回
5. 从中断/异常的处理函数返回，恢复被打断的任务，使其继续执行

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------- 

<span id="E3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

#### X86 中断虚拟化

![](/assets/PDB/HK/TH001766.png)

在虚拟化场景中，VMM 也需要为 Guest OS 展现一个与物理中断架构类似的虚拟中断架构。如上图展示虚拟机的中断架构，和物理平台一样，每个 VCPU 都对应一个虚拟 Local APIC 用于接收中断. 虚拟平台也包含了虚拟 I/O APIC 或者虚拟 PIC 用于发送中断。和 VCPU 一样，虚拟 Local APIC、虚拟 I/O APIC 和 虚拟 PIC 都是由 VMM 维护. 当虚拟设备需要发送中断时，虚拟设备会调用虚拟 I/O APIC 的接口发送中断，虚拟 I/O APIC 根据中断请求，挑选出相应的虚拟 Local APIC, 调用其接口发出中断请求，虚拟 Local APIC 进一步利用 VT-x 的事件注入机制将中断注入到相应的 VCPU. 由此可见中断虚拟化主要任务就是实现虚拟 PIC、虚拟 I/O APIC 和虚拟 Local APIC，并且实现虚拟中断的生成、采集和注入的过程。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------

<span id="E5"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### PIC 8259A 虚拟化

![](/assets/PDB/HK/TH001757.png)

虚拟 PIC 本质上是一个虚拟设备，因此可以放在用户空间侧模拟(QEMU/Broiler)，也可以放在内核侧 KVM 来模拟。根据 PIC 硬件规范，在软件上模拟出虚拟 PIC 与物理 PIC 一样的接口。Broiler 将 PIC 的设备模拟放到了 KVM 里实现，因此 vPIC 是一个 In-Kernel 设备，vPIC 虚拟了 8259A 对中断的基本处理模拟，另外还包括 Broiler 向 vPIC 提交一个中断，vPIC 根据中断优先级选择合适的中断进行注入，注入的过程会促使 VM 发送 VM-EXIT，并将需要注入的中断写入到虚拟机 VMCS 指定的域中，当虚拟机再次 VM-ENTRY 时检查到有中断注入，那么虚拟机运行时触发中断。本节用于全面介绍 vPIC 中断虚拟过程，并分作一下几个章节进行讲解:

> - [8259A 中断控制器编程](#E55)
>
> - [vPIC 创建](#E50)
>
> - [Broiler vPIC 中断配置](#E51)
>
> - [Broiler 设备使用 vPIC 中断](#E52)
>
> - [vPIC 中断注入](#E53)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

###### <span id="E55">8259A 中断控制器编程</span>

x86 架构中 vPIC 通过模拟 8259A 中断控制器的逻辑来实现中断模拟，并侧重模拟 8259A 内部的寄存器处理逻辑，而 8259A 中断控制器编程侧重从操作系统对 8259A 的使用进行描述，那么本节用于介绍 8259A 的编程逻辑，这对 vPIC 的模拟有一定的帮助。根据 PIC 硬件规范，PIC 主要为软件提供了以下几个接口用于操作 PIC:

* 4 个初始化命令字(Initialization Command Words): ICW1/ICW2/ICW3/ICW4
* 3 个操作命令字(Operation Command Words): OCW1/OCW2/OCW3

![](/assets/PDB/HK/TH001797.png)

ICW1 寄存器用于初始化 8259A 的连接方式和中断触发方式，其中 BIT0 用于指明 ICW4 是否启用，在 X86 架构该 BIT 必须置位. BIT1 用于指明系统中是单片 8259A 还是级联两块 8259A，置位表示单片，清零则表示级联 2 块 8259A. BIT3 用于指明中断的触发方式，置位表示电平，清零则表示边缘触发。BIT4 在 x86 架构下必须置位. ICW1 需要写入主 8259A 的 0x20 端口和从 8259A 的 0xA0 端口.

![](/assets/PDB/HK/TH001798.png)

ICW2 寄存器用于设置其实中断向量，其中 BIT0-BIT0 用于指明中断号。ICW2 需要写入主 8259A 的 0x21 端口和从 8259A 的 0xA1 端口.

![](/assets/PDB/HK/TH001799.png)

ICW3 寄存器用于指定主从 8259A 的级联引脚，在 x86 架构中，从 8259A 级联到主 8259A 的 IRQ2 引脚上，因此主 8259A 的 ICW3 寄存器值为 0x04, 从 8259A 的 ICW3 寄存器值为 0x02. ICW3 需要写入主 8259A 的 0x21 端口和从 8259A 的 0xa1 端口.

![](/assets/PDB/HK/TH001800.png)

ICW4 寄存器用于初始化 8259A 数据连接方式和中断触发方式。在 x86 架构中 BIT0 uPM 必须置位; BIT1 位 AEOA，如果置位中断自动结束 AUTO EOI，如果清零则 8259A 需要收到 EOI 才算中断处理完成; BIT2-BIT3 用于指明缓存模式，BIT3 清零，那么非缓存模式，BIT3 置位则缓存模式; BIT2 用于指明主从 8259A 的缓存模式，置位表示主 8259A，清零则表示从 8259A. BIT4 用于设置嵌套模式，如果置位则表示特殊全嵌套模式，清零则全嵌套模式。ICW4 寄存器需要写入主 8259A 的 0x21 端口和从 8259A 的 0xA1 端口.

![](/assets/PDB/HK/TH001801.png)

上图是简单的代码演示主从 8259A 的初始化. 

![](/assets/PDB/HK/TH001804.png)

OCW1 寄存器用于屏蔽连接在 8259A 上的中断源，某位置位则屏蔽对应的中断源，某个清零则不屏蔽对应的中断源. OCW1 寄存器需要写入主 8259A 的 0x20 端口和从 8259A 的 0xA0 端口.

![](/assets/PDB/HK/TH001802.png)

OCW2 寄存器用于设置中断结束方式和优先级模式。BIT0-BIT2 用于指明中断优先级。BIT3-BIT4 在 X86 架构必须为 0. BIT5 EOI 手动结束中断时有意义，该位置位表示中断结束就清除 ISR 相应位, 改位清零则无意义. BIT6 SL 位，该位置位使用 BIT0-BIT2 指定需要结束的中断, 该位清零则结束正在处理的中断，并将 ISR 优先级最高的位清零. BIT7 R 位，该位置位则采用循环优先级方式，每个 IR 接口优先级在 0-7 循环，该位清零则采用固定优先级，IR 接口号越低优先级越高. OCW2 寄存器需要写入主 8259A 的 0x20 端口和从 8259A 的 0xA0 端口.

![](/assets/PDB/HK/TH001803.png)

OCW3 寄存器用于设置特殊屏蔽方式以及查询方式。BIT0 RIS 标志位，该位置位则读取 ISR 寄存器的值，该位清零则读取 IRR 寄存器. BIT1 RR 标志位，该位置位则读取寄存器，该位清零则无意义。BIT2 P 标志位，该位置位则表示中断查询命令查询当前最高中断优先级，该位清零则无意义; BIT3-BIT4 在 X86 架构固定位 0x01; BIT5 SMM 标志位，该位置位表示工作域特殊屏蔽模式，该位清零则未工作与特殊屏蔽模式; BIT6 ESMM 标志位，该位置位则表示启用特殊屏蔽模拟, 该位清零则表示关闭特殊屏蔽模式. OCW3 需要写入主 8259A 0x20 端口和从 8259A 的 0xA0 端口.

![](/assets/PDB/HK/TH001805.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

###### <span id="E50">vPIC 创建</span>

![](/assets/PDB/HK/TH001775.png)

vPIC 的创建主要分作两部分，第一个部分是在内核空间虚拟一个 vPIC 设备，其二是对默认引脚的模拟. Broiler 在初始化阶段通过调用 kvm_init() 函数，并使用 iotcl 方式向 KVM 传入命令 KVM_CREATE_IRQCHIP 用于创建 vPIC. KVM 的 kvm_arch_vm_ioctl() 函数收到了 Broiler 传递下来的命令，然后找到对应的处理分支 KVM_CREATE_IRQCHIP, 分支里有两个主要的函数分支，其中 kvm_pic_init() 函数用于在 KVM 内虚拟一个 vPIC 设备，用于模拟 vPIC 的 IO 端口; 另外一个函数是 kvm_setup_default_irq_routing() 函数，该函数用于对 vPIC 默认引脚的模拟.

![](/assets/PDB/HK/TH001776.png)

KVM 使用 struct kvm_pic 数据结构描述一个 vPIC 设备，pics[] 成员表示 x86 架构中存在两块 8259A 芯片，其中 pics[0] 作为主 PIC，而 pics[1] 作为从 PIC，pics[] 使用 struct kvm_kpic_state 数据结构进行描述，该数据结构的成员用于模拟 PIC 设备内部的寄存器，例如 irr 对应 PIC 的 Interrupt request Register, imr 对应 PIC 的 Interrupt Mask Register, 以及 isr 对应 PIC 的 In Service Register 等. output 成员表示 vPIC 芯片是否有新中断需要注入，dev_master/dev_slave/dev_eclr 则是 KVM 模拟的三个设备，前两个分别模拟主 PIC 设备和从 PIC 设备。irq_states[] 数组则是对 vPIC 每个引脚状态的模拟.

![](/assets/PDB/HK/TH001767.png)

回到 KVM 对 vPIC 设备模拟的调用逻辑，KVM 调用 kvm_pic_init() 函数对 vPIC 设备进行模拟，其通过调用 kvm_iodevce_init() 函数对主 vPIC 设备、从 vPIC 设备和 eclr 设备进行模拟，并为这些设备提供了 IO 端口读写进行模拟，IO 端口读操作最终通过 pcidev_read() 函数实现，IO 端口写操作最终通过 pcidev_write() 函数实现. 函数通过调用 kvm_io_bus_register_dev() 函数将 0x20/0x21/0xa0/0xa1 注册到 KVM，只要 Guest OS 访问这几个 IO 端口就会因为 EXIT_REASON_IO_INSTRUCTION 原因 VM-EXIT.

![](/assets/PDB/HK/TH001777.png)

根据 8259A 芯片的逻辑，picdev_read() 函数实现了对 ICW/OCW 寄存器读模拟，可以看到对端口 0x20/0x21/0xa0/0xa1 的读取是根据地址推算出主从 vPIC，然后通过 pic_ioport_read() 函数从其对应的 struct kvm_kpic_state 数据接口中读取指定成员, 以此完成 IO 端口读模拟.

![](/assets/PDB/HK/TH001778.png)

同理根据 8259A 芯片的逻辑，picdev_write() 函数实现了对 ICW/OCW 寄存器写模拟，可以看到对端口 0x20/0x21/0xa0/0xa1 的写是根据地址推算出主从 vPIC，然后通过 pic_ioport_write() 函数从其对应的 struct kvm_kpic_state 数据接口中写入指定成员，以此完成 IO 端口写模拟.

![](/assets/PDB/HK/TH001779.png)

vPIC 创建的最后一步是对默认引脚的模拟，通过之前的分析可以知道 x86 架构存在两片级联的 8259A 芯片，其中一片做主片另外一片做从片。每个 8259A 包含 8 个引脚，其中主片的 Pin2 连接从片的 INT 引脚。KVM 对引脚的模拟通过调用 kvm_set_default_irq_routing() 函数实现，该函数包含了公共路径代码，对 vPIC 引脚的模拟主要在 setup_routing_entry() 函数中实现，其通过调用函数 kvm_set_routing_entry() 函数将 vPIC 对应的引脚中断模拟函数设置为 kvm_set_pic_irq() 函数，另外将模拟好的引脚加入到 hash 链表上。

![](/assets/PDB/HK/TH001780.png)

KVM 使用 struct kvm_irq_routing_table 数据结构维护 vPIC 和 vIOAPIC 的引脚，其中 chip[] 数组为了所有引脚信息，nr_rt_entries 成员则表示引脚的总数，另外使用 map[] 哈希链表维护了引脚和 Chip 的映射关系，即引脚是属于 vPIC 还是 vIOAPIC. KVM 使用 struct kvm_kernel_irq_routing_entry 数据结构描述一个 GSI，GSI 可以对应 vPIC 的一个引脚，也可以描述 vIOAPIC 的一个引脚，其成员 gsi 表示引脚的 GSI 号，set 回调函数则表示引脚的中断模拟，irqchip 成员则表示引脚对应的 CHIP 信息和引脚信息，最后 link 成员用于将模拟的引脚统一维护在 KVM struct kvm_irq_routing_table 数组结构 map[] 哈希链表上.

![](/assets/PDB/HK/TH001781.png)

struct kvm_irq_routing_table 数据架构维护了 vPIC 和 vIOAPIC 引脚与 GSI 的映射关系，通过这个关系可以从设备的引脚推出 GSI 号，而 map[] 哈希链表则维护了 GSI 与引脚的关系，在 KVM 中模拟的 vPIC 和 vIOAPIC 的引脚是可以重叠的，因此一个 GSI 号可能对应 vPIC 或者 vIOAPIC 的引脚，或者同时对应 vPIC 和 vIOAPIC 的引脚，那么使用一个 map[]  hash 链表，并使用 GSI 作为 key, struct kvm_kernel_irq_routing_entry 作为 hash 链表的成员, 最终形成了上图的逻辑结构.

![](/assets/PDB/HK/TH001782.png)

KVM 默认支持的中断设备的引脚如上图，从 ROUTING_ENTRY2 宏的定义可以看出 vPIC 的引脚是 \[0, 15], vIOAPIC 的引脚范围是 \[0, 23]，那么 vPIC 和 vIOAPIC 之间共享了 \[0, 15] 的引脚.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

###### <span id="E51">Broiler vPIC 中断配置</span>

![](/assets/PDB/HK/TH001784.png)

KVM 在创建 vPIC 的时候，已经默认创建了一套 vPIC 和 vIOAPIC 引脚映射逻辑，虽然可以使用，但作为 HypV 软件，Broiler 还是可以自定义映射 vPIC 的引脚与 GSI 映射逻辑。如上图是 Broiler 虚拟的中断引脚连接逻辑，Broielr 规划的中断引脚逻辑是: 主 vPIC 的 8 个引脚独占 GSI0-GSI7, 从 vPIC 的前 4 个引脚独占 GSI8-GIS11, 后 4 个引脚与 vIOAPIC 前 12-15 引脚共享 GSI12-GSI15, vIOAPC 16-24 引脚独占 GSI16-GSI24.

![](/assets/PDB/HK/TH001785.png)

Broiler 在 broiler_irq_init() 函数中对 GSI 进行了重映射，16-20 行用于映射 GSI0-GSI7 给 IRQCHIP_MASTER，33-25 行用于映射 GSI8-GSI15 给 IRQCHIP_SLAVE, 27-29 行用于映射 GSI12-GSI24 个 IRQCHIP_IOAPIC, 可以看出从 vPIC 和 vIOAPIC 共享了 GSI12-GSI15, 最后调用 ioctl 向 KVM 传入 KVM_SET_GSI_ROUTING 使映射生效. 

![](/assets/PDB/HK/TH001783.png)

vPIC 的模拟位于内核是一个 In-Kernel 的设备，当 Broiler 对中断映射有了更改，需要通过 ioctl() 函数传入 KVM_SET_GSI_ROUTING 命令来更新 vPIC 的中断引脚与 GSI 的映射。KVM 的 kvm_vm_ioctl() 函数收到 KVM_SET_GSI_ROUTING 命令之后，调用 kvm_set_irq_routing() 函数进行更新，更新核心通过 kvm_set_routing_entry() 函数，其会将主从 vPIC 的引脚的中断模拟函数设置为 kvm_set_pic_irq() 函数.

![](/assets/PDB/HK/TH001786.png)

struct kvm_irq_routing_table 数据架构维护了 vPIC 引脚与 GSI 的映射关系，通过这个关系可以从设备的引脚推出 GSI 号，而 map[] 哈希链表则维护了 GSI 与引脚的关系，在 KVM 中模拟的 vPIC 的引脚是可以重叠的，因此一个 GSI 号可能对应 vPIC 或者 vIOAPIC 的引脚，或者同时对应 vPIC 和 vIOAPIC 的引脚，那么使用一个 map[]  hash 链表，并使用 GSI 作为 key, struct kvm_kernel_irq_routing_entry 作为 hash 链表的成员, 映射完毕之后 Broiler 的 vPIC 引脚连接如上.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="E52">Broiler 设备使用 vPIC 中断</span>

![](/assets/PDB/HK/TH001787.png)

Broiler 配置完 vPIC 中断之后，那么接下来就是在设备中使用 vPIC 提供的中断，Broiler 目前支持的设备包括 PCI 设备和 IO 端口设备。对于 IO 端口设备 Broiler 提供了 irq_alloc_from_irqchip() 函数从 vPIC 中分配中断，而 PIC 设备如果使用 INTX 中断的话，Broiler 提供了 pci_assign_irq() 函数从 vPIC 中分配中断。对于分配的 vPIC 中断，中断的触发分为电平触发和边缘触发，Broiler 模拟的设备可以使用 broiler_irq_line() 函数进行电平触发，而使用 broiler_irq_trigger() 函数进行边缘触发。那么接下来以 Broiler 一个 IO 端口设备使用电平触发方式给 Guest OS 前端驱动发中断的案例进行讲解:

![](/assets/PDB/HK/TH001768.png)

从虚拟机内部看到系统 IO 空间中，主 PIC 映射到端口 0x20-0x21，从 PIC 映射到端口 0xa0-0xa1. 接下来在 Broiler 虚拟一个设备，其包含一个异步 IO，对该 IO 进行读操作会出发一个中断(源码位置: foodstuff/Broiler-interrup-vPIC.c)

![](/assets/PDB/HK/TH001788.png)

Broiler 模拟了一个设备，该设备包含了一段 IO 端口，其范围是 \[0x6020, 0x6030], 里面包含了两个寄存器，第一个寄存器 IRQ_NUM_REG 用于获得设置使用的 IRQ，第二个寄存器是一个异步 IO，对该寄存器进行写操作会触发中断。Broiler 在 70 行调用 irq_alloc_from_irqchip() 函数从 vPIC 中分配一个 IRQ 号，然后调用 broiler_irq_line() 函数将 IRQ 的电平设置为低低电平。Broiler 在 74-103 行实现一个异步 IO, 如果 Guest OS 对该 IO 端口进行写操作，那么会唤醒 irq_threads 线程，线程的作用是向 Guest OS 注入一个 IRQ 中断，可以看到 45 行调用 broiler_irq_line() 函数拉高电平以便模拟高电平，进而产生中断，此时 KVM 会收到 ioctl 命令 KVM_IRQ_LINE, 然后 KVM 向 Guest OS 注入中断. 那么有了 Broiler 侧的模拟设备，接下来就是 Guest OS 内部驱动来处理中断，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] KVM --->
          [*] vInterrupt: Broiler vPIC interrupt

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-vPIC-interrupt-default/
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
![](/assets/PDB/HK/TH001770.png)
![](/assets/PDB/HK/TH001771.png)
![](/assets/PDB/HK/TH001772.png)

> [Broiler-vPIC-interrupt-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Virtualization/vInterrupt/Broiler-vPIC-interrupt)

Guest OS 驱动源码比较简单，首先通过 request_resource() 向 IO 空间注册了设备的 IO 端口，其 IO 端口域为 \[0x6020, 0x6030], 驱动接着调用 ioport_map() 函数将 IO 端口映射到虚拟内存，接着在 52 行从设备的 0x6024 端口获得设备使用的中断号，接着在 53 行调用 request_irq() 进行中断处理函数注册，此时中断的触发方式设置为高电平触发，中断处理函数是 Broiler_irq_handler()，其内部用于在接收到中断之后打印中断号。最后驱动向端口 0x6020 进行写操作，那么这会触发设备向 CPU 发送一个中断，接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001773.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-vPIC-interrupt-default.ko, 可以看到驱动加载成功，等待 5s 之后中断处理程序收到设备发来的中断，此时可以看到 IRQ 为 6，接着查看 IO 空间，可以看到端口 0x6020-0x6030 分配给 "Broiler PIO vPIC" 使用。最后可以查看 /proc/interrupts 节点获得中断映射关系，也就是开篇的图片可以看到 Broiler-PIO-vPIC 使用的中断.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="E53">vPIC 中断注入</span>

![](/assets/PDB/HK/TH001789.png)

vPIC 中断注入流程流程如上图，分作两个大的部分，第一部分是 Broiler 通过 ioctl() 函数向 KVM 传入 KVM_IRQ_LINE 命令，以此告诉 KVM 进行 vPIC 中断注入，此时 Broiler 和 KVM 是异步运行的，此时 Guest OS 在运行并没有 VM_EXIT. 当 KVM 的 vPIC 通过中断评估之后决定将某个中断注入到 Guesst，这时会向 KVM 发送 KVM_REQ_EVENT 请求，并发送 IPI 中断让虚拟机发生 VM-EXIT; 中断注入的第二阶段则是让虚拟机 VM-EXIT 之后再次 VM_ENTRY, 此时 KVM 会检查 VM_ENTRY_INTR_INFO_FIELD 域里是否注入了中断，如果注入那么虚拟机 VM-ENTRY 之后就触发中断，因此第二部分的任务就是在 VM-ENTRY 之前向 VM_ENTRY_INTR_INFO_FIELD 写入要注入的中断，KVM 通过在 vmx_inject_irq() 函数中调用 vmcs_write32() 函数向 VM_ENTRY_INTR_INFO_FIELD 域中写入了要注入的中断，最后虚拟机 VM-ENTRY 之后就收到中断，这个阶段 KVM 是同步运行的. 那么接下来对流程进行细节分析.

![](/assets/PDB/HK/TH001790.png)

pic_set_irq1() 函数用于模拟中断产生之后，vPIC 的 Interrupt Request Register 收到中断之后将对应 bit 置位的模拟，从函数的实现可以看出 IRR 寄存器接收电平触发和边缘触发的中断，93-100 行处理电平触发的中断，如果模拟中断是电平触发，那么如果 level 为真，即中断引脚产生了一个高电平，那么 IRR 寄存器就将引脚对应的 bit 置位，并将 last_irr 寄存器对应的 bit 也置位; 反之如果是一个低电平，那么中断消除，于是将 IRR 寄存器对应的 bit 清零. 如果是边缘触发，那么函数使用 102-110 行进行中断边缘触发模拟，103-108 行如果此时 last_irr 对应引脚之前是低电平，那么此时中断引脚触发一个上升沿信号，那么此时 IRR 寄存器将引脚对应的 bit 置位; 反之只使用 last_irr 寄存器记录电平信号; 108-109 行产生一个低电平，并且下降沿不触发中断，那么只使用 last_irr 寄存器记录电平信息. 函数在 112 行检查 Interrupt mask register 寄存器是否已经屏蔽该中断，如果屏蔽直接返回 -1; 反之返回 1.

![](/assets/PDB/HK/TH001791.png)

pic_get_irq() 函数用于中断评估，以此决定是否有中断送往 CPU 进行处理. 函数 137 行的作用是模拟通过 IRR 寄存器和 IMR 寄存器获得不被屏蔽可以处理的中断，函数 138 行则根据优先级策略获得最高优先级。函数接着在 146 行模拟从 ISR 寄存器获得当前正在处理的中断，然后获得对应的优先级，最后在 150-156 行的优先级判断该 PIC 是否有中断送往 CPU 处理.

![](/assets/PDB/HK/TH001792.png)

pic_update_irq() 函数用于判断主从 vPIC 中是否有中断产生，如果有就进行标记，以便虚拟机再次 VM-ENTRY 时注入中断。函数在 167 行判断从 vPIC 是否有中断需要 CPU 处理，如果有则进行入 168 分支进行处理，函数在该分支将主 vPIC 的 IRQ2 置位，以此模拟从 vPIC 产生中断并切主 vPIC 的 2 号引脚接受该中断。函数接着在 175 行再次调用 pic_get_irq() 函数判断主 vPIC 是否有中断需要 CPU 处理，如果有则调用 pic_irq_request() 函数告诉 KVM 主 vPIC 有中断需要 CPU 处理.

![](/assets/PDB/HK/TH001793.png)

pic_unlock() 函数的作用是向 KVM 提出 KVM_REQ_EVENT 需求，并让虚拟机 VM-EXIT. 函数在 62 行调用 kvm_make_request() 函数向 KVM 提出 KVM_REQ_EVENT 需求，KVM 在 VM-ENTRY 之间如果检查到有 KVM_REQ_EVENT 需求，那么其会检查 VM_ENTRY_INTR_INFO_FIELD 域是否有中断需要注入. 中断的注入只能在 VM-ENTRY 时才能注入，那么对于没有 VM-EXIT 的虚拟机，KVM 通过让 VCPU 发送 IPI 中断让虚拟机 VM-EXIT。至此 vPIC 的中断主动注入已经完成，接下来就是虚拟机再次 VM-ENTRY 之前将需要注入的中断写入 VM_ENTRY_INTR_INFO_FIELD.

![](/assets/PDB/HK/TH001794.png)

虚拟机再次 VM-ENTRY 之前会调用 vcpu_enter_guest() 函数进行检查，此时函数在 8865 行发现有 KVM_REQ_EVENT 请求，那函数 8873 行调用 inject_pending_event() 函数进行中断注入. 

![](/assets/PDB/HK/TH001795.png)

在 inject_pending_event() 内部，函数在 8310 行通过 kvm_cpu_has_injectable_intr() 函数知道 vPIC 中断需要注入，那么函数进入 8311 分支进行处理，对于 vPIC 中断函数进入 8315 分支进行处理，函数首先调用 kvm_queue_interrupt() 函数获得需要注入中断的 vector，然后调用 vmx_inject_irq() 函数向 VMCS 的 VM_ENTRY_INTR_INFO_FIELD 域注入中断.

![](/assets/PDB/HK/TH001796.png)

vmx_inject_irq() 函数用于最终的中断注入操作，从函数实现可以看到 4500 行获得需要注入的中断号，然后在 4519 行调用 vmcs_write32() 函数向 irq 在 VM_ENTRY_INTR_INFO_FIELD 域中对应的 bit 置位，至此 vPIC 的中断注入完结, 虚拟机 VM-ENTRY 检查到该域有置位，那么虚拟机 RESUME 之后就向虚拟机注入一个中断.

![](/assets/PDB/HK/TH001824.png)

VM_ENTRY_INTR_INFO_FIELD 域是一个 32 位域，Vector 字段用于描述注入的中断向量号，
Deliver err Code 域指明是否需要向 Guest 的堆栈中写入错误码，Valid 域用于表示需要
向 Guest OS 注入中断，VM-EXIT 时自动清除该域，Interrupt type 指明注入中断的类型>，具体支持如下:

* 0: External Interrupt
* 1: Reserved
* 2: NMI
* 3: Hardware exception (e.g. #PF)
* 4: Software interrupt (INT n)
* 5: Privileged software exception (INT 1)
* 6: Software exception (INT 3 or INTO)
* 7: Other event

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------

<span id="E6"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### IOAPIC 虚拟化

![](/assets/PDB/HK/TH001758.png)

虚拟 IOAPIC 本质上是一个虚拟设备，因此可以放在用户空间侧模拟(QEMU/Broiler), 也可以放在内核侧 KVM 来模拟。根据 IOAPIC 硬件规范，在软件上模拟出 IOAPIC 与物理 IOAPIC 一样的接口。Broiler 将 IOAPIC 设备模拟放到了 KVM 里，因此 vIOAPIC 是一个 In-Kernel 设置，vIOAPIC 虚拟了 IOAPIC 中断的基本处理流程，另外还包括 Broiler 向 vIOAPIC 提交一个中断，vIOAPIC 根据中断优先级选择合适的中断向目的 LAPIC 进行注入，注入的过程会促使 VM 发送 VM-EXIT，并将需要注入的中断写入到虚拟机 VMCS 指定的域中，当虚拟机再次 VM-ENTRY 时检查到有中断注入，那么虚拟机运行时触发中断。本节用于全面介绍 vIOAPIC 中断虚拟过程，并分作一下几个章节进行讲解:

> - [IOAPIC 中断控制器编程](#E65)
>
> - [vIOAPIC 创建](#E60)
>
> - [Broiler vIOAPIC 中断配置](#E61)
>
> - [Broiler 设备使用 vIOAPIC 中断](#E62)
>
> - [vIOAPIC 中断注入](#E63)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

###### <span id="E65">IOAPIC 中断控制器编程</span>

![](/assets/PDB/HK/TH001759.png)

IOAPIC 的主要作用是中断分发，最初有一条专门的 APIC 总线用于 IOAPIC 和 LAPIC 通行，在架构升级过程中，X86 将 IOAPIC 和 Local APIC 的通信合并到系统总线中。IOAPIC 内部包含了一些列的寄存器，这些寄存器通过 MMIO 机制映射到系统存储域的 0xFEC00000 处，其 Index、Data、IRQPin Assertion 和 EOI 寄存器可以直接访问，其他寄存器需要通过读写 Index/Data 寄存器方式访问，具体访问是通过先将 8bit 索引写入到 Index 寄存器，然后从 Data 寄存器读写对应的内容.

![](/assets/PDB/HK/TH001807.png)

**PRT Table**(Programmable Redirection Table) 可编程重定位表，IOAPIC 内部包含了 一个 PRT 表，可以根据 table 的格式发送中断给特定 CPU 的 Local APIC，再由 Local APIC 通知 CPU 进行处理。通常一个 IOAPIC 由 24 个中断引脚，每个引脚对应一个 **RTE**(Redirection Table Entry)，这些引脚本身并没有优先级的区分，但是 RTE 中会有 Vector，Local 会基于 Vector 设定优先级.

![](/assets/PDB/HK/TH001808.png)

* Destination Field: 根据 Destination Mode 域的值含义有所不同，如果处于 Physical Mode, 其值为 APIC ID 用于标识一个唯一的 APIC。如果处于 Logical Mode，其值根据 Local APIC 的不同配置，代表一组 CPU.
* Interrupt Mask: 中断屏蔽位，置位时对应的中断引脚被屏蔽; 清零时对应引脚产生的中断被发送至 Local APIC.
* Trigger Mode: 指明该引脚的中断由什么方式触发，置位为电平触发，清零为边缘触发.
* Remote IRR: 远程 IRR，且只对电平触发中断有效，Local APIC 会在收到该引脚的中断之后将其置位; Local APIC 写 EOI 时清零该位.
* Interrupt Pin Polarity: 指定引脚有效电平是高电平还是低电平，置位为低电平有效，清零时为高电平有效.
* Delivery Status: 传送状态，置位时表示没有中断 IDEL; 清零时表示 IOAPIC 已经收到该中断，但由于某种原因该中断没有发送给 Local APIC, Send Pending.
* Destination Mode: 目的地模式，置位为 Physical Mode; 清零为 Logical Mode.
* Delivery Mode: 传送模式, 用于指定该中断以何种方式发送给目的 APIC，各种模式需要和相应的触发方式配合:
  * 000: Fixed Mode 发送给 Destination Field 列出的所有 CPU.
  * 001: Lowest Priority 发送给 Destination Field 列出的 CPU 中优先级最低的 CPU
  * 010: SMI(System Management Interrupt) 系统管理中断，只能触发 edge 中断
  * 100: NMI(None Mask Interrupt) 不可屏蔽中断，发送给 Destination Filed CPU.
  * 101: INIT 发送给 Destination Filed CPU，LAPIC 收到后执行 INIT 中断
  * 111: ExtINT 发送给 Destination Field CPU, CPU 收到之后认为是一个 IPI 中断.
* Interrupt Vector: 中断向量，指定该中断对应的 Vector，范围从 0x10 到 0xFE.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

###### <span id="E60">vIOAPIC 创建</span>

![](/assets/PDB/HK/TH001806.png)

vIOAPIC 的创建主要分作两部分，第一个部分是在内核空间虚拟一个 vIOAPIC 设备，其二是对默认引脚的模拟. Broiler 在初始化阶段通过调用 kvm_init() 函数，并使用 iotcl 方式向 KVM 传入命令 KVM_CREATE_IRQCHIP 用于创建 vIOAPIC. KVM 的 kvm_arch_vm_ioctl() 函数收到了 Broiler 传递下来的命令，然后找到对应的处理分支 KVM_CREATE_IRQCHIP, 分支里有两个主要的函数分支，其中 kvm_ioapic_init() 函数用于在 KVM 内虚拟一个 vIOAPIC 设备，用于模拟 vIOAPIC 的寄存器读写操作，其通过 MMIO 方式进行访问; 另外一个函数是 kvm_setup_default_irq_routing() 函数，该函数用于对 vIOAPIC 默认引脚的模拟 kvm_set_ioapic_irq.

![](/assets/PDB/HK/TH001809.png)

KVM 使用 struct ioapic 数据结构描述一个 vIOAPIC 设备，base_address 成员指明了 vIOAPIC 内部内存通过 MMIO 映射到系统存储域的地址，ioregsel 成员用于模拟 IOAPIC 的 Index Register; redirtbl 成员用于模拟 IOAPIC 的 PRT Table，每个成员对应一个引脚;irq_states[] 成员用于模拟 IOAPIC 引脚状态，每个引脚对应 irq_states[] 数组的一个成员; dev 成员则用于模拟 vIOAPIC In-Kernel 设备，kvm 成员指向虚拟机对应的 kvm; irq_eoi[] 数组成员用于描述每个中断的 EOI 状态.

![](/assets/PDB/HK/TH001808.png)

KVM 使用 union kvm_ioapic_redirect_entry 联合体描述一个 PRT Entry, vector 成员用于描述 Interrupt Vector 字段; delivery_mode 成员用于模拟 Delivery Mode 字段，dest_mode 成员用于模拟 Destination Mode 字段; delivery_status 成员用于模拟 Delivery Status 字段, polarity 成员用于模拟 Interrupt Pin Polarity 字段; remote_irr 成员用于模拟 Remote IRR 字段，trig_mode 成员用于模拟 Trigger Mode 字段; mask 成员用于模拟 Interrupt Mask 字段，dest_id 用于模拟 Destination Field 字段.

![](/assets/PDB/HK/TH001810.png)

回到 KVM 对 vPIC 设备模拟的调用逻辑，KVM 调用 kvm_ioapic_init() 函数对 vIOAPOC 设备进行模拟，其通过 kvm_iodevice_init() 函数对 vIOAPIC 设备进行模拟，并为设备提供了 IO 端口读写模拟，IO 端口的读模拟通过函数 ioapic_mmio_read() 实现，IO 端口的写模拟通过函数 ioapic_mmio_write() 实现。函数最后通过调用 kvm_io_bus_register_dev() 函数将起始地址为 IOAPIC_DEFAULT_BASE_ADDRESS 长度为 IOAPIC_MEM_LENGTH MMIO 地址映射为 vIOAPIC 寄存器所使用的地址, Guest OS 一旦对这段 MMIO 地址进行读写操作，那么会导致 VM-EXIT 并调用到 ioapic_mmio_ops 提供的 IO 读写模拟函数. 函数通过 kvm_ioapic_reset() 函数对 vIOAPIC 进行了初始化.

![](/assets/PDB/HK/TH001811.png)

根据 IOAPIC 硬件逻辑，ioapic_mmio_read() 函数实现了对 vIOAPIC 寄存器的读模拟，IOAPIC 中 Index Register 和 Data Register 寄存器可以直接通过 MMIO 访问，其他寄存器则需要使用这两个寄存器简介访问. 函数 591-593 行实现了对 Index Register 寄存器的直接读模拟，而 595-597 行模拟 IOAPIC 间接访问内部寄存器的逻辑，其通过 ioapic_read_indirect() 函数实现间接模拟.

![](/assets/PDB/HK/TH001812.png)

根据 IOAPIC 硬件逻辑，ioapic_mmio_write() 函数实现了对 vIOAPIC 寄存器的写模拟，IOAPIC 中的 Index Register 和 Data Register 寄存器可以直接通过 MMIO 访问，其他寄存器需要借助这两个寄存器间接访问. 函数 649-651 行实现了对 Index Register 寄存器的直接写模拟，而 653-655 行模拟 IOAPIC 寄存器访问内部寄存器的逻辑，其通过 ioapic_write_indirect() 函数实现.

![](/assets/PDB/HK/TH001813.png)

vIOAPIC 创建的最后一步是对默认引脚的模拟，通过之前的分析 IOAPIC 一共包含了 24 根引脚。KVM 对引脚的模拟通过调用 kvm_set_default_irq_routing() 函数实现，该函数包含了公共路径代码，对 vIOAPIC 引脚的模拟主要在 setup_routing_entry() 函数中实现，其通过调用函数 kvm_set_routing_entry() 函数将 vIOAPIC 对应的引脚中断模拟函数设置为 kvm_set_ioapic_irq() 函数，另外将模拟好的引脚加入到 hash 链表上。

![](/assets/PDB/HK/TH001780.png)

KVM 使用 struct kvm_irq_routing_table 数据结构维护 vPIC 和 vIOAPIC 的引脚，其中 chip[] 数组为了所有引脚信息，nr_rt_entries 成员则表示引脚的总数，另外使用 map[] 哈希链表维护了引脚和 Chip 的映射关系，即引脚是属于 vPIC 还是 vIOAPIC. KVM 使用 struct kvm_kernel_irq_routing_entry 数据结构描述一个 GSI，GSI 可以对应 vPIC 的一个引脚，也可以描述 vIOAPIC 的一个引脚，其成员 gsi 表示引脚的 GSI 号，set 回调函数则表示引脚的中断模拟，irqchip 成员则表示引脚对应的 CHIP 信息和引脚信息，最后 link 成员用于将模拟的引脚统一维护在 KVM struct kvm_irq_routing_table 数组结构 map[] 哈希链表上.

![](/assets/PDB/HK/TH001781.png)

struct kvm_irq_routing_table 数据架构维护了 vPIC 和 vIOAPIC 引脚与 GSI 的映射关系，通过这个关系可以从设备的引脚推出 GSI 号，而 map[] 哈希链表则维护了 GSI 与引脚的关系，在 KVM 中模拟的 vPIC 和 vIOAPIC 的引脚是可以重叠的，因此一个 GSI 号可能对应 vPIC 或者 vIOAPIC 的引脚，或者同时对应 vPIC 和 vIOAPIC 的引脚，那么使用一个 map[]  hash 链表，并使用 GSI 作为 key, struct kvm_kernel_irq_routing_entry 作为 hash 链表的成员, 最终形成了上图的逻辑结构.

![](/assets/PDB/HK/TH001782.png)

KVM 默认支持的中断设备的引脚如上图，从 ROUTING_ENTRY2 宏的定义可以看出 vPIC 的引脚是 \[0, 15], vIOAPIC 的引脚范围是 \[0, 23]，那么 vPIC 和 vIOAPIC 之间共享了 \[0, 15] 的引脚.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

###### <span id="E61">Broiler vIOAPIC 中断配置</span>

![](/assets/PDB/HK/TH001784.png)

KVM 在创建 vIOAPIC 的时候，已经默认创建了一套 vPIC 和 vIOAPIC 引脚映射逻辑，虽然可以使用，但作为 HypV 软件，Broiler 还是可以自定义映射 vIOAPIC 的引脚与 GSI 映射逻辑。如上图是 Broiler 虚拟的中断引脚连接逻辑，Broielr 规划的中断引脚逻辑是: 主 vPIC 的 8 个引脚独占 GSI0-GSI7, 从 vPIC 的前 4 个引脚独占 GSI8-GIS11, 后 4 个引脚与 vIOAPIC 前 12-15 引脚共享 GSI12-GSI15, vIOAPC 16-24 引脚独占 GSI16-GSI24.

![](/assets/PDB/HK/TH001785.png)

Broiler 在 broiler_irq_init() 函数中对 GSI 进行了重映射，16-20 行用于映射 GSI0-GSI7 给 IRQCHIP_MASTER，33-25 行用于映射 GSI8-GSI15 给 IRQCHIP_SLAVE, 27-29 行用于映射 GSI12-GSI24 个 IRQCHIP_IOAPIC, 可以看出从 vPIC 和 vIOAPIC 共享了 GSI12-GSI15, 最后调用 ioctl 向 KVM 传入 KVM_SET_GSI_ROUTING 使映射生效. 

![](/assets/PDB/HK/TH001814.png)

vIOAPIC 的模拟位于内核是一个 In-Kernel 的设备，当 Broiler 对中断映射有了更改，需要通过 ioctl() 函数传入 KVM_SET_GSI_ROUTING 命令来更新 vIOAPIC 的中断引脚与 GSI 的映射。KVM 的 kvm_vm_ioctl() 函数收到 KVM_SET_GSI_ROUTING 命令之后，调用 kvm_set_irq_routing() 函数进行更新，更新核心通过 kvm_set_routing_entry() 函数，其会将主从 vIOAPIC 的引脚的中断模拟函数设置为 kvm_set_ioapic_irq() 函数.

![](/assets/PDB/HK/TH001786.png)

struct kvm_irq_routing_table 数据架构维护了 vIOAPIC 引脚与 GSI 的映射关系，通过这个关系可以从设备的引脚推出 GSI 号，而 map[] 哈希链表则维护了 GSI 与引脚的关系，在 KVM 中模拟的 vIOAPIC 的引脚是可以重叠的，因此一个 GSI 号可能对应 vPIC 或者 vIOAPIC 的引脚，或者同时对应 vPIC 和 vIOAPIC 的引脚，那么使用一个 map[] hash 链表，并使用 GSI 作为 key, struct kvm_kernel_irq_routing_entry 作为 hash 链表的成员, 映射完毕之后 Broiler 的 vIOAPIC 引脚连接如上.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="E62">Broiler 设备使用 vIOAPIC 中断</span>

![](/assets/PDB/HK/TH001761.png)

Broiler 配置完 vIOAPIC 中断之后，那么接下来就是在设备中使用 vIOAPIC 提供的中断，Broiler 目前支持的设备包括 PCI 设备和 IO 端口设备。对于 IO 端口设备 Broiler 提供了 irq_alloc_from_ioapic() 函数从 vIOAPIC 中分配中断，而 PCI 设备如果使用 INTX 中断的话，Broiler 提供了 pci_assign_ioapic_irq() 函数从 vIOAPIC 中分配中断。对于分配的 vIOAPIC 中断，中断的触发分为电平触发和边缘触发，Broiler 模拟的设备可以使用 broiler_irq_trigger() 函数进行边缘触发。那么接下来以 Broiler 一个 IO 端口设备使用边缘触发方式给 Guest OS 前端驱动发中断的案例进行讲解:

![](/assets/PDB/HK/TH001815.png)

从虚拟机内部看到系统存储域空间中，IOAPIC 寄存器映射的 MMIO 地址为 \[0xFEC00000, 0xFEC003ff]. 接下来在 Broiler 虚拟一个设备，其包含一个异步 IO，对该 IO 进行读操作会出发一个中断(源码位置: foodstuff/Broiler-interrup-vIOAPIC.c)

![](/assets/PDB/HK/TH001816.png)

Broiler 模拟了一个设备，该设备包含了一段 IO 端口，其范围是 \[0x6040, 0x6040], 里面包含了两个寄存器，第一个寄存器 IRQ_NUM_REG 用于获得设置使用的 IRQ，第二个寄存器是一个异步 IO，对该寄存器进行写操作会触发中断。Broiler 在 70 行调用 irq_alloc_from_ioapic() 函数从 vIOAPIC 中分配一个 IRQ。Broiler 在 72-103 行实现一个异步 IO, 如果 Guest OS 对该 IO 端口进行写操作，那么会唤醒 irq_threads 线程，线程的作用是向 Guest OS 注入一个 IRQ 中断，可以看到 45 行调用 broiler_irq_trigger() 函数模拟一次上升沿，进而产生中断，此时 KVM 会收到 ioctl 命令 KVM_IRQ_LINE, 然后 KVM 向 Guest OS 注入中断. 那么有了 Broiler 侧的模拟设备，接下来就是 Guest OS 内部驱动来处理中断，BiscuitOS 已经支持该驱动程序的部署，其部署逻辑如下:

{% highlight bash %}
# 切换到 BiscuitOS 目录
cd BiscuitOS
make linux-5.10-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] KVM --->
          [*] vInterrupt: Broiler vIOAPIC interrupt

# 保存配置并使配置生效
make

# 进入 Broiler 目录
cd output/linux-5.10-x86_64/package/Broiler-vIOAPIC-interrupt-default/
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
![](/assets/PDB/HK/TH001770.png)
![](/assets/PDB/HK/TH001817.png)
![](/assets/PDB/HK/TH001818.png)

> [Broiler-vIOAPIC-interrupt-default Gitee @link](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Virtualization/vInterrupt/Broiler-vIOAPIC-interrupt)

Guest OS 驱动源码比较简单，首先通过 request_resource() 向 IO 空间注册了设备的 IO 端口，其 IO 端口域为 \[0x6040, 0x6050], 驱动接着调用 ioport_map() 函数将 IO 端口映射到虚拟内存，接着在 52 行从设备的 0x6044 端口获得设备使用的中断号，接着在 53 行调用 request_irq() 进行中断处理函数注册，此时中断的触发方式设置为上升沿触发，中断处理函数是 Broiler_irq_handler()，其内部用于在接收到中断之后打印中断号。最后驱动向端口 0x6040 进行写操作，那么这会触发设备向 CPU 发送一个中断，接下来在 BiscuitOS 实践该案例:

![](/assets/PDB/HK/TH001819.png)

Broiler 启动 BiscuitOS 系统之后，加载驱动 Broiler-vIOAPIC-interrupt-default.ko, 可以看到驱动加载成功，等待 5s 之后中断处理程序收到设备发来的中断，此时可以看到 IRQ 为 8，接着查看 IO 空间，可以看到端口 0x6040-0x6050 分配给 "Broiler PIO vIOAPIC" 使用。最后可以查看 /proc/interrupts 节点获得中断映射关系，也就是开篇的图片可以看到 Broiler-PIO-vIOAPIC 使用的中断.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="E63">vIOAPIC 中断注入</span>

![](/assets/PDB/HK/TH001820.png)

vIOAPIC 中断注入流程如上图，分作两大部分，第一部分是 Broiler 通过 ioctl() 函数向 KVM 传入 KVM_IRQ_LINE 命令，以此告诉 KVM 进行 vIOAPIC 中断注入，此时 Broiler 和 KVM 是异步运行的，且 Guest OS 在运行并没有 VM-EXIT. 当 KVM 的 vIOAPIC 通过 PRT Table 对应的 Entry 确认中断要送往的 LAPIC 可以接收中断，设置相关的 PRT Entry 之后 KVM 发送 KVM_REQ_EVENT 请求，并向目标 VCPU 发送 IPI 中断让虚拟机发送 VM-EXIT; 中断注入的第二阶段则是让虚拟机 VM-EXIT 之后再次 VM-ENTRY 时，KVM 会检查 VM_ENTR-Y_INTR_INFO_FIELD 域里是否注入中断，如果注入那么虚拟机 VM-ENTRY 之后就触发中断，因此第二部分的任务就是在 VM-ENTRY 之前向 VM_ENTRY_INTR_INFO_FIELD 写入要注入的中断，KVM 通过在 vmx_inject_irq() 函数中调用 vmcs_write32() 函数向 VM_ENTRY_INT-R_INFO_FIELD 域中写入了要注入的中断，最后虚拟机 VM-ENTRY 之后就收到中断，这个阶段 KVM 是同步运行的. 那么接下来对流程进行细节分析.

![](/assets/PDB/HK/TH001821.png)

ioapic_service() 函数用于解析引脚对应的 PRT Entry，例如在 424 行获得目的 CPU ID，425 行获得 Vector 信息，427 行获得触发模式等。函数在 436-448 行判断 IOAPIC 中断是否为 RTC 的中断，如果不是则调用 kvm_irq_delivery_to_apic() 函数进行中断分发.

![](/assets/PDB/HK/TH001822.png)

kvm_irq_delivery_to_apic() 函数相对复杂，其用于将 IOAPIC 中断送到目的 LAPIC。函数在 55 行调用 kvm_irq_delivery_to_apic_fast() 函数进行快速分发，如果没有分发成功则在 66-89 行进行慢速分发。67 行遍历所有 VCPU 的时候将不支持 vIOAPIC 的 VCPU 过滤掉，然后在 70 行调用 kvm_apic_match_dest() 函数找出目的 VCPU 对应的 LAPIC，接着 74 行调用 kvm_lowest_prio_delivery() 函数判断 IOAPIC 中断是否支持 APIC_DM_LOWEST 方式分发中断，如果不支持，那么函数直接调用 kvm_apic_set_irq() 函数使用非 APIC_DM_LOWEST 方式进行中断分发.

![](/assets/PDB/HK/TH001823.png)

\_\_apic_accept_irq() 函数进行最后的中断分发，由于 IOAPIC 支持多种类型的中断，中断分发大同小异，这里以 APIC_DM_FIXED 为例进行讲解。函数 70-82 行进行用于设置 IOAPIC 需要通知的 VCPU 集合，84-91 行检查中断的状态，如果状态发送改变并且根据触发模式，调用 kvm_lapic_set_vector()/kvm_lapic_clear_vector() 函数设置对应 LAPIC 的 APIC_TMR 寄存器。函数接着在 93-97 行检查中使用 POSTED 方式分发中断，那么函数调用 kvm_lapic_set_irr() 设置指定的 LAPIC 的 IRR 寄存器，以此标记 LAPIC 有中断 pending，最后调用 kvm_make_request() 函数向 KVM 注册 KVM_REQ_EVENT 事件，该事件会让虚拟机 VM-ENTRY 时检测是否用中断 Pending，接着调用 kvm_vcpu_kick() 函数让虚拟机发送 VM-EXIT.

![](/assets/PDB/HK/TH001794.png)

虚拟机再次 VM-ENTRY 之前会调用 vcpu_enter_guest() 函数进行检查，此时函数在 8865 行发现有 KVM_REQ_EVENT 请求，那函数 8873 行调用 inject_pending_event() 函数进行中断注入. 

![](/assets/PDB/HK/TH001795.png)

在 inject_pending_event() 内部，函数在 8310 行通过 kvm_cpu_has_injectable_intr() 函数知道 vIOAPIC 中断需要注入，那么函数进入 8311 分支进行处理，对于 vIOAPIC 中断函数进入 8315 分支进行处理，函数首先调用 kvm_queue_interrupt() 函数获得需要注入中断的 vector，然后调用 vmx_inject_irq() 函数向 VMCS 的 VM_ENTRY_INTR_INFO_FIELD 域注入中断.

![](/assets/PDB/HK/TH001796.png)

vmx_inject_irq() 函数用于最终的中断注入操作，从函数实现可以看到 4500 行获得需要注入的中断号，然后在 4519 行调用 vmcs_write32() 函数向 irq 在 VM_ENTRY_INTR_INFO_FIELD 域中对应的 bit 置位，至此 vIOAPIC 的中断注入完结, 虚拟机 VM-ENTRY 检查到该域有置位，那么虚拟机 RESUME 之后就向虚拟机注入一个中断.

![](/assets/PDB/HK/TH001824.png)

VM_ENTRY_INTR_INFO_FIELD 域是一个 32 位域，Vector 字段用于描述注入的中断向量号，Deliver err Code 域指明是否需要向 Guest 的堆栈中写入错误码，Valid 域用于表示需要向 Guest OS 注入中断，VM-EXIT 时自动清除该域，Interrupt type 指明注入中断的类型，具体支持如下:

* 0: External Interrupt
* 1: Reserved
* 2: NMI
* 3: Hardware exception (e.g. #PF)
* 4: Software interrupt (INT n)
* 5: Privileged software exception (INT 1)
* 6: Software exception (INT 3 or INTO)
* 7: Other event

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="E7"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

#### MSI 中断虚拟化

![](/assets/PDB/HK/TH001826.png)

PCI/PCIe 设备作为系统外设，其也需要通过中断与系统交互。在 PCI 协议规范中将 PCI 设备中断请求投递给 CPU 端的机制称为 **PCI Interrupt Routing**, 在以 Physical/Virtual INTX# Pin 为源头的 PCI Interrupt Routing 称为 **Line-Based PCI Interrupt Routing**, 其包含三个环节:

* PCI Interrupt Pin (INTX#) Selection
* PCI INTX# Pin To PIRQy Mapping
* PIRy To Interrupt Controller IRQz Mapping

在 Intel Chipset 中提供了 8 个 PCI Interrupt Signal PIRQy(y=A-H), 用来连接物理或者虚拟的 INTX# Pin. 对于 External PCI Device, 其 INTX# Pin 物理上直接连接到 Chipset 某个 PIRQy, 对于 Internal/External PCIe Device 已经没有物理的 INTX# Pin，但有虚拟 INTX# Pin 概念，也需要先映射到 PIRQy. 在引入 INTX# Pin 到 PIRQy 设计之后，整个 PCI Interrupt Routing 逻辑多了一层转换，能更好地避免 IRQ 冲突.

![](/assets/PDB/HK/TH001825.png)

在 Line-Based PCI Interrupt Routing 机制中，PCI 设备的中断需要 PIC 或者 IOAPIC 进行转发; 另外每个 PCI Device 包含多个 Function，但每个 PCI Device 只有 INTXA~INTXD 四个 Pin，每个 PCI Function 都可以使用四个 Pin，那么就出现 PIRQ 和 IRQ 被多个 PCI Function 复用，对中断管理比较麻烦，另外无法做到对每个 PCI Function 的 INTX# 进行屏蔽。为了解决以上问题 PCI 规范提出了更为先进的中断机制: **PCI Message-base Interrupt** 机制, 简称 MSI. MSI 的出现解决了 Line-Based Interrupt Routing 的几个问题:

* 无需经过 PIC/IOAPIC 转发中断，直接通过 Memory Write Transaction 向 CPU 发中断
* 每个 PCI Function 可以支持分配多个中断向量，满足同一个设备有多个中断请求的需求
* 当分配多个中断向量给同 1 个 PCI Function 时，提供按中断向量进行屏蔽的功能.

![](/assets/PDB/HK/TH001827.png)

根据 PCI 规范的定义 MSI 中断请求发送时，PCI/PCIe 设备会实际产生一个 Memory Write Transaction, 对应的数据封包为 Memory Write 类型的 Transaction Layer Packet(Posted TLP), 其格式如下. 其中目标 Memory Address 称为 **Message Address**, 要写入该内存地址的数据称为 **Message Data**, 这两个字段来自 MSI Capability Structure 中的设定，具体数值由系统软件在 Enable MSI 过程中必须预设好. 当 PCI/PCIe 设备向 Message Address 写入 Message Data 之后，CPU 就收到 MSI 中断.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------

###### <span id="E71">MSI 中断编程</span>




 



















 











