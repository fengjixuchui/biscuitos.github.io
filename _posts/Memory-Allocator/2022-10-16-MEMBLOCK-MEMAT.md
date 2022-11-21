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
> - [MEMBLOCK 物理内存分配器 API 合集](#B5)
>
> - [MEMBLOCK 物理内存分配器使用场景](#X)
>
> - MEMBLOCK 物理内存分配器 BUG 合集
>
> - MEMBLOCK 物理内存分配器进阶研究

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

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

MEMBLOCK 物理内存分配器维护了早期系统的物理内存，其将物理内存划分成多个长度不同的 Region，Region 是 MEMBLOCK 分配器最小的管理单元. MEMBLOCK 继续将每个 Region 划分成两大类: **memory 区域**为 MEMBLOCK 分配器管理的物理内存，**reseved 区域**为 MEMBLO-CK 分配器已经分配的物理内存。

![](/assets/PDB/HK/TH002141.png)

MEMBLOCK 分配器使用 struct memblock、struct memblock_type 和 struct memblock_reg-ion 三个数据结构体进行描述。struct memblock_region 表示 Region，其描述了物理区域的起始物理地址和长度、所属 NUMA NODE 以及标识信息; struct memblock_type 表示物理内存的类型，其描述某种类型当前 Region 的数量、最大支持 Region 数量、维护总物理内存量、内存类型的名字以及 Region 位置, MEMBLOCK 分配器只支持 memory(可分配物理内存) 和 reserved(已经被使用物理内存) 两种类型; struct memblock 对 MEMBLOCK 分配器的整体抽象，其描述分配器分配的方向、最大可分配物理内存、memory 和 reserved 区域.

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

#### MEMBLOCK 物理内存分配器使用 

> <span id="B5"></span>
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

###### <span id="B5A">memblock_alloc_exact_nid_raw</span>
