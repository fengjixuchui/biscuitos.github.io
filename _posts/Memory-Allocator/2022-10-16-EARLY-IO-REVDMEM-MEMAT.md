---
layout: post
title:  "Early IO/RSVD-Memory Memory Allocator"
date:   2022-10-16 12:01:00 +0800
categories: [MMU]
excerpt: Early IO/RESV-MEM Remap.
tags:
  - Boot-Time Allocator
  - Linux 6.0+
  - BiscuitOS DOC 3.0+
  - Fixmap Allocator Sub
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [Early IO/RSVD-MEM 内存分配器原理](#A)
>
> - [Early IO/RSVD-MEM 内存分配器实践](#B)
>
> - [Early IO/RSVD-MEM 内存分配器使用](#C)
>
>   - [内核启动早期访问外设 MMIO](#C1)
>
>   - [内核启动早期访问预留内存](#C2)
>
>   - [内核启动早期以只读方式访问预留内存](#C3)
>
>   - [内核启动早期以特定权限访问预留内存](#C4)
>
>   - [内核启动早期大批量拷贝预留内存](#C5)
>
> - [Early IO/RSVD-MEM API 合集](#D)
>
> - [Early IO/RSVD-MEM 内存世界地图](#F)
>
> - Early IO/RSVD-MEM 应用场景
>
>   - [DMA Remapping/IOMMU 应用场景](#E1)
>
>   - [XHCI USB 主机控制器应用场景](#E2)
>
>   - [Apple AirPort Card 应用场景](#E3)
>
>   - [vSMP Foundation 超级计算机应用场景](#E4)
>
>   - [x2apic 中断控制器应用场景](#E5)
>
>   - [Early-Printk PCISerial 应用场景](#E6)
>
>   - [EFI 固件使用场景](#E7)
>
>   - [INITRD/ACPI 固件使用场景](#E8)
>
>   - [E820 内存管理器使用场景](#E9)
>
> - Early IO/RSVD-MEM 分配器 BUG 合集
>
>   - [对只读映射的 Early RSVD-MEM 进行写操作 BUG](/blog/Memory-ERROR/#A00A06)
>
>   - [Early IO/RSVD-MEM 分配器弃用之后再使用 BUG](/blog/Memory-ERROR/#A00A07)
>
>   - [Early IO/RSVD-MEM 分配器没有可用的 Slot BUG](/blog/Memory-ERROR/#A00A08)
>
>   - [Early IO/RSVD-MEM 分配的内存没有回收 BUG](/blog/Memory-ERROR/#A00A09)
>
>   - [Early IO/RSVD-MEM 分配器映射长度为 0 BUG](/blog/Memory-ERROR/#A00A0A)
>
>   - [Early IO/RSVD-MEM 映射物理区域越界 BUG](/blog/Memory-ERROR/#A00A0B)
>
>   - [Early IO/RSVD-MEM 映射物理区域太大 BUG](/blog/Memory-ERROR/#A00A0C)
>
>   - [Early IO/RSVD-MEM 映射物理区域重复释放 BUG](/blog/Memory-ERROR/#A00A0D)
>
>   - [Early IO/RSVD-MEM 释放错误长度映射区 BUG](/blog/Memory-ERROR/#A00A0E)
>
> - Early IO/RSVD-MEM 分配器进阶研究

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### Early IO/RSVD-MEM 内存分配器原理

![](/assets/PDB/HK/TH001990.png)

**Early IO/RSVD-MEM Memory Allocator** 称为早期 IO/预留内存分配器，其属于**固定映射内存分配器**的一个分支，其目的是内核启动早期需要访问外设寄存器或预留内存，这个时候 IOREMAP 机制还没有建立，于是内核在这个阶段为外设的 MMIO 和预留内存做临时映射，当其任务完毕之后，内核会销毁该分配器, 然后专用功能完备的 IOREMAP 分配器, 那么分配器也属于临时映射分配器.

![](/assets/PDB/HK/TH001481.png)

在 Intel X86 架构中，系统物理地址总线可寻址的空间称为**存储域**，其主要由两部分组成，一部分是 DDR 控制器管理的 **DDR 域**，另外一部分是由外设的寄存器映射到存储域的 **MMIO (Memory-Mapped IO)**。其中 PCI/PCIe 设备的寄存器映射到存储域的 MMIO 称为 **PCI 总线地址**, 另外 Intel X86 架构也通过 Host PCI 主桥/Root Port(RC) 维护一颗 PCI 总线，该 PCI 总线可寻址的空间称为 **PCI 总线域**. **RSVD-MEM** 指定是通过软件的手段让操作系统无法看到的物理内存，**IO** 在本文特指外设映射到存储域的 MMIO.

![](/assets/PDB/HK/TH001695.png)

在 X86 架构中，存储域空间长度和 PCI 总线域长度是相同的，并且 DDR 域的地址可以映射到存储域，也可以映射到 PCI 总线域，同理 PCI 域的地址也可以映射到存储域，且该地址在存储域上称为 PCI 总线地址。

![](/assets/PDB/HK/TH002061.png)

系统要能够访问 MMIO 或者 RSVD-Memroy，都需要分配一段虚拟地址，然后建立从虚拟地址到 MMIO/RSVD-MEM 的页表。但由于系统处于早期，整个内核空间并没有初始化完，因此不是所有的内核空间虚拟内存都可以使用，另外 IOREMAP 分配器要能真正运行起来还需要依赖其他分配器，但这个时候其他分配器也没有初始化准备好，因此内核为了解决系统早期对访问 MMIO/RSVD-MEM 的需求，提出了 Early IO/RESV-MEM 分配器的策略.

![](/assets/PDB/HK/TH002063.png)

**Early IO/RSVD-MEM Memory Allocator** 的实现基于 **Fixmap Memory Allocator**, 分配器维护了 "fix_virt_virt(FIX_BTMAP_BEGIN)" 到 "fix_virt_virt(FIX_BTMAP_END)" 虚拟区域，由于 Fixmap Memory Allocator 的特性，这段虚拟区域是一个固定且预留做 Early IO/RSVD-MEM 分配器使用. 分配器一共包含 "NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS" 个 Index, 每个 Index 维护了 PAGE_SIZE 的虚拟区域, 分配器将这些 Index 按 NR_FIX_BTMAPS 粒度分成了 FIX_BTMAPS_SLOTS 个 Slot，因此每个 Slot 包含了 NR_FIX_BTMAPS 个 Index.

![](/assets/PDB/HK/TH002062.png)

分配器为 IO/RESV-MEM 分配虚拟内存时都是按 SLOT 粒度去分配，那么每次就可以分配到 NR_FIX_BTMAPS * PAGE_SIZE 虚拟区域，分配器接下来根据 MMIO 或者 RSVD-MEM 的长度为 1~64 个 PAGE_SIZE 区域建立页表，那么分配器可以分配 PAGE_SIZE(4KiB)、2\*PAGE_SIZE(8KiB)、3\*PAGE_SIZE(12KiB)、4\*PAGE_SIZE(16KiB) 以及最大 64\*PAGE_SIZE(256KiB) 的虚拟内存区域，此时分配的虚拟区域起始地址为: **fix_to_virt(SLOT_NR * NR_FIX_BTMAPS + FIX_BTMAP_BEGIN)**, 其中 SLOT_NR 为分配器分配的 Slot 号.

-------------------------------------------------

##### 分配器初始化

![](/assets/PDB/HK/TH002064.png)

Early IO/RSVD-MEM 分配器通过 early_ioremap_init() 函数进行初始化，可以看到其初始化时机已经过了系统启动的最早期，这个时候 Permanent Mapping 分配器已经可以工作了，且恒等映射分配器也已经工作了。early_ioremap_setup() 函数对分配器私有的数据进行初始化，early_ioremap_pmd()/pmd_populate_kernel() 函数为分配器维护的虚拟区域分配 PTE 页表页.

![](/assets/PDB/HK/TH002000.png)

内核初始化早期汇编阶段，从汇编代码可以看出 level2_fixmap_pgt 页表中，最后的 (4+FIXMAP_PMD_NUM) PMD 预留给 Permanent Mapping Memory Allocator 和 8MiB Hole. 其余 PMD 作为正常使用，这里也包含了 Early IO/RSVD-MEM 分配器维护的虚拟区域对应的 PMD Entry.

![](/assets/PDB/HK/TH002065.png)

在 Early IO/RSVD-MEM 分配器初始化之前, 分配器维护的虚拟区域的 PGD、PUD 以及 PMD 页表页已经存在，那么分配器需要在初始化阶段为其分配 PTE 页表页。通过分配器维护的虚拟区域可以知道，其一共维护了 NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS * PAGE_SIZE，以此需要 NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS 个 PTE Entry，因此正好需要一个完整的 PTE 页表。

![](/assets/PDB/HK/TH002066.png)

分配器通过 bm_pte[] 数组作为 PTE 页表页，然后 early_ioremap_pmd() 函数通过遍历内核的页表 PMD Entry，然后调用 pmd_populate_kernel() 函数，将 PMD Entry 填入 bm_pte[] 相关的信息，以此构造分配器维护虚拟区域的最后一级页表页.

![](/assets/PDB/HK/TH002067.png)

当分配器的最后一级页表页建立之后，其维护的虚拟区域对应的页表架构如上，如果需要映射 MMIO 或者 RSVD-MEM 时，只需配置最后一级 PTE 页表即可，无需分配页表页.

![](/assets/PDB/HK/TH002068.png)

最后分配器维护了三个私有数据 prev_map[]、prev_size[] 和 slot_virt[]，三个数组都包含 FIX_BTMAP_SLOTS 个成员，它们的作用是当分配器为 MMIO 或者 RESV-MEM 分配一次内存时，prev_map[] 用于标记 SLOT 那些是空闲的，prev_map[\*] 置位表示该 SLOT 已经分配，反之清零则表示空闲; prev_size[\*] 则表示这次分配的长度, slot_virt[\*] 则表示这次分配的虚拟地址. early_ioremap_setup() 函数用于初始化三数组，函数首先确认 prev_map[] 数组中所有成员都清零，以此防止内核在分配器没有初始化之前就被使用，函数将 slot_virt[\*] 各成员设置为 IDX 对应的虚拟地址, 这样可以快速分配虚拟地址. 至此 Early IO/RESV-MEM 分配器初始化完毕.

--------------------------------------------

##### 分配器分配内存

![](/assets/PDB/HK/TH002069.png)

分配器维护了三个核心数组，prev_map[] 数组用于记录 Slot 是否空闲，但成员置位时表示该 Slot 已经分配，反之 Slot 空闲; prev_virt[] 数组成员用于记录每个 Slot 对应的虚拟区域的起始虚拟地址; slot_size[] 数组成员用于记录每个 Slot 映射区域的大小，最小为 4KiB 最大为 256KiB. 当分配器需要分配内存是，分配器首先在 prev_map[] 数组中查找一个空闲 Slot，获得其成员在数组中的偏移 Index 之后，将映射的长度存储在 slot_size[index], 接着根据 (FIX_BTMAP_BEGIN - index * NR_FIX_BTMAPS) 作为索引在 fix_to_virt() 函数获得分配器维护的虚拟地址，然后分配器建立虚拟内存到物理区域的页表，最后将虚拟区域的起始地址存储到 prev_map[index]. 

![](/assets/PDB/HK/TH002070.png)

分配器提供了多种接口，用于满足不同场景的需求。early_ioremap() 函数用于在系统早期分配虚拟内存并映射到 phys_addr 对应的物理区域上，物理区域可以是物理内存、预留物理内存和外设 MMIO，理论上映射到外设 MMIO 更好，这样可以有效利用分配器维护的虚拟内存。early_memremap() 函数用于系统早期映射 phys_addr 指向的预留内存，预留内存指的是系统看不到的物理内存. early_memremap_ro() 函数的作用是在系统早期分配虚拟内存以只读的方式映射预留内存，有的场景需要只读方式读取预留内存中内容. early_memremap_prot() 函数提供了更灵活的映射方式，用于在系统早期分配虚拟内存，并可以灵活使用不同的页表属性映射物理区域，物理区域可以是外设 MMIO 也可以是预留内存。copy_from_early_mem() 函数则提供了系统早期需要从外设或者预留内存拷贝数据的场景，函数分配虚拟内存并映射到物理区域上，并将指定长度的内容从物理区域拷贝到 dest 指向的存储空间.

![](/assets/PDB/HK/TH002071.png)

上面的案例就是一种简单使用 Early IO/RSVD-MEM 分配器的场景，案例中调用 early_ioremap() 函数分配了一段虚拟内存映射到 BROILER_MMIO_BASE 对应的 MMIO 上，并使用 mmio 变量存储虚拟地址，接着程序可以直接使用 mmio 访问外设的 MMIO。当使用完毕之后，再调用 early_iounmap() 函数解除映射.

---------------------------------------

##### 分配器建立页表

![](/assets/PDB/HK/TH002067.png)

在分配器初始化小节处讨论过，分配器初始化完毕之后，分配器维护的虚拟内存对应的 PGD、P5D、P4D、PUD、PMD 页表已经存在，而且分配器使用 bm_pte[] 数组作为 PTE 页表，那么分配器建立页表时，只需填充 PTE Entry 即可.

![](/assets/PDB/HK/TH002072.png)

分配器建立页表的基础入口为 \_\_early_ioremap() 函数, 函数首先在 prev_map[] 数组中找到一个空闲的 Slot，其对应的索引为 idx, 然后将映射内存的长度存储在 prev_size[idx] 中，接着按 4KiB 粒度计算映射页的数量 npages，并将映射的长度存储在 prev_size[idx] 中，接着使用 while 循环 npages 次，每次为 4KiB 的物理区域建立页表，每次循环过程中使用 \_\_early_set_fixmap() 函数建立实际的页表，其通过虚拟地址和 pte_index() 函数获得 PTE Entry 的偏移，然后从 bm_pte[] 数组中找到 PTE Entry，接下来就是调用 set_pte() 函数填充 PTE entry 页表内容，最后调用 flush_tlb_one_kernel() 函数更新 TLB。页表建立完毕之后，最后将映射的虚拟内存更新到 prev_map[idx] 成员，并返回 prev_map[idx] 成员对应的虚拟地址.

![](/assets/PDB/HK/TH002073.png) 

当页表建立完毕之后，可以看到整个过程只需填充最后一级页表的内容，页表页都已经分配，并且最后一级页表就是 bm_pte[]，页表建立完毕之后就可以通过虚拟内存访问预留物理内存或者外设的 MMIO.

![](/assets/PDB/HK/TH002074.png)

分配器支持建立 FIXMAP_PAGE_NORMAL、FIXMAP_PAGE_RO、FIXMAP_PAGE_IO 类型的页表，其中 FIXMAP_PAGE_NORMAL 为 PAGE_KERNEL，即建立一个普通内核页表，其包含了 \_PAGE_PRESENT、\_PAGE_RW、\_PAGE_ACCESSED、\_PAGE_NX、\_PAGE_DIRTY 和 \_PAGE_GLOBAL 标志位; FIXMAP_PAGE_RO 为 PAGE_KERNEL_RO，即建立一个普通只读内核页表，其包含了 \_PAGE_PRESENT、\_PAGE_ACCESSED、\_PAGE_NX、\_PAGE_DIRTY、\_PAGE_ENC 和 \_PAGE_GLOBAL 标志集合; FIXMAP_PAGE_IO 为 PAGE_KERNEL_IO，即建立一个 MMIO 使用的内核页表，其包含了 \_PAGE_PRESENT、\_PAGE_RW、\_PAGE_ACCESSED、\_PAGE_NX、\_PAGE_DIRTY 和 \_PAGE_GLOBAL.

--------------------------------------

##### 分配器虚拟内存使用

![](/assets/PDB/HK/TH002071.png)

上图案例为使用分配器提供的虚拟内存映射了外设的 MMIO，其使用封装好的函数 early_ioremap() 函数进行虚拟内存分配和页表映射，然后返回虚拟地址，程序可以直接操作虚拟地址，以此访问到外设的 MMIO. 不使用虚拟地址时，可以使用 early_memunmap() 函数解除映射并回收虚拟内存.

![](/assets/PDB/HK/TH002075.png)

上图案例为使用分配器提供的虚拟内存映射了预留内存，其使用封装好的函数 early_memremap() 函数进行虚拟地址分配和页表映射，然后返回虚拟地址，程序可以直接操作虚拟地址，以此访问到预留内存。不使用虚拟地址时，可以使用 early_memunmap() 函数解除映射并回收虚拟内存.

![](/assets/PDB/HK/TH002070.png)

分配器提供了多种接口，用于满足不同场景的需求。early_ioremap() 函数用于在系统早期分配虚拟内存并映射到 phys_addr 对应的>物理区域上，物理区域可以是物理内存、预留物理内存和外设 MMIO，理论上映射到外设 MMIO 更好，这样可以有效利用分配器维护的>虚拟内存。early_memremap() 函数用于系统早期映射 phys_addr 指向的预留内存，预留内存指的是系统看不到的物理内存. early_memremap_ro() 函数的作用是在系统早期分配虚拟内存以只读的方式映射预留内存，有的场景需要只读方式读取预留内存中内容. early_memremap_prot() 函数提供了更灵活的映射方式，用于在系统早期分配虚拟内存，并可以灵活使用不同的页表属性映射物理区域，物理区域可以是外设 MMIO 也可以是预留内存。copy_from_early_mem() 函数则提供了系统早期需要从外设或者预留内存拷贝数据的场景，函数分配虚拟内存并映射到物理区域上，并将指定长度的内容从物理区域拷贝到 dest 指向的存储空间.

-------------------------------------

##### 分配器释放内存

![](/assets/PDB/HK/TH002067.png)

通过前面的分析，分配器释放内存是分配内存的反向操作，其核心目的是清除掉虚拟地址对应的最后一级页表的内容即可，然后将 prev_map[] 对应的 Slot 成员清零，这样分配器就实现内存回收.

![](/assets/PDB/HK/TH002076.png)

分配器释放内存的入口为 early_memunmap()/early_iounmap() 函数，函数首先在 prev_map[] 数组中找到需要释放虚拟地址所在 Slot 的索引 idx，然后计算 size 参数需要释放的 PAGE 数量，如果 size 与 prev_size[idx] 不等，那么直接报错，因此分配器需要完整释放 Slot 里面的所有虚拟内存。函数接着使用 while 循环 npages 次，每次循环中，函数根据需要释放的虚拟地址在 bm_pte[] 数组中找到要释放的 pte 项，当找到后调用 pte_clear() 函数清除页表，最后调用 flush_tlb_one_kernel() 函数刷新 TLB。当循环执行完毕之后，Slot 对应的虚拟内存已经完全释放，那么函数将 prev_map[idx] 标记为 NULL，因此说明 Slot 又空闲了.

-------------------------------------

##### 分配器维护虚拟内存大小

![](/assets/PDB/HK/HK000226.png)

Early IO/MEM 内存分配器维护了一段虚拟内存区域，这段区域位于内核空间的末尾，因此其在 Upper 内核页表都属于末尾项。存分配器属于固定映射分配器管理的虚拟内存区域中的一部分，其与永久映射分配器管理的区域相邻.

![](/assets/PDB/HK/TH002077.png)

分配器维护的虚拟区域在 enum fixed_addresses 中占用了 FIX_BTMAP_BEGIN 到 FIX_BTMAP_END 之间的索引，那么其虚拟区域范围是 \[fix_to_virt(FIX_BTMAP_BEGIN), fix_to_virt(FIX_BITMAP_END)] 区域. 其中 FIX_BTMAPS_SLOTS 表示分配器维护的 SLOT 数量，默认设置为 8，NR_FIX_BTMAPS 表示一个 SLOT 中最多映射物理区域的数量，那么分配器维护区域的总长度为 NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS * 4KiB. 默认情况下分配器一共占用了 512 个 INDEX，那么分配器维护的虚拟内存区域为 **2MiB**. FIX_BTMAP_END 定义时，通过与 \_\_end_of_permanent_fixed_addresses 的对齐操作，目的就是让虚拟内存使用的页表独占一个 PMD Entry，确保不与永久映射分配器维护的虚拟区域共用 PMD Entry，这是因为永久映射分配器的 PTE 页表页在编译阶段就存在，而 Early IO/RSVD-MEM 分配器维护的虚拟区域需要在系统启动阶段才建立.

------------------------------------------

##### 分配器生命周期

Early IO/RSVD-MEM 分配器正如名字一样，它只存在于系统早期，当其任务完成之后，系统将正式启动 ioremap 分配器取而代之，那么分配器的生命周期如何? 系统要在启动阶段启用 Early IO/RSVD-MEM 分配器，那么需要保证 CONFIG_GENERIC_EARLY_IOREMAP 宏已经启用，在 X86 架构中该宏始终启用.

![](/assets/PDB/HK/TH002139.png)

在**分配器初始化**章节了解到，分配器在 early_ioremap_init() 函数调用完毕之后，内核就可以使用分配器提供的接口临时映射物理区域. 当系统初始化到后期，分配器会调用 check_early_ioremap_leak() 函数检查分配器还有哪些内存没有回收，如果有就警告对其进行回收, 这之后可以认为分配器**停止使用**, 接着内核运行到 free_initmem() 函数，由于分配器提供的代码都位于 \_\_init section, 那么内核会清除掉 \_\_init section 所有的代码段，那么分配器**彻底抹除**. 当系统继续运行，并将 system_state 设置为 SYSTEM_RUNNING 时，那么 ioremap 分配器已经可以使用，因此可以看做分配器的**轮回新生**.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### Early IO/RSVD-MEM 分配器实践

开发者可以在 BiscuitOS 或者 Broiler 上实践 Early IO/RSVD-MEM 分配器，提供了多个实践案例，具体案例可以参考[Early IO/RSVD-MEM 分配器使用章节](#C). 本节用于介绍如何进行 Early IO/RSVD-MEM 分配的实践，本文以 Linux 6.0 X86_64 架构进行介绍。开发者首先需要搭建 BiscuitOS 或 Broiler 环境，具体参考如下:

> [BiscuitOS Linux 6.0 X86-64 开发环境部署](/blog/Kernel_Build/#Linux_5Y)
>
> [Broiler 开发环境部署](/blog/Broiler/#B)

环境部署完毕之后，接下来参考如下命令在 BiscuitOS 中部署永久映射相关的实践案例:

{% highlight bash %}
cd BiscuitOS
make linux-6.0-x86_64_defconfig
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_ioremap  --->

BiscuitOS/output/linux-6.0-x86_64/package/BiscuitOS-EARLY-IOREMAP-early_ioremap-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_ioremap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_ioremap)

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH002078.png)
![](/assets/PDB/HK/TH002079.png)
![](/assets/PDB/HK/TH002080.png)

程序源码很精简，程序在 22 行调用 early_ioremap() 函数映射 MMIO 地址，映射长度为 BROILER_MMIO_SIZE 4KiB，函数返回映射之后的虚拟地址，并存储在 mmio 中，函数在 28 行通过访问 mmio 变量实现对 MMIO 地址访问. 当任务使用完毕之后，程序在 31 行调用 early_iounmap() 函数解除对 MMIO 的映射. 接下来是编译源码，可以选择在 BiscuitOS 上实践，也可以在 Broiler 上实践，考虑到使用到 Broiler 模拟的硬件，那么这里以 Broiler 实践为准:

{% highlight bash %}
cd BiscuitOS/output/linux-6.0-x86_64/package/BiscuitOS-EARLY-IOREMAP-early_ioremap-default
# 1.1 Only Running on BiscuitOS
  make build
# 1.2 Need Running on Broiler
  make broiler
{% endhighlight %}

由于需要特定硬件的支持，因此可以使用 Broiler 模拟该硬件，那么接下来在 Broiler 上实践，那么运行 1.2 的命令 `make broiler`, 接下来在 Broiler 中实践

![](/assets/PDB/HK/TH002081.png)
![](/assets/PDB/HK/TH002082.png)

可以上上图看到 BiscuitOS 启动过程中，由于 0xD0000000 对应的地址上一块物理内存，那么读到的值是 0。从上图可以看到 Broiler 启动过程中，由于 0xD0000000 有一个虚拟的 PCIe 设备提供，因此程序从 0xD0000000 中读到了 0x1000 的值，这是符合预期的，因此 Early IO/RSVD-MEM 实践成功.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### Early IO/RSVD-MEM 内存分配器使用

Early IO/RSVD-MEM 分配器提供了很多接口函数用于不同的场景，整理如下:

> - [内核启动早期访问外设 MMIO](#C1)
>
> - [内核启动早期访问预留内存](#C2)
>
> - [内核启动早期以只读方式访问预留内存](#C3)
>
> - [内核启动早期以特定权限访问预留内存](#C4)
>
> - [内核启动早期大批量拷贝预留内存](#C5)
>
> - <span id="D">Early IO/RSVD-MEM 分配器 API 合集</span>
>
>   - [copy_from_early_mem](#D5)
>
>   - [early_iounmap](#D6)
>
>   - [early_ioremap](#D1) 
>
>   - [early_memunmap](#D7)
>
>   - [early_memremap](#D2)
>
>   - [early_memremap_prot](#D4)
>
>   - [early_memremap_ro](#D3)

-------------------------------------------

<span id="C1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

##### 内核启动早期访问外设 MMIO

在内核启动早期，ioremap 机制还没有初始化，操作系统需要访问外设 MMIO，那么可以使用 Early IO/RSVD-MEM 分配器实现对外设 MMIO 早期访问, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_ioremap()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_ioremap-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_ioremap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_ioremap)

![](/assets/PDB/HK/TH002083.png)

程序源码很精简，程序在 22 行调用 early_ioremap() 函数将 BROILER_MMIO_BASE 对应的 MMIO 映射到内核虚拟地址空间，这段虚拟地址空间由 Early IO/RSVD-MEM 分配器维护。映射的物理区域为 0xD0000000 到 0xD0001000. 映射完毕之后函数返回虚拟地址并存储在变量 mmio 中，内核可以通过访问 mmio 间接访问到物理区域 BROILER_MMIO_BASE. 但内核使用完毕之后，需要调用 early_iounmap() 函数解除对 BROILER_MMIO_BASE 物理区域的映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000E.jpg)

##### 内核启动早期访问预留内存

预留内存指定是操作系统看不到的物理内存，其可以通过 CMDLINE 的 "memmap=" 字段进行简单设置，就可以将一段可用的物理内存转换成预留物理内存，预留物理内存不再操作系统管理范围之内，厂家一般将固件信息存放在预留内存中以供私有程序使用。如果操作系统想访问预留内存，那么需要建立页表之后进行访问，但在内核启动早期，ioremap 机制并未初始化，那么可以使用 Early IO/RSVD-MEM 分配器实现对预留内存的早期访问, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_memremap()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_memremap-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_memremap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_memremap)

![](/assets/PDB/HK/TH002084.png)

程序源码很精简，程序在 25 行调用 early_memremap() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间，并返回映射之后的虚拟地址，程序可以通过访问 mem 变量间接访问到预留内存。当访问完毕之后，可以调用 early_memunmap() 函数解除映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000W.jpg)

##### 内核启动早期以只读方式访问预留内存

预留内存指定是操作系统看不到的物理内存，其可以通过 CMDLINE 的 "memmap=" 字段进行简单设置，就可以将一段可用的物理内存转换成预留物理内存，预留物理内存不在操作系统管理范围之内，厂家一般将固件信息存放在预留内存中以供私有程序使用。如果操作系统想访问预留内存，那么需要建立页表之后进行访问，但在内核启动早期，ioremap 机制并未初始化，如果操作系统只想与只读的形式访问预留内存，那么可以使用 Early IO/RSVD-MEM 分配器实现对预留内存的早期访问, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_memremap_ro()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_memremap_ro-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_memremap_ro-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_memremap_ro)

![](/assets/PDB/HK/TH002085.png)

程序源码很精简，程序在 25 行调用 early_memremap_ro() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间，并返回映射之后的虚拟地址，程序可以通过访问 mem 变量间接访问到预留内存。当访问完毕之后，可以调用 early_memunmap() 函数解除映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

##### 内核启动早期以特定权限访问预留内存

预留内存指定是操作系统看不到的物理内存，其可以通过 CMDLINE 的 "memmap=" 字段进行简单设置，就可以将一段可用的物理内存转换成预留物理内存，预留物理内存不在操作系统管理范围之内，厂家一般将固件信息存放在预留内存中以供私有程序使用。如果操作系统想访问预留内存，那么需要建立页表之后进行访问，但在内核启动早期，ioremap 机制并未初始化，如果操作系统想以指定模式形式访问预留内存，那么可以使用 Early IO/RSVD-MEM 分配器实现对预留内存的早期访问, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_memremap_prot()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_memremap_prot-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_memremap_prot-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_memremap_prot)

![](/assets/PDB/HK/TH002086.png)

程序源码很精简，程序在 26 行调用 early_memremap_prot() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间, 映射时使用了 \_\_PAGE_KERNEL_NOENC 属性的页表，并返回映射之后的虚拟地址，程序可以通过访问 mem 变量间接访问到预留内存。当访问完毕之后，可以调用 early_memunmap() 函数解除映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C5"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

##### 内核启动早期大批量拷贝预留内存

预留内存指定是操作系统看不到的物理内存，其可以通过 CMDLINE 的 "memmap=" 字段进行简单设置，就可以将一段可用的物理内存转换成预留物理内存，预留物理内存不在操作系统管理范围之内，厂家一般将固件信息存放在预留内存中以供私有程序使用。如果操作系统向从预留内存拷贝大批量的数据，但在内核启动早期，ioremap 机制并未初始化，那么可以使用 Early IO/RSVD-MEM 分配器实现对预留内存的早期访问, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: copy_from_early_mem()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-copy_from_early_mem-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-copy_from_early_mem-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-copy_from_early_mem)

![](/assets/PDB/HK/TH002087.png)

程序源码很精简，程序在 25 行调用 copy_from_early_mem() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间, 并直接将 RESERVED_MEMORY_SIZE 长的数据直接拷贝到 mem 变量里。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="D1">early_ioremap</span>

![](/assets/PDB/HK/TH002088.png)

early_ioremap() 函数用于在内核启动早期映射外设 MMIO 物理区域. 参数 phys_addr 指向物理区域的起始地址，size 参数指明映射的长度, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_ioremap()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_ioremap-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_ioremap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_ioremap)

![](/assets/PDB/HK/TH002083.png)

程序源码很精简，程序在 22 行调用 early_ioremap() 函数将 BROILER_MMIO_BASE 对应的 MMIO 映射到内核虚拟地址空间，这段虚拟地址空间由 Early IO/RSVD-MEM 分配器维护。映射的物理区域为 0xD0000000 到 0xD0001000. 映射完毕之后函数返回虚拟地址并存储在变量 mmio 中，内核可以通过访问 mmio 间接访问到物理区域 BROILER_MMIO_BASE. 但内核使用完毕之后，需要调用 early_iounmap() 函数解除对 BROILER_MMIO_BASE 物理区域的映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="D2">early_memremap</span>

![](/assets/PDB/HK/TH002089.png)

early_memremap() 函数用于在内核启动阶段映射预留内存。参数 phys_addr 指向预留内存的起始地址，size 参数用于说明映射的长度. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_memremap()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_memremap-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_memremap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_memremap)

![](/assets/PDB/HK/TH002084.png)

程序源码很精简，程序在 25 行调用 early_memremap() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间，并返回映射之后的虚拟地址，程序可以通过访问 mem 变量间接访问到预留内存。当访问完毕之后，可以调用 early_memunmap() 函数解除映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="D3">early_memremap_ro</span>

![](/assets/PDB/HK/TH002090.png)

early_memremap_ro() 函数用于以只读方式映射预留内存，参数 phys_addr 指向预留内存的起始地址，size 参数用于说明映射的长度. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_memremap_ro()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_memremap_ro-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_memremap_ro-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_memremap_ro)

![](/assets/PDB/HK/TH002085.png)

程序源码很精简，程序在 25 行调用 early_memremap_ro() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间，并返回映射之后的虚拟地址，程序可以通过访问 mem 变量间接访问到预留内存。当访问完毕之后，可以调用 early_memunmap() 函数解除映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="D4">early_memremap_prot</span>

![](/assets/PDB/HK/TH002091.png)

early_memremap_prot() 函数作用是以指定的页表属性映射预留内存，参数 phys_addr 指向预留内存的起始物理地址，size 参数指明映射预留内存的长度，参数 prot_val 指明使用页表的属性. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_memremap_prot()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_memremap_prot-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_memremap_prot-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_memremap_prot)

![](/assets/PDB/HK/TH002086.png)

程序源码很精简，程序在 26 行调用 early_memremap_prot() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间, 映射时使用了 \_\_PAGE_KERNEL_NOENC 属性的页表，并返回映射之后的虚拟地址，程序可以通过访问 mem 变量间接访问到预留内存。当访问完毕之后，可以调用 early_memunmap() 函数解除映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="D5">copy_from_early_mem</span>

![](/assets/PDB/HK/TH002092.png)

copy_from_early_mem() 函数的作用是从预留内存中拷贝大量的数据，参数 dest 指明拷贝之后的数据存储位置，参数 src 指明预留物理内存起始地址，参数 size 指明了拷贝内存的数量. 其在 BiscuitOS 上部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: copy_from_early_mem()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-copy_from_early_mem-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-copy_from_early_mem-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-copy_from_early_mem)

![](/assets/PDB/HK/TH002087.png)

程序源码很精简，程序在 25 行调用 copy_from_early_mem() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间, 并直接将 RESERVED_MEMORY_SIZE 长的数据直接拷贝到 mem 变量里。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="D6">early_iounmap</span>

![](/assets/PDB/HK/TH002093.png)

early_iounmap() 函数用于在内核启动早期解除对外设 MMIO 物理区域的映射. 参数 addr 指向映射 MMIO 的虚拟地址，size 参数指明映射的长度, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_iounmap()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_iounmap-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_iounmap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_iounmap)

![](/assets/PDB/HK/TH002083.png)

程序源码很精简，程序在 22 行调用 early_ioremap() 函数将 BROILER_MMIO_BASE 对应的 MMIO 映射到内核虚拟地址空间，这段虚拟地址空间由 Early IO/RSVD-MEM 分配器维护。映射的物理区域为 0xD0000000 到 0xD0001000. 映射完毕之后函数返回虚拟地址并存储在变量 mmio 中，内核可以通过访问 mmio 间接访问到物理区域 BROILER_MMIO_BASE. 但内核使用完毕之后，需要调用 early_iounmap() 函数解除对 BROILER_MMIO_BASE 物理区域的映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="D7">early_memunmap</span>

![](/assets/PDB/HK/TH002094.png)

early_memunmap() 函数用于在内核启动阶段解除对预留内存的映射。参数 addr 指向映射预留内存的虚拟地址，size 参数用于说明映射的长度. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] EARLY-IOREMAP Memory Allocator  --->
          [*] EARLY-IOREMAP API: early_memunmap()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-EARLY-IOREMAP-early_memunmap-default
{% endhighlight %}

> [BiscuitOS-EARLY-IOREMAP-early_memunmap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/EARLY-IOREMAP/BiscuitOS-EARLY-IOREMAP-early_memunmap)

![](/assets/PDB/HK/TH002084.png)

程序源码很精简，程序在 25 行调用 early_memremap() 函数将 RESERVED_MEMORY_BASE 起始处，长 RESERVED_MEMORY_SIZE 的预留内存映射到内核地址空间，并返回映射之后的虚拟地址，程序可以通过访问 mem 变量间接访问到预留内存。当访问完毕之后，可以调用 early_memunmap() 函数解除映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------

#### Early IO/RSVD-MEM 应用场景

--------------------------------------

<span id="E1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

##### DMA Remapping/IOMMU 应用场景

众所周知，外设可以访直接访问的内存称为 **DMA(Direct Memory Access)**, DMA 要访问的内存地址称为 DMA 地址或者存储域地址. 在 DMA 技术刚出现的时候，DMA 地址都是物理内存地址，优点就是简单直接，缺点就是不灵活，比如要求物理内存必须是连续的一整块而且不能是高位的地址等，也不能充分满足虚拟机的需求。为了优化这些缺点，引入了 **DMA(DMA Remapping)** 技术, Intel 为支持虚拟机而设计的 IO 虚拟机化技术。外设访问的 DMA 地址不再是物理内存地址，而是要通过 DMA Remapping 硬件进行转义，DMAR 硬件会把 DMA 地址翻译成存储域地址，并检查访问权限等。负责 DMA Remapping 操作的硬件称为 IOMMU。

![](/assets/PDB/HK/TH002095.png)

在 IOMMU/DMAR 初始化过程中，内核还处于系统初始化早期，DMAR 需要将 ACPI acpi_dmar_header 中描述的外设地址 MMIO 映射到内核的虚拟地址空间，此时 ioremap 机制还没有初始化，因此需要 Early IO/RSVD-MEM 分配器提供的内存进行映射。

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时对映射外设 MMIO 的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

##### XHCI USB 主机控制器应用场景

**XHCI (eXtensible Host Controller Interface)** 是由 Intel 开发可扩展的 USB 主机控制器接口，主要面向 USB3.0，同时也支持 USB2.0 以下的设备。USB2.0 主要是 EHCI, USB1.1 则是 OHCI 和 UHCI. XHCI 具有如下特点:

* XHCI 支持所有 USB3.0 特性
* 支持所有 USB 设备
* 为原有的 USB 主控制器存在的问题提供简单稳定的解决方案
* 最优化内存访问效率
* 减少硬件接口的复杂性
* 支持 32bit 和 64bit 模式
* 支持虚拟内存
* 支持虚拟化技术

XHCI 通过使用 PCIe 的 SR-IOV 规范，提供的虚拟机管理器能够启动虚拟 xHCI 控制器，并且可将任何 USB 设备指派到任意 VxHC 实例。XHCI 相关的寄存器映射到存储域上形成 MMIO 区域。

![](/assets/PDB/HK/TH002096.png)

内核初始化早期，需要通过 earlyprink 机制将内核启动早期的 LOG 打印到指定的输出设备，如果此时输出设备为 xdbc, 那么内核会调用 early_xdbc_parse_parameter() 进行初始化，由于 xdbc 是一个 XHCI USB3.0 设备，那么其设备内部寄存器是映射到了存储域的一段 MMIO 区域，此时 ioremap 机制还没有初始化，为了满足需求可以使用 Early IO/RSVD-MEM 分配器进行映射.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时对映射外设 MMIO 的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

##### Apple AirPort Card 应用场景

![](/assets/PDB/HK/TH002097.png)

在 Macbook 笔记本电脑上，苹果提供了新的无线模块 AirPort Card, 采用一种名为 802.11g 的高速无线技术，可以提供比早期 AirPort 快 5 倍的数据传输速度. 该模块也是以 PCIe 设备插入到主板上，因此其 BAR 空间包含了很多内部寄存器，硬件上将 AirePort 内部寄存器映射到了存储域空间并占用一段 MMIO 区域.

![](/assets/PDB/HK/TH002098.png)

内核初始化早期，Macbook 电脑需要启用 AirPort Card 来启用无线网，由于 AirePort Card 是一个 PCIe 设备，其内部的寄存器映射到了存储域，占用一段 MMIO 区域，那么内核需要将虚拟地址映射到 MMIO 之后才能操纵 AirPort Card. 由于还是系统启动早期，ioremap 机制还没有初始化，因此只能通过 early_ioremap() 函数进行映射，因此从上面的流程可以看到在调用 apple_aireport_reset() 函数使用，使用 early_ioremap() 函数映射了 AireProt Card 内部寄存器.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时对映射外设 MMIO 的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

##### vSMP Foundation 超级计算机应用场景

ScaleMP 是高端计算虚拟化领域的领导者，可提供高性能和低总拥有成本。创新的 Versatile SMP™ (vSMP) 体系结构将多个 x86 系统聚合到单个虚拟的 x86 系统中，从而带来行业标准的高端 Symmetric 多处理器(SMP) 计算机。ScaleMP 使用软件取代自定义硬件和组件，可提供革命性的新型计算范式. vSMP Foundation 对多达 16 个 x86 系统进行聚合，以创建具有 4 到 32 个处理器（128核）和高达 4TiB 共享内存的单个系统。

![](/assets/PDB/HK/TH002099.png)

在 ScaleMP 机器初始化阶段，首先需要初始化 vsmp_box PCIe 设备，然后访问其内部寄存器，以此进行管理和初始化的目的，由于其属于 PCIe 设备，并且其内部寄存器已经映射到存储域，并占用一段 MMIO 区域。那么系统为了可以访问 vsmp_box 内部的寄存器，那么需要将虚拟地址映射到其映射的 MMIO 地址上。本来可以使用 ioremap 机制可以映射 MMIO，但由于系统启动早期，ioremap 机制还没有工作。于是可以使用系统在该阶段提供的 Early IO/RSVD-MEM 分配器，其中 early_ioremap() 函数就可以完成对 MMIO 区域的映射任务.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时对映射外设 MMIO 的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E5"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000V.jpg)

##### x2apic 中断控制器应用场景

在 X86 架构中存在 IOAPIC 和 LOCAL APIC 两种中断控制器，其中每个 CPU 都有一个 LOCAL APIC，LOCAL APIC 可以接受的中断源包括:

* 本地相连的 I/O 设备，如直接连在 INT0、INT1 管脚上的设备
* 外部 I/O 设备，这些设备产生的中断经过 I/O APIC，然后在通过 LOCAL APIC 到达处理器
* IPI 中断，处理器间可以使用 IPI 中断对方
* APIC 定时器中断，APIC 上自带定时器，在 OS 中也很常用
* Performance monitoring counter interrupts, 性能监控计数器中断
* 温度传感器中断
* APIC 内部错误中断

LOCAL APIC 可以看做一个独立的硬件，有自己的寄存器，叫做 Local vector table or LVT，其大小为 4KiB，并映射到存储域，每个 Local APIC 的 4KiB 寄存器共同占用一段 4KiB 的 MMIO，可以理解 LVT 为 PER-CPU 粒度的，虽然他们映射 MMIO 地址相同，但每当访问这段 MMIO 地址时，CPU 都会访问到自己 LOCAL APIC 的 4KiB LVT. APIC 实际可以分为三个版本:

* APIC(Early P6)
* xAPIC(Pentium 4 and Xeon)
* x2APIC

![](/assets/PDB/HK/TH002100.png)

在内核启动早期，系统需要临时从 x2APIC 的 UV_LOCAL_MMR_BASE 寄存器中读取值，那么可以将虚拟地址映射到 UV_LOCAL_MMR_BASE MMIO 地址上，由于 ioremap 机制还没有初始化，那么可以使用 Eearly IO/RSVD-MEM 分配器提供的 early_ioremap() 进行映射，临时映射使用完毕之后，再使用 early_iounmap() 函数解除临时映射.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时对映射外设 MMIO 的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E6"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

##### Early-Printk PCISerial 应用场景

在内核启动早期，需要通过 Early-Printk 机制将早期的内核启动日志输出到指定的输出设备，指定输出设备包括 serial、ttyS、pciserial、vga、dbgp、xdbc 以及 xen。其中 pciserial 是基于 PCI 设备的输出设备，那么内核需要访问该设备内部寄存器，才能达到将早期 Log 定向到 PCISerial 设备.

![](/assets/PDB/HK/TH002101.png)

PCISerial 设备内部寄存器映射到存储域，并占用一段 MMIO 地址，内核如果像访问 PCISerial 设备内部寄存器，那么需要将虚拟地址映射到其对应的 MMIO 区域上，但由于 ioremap 机制还没有建立，因此可以使用 Early IO/RSVD-MEM 分配器提供的 early_ioremap() 函数进行临时的映射，如上图 early_serial_base 存储了映射的虚拟地址，程序可以通过访问 early_serial_base 进而访问 PCISerial 内部寄存器.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时对映射外设 MMIO 的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E7"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000N.jpg)

##### EFI 固件使用场景

**UEFI(Unified Extensible Firmware Interface)**，即**统一的可扩展固件接口**，是一种详细描述全新类型接口的标准，是适用于电脑的标准固件接口，旨在代替 BIOS。此标准由 Intel 公司带头组织 UEFI 联盟中的 140 多个技术公司共同创建。UEFI 旨在提高软件互操作性和解决 BIOS 的局限性。作为传统 BIOS 的继任者，UEFI 拥有前辈所不具备的诸多功能，比如图形化界面、多种多样的操作方式、允许植入硬件驱动等等。这些特性让 UEFI 相比于传统 BIOS 更加易用、更加多功能、更加方便。

![](/assets/PDB/HK/TH002102.png)

UEFI 运行时会将一些 firmware 信息存储在内存里，然后将这块内存标记为预留内存，然后待操作系统启动时从预留内存中读取相关信息。预留内存是操作系统不维护的物理内存，如果要访问预留内存，那么需要建立虚拟内存到预留内存的页表，这样才能访问预留内存。但由于内核启动早期 ioremap 机制没有工作，那么内核只能通过 Early IO/RSVD-MEM 分配器提供的 early_memremap() 函数映射 UEFI 的预留内存。正如上图 \_\_efi_memmap_init() 函数确认现在是内核启动早期，那么调用 early_memremap() 映射预留内存，并将映射之后的虚拟地址存储在 map.map 变量里.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时映射预留内存的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E8"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

##### INITRD/ACPI 固件使用场景

**ACPI(Advanced Configuration and Power Interface)** 高级配置和电源接口，是在系统启动阶段由 BIOS/UEFI 收集系统各方面信息并创建的，它大致以树形的组织形式存在系统物理内存中。整个 ACPI 表以 RSDP(Root System Descriptor Pointer Table，根系统描述符指针表) 为入口点，存放了 R(X)DST 的地址。RSDT(根系统说明表) 是32位地址，XSDT 是 64 位地址，其功能一样。对于基于 Legacy BIOS 的系统而言，RSDP 表所在的物理地址并不固定，要么位于 EBDA(Extended BIOS Data Area) 的前1KB范围内, 要么位于 0x000E0000 到 0x000FFFFF 的物理地址范围内.

![](/assets/PDB/HK/TH002103.png)

在存在 INITRD 的系统中，内核启动过程中，内核会从 ACPI 中获得一些信息，并进行处理之后将这些信息更新到 INITRD 固件中，当由于 INITRD 位于预留内存里，因此需要将虚拟地址映射到预留内存之后才能进行信息更新，由于 ioremap 机制并没有工作，因此在这个阶段可以使用 Early IO/RSVD-MEM 分配器提供的 early_memremap() 函数进行映射，映射完毕之后就可以将固件信息更新到 INITRD "kernel/firmware/acpi/" 里，映射完毕之后再调用 early_memunmap() 函数解除对预留内存的映射.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时映射预留内存的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

<span id="E9"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

##### E820 内存管理器使用场景

E820 内存管理器用于管理 E820 表，其通过 BIOS 获得系统物理内存整体布局，并将物理内存划分为可用物理内存、预留内存、PMEM 等多种类型。在系统启动过程中，物理内存被用作不同的目的，有的区域变成预留内存，有的区域还是可用内存，因此整个物理内存会被拆分成很多零散的区域，每个区域都使用 E820 Entry 进行描述。E820 表最多支持 128 E820 Entry，如果区域超过这个限制，那么需要通过 SETUP_E820_EXT 进行 E820 表拓展.

![](/assets/PDB/HK/TH002104.png)

由于存储 E820 表需要通过 boot_params.hdr.setup_data 指定，该值存储的是一个物理地址，那么需要在这个阶段对其进行映射之后才能使用，但由于 memremap 机制没有工作，因此可以使用 Early IO/RSVD-MEM 分配器提供的 early_memremap() 函数进行映射，正如上图内核调用 e820__memory_setup_extended() 函数对 E820 表进行扩展.

**结论**: Early IO/RSVD-MEM 分配器可以满足内核初始化早期临时映射物理内存的需求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------
