---
layout: post
title:  "Linux 4.x Kernel Virtual Space"
date:   2019-01-04 09:17:30 +0800
categories: [MMU]
excerpt: Linux 4.x Kernel Virtual Space.
tags:
  - MMU
---

# 目录

> - [内核空间虚拟内存布局](#内核空间虚拟内存布局)
>
> - [内核空间虚拟实践](#内核空间虚拟实践)
>
>   - [DMA 虚拟内存区](/blog/MMU-Linux4x-DMA/)
>
>   - [Normal 虚拟内存区](/blog/MMU-Linux4x-Normal/)
>
>   - [高端虚拟内存区](/blog/MMU-Linux4x-VMALLOC/)
>
>   - [VMALLOC 分配虚拟内存区](/blog/MMU-Linux4x-VMALLOC/)
>
>   - [MODULE 虚拟内存区](/blog/MMU-Linux4x-MODULE/)
>
>   - [持久映射虚拟内存区](/blog/MMU-Linux4x-PKMAP/)
>
>   - [固定映射虚拟内存区](/blog/MMU-Linux4x-FIXUP/)
>
>   - [临时映射虚拟内存区](/blog/MMU-Linux4x-FIXUP/)
>
> - [相关拓展](#内核空间虚拟实践)
>
> - [附录](#附录)

---------------------------------------

# <span id="内核空间虚拟内存布局">内核空间虚拟内存布局</span>

在 IA32 系统上内核通常将总的 4 GiB 可用虚拟地址空间按 3：1 的比例划分。低端 
3 GiB 用于用户态应用程序，而高端的 1 GiB 则专门用于内核。按 3：1 的比例划分
地址空间，只是约略反应了内核中的情况，内核地址空间自身分为各个段，如下图：

{% highlight ruby %}
4G +----------------+
   |                |
   +----------------+-- FIXADDR_TOP
   |                |
   |                | FIX_KMAP_END
   |     Fixmap     |
   |                | FIX_KMAP_BEGIN
   |                |
   +----------------+-- FIXADDR_START
   |                |
   |                |
   +----------------+--
   |                | A
   |                | |
   |   Persistent   | | LAST_PKMAP * PAGE_SIZE
   |    Mappings    | |
   |                | V
   +----------------+-- PKMAP_BASE
   |                |
   +----------------+-- VMALLOC_END / MODULE_END
   |                |
   |                |
   |    VMALLOC     |
   |                |
   |                |
   +----------------+-- VMALLOC_START / MODULE_VADDR
   |                | A
   |                | |
   |                | | VMALLOC_OFFSET
   |                | |
   |                | V
   +----------------+-- high_memory
   |                |
   |                |
   |                |
   | Mapping of all |
   |  physical page |
   |     frames     |
   |    (Normal)    |
   |                |
   |                |
   +----------------+-- MAX_DMA_ADDRESS
   |                |
   |      DMA       |
   |                |
   +----------------+
   |     .bss       |
   +----------------+
   |     .data      |
   +----------------+
   | 8k thread size |
   +----------------+
   |     .init      |
   +----------------+
   |     .text      |
   +----------------+ 0xC0008000
   | swapper_pg_dir |
   +----------------+ 0xC0004000
   |                |
3G +----------------+-- TASK_SIZE / PAGE_OFFSET
   |                |
   |                |
   |                |
0G +----------------+
{% endhighlight %}

地址空间的第一段用于将系统的所有物理内存页映射到内核的虚拟地址空间中。由于内核
地址空间从偏移量 0xC000 0000 开始，即经常提到的 3GiB， 每个虚拟地址 X 都对应物
理地址 x -- 0xC000 0000，因此这是一个简单的线性平移。

直接映射区域从 0xC000 0000 到 high_memory 地址。由于内核的虚拟地址空间只有 
1 GiB，最多只能映射 1 GiB 物理内存。IA32 系统（PAE 未开启）最大的内存配置可达到
4 GiB，引出的一个问题，如何处理剩下的内存？IA32 中，如果物理内存超过 896 MiB，
则内核无法直接映射全部物理内存。该值甚至比此前提到的最大限制 1 GiB 还小，因为
内核必须保留地址空间最后 128 MiB 用于其他目的。这将 128 MiB 加上直接映射的 
896 MiB 内存，则得到内核虚拟地址空间的总数为 1024 MiB = 1 GiB。内核使用两个经
常使用的缩写 normal 和 highmem，来区分是否可以直接映射的页帧。

### 内核高端的 128 Mib 虚拟内存布局

内核高端的 128 MiB 虚拟内存主要分为三个部分：

> 1. [虚拟地址连续但物理地址不连续的虚拟内存区](#虚拟地址连续)
>
> 2. [持久映射虚拟内存区](#持久映射虚拟内存区)
>
> 3. [固定映射虚拟内存区](#固定映射虚拟内存区)

### <span id="虚拟地址连续">虚拟地址连续但物理地址不连续的虚拟内存区</span>

该虚拟内存区域起始地址是 VMALLOC_START, 终止地址是 VMALLOC_END，内核通过 
vmalloc() 函数分配该区域内的虚拟内存。该机制常用于用户过程，内核自生会试图尽
力避免非连续的物理地址。内核通常会成功，因为大部分大的内存块都在启动时分配给内
核，那时内存的碎片尚不严重。但在已经运行了很长时间的系统上，在内核需要物理内存
时，就可能出现可用空间不连续的情况。此类情况，主要出现在动态加载模块时。动态加
载模块使用的虚拟地址也是在这段区域内。[VMALLOC 虚拟内存实践](/blog/MMU-Linux4x-VMALLOC/)

### <span id="持久映射虚拟内存区">持久映射虚拟内存区</span>

该虚拟内存区域起始地址是 PKMAP_BASE，该区域的大小为 LAST_PKMAP * PAGE_SIZE，
IA32 体系中，LAST_PKMAP 通常为 1025，所以持久映射虚拟内存的大小为 1024 * 
4096 = 4 MiB。持久映射用于将高端内存区域中的非持久页映射到内核中。使用 kmap() 
函数分配该区域内的虚拟内存。[PKMAP 虚拟内存实践](/blog/MMU-Linux4x-PKMAP/)

### <span id="固定映射虚拟内存区">固定映射虚拟内存区</span>

该虚拟内存区域起始地址是 FIXADDR_START，终止地址是 FIXADDR_TOP，内核在编译阶段
就设置好该区域的虚拟内存布局。内核也在该区域为每个 CPU 核留了一段可以零时映射的
区域，该区域称为临时映射虚拟内存区，该区域起始索引是 FIX_KMAP_BIGEN，终止索引是 
FIX_KMAP_END，内核可以通过 fix_to_virt() 和 set_fixmap() 函数进行该虚拟内存区域
的分配。固定映射是与物理地址空间中的固定页关联的虚拟地址空间项，但具体关联的页
帧可以自由选择。它与通过固定公式与物理内存关联的直接映射页相反，虚拟固定映射地
址与物理内存位置之间的关联可以自行定义，关联建立后内核总是会看到。[固定映射/临
时映射实践](/blog/MMU-Linux4x-FIXUP/)

#### 内核保留虚拟内存区域

在 IA32 体系中，PAGE_OFFSET 开始处的虚拟内存保留给系统内核使用，这段虚拟内存包
含了内核的代码段，数据段，初始化数据段，BSS 段，全局页目录 swapper_pg_dir, 和
第一个页表等系统预留的虚拟区域。该区域的段和用户空间的段属性类似，因为内核本身
也是一个大的程序，所以也必须包含程序运行时需要的各种段数据。

------------------------------------------------

# <span id="内核空间虚拟实践">内核虚拟地址实践</span>

内核虚拟地址空间主要分做以下几部分：

> - [DMA 虚拟内存区](/blog/MMU-Linux4x-DMA/)
>
> - [Normal 虚拟内存区](/blog/MMU-Linux4x-Normal/)
>
> - [高端虚拟内存区](/blog/MMU-Linux4x-VMALLOC/)
>
> - [VMALLOC 分配虚拟内存区](/blog/MMU-Linux4x-VMALLOC/)
>
> - [MODULE 虚拟内存区](/blog/MMU-Linux4x-MODULE/)
>
> - [持久映射虚拟内存区](/blog/MMU-Linux4x-PKMAP/)
>
> - [固定映射虚拟内存区](/blog/MMU-Linux4x-FIXUP/)
>
> - [临时映射虚拟内存区](/blog/MMU-Linux4x-FIXUP/)

-------------------------------------------

# <span id="附录">附录</span>

打赏一下吧 :-)

![LD](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
