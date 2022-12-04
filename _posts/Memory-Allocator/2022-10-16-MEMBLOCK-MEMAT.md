---
layout: post
title:  "MEMBLOCK Physical Memory Allocator"
date:   2022-10-16 12:05:00 +0800
categories: [MMU]
excerpt: Permanent Mapping.
tags:
  - Boot-Time Allocator
  - Linux 6.0+
  - Physical Allocator Sub
  - BiscuitOS DOC 3.0+
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [MEMBLOCK 物理内存分配器原理](#A)
>
> - [MEMBLOCK 物理内存分配器实践](#C)
>
> - [MEMBLOCK 物理内存分配器使用](#B)
>
>   - [MEMBLOCK 分配物理内存](#B0)
>
>   - [MEMBLOCK 回收物理内存](#B1)
>
>   - [MEMBLOCK 新增物理内存](#B2)
>
>   - [MEMBLOCK 遍历物理内存区域](#B3)
>
>   - [MEMBLOCK 获取物理内存信息](#B4)
>
>   - [MEMBLOCK 控制与检测操作](#B7)
>
>   - [MEMBLOCK 分配器系统接口](#B8)
>
> - [MEMBLOCK 物理内存分配器 API 合集](#B5)
>
> - [MEMBLOCK 物理内存分配器使用场景](#X)
>
>   - [Mirror Memory 应用场景](#X0)
>
>   - [系统预留内存应用场景](#X1)
>
>   - [CRASH VMCORE 应用场景](#X2)
>
>   - [Hugetlb 1G 大页应用场景](#X3)
>
> - MEMBLOCK 物理内存分配器 BUG 合集
>
> - MEMBLOCK 物理内存分配器进阶研究
>
>   - [MEMBLOCK 分配器与 Mirror Memory 研究](#T0)
>
>   - [MEMBLOCK 分配器之 phymem Region 研究](#T1)
>
>   - [MEMBLOCK 分配器之 no-map 研究](#T2)
>
>   - [MEMBLOCK 分配器 DRVIER_MANAGED 研究](#T3)
>
>   - [MEMBLOCK 分配器与 Hotplug Memory 研究](#T4)
>
>   - [MEMBLOCK 分配器 Keep Work 研究](#T5)
>
>   - [MEMBLOCK 分配器 Split 与 Merge 研究](#T7)
>
>   - [MEMBLOCK 分配器调试研究](#T8)
>
>   - [MEMBLOCK 分配器 BiscuitOS Doc 2.0+](/blog/MMU-ARM32-MEMBLOCK-index/)
>
>   - [Boot time memory management From Kernel.org](https://www.kernel.org/doc/html/latest/core-api/boot-time-mm.html)
>
> - [MEMBLOCK 分配器讨论 and Q&A](https://github.com/BiscuitOS/BiscuitOS/discussions/32)

######  🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### MEMBLOCK 物理内存分配器原理

![](/assets/PDB/HK/HK000226.png)

在 Linux 内核中通常使用 Buddy 分配器管理系统物理内存，其包括了物理内存的分配与回收。Buddy 分配器在系统初始化到一定阶段之后才开始工作，那么系统早期需要内存找谁申请呢? Linux 早期是否会统一管理物理内存? Linux 早期虚拟内存又是怎么分配? 基于这些问题为给位开发者介绍这次的主角 MEMBLOCK 物理内存分配器.

![](/assets/PDB/HK/TH001990.png)

内核启动早期采用了不同的分配器，以此完成不同的需求. 当系统启动后，需要对早期的 IOMEM 和 预留内存访问，内核提供了 Early IO/RSVD-MEM 分配器; 当内核启动早期需要访问唯一的功能，那么可以使用 Permanent Mapping 分配器; 当内核启动早期需要分配物理内存或虚拟内存，那么可以使用 MEMBLOCK 物理内存分配器.

![](/assets/PDB/HK/TH002140.png)

MEMBLOCK 物理内存分配器维护了早期系统的物理内存，其将物理内存划分成多个长度不同的 Region，Region 是 MEMBLOCK 分配器最小的管理单元. MEMBLOCK 分配器将每个 Region 划分成两大类: **memory 区域**为 MEMBLOCK 分配器管理的可用物理内存，**reseved 区域**为 MEMBLOCK 分配器预留的物理内存.

![](/assets/PDB/HK/TH002147.png)

MEMBLOCK 分配器的 memory 区域为系统可用的物理区域，reserved 区域则为已经分配的物理区域，因此 MEMBLOCK 分配器的**可分配区域**为 memory 区域中剔除 reserved 区域之后的区域, 因此三者的关系如上图.

![](/assets/PDB/HK/TH002141.png)

MEMBLOCK 分配器使用 struct memblock、struct memblock_type 和 struct memblock_reg-ion 三个数据结构体进行描述。struct memblock_region 表示 Region，其描述了物理区域的起始物理地址和长度、所属 NUMA NODE 以及标识信息; struct memblock_type 表示物理内存的类型，其描述某种类型当前 Region 的数量、最大支持 Region 数量、维护总物理内存量、内存类型的名字以及 Region 位置, MEMBLOCK 分配器只支持 memory(可分配物理内存) 和 reserved(已经被使用物理内存) 两种类型; struct memblock 对 MEMBLOCK 分配器的整体抽象，其描述分配器分配的方向、最大可分配物理内存、memory 和 reserved 区域.

![](/assets/PDB/HK/TH002281.png)

MEMBLOCK 分配器包含了 memory 和 reserved 两种区域，每种区域包含了多个物理区域，这些物理区域使用 struct memblock_region 进行维护，MEMBLOCK 分配器的 memory 或 reserved 区域按从地址到高地址的顺序，依次维护在给自的数组里，因此可以正序遍历或者倒序遍历所有区域.

-----------------------------------------------

##### 分配器初始化 

![](/assets/PDB/HK/TH002142.png)

内核在 \_\_initdata_memblock Section 内定义了 MEMBLOCK 分配器管理物理内存所需的空间，这部分内存数据编译时分配(属于静态分配的一种)。内核为 MEMBLOCK 分配器提前分配好了存储 Region 的 memblock_memory_init_regions[] 数组和 memblock_reserved_init_regions[] 数组。MEMBLOCK 分配器默认的分配方向 memblock.bottom_up 为 false，那么 MEMBLOCK 分配器从高地址向低地址分配.

![](/assets/PDB/HK/TH002143.png)

MEMBLOCK 分配器相关的数据通过静态声明方式在编译阶段分配，其核心数据结构存放在 \_\_initdata_memblock Section 内，如果系统不支持 CONFIG_ARCH_KEEP_MEMBLOCK 宏，那么系统初始化完毕 MEMBLOCK 分配器一同被清除掉; 反之如果系统支持 CONFIG_ARCH_KEE-P_MEMBLOCK 宏，那么 MEMBLOCK 分配器在系统正常运行之后还可以继续使用.

![](/assets/PDB/HK/HK000427.png)

在 X86 架构中，内核启动之前 BIOS 会对可用的物理内存进行探测，并将探测到的内存信息存储构造成指定的 BIOS 中断向量，内核正式启动时通过访问指定的 BIOS 中断向量获得物理内存信息。内核获得相关的物理内存信息之后就着手构建 E820 表，E820 表将内存大致分配可用的物理内存和预留物理内存。**可用物理内存**为系统可以管理的物理内存，**预留物理内存**则是系统无法管理的物理内存。随着内核的不断初始化，内核本身会使用一部分物理内存，那么 MEMBLOCK 分配器在初始化时, 将 E820 表的可用物理内存添加到 MEMBLOCK 分配器的 memory 区域，以此表示 MEMBLOCK 分配器所管理的物理内存，内核将已经使用的物理内存通过调用 memblock_reserve() 函数添加到 MEMBLOCK 分配器的 reserved 区域，以此表示 MEMBLOCK 分配器已经分配的物理内存. 那么对于 MEMBLOCK 分配器可以分配的物理内存就是 memory 区域中去除掉 reserved 区域剩下的区域.

![](/assets/PDB/HK/TH002145.png)

内核在初始化过程中，通过调用 memblock_reserved() 函数将多个区域添加到了 MEMBLOCK 分配器的 reserved 区域，那么 MEMBLOCK 分配器认为 reserved 区域的物理内存已经在使用了，因此不会分配这个区域的物理内存给调用者; 内核在 start_kernel -> setup_arch -> e820__memblock_setup() 函数，通过调用 memblock_add() 函数将 E820 表中可用物理内存区域添加到 MEMBLOCK 分配器的 memory 区域，那么 MEMBLOCK 分配器 memory 区域表示其维护的物理内存. 至此 MEMBLOCK 分配器初始化完毕，接下来将对外提供内存分配服务.

---------------------------------------------

##### 分配器分配内存

![](/assets/PDB/HK/TH002147.png)

MEMBLOCK 分配器为了两个区域: **memory 区域**表示 MEMBLOCK 分配器管理的物理内存范围; **reserved 区域**表示 MEMBLOCK 分配器已经分配的物理内存区域。MEMBLOCK 分配器可以提供分配的物理内存则为 memory 中剔除 reserved 之后的区域，如上图中 **Allocatable Area** 为 MEMBLOCK 分配器可以分配物理内存区域, **Allocated Area** 为 MEMBLOCK 分配器已经分配物理内存区域.

![](/assets/PDB/HK/TH002146.png)

MEMBLOCK 分配器分配内存的原理很简单，就是从 FreeArea 中找到一块区域，然后将这块区域加入到 reseved 区域，那么调用者就拥有这块物理内存的使用权限。MEMBLOCK 分配器本身只管理物理内存，但在 X86 架构中，系统从内核空间起始地址开始依次从物理内存起始地址建立页表，一次性将所有可用物理内存都被内核空间虚拟地址映射，那么 MEMBLOCK 可以通过一个简单的公式就可以知道物理内存对应的虚拟地址.

![](/assets/PDB/HK/TH002148.png)

memblock_alloc() 函数是 MEMBLOCK 分配器提供用于分配指定长度的内存，内核通过 \_\_memblock_find_range_top_down() 或者 \_\_memblock_find_range_bottom_up() 函数判断是物理内存顶部向底部，还是从底部向顶部分配物理内存: 例如从顶部向底部查找可用的物理内存，那么系统会调用 for_each_free_mem_range_reverse() 函数倒序遍历 memory 区域中不包含 reserved 区域的物理区域，只要找到满足条件的物理区域，那么系统会调用 memblock_reserve() 函数将找到的区域加入到 reserved 区域，并调用 phys_to_virt() 将物理地址对应的虚拟地址返回给调用者，返回之前系统还会调用 memset() 函数将物理区域内容清空, 以上便是 MEMBLOCK 分配器提供的最基础物理内存分配逻辑.

![](/assets/PDB/HK/TH002149.png) 

MEMBLOCK 分配器不仅要提供基础的物理内存分配能力，还要根据场景的需要提供满足要求的物理内存，例如在 NUMA 机器上，有的请求者需要从指定的 NUMA NODE 上分配物理内存，那么 MEMBLOCK 分配器必须提供相应的接口，针对 NUMA 问题，MEMBLOCK 分配器将 memory 区域按 NUMA NODE 划分成不同的 region, 每个 region 只能属于指定的 NUMA NODE. 当 MEMBLOCK 分配器需要从指定的 NUMA NODE 分配物理内存时，分配器只需从指定 NUMA NODE 的 memory 区域中找到一个空闲的物理区域即可. 具体案例如下:

> [MEMBLOCK 分配器从指定的 NUMA NODE 上分配物理内存]()

![](/assets/PDB/HK/TH002150.png)
 
在有的场景调用者需要分配高地址的内存，有的场景则需要分配低地址的物理内存，MEMBLOCK 分配器因此支持从 Bottom 向 UP 方式分配物理内存，也可以从 UP 向 Bottom 分配物理内存。当需要分配高地址物理内存时，MEMBLOCK 分配器倒序遍历 memory 区域，然后找到第一块不包含 reserved 区域且符合要求的物理区域; 当需要分配器低地址物理内存时，MEMBLOCK 分配器正序遍历 memory 区域，然后找到第一块不包含 reserved 区域且符合要求的物理区域. 具体案例如下:

> [MEMBLOCK 分配器分配高地址/低地址物理内存]()

![](/assets/PDB/HK/TH002151.png)

在有的场景中，调用者期望从某段物理区域中分配内存，MEMBLOCK 分配器因此支持从指定区域分配器物理内存。当请求者指定了期望的物理区域之后，MEMBLOCK 分配器遍历 memory 区域，然后找到期望区域所在的 region，接着找到一块没有 reserved region 的区域，接着与期望区域做交集，交集内的区域就是符合条件的区域，那么 MEMBLOCK 分配器就找到符合要求的物理内存，将符合要求的区域加入到 reserved 区域之后返回给调用者使用.

> [MEMBLOCK 分配器从指定区域分配器物理内存]()

------------------------------------------------

##### 分配内存建立页表

![](/assets/PDB/HK/TH002152.png)

在内核启动早期，MEMBLOCK 分配器还没有工作之前，内核从内核空间起始地址开始与物理内存起始地址开始，一比一建立页表映射，在内核空间形成了一大段连续的映射区域，这里称这段区域为**线性映射区**. 从软件角度来看，软件可以通过一个简单的线性公式在出线性映射区算出虚拟地址对应的物理地址，而无需通过遍历页表的方式. 同理在该区域也可以通过一个简单的线性公式知道物理地址对应的虚拟地址.

![](/assets/PDB/HK/TH002153.png)

线性区的映射由一个一个页表组成，页表可以映射 4KiB 物理区域，也可以映射 2MiB 区域，这取决于开始映射虚拟地址是否按 4KiB 或者 2MiB 对齐。上图为线性区中映射 4KiB 物理页的情况，如果通过查找页表的方式查找虚拟地址对应的物理地址，那么需要依次遍历 4/5 级页表之后才能找到 PTE Entry 里面的物理地址，同理向知道物理地址对应的虚拟地址，那么系统可能需要额外维护一套逆向映射的数据，因此可以看到**线性映射区**的优点，只需通过简单的线性关系就可以找到对方.

![](/assets/PDB/HK/TH002154.png)

当线性映射区映射 4KiB 物理页时，其 PTE 页表的内容如上图。页表的 Present-Bit 置位表示物理页存在; Read/Write-Bit 置位指明映射的物理页可读可写; User/Supervisor-Bit 清零说明映射的虚拟地址只有内核才可以使用，用户空间进程不能使用; Accessed-Bit 和 Dirty-Bit 置位指明物理页已经被访问过和修改过; Global-Bit 置位指明映射可以被所有进程看到; XD-Bit 置位指明物理页里的内容不具有执行的权限; Physical Memory Offset 域存储物理区域起始页帧号.

![](/assets/PDB/HK/TH002155.png)

线性映射区也映射 2MiB 的物理内存，上图为映射 2MiB 物理页的情况. 映射 2MiB 物理页的物理区域必须按 2MiB 对齐，好处就是大大减少 PTE 页表的使用，以及占用更少的 TLB 项. 同映射 4KiB 页表一样，MEMBLOCK 分配器只需通过简单的线性公式就可以获得映射区虚拟地址映射的物理地址，以及物理地址被线性区映射的虚拟地址.

![](/assets/PDB/HK/TH002156.png)

当线性映射区映射 2MiB 物理页时，其 PMD 页表的内容如上图。页表的 Present-Bit 置位表示物理页存在; Read/Write-Bit 置位指明映射的物理页可读可写; User/Supervisor-Bit 清零说明映射的虚拟地址只有内核才可以使用，用户空间进程不能使用; Accessed-Bit 和 Dirty-Bit 置位指明物理页已经被访问过和修改过; PageSize-Bit 指明说明 PMD 页表为最后一级也表; Global-Bit 置位指明映射可以被所有进程看到; XD-Bit 置位指明物理页里的内容不具有执行的权限; Physical Memory Offset 域存储物理区域起始页帧号.

![](/assets/PDB/HK/TH002157.png)

通过对**线性映射区**的分析，MEMBLOCK 分配器在管理并分配物理内存之后，可以使用 \_\_va() 或者 phys_to_virt() 函数将物理地址转换成虚拟地址之后供调用者使用。同理 MEMBLOCK 分配器在获得虚拟地址之后，可以使用 \_\_pa() 或者 virt_to_phys() 函数将虚拟地址转换成物理地址地址. 

![](/assets/PDB/HK/TH002158.png)

例如在上图的案例中，案例通过在 35 行调用 memblock_phys_alloc_range() 函数从 MEM-BLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END 区间里，分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，当分配成功之后物理地址存储在 phys 变量里，然后案例直接在 44 行使用 phys_to_virt() 函数将物理地址转换成了虚拟地址，并在 46-47 行直接使用虚拟内存，最后在 48 行将虚拟地址和物理地址都打印出来。当物理内存使用完毕之后，可以使用 memb-lock_phys_free() 函数对物理内存进行回收. 接下来通过在 BiscuitOS 上运行案例查看运行结果:

![](/assets/PDB/HK/TH002159.png)

当 BiscuitOS 启动过程中，可以看到案例打印了 "Hello BiscuitOS", 然后其对应的虚拟地址为 0xffff888023ffffc0，物理地址为 0x23ffffc0，虚拟地址和物理地址之间正好偏移 PAGE_OFFSET(0xffff888000000000). 经过上面的验证，MEMBLOCK 分配器分配的物理内存和虚拟地址之间可以不用查页表或逆向映射关系，仅仅通过一个线性关系就可以获得两者之间的关系，这大大简化了 MEMBLOCK 分配器分配内存的难度.

---------------------------------------

##### 分配器使用

![](/assets/PDB/HK/TH002160.png)

MEMBLOCK 分配器可以工作分作两个阶段，第一个阶段是可以标记物理内存已经分配，第二个阶段是向外提供物理内存分配和回收的能力. 在分配器初始化章节可以知道内核启动之后，进入保护模式之后内核就有能力调用 memblock_reserve() 函数，那么此时已经进入 MEMBLOCK 分配器工作的第一阶段; 但只有内核将 E820 表维护的可用物理内存信息通过 memblock_add() 函数添加到 MEMBLOCK 分配器的 memory 区域之后，MEMBLOCK 分配器才可以向外提供物理内存分配和回收能力，此时 MEMBLOCK 分配器才完全工作.

![](/assets/PDB/HK/TH002161.png)

在内核启动阶段，内核使用了某些物理区域，那么 MEMBLOCK 分配器使用 reserved 区域管理已经使用的物理区域，其提供了 memblock_reserve() 函数用于将某段物理区域标记为已分配。同理要将某段物理区从 reserved 区域中移除，可以使用 memblock_remove() 函数.

![](/assets/PDB/HK/TH002162.png)

MEMBLOCK 分配器完整工作之后，可以使用 memblock_alloc() 函数分配物理内存，由于分配的物理内存属于线性映射区的物理内存，那么可以使用简单的线性公式获得对应的虚拟地址，因此调用者获得物理内存之后，将物理地址转换成虚拟地址之后就可以直接使用内存. 当调用者使用完物理内存之后，MEMBLOCK 分配器可以回收物理内存, 此时调用 memblock_phys_free() 函数等.

![](/assets/PDB/HK/TH002163.png)

MEMBLOCK 分配器提供了不同粒度的物理内存分配接口，第一类为从指定物理区域内分配物理内存，接口可以返回物理地址或虚拟地址; 第二类用于 NUMA 场景，从指定的 NUMA NODE 中分配物理内存，接口可以返回物理地址或虚拟地址;第三类无条件的分配物理内存，接口可以返回物理地址或虚拟地址.

![](/assets/PDB/HK/TH002164.png)

MEMBLOCK 分配器也提供了一些列接口用于遍历 memory/reserved 区域，memblock.m-emory 表示可用物理内存区域，memblock.reserved 表示已经分配的物理区域，memblock.mem-ory 中移除 memblock.reserved 区域之后剩下的就是可分配的物理区域，因此 MEMBLOCK 分配器也提供了接口直接遍历可分配的区域.

![](/assets/PDB/HK/TH002165.png)

MEMBLOCK 分配器提供了不同粒度的遍历 memory/reserved 区域的接口，第一类遍历指定的 memory 区域或者 reserved 区域; 第二类无差别的遍历区域，并可以遍历两个区域中不重叠的部分; 第三类则为第二类的倒序遍历.

![](/assets/PDB/HK/TH002166.png)

MEMBLOCK 分配器还提供了一些用于检查物理内存的接口，这些接口可以让调用者快速获得某些物理区域的映射信息、是否被使用、是否与某些区域重叠等. 例如上图中的 memblock_is_map_memory() 函数用于检查物理内存是否已经被虚拟地址映射.

![](/assets/PDB/HK/TH002167.png)

MEMBLOCK 分配器还是提供了比较丰富的接口来获得指定的信息，例如可用物理内存的总量，以及 memblock.reserved 的总量. 另外一类就是检查物理内存或者 Region 的属性接口，最后一类就是物理地址起始和结束的接口. 至此 MEMBLOCK 分配器的使用结束结束，更多使用细节请参考:

> [MEMBLOCK 物理内存分配器使用](#B)

-------------------------------------------

##### 分配器回收内存

![](/assets/PDB/HK/TH002168.png)

MEMBLOCK 分配器支持物理内存的回收，但其仅仅内部回收不做页表的清除. MEMBLOCK 分配器当收到请求者回收物理内存的请求之后，分配器先从 memblock.reserved 区域中找到与需要回收区域重叠的 region，然后 MEMBLOCK 分配器将重叠区域从 memblock.reserved 区域中移除，有的区域可能被拆分成一个或者两个，那么 memblock.reserved 区域 region 的数量可能会增加. 至此 MEMBLOCK 分配器回收物理内存完毕.

![](/assets/PDB/HK/TH002169.png)

memblock_free() 函数是 MEMBLOCK 分配器提供的核心回收物理内存函数，其接受回收物理内存对应的虚拟地址，memblock_alloc() 函数调用 memblock_phys_free() 函数时，通过调用 \_\_pa() 函数虚拟地址转换成物理地址，然后调用 memblock_remove_range() 函数，函数有两个目的，其一找出需要回收的物理内存在 memblock.reserved 区域中占用的 region 范围，然后将调用 memblock_remove_regions() 将涉及的区域从 memblock.reserved 中移除.

![](/assets/PDB/HK/TH002170.png)

案例介绍了 MEMBLOCK 分配器回收物理内存的过程，案例首先在 21 行调用 memblock_alloc() 函数分配指定长度的物理内存，函数同时将物理内存对应的虚拟地址返回给调用者，案例接着在 27-28 行使用了内存，最后在 31 行调用 memblock_free() 函数将内存进行回收，至此一次完整的 MEMBLOCK 分配器内存分配、使用和回收完结.

-------------------------------------------

##### 分配器与 Buddy 分配器

![](/assets/PDB/HK/TH002353.png)

内核启动到一定阶段，MEMBLOCK 分配器完成早期物理内存分配的任务之后，接下来需要将其管理的物理内存移交给 Buddy 分配器进行管理，这也是 Buddy 分配器获得物理页的起源。代码流程大概如上，比较核心的点是内核启动到 mm_init() 函数时，调用 memblock_free_all() 函数来完成核心任务，包括如下:

* 将每个 pgdate 的每个 Zone 管理物理页数 managed_pages 清零
* 遍历 memblock.reserved 区域将其对应的物理页标记为 PageReserved
* 遍历 MEMBLOCK_NOMAP 区域将其对应的物理页标记为 PageReserved
* 将 MEMBLOCK_HOTPLUG 区域修改为 MEMBLOCK_NONE
* 遍历 memblock.memory 区域将 MEMBLOCK_NONE 对应的物理页添加到 Buddy
* 更新 _totalram_pages 表示 Buddy 分配器起始物理页数量

![](/assets/PDB/HK/TH002354.png)

MEMBLOCK 分配器 memblock.reserved 区域对应的物理页全部转换成 PageReserved, 而 memblock.memory 区域中的 MEMBLOCK_HOTPLUG 和 MEMBLOCK_NONE Region 作为 Buddy 分配器管理的物理页; memblock.memory 区域的 MEMBLOCK_NOMAP Region 对应的物理页转换成 PageReserved; memblock.memory 区域的 MEMBLOCK_DRIVER_MANAGED Region 对应的物理页则由驱动探测和加载，并由驱动进行管理维护.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### MEMBLOCK 物理内存分配器实践

开发者可以在 BiscuitOS 或者 Broiler 上实践 MEMBLOCK 物理内存分配器，提供了多个实践案例，具体案例可以参考[MEMBLOCK 分配器使用章节](#B). 本节用于介绍如何进行 MEMBLOCK 物理内存分配的实践，本文以 Linux 6.0 X86_64 架构进行介绍。开发者首先需要搭建 BiscuitOS 或 Broiler 环境，具体参考如下:

> [BiscuitOS Linux 6.0 X86-64 开发环境部署](/blog/Kernel_Build/#Linux_5Y)
>
> [Broiler 开发环境部署](/blog/Broiler/#B)

环境部署完毕之后，接下来参考如下命令在 BiscuitOS 中部署永久映射相关的实践案例:

{% highlight bash %}
cd BiscuitOS
make linux-6.0-x86_64_defconfig
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc()  --->

BiscuitOS/output/linux-6.0-x86_64/package/BiscuitOS-MEMBLOCK-memblock_alloc-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc)

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH002171.png)
![](/assets/PDB/HK/TH002172.png)
![](/assets/PDB/HK/TH002173.png)

程序源码很精简，程序在 21 行调用 memblock_alloc() 函数分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，分配成功之后函数返回了物理内存对应的虚拟地址，并存储在 mem 变量里，接着在 27-28 行通过 mem 变量使用了分配的物理内存，最后物理内存使用完毕之后在 31 行调用 memblock_free() 函数将分配的物理内存进行回收. 接下来是编译源码，可以选择 BiscuitOS 上实践，也可以在 Broiler 上实践，考虑到不涉及外设，那么以 BiscuitOS 实践进行讲解:

{% highlight bash %}
cd BiscuitOS/output/linux-6.0-x86_64/package/BiscuitOS-MEMBLOCK-memblock_alloc-default
# 1.1 Only Running on BiscuitOS
  make build
# 1.2 Need Running on Broiler
  make broiler
{% endhighlight %}

![](/assets/PDB/HK/TH002174.png)

可以从上图看到 BiscuitOS 启动过程中，案例程序被调用并使用 MEMBLOCK 分配器的内存打印了字符串 "==== Hello BiscuitOS ====". 以上便是 MEMBLOCK 分配器最小实践.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000U.jpg)

#### MEMBLOCK 物理内存分配器使用 

##### <span id="B0">MEMBLOCK 分配物理内存</span>

MEMBLOCK 分配器提供了很多接口满足内核不同粒度和场景的物理内存分配，有的接口可以返回物理地址也可以返回虚拟地址，有的可以从指定的 NUMA NODE 或者物理区域内分配物理内存，归结如下:

> - [标准接口: 分配指定长度的物理内存\[VA\]](#B59)
>
> - [标准接口: 保证可以分配到物理内存\[VA\]](#B5F)
>
> - [精准接口: 可指定范围/NUMA NODE/属性的分配物理物理内存\[VA\]](#B5A)
>
> - [特殊接口: 从某个地址之后分配物理内存\[VA\]](#B5B)
>
> - [特殊接口: 从低端分配物理内存\[VA\]](#B5C)
>
> - [特殊接口: 从指定 NUMA NODE 上分配物理内存\[VA\]](#B5D)
>
> - [安全接口: 优先从某个 NUMA NODE 上分配物理内存\[VA\]](#B5E)
>
> - [安全接口: 尽量从某个 NUMA NODE 上分配物理内存\[VA\]](#B5G)
>
> - [物理接口: 直接分配物理内存\[PA\]](#B61)
>
> - [物理接口: 直接从某个范围分配物理内存\[PA\]](#B5X)

##### <span id="B1">MEMBLOCK 回收物理内存</span>

MEMBLOCK 分配器提供了多个接口用于回收 MEMBLOCK 分配器分配出去的物理内存，对于不是分配器分配出去的物理内存，其也提供了多个接口用于移除这类物理区域:

> - [标准接口: 回收分配物理内存对应的虚拟地址](#B5P)
>
> - [物理接口: 直接回收物理内存](#B5Y)
>
> - [特殊接口: 移除一段可用物理内存区域](#B5Z)

##### <span id="B2">MEMBLOCK 新增物理内存</span>

MEMBLOCK 分配器提供了多个接口用于新增物理区域，新增的物理区域可以插入到 memblock.memory 区域或者 memblock.reserved 区域, 归结如下:

> - [标准接口: 新增一段预留物理区域](#B60)
>
> - [标准接口: 新增一段可用物理内存](#B57)
>
> - [标准接口: 向指定 NUMA NODE 新增一段物理内存](#B58)

##### <span id="B3">MEMBLOCK 遍历物理内存</span>

MEMBLOCK 分配器提供了丰富的遍历 memblock.memory 区域和 memblock.reserved 区域的接口，可以单独遍历，也可以按特定需求遍历，既可以正序遍历，也可以倒序遍历。另外还提供了遍历可分配区域的接口，归结如下:

> - [标准接口: 正序遍历 memblock.memory 可用物理内存区域](#B52)
>
> - [标准接口: 倒序遍历 memblock.memory 可用物理内存区域](#B53)
>
> - [标准接口: 正序遍历 memblock.reserved 预留物理内存区域](#B55)
>
> - [标准接口: 正序遍历 MEMBLOCK 分配器可分配物理内存](#B62)
>
> - [标准接口: 倒序遍历 MEMBLOCK 分配器可分配物理内存](#B63)
>
> - [特殊接口: 按 Region 粒度遍历 memblock.memory 可用物理内存区域](#B54)
>
> - [特殊接口: 按 Region 粒度遍历 memblock.reserved 预留物理内存区域](#B56)
>
> - [基础接口: 正序遍历 MEMBLOCK 指定的 Region](#B50)
>
> - [基础接口: 倒序遍历 MEMBLOCK 指定的 Region](#B51)

MEMBLOCK 分配器由于在系统启动阶段统一管理了所有可用的物理内存，因此可以通过 MEMBLOCK 分配器获得系统物理内存相关信息，归结如下:

##### <span id="B4">MEMBLOCK 获取物理内存信息</span>

> - [获得系统 DRAM 起始地址](#B5K)
>
> - [获得系统 DRAM 结束地址](#B5J)
>
> - [获得系统可用物理内存数量](#B5M)
>
> - [获得当前系统预留物理内存数量](#B5L)
>
> - [获得 MEMBLOCK 分配器最大可分配物理地址](#B5Q)

##### <span id="B7">MEMBLOCK 控制与检测操作</span>

MEMBLOCK 分配器提供了多个控制接口用于满足不同的分配需求，另外还提供了很多检测接口，可以便捷检测物理地址的属性，归结如下:

> - [设置 MEMBLOCK 分配器最大可分配物理地址](#B5R)
>
> - [获得 MEMBLOCK 分配器分配方向](#B5H)
>
> - [设置 MEMBLOCK 分配器分配方向]()
>
> - [判断物理地址是否属于可用物理内存](#B5S)
>
> - [判断物理区域是否属于可用物理内存](#B5V)
>
> - [判断物理内存区域是否已经被虚拟地址映射](#B5N)
>
> - [判断物理地址是否属于预留内存](#B5T)

MEMBLOCK 分配器的内部包含多个函数，其中可以提供给外部使用的接口归结如下:

> <span id="B5"></span>
> - [for_each_free_mem_range](#B62)
>
> - [for_each_free_mem_range_reverse](#B63)
>
> - [\_\_for_each_mem_range](#B50)
>
> - [\_\_for_each_mem_range_rev](#B51)
>
> - [for_each_mem_range](#B52)
>
> - [for_each_mem_range_rev](#B53)
>
> - [for_each_mem_region](#B54)
>
> - [for_each_reserved_mem_range](#B55)
>
> - [for_each_reserved_mem_region](#B56)
>
> - [memblock_add](#B57)
>
> - [memblock_add_node](#B58)
>
> - [memblock_alloc](#B59)
>
> - [memblock_alloc_exact_nid_raw](#B5A)
>
> - [memblock_alloc_from](#B5B)
>
> - [memblock_alloc_low](#B5C)
>
> - [memblock_alloc_node](#B5D)
>
> - [memblock_alloc_range_nid](#B5E)
>
> - [memblock_alloc_raw](#B5F)
>
> - [memblock_alloc_try_nid](#B5G)
>
> - [memblock_bottom_up](#B5H)
>
> - [memblock_end_of_DRAM](#B5J)
>
> - [memblock_free](#B5P)
>
> - [memblock_get_current_limit](#B5Q)
>
> - [memblock_is_map_memory](#B5N)
>
> - [memblock_is_memory](#B5S)
>
> - [memblock_is_region_reserved](#B5U)
>
> - [memblock_is_region_memory](#B5V)
>
> - [memblock_is_reserved](#B5T)
>
> - [memblock_overlaps_region](#B5W)
>
> - [memblock_phys_alloc](#B61)
>
> - [memblock_phys_alloc_range](#B5X)
>
> - [memblock_phys_free](#B5Y)
>
> - [memblock_phys_mem_size](#B5M)
>
> - [memblock_reserved_size](#B5L)
>
> - [memblock_remove](#B5Z)
>
> - [memblock_reserve](#B60)
>
> - [memblock_set_current_limit](#B5R)
>
> - [memblock_start_of_DRAM](#B5K)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

<span id="B8"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

##### MEMBLOCK 分配器系统接口

内核提供了多个系统接口来获得 MEMBLOCK 分配器的信息，本节就详细介绍相关的接口和使用说明:

###### 固件映射接口

![](/assets/PDB/HK/TH002343.png)

内核提供了 /sys/firmware/memmap 接口，用于获得从固件 E820 表和 MEMBLOCK 分配器获得系统可用物理内存和系统预留内存的信息，其按 memblock.memory 区域进行划分，每个节点包含了 start、end 和 type 属性，可以从中获得每个 Region 的范围和类型. 当不是所有的 memblock.memory Region 都维护在该接口里，例如 MEMBLOCK_DRVIER_MANAGED 就不维护在这里.

###### MEMBLOCK 调试接口

![](/assets/PDB/HK/TH002344.png)
 
当内核启用 CONFIG_DEBUG_FS 和 CONFIG_ARCH_KEEP_MEMBLOCK 两个宏之后，MEMBLOCK 分配器可以一直存在，那么可以在 /sys/kernel/debug/memeblock 节点下获得 MEMBLOCK 分配器的信息，其中 memory 节点代表 memblock.memory 区域信息，physmem 节点代表 physmem 区域信息，reserved 节点代表 memblock.reserved 区域信息。信息按 Region 的顺序、起始物理地址和结束物理地址组成.

###### 系统物理地址总线接口

![](/assets/PDB/HK/TH002345.png)

内核提供的 /proc/iomem 接口可以获得系统物理地址总线上的信息，其中包括可系统可用的物理内存 "System RAM", 以及系统预留区域，这些预留区域可能是物理内存。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

###### <span id="B50">\_\_for_each_mem_range</span>

![](/assets/PDB/HK/TH002175.png)

\_\_for_each_mem_range() 函数用于按需求遍历 MEMBLOCK 的区域，其可以满足以下几种需求的遍历:

* memblock.memory 区域或 memblock.reserved 区域的单独遍历所有 Region.
* 遍历 memblock.memory 区域指定 NUMA NODE 上的所有 Region.
* 遍历 memblock.memory 区域中移除 memblock.reserved 区域之后的所有 Region.
* 遍历包含 MEMBLOCK_HOTPLUG 标志的 Region.
* 遍历包含 MEMBLOCK_MIRROR 标志的 Region.
* 遍历包含 MEMBLOCK_NOMAP 标志的 Region.
* MEMBLOCK_NONE 无差别的遍历 Region.

参数 i 用于记录遍历 Region 的顺序，type_a 参数指向需要遍历的区域，type_b 参数指向需要从 type_a 区域中移除的区域，参数 nid 指明需要遍历的 NUMA NODE, 参数 p_start 记录每次遍历到区域的起始物理地址，参数 p_end 记录每次遍历到区域的结束物理地址，参数 p_nid 则记录每次遍历到区域所在 NUMA NODE. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: __for_each_mem_range()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-\_\_for_each_mem_range-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-\_\_for_each_mem_range-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-__for_each_mem_range)

![](/assets/PDB/HK/TH002176.png)

程序源码很精简，程序首先在 26-27 构造了两个指定地址的区域，以便方便调试观察，第一个区域 MEMBLOCK_FAKE_MEMORY 添加到 memblock.memory 区域里，第二个区域 MEMBLOCK_FAKE_RESERVE 添加到 memblock.reserved 区域里。接着在 30-35 行调用 \_\_for_each_mem_range() 函数遍历 memblock.memory MEMBLOCK_FAKE_NODE NODE 上所有的区域，memblock.memory 在其他 NUMA NODE 上的区域不被遍历; 案例接下来在 37-43 行调用 \_\_for_each_mem_range() 函数遍历 memblock.reseved 所有区域; 案例最后在 45-52 行调用 调用 \_\_for_each_mem_range() 函数遍历 MEMBLOCK_FAKE_NODE 里包含 memblock.memory 区域但不包含 memblock.reseved 区域的物理区域. 遍历完之后函数在 55-56 行释放 MEMBLOCK_FAKE_MEMORY 和 MEMBLOCK_FAKE_RESERVE 区域. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002177.png)

从 BiscuitOS 实践结果来看，第一次遍历了 NUMA NODE 0 上的所有物理区域，第二次遍历了所有 memblock.reserved 预留的区域，第三次则遍历了所谓的空闲物理内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

###### <span id="B51">\_\_for_each_mem_range_rev</span>

![](/assets/PDB/HK/TH002178.png)

\_\_for_each_mem_range_rev() 函数用于按需求倒序遍历 MEMBLOCK 的区域，其可以满足以下几种需求的遍历:

* memblock.memory 区域或 memblock.reserved 区域的单独倒序遍历所有 Region.
* 倒序遍历 memblock.memory 区域指定 NUMA NODE 上的所有 Region.
* 倒序遍历 memblock.memory 区域中移除 memblock.reserved 区域之后的所有 Region.
* 倒序遍历包含 MEMBLOCK_HOTPLUG 标志的 Region.
* 倒序遍历包含 MEMBLOCK_MIRROR 标志的 Region.
* 倒序遍历包含 MEMBLOCK_NOMAP 标志的 Region.
* MEMBLOCK_NONE 无差别的倒序遍历 Region.

参数 i 用于记录遍历 Region 的顺序，type_a 参数指向需要遍历的区域，type_b 参数指向需要从 type_a 区域中移除的区域，参数 nid 指明需要遍历的 NUMA NODE, 参数 p_start 记录每次遍历到区域的起始物理地址，参数 p_end 记录每次遍历到区域的结束物理地址，参数 p_nid 则记录每次遍历到区域所在 NUMA NODE. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: __for_each_mem_range_rev()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-\_\_for_each_mem_range_rev-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-\_\_for_each_mem_range_rev-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-__for_each_mem_range_rev)

![](/assets/PDB/HK/TH002179.png)

程序源码很精简，程序首先在 26-27 构造了两个指定地址的区域，以便方便调试观察，第一个区域 MEMBLOCK_FAKE_MEMORY 添加到 memblock.memory 区域里，第二个区域 MEMBLOCK_FAKE_RESERVE 添加到 memblock.reserved 区域里。接着在 30-35 行调用 \_\_for_each_mem_range_rev() 函数倒序遍历 memblock.memory MEMBLOCK_FAKE_NODE NODE 上所有的区域，memblock.memory 在其他 NUMA NODE 上的区域不被遍历; 案例接下来在 37-43 行调用 \_\_for_each_mem_range_rev() 函数遍历 MEMBLOCK_FAKE_NODE 里包含 memblock.reserved 区域但不包含 memblock.memory 区域的物理区域. 遍历完之后函数在 45-46 行释放 MEMBLOCK_FAKE_MEMORY 和 MEMBLOCK_FAKE_RESERVE 区域. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002180.png)

从 BiscuitOS 实践结果来看，第一次倒序遍历了 NUMA NODE 0 上的所有物理区域，第二次倒序遍历了所有 memblock.reserved 区域剔除掉 memblock.memory 区域之后的区域.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

###### <span id="B52">for_each_mem_range</span>

![](/assets/PDB/HK/TH002182.png)

for_each_mem_range() 函数用于按需求遍历 MEMBLOCK 的 memory 区域，其可以满足以下几种需求的遍历:

* memblock.memory 区域单独遍历所有 Region.
* MEMBLOCK_NONE 无差别的遍历 memblock.memory 所有 Region.

参数 i 用于记录遍历 Region 的顺序，参数 p_start 记录每次遍历到区域的起始物理地址，参数 p_end 记录每次遍历到区域的结束物理地址. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: for_each_mem_range()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-for_each_mem_range-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-for_each_mem_range-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-for_each_mem_range)

![](/assets/PDB/HK/TH002181.png)

程序源码很精简，程序首先在 24-25 构造了两个指定地址的区域，以便方便调试观察，第一个区域 MEMBLOCK_FAKE_MEMORY 添加到 memblock.memory 区域里，第二个区域 MEMBLOCK_FAKE_RESERVE 添加到 memblock.reserved 区域里。接着在 27-30 行调用 for_each_mem_range() 函数遍历 memblock.memory 上所有的区域. 遍历完之后函数在 33-34 行释放 MEMBLOCK_FAKE_MEMORY 和 MEMBLOCK_FAKE_RESERVE 区域. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002183.png)

从 BiscuitOS 实践结果来看，正序遍历了 memblock.memory 区域里所有的 Region. 

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B53">for_each_mem_range_rev</span>

![](/assets/PDB/HK/TH002184.png)

for_each_mem_range_rev() 函数用于按需求倒序遍历 MEMBLOCK 的 memory 区域，其可以满足以下几种需求的遍历:

* memblock.memory 区域单独倒序遍历所有 Region.
* MEMBLOCK_NONE 无差别的倒序遍历 memblock.memory 所有 Region.

参数 i 用于记录遍历 Region 的顺序，参数 p_start 记录每次遍历到区域的起始物理地址，参数 p_end 记录每次遍历到区域的结束物理地址. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: for_each_mem_range_rev()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-for_each_mem_range_rev-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-for_each_mem_range_rev-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-for_each_mem_range_rev)

![](/assets/PDB/HK/TH002185.png)

程序源码很精简，程序首先在 24-25 构造了两个指定地址的区域，以便方便调试观察，第一个区域 MEMBLOCK_FAKE_MEMORY 添加到 memblock.memory 区域里，第二个区域 MEMBLOCK_FAKE_RESERVE 添加到 memblock.reserved 区域里。接着在 27-30 行调用 for_each_mem_range() 函数遍历 memblock.memory 上所有的区域. 然后在 32-35 行调用 for_each_mem_range_rev() 函数倒序遍历 memblock.memory 上所有的区域. 遍历完之后函数在 38-39 行释放 MEMBLOCK_FAKE_MEMORY 和 MEMBLOCK_FAKE_RESERVE 区域. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002186.png)

从 BiscuitOS 实践结果来看，内核先正序遍历了 memblock.memory 区域里所有的 Region. 然后倒序遍历了 memblock.memory 区域所有的 Region.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B54">for_each_mem_region</span>

![](/assets/PDB/HK/TH002187.png)

for_each_mem_region() 函数用于遍历 MEMBLOCK 的 memory 区域，其可以满足以下几种需求的遍历:

* memblock.memory 区域单独遍历所有 Region.
* MEMBLOCK_NONE 无差别的遍历 memblock.memory 所有 Region.

参数 region 用于记录每次遍历到的 Region，其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: for_each_mem_region()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-for_each_mem_region-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-for_each_mem_region-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-for_each_mem_region)

![](/assets/PDB/HK/TH002188.png)

程序源码很精简，程序首先在 24-25 构造了两个指定地址的区域，以便方便调试观察，第一个区域 MEMBLOCK_FAKE_MEMORY 添加到 memblock.memory 区域里，第二个区域 MEMBLOCK_FAKE_RESERVE 添加到 memblock.reserved 区域里。接着在 27-30 行调用 for_each_mem_region() 函数遍历 memblock.memory 上所有的区域. 遍历完之后函数在 32-33 行释放 MEMBLOCK_FAKE_MEMORY 和 MEMBLOCK_FAKE_RESERVE 区域. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002189.png)

从 BiscuitOS 实践结果来看，内核遍历了 memblock.memory 区域里所有的 Region.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B55">for_each_reserved_mem_range</span>

![](/assets/PDB/HK/TH002190.png)

for_each_reserved_mem_range() 函数用于按遍历 MEMBLOCK 的 reserved 区域，其可以满足以下几种需求的遍历:

* memblock.reserved 区域单独遍历所有 Region.
* MEMBLOCK_NONE 无差别的遍历 memblock.reserved 所有 Region.

参数 i 用于记录遍历 Region 的顺序，参数 p_start 记录每次遍历到区域的起始物理地址，参数 p_end 记录每次遍历到区域的结束物理地址. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: for_each_reserved_mem_range()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-for_each_reserved_mem_range-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-for_each_reserved_mem_range-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-for_each_reserved_mem_range)

![](/assets/PDB/HK/TH002191.png)

程序源码很精简，程序首先在 24-25 构造了两个指定地址的区域，以便方便调试观察，第一个区域 MEMBLOCK_FAKE_MEMORY 添加到 memblock.memory 区域里，第二个区域 MEMBLOCK_FAKE_RESERVE 添加到 memblock.reserved 区域里。接着在 27-30 行调用 for_each_reserved_mem_range() 函数遍历 memblock.reserved 上所有的区域. 遍历完之后函数在 34-35 行释放 MEMBLOCK_FAKE_MEMORY 和 MEMBLOCK_FAKE_RESERVE 区域. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002192.png)

从 BiscuitOS 实践结果来看，内核遍历了 memblock.reserved 区域里所有的 Region.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B56">for_each_reserved_mem_region</span>

![](/assets/PDB/HK/TH002193.png)

for_each_reserved_mem_region() 函数用于按遍历 MEMBLOCK 的 reserved 区域，其可以满足以下几种需求的遍历:

* memblock.reserved 区域单独遍历所有 Region.
* MEMBLOCK_NONE 无差别的遍历 memblock.reserved 所有 Region.

参数 region 用于记录每次遍历到的 Region，其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: for_each_reserved_mem_region()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-for_each_reserved_mem_region-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-for_each_reserved_mem_region-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-for_each_reserved_mem_region)

![](/assets/PDB/HK/TH002194.png)

程序源码很精简，程序首先在 24-25 构造了两个指定地址的区域，以便方便调试观察，第一个区域 MEMBLOCK_FAKE_MEMORY 添加到 memblock.memory 区域里，第二个区域 MEMBLOCK_FAKE_RESERVE 添加到 memblock.reserved 区域里。接着在 27-30 行调用 for_each_reserved_mem_region() 函数遍历 memblock.reserved 上所有的区域. 遍历完之后函数在 34-35 行释放 MEMBLOCK_FAKE_MEMORY 和 MEMBLOCK_FAKE_RESERVE 区域. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002195.png)

从 BiscuitOS 实践结果来看，内核遍历了 memblock.reserved 区域里所有的 Region.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B57">memblock_add</span>

![](/assets/PDB/HK/TH002196.png)

memblock_add() 函数用于向 MEMBLOCK 分配器添加一块可用物理内存区域，其最终会被添加到 memblock.memory 区域里. 参数 base 用于指明新添加区域的起始地址，参数 size 指明新添加区域的长度，其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_add()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_add-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_add-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_add)

![](/assets/PDB/HK/TH002197.png)

程序源码很精简，程序首先在 23 行调用 memblock_add() 函数添加了一块可用物理区域，该区域的起始地址是 MEMBLOCK_FAKE_BASE，长度为 MEMBLOCK_FAKE_SIZE。程序接着调用 for_each_mem_range() 函数遍历了 memblock.memory 区域里的所有 Region，程序最后在 31 行调用 memblock_remove() 函数将该区域移除. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002198.png)

从 BiscuitOS 实践结果来看，内核遍历了 memblock.memory 区域里所有的 Region, 其中也包含了 MEMBLOCK_FAKE_BASE 区域.
 
![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B58">memblock_add_node</span>

![](/assets/PDB/HK/TH002199.png)

memblock_add_node() 函数用于向 MEMBLOCK 分配器添加一块可用物理内存区域到指定 memblock.memory 区域的 NUMA NODE. 参数 base 用于指明新添加区域的起始地址，参数 size 指明新添加区域的长度，参数 nid 指明需要插入的 NUMA NODE，参数 flags 指明物理区域的属性，其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_add_node()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_add_node-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_add_node-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_add_node)

![](/assets/PDB/HK/TH002200.png)

程序源码很精简，程序首先在 35 行调用 memblock_add_node() 函数添加了一块可用物理区域到 memblock.memory 的 MEMBLOCK_FAKE_NODE 上，该区域的起始地址是 MEMBLOCK_FAKE_BASE，长度为 MEMBLOCK_FAKE_SIZE。程序接着在 38-42 行调用 \_\_for_each_mem_range() 函数遍历 MEMBLOCK_FAKE_NODE 上所有可用物理区域，最后程序在 44 行调用 memblock_remove() 函数将该区域移除. 接下来在 BiscuitOS 上查看运行案例:

![](/assets/PDB/HK/TH002201.png)

从 BiscuitOS 实践结果来看，内核遍历了 memblock.memory MEMBLOCK_FAKE_NODE 节点上所有区域的 Region, 其中也包含了 MEMBLOCK_FAKE_BASE 区域.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B59">memblock_alloc</span>

![](/assets/PDB/HK/TH002202.png)

memblock_alloc() 函数用于从 MEMBLOCK 分配器中分配指定长度的物理内存，并返回物理内存对应的虚拟地址。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc)

![](/assets/PDB/HK/TH002203.png)

程序源码很精简，程序在 21 行调用 memblock_alloc() 函数申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐，函数申请成功之后返回物理内存对应的虚拟地址，并存储在 mem 变量里。接着在 27-28 行使用申请到的物理内存。最后内存使用完毕之后，在 31 行调用 memblock_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002204.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，并可以使用该内存，使用完毕之后也可以正确回收.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5A">memblock_alloc_exact_nid_raw</span>

![](/assets/PDB/HK/TH002205.png)

memblock_alloc_exact_nid_raw() 函数用于从 MEMBLOCK 分配器中精准分配内存，其可以从指定范围或者指定 NUMA NODE 上分配物理内存。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式, 参数 min_addr 和 max_addr 指明分配物理内存的范围，参数 nid 指明从指定 NUMA NODE 上分配. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc_exact_nid_raw()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc_exact_nid_raw-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc_exact_nid_raw-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc_exact_nid_raw)

![](/assets/PDB/HK/TH002206.png)

程序源码很精简，程序在 34 行调用 memblock_alloc_exact_nid_raw() 函数申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐，另外分配的物理内存必须来自 MEMBLOCK_FAKE_NODE, 其因属于 MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END 的区域。函数申请成功之后返回物理内存对应的虚拟地址，并存储在 mem 变量里。接着在 42-45 行使用申请到的物理内存。最后内存使用完毕之后，在 47 行调用 memblock_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002207.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，分配的内存来自 MEMBLOCK_FAKE_NODE, 并且属于 MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END 的区域，并可以使用该内存，使用完毕之后也可以正确回收.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5B">memblock_alloc_from</span>

![](/assets/PDB/HK/TH002208.png)

memblock_alloc_from() 函数用于从指定位置之后的区域中分配物理内存。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式, 参数 min_addr 指明分配物理内存的起始地址. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc_from()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc_from-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc_from-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc_from)

![](/assets/PDB/HK/TH002209.png)

程序源码很精简，程序在 22 行调用 memblock_alloc_from() 函数从 MEMBLOCK_FAKE_FROM 地址开始，申请申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐的物理内存。函数申请成功之后返回物理内存对应的虚拟地址，并存储在 mem 变量里。接着在 29-31 行使用申请到的物理内存。最后内存使用完毕之后，在 34 行调用 memblock_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002210.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，分配的物理内存地址超过 MEMBLOCK_FAKE_FROM，并可以使用该内存，使用完毕之后也可以正确回收.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5C">memblock_alloc_low</span>

![](/assets/PDB/HK/TH002211.png)

memblock_alloc_from() 函数用于分配低地址物理内存，其范围是 MEMBLOCK_LOW_LIMIT 到 ARCH_LOW_ADDRESS_LIMIT。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc_low()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc_low-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc_low-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc_low)

![](/assets/PDB/HK/TH002212.png)

程序源码很精简，程序在 25 行调用 memblock_alloc_low() 函数申请申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐的物理内存。函数申请成功之后返回物理内存对应的虚拟地址，并存储在 mem 变量里。接着在 31-33 行使用申请到的物理内存。最后内存使用完毕之后，在 36 行调用 memblock_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002210.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，分配的物理内存地址在 MEMBLOCK_LOW_LIMIT 到 ARCH_LOW_ADDRESS_LIMIT 之间，并可以使用该内存，使用完毕之后也可以正确回收.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5D">memblock_alloc_node</span>

![](/assets/PDB/HK/TH002213.png)

memblock_alloc_node() 函数用于从指定 NUMA NODE 上分配物理内存。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式，参数 nid 指明了 NUMA NODE. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc_node()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc_node-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc_node-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc_node)

![](/assets/PDB/HK/TH002214.png)

程序源码很精简，程序在 32 行调用 memblock_alloc_node() 函数从 MEMBLOCK_FAKE_NODE 上申请申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐的物理内存。函数申请成功之后返回物理内存对应的虚拟地址，并存储在 mem 变量里。接着在 39-40 行使用申请到的物理内存。最后内存使用完毕之后，在 44 行调用 memblock_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002215.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，此时 MEMBLOCK_FAKE_NODE 的范围是 0x20000000 到 0x300000000，因此符合分配的要求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5E">memblock_alloc_range_nid</span>

![](/assets/PDB/HK/TH002216.png)

memblock_alloc_range_nid() 函数用于精准分配物理内存。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式，参数 start 和 end 指明了分配的范围，参数 nid 指明了 NUMA NODE，参数 exact_nid 指明是否支持精准分配. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc_range_nid()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc_range_nid-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc_range_nid-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc_range_nid)

![](/assets/PDB/HK/TH002217.png)

程序源码很精简，程序在 34 行调用 memblock_alloc_range_nid() 函数从 MEMBLOCK_FAKE_NODE 上申请申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐的物理内存, 另外分配的范围是: MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END。函数申请成功之后返回物理地址，然后打印物理地址。最后内存使用完毕之后，在 45 行调用 memblock_phys_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002218.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，此时 MEMBLOCK_FAKE_NODE 的范围是 0x22000000 到 0x280000000，因此符合分配的要求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5F">memblock_alloc_raw</span>

![](/assets/PDB/HK/TH002219.png)

memblock_alloc_raw() 函数用于分配物理内存。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc_raw()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc_raw-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc_raw-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc_raw)

![](/assets/PDB/HK/TH002220.png)

程序源码很精简，程序在 21 行调用 memblock_alloc_raw() 函数申请申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐的物理内存, 函数申请成功之后返回物理地址，然后在 27-28 行使用内存。最后内存使用完毕之后，在 31 行调用 memblock_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002221.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，并可以使用分配的物理内存，因此符合分配的要求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5G">memblock_alloc_try_nid</span>

![](/assets/PDB/HK/TH002222.png)

memblock_alloc_try_nid() 函数用于精准从指定 NUMA NODE 分配物理内存。参数 size 指明申请物理内存的长度，align 参数指明分配内存对齐方式，参数 min_addr 和 max_addr 指明分配物理内存的范围，参数 nid 指明分配的 NUMA NODE. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_alloc_try_nid()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_alloc_try_nid-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_alloc_try_nid-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_alloc_try_nid)

![](/assets/PDB/HK/TH002223.png)

程序源码很精简，程序在 28 行调用 memblock_alloc_try_nid() 函数从 MEMBLOCK_FAKE_NODE 上申请申请 MEMBLOCK_FAKE_SIZE 长度的物理内存，并且物理内存按 SMP_CACHE_BYTES 方式对齐的物理内存，申请的物理内存需要落在 MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END 之间, 函数申请成功之后返回虚拟地址，然后在 36-39 行使用内存。最后内存使用完毕之后，在 41 行调用 memblock_free() 函数进行释放. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002224.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配内存，并且物理内存来自 MEMBLOCK_FAKE_NODE, 且正好落在 MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FA-KE_END 之间的物理内存，最后可以使用分配的物理内存，因此符合分配的要求.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5H">memblock_bottom_up</span>

![](/assets/PDB/HK/TH002225.png)

memblock_bottom_up() 函数用于获得 MEMBLOCK 分配器分配方向。MEMBLOCK 分配器支持从顶端向底部方向的分配，也支持从底部向顶端方向的分配. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_bottom_up()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_bottom_up-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_bottom_up-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_bottom_up)

![](/assets/PDB/HK/TH002226.png)

程序源码很精简，程序在 19 行调用 memblock_bottom_up() 函数获得 MEMBLOCK 分配器默认的分配方向，然后在 22 行调用 memblock_set_bottom_up() 函数将分配方向修改为 bottom 到 up，接着 23-27 行调用 memblock_alloc() 分配一块内存; 同理在 30 行再次调用 memblock_set_bottom_up() 函数将分配方向修改为 up 到 bottom，接着在 31-35 行调用 memblock_alloc() 函数分配一块内存，接着在 37-39 行打印两块内存的物理地址。最后回收内存并设置为默认的分配方向. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002227.png)

从 BiscuitOS 实践结果来看，两次分配都从不同的方向分配到了物理内存，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5J">memblock_end_of_DRAM</span>

![](/assets/PDB/HK/TH002228.png)

memblock_end_of_DRAM() 函数可以获得最大 DRAM 地址。MEMBLOCK 分配器的 memory 区域维护了系统可用的物理内存，因此可以获得最大物理内存地址. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_end_of_DRAM()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_end_of_DRAM-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_end_of_DRAM-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_end_of_DRAM)

![](/assets/PDB/HK/TH002229.png)

程序源码很精简，通过 MEMBLOCK 分配器提供的接口获得内存相关的信息，其中 21 行调用 memblock_phys_mem_size() 函数获得可用物理内存的数量，25 行调用 memblock_reserved_size() 函数获得预留内存的数量，最后在 30 行调用 memblock_start_of_DRAM() 和 memblock_end_of_DRAM() 函数获得可用物理内存的范围. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002230.png)

从 BiscuitOS 实践结果来看，可用物理内存为 0x3ff7cc00，预留内存为 0x2509000，可用物理内存的范围是 0x1000 到 0x3ffde000，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5K">memblock_start_of_DRAM</span>

![](/assets/PDB/HK/TH002231.png)

memblock_start_of_DRAM() 函数可以获得 DRAM 起始地址。MEMBLOCK 分配器的 memory 区域维护了系统可用的物理内存，因此可以获得起始物理内存地址. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_start_of_DRAM()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_start_of_DRAM-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_start_of_DRAM-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_start_of_DRAM)

![](/assets/PDB/HK/TH002232.png)

程序源码很精简，通过 MEMBLOCK 分配器提供的接口获得内存相关的信息，其中 21 行调用 memblock_phys_mem_size() 函数获得可用物理内存的数量，25 行调用 memblock_reserved_size() 函数获得预留内存的数量，最后在 30 行调用 memblock_start_of_DRAM() 和 memblock_end_of_DRAM() 函数获得可用物理内存的范围. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002230.png)

从 BiscuitOS 实践结果来看，可用物理内存为 0x3ff7cc00，预留内存为 0x2509000，可用物理内存的范围是 0x1000 到 0x3ffde000，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5L">memblock_reserved_size</span>

![](/assets/PDB/HK/TH002234.png)

memblock_reserved_size() 函数可以获得 MEMBLOCK 分配器 reserved 区域的大小。MEMBLOCK 分配器的 reserved 区域维护了系统已经被使用的物理内存. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_reserved_size()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_reserved_size-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_reserved_size-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_reserved_size)

![](/assets/PDB/HK/TH002233.png)

程序源码很精简，通过 MEMBLOCK 分配器提供的接口获得内存相关的信息，其中 21 行调用 memblock_phys_mem_size() 函数获得可用物理内存的数量，25 行调用 memblock_reserved_size() 函数获得预留内存的数量，最后在 30 行调用 memblock_start_of_DRAM() 和 memblock_end_of_DRAM() 函数获得可用物理内存的范围. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002230.png)

从 BiscuitOS 实践结果来看，可用物理内存为 0x3ff7cc00，预留内存为 0x2509000，可用物理内存的范围是 0x1000 到 0x3ffde000，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5M">memblock_phys_mem_size</span>

![](/assets/PDB/HK/TH002235.png)

memblock_phys_mem_size() 函数可以获得 MEMBLOCK 分配器维护可用物理内存的大小。MEMBLOCK 分配器的 memory 区域维护了系统所有可用的物理内存. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_phys_mem_size()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_phys_mem_size-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_phys_mem_size-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_phys_mem_size)

![](/assets/PDB/HK/TH002236.png)

程序源码很精简，通过 MEMBLOCK 分配器提供的接口获得内存相关的信息，其中 21 行调用 memblock_phys_mem_size() 函数获得可用物理内存的数量，25 行调用 memblock_reserved_size() 函数获得预留内存的数量，最后在 30 行调用 memblock_start_of_DRAM() 和 memblock_end_of_DRAM() 函数获得可用物理内存的范围. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002230.png)

从 BiscuitOS 实践结果来看，可用物理内存为 0x3ff7cc00，预留内存为 0x2509000，可用物理内存的范围是 0x1000 到 0x3ffde000，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5N">memblock_is_map_memory</span>

![](/assets/PDB/HK/TH002237.png)

memblock_is_map_memory() 函数用于判断某段物理地址是否已经被虚拟地址映射。在 X86 架构中，MEMBLOCK 分配器维护的 memblock 区域的所有物理内存属于线性映射区，因此只要物理区域属于 memory 区域，那么函数就返回 true。参数 addr 指向需要检测的物理地址. 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_is_map_memory()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_is_map_memory-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_is_map_memory-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_is_map_memory)

![](/assets/PDB/HK/TH002238.png)

程序源码很精简，案例首先在 22 行通过调用 memblock_alloc() 函数分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，并返回物理内存对应的虚拟地址，接着在 28 行调用 \_\_pa() 函数获得虚拟地址对应的物理地址，接着在 32 行调用 memblock_is_map_memory() 函数判断物理地址是否已经被虚拟地址映射. 案例接着在 34 行将原来的物理地址加上 SMP_CACHE_BYTES 之后获得一个新的物理地址，同样检查新的物理地址是否已经被虚拟地址映射。最后案例在 39 行调用 memblock_free() 函数释放了分配的物理内存. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002239.png)

从 BiscuitOS 实践结果来看，案例分配的物理地址是 0x3ffddfc0，其一定属于被虚拟地址映射的物理地址，然后 0x3ffddfc0 加上 SMP_CACHE_BYTES 之后的物理地址就不是一个被虚拟地址映射的物理地址.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5P">memblock_free</span>

![](/assets/PDB/HK/TH002240.png)

memblock_free() 函数用于回收 MEMBLOCK 分配器分配的物理地址。参数 ptr 为释放的物理地址对应的虚拟地址，参数 size 表示回收物理内存的长度. memblock_free() 函数回收的物理内存将从 memblock.reserved 区域中移除. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_free()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_free-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_free-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_free)

![](/assets/PDB/HK/TH002241.png)

程序源码很精简，案例首先在 21 行通过调用 memblock_alloc() 函数分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，并返回物理内存对应的虚拟地址，接着在 27-28 行使用这段物理内存，最后使用完毕之后在 31 行调用 memblock_free() 函数回收分配的物理内存. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002242.png)

从 BiscuitOS 实践结果来看，案例可以分配并使用物理内存，最后可以正确回收物理内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5Q">memblock_get_current_limit</span>

![](/assets/PDB/HK/TH002243.png)

memblock_get_current_limit() 函数用于获得 MEMBLOCK 分配器可分配的最大物理内存，该值不代表 MEMBLOCK 分配器维护的最大可用物理内存，limit 值可以动态修改. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_get_current_limit()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_get_current_limit-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_get_current_limit-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_get_current_limit)

![](/assets/PDB/HK/TH002244.png)

程序源码很精简，案例首先在 23 行获得默认的 Limit，然后在 25 行调用 memblock_set_current_limit() 函数将 Limit 设置为 MEMBLOCK_FAKE_LIMIT, 接着在 28 行通过调用 memblock_alloc() 函数分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，并返回物理内存对应的虚拟地址，接着在 34-35 行使用这段物理内存，最后使用完毕之后在 38 行调用 memblock_free() 函数回收分配的物理内存，最后在 41 行调用 memblock_set_current_limit() 函数设置为默认的 Limit. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002245.png)

从 BiscuitOS 实践结果来看，案例分配的物理内存为 0xffffc0，其没有超过 MEMBLOCK_FAKE_LIMIT 的地址，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5R">memblock_set_current_limit</span>

![](/assets/PDB/HK/TH002246.png)

memblock_set_current_limit() 函数用于设置 MEMBLOCK 分配器可分配的最大物理内存，该值不代表 MEMBLOCK 分配器维护的最大可用物理内存，limit 值可以动态修改. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_set_current_limit()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_set_current_limit-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_set_current_limit-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_set_current_limit)

![](/assets/PDB/HK/TH002247.png)

程序源码很精简，案例首先在 23 行获得默认的 Limit，然后在 25 行调用 memblock_set_current_limit() 函数将 Limit 设置为 MEMBLOCK_FAKE_LIMIT, 接着在 28 行通过调用 memblock_alloc() 函数分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，并返回物理内存对应的虚拟地址，接着在 34-35 行使用这段物理内存，最后使用完毕之后在 38 行调用 memblock_free() 函数回收分配的物理内存，最后在 41 行调用 memblock_set_current_limit() 函数设置为默认的 Limit. 接下来在 BiscuitOS 上运行案例:

![](/assets/PDB/HK/TH002245.png)

从 BiscuitOS 实践结果来看，案例分配的物理内存为 0xffffc0，其没有超过 MEMBLOCK_FAKE_LIMIT 的地址，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5S">memblock_is_memory</span>

![](/assets/PDB/HK/TH002248.png)

memblock_is_memory() 函数用于判断物理区域是否属于 MEMBLOCK 分配器维护的 memory 可用物理内存区域. 参数 addr 指向检查的物理区域. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_is_memory()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_is_memory-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_is_memory-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_is_memory)

![](/assets/PDB/HK/TH002249.png)

程序源码很精简，案例首先在 20 行调用 memblock_add() 函数向 MEMBLOCK 分配的 memory 区域添加一块可用物理区域，其范围是 MEMBLOCK_FAKE_BASE, 长度为 MEMBLOCK_SIZE. 函数接着在 23 行调用 memblock_is_memory() 函数判断新添加的区域是否属于可用物理区域，接着函数在 26 行调用 memblock_reserve() 函数将 MEMBLOCK_FAKE_BASE 之后 MEMBLOCK_FAKE_SIZE 的物理区域，长度为 MEMBLOCK_FAKE_SIZE，添加到了 reserved 区域，案例在 28 行调用 memblock_is_reserved() 函数判断新添加的区域是否属于 reserved 区域，最后在 33 和 35 行调用 memblock_remove() 函数将两段物理区域从 MEMBLOCK 分配器中移除。接下来在 BiscuitOS 上实践案例.

![](/assets/PDB/HK/TH002250.png)

从 BiscuitOS 实践结果来看，0x800000000 属于 MEMBLOCK 的 memory 区域，而 0x800100000 属于 MEMBLOCK 的 reserved 区域, 符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5T">memblock_is_reserved</span>

![](/assets/PDB/HK/TH002251.png)

memblock_is_reserved() 函数用于判断物理区域是否属于 MEMBLOCK 分配器维护的 reserved 预留物理内存区域. 参数 addr 指向检查的物理区域. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_is_reserved()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_is_reserved-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_is_reserved-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_is_reserved)

![](/assets/PDB/HK/TH002252.png)

程序源码很精简，案例首先在 20 行调用 memblock_add() 函数向 MEMBLOCK 分配的 memory 区域添加一块可用物理区域，其范围是 MEMBLOCK_FAKE_BASE, 长度为 MEMBLOCK_FAKE_SIZE. 函数接着在 23 行调用 memblock_is_memory() 函数判断新添加的区域是否属于可用物理区域，接着函数在 26 行调用 memblock_reserve() 函数将 MEMBLOCK_FAKE_BASE 之后 MEMBLOCK_FAKE_SIZE 的物理区域，长度为 MEMBLOCK_FAKE_SIZE，添加到了 reserved 区域，案例在 28 行调用 memblock_is_reserved() 函数判断新添加的区域是否属于 reserved 区域，最后在 33 和 35 行调用 memblock_remove() 函数将两段物理区域从 MEMBLOCK 分配器中移除。接下来在 BiscuitOS 上实践案例.

![](/assets/PDB/HK/TH002250.png)

从 BiscuitOS 实践结果来看，0x800000000 属于 MEMBLOCK 的 memory 区域，而 0x800100000 属于 MEMBLOCK 的 reserved 区域, 符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5U">memblock_is_region_reserved</span>

![](/assets/PDB/HK/TH002253.png)

memblock_is_region_reserved() 函数用于判断物理区域是否属于 MEMBLOCK 分配器维护的 reserved 预留物理内存区域, 或者判断某段物理内存是否已经不可分配. 参数 base 指向检查的物理区域，size 参数指明区域的长度. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_is_region_reserved()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_is_region_reserved-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_is_region_reserved-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_is_region_reserved)

![](/assets/PDB/HK/TH002254.png)

程序源码很精简，案例首先在 20 行调用 memblock_reserve() 函数向 MEMBLOCK 分配的 reserved 区域添加一块预留的物理区域，其范围是 MEMBLOCK_FAKE_BASE, 长度为 MEMBLOCK_FAKE_SIZE. 函数接着在 22 行调用 memblock_is_region_reserved() 函数判断新添加的区域是否属于预留物理区域，如果是则打印相关的字符串，最后函数在 26 行调用 memblock_remove() 将测试用的区域从 MEMBLOCK 分配器中移除. 接下来在 BiscuitOS 上实践案例:

![](/assets/PDB/HK/TH002255.png)

从 BiscuitOS 实践结果来看，0x800000000 属于 MEMBLOCK 的 reserved 区域, 符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5V">memblock_is_region_memory</span>

![](/assets/PDB/HK/TH002256.png)

memblock_is_region_memory() 函数用于判断物理区域是否属于 MEMBLOCK 分配器维护的 memory 预留物理内存区域, 或者判断某段物理内存是否为可用物理内存. 参数 base 指向检查的物理区域，size 参数指明区域的长度. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_is_region_memory()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_is_region_memory-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_is_region_memory-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_is_region_memory)

![](/assets/PDB/HK/TH002257.png)

程序源码很精简，案例首先在 20 行调用 memblock_add() 函数向 MEMBLOCK 分配的 memory 区域添加一块可用的物理区域，其范围是 MEMBLOCK_FAKE_BASE, 长度为 MEMBLOCK_FAKE_SIZE. 函数接着在 22 行调用 memblock_is_region_memory() 函数判断新添加的区域是否属于可用物理区域，如果是则打印相关的字符串，最后函数在 26 行调用 memblock_remove() 将测试用的区域从 MEMBLOCK 分配器中移除. 接下来在 BiscuitOS 上实践案例:

![](/assets/PDB/HK/TH002258.png)

从 BiscuitOS 实践结果来看，0x800000000 属于 MEMBLOCK 的 memory 区域, 符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5W">memblock_overlaps_region</span>

![](/assets/PDB/HK/TH002259.png)

memblock_overlaps_region() 函数用于判断某块区域是否与 memblock.memory 或者 memblock.reserved 区域重叠，如果重叠则返回 1. 参数 type 指向 memblock.memory 或者 memblock.reserved 区域，参数 base 和 size 指明了检测区域的范围. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_addrs_overlap()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_addrs_overlap-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_addrs_overlap-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_addrs_overlap)

![](/assets/PDB/HK/TH002260.png)

程序源码很精简，案例在 22 行调用 memblock_addrs_overlap() 函数检测 MEMBLOCK_FAKE_BASE 区域是否与 memblock.memory 区域重叠. 接下来 BiscuitOS 上实践案例:

![](/assets/PDB/HK/TH002261.png)

从 BiscuitOS 实践结果来看，0xC000000 到 0xC100000 区域与 MEMBLOCK 的 memory 区域中的 Region 重叠, 符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5X">memblock_phys_alloc_range</span>

![](/assets/PDB/HK/TH002262.png)

memblock_phys_alloc_range() 函数用于从指定范围内分配物理内存，参数 size 指明分配物理内存的长度，参数 align 指明分配的物理内存对其方式，参数 start 和 end 指明了分配物理内存的范围. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_phys_alloc_range()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_phys_alloc_range-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_phys_alloc_range-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_phys_alloc_range)

![](/assets/PDB/HK/TH002263.png)

程序源码很精简，案例在 35 行调用 memblock_phys_alloc_range() 函数从 MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END 区域中分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，物理内存必须按 SMP_CACHE_BYTES 方式对齐。当分配成功之后，函数将分配的物理地址存储在 phys 变量里。案例接着在 44 行调用 phys_to_virt() 函数将 phys 物理地址转换成虚拟地址，并在 46-48 行使用内存，最后函数在 51 行调用 memblock_phys_free() 函数释放分配的物理内存. 案例在 BiscuitOS 上的实践如下:

![](/assets/PDB/HK/TH002264.png)

从 BiscuitOS 实践结果来看，分配的物理内存属于 MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END 区域，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5Y">memblock_phys_free</span>

![](/assets/PDB/HK/TH002265.png)

memblock_phys_free() 函数用于回收 MEMBLOCK 分配的物理内存，参数 base 和 size 指明了回收物理内存的范围。案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_phys_free()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_phys_free-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_phys_free-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_phys_free)

![](/assets/PDB/HK/TH002266.png)

程序源码很精简，案例在 21 行调用 memblock_alloc_raw() 函数分配一块长度为 MEMBLOCK_FAKE_SIZE 的物理内存，然后在使用完毕之后，在 31 行调用 memblock_phys_free() 函数进行回收。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5Z">memblock_remove</span>

![](/assets/PDB/HK/TH002267.png)

memblock_remove() 函数用于从 MEMBLOCK 分配器的 memory 区域移除一段可用物理区域。参数 base 和 size 指明了移除物理区域的范围. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_remove()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_remove-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_remove-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_remove)

![](/assets/PDB/HK/TH002268.png)

程序源码很精简，案例在 21 行调用 memblock_add() 函数向 MEMBLOCK 分配器 memory 区域新增一块物理区域，然后在 26 行调用 for_each_mem_range() 函数遍历 memblock.memory 区域所有可用物理区域，接着函数在 30 行调用 memblock_remove() 函数从 memblock.memory 区域中移除新增的物理区域，最后在 33 行再次调用 for_each_mem_range() 函数遍历 memblock.memory 所有可用物理区域。案例在 BiscuitOS 中实践如下:

![](/assets/PDB/HK/TH002269.png)

从 BiscuitOS 实践结果来看，memblock_remove() 函数移除的区域确实来自 memblock.memory，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B60">memblock_reserve</span>

![](/assets/PDB/HK/TH002270.png)

memblock_reserve() 函数用于将一段物理区域进行预留，或者标记为 MEMBLOCK 分配器不可分配的物理内存。参数 base 和 size 指明的预留物理区域的范围。其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_reserve()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_reserve-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_reserve-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_reserve)

![](/assets/PDB/HK/TH002271.png)

程序源码很精简，案例在 20 行调用 memblock_reserve() 函数向 MEMBLOCK 分配器 reserved 区域新增一块物理区域，那么这块物理区域就不能再被分配。然后在 22 行调用 memblock_is_region_reserved() 函数判断该段区域是否已经被预留，如果是则打印; 最后函数在 26 行调用 memblock_remove() 函数将区域从 memblock.reserved 中移除. 案例在 BiscuitOS 中实践如下: 

![](/assets/PDB/HK/TH002272.png)

从 BiscuitOS 实践结果来看，memblock_reserve() 函数可以将某段物理内存添加到 MEMBLOCK 分配器的 memblock.memory 区域，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B61">memblock_phys_alloc</span>

![](/assets/PDB/HK/TH002273.png)

memblock_phys_alloc() 函数用于从指定范围内分配物理内存，参数 size 指明分配物理内存的长度，参数 align 指明分配的物理内存对其方式. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: memblock_phys_alloc()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-memblock_phys_alloc-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-memblock_phys_alloc-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-memblock_phys_alloc)

![](/assets/PDB/HK/TH002274.png)

程序源码很精简，案例在 35 行调用 memblock_phys_alloc() 函数分配长度为 MEMBLOCK_FAKE_SIZE 的物理内存，物理内存必须按 SMP_CACHE_BYTES 方式对齐。当分配成功之后，函数将分配的物理地址存储在 phys 变量里。案例接着在 44 行调用 phys_to_virt() 函数将 phys 物理地址转换成虚拟地址，并在 46-48 行使用内存，最后函数在 51 行调用 memblock_phys_free() 函数释放分配的物理内存. 案例在 BiscuitOS 上的实践如下:

![](/assets/PDB/HK/TH002275.png)

从 BiscuitOS 实践结果来看，案例可以从 MEMBLOCK 分配器中分配到物理内存，并正常使用和回收，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B62">for_each_free_mem_range</span>

![](/assets/PDB/HK/TH002276.png)

for_each_free_mem_range() 函数用于遍历 MEMBLOCK 分配器中可以分配的物理内存。参数 i 指明遍历的 Region 序号，参数 nid 指明需要遍历的空闲内存所在的 NUMA NODE，参数 flags 指明遍历空闲内存的属性，参数 p_start 和 p_end 用于记录每次遍历空闲区域的起始物理地址和结束物理地址，参数 p_nid 指明了遍历到的区域所在的 NUMA NODE. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: for_each_free_mem_range()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-for_each_free_mem_range-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-for_each_free_mem_range-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-for_each_free_mem_range)

![](/assets/PDB/HK/TH002277.png)

程序源码很精简，案例在 36 行调用 memblock_add() 函数向 memblock.memory 区域添加 MEMBLOCK_FAKE_MEMORY 起始的一段物理内存，在 37 行调用 memblock_reserve() 函数向 memblock.reserved 区域添加 MEMBLOCK_FAKE_RESRVE 起始的一段物理内存。案例接着在 40-45 行调用 for_each_free_mem_range() 函数正序遍历 MEMBLOCK_FAKE_NODE 上所有空闲物理内存; 同理案例在 48-54 行调用 for_each_free_mem_range() 函数正序遍历 MEMBLOCK_FAKE_NODE + 1 上的所有空闲物理内存. 最后案例在 57-63 行调用 for_each_free_mem_range_reverse() 函数倒序遍历 MEMBLOCK_FAKE_NODE 上所有空闲物理内存. 案例最后在 67-68 行调用 memblock_remove() 函数移除之前新增的区域. 案例在 biscuitOS 上实践如下:

![](/assets/PDB/HK/TH002278.png)

从 BiscuitOS 实践结果来看，第一次只正序遍历了 MEMBLOCK_FAKE_NODE 上的所有空闲内存，第二次遍历了下一个 NODE 上的所有空闲物理内存，最后以此倒序遍历了 MEMBLOCK_FAKE_NODE 上的所有空闲内存，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B63">for_each_free_mem_range_reverse</span>

![](/assets/PDB/HK/TH002279.png)

for_each_free_mem_range_severse() 函数用于倒序遍历 MEMBLOCK 分配器中可以分配的物理内存。参数 i 指明遍历的 Region 序号，参数 nid 指明需要遍历的空闲内存所在的 NUMA NODE，参数 flags 指明遍历空闲内存的属性，参数 p_start 和 p_end 用于记录每次遍历空闲区域的起始物理地址和结束物理地址，参数 p_nid 指明了遍历到的区域所在的 NUMA NODE. 案例在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK API: for_each_free_mem_range_reverse()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-for_each_free_mem_range_reverse-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-for_each_free_mem_range_reverse-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-for_each_free_mem_range_reverse)

![](/assets/PDB/HK/TH002280.png)

程序源码很精简，案例在 36 行调用 memblock_add() 函数向 memblock.memory 区域添加 MEMBLOCK_FAKE_MEMORY 起始的一段物理内存，在 37 行调用 memblock_reserve() 函数向 memblock.reserved 区域添加 MEMBLOCK_FAKE_RESRVE 起始的一段物理内存。案例接着在 40-45 行调用 for_each_free_mem_range() 函数正序遍历 MEMBLOCK_FAKE_NODE 上所有空闲物理内存; 同理案例在 48-54 行调用 for_each_free_mem_range() 函数正序遍历 MEMBLOCK_FAKE_NODE + 1 上的所有空闲物理内存. 最后案例在 57-63 行调用 for_each_free_mem_range_reverse() 函数倒序遍历 MEMBLOCK_FAKE_NODE 上所有空闲物理内存. 案例最后在 67-68 行调用 memblock_remove() 函数移除之前新增的区域. 案例在 biscuitOS 上实践如下:

![](/assets/PDB/HK/TH002278.png)

从 BiscuitOS 实践结果来看，第一次只正序遍历了 MEMBLOCK_FAKE_NODE 上的所有空闲内存，第二次遍历了下一个 NODE 上的所有空闲物理内存，最后以此倒序遍历了 MEMBLOCK_FAKE_NODE 上的所有空闲内存，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

#### MEMBLOCK 物理内存分配器进阶研究

------------------------------------------

<span id="T0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

##### Mirror Memory 研究

![](/assets/PDB/HK/TH002282.png)

Memory Mirror 是一种硬件控制行为，但硬件系统采用这种行为之后，硬件上会将同体积的两块内存作为主备。当系统对 Mirror 内存进行写操作时，内存控制器会将写请求同时下发到两块内存上。当系统对 Mirror 内存进行读操作时，为了保持一致性，内存控制器会同时读取两块内存上的数据，然后进行比较，确保数据是一致的。Memory mirror 对硬件的影响很大，直接改了内存的 interleaving Mode，可能导致性能受损.

![](/assets/PDB/HK/TH002283.png)

Memory mirror 虽然缺点明显，但在 RAS 高可用场景，如果 MemoryA 发送 UNcorrectable Error(UE)，如果没有 Memory mirror，那么 CPU 读取 UE 内存之后直接导致宕机; 但如果使能 Memory mirror 的话，如果 Memory A 上发送了 UE，那么内存控制器就直接使用 Memory B 上的数据，这大大提供了系统可用性.

![](/assets/PDB/HK/TH002284.png)

Memroy mirror 从软件角度来看，其就是一段普通的物理内存，但其有具有硬件所见的特殊功能，因此需要特殊对待。在系统启动阶段，有的组件或子系统回去使用这部分内存，由于 MEMBLOCK 分配器负责早期的物理内存管理，因此 MEMBLOCK 分配器需要对 Memory mirror 进行特殊管理。memblock.memory 区域表示系统可用物理内存，Memory mirror 也属于系统可用物理内存，因此 Memory mirror 内存也维护在 memblock.memory 区域。MEMBLOCK 分配器将维护的 Region 标记为 MEMBLOCK_MIRROR.

![](/assets/PDB/HK/TH002285.png)

由于 Memory Mirror 物理内存维护在 memblock.memory 区域里，并且 MEMBLOCK 分配器对外提供了 Mirrorable 物理内存分配和 Non-Mirrorable 物理内存的分配，因此当请求者需要分配 Mirror 物理内存时，MEMBLOCK 分配器优先提供 Mirrorable 物理内存，如果系统没有足够的 Mirrorable 内存，那么 MEMBLOCK 分配器会发出警告，但分配器还是会提供 Non-Mirrorable 物理内存. 那么接下来通过一个 Memory mirror 案例进行分析, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MIRROR MEMORY: mirror memory on MEMBLOCK()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-MIRROR-MEMORY-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-MIRROR-MEMORY-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-MIRROR-MEMORY)

![](/assets/PDB/HK/TH002286.png)

为了支持 MEMBLOCK MIRROR MEMORY 功能，需要在 CMDLINE 中添加 'kernelcore=mirror' 字段。回到案例源码，其在 41 行调用 memblock_mark_mirror() 函数将 MEMBLOCK_MIRROR_BASE 到 MEMBLOCK_MIRROR_END 的区域标记为 Mirrorable，那么这段区域在 memblock.memory 区域中被标记为 MEMBLOCK_MIRROR, 于是在 45 行调用 \_\_for_each_mem_range() 函数遍历 memblock.memory 中的 MEMBLOCK_MIRROR 区域进行验证. 案例接着在 50 行调用 memblock_alloc_range_nid() 函数用于从 MEMBLOCK_MIRROR 中分配 mirror 物理内存，在调用函数时只指定了分配范围对应 Mirrorable 区域。分配完毕之后函数在 60 行再次调用 for_each_reserved_mem_region() 函数检查 memblock.reserved 中的 MEMBLOCK_MIRROR 区域，以此检查分配的 mirror 物理内存已经被加入到 memblock.reserved 里. 最后在 65-71 行对 MIRROR 物理内存进行使用和回收。接下来在 BiscuitOS 上进行实践:

![](/assets/PDB/HK/TH002287.png)

BiscuitOS 运行之后，可以从 Dmesg 中看到第一次遍历 memblock.memory 的时候，可以只遍历 MEMBLOCK_MIRROR 区域，正好是 MEMBLOCK_MIRROR_BASE 到 MEMBLOCK_M-IRROR_END. 当分配 mirror 物理内存之后再次遍历 memblock.reserved 区域，此时分配的物理地址是 0x2200ffc0，可以在 memblock.reserved 中找到，但是其 flags 并不是 MEMBLO-CK_MIRROR. 以上实践符合预期. 

![](/assets/PDB/HK/TH002288.png)

MEMBLOCK 分配器优先为需求 Mirror 内存的请求者提供 Mirrorable 内存，如果 MEMBLOCK 分配器已经将所有 Mirrorable 内存分配完，那么 MEMBLOCK 分配器会在给请求者发出警告，然后为其分配 Un-Mirrorable 的物理内存。接下来通过一个案例进行实践:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MIRROR MEMORY: Lake Mirrorable Memory  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-MIRROR-MEMORY-lake-memory-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-MIRROR-MEMORY-lake-memory-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-MIRROR-MEMORY-lake-memory)

![](/assets/PDB/HK/TH002289.png)

为了支持 MEMBLOCK MIRROR MEMORY 功能，需要在 CMDLINE 中添加 'kernelcore=mirror' 字段。回到案例源码，其首先在 42-46 行先遍历了 MEMBLOCK_FAKE_NODE 上可分配的物理内存，然后在 49 行调用 memblock_mark_mirror() 函数将 MEMBLOCK_MIRROR_BASE 长度为 MEMBLOCK_MIRROR_SIZE 的物理内存标记为 Mirrorable. 接着函数在 52 行调用 memblock_alloc_range_nid() 函数从 MEMBLOCK_FAKE_BASE 到 MEMBLOCK_FAKE_END 区域分配器长度为 MEMBLOCK_FAKE_SIZE 的 Mirrorable 物理内存。接着就是分配的物理内存使用和回收。在案例中可以看到 Mirrorable 物理内存只有 0x1000, 但请求者需要 0x2000 的 Mirrorable 物理内存。那么接下来在 BiscuitOS 上实践，以此观察 MEMBLOCK 分配器如何处理这种情况:

![](/assets/PDB/HK/TH002290.png)

BiscuitOS 运行之后，可以看到 MEMBLOCK_FAKE_NODE 上的可用物理内存包括了 MEMBLOCK_MIRROR_BASE 到 MEMBLOCK_MIRROR_END 区域，那么当分配的长度超过 Mirrorable 内存长度之后，MEMBLOCK 分配器分配的物理内存只有分配 Un-Mirrorable 物理内存，此时系统会提示 "Could not allocate 0x0000000000002000 bytes of mirrored memory". 那么值得注意的是: MEMBLOCK 是否会分配既有 Mirrorable 又有 Un-Mirrorable 物理内存, 接下来基于上面的案例，进行修改:

![](/assets/PDB/HK/TH002291.png)

其他不变的前提下，将 MEMBLOCK_FAKE_BASE 修改为 MEMBLOCK_MIRROR_BASE 前一个 PAGE_SIZE，MEMBLOCK_FAKE_END 设置为 MEMBLOCK_MIRROR_BASE 后一个 PAGE_SIZE, 那么此时可分配区域长度正好是 MEMBLOCK_FAKE_SIZE:;

![](/assets/PDB/HK/TH002292.png)

BiscuitOS 运行之后，可以看到 MEMBLOCK 分配器分配内存失败了，因此可以知道 MEMBLOCK 分配器不会同时分配即 Mirrorable 和 Un-Mirrorable 的物理内存. 

> - [如何选择内存模式 (Independent/Mirroring/Lock Step) 区别与性能评测](http://www.shiiko.cn/2019/02/notes/memory-mode/)
>
> - [Configuring the memory mirroring mode](https://techlibrary.hpe.com/docs/iss/proliant-gen10-uefi/GUID-F8C8A028-EB57-4EAC-A664-331EAC4EA821.html)
>
> - [内存的可靠性、可用性和诊断功能(内存 RAS)](https://blog.csdn.net/woody1209/article/details/100924764)
>
> - [Address Range Partial Memory Mirroring - Intel](https://www.intel.com/content/www/us/en/developer/articles/technical/address-range-partial-memory-mirroring.html)
>
> - [Address Range Memory Mirrori - FUJITSU](https://events.static.linuxfound.org/sites/events/files/slides/Address%20Range%20Memory%20Mirroring-RC.pdf)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="T1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000O.jpg)

##### physmem Region 研究

![](/assets/PDB/HK/TH002293.png)

在 MEMBLOCK 分配器中，memblock.memory 区域维护的是系统可用的物理内存 **Available Physical Memory**, memblock.reserved 区域维护的是 MEMBLOCK 分配器已经分配的可用物理内存, 那么 MEMBLOCK 分配器可分配的物理内存就是 memblock.memory 区域中剔除 memblock.reserved 区域之后的物理内存。但在系统中还存在另外一种内存，这部分物理内存不被系统管理，称为 **System Reserved Physical Memory**, 也就是通常所说的预留内存. 系统预留物理内存是特定预留给某个某块或子系统使用，内核无法用通用内存管理器管理到.

![](/assets/PDB/HK/TH002140.png)

系统默认的 MEMBLOCK 分配器使用 memblock.memory 和 memblock.reserved 两组数据结构进行维护，其无法管理到系统预留物理内存。在 X86 架构中，由于 E820 表的存在，系统预留内存的信息都维护在 E820 表中，但在有的架构中并不存在类似 E820 表的机制，因此在这些架构中，想在 Boot 阶段维护系统预留内存，那么可以提供 MEMBLOCK 分配器提供的 physmem 区域.

![](/assets/PDB/HK/TH002294.png)

MEMBLOCK 分配器新建立一个名为 physmem 的区域，这个区域包含了 memblock_physmem_init_regions[] 数组，数组中的成员按从地址到高地址的将系统所有的物理内存区域都进行维护，那么系统预留内存和系统可用物理内存被维护在 physmem 区域里.

![](/assets/PDB/HK/TH002295.png)

physmem 区域中包含了系统所有的物理内存，包括系统可用物理内存和系统预留物理内存，因此 physmem 区域剔除掉 memblock.memory 区域就是系统预留内存.

![](/assets/PDB/HK/TH002296.png)

MEMBLOCK 分配器中，如果要使用 physmem 区域，那么需要打开 CONFIG_HAVE_MEMBLOCK_PHYS_MAP 宏. physmem 区域定义为 struct memblock_type, 其默认情况下支持 INIT_PHYSMEM_REGIONS 个物理区域，这些区域通过 memblock_physmem_init_regions[] 数组维护，当添加的物理区域超过 INIT_PHYSMEM_REGIONS 时，MEMBLOCK 分配器会动态扩容 Regions 的数量. 那么接下来介绍 X86-64 架构上如何启用 physmem 区域:

![](/assets/PDB/HK/TH002297.png)

首先是启用 CONFIG_HAVE_MEMBLOCK_PHYS_MAP 宏，可以在 arch/x86/Kconfig 文件中 "config X86_64" 处选中宏，使用上图的语句 "select HAVE_MEMBLOCK_PHYS_MAP", 其他架构类似，这样重新编译内核的时候，宏就启用。

![](/assets/PDB/HK/TH002298.png)

宏启动之后，接下来就是在内核启动的合适位置，将系统探测到的物理内存添加到 physmem 区域，在 X86-64 架构中，无论支不支持 NUMA 机制，E820 表中都记录了系统物理内存信息，但刚从 BIOS 中获得的 E820 表里是可以包含所有的物理内存，因此在 e820__memory_setup() 函数中添加 1223-1325 行对 e820__memblock_add_physmem() 函数的调用，其实现在 1293-1304 行，函数通过遍历 E820 表的所有 Entry，将其中 E820_TYPE_RAM 的区域全部通过调用 memblock_physmem_add() 函数添加到 physmem 区域里. 接下来使用一个案例说明如何通过 MEMBLOCK 分配器获得系统预留的物理内存，其在 BiscuitOS 中的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK physmem Region  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-physmem-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-physmem-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-physmem)

![](/assets/PDB/HK/TH002299.png)

为了更好的验证 physmem 区域，可以在 CMDLINE 中添加 'memmap=1M$0x10000000 memmap=1M$0x10200000 memmap=1M$0x10400000' 字段，那么系统就存在新增的三块预留区域。案例很简单，案例在 25 行调用 for_each_physmem_range() 函数遍历所有的系统物理内存，案例接着在 30 行同样调用 for_each_physmem_range() 函数，不同的是第二个参数是 memblock.memory 区域，其效果相当于从 physmem 区域中移除 memblock.memory 区域，那么就是系统预留物理内存，一旦有了系统预留内存信息，就可以使用 MEMBLOCK 分配器提供的接口使用物理内存。最后在 BiscuitOS 中实践案例:

![](/assets/PDB/HK/TH002300.png)

当 BiscuitOS 运行之后，可以看到系统物理内存包括两个区域，另外系统预留区域中包括了之前新增的三个区域，符合预期. 通过上面的分析 physmem 区域可以用来维护系统所有的物理内存，并结合 memblock.memory 区域可以获得系统预留内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="T5"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

##### Keeping Work 研究

![](/assets/PDB/HK/TH001990.png)

MEMBLOCK 分配器作为系统启动阶段用于管理系统物理内存的分配器，当系统启动完毕，MEMBLOCK 分配器完成使命，Buddy 分配器接过物理内存管理的任务之后，系统将终结 MEMBLOCK 分配器的生命. 本节研究的 Keeping Work 是想继续延续 MEMBLOCK 分配器的生命，使其在系统启动完毕之后，继续可以为系统提供服务.

![](/assets/PDB/HK/TH002301.png)
![](/assets/PDB/HK/TH002302.png)

为了让 MEMBLOCK 分配器可以继续工作，那么需要在内核打开 CONFIG_A-RCH_KEEP_MEMBLOCK 宏. 例如在 X86-64 架构需要打开该宏，可以修改 "arch/x86/Kconfig" 配置文件，在上图 config X86_64 宏下面填入 **select ARCH_KEEP_MEMBLOCK** 内容, 最后重编译内核即可:

![](/assets/PDB/HK/TH002303.png)

当系统启动完毕之后，系统新增 "/sys/kernel/debug/memblock/" 目录，目录下包含了 memory/physmem/reserved 三个区域，然后通过 cat 命令可以读取每个区域中包含的 Region 信息. 那么 MEMBLOCK 分配器 Keeping Work 之后可以继续获得三个分区的信息，接下来通过几个案例进一步分析，案例一在 BiscuitOS 中的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK KEEP: Iterate Regions  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-KEEP-iterate-region-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-KEEP-iterate-region-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-KEEP-iterate-region)

![](/assets/PDB/HK/TH002304.png)

案例代码很简单，案例在 21 行调用 for_each_mem_range() 函数遍历了 memblock.memory 系统可用物理内存所有区域，另外案例在 27 行调用 device_initcall() 函数，说明此时系统已经启动完成, 那么接下来在 BiscuitOS 上实践案例代码:

![](/assets/PDB/HK/TH002305.png)

BiscuitOS 启动之后，可以看到案例被加载之后，其将系统所有可用物理内存打印出来了，因此在 MEMBLOCK 分配器 Keeping Work 之后，可以继续对 memblock.memory/physmem/memblock.reserved 使用. 那么接下来使用另外一个案例，其在 BiscuitOS 中的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK KEEP: Allocate Memory  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-KEEP-allocate-memory-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-KEEP-allocate-memory-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-KEEP-allocate-memory)

![](/assets/PDB/HK/TH002306.png)

案例代码很简单，案例在 22 行调用 memblock_alloc() 函数分配长度为 MEMBLOC-K_FAKE_SIZE 的物理内存，然后在 28-29 行使用分配的内存，最后在 32 行调用 memblock_free() 函数将分配的内存释放. 那么接下来在 BiscuitOS 上实践案例代码:

![](/assets/PDB/HK/TH002307.png)

BiscuitOS 运行之后，案例被调用时，系统 WARNING mm/memblock.c 文件 1504 处有问题，其函数调用栈显示系统调用 memblock_alloc_try_nid() 函数时发生 WARNING，警告完之后案例还是分配到了内存，并打印了字符串 "Hello BiscuitOS". 以上便是 Keep Work 的两个案例，那么为什么 MEMBLOCK 分配器在 Keeping Work 之后行为与 Boot 阶段出现差异，接下来进行详细分析:

![](/assets/PDB/HK/TH002142.png)
![](/assets/PDB/HK/TH002143.png)

内核在定义 memblock/memblock_memory_init_regions/memblock_reserved_init_regions/memblock_physmem_init_regions/physmem 数据时，将其放置在 \_\_initdata_memblock Section 内，当内核启用了 CONFIG_ARCH_KEEP_MEMBLOCK 宏之后，\_\_initdata_memblock 为空，不再属于 .memblock.text Section. 那么内核初始化到后期之后，这些数据就不会被内核抹除掉，可以继续使用.

![](/assets/PDB/HK/TH002308.png)

至于调用 memblock_alloc() 相关的分配函数，其最终都会调用 memblock_alloc_internal() 函数，函数如果通过 slab_is_available() 函数探测到 SLAB 分配器已经启用，那么其会通过 WARN_ON_ONCE() 函数进行警告，因此告诉调用者请使用 SLAB 分配器内存，不要直接使用 MEMBLOCK 分配器分配内存, 最后内核还是从 SLAB 分配器中分配了内存，没有从 MEMBLOCK 分配器中分配，因此 Keeping Work 之后，MEMBLOCK 分配器已经不提供物理内存分配能力了. 综上所属，MEMBLOCK Keeping Work 的有效用途就是查看 memblock.memory、memblock.reserved、physmem 区域的信息.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="T3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

##### DRVIER-MANAGED 研究

![](/assets/PDB/HK/TH002309.png)

MEMBLOCK 分配器通过 memblock.memory 管理了系统可用的物理内存，这些物理内存按区域进行维护。由于物理内存的用途不同，MEMBLOCK 分配器再将可用物理内存分作了 5 类，第一类 **MEMBLOCK_NONE** 为普通可用物理内存，作为 Buddy 分配器维护的物理内存; 第二类 **MEMBLOCK_MIRROR** 为 Mirror Memory，作为内存高可用的自动镜像内存使用; 第三类 **MEMBLOCK_NOMAP** 为系统预留物理内存，在有的架构中用来管理系统预留物理内存; 第四类 **MEMBLOCK_HOTPLUG** 为热插拔的物理内存，其热插之后作为普通内存使用; 第五类 **MEMBLOCK_DRIVER_MANAGED** 为设备管理的物理内存，由设备添加并独占使用.

![](/assets/PDB/HK/TH002310.png)

在以前的内核中，系统使用的物理内存为 "System RAM", 而设备使用的物理内存只能来自默认约定的物理内存. 而在高版本内核中，MEMBLOCK 分配器支持 MEMBLOCK_D-RIVER_MANAGED 区域之后，设备可以独立通过热插的方式增加物理内存，并且设备独占使用这段物理内存。接下来先通过一个实践案例讲解 MEMBLOCK_DRIVER_MANAGED 的用法，其在 BiscuitOS 中的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK Driver Managned Memory  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-DRIVER-MANAGED-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-DRIVER-MANAGED-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-DRIVER-MANAGED)

![](/assets/PDB/HK/TH002311.png)

案例首先按照 4-6 行的建议在 CMDLINE 中添加 "memmap=128M$0x10000000" 字段到 CMDLINE 中，然后启用 CONFIG_ARCH_KEEP_MEMBLOCK 宏让 MEMBLOCK 分配器一直有效. 案例接着在 24-28 行定义了一段系统物理内存资源，这段物理内存资源的范围是 BISCUITOS_FAKE_BASE 到 BISCUITOS_FAKE_END, 其 flags 设置为 IORESOURCE_SYSRA-M_DRIVER_MANAGED, 意味这段内存是有设备进行管理的，最后 name 设置为 "System RAM Driver Managed". 案例在 38 行调用 add_memory_resource() 函数将 BiscuitOS_resource 描述的物理内存热插到系统。案例在 42 行调用 for_each_mem_region() 函数遍历系统所有可用物理内存，并在 43 行调用 memblock_is_driver_managed() 函数对每次遍历的 Region 进行判断，如果物理区域是 MEMBLOCK_DRIVER_MANAGED 属性，那么打印区域并将区域的起始物理地址转换成对应的虚拟地址，然后存储在 mem 变量里. 案例最后在 50 行判断了 mem 不空的情况下使用对应的内存，并打印内存的内容. 接下来在 BiscuitOS 上实践案例:

![](/assets/PDB/HK/TH002312.png)

当 BiscuitOS 运行之后，可以从启动 Dmesg 中看到案例加载之后，MEMBLOCK 分配器的 MEMBLOCK_DRIVER_MANAGED 区域为 \[0x10000000 - 0x18000000\], 并且内存可以正常使用. 此时在 /sys/devices/kernel/memory 目录下，看到热插之后的设备 memory2，查看其内部的物理内存都是 offline 的，最后在查看 /sys/kernel/debug/memblock/ 目录下 memory 区域中所有物理区域，可以看到 \[0x10000000 - 0x18000000\] 也是独立的物理区域. 另外查看固件映射信息(/sys/firmware/memmap)，并没有包括 MEMBLOCK_DRIVER_MANAGED 中的区域. 通过案例可以知道 MEMBLOCK_DRIVER_MANAGED 的物理区域可以通过设备独立填充并热插到系统，并被设备独占使用(既然都可以获得独占物理内存，当然可以独立管理). 那么接下来分析 MEMBLOCK 分配器如何实现 MEMBLOCK_DRIVER_MANAGED:

![](/assets/PDB/HK/TH002313.png)

MEMBLOCK 分配器新增了 MEMBLOCK_DRIVER_MANAGED 区域标志，用于在 memblock.memory 区域中与其他 memblock_flags 类型的物理区域区分开来。MEMBLOCK 分配器提供了 memblock_is_driver_managed() 函数进行区分, 结合 for_each_mem_region() 函数可以从系统可用物理内存中找出设备管理的物理内存。设备在获得设备管理内存之后，可以使用自定义的分配器进行管理. 

![](/assets/PDB/HK/TH002314.png)

当调用 add_memory_resource() 函数热插一段物理内存时，如果检查到系统已经启用了 CONFIG_ARCH_KEEP_MEMBLOCK 宏，然后 resource 的 flags 包含了 IORESOURCE_S-YSRAM_DRIVER_MANAGED，那么函数将 memblock_flags 设置为 MEMBLOCK_DRIVE-R_MANAGED，并调用 memblock_add_node() 函数向 memblock.memory 区域中新增区域，以上便是设备可以独立插入并管理物理内存的核心，其余的就是内存热插相关的逻辑.

![](/assets/PDB/HK/TH002315.png)

上图是 MEMBLOCK_DRIVER_MANAGED Commit 介绍，其核心思想就是: 在以前内核热插一块物理内存时，其作为 "System RAM" 插入系统，然后驱动要使用只能通过固件提供的映射(/sys/firmware/memmap) 按顺序遍历所有可用内存来使用，可能使用的不是热插的内存; 但有了 MEMBLOCK_DRIVER_MANAGED 之后，其结合 IORESOURCE_SYSRAM_DRIVER_MANAGED 标志可以通过驱动独立将一块物理内存热插到系统，其不为 "System RAM", 且不需要通过固件提供的映射就可以找到这块物理内存并使用. 总结为 MEMBLOCK_DRIVER_MANAGED 物理区域可以被驱动独立探测、加载和使用. 那么接下来通过一个案例实践 MEMBLOCK_HOTPLUG 的差异, 其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK with Hotplug Memory  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-Hotplug-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-Hotplug-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-Hotplug)

![](/assets/PDB/HK/TH002317.png)
![](/assets/PDB/HK/TH002316.png)

在使用案例之前，需要在 RunBiscuitOS.sh 脚本中添加内存热插相关的命令行，那么系统在启动阶段就会自动热插一块物理内存。案例很简单，在内核启动后期在 23 行调用 for_each_mem_region() 函数遍历系统可用物理内存，也包括了刚刚热插的物理内存，并在每次遍历过程中调用 memblock_is_hotplugable() 函数判断物理内存区域是否热插的，如果是则打印物理内存区域信息，并记录区域的起始地址，然后将物理地址转换成虚拟地址，并在 31 行分支使用内存. 接着在 BiscuitOS 上实践案例:

![](/assets/PDB/HK/TH002318.png)

BiscuitOS 运行之后，从 Demsg 中可以看出案例并没有在 MEMBLOCK 的 memory 区域中找到热插的物理内存，但可以在 /sys/firmware/memmap 和 /sys/kernel/debug/memblock/memory 中找到热插的物理内存区域，因此结合之前和 MEMBLOCK_DRIVER_MANAGED 的对比讨论基本可以验证 Commit 里提到的，Hotplug 的物理内存只能作为 "System RAM" 被系统使用，而 MEMBLOCK_DRIVER_MANAGED 的物理内存可以被驱动探测并添加到系统，然后由驱动独立使用. 至此 MEMBLOCK_DRIVER_MANAGED 研究完结.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="T4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

##### Boot Hotplug 内存研究

![](/assets/PDB/HK/TH002309.png)

内存热插拔是内核提供的一种机制，可以将 DIM、NVME 热插到系统内，然后通过 ACPI 机制通知系统有新的内存条插入到系统, 并将内存作为系统可用内存进行使用。如果将热插内存的时机按系统启动阶段进行划分，那么在内核启动阶段热插的物理内存称为 Boot Hotplug 内存，其热插进系统之后先由 MEMBLOCK 分配器进行管理; 如果在系统初始化之后进行内存热插，其热插进系统之后由 Buddy 分配器进行管理。MEMBLOCK 分配器将 boot 阶段热插的内存维护的 memblock.memory 区域里，并将热插的物理内存区域标记为 MEMBLOCK_HOTPLUG.

![](/assets/PDB/HK/TH002319.png)

内核处理 Boot-Plug 内存时的流程如上，热插的内存首先被 ACPI 感知到，内核在调用 numa_init() 函数过程中，系统回去遍历 ACPI 的 SRAT 表，以此获得内存硬件信息，其中在 acpi_numa_memory_affinity_init() 函数可以从 SRAT 表中获得热插的内存信息，其包含 ACPI_SRAT_MEM_HOT_PLUGGABLE 标志，接下来函数调用 memblock_mark_hotplug() 函数将物理内存区域标记为 MEMBLOCK_HOTPLUG. 但随着 NUMA 的初始化完毕，内核又调用 numa_clear_kernel_node_hotplug() 函数将所有的物理区域的 MEMBLOCK_HOTPLUG 标志清除，以此将热插和非热插的内存都看做 "System RAM" 使用，因此系统初始化完毕之后很难判断哪些是热插的内存. 接下来通过一个案例进行分析，其在 BiscuitOS 上的部署逻辑为:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK with Hotplug Memory  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-Hotplug-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-Hotplug-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-Hotplug)

![](/assets/PDB/HK/TH002317.png)
![](/assets/PDB/HK/TH002316.png)

在使用案例之前，需要在 RunBiscuitOS.sh 脚本中添加内存热插相关的命令行，那么系统在启动阶段就会自动热插一块物理内存。案例很简单，在内核启动后期在 23 行调用 for_each_mem_region() 函数遍历系统可用物理内存，也包括了刚刚热插的物理内存，并在每次遍历过程中调用 memblock_is_hotplugable() 函数判断物理内存区域是否热插的，如果是则打印物理内存区域信息，并记录区域的起始地址，然后将物理地址转换成虚拟地址，并在 31 行分支使用内存. 接着在 BiscuitOS 上实践案例:

![](/assets/PDB/HK/TH002318.png)

BiscuitOS 运行之后，从 Demsg 中可以看出案例并没有在 MEMBLOCK 的 memory 区域中找到热插的物理内存，但可以在 /sys/firmware/memmap 和 /sys/kernel/debug/memblock/memory 中找到热插的物理内存区域，因此 Boot Hotplug 的物理内存被当做了 "System RAM" 使用. 至此 Boot Hotplug 内存研究完毕.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="T2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Z.jpg)

##### NOMAP 内存研究

系统物理内存可以分为两大类，一类为系统可用物理内存，就是系统可以管理维护的物理内存; 另一类为系统预留内存，这部分物理内存真实存在，但不在操作系统的管理范围内。不同的架构维护系统预留内存的方式不同，例如在 X86 架构中，其使用 E820 表和 CMDLINE 的 memap 字段可以设置系统预留内存的范围。但有的架构没有 E820 表，那么其可以借助 MEMBLOCK 分配器提供的 MEMBLOCK_NOMAP 标志将系统预留内存进行独立区分

![](/assets/PDB/HK/TH002320.png)

例如在 ARM 架构中，其借助 MEMBLOCK 分配器实现了系统预留内存的功能，核心实现是: 将系统所有的物理内存存储在 memblock.memory 区域，然后将需要预留的区域从 memblock.memory 区域中分配出来，那么预留内存区域会再次维护在 memblock.reserved 区域，接着将分配出来的区域在 memblock.memory 区域中标记为 MEMBLOCK_NONE, 那么系统在建立页表时不会对 MEMBLOCK_NOMAP 的区域建立页表. 接下来通过一个实践案例进行讲解，其在 BiscuitOS 上的部署逻辑为:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK NOMAP Memory  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-NOMAP-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-NOMAP-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-NOMAP)

![](/assets/PDB/HK/TH002322.png)
![](/assets/PDB/HK/TH002321.png)

案例需要运行在 ARM 架构，并且需要在 vexpress-v2p-ca9.dts 文件中添加预留内存的描述，例如上上图在 ARM 架构中物理内存的范围从 memory 中知道为 \[0x60000000, 0xA0000000\], 那么在 reserved-memory 中新增 BiscuitOS 预留区域，其从 reg 指向的 0x70000000 开始，长度 size 为 0x02000000(32MiB), 并添加 alloc-range 属性指定了可接受的预留范围，最后添加 no-map 属性告诉系统不要为这段预留内存建立页表. 另外案例需要保持 CONFIG_ARCH_KEEP_MEMBLOCK 宏开启。回到案例其在 39 行调用 for_each_mem_region() 遍历 memblock.memory 区域里所有的 Region，案例在 40 行调用 memblock_is_nomap() 函数对每次遍历的 Region 检查其是否是 MEMBLOCK_NOMAP 区域，如果是则打印 Region 的范围，并记录 Region 的首地址. 最后函数在 47-51 行使用了 MEMBLOCK_NOMAP 的物理内存，默认情况下 TRIGGER_BUG 为 0，因此这段代码默认不会执行，那么接下来在 BiscuitOS 上实践案例:

![](/assets/PDB/HK/TH002323.png)

首先从 dmsg 中可以看到遍历 NO-MAP 的区域正好是 DTS 里配置的区域。接着查看 /sys/kernel/debug/memblock 的 memory 和 reserved 区域，均可以看到 DTS 中配置的 NO-MAP 区域. 最后查看 /proc/iomem 打印的信息，可以看到系统物理总线上对 DTS 中 NO-MAP 区域已经不属于 "System RAM" 范围了，因此实践的结果符合预期。接下来验证 NO-MAP 区域是否建立页表，可以在案例中将 TRIGGER_BUG 宏设置为 1，让案例去使用 NO-MAP 的内存，如果已经建立页表，那么可以直接使用 phys_to_virt() 函数直接获得虚拟机地址，然后直接使用。接下来将改动后的案例在 BiscuitOS 上实践:

![](/assets/PDB/HK/TH002324.png)

案例运行之后 BiscuitOS 直接挂死了，那么说明可以说明 NO-MAP 的物理内存确实没有建立页表。那么对于 NO-MAP 的预留内存，如果要使用该如何使用呢? 可以参考如下案例进行使用, 其在 BiscuitOS 上的部署逻辑为:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK NOMAP Memory with ioremap  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-NOMAP-IOREMAP-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-NOMAP-IOREMAP-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-NOMAP-IOREMAP)

![](/assets/PDB/HK/TH002325.png)

案例不同之处在于，当获得 NO-MAP 区域的物理地址之后，案例在 46 行调用 ioremap() 函数为预留物理内存建立映射，然后可以使用内存，使用完毕之后在 51 行调用 iounmap() 函数撤销物理内存的页表，这相当于归还预留物理内存，那么接下来在 BiscuitOS 上实践:

![](/assets/PDB/HK/TH002326.png)

BiscuitOS 启动之后，可以从 Dmesg 中看到案例成功的遍历到了 NO-MAP 的区域，并使用了 NO-MAP 的物理内存。接下来从源码角度分析 NO-MAP 预留内存的实现过程.

![](/assets/PDB/HK/TH002322.png)
![](/assets/PDB/HK/TH002327.png)

当需要在 ARM 架构里面新增一块 NO-MAP 的预留内存，那么需要在对应的 DTS 文件的 reserved-memory 节点里添加新的节点，例如上图的 BiscuitOS 节点，节点一定要包含 compatible、reg、size、alloc-ranges 和 no-map 属性，因此在系统启动阶段解析 DTS 是，fdt_init_reserved_mem() 函数会解析 reserved-mem 节点及其子节点，函数首先会解析子节点中带 "no-map" 属性的节点，然后以此解析子节点中一定要包含 size、no-map 和 alloc-ranges 属性之后，函数会最终调用到 early_init_dt_alloc_reserved_memory() 函数，其内部首先调用 memblock_phys_alloc_range() 函数从 memblock.memory 区域的 start 到 end 区域中分配需要预留长度的物理内存，然后把这些预留内存通过 memblock_mark_nomap() 函数标记为 MEMBLOCK_NOMAP 属性.因此就可以解释在 ARM 架构里，系统预留的物理内存既可以在 memblock.memory 中看到，又可以在 memblock.reserved 中看到. 至此 MEMBLOCK_NOMAP 内存研究到此结束.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="T7"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

##### Region Merge/Split 研究

![](/assets/PDB/HK/TH002295.png)

在 MEMBLOCK 分配器中，memblock.memory 区域维护了系统可用的物理内存，memblock.reserved 区域维护了已经使用的物理内存，physmem 中维护了系统所有的物理内存。这些区域都是通过 struct memblock_region 进行维护. Region 是 MEMBLOCK 分配器管理的最小单元. physmem 区域的 Region 包含了 memblock.memory 所有 Region，memblock.memory 区域 Region 包含了 memblock.reserved 所有 Region。

![](/assets/PDB/HK/TH002328.png)

struct memblock_region 的定义如上，其通过 base 成员描述了 Region 起始物理地址，size 成员描述了 Region 的长度，nid 成员描述了 Region 所属的 NUMA NODE，flags 成员描述了 Region 的属性。Region 一共了包含了 5 中属性:

* MEMBLOCK_NONE 表示普通可用物理内存
* MEMBLOCK_HOTPLUG 表示 Boot 阶段热插的物理内存
* MEMBLOCK_MIRROR 表示 Mirror 属性的物理内存
* MEMBLOCK_NOMAP 表示 ARM 架构中系不建立页表的系统预留内存
* MEMBLOCK_DRIVER_MANAGED 表示可由驱动探测和热插并使用的物理内存

MEMBLOCK 分配器规定了一个 Region 只能有一种属性，默认为 MEMBLOCK_NOMAP; 同一个区域内同一属性的 Region 不能重叠，不同属性的 Region 也不能重叠; 不同区域的同一属性的 Region 可以重叠. 由于有了这些限制，那么会出现不同 Region 之间在 MEMBLOCK 分配器分配、回收、新增时候的 Merge 和 Split，本节对该问题进行进一步研究:

###### 新增分离(Separation) Region

![](/assets/PDB/HK/TH002329.png)

当新增的 Region 与已经存在的 Region 位置上相离(Separation)，那么新增的 Region 不用与其他 Region 合并，作为一个新的 Region 插入，并且 memblock.memory 或者 memblock.reserved 中会重新排序. 接下来一个案例进行说明，其中 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK Region: Adding Separation Region  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-MERGE-Separation-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-MERGE-Separation-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-MERGE-Separation)

![](/assets/PDB/HK/TH002330.png)

案例很精简，案例在 25 行向 memblock.memory 区域新增 Region \[0x810000000, 0x810100000\], 然后在 29 行遍历 memblock.memory 区域。接着函数在 34-35 行调用 memblock_add() 函数向 memblock.memory 区域新增两个 Region \[0x800000000, 0x800100000\] 和 \[0x820000000, 0x820100000\]， 案例再次在 39 行遍历 memblock.memory 所有 Region。案例最后在 44-46 行调用 memblock_remove() 函数将新增区域移除。接下来在 BiscuitOS 上实践该案例:

![](/assets/PDB/HK/TH002331.png)

BiscuitOS 运行之后，从 Dmesg 可以看到打印的 LOG 显示新增的三块 Separation 区域在 memblock.memory 都是独立的 Region，并且已经按低地址到高地址的排好序。实践符合预期，那么符合这样的场景有:

* 通过 memblock_add() 向 memblock.memory 区域中新增分离物理区域
* 通过 memblock_add() 向 physmem 区域中新增分离的物理区域
* 通过 memblock_reserve() 向 memblock.reserved 区域新增分离物理区域
* 通过 memblock_alloc() 从独立的 Region 中分配整块物理内存

###### 相同属性新增相邻(Adjacent) Region

![](/assets/PDB/HK/TH002332.png)

当新增的 Region 与已经存在的 Region 位置上相邻(Adjacent)，并且新区域属性相同，那么新增的 Region 与其他 Region 合并，但不作为一个新的 Region 插入，而是保留原始 Region. 接下来一个案例进行说明，其中 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK Region: Adding Adjacent Region  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-MERGE-Adjacent-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-MERGE-Adjacent-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-MERGE-Adjacent)

![](/assets/PDB/HK/TH002333.png)

案例很精简，案例在 25 行向 memblock.memory 区域新增 Region \[0x800100000, 0x800200000\], 然后在 29 行遍历 memblock.memory 区域。接着函数在 34-35 行调用 memblock_add() 函数向 memblock.memory 区域新增两个 Region \[0x800000000, 0x800100000\] 和 \[0x8000200000, 0x800300000\]， 案例再次在 39 行遍历 memblock.memory 所有 Region。案例最后在 44-46 行调用 memblock_remove() 函数将新增区域移除。接下来在 BiscuitOS 上实践该案例:

![](/assets/PDB/HK/TH002334.png)

BiscuitOS 运行之后，从 Dmesg 可以看到打印的 LOG 显示新增的三块 Adjacent 区域在 memblock.memory 都是属性相同且相邻的 Region，最后都合并成同一个 Region，并使用原始的 Region。实践符合预期，那么符合这样的场景有:

* 通过 memblock_add() 向 memblock.memory 区域中新增属性相同且相邻物理区域
* 通过 memblock_add() 向 physmem 区域中新增相邻的物理区域
* 通过 memblock_mark_hotplug() 标记两块相邻的区域
* 通过 memblock_mark_driver_managed() 标记两块相邻的区域

###### 不同属性新增相邻(Adjacent) Region

![](/assets/PDB/HK/TH002335.png)

当新增的 Region 与已经存在的 Region 位置上相邻(Adjacent)，但新区域属性不相同，那么新增的 Region 与其他 Region 不合并，作为一个新的 Region 插入，并且 memblock.memory 或者 memblock.reserved 中会重新排序. 接下来一个案例进行说明，其中 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK Region: Adding Adjacent Region with Diff flags  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-MERGE-Adjacent-Diff-flags-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-MERGE-Adjacent-Diff-flags-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-MERGE-Adjacent-Diff-flags)

![](/assets/PDB/HK/TH002336.png)

案例很精简，案例在 25 行向 memblock.memory 区域新增 Region \[0x800000000, 0x800300000\], 然后在 29 行遍历 memblock.memory 区域。接着函数在 34-35 行调用 memblock_mark_hotplug() 函数将 memblock.memory 区域标记两个 Region \[0x800000000, 0x800100000\] 和 \[0x8000200000, 0x800300000\] 为 MEMBLOCK_HOTPLUG， 案例再次在 39 行遍历 memblock.memory 所有 Region。案例最后在 44 行调用 memblock_remove() 函数将新增区域移除。接下来在 BiscuitOS 上实践该案例:

![](/assets/PDB/HK/TH002337.png)

BiscuitOS 运行之后，从 Dmesg 可以看到打印的 LOG 显示新增的三块 Adjacent 区域在 memblock.memory 都是属性不相同但相邻的 Region，最后没有合并成同一个 Region，而是独立的三个 Region，且 Region 按顺序重排。实践符合预期，那么符合这样的场景有:

* 通过 memblock_mark_hotplug() 将部分区域标记为 MEMBLOCK_HOTPLUG
* 通过 memblock_mark_driver_managed() 将部分区域标记

###### 相同属性新增相邻(Intersection) Region

![](/assets/PDB/HK/TH002338.png)

当新增的 Region 与已经存在的 Region 位置上相邻(Adjacent)，但新区域属性不相同，那么新增的 Region 与其他 Region 不合并，作为一个新的 Region 插入，并且 memblock.memory 或者 memblock.reserved 中会重新排序. 接下来一个案例进行说明，其中 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK Region: Adding Intersection Region  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-MERGE-Intersection-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-MERGE-Intersection-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-MERGE-Intersection)

![](/assets/PDB/HK/TH002339.png)

案例很精简，案例在 25 行向 memblock.memory 区域新增 Region \[0x800100000, 0x800200000\], 然后在 29 行遍历 memblock.memory 区域。接着函数在 34-35 行调用 memblock_add() 函数向 memblock.memory 区域新增两个 Region \[0x800010000, 0x801100000\] 和 \[0x8000120000, 0x800220000\] ， 案例再次在 39 行遍历 memblock.memory 所有 Region。案例最后在 44 行调用 memblock_remove() 函数将新增区域移除。接下来在 BiscuitOS 上实践该案例:

![](/assets/PDB/HK/TH002340.png)

BiscuitOS 运行之后，从 Dmesg 可以看到打印的 LOG 显示新增的三块 Intersection 区域在 memblock.memory 都是属性相同但相交的 Region，最后合并成同一个 Region，且使用原来的 Region。实践符合预期，那么符合这样的场景有:

* 通过 memblock_add() 向 memblock.memory 区域新增相交的物理区域
* 通过 memblock_add() 向 physmem 区域新增相交的物理区域
* 通过 memblock_reserved() 预留相交的物理区域

###### 不同属性新增相交(Intersection) Region

![](/assets/PDB/HK/TH002335.png)

当新增的 Region 与已经存在的 Region 位置上相交(Intersection)，但新区域属性不相同，那么新增的 Region 与其他 Region 不合并，作为一个新的 Region 插入，并且 memblock.memory 或者 memblock.reserved 中会重新排序. 接下来一个案例进行说明，其中 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] MEMBLOCK Memory Allocator  --->
          [*] MEMBLOCK Region: Adding Intersection Region with Diff flags  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-MEMBLOCK-MERGE-Intersection-Diff-flags-default
{% endhighlight %}

> [BiscuitOS-MEMBLOCK-MERGE-Intersection-Diff-flags-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MEMBLOCK/BiscuitOS-MEMBLOCK-MERGE-Intersection-Diff-flags)

![](/assets/PDB/HK/TH002341.png)

案例很精简，案例在 25 行向 memblock.memory 区域新增 Region \[0x800100000, 0x800200000\], 然后在 29 行遍历 memblock.memory 区域。接着函数在 34-35 行调用 memblock_mark_hotplug() 函数将 memblock.memory 区域标记两个 Region \[0x800010000, 0x800110000\] 和 \[0x8000120000, 0x800220000\] 为 MEMBLOCK_HOTPLUG， 案例再次在 39 行遍历 memblock.memory 所有 Region。案例最后在 44 行调用 memblock_remove() 函数将新增区域移除。接下来在 BiscuitOS 上实践该案例:

![](/assets/PDB/HK/TH002342.png)

BiscuitOS 运行之后，从 Dmesg 可以看到打印的 LOG 显示新增的三块 Intersection 区域在 memblock.memory 都是属性不相同但相交的 Region，最后没有合并成同一个 Region，而是独立的三个 Region，且 Region 按顺序重排。实践符合预期，那么符合这样的场景有:

* 通过 memblock_mark_hotplug() 将部分区域标记为 MEMBLOCK_HOTPLUG
* 通过 memblock_mark_driver_managed() 将部分区域标记

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="T8"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

##### 分配器调试研究

MEMBLOCK 分配器提供了调试功能，可以对每次分配、回收和区域修改输出调试信息。默认情况下内核关闭了 MEMBLOCK 分配器的调试功能，如果需要可以通过在 CMDLINE 中添加 "memblock=debug" 字段进行开启, 开启之后的效果如下:

![](/assets/PDB/HK/TH002346.png)

可以在 Dmesg 中看到很多的打印信息，其中包括调用的函数名字、分配或释放的长度、范围以及调用位置信息。可以利用这些信息对某个某些 MEMBLOCK 分配器行为进行跟踪，或者某些子系统在 Boot 阶段使用物理内存进行跟踪。

![](/assets/PDB/HK/TH002344.png)
 
当内核启用 CONFIG_DEBUG_FS 和 CONFIG_ARCH_KEEP_MEMBLOCK 两个宏之后，MEMBLOCK 分配器可以一直存在，那么可以在 /sys/kernel/debug/memeblock 节点下获得 MEMBLOCK 分配器的信息，其中 memory 节点代表 memblock.memory 区域信息，physmem 节点代表 physmem 区域信息，reserved 节点代表 memblock.reserved 区域信息。信息按 Region 的顺序、起始物理地址和结束物理地址组成.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

#### MEMBLOCK 物理内存分配器使用场景

------------------------------------------

----------------------------------------------

<span id="X0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

##### Mirror Memory 应用场景 

![](/assets/PDB/HK/TH002347.png)

Mirror Memory 是硬件提供了一种内存高可用机制，当系统启用 Mirror Memory 之后，其会将内存分作两份，其中一份为系统正常使用的，另外一部分用作备份。当系统对 Mirror Memory 进行写的时候，内存控制器会同时写入两份内存里，当系统读取内存时，内存控制器同时从两份内存中读取数据，然后比较一致之后在返回值给系统。当 Mirror Memory 发送了 UE(Uncorrect Error) 之后，系统在读取 UE 内存时，内存控制器直接从备份中返回数据，这样避免系统消费到 UE 内存而宕机. 更多细节可以参考:

> [《MEMBLOCK 分配器 Mirror Memory 研究》](#T0)

![](/assets/PDB/HK/TH002309.png)

MEMBLOCK 分配器提供了标记物理区域为不同属性的能力，其中 MEMBLOCK_MIRROR 属性表示物理区域为 Mirror Memory。并向外提供了 memblock_mark_mirror() 函数标记物理内存为 MEMBLOCK_MIRROR 属性，并且提供了方法用于分配 Mirror Memory. 

![](/assets/PDB/HK/TH002348.png)

BIOS 负责开启或关闭 Mirror Memory 的功能，然后系统通过 UEFI 获得 Mirror Memory 的具体信息。在 UEFI 初始化过程中，内核调用 efi_find_mirror() 函数获得 EFI 中保存的 Mirror Memory 信息，当找到之后调用 memblock_mark_mirror() 函数，其在 MEMBLOCK 分配器的 memblock.memory 区域中将指定区域标记为 MEMBLOCK_MIRROR, 待系统继续初始化，MEMBLOCK_MIRROR 物理内存会有 Buddy 进行管理并提供给系统使用.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="X1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

##### 系统预留内存应用场景

对于 Linux 来说，物理内存可以分为三类: 第一类系统可以维护和使用的物理内存，称为系统可用物理内存; 第二类是系统无法管理留给特定模块使用的物理内存，称为系统预留内存; 第三类为内存控制器看到的物理内存，暂时描述为 DDR 内存。三者的管理是: DDR 内存是系统可用物理内存与系预留内存的总和 (不考虑 SMM). 在不同的架构中对系统预留内存的管理各不相同，例如在 X86 中借助 E820 表维护，又例如 ARM 架构其通过 MEMBLOCK 分配器进行管理。那么本节描述系统使用 MEMBLOCK 分配器管理系统预留内存.

![](/assets/PDB/HK/TH002295.png)

MEMBLOCK 分配器除了提供 memblock.memory 和 memblock.reserved 区域之后，额外提供了 physmem 区域，physmem 区域表示的就是 DDR 内存，那么简单的从 physmem 区域中移除 memblock.memory 之后就是系统预留内存。

![](/assets/PDB/HK/TH002344.png)

基于这个原理在启用 CONFIG_ARCH_KEEP_MEMBLOCK 的系统里，可以在 /sys/kernel/debug/memblock 中获得 memblock.memory 和 physmem 的信息，那么通过两个区域就可以知道系统预留内存的信息.

![](/assets/PDB/HK/TH002320.png)

MEMBLOCK 分配器在 ARM 架构还提供了另外一种方法管理系统预留内存，其通过 DTS 将需要预留的内存标记为 no-map 属性，内核在启动解析 DTS 时，会按 no-map 节点的描述在 memblock.memory 中找到指定的区域，然后将这部分内存区域标记为 MEMBLOCK_NOMAP，并将这部分区域也添加在 memblock.reserved 区域，这样系统不会为这段区域的物理内存建立页表，因此系统也无法直接使用这些物理内存，最后这些物理内存也就称为系统预留内存. 更多技术细节可以参考:

> [《MEMBLOCK 分配器 physmem 区域研究》](#T1)
>
> [《MEMBLOCK 分配器 NO-MAP 研究》](#T2)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="X2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

##### CRASH VMCORE 应用场景

内核核心转储又成为 core dump，在 Unix/Linux 中，将主内存 "Main Memory" 称为核心 core, 这是因为在半导体作为内存材料前，便使用核心 "core" 表示内存。另外核心镜像 "core image" 就是内核作为一个内核线程执行时在内存中的内容。当内核线程发生错误或者收到特定信号而终止执行时，系统在借助 KEXEC 工具的情况下，可以将核心镜像写入一个文件，以便后期调试问题之用，这个过程就是所谓的核心转储 "core dump". 系统会在系统预留一段物理内存用于存在 Kdump 系统的镜像，这段物理内存预留之后不能被其他使用。当系统发生 CoreDump 时，系统会启动 Kdump 系统 Dump 内存。由于 Kdump 系统预留的内存需要在系统启动阶段就需要进行预留，那么可以借助 MEMBLOCK 分配器提供的能力实现.

![](/assets/PDB/HK/TH002349.png)

要开启 Kdump 功能需要在 CMDLINE 中添加 "crashkernel=" 字段指明 Kdump 预留内存的大小或者起始物理地址，然后内存在启动过程中调用 reserve_crashkernel() 函数解析 "crashkernel=" 字段，然后获得预留内存的大小，然后调用 memblock_phys_alloc_range() 函数分配指定长度的物理内存，一旦分配出去只能 kdump 独占使用，最后函数更新 crash_res 的信息，最后调用 insert_resource() 函数将其添加到系统地址总线资源树上.

![](/assets/PDB/HK/TH002350.png)

系统启动之后，可以从 /proc/iomem 中看到 Crash kernel 占用了 \[37000000, 3effffff\] 区域，一共 128MiB, 因此 MEMBLOCK 分配器可以为 CRASH 机制提供早期的物理内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="X3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

##### Hugetlb 1G 大页应用场景

![](/assets/PDB/HK/TH001196.png)

在 Hugetlb 大页机制中，内核可以向用户空间进程提供 1Gig 的物理大页，1Gig 的物理大页可以通过动态分配，也可以在系统启动阶段就分配。本节用于介绍 MEMBLOCK 分配器在启动阶段实现 1Gig 物理大页分配场景, 在分析之前，系统为了在启动阶段分配 1Gig 的 hugetlb 大页，需要在 CMDLINE 中添加 "hugepagesz=1G hugepages={num}" 字段，其中 num 指明分配 1Gig Hugetlb 大页的数量.

![](/assets/PDB/HK/TH002351.png)

内核启动时解析 "hugepagesz=" 时调用 hugepages_setup() 函数，在函数 hugetlb_max_hstate 的值来自 "hugepages=" 字段，此时判断该值不为零，且 hstate_is_gigantic() 函数有效，那么内核认为现在需要为分配 1Gig 的 Hugetlb 大页，接下来函数调用 hugetlb_hstate_alloc_pages() 函数进行分配，其内部再次检查 hugetlb_max_hstate，当该值不为 0 时且分配 1Gig 大页，那么函数最终调用 alloc_bootmem_huge_page() 函数在 boot 阶段分配 1Gig 大页。函数最终调用到 memblock_alloc_try_nid_raw() 函数进行实际的内存分配，那么 1Gig 物理大页所在的区域就被添加到 memblock.reserved 区域，后续 Buddy 分配器将占用的 struct page 全部标记为 PageReserved 标志.

![](/assets/PDB/HK/TH002352.png)

系统启动之后，可以在 /sys/kernel/mm/hugepages/hugepages-1048576kB/ 目录下查看 1Gig Hugetlb 大页的情况，通过上图可以看出 1Gig Hugetlb 大页池子中有一个是空闲的，没有超发的大页. 以上便是 MEMBLOCK 分配器在 1Gig 大页的应用场景.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------
