---
layout: post
title:  "Buddy Allocator"
date:   2020-05-07 09:37:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [buddy 分配器原理](#A)
>
> - [buddy 分配器使用](#B)
>
> - [buddy 分配器实践](#C)
>
> - [buddy 源码分析](#D)
>
> - [buddy 分配器调试](#E)
>
> - [buddy 分配进阶研究](#F)
>
> - [buddy 时间轴](#G)
>
> - [buddy 历史补丁](#H)
>
> - [buddy API](#K)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### buddy 分配器原理

![](/assets/PDB/RPI/RPI000827.png)

Buddy 内存管理器用于管理 Linux 内核中可用的物理内存。在 Linux 内核中，将
物理内存以 PAGE_SIZE 为单位，划分成大量的内存区域，这些内存区域称为物理页，
或称为 "Page"。Linux 将每个物理页与物理地址进行关联进行编号，每个物理页就有
一个唯一的 ID，这个 ID 称为物理页帧号，或称为 "Page Fram"。在不同的体系结构
中，物理内存的物理地址不一定都是从地址 0 开始，因此物理页帧不一定从 0 开始
编号。物理页、物理页帧与物理内存之间的关系如下图:

![](/assets/PDB/RPI/RPI000828.png)

正如上图所示，物理内存的起始地址是 PHYS_OFFSET, 物理内存以 PAGE_SIZE 为单位
被划分许多内存区域 "Memory Region", 将这些内存区域按物理地址从低到高一次
标号，因此就形成了物理页帧号 "Page Frame". 由于 PHYS_OFFSET 在不同的体系结构
中数值不一样，但可以得到其之间的关系:

{% highlight bash %}
Region0 起始地址 = PHYS_OFFSET + Fram0 * PAGE_SIZE
                 = PHYS_OFFSET + 0 * PAGE_SIZE
                 = PHYS_OFFSET
                 = ((PHYS_OFFSET >> PAGE_SHIFT) << PAGE_SHIFT) + (((0 * PAGE_SIZE) >> PAGE_SHIFT) << PAGE_SHIFT)
                 = (PHYS_OFFSET >> PAGE_SHIFT + 0) << PAGE_SHIFT
Region1 起始地址 = PHYS_OFFSET + Fram1 * PAGE_SIZE
                 = PHYS_OFFSET + 1 * PAGE_SIZE
                 = ((PHYS_OFFSET >> PAGE_SHIFT) << PAGE_SHIFT) + (((Fram1 * PAGE_SIZE) >> PAGE_SHIFT) << PAGE_SHIFT)
                 = ((PHYS_OFFSET >> PAGE_SHIFT + ((Fram1 * PAGE_SIZE) >> PAGE_SHIFT))) << PAGE_SHIFT
                 = (PHYS_OFFSET >> PAGE_SHIFT + Fram1) << PAGE_SHIFT
Region2 起始地址 = PHYS_OFFSET + Fram2 * PAGE_SIZE
                 = PHYS_OFFSET + 2 * PAGE_SIZE
                 = ((PHYS_OFFSET >> PAGE_SHIFT) << PAGE_SHIFT) + (((Fram2 * PAGE_SIZE) >> PAGE_SHIFT) << PAGE_SHIFT)
                 = ((PHYS_OFFSET >> PAGE_SHIFT + ((Fram2 * PAGE_SIZE) >> PAGE_SHIFT))) << PAGE_SHIFT
                 = (PHYS_OFFSET >> PAGE_SHIFT + Fram2) << PAGE_SHIFT
{% endhighlight %}

从上面的推到关系可以反推得到:

![](/assets/PDB/RPI/RPI000829.png)

{% highlight bash %}
物理地址 = PHYS_OFFSET + 物理页帧号 << PAGE_SHIFT
PhysAddr = PHYS_OFFSET + PAGE_Frame << PAGE_SHIFT
物理页帧号 = 物理地址 >> PAGE_SHIFT
PAGE_Frame = PhysAddr >> PAGE_SHIFT
{% endhighlight %}

经过上面的讨论，已经明确了物理地址和物理页帧之间的关系，Linux 习惯称物理
地址为 "PA"、"phys" 或者 "pa"，称物理页帧为 "PFN" 或 pfn，称物理页为 "page",
内核提供了多个三者之间的转换函数:

{% highlight bash %}
page = pfn_to_page(pfn)
page = phys_to_page(pa)
pfn  = page_to_pfn(page)
pfn  = phys_to_pfn(pa)
pa   = page_to_phys(page)
pa   = pfn_to_phys
{% endhighlight %}

Linux 内核将物理内存通过物理页，物理页帧和物理地址定义管理之后，又以物理
页为单位，根据物理页的用途，将物理页划分到不同的区间，Linux 通常使用 Zone 
来表示区间。一个 Zone 里面包含多个物理页，Zone 定义了这些物理页的用途范围。
Linux 目前支持的 Zone 类型有 ZONE_DMA、ZONE_DMA32、ZONE_NORMAL 和 
ZONE_HIGHMEM. 内核使用 struct zone 来管理一个分区里面的所有物理页的使用，
包括当前分区物理页数量、可用物理页数量、已经分配物理页数量等信息。Liux
Zone 划分如下图:

![](/assets/PDB/RPI/RPI000735.png)

不同的架构 ZONE 的划分不一样，但基本已上图类似。在上图中定义了 4 个 Zone，
其中 ZONE_DMA 和 ZONE_DMA32 位于物理内存的最前端 (但不是一定位于最前端)，
主要是给一些 ISA 总线等总线上 I/O 设备使用的物理内存, 或者是给架构中 DMA
直接使用的物理内存，也可作为普通物理内存使用. ZONE_NORMAL 区间内的物理页
用于内核日常运行所使用的普通物理内存，多数架构中 ZONE_NORMAL 区间内存的
物理内存都与虚拟地址进行一一映射，即线性映射，但值得注意的是映射和物理内存
是相互独立又相互关联的存在。ZONE_HIGMEM 区间是在一些架构中存在，主要是由于
虚拟内存不够，不能将物理内存与虚拟内存一一映射的这类物理页，统一放到了
ZONE_HIGHMEM 区间内，Linux 内核的 VMALLOC，KMAP 和 FIXMAP 等一些内存分配
器会将 ZONE_HIGHMEM 区间内的物理页通过一定的逻辑关系映射到虚拟地址，以供
进程(内核进程、内核线程或用户进程)使用。Linux 内核使用 struct zone 管理
一个 ZONE 分区。

###### Buddy 分配器初始化

![](/assets/PDB/RPI/RPI000827.png)

有了上面的基础数据介绍之后，那么开始讨论今天的主角 Buddy 物理内存管理器。
Buddy 物理内存管理器用于管理 Zone 区间内的物理页，包括物理页的申请、释放
和回收。正如上图所示，struct zone 管理一个 ZONE 分区，其中 free_area
成员一个链表集合，总共包含 MAX_ORDER 个链表。每个链表用于维护一定的物理页块，
这里的物理页块指的是一个或多个物理页的集合。在 struct zone 结构中， 
free_area[0] 维护的物理页数为 1 的物理页块; free_area[1] 维护物理页数 2
的物理页块; free_area[2] 维护物理页数为 4 的物理页块. 以此类推，free_area[n]
维护物理页数为 2^n 的物理页块.

![](/assets/PDB/RPI/RPI000831.png)

Linux 内核初始化过程中，bootmem/MEMBLOCK 内存分配器初始化完物理内存之后，
使用循环将可以物理内存按一定的长度转移给 Buddy 内存分配器。这里值得注意
的是 bootmem 分配器和 MEMBLOCK 分配器每次转移的物理内存块长度不同。对于
bootmem 分配器转移物理块的最大长度是 BITS_PER_LONG 个物理页集合，最小长度
是一个物理页; MEMBLOCK 分配器转移的物理块的最大长度是 2^MAX_ORDER 个物理
页集合，最小长度是一个物理页。无论是 bootmem 分配器还是 MEMBLOCK 分配器，
Buddy 内存管理器在介绍到物理页块之后，按最大 2 的幂次个物理页对物理页块
进行拆分，正如上图所示，被才分成三个块，每个块都包含 2 的倍数个物理页。

![](/assets/PDB/RPI/RPI000830.png)

Buddy 内存分配器将拆分只有的物理页块依次插入到指定的 free_area[] 链表中，
在每次插入过程中，Buddy 内存分配器先找到物理页块对应的 free_area[] 链表.

![](/assets/PDB/RPI/RPI000833.png)

Buddy 内存管理器在指定的 free_area[] 链表中遍历所有的成员，查找是否与
即将插入的物理页块存在 "Buddy" 关系，何为 Buddy 关系呢? 正如上图所示，
例如在物理页集合 PageSet0 中，其包含 2^N 个物理页，在这个物理页集合中，
第一个物理页的页帧号为 PFN0, 那么其 "Buddy" 物理页帧号的关系如下:

{% highlight bash %}
PFN_Buddy  = PFN0 ^ (1 << N)
{% endhighlight %} 

N 代表物理页块的幂次，通过上面的公式就可以获得一个 PageSet 的 "Buddy"
PageSet 的页帧号，如上图所示，PageSet0 和 PageSet1 是 "Buddy"; PageSet0
与 PageSet1 组成的物理块和 PageSet2 与 PageSet3 组成的物理块是 "Buddy";
PageSet0、PageSet1、PageSet2、PageSet3 组成的物理页块与 PageSet4、PageSet5、
PageSet6、PageSet7 组成的物理页块是 "Buddy" 关系。

![](/assets/PDB/RPI/RPI000832.png)

当在 free_area[] 指定的链表中遍历所有的成员，并未发现存在插入物理页块的
"Buddy" 物理页块，那么 Buddy 就按上图所示的插入到 free_area[] 指定链表的
尾部。

![](/assets/PDB/RPI/RPI000834.png)

当 Buddy 管理器遍历 Free_area[] 指定链表的所有成员时，找到了即将插入物理页块
的 "Buddy" 时，Buddy 管理器继续检测找到的 "Buddy" 物理页块是否满足条件进行
合并，如果满足合并条件，那么 Buddy 管理器就将 "Buddy" 物理页块从指定的 
free_area[] 链表中移除，并与插入的物理页块合成一个更大的物理页块。Buddy
内存分配器继续重复上面的逻辑，继续在下一级 free_area[] 链表中查找 "Buddy"
物理页块，直到找不到 "Buddy" 物理页块或者已经到达 free_area[] 最后一个链表，
那么 Buddy 内存分配器就将物理页块插入到指定的 free_area[] 链表中。

Buddy 内存管理器将 bootmem/MEMBLOCK 分配器转移的物理页块全部加入到 free_area[]
链表之后，bootmem/MEMBLOCK 分配器任务就完成，内核将弃用 bootmem/MEMBLOCK
内存分配器，之后 Buddy 内存分配器将管理可用的物理内存，至此 Buddy 内存分配器
初始化完毕. 从上面的讨论可以知道，每个 ZONE 分区都维护自己的 free_area[]
链表集合，那么对于单 node 的系统，系统中有多少个 ZONE 分区，系统就有多少个
独立的 "Buddy 内存分配器"。

![](/assets/PDB/RPI/RPI000835.png)
![](/assets/PDB/RPI/RPI000836.png)
![](/assets/PDB/RPI/RPI000837.png)
![](/assets/PDB/RPI/RPI000838.png)

那么多个独立 Buddy 分配器之间存在怎样的联系呢? 当从指定 ZONE 上分配物理内存
的时候，如果当前 ZONE 有可用的物理内存，那么 Buddy 就会从指定 ZONE 对应的
"Buddy 分配器" 上分配可用物理内存; 如果当前 ZONE 上没有可用物理内存，那么
Buddy 就按一定的顺序从其他 Zone 上获得可用物理内存，关系如下:

![](/assets/PDB/RPI/RPI000839.png)

从上图不免可以得到 Buddy 分配的关系:

{% highlight bash %}
1. 当从 ZONE_HIGHMEM 区间上分配可用物理内存，如果 ZONE_HIGHMEM 没有可用物理
   物理内存，那么 Buddy 继续从 ZONE_NORAML 中分配物理内存; 如果 ZONE_NORMAL
   分区中没有可用物理内存，那么从 ZONE_DMA32 中分配物理内存; 如果 ZONE_DMA32
   分区中没有可用物理内存，那么从 ZONE_DMA 中分配物理内存; 如果 ZONE_DMA 分
   区中没有可用物理内存，那么 OOM.
2. 当从 ZONE_NORMAL 区间上分配可用物理内存，如果 ZONE_NORMAL 分区中没有可用
   物理内存，那么从 ZONE_DMA32 中分配物理内存; 如果 ZONE_DMA32 分区中没有可
   用物理内存，那么从 ZONE_DMA 中分配物理内存; 如果 ZONE_DMA 分区中没有可用
   物理内存，那么 OOM.
3. 当从 ZONE_DMA32 区间上分配可用物理内存, 如果 ZONE_DMA32 分区中没有可用物
   理内存，那么从 ZONE_DMA 中分配物理内存; 如果 ZONE_DMA 分区中没有可用物理
   内存，那么 OOM.
4. 当从 ZONE_DMA 区间上分配可用物理内存, 如果 ZONE_DMA 分区中没有可用物理内
   存，那么 OOM.
{% endhighlight %}

从上面分配关系就可以知道为什么有的区间物理内存很珍贵，而有的物理内存很廉价.

###### Buddy 分配器分配

Buddy 分配器以物理页为单位为内核其他子系统提供内存的分配申请。Buddy 分配器
提供了多个便捷的函数 API 以便其他子系统进行物理内存分配。

{% highlight bash %}
static inline struct page * alloc_pages(gfp_t gfp_mask, unsigned int order)
{% endhighlight %}

例如使用上面的函数进行物理内存分配的时候，需要提供分配标志以及分配大小。
分配标志是用于表明分配的目的和分配的紧急缓慢，标志定义在 "linux/gfp.h":

{% highlight bash %}
GFP_DMA
  作用: 从 ZONE_DMA 内分配内存
GFP_DMA32
  作用: 优先从 ZONE_DMA32 中分配内存，未找到可以到 ZONE_DMA 中分配物理内存
GFP_KERNEL
  作用: 优先从 ZONE_KERNEL 中分配物理内存，未找到可以依次从 ZONE_DMA32 到
        ZONE_DMA 分区中进行查找.
__GFP_HIGHMEM
  作用: 优先从 ZONE_HIGHMEM 中分配物理内存，未找到可以依次从 ZONE_NORMAL
        到 ZONE_DMA32, 最后到 ZONE_DMA 分区中查找可用物理内存.
GFP_ATOMIC
  作用: 分配物理内存不能等待或休眠，如果没有可用物理内存，直接返回。
__GFP_WAIT
  作用: 如果未找到可用物理内存，可以进入休眠等待 Kswap 查找可用物理内存.
__GFP_ZERO
  作用: 分配的物理内存必须被清零.
__GFP_HIGH
  作用: 当系统可用物理很紧缺的情况下，可以从紧急内存池中分配物理内存.
__GFP_NOFAIL:
  作用: 当系统中没有可用物理内存的情况下，可以进入睡眠等待 kswap 交换出更
        多的可用物理内存, 或者等待其他进程释放物理内存，直到分配到可用物理
        内存.
__GFP_COLD:
  作用: 从 ZONE 分区的 PCP 冷页表中获得可用物理页.
__GFP_REPEAT:
  作用: 当 Buddy 分配器没有可用物理内存时，进行 retry 操作以此分配到可用物
        理内存，但由于 retry 的次数有限，可能会失败.
__GFP_NORETRY:
  作用: 当 Buddy 分配器没有可用物理内存时，不进行 retry 操作，直接反馈结果。 
{% endhighlight %}

在上面的例子中，使用 alloc_pages() 函数从 Buddy 分配器中分配物理内存，Buddy
分配器首先通过 GFP 标志判断从哪个 ZONE 中分配物理内存，确认完毕之后再通过
参数确定需要分配页的大小。综合上面的信息之后，Buddy 内存分配器就从指定的
ZONE 的 free_area[] 链表中查找可用的物理页块。这时会涉及两种基本情况:

![](/assets/PDB/RPI/RPI000840.png)

例如上面的情况，需要从指定的 ZONE 中分配 1 个物理页，但由于此时该 ZONE
的 free_area[0] 里面中没有可用的物理页块，那么 Buddy 分配器就会从下一级
free_area[1] 链表中查找一个可用的物理页，如果找到将其从 free_area[2] 链
表中移除，然后对半拆分成两个物理页块，一个给分配物理内存的请求者，另外
一个物理页块插入到低一级的 free_area[0] 里面, 如下图是分配之后的情况:

![](/assets/PDB/RPI/RPI000841.png)

在第一种情况下，如果向高一级的 free_area[] 查找可用物理页没有找到，那么
再向更高一级 free_area[] 查找，直到查找到 free_area[MAX_ORDER-1] 链表为止。
如果在更高的 free_area[] 链表中查找到可用物理页块，那么 Buddy 分配器就会
将可用物理页块从当前的 free_area[] 中移除，然后从中间分成两块相等的物理
页块，一块插入低一级的 free_area[] 链表中，另外一块如果比需求的物理页块
还大，那么继续从中间均分成两块，一块插入到更低一级 free_area[] 链表中。
重复上面的动作知道分到合适的物理页块.

![](/assets/PDB/RPI/RPI000842.png)

对于第二种情况，Buddy 分配器从指定的 ZONE 上分配指定大小的物理内存，正好
对于的 free_area[] 链表上正好存在可以物理页块，那么 Buddy 就直接从指定的
free_area[] 上移除可用物理页块给申请者即可。

以上两种情况均是 Buddy 分配器的通常遇到的情况，但由于运行时间很长，系统中
存在某些长度的物理页块已经很紧缺，甚至已经没有，这个 Buddy 内存分配器往往
采取一些测量来应对这些问题。

![](/assets/PDB/RPI/RPI000839.png)

第一种情况就是当前 ZONE 已经没有可用的物理页块了，那么就按上面的关系从其他
ZONE 中进行查找. 如果找到就反馈给调用者，如果没有找到，那么就进行第二种情况。

第二种情况当前可用物理页数量已经低于 watermark, 那么 Buddy 内存分配器会唤
醒 Kswap 内核线程，Kswap 线程会查找各个文件系统中 LRU Page 链表情况，通过
一定的算法确认哪些 Page 可以换页到磁盘上。换页完成之后会释放出更多的可用物
理页，那么 Buddy 确认是否满足申请，如果满足，那么将可用的物理页传递给申请
者。如果不满足那么 Buddy 进入第三种情况。

第三种情况是 Buddy 内存分配器就会执行 try_to_free_pages() 操作，该操作会
在内核中尽可能查找可用的物理内存，尽量释放可以释放的物理内存，以便满足
申请需求，这样的操作如果不能满足申请需求，那么 Buddy 内存分配器将会触发
OOM。

###### Buddy 分配器释放

当使用完物理内存之后，可使用 Buddy 内存分配器提供的函数将物理内存返回
给 Buddy 内存分配器，如下:

{% highlight bash %}
fastcall void __free_pages(struct page *page, unsigned int order)
{% endhighlight %}

调用者只需将指向 page 的指针和物理页大小的信息传递给 Buddy 分配器，
Buddy 分配器获得这些信息之后首先确认 page 属于哪个 ZONE 分区，然后根据
order 确认插入到哪个 free_area[] 链表里。当插入的过程中，Buddy 分配器
会查找 page 的 "Buddy" 是否正在指定的 free_area[] 链表中，如果 "Buddy"
物理页块正好存在，而且可用，那么 Buddy 分配器将 page 与 "Buddy" page
进行合并成更大的物理页块，然后 Buddy 分配器再向高一级的 free_area[]
链表中查找新物理页块的 "Buddy" 物理页块，同理如果 "Buddy" 可用，那么继续
合并更大的物理页块，再循环向更高一级的 free_area[] 中合并，直到合并到
free_area[MAX_ORDER-1] 为止。如果 page 找不到 "Buddy" page, 那么 Buddy
分配器就将 page 插入到指定的 free_area[] 链表即可.

---------------------------------

###### buddy 的优点

Buddy 内存分配器用于分配物理内存，也只管理物理内存，并不涉及虚拟内存
的管理。系统可以从 Buddy 内存分配器中分配连续的物理页块。

###### buddy 的缺点

由于系统长时间的运行，Buddy 内存分配器中大块的物理页块越来越少，对
大块连续物理内存的申请带来不便.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### buddy 分配器使用

> - [基础用法介绍](#B0000)
>
> - [ZONE_DMA 中分配物理页](#B0001)
>
> - [ZONE_DMA32 中分配物理页](#B0002)
>
> - [ZONE_NORMAL 中分配物理页](#B0003)
>
> - [ZONE_HIGHMEM 中分配物理页](#B0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0000">基础用法介绍</span>

Buddy 内存分配器相关的源文件位于:

{% highlight bash %}
mm/page_alloc.c include/linux/gfp.h
{% endhighlight %}

Buddy 内存分配器提供了多个 API 以便物理内存的分配是释放:

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0001">ZONE_DMA 中分配物理页</span>

![](/assets/PDB/RPI/RPI000835.png)

从 ZONE_DMA 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/gfp.h>
#include <linux/mm.h>

static int TestCase_alloc_page_from_DMA(void)
{
        struct page_bs *pages;
        int order = 2;
        void *addr;

        /* allocate page from Buddy (DMA) */
        pages = alloc_pages(GFP_DMA, order);
        if (!pages) {
                printk("%s error\n", __func__);
                return -ENOMEM;
        }

        /* Obtain page virtual address */
        addr = page_address(pages);
        if (!addr) {
                printk("%s bad page address!\n", __func__);
                return -EINVAL;
        }
        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

        /* free all pages to buddy */
        free_pages((unsigned long)addr, order);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0002">ZONE_DMA32 中分配物理页</span>

![](/assets/PDB/RPI/RPI000836.png)

从 ZONE_DMA32 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/gfp.h>
#include <linux/mm.h>

static int TestCase_alloc_page_from_DMA32(void)
{
        struct page_bs *pages;
        int order = 2;
        void *addr;

        /* allocate page from Buddy (DMA32) */
        pages = alloc_pages(GFP_DMA32, order);
        if (!pages) {
                printk("%s error\n", __func__);
                return -ENOMEM;
        }

        /* Obtain page virtual address */
        addr = page_address(pages);
        if (!addr) {
                printk("%s bad page address!\n", __func__);
                return -EINVAL;
        }
        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

        /* free all pages to buddy */
        free_pages((unsigned long)addr, order);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0003">ZONE_NORMAL 中分配物理页</span>

![](/assets/PDB/RPI/RPI000837.png)

从 ZONE_NORMAL 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/gfp.h>
#include <linux/mm.h>

static int TestCase_alloc_page_from_NORMAL(void)
{
        struct page_bs *pages;
        int order = 2;
        void *addr;

        /* allocate page from Buddy (NORMAL) */
        pages = alloc_pages(GFP_NORMAL, order);
        if (!pages) {
                printk("%s error\n", __func__);
                return -ENOMEM;
        }

        /* Obtain page virtual address */
        addr = page_address(pages);
        if (!addr) {
                printk("%s bad page address!\n", __func__);
                return -EINVAL;
        }
        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

        /* free all pages to buddy */
        free_pages((unsigned long)addr, order);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0004">ZONE_HIGHMEM 中分配物理页</span>

![](/assets/PDB/RPI/RPI000838.png)

从 ZONE_HIGHMEM 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/highmem.h>

static int TestCase_alloc_page_from_HIGHMEM(void)
{
        struct page_bs *pages;
        int order = 2;

        /* allocate page from Buddy (NORMAL) */
        pages = alloc_pages(__GFP_HIGHMEM, order);
        if (!pages) {
                printk("%s error\n", __func__);
                return -ENOMEM;
        }

	/* Do other mapping! */

        /* free all pages to buddy */
        __free_pages(pages, order);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### buddy 分配器实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)
>
> - [实践建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C0003)
>
> - [测试建议](#C0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

本实践是基于 BiscuitOS Linux 5.0 ARM32 环境进行搭建，因此开发者首先
准备实践环境，请查看如下文档进行搭建:

> - [BiscuitOS Linux 5.0 ARM32 环境部署](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

--------------------------------------------

#### <span id="C0001">实践部署</span>

准备好基础开发环境之后，开发者接下来部署项目所需的开发环境。由于项目
支持多个版本的 buddy，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 buddy 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](/assets/PDB/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](/assets/PDB/RPI/RPI000843.png)

选择并进入 "[\*]   Buddy Allocator  --->" 目录。

![](/assets/PDB/RPI/RPI000844.png)

选择 "[\*]   buddy on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_buddy-2.6.12
make download
{% endhighlight %}

![](/assets/PDB/RPI/RPI000845.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](/assets/PDB/RPI/RPI000846.png)

arch 目录下包含内存初始化早期，与体系结构相关的处理部分。mm 目录下面包含
了与各个内存分配器和内存管理行为相关的代码。init 目录下是整个模块的初始化
入口流程。modules 目录下包含了内存分配器的使用例程和测试代码. fs 目录下
包含了内存管理信息输出到文件系统的相关实现。入口函数是 init/main.c 的
start_kernel()。

如果你是第一次使用这个项目，需要修改 DTS 的内容。如果不是可以跳到下一节。
开发者参考源码目录里面的 "BiscuitOS.dts" 文件，将文件中描述的内容添加
到系统的 DTS 里面，"BiscuitOS.dts" 里的内容用来从系统中预留 100MB 的物理
内存供项目使用，具体如下:

![](/assets/PDB/RPI/RPI000738.png)

开发者将 "BiscuitOS.dts" 的内容添加到:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

添加完毕之后，使用如下命令更新 DTS:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_buddy-2.6.12
make kernel
{% endhighlight %}

![](/assets/PDB/RPI/RPI000847.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_buddy-2.6.12
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000848].png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_buddy-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_buddy-2.6.12
make run
{% endhighlight %}

![](/assets/PDB/RPI/RPI000849.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_buddy-2.6.12.ko"，打印如上信息，那么
BiscuitOS Memory Manager Unit History 项目的内存管理子系统已经可以使用，
接下来开发者可以在 BiscuitOS 中使用如下命令查看内存管理子系统的使用情况:

{% highlight bash %}
cat /proc/buddyinfo_bs
cat /proc/vmstat_bs
{% endhighlight %}

![](/assets/PDB/RPI/RPI000756.png)

--------------------------------------

###### <span id="C0004">测试建议</span>

BiscuitOS Memory Manager Unit History 项目提供了大量的测试用例用于测试
不同内存分配器的功能。结合项目提供的 initcall 机制，项目将测试用例分作
两类，第一类类似于内核源码树内编译，也就是同 MMU 子系统一同编译的源码。
第二类类似于模块编译，是在 MMU 模块加载之后独立加载的模块。以上两种方案
皆在最大程度的测试内存管理器的功能。

要在项目中使用以上两种测试代码，开发者可以通过项目提供的 Makefile 进行
配置。以 linux 2.6.12 为例， Makefile 的位置如下:

{% highlight bash %}
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_buddy-2.6.12/BiscuitOS_buddy-2.6.12/Makefile
{% endhighlight %}

![](/assets/PDB/RPI/RPI000771.png)

Makefile 内提供了两种方案的编译开关，例如需要使用打开 buddy 内存管理器的
源码树内部调试功能，需要保证 Makefile 内下面语句不被注释:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/buddy/main.o
{% endhighlight %}

如果要关闭 buddy 内存管理器的源码树内部调试功能，可以将其注释:

{% highlight bash %}
# $(MODULE_NAME)-m                += modules/buddy/main.o
{% endhighlight %}

同理，需要打开 buddy 模块测试功能，可以参照下面的代码:

{% highlight bash %}
obj-m                             += $(MODULE_NAME)-buddy.o
$(MODULE_NAME)-buddy-m            := modules/buddy/module.o
{% endhighlight %}

如果不需要 buddy 模块测试功能，可以参考下面代码, 将其注释:

{% highlight bash %}
# obj-m                             += $(MODULE_NAME)-buddy.o
# $(MODULE_NAME)-buddy-m            := modules/buddy/module.o
{% endhighlight %}

在上面的例子中，例如打开了 buddy 的模块调试功能，重新编译模块并在 BiscuitOS
上运行，如下图，可以在 "lib/module/5.0.0/extra/" 目录下看到两个模块:

![](/assets/PDB/RPI/RPI000772.png)

然后先向 BiscuitOS 中插入 "BiscuitOS_buddy-2.6.12.ko" 模块，然后再插入
"BiscuitOS_buddy-2.6.12-buddy.ko" 模块。如下:

![](/assets/PDB/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](/assets/PDB/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

Initcall 机制提供了以下函数用于 buddy 调试:

{% highlight bash %}
buddy_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，buddy_initcall_bs() 调用的函数将
在 buddy 分配器初始化完毕之后自动调用，因此可用此法调试 Buddy 内存分配器。
buddy 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_buddy-2.6.12/BiscuitOS_buddy-2.6.12/module/buddy/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                  += modules/buddy/main.o
{% endhighlight %}

模块方式调试 Buddy 分配器，在 Makefile 中打开调试开关:

{% highlight bash %}
obj-m                             += $(MODULE_NAME)-buddy.o
$(MODULE_NAME)-buddy-m            := modules/buddy/module.o
{% endhighlight %}


![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### buddy 历史补丁

> - [buddy Linux 2.6.12](#H-linux-2.6.12)
>
> - [buddy Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [buddy Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [buddy Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [buddy Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [buddy Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [buddy Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [buddy Linux 2.6.13](#H-linux-2.6.13)
>
> - [buddy Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [buddy Linux 2.6.14](#H-linux-2.6.14)
>
> - [buddy Linux 2.6.15](#H-linux-2.6.15)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](/assets/PDB/RPI/RPI000785.JPG)

#### buddy Linux 2.6.12

Linux 2.6.12 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](/assets/PDB/RPI/RPI000786.JPG)

#### buddy Linux 2.6.12.1

Linux 2.6.12.1 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相比 Linux 2.6.12 版本，该版本并未产生 Buddy 分配器相关的补丁。更多补丁
的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](/assets/PDB/RPI/RPI000787.JPG)

#### buddy Linux 2.6.12.2

Linux 2.6.12.2 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相比 Linux 2.6.12.1 版本，该版本并未产生 Buddy 分配器相关的补丁。更多补丁
的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](/assets/PDB/RPI/RPI000788.JPG)

#### buddy Linux 2.6.12.3

Linux 2.6.12.3 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相比 Linux 2.6.12.2 版本，该版本并未产生 Buddy 分配器相关的补丁。更多补丁
的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](/assets/PDB/RPI/RPI000789.JPG)

#### buddy Linux 2.6.12.4

Linux 2.6.12.4 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相比 Linux 2.6.12.3 版本，该版本并未产生 Buddy 分配器相关的补丁。更多补丁
的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](/assets/PDB/RPI/RPI000790.JPG)

#### buddy Linux 2.6.12.5

Linux 2.6.12.5 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相比 Linux 2.6.12.4 版本，该版本并未产生 Buddy 分配器相关的补丁。更多补丁
的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](/assets/PDB/RPI/RPI000791.JPG)

#### buddy Linux 2.6.12.6

Linux 2.6.12.6 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相比 Linux 2.6.12.5 版本，该版本并未产生 Buddy 分配器相关的补丁。更多补丁
的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](/assets/PDB/RPI/RPI000792.JPG)

#### buddy Linux 2.6.13

Linux 2.6.13 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相对上一个版本 linux 2.6.12.6，Buddy 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/page_alloc.c include/linux/gfp.h include/linux/mmzone.h include/linux/mm.h

2005-05-01 08:58 Nick Piggin       o [PATCH] mempool: NOMEMALLOC and NORETRY
                                     [main] b84a35be0285229b0a8a5e2e04d79360c5b75562
2005-06-21 17:14 Nikita Danilov    o [PATCH] mm: add /proc/zoneinfo
                                     [main] 295ab93497ec703f7d6eaf0787dd9768b83035fe
2005-06-21 17:14 Martin Hicks      o [PATCH] VM: early zone reclaim
                                     [main] 753ee728964e5afb80c17659cc6c3a6fd0a42fe0 
2005-06-21 17:14 Martin Hicks      o [PATCH] VM: add __GFP_NORECLAIM
                                     [main] 0c35bbadc59f5ed105c34471143eceb4c0dd9c95 
2005-06-21 17:14 Martin Hicks      o [PATCH] VM: rate limit early reclaim
                                     [main] 1e7e5a9048b30c57ba1ddaa6cdf59b21b65cde99  
2005-06-21 17:14 Darren Hart       o [PATCH] vm: try_to_free_pages unused argument
                                     [main] 1ad539b2bd89bf2e129123eb24d5bcc4484a35de 
2005-06-21 17:14 Benjamin LaHaise  o [PATCH] __mod_page_state(): pass unsigned long instead of unsigned
                                     [main] 83e5d8f7253cb7b14472385a6d57df1e9f848e8e
2005-06-21 17:14 Benjamin LaHaise  o [PATCH] __read_page_state(): pass unsigned long instead of unsigned
                                     [main] c2f29ea111e3344ed48257c2a142c3db514e1529 
2005-06-21 17:14 Janet Morgan      o [PATCH] add OOM debug
                                     [main] 578c2fd6a7f378434655e5c480e23152a3994404 
2005-06-21 17:15 Hugh Dickins      o [PATCH] bad_page: clear reclaim and slab
                                     [main] 334795eca421287c41c257992027d29659dc0f97 
2005-06-21 17:15 Denis Vlasenko    o [PATCH] Kill stray newline
                                     [main] c0d62219a48bd91ec40fb254c930914dccc77ff1 
2005-06-23 00:07 Dave Hansen       o [PATCH] sparsemem base: reorganize page->flags bit operations
                                     [main] 348f8b6c4837a07304d2f72b11ce8d96588065e0 
2005-06-23 00:07 Dave Hansen       o [PATCH] Introduce new Kconfig option for NUMA or DISCONTIG
                                     [main] 93b7504e3e6c1d98586854806e51bea329ea3aa9 
2005-06-27 14:36 Bob Picco         o [PATCH] fix WANT_PAGE_VIRTUAL in memmap_init
                                     [main] 3212c6be251219c0f4c2df0c93e122ff5be0d9dc 
2005-07-07 17:56 Marcelo Tosatti   o [PATCH] print order information when OOM killing
                                     [main] 79b9ce311e192e9a31fd9f3cf1ee4a4edf9e2650 
2005-07-07 17:56 Marcelo Tosatti   o [PATCH] remove completly bogus comment inside __alloc_pages()
                                     [main] 37b173a4d03d1681e6c9529bc43d7a3308132db6 
2005-07-07 17:56 Alexey Dobriyan   o [PATCH] propagate __nocast annotations
                                     [main] 0db925af1db5f3dfe1691c35b39496e2baaff9c9 
2005-07-27 11:44 Andy Whitcroft    o [PATCH] Remove bogus warning in page_alloc.c
                                     [main] 12b1c5f382194d3f656e78fb5c9c8f2bfbe8ed8a 
2005-07-29 22:59 Martin J. Bligh   o [PATCH] Fix NUMA node sizing in nr_free_zone_pages
                                     [main] e310fd43256b3cf4d37f6447b8f7413ca744657a 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000851.png)

{% highlight bash %}
git format-patch -1 b84a35be0285229b0a8a5e2e04d79360c5b75562
vi 0001-PATCH-mempool-NOMEMALLOC-and-NORETRY.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000852.png)

该补丁向 Buddy 管理器中添加了 "\_\_GFP_NOMEMPOOL" 标志，使用该标志之后
当从 MEMPOOL 中分配物理内存的时候，如果分配表值中包含了 "\_\_GFP_NOMEMPOOL"
标志，那么将从 Buddy 内存分配器中获得物理页而不是从 MEMPOOL 中获得物理内存.

{% highlight bash %}
git format-patch -1 295ab93497ec703f7d6eaf0787dd9768b83035fe 
vi 0001-PATCH-mm-add-proc-zoneinfo.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000853.png)

该补丁用于向系统添加 "/proc/zoneinfo"，以便从用户空间获得 ZONE 的信息，
在 BiscuitOS 上使用情况如下:

![](/assets/PDB/RPI/RPI000854.png)

{% highlight bash %}
git format-patch -1 753ee728964e5afb80c17659cc6c3a6fd0a42fe0 
vi 0001-PATCH-VM-early-zone-reclaim.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000855.png)

该补丁是向 Buddy 内存分配器添加了页回收机制，并在 IA64 和 i386 架构上添加
了新的系统调用 sys_set_zone_reclaim. 首先在 struct zone 结构中添加了 
"reclaim_pages" 成员，并在 "\_\_alloc_pages()" 中添加了 zone_reclaim_retry
处理，当当前 ZONE 可用物理内存小于 watermark 之后，如果 should_reclaim_zone()
确认可以回收物理页，那么 Buddy 内存分配器就执行 zone_reclaim() 进行物理页
回收，回收完成之后，Buddy 内存分配器再次分配物理内存。sys_set_zone_reclaim
系统调用主要是用于设置 Buddy 内存分配器是否支持页回收机制。

{% highlight bash %}
git format-patch -1 0c35bbadc59f5ed105c34471143eceb4c0dd9c95 
vi 0001-PATCH-VM-add-__GFP_NORECLAIM.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000856.png)

该补丁向 Buddy 内存管理器中添加了 \_\_GFP_NORECLAIM 标志，以便在 Buddy 分配
器在没有可用物理内存的时候，不执行回收策略。

{% highlight bash %}
git format-patch -1 1e7e5a9048b30c57ba1ddaa6cdf59b21b65cde99
vi 0001-PATCH-VM-rate-limit-early-reclaim.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000857.png)

该补丁用于在 struct zone 中添加 "reclaim_in_progress" 成员，该成员用于
统计标记该 ZONE 是否正在进行页回收操作. 每当 ZONE 进行页回收的时候就会
增加 reclaim_in_progress 的值，以告诉其他对 ZONE 进行页回收操作当前 ZONE
正在进行页回收操作。页回收结束之后再减少 reclaim_in_progress 的引用计数。

{% highlight bash %}
git format-patch -1 1ad539b2bd89bf2e129123eb24d5bcc4484a35de 
vi 0001-PATCH-vm-try_to_free_pages-unused-argument.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000858.png)

该补丁用于修改的 try_to_free_pages() 函数的参数，移除了 order 参数. 移除
的原因是从 2.6.0 开始，order 参数就没有使用过.

{% highlight bash %}
git format-patch -1 83e5d8f7253cb7b14472385a6d57df1e9f848e8e
vi 0001-PATCH-__mod_page_state-pass-unsigned-long-instead-of.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000859.png)

该补丁用于将 \_\_mod_page_state() 函数的 offset 参数类型由 unsigned 转变为
unsigned long。

{% highlight bash %}
git format-patch -1 c2f29ea111e3344ed48257c2a142c3db514e1529 
vi 0001-PATCH-__read_page_state-pass-unsigned-long-instead-o.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000860.png)

该补丁用于修改 \_\_read_page_state() 函数 offset 参数的类型，由 unsigned
转变为 unsigned long. 以便解决一些编译遇到的问题.

{% highlight bash %}
git format-patch -1 578c2fd6a7f378434655e5c480e23152a3994404 
vi 0001-PATCH-add-OOM-debug.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000861.png)

该补丁用于在出现 OOM 时候打印所有 mem 信息。并且在 Buddy 分配器无法分配
可用物理内存的时候，打印当前内存信息.

{% highlight bash %}
git format-patch -1 334795eca421287c41c257992027d29659dc0f97 
vi 0001-PATCH-bad_page-clear-reclaim-and-slab.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000862.png)

该补丁在 bad_page() 函数判断 page 标志是增加了 PG_slab、PG_private 和 
PG_reclaim 标志. 在 prep_new_page() 函数中添加了对 page 映射数量、是否
映射和引用计数进行检测，并对 page 的 flags 检测时增加了 PG_slab 检测。
以上的修改在 Buddy 分配器添加了 PG_slab 和 PG_reclaim 标志的支持，以便
free_pages_check() 的正确检测.

{% highlight bash %}
git format-patch -1 c0d62219a48bd91ec40fb254c930914dccc77ff1
vi 0001-PATCH-Kill-stray-newline.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000863.png)

该补丁修改了打印信息的格式.

{% highlight bash %}
git format-patch -1 348f8b6c4837a07304d2f72b11ce8d96588065e0 
vi 0001-PATCH-sparsemem-base-reorganize-page-flags-bit-opera.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000864.png)

该补丁基于 NODE 相关信息重新规划了 ZONES_MASK, ZONETABLE_MASK 和 NODES_MASK
等宏，以此重新规划了 zone_table[] 的布局，这样的修改影响到了 page_zonenum()，
page_to_nid(), page_zone() 等函数的实现。并在初始化每个 page 的时候，使用
set_page_links() 将 page 关联到相应的 NODE 和 ZONE 上。

{% highlight bash %}
git format-patch -1 93b7504e3e6c1d98586854806e51bea329ea3aa9 
vi 0001-PATCH-Introduce-new-Kconfig-option-for-NUMA-or-DISCO.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000865.png)

该补丁新增了一个宏 CONFIG_NEED_MULTIPLE_NODES, 并在 Buddy 分配器相关的代码里，
将 CONFIG_DISCONTIGMEM 替换成 CONFIG_NEED_MULTIPLE_NODES，在函数
"free_area_init()" 中将原先的 contig_page_data 替换成了 NODE_DATA(0)。

{% highlight bash %}
git format-patch -1 3212c6be251219c0f4c2df0c93e122ff5be0d9dc 
vi 0001-PATCH-fix-WANT_PAGE_VIRTUAL-in-memmap_init.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000866.png)

该补丁用于在 Buddy 分配器初始化每个 page 的时候，如果系统支持 WANT_PAGE_VIRTUAL,
那么如果这个 page 不是 ZONE_HIGHMEM，那么 memap_init_zone() 函数就会给
这个 page 设置一个线性地址，因为非 ZONE_HIGHMEM 的 page 对应的物理地址和
虚拟地址都是线性映射的.

{% highlight bash %}
git format-patch -1 79b9ce311e192e9a31fd9f3cf1ee4a4edf9e2650 
vi 0001-PATCH-print-order-information-when-OOM-killing.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000867.png)

该补丁修改了 out_of_memory() 的参数，添加了 order 参数用于指定 Buddy
分配器中是哪条 free_area[] 上没有可用物理内存了。

{% highlight bash %}
git format-patch -1 37b173a4d03d1681e6c9529bc43d7a3308132db6 
vi 0001-PATCH-remove-completly-bogus-comment-inside-__alloc_.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000868.png)

该补丁从 Buddy 内存分配器的 \_\_alloc_oages() 函数中移除了一些注释.

{% highlight bash %}
git format-patch -1 0db925af1db5f3dfe1691c35b39496e2baaff9c9 
vi 0001-PATCH-propagate-__nocast-annotations.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000869.png)

该补丁用于将与 Buddy 内存分配器相关的函数中，gfp 参数的类型修改为
"unsinged int \_\_nocast" 类型，以便内核做静态检测. 并对 \_\_GFP_DMA 和
\_\_GFP_HIGHMEM 宏的值增加了 "u" 限制.

{% highlight bash %}
git format-patch -1 12b1c5f382194d3f656e78fb5c9c8f2bfbe8ed8a 
vi 0001-PATCH-Remove-bogus-warning-in-page_alloc.c.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000870.png)

该补丁在 Buddy 内存分配器创建过程中，移除了 ZONE 对齐的一些操作.

{% highlight bash %}
git format-patch -1 e310fd43256b3cf4d37f6447b8f7413ca744657a
vi 0001-PATCH-Fix-NUMA-node-sizing-in-nr_free_zone_pages.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000871.png)

该补丁用于在计算一个 ZONE 上物理的数量支持了多 NODE. 更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](/assets/PDB/RPI/RPI000793.JPG)

#### buddy Linux 2.6.13.1

Linux 2.6.13.1 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相比 Linux 2.6.13 版本，该版本并未产生 Buddy 分配器相关的补丁。更多补丁
的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](/assets/PDB/RPI/RPI000794.JPG)

#### buddy Linux 2.6.14

Linux 2.6.14 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相对上一个版本 linux 2.6.13.1，Buddy 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/page_alloc.c include/linux/gfp.h include/linux/mmzone.h include/linux/mm.h

2005-09-03 15:54 Nick Piggin              o [PATCH] mm: remove atomic
                                            [main] 242e54686257493f0b10ac557e730419d9af7d24
2005-09-03 15:54 Martin Hicks             o [PATCH] VM: zone reclaim atomic ops cleanup
                                            [main] 53e9a6159fdc6419874ce4d86d3577dbedc77b62
2005-09-03 15:55 Martin Hicks             o [PATCH] VM: add page_state info to per-node meminfo
                                            [main] c07e02db76940c75fc92f2f2c9adcdbb09ed70d0
2005-09-06 15:16 Christoph Lameter        o [PATCH] More __read_mostly variables
                                            [main] c3d8c1414573be8cf7c8fdc1e076935697c7f6af
2005-09-06 15:17 Ravikiran G Thirumalai   o [PATCH] Additions to .data.read_mostly section
                                            [main] 6c231b7bab0aa6860cd9da2de8a064eddc34c146
2005-09-06 15:18 Paul Jackson             o [PATCH] cpusets: new __GFP_HARDWALL flag
                                            [main] f90b1d2f1aaaa40c6519a32e69615edc25bb97d5
2005-09-06 15:18 Paul Jackson             o [PATCH] cpusets: formalize intermediate GFP_KERNEL containment
                                            [main] 9bf2229f8817677127a60c177aefce1badd22d7b
2005-09-10 00:26 Renaud Lienhart          o [PATCH] remove invalid comment in mm/page_alloc.c
                                            [main] 207f36eec9e7b1077d7a0aaadb4800e2c9b4cfa4
2005-09-13 01:25 Randy Dunlap             o [PATCH] use add_taint() for setting tainted bit flags
                                            [main] 9f1583339a6f52c0c26441d39a0deff8246800f7
2005-10-07 07:46 Al Viro                  o [PATCH] gfp flags annotations - part 1
                                            [main] dd0fc66fb33cd610bc1a5db8a5e232d34879b4d7
{% endhighlight %}

![](/assets/PDB/RPI/RPI000872.png)

{% highlight bash %}
git format-patch -1 242e54686257493f0b10ac557e730419d9af7d24 
vi 0001-PATCH-mm-remove-atomic.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000873.png)

该补丁添加了 "\_\_ClearPageDirty(page)", 用于标记某个页变沾了，并在 Buddy
内存分配器分配过程中，对可用的物理进行检测的时候，如果检测到页是沾的，那么
调用该函数.

{% highlight bash %}
git format-patch -1 53e9a6159fdc6419874ce4d86d3577dbedc77b62 
vi 0001-PATCH-VM-zone-reclaim-atomic-ops-cleanup.patch
{% endhighlight %}

该补丁由于修改 struct zone 的 reclaim_in_progress 成员，该成员用于标记
当前 ZONE 是否在进行页回收。在 Buddy 内存管理器初始化的时候，将各个 ZONE
的 reclaim_in_progress 初始化为 0，并在 shrik_zone() 外面的 reclaim_in_progress
操作全部移到了 shrink_zone() 函数内部.

![](/assets/PDB/RPI/RPI000874.png)

{% highlight bash %}
git format-patch -1 c07e02db76940c75fc92f2f2c9adcdbb09ed70d0
vi 0001-PATCH-VM-add-page_state-info-to-per-node-meminfo.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000875.png)

该补丁修改 Buddy 内存管理器中 page 使用状况相关的函数. 添加了 cpu_mask 的
支持. 补丁还添加了 NODE 节点内存信息的打印.

{% highlight bash %}
git format-patch -1 c3d8c1414573be8cf7c8fdc1e076935697c7f6af
vi 0001-PATCH-More-__read_mostly-variables.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000876.png)

该补丁用于对 Buddy 内存管理器使用的一些数据定义为 "\_\_read_mostly".

{% highlight bash %}
git format-patch -1 6c231b7bab0aa6860cd9da2de8a064eddc34c146
vi 0001-PATCH-Additions-to-.data.read_mostly-section.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000877.png)

该补丁用于对 Buddy 内存管理器使用的一些数据定义为 "\_\_read_mostly".

{% highlight bash %}
git format-patch -1 f90b1d2f1aaaa40c6519a32e69615edc25bb97d5
vi 0001-PATCH-cpusets-new-__GFP_HARDWALL-flag.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000878.png)

向 Buddy 内存分配器添加了 \_\_GFP_HARDWALL 标志.

{% highlight bash %}
git format-patch -1 9bf2229f8817677127a60c177aefce1badd22d7b 
vi 0001-PATCH-cpusets-formalize-intermediate-GFP_KERNEL-cont.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000879.png)

该补丁修改了 cpuset_zone_allowed() 函数，添加了 gfp_mask 参数，补丁将 Buddy
内存分配器调用的 cpuset_zone_allowed() 函数的 gfp_mask 参数全部设置为
\_\_GFP_HARDWALL。

{% highlight bash %}
git format-patch -1 207f36eec9e7b1077d7a0aaadb4800e2c9b4cfa4 
vi 0001-PATCH-remove-invalid-comment-in-mm-page_alloc.c.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000880.png)

该补丁修改了注释，表明 free_pages_bulk() 不支持 count 为 0 的 list 释放.

{% highlight bash %}
git format-patch -1 9f1583339a6f52c0c26441d39a0deff8246800f7
vi 0001-PATCH-use-add_taint-for-setting-tainted-bit-flags.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000881.png)

该补丁向 Buddy 内存分配器的 bad_page() 函数添加了 add_taile() 的处理，移除
之前的同类代码.

{% highlight bash %}
git format-patch -1 dd0fc66fb33cd610bc1a5db8a5e232d34879b4d7 
vi 0001-PATCH-gfp-flags-annotations-part-1.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000882.png)

该补丁用于将 Buddy 内存分配器相关函数的 gfp 参数全部修改为 gfp_t 类型.
更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](/assets/PDB/RPI/RPI000795.JPG)

#### buddy Linux 2.6.15

Linux 2.6.15 依旧采用 buddy 作为其物理内存管理器。

![](/assets/PDB/RPI/RPI000827.png)

###### Buddy 内存分配

{% highlight bash %}
alloc_pages_node
alloc_pages_current
__get_free_pages
get_zeroed_page
__alloc_pages
__get_free_pages
{% endhighlight %}

###### Buddy 内存释放

{% highlight bash %}
__alloc_pages
__free_pages
free_pages
__free_page
free_page
{% endhighlight %}

###### Buddy 其他操作

{% highlight bash %}
gfp_zone
page_alloc_init
nr_free_pages
nr_free_highpages
{% endhighlight %}

具体函数解析说明，请查看:

> - [buddy API](#K)

###### 与项目相关

buddy 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相对上一个版本 linux 2.6.13.1，Buddy 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/page_alloc.c include/linux/gfp.h include/linux/mmzone.h include/linux/mm.h

2005-10-21 02:55 Al Viro          o [PATCH] gfp_t: infrastructure
                                    [main] af4ca457eaf2d6682059c18463eb106e2ce58198
2005-10-21 03:22 Al Viro          o [PATCH] gfp_t: the rest
                                    [main] 260b23674fdb570f3235ce55892246bef1c24c2a
2005-10-29 18:16 Nick Piggin      o [PATCH] core remove PageReserved
                                    [main] b5810039a54e5babf428e9a1e89fc1940fabff11
2005-10-29 18:16 Hugh Dickins     o [PATCH] mm: split page table lock
                                    [main] 4c21e2f2441dc5fbb957b030333f5a3f2d02dea7
2005-10-29 18:16 Dave Hansen      o [PATCH] memory hotplug prep: break out zone initialization
                                    [main] ed8ece2ec8d3c2031b1a1a0737568bb0d49454e0
2005-10-29 18:16 Dave Hansen      o [PATCH] memory hotplug locking: node_size_lock
                                    [main] 208d54e5513c0c02d85af0990901354c74364d5c
2005-10-29 18:16 Dave Hansen      o [PATCH] memory hotplug locking: zone span seqlock
                                    [main] bdc8cb984576ab5b550c8b24c6fa111a873503e3
2005-10-29 18:17 John Hawkes      o [PATCH] mm: wider use of for_each_*cpu()
                                    [main] 2f96996de0eda378df2a5f857ee1ef615ae10a4f
2005-11-07 01:01 Adrian Bunk      o [PATCH] unexport nr_swap_pages
                                    [main] f8b8db77b0cc36670ef4ed6bc31e64537ffa197e
2005-11-10 15:45 Dave Jones       o [PATCH] Don't print per-cpu vm stats for offline cpus.
                                    [main] 6b482c6779daaa893b277fc9b70767a7c2e7c5eb
2005-11-13 16:06 Kirill Korotaev  o [PATCH] mm: __GFP_NOFAIL fix
                                    [main] 885036d32f5d3c427c3e2b385b5a5503805e3e52
2005-11-13 16:06 Rohit Seth       o [PATCH] mm: __alloc_pages cleanup
                                    [main] 7fb1d9fca5c6e3b06773b69165a73f3fb786b8ee
2005-11-13 16:06 Paul Jackson     o [PATCH] mm: gfp_noreclaim cleanup
                                    [main] 2d6c666e8704cf06267f29a4fa3d2cf823469c38
2005-11-13 16:06 Nick Piggin      o [PATCH] mm: highmem watermarks
                                    [main] 669ed17521b9b78cdbeac8a53c30599aca9527ce
2005-11-05 17:25 Andi Kleen       o [PATCH] x86_64: Add 4GB DMA32 zone
                                    [main] a2f1b424900715ed9d1699c3bb88a434a2b42bc0
2005-11-05 17:25 Andi Kleen       o [PATCH] x86_64: Remove obsolete ARCH_HAS_ATOMIC_UNSIGNED and page_flags_t
                                    [main] 07808b74e7dab1aa385e698795875337d72daf7d
2005-11-17 21:35 Jens Axboe       o [PATCH] VM: fix zone list restart in page allocatate
                                    [main] 6b1de9161e973bac8c4675db608fe4f38d2689bd
2005-11-21 21:32 Hugh Dickins     o [PATCH] unpaged: unifdefed PageCompound
                                    [main] 664beed0190fae687ac51295694004902ddeb18e
2005-11-21 21:32 Hugh Dickins     o [PATCH] unpaged: PG_reserved bad_page
                                    [main] 689bcebfda16d7bace742740bfb3137fff30b529
2005-11-22 19:39 Linus Torvalds   o Fix up GFP_ZONEMASK for GFP_DMA32 usage
                                    [main] ac3461ad632e86e7debd871776683c05ef3ba4c6
2005-11-28 13:44 Nick Piggin      o [PATCH] mm: __alloc_pages cleanup fix
                                    [main] 3148890bfa4f36c9949871264e06ef4d449eeff9
{% endhighlight %}

![](/assets/PDB/RPI/RPI000883.png)

{% highlight bash %}
git format-patch -1 af4ca457eaf2d6682059c18463eb106e2ce58198 
vi 0001-PATCH-gfp_t-infrastructure.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000884.png)

该补丁为 Buddy 内存分配器的标志添加了 \_\_force 进行 sparse 静态强制类型
检测。

{% highlight bash %}
git format-patch -1 260b23674fdb570f3235ce55892246bef1c24c2a 
vi 0001-PATCH-gfp_t-the-rest.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000885.png)

该补丁继续将 Buddy 内存管理器核心函数内的 gfp 标志的类型设置为 gfp_t. 添加
了 highest_zone() 实现，这在建立每个 ZONE 的后备 ZONE 的时候提供了便捷
接口.

{% highlight bash %}
git format-patch -1 b5810039a54e5babf428e9a1e89fc1940fabff11 
vi 0001-PATCH-core-remove-PageReserved.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000886.png)

该补丁用于 Buddy 分配器在 put_page() 的时候，由原先检测 page 是否预留并且
没有其他地方引用它，修改成了只检测是否有其他地方引用它. Buddy 分配器在检测
某个页是否属于 Buddy 页的标志中移除了预留页的检测，在将 PG_reserved 和 
PG_writeback 两个标志加入到 free_page_check 过程中.

{% highlight bash %}
git format-patch -1 4c21e2f2441dc5fbb957b030333f5a3f2d02dea7 
vi 0001-PATCH-mm-split-page-table-lock.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000887.png)

该补丁将 struct page 的 private 成员与新的 ptl 成员组成新的联合体 u. 并且
提供了 private 成员的访问接口 "page_private()" 和 "set_page_private()".

{% highlight bash %}
git format-patch -1 ed8ece2ec8d3c2031b1a1a0737568bb0d49454e0 
vi 0001-PATCH-memory-hotplug-prep-break-out-zone-initializat.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000888.png)

该补丁用于当在 Buddy 内存分配器初始化过程中，如果 ZONE 是一个空的，那么
也要兼容空 ZONE 的初始化。

{% highlight bash %}
git format-patch -1 208d54e5513c0c02d85af0990901354c74364d5c
vi 0001-PATCH-memory-hotplug-locking-node_size_lock.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000889.png)

该补丁在 struct pglist_data 结构中添加了与 CONFIG_MEMORY_HOTPLUG 相关的
新成员 node_size_lock 锁。Buddy 内存分配器初始化过程中，会调用 
pgdat_resize_init() 函数.

{% highlight bash %}
git format-patch -1 bdc8cb984576ab5b550c8b24c6fa111a873503e3 
vi 0001-PATCH-memory-hotplug-locking-zone-span-seqlock.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000890.png)

该补丁向 struct zone 中增加了 span_seqlock 成员，该成员用于操作当前 ZONE
的 spanned_pages 成员时使用读写锁. 并在 Buddy 分配器初始化的时候添加对该
读写锁的初始化。

{% highlight bash %}
git format-patch -1 2f96996de0eda378df2a5f857ee1ef615ae10a4f 
vi 0001-PATCH-mm-wider-use-of-for_each_-cpu.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000891.png)

该补丁用于在 Buddy 管理器内部，show_free_areas() 在打印所有 ZONE 内存信息
的时候，将遍历所有 CPU 的函数提出按成了 for_each_cpu(), 该函数会遍历系统
所有可以使用的 CPU，因此不需要再次调用 cpu_possible() 进行确认了.

{% highlight bash %}
git format-patch -1 f8b8db77b0cc36670ef4ed6bc31e64537ffa197e 
vi 0001-PATCH-unexport-nr_swap_pages.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000892.png)

该补丁取消将 nr_swap_pages 导出到全局的符号表里.

{% highlight bash %}
git format-patch -1 6b482c6779daaa893b277fc9b70767a7c2e7c5eb
vi 0001-PATCH-Don-t-print-per-cpu-vm-stats-for-offline-cpus.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000893.png)

该补丁用于 Buddy 内存分配器的 show_free_area() 打印所有 ZONE 内存信息的时候，
遍历所有的 CPU，从原先的 for_each_cpu() 替换成了 for_each_online_cpu(),
这样将遍历当前正在使用的 CPU.

{% highlight bash %}
git format-patch -1 885036d32f5d3c427c3e2b385b5a5503805e3e52 
vi 0001-PATCH-mm-__GFP_NOFAIL-fix.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000894.png)

该补丁用于在 Buddy 内存分配器分配物理内存的时候，当没有可用物理内存分配的
时候，如果此时分配标志中包含了 \_\_GFP_NOFAIL, 那么函数就调用 
blk_congestion_write() 获得更多可用物理页，然后跳转到 nofail_alloc 处重新
分配物理内存.

{% highlight bash %}
git format-patch -1 7fb1d9fca5c6e3b06773b69165a73f3fb786b8ee 
vi 0001-PATCH-mm-__alloc_pages-cleanup.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000895.png)

该补丁重新整理了 Buddy 内存分配器 \_\_alloc_pages() 的过程，新建立了函数
get_page_from_freelist(), 并将函数替代的代码移除。并增加了 ALLOC_ 标志控制
整个分配过程.

{% highlight bash %}
git format-patch -1 2d6c666e8704cf06267f29a4fa3d2cf823469c38
vi 0001-PATCH-mm-gfp_noreclaim-cleanup.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000896.png)

该补丁用于从 Buddy 内存管理器中移除 \_\_GFP_NORECLAIM 标志.

{% highlight bash %}
git format-patch -1 669ed17521b9b78cdbeac8a53c30599aca9527ce 
vi 0001-PATCH-mm-highmem-watermarks.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000897.png)

该补丁为 ZONE_HIGHMEM 也添加了水位线.

{% highlight bash %}
git format-patch -1 a2f1b424900715ed9d1699c3bb88a434a2b42bc0
vi 0001-PATCH-x86_64-Add-4GB-DMA32-zone.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000898.png)

该补丁用于向 X86_64 系统 4GB 内存的情况下增加 DMA32 ZONE, 并未 ZONE_DMA32
设置后备 ZONE 分区。

{% highlight bash %}
git format-patch -1 07808b74e7dab1aa385e698795875337d72daf7d
vi 0001-PATCH-x86_64-Remove-obsolete-ARCH_HAS_ATOMIC_UNSIGNE.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000899.png)

该补丁将 struct page 的 flags 成员的类型修改为 unsigned long 型.

{% highlight bash %}
git format-patch -1 6b1de9161e973bac8c4675db608fe4f38d2689bd 
vi 0001-PATCH-VM-fix-zone-list-restart-in-page-allocatate.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000900.png)

该补丁微调正了 Buddy 分配器分配内存的细节.

{% highlight bash %}
git format-patch -1 664beed0190fae687ac51295694004902ddeb18e
vi 0001-PATCH-unpaged-unifdefed-PageCompound.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000901.png)

该补丁移除了 CONFIG_HUGETLB_PAGE 的一些设置.

{% highlight bash %}
git format-patch -1 689bcebfda16d7bace742740bfb3137fff30b529
vi 0001-PATCH-unpaged-PG_reserved-bad_page.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000902.png)

该补丁让预留页测试不能被 free 之后供 Buddy 分配器分配.

{% highlight bash %}
git format-patch -1 ac3461ad632e86e7debd871776683c05ef3ba4c6
vi 0001-Fix-up-GFP_ZONEMASK-for-GFP_DMA32-usage.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000903.png)

该补丁修改 GFP_ 标志与具体 ZONE 的匹配逻辑.

{% highlight bash %}
git format-patch -1 3148890bfa4f36c9949871264e06ef4d449eeff9 
vi 0001-PATCH-mm-__alloc_pages-cleanup-fix.patch 
{% endhighlight %}

![](/assets/PDB/RPI/RPI000904.png)

该补丁向 Buddy 内存管理器分配物理页时，新增了多个 ALLOC\_ 与水位线有关的标志.
更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### buddy 历史时间轴

![](/assets/PDB/RPI/RPI000905.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

#### buddy API

###### alloc_large_system_hash

{% highlight bash %}
void *__init alloc_large_system_hash(const char *tablename,
                                     unsigned long bucketsize,
                                     unsigned long numentries,
                                     int scale,
                                     int flags,
                                     unsigned int *_hash_shift,
                                     unsigned int *_hash_mask,
                                     unsigned long limit)
  作用: 为特定子系统分配一个指定大小的 Hash 表.
{% endhighlight %}

###### alloc_node_mem_map

{% highlight bash %}
static void __init alloc_node_mem_map(struct pglist_data *pgdat)
  作用: 为 Buddy 内存管理器分配 mem_map[] 数组，管理所有可用物理页.
{% endhighlight %}

###### \_\_alloc_pages

{% highlight bash %}
struct page * fastcall
__alloc_pages(gfp_t gfp_mask, unsigned int order,
                struct zonelist *zonelist)
  作用: Buddy 分配器分配物理页核心实现.
{% endhighlight %}

###### alloc_pages

{% highlight bash %}
#define alloc_pages(gfp_mask, order) \
                alloc_pages_node(numa_node_id(), gfp_mask, order)
  作用: 从 Buddy 管理器分配指定数量的物理页.
{% endhighlight %}

###### alloc_pages_node

{% highlight bash %}
static inline struct page *alloc_pages_node(int nid, gfp_t gfp_mask,
                                                unsigned int order)
  作用: Buddy 分配器从指定的 NODE 中分配指定数量的物理页.
{% endhighlight %}

###### bad_page

{% highlight bash %}
static void bad_page(const char *function, struct page *page)
  作用: 检查 page 是否是坏的.
{% endhighlight %}

###### bad_range

{% highlight bash %}
static int bad_range(struct zone *zone, struct page *page)
  作用: 检查页的范围是否合法.
{% endhighlight %}

###### buffered_rmqueue

{% highlight bash %}
static struct page *
buffered_rmqueue(struct zone *zone, int order, gfp_t gfp_flags)
  作用: 从 Buddy 内存管理器中请求指定数量的物理页.
{% endhighlight %}

###### build_all_zonelists

{% highlight bash %}
void __init build_all_zonelists(void)
  作用: Buddy 管理器为每个 ZONE 设置后备 ZONE 信息.
{% endhighlight %}

###### build_zonelists

{% highlight bash %}
static void __init build_zonelists(pg_data_t *pgdat)
  作用: Buddy 管理器为每个 ZONE 设置后备 ZONE 信息.
{% endhighlight %}

###### calculate_zone_totalpages

{% highlight bash %}
static void __init calculate_zone_totalpages(struct pglist_data *pgdat,
                unsigned long *zones_size, unsigned long *zholes_size)
  作用: 统计每个 NODE 上物理页的信息，即可用物理页、真实可用物理页、物理页
        的数量.
{% endhighlight %}

###### expand

{% highlight bash %}
static inline struct page *
expand(struct zone *zone, struct page *page,
        int low, int high, struct free_area *area)
  作用: Buddy 管理器从其他 free_area[] 获得更大块的物理页块，将其从中间拆分
        成两块一样的物理页块，然后将其中一块插入到低一级的 free_area[] 链表
        中，如果此时另外一块物理页块还是大于需求的大小，那么 Buddy 继续循环
        上面的操作，知道拆分到合适大小的物理页块, 并返回该物理页.
{% endhighlight %}

###### \_\_find_combined_index

{% highlight bash %}
static inline unsigned long
__find_combined_index(unsigned long page_idx, unsigned int order)
  作用: Buddy 管理器获得 "Buddy" 物理页的索引号.
{% endhighlight %}

###### for_each_pgdat

{% highlight bash %}
#define for_each_pgdat(pgdat) \
        for (pgdat = pgdat_list; pgdat; pgdat = pgdat->pgdat_next)
  作用: 遍历所有的节点.
{% endhighlight %}

###### for_each_zone

{% highlight bash %}
#define for_each_zone(zone) \
        for (zone = pgdat_list->node_zones; zone; zone = next_zone(zone))
  作用: 遍历所有的 ZONE 分区.
{% endhighlight %}

###### free_area_init

{% highlight bash %}
void __init free_area_init(unsigned long *zones_size)
  作用: 初始化 Buddy 管理器.
{% endhighlight %}

###### free_area_init_core

{% highlight bash %}
static void __init free_area_init_core(struct pglist_data *pgdat,
                unsigned long *zones_size, unsigned long *zholes_size)
  作用: Buddy 管理器初始化核心程序.
{% endhighlight %}

###### free_area_init_node

{% highlight bash %}
void __init free_area_init_node(int nid, struct pglist_data *pgdat,
                unsigned long *zones_size, unsigned long node_start_pfn,
                unsigned long *zholes_size)
  作用: Buddy 管理器初始化指定节点上的所有 ZONE 分区.
{% endhighlight %}

###### \_\_free_pages

{% highlight bash %}
fastcall void __free_pages(struct page *page, unsigned int order)
  作用: Buddy 管理器释放指定数量的物理页.
{% endhighlight %}

###### free_pages

{% highlight bash %}
fastcall void free_pages(unsigned long addr, unsigned int order)
  作用: Buddy 管理器释放指定数量且已经映射的物理页.
{% endhighlight %}

###### \_\_free_pages_bulk

{% highlight bash %}
static inline void __free_pages_bulk (struct page *page,
                struct zone *zone, unsigned int order)
  作用: 释放一定数量的物理页块到 Buddy 内存管理器.
{% endhighlight %}

###### free_pages_bulk

{% highlight bash %}
static int
free_pages_bulk(struct zone *zone, int count,
                struct list_head *list, unsigned int order)
  作用: 释放一定量的物理页块到 Buddy 管理器里.
{% endhighlight %}

###### free_pages_check

{% highlight bash %}
static inline int free_pages_check(const char *function, struct page *page)
  作用: Buddy 管理器对即将释放的页进行检测, 检测是否满足释放的条件.
{% endhighlight %}

###### \_\_free_pages_ok

{% highlight bash %}
void __free_pages_ok(struct page *page, unsigned int order)
  作用: 释放指定页到 Buddy 管理器里.
{% endhighlight %}

###### \_\_get_dma_pages

{% highlight bash %}
#define __get_dma_pages(gfp_mask, order) \
                __get_free_pages((gfp_mask) | GFP_DMA,(order))
  作用: 从 Buddy 管理器分配指定数量的 DMA 页，并返回其对应的虚拟地址.
{% endhighlight %}

###### \_\_get_free_pages

{% highlight bash %}
fastcall unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order)
  作用: 从 Buddy 分配器中分配指定数量的物理页，并返回物理页对应的虚拟地址.
{% endhighlight %}

###### get_page_from_freelist

{% highlight bash %}
static struct page *
get_page_from_freelist(gfp_t gfp_mask, unsigned int order,
                struct zonelist *zonelist, int alloc_flags)
  作用: Buddy 管理器从指定 ZONE 的 free_area[] 链表中获得指定数量的物理页.
{% endhighlight %}

###### get_zeroed_page

{% highlight bash %}
fastcall unsigned long get_zeroed_page(gfp_t gfp_mask)
  作用: 从 Buddy 分配器中分配一个空白的页，并且获得物理页对应的虚拟地址.
{% endhighlight %}

###### highest_zone

{% highlight bash %}
static inline int highest_zone(int zone_bits)
  作用: 设置 ZONE 的最大备用 ZONE.
{% endhighlight %}

###### init_currently_empty_zone

{% highlight bash %}
static __devinit void init_currently_empty_zone(struct zone *zone,
                unsigned long zone_start_pfn, unsigned long size)
  作用: Buddy 管理器初始化一个空的 ZONE 分区.
{% endhighlight %}

###### init_per_zone_pages_min

{% highlight bash %}
static int __init init_per_zone_pages_min(void)
  作用: 设置 ZONE 的水位线.
{% endhighlight %}

###### is_highmem

{% highlight bash %}
static inline int is_highmem(struct zone *zone)
  作用: 判断 ZONE 是否是 ZONE_HIGHMEM
{% endhighlight %}

###### is_highmem_idx

{% highlight bash %}
static inline int is_highmem_idx(int idx)
  作用: 判断 ZONE 索引对应的 ZONE 是否是 ZONE_HIGHMEM.
{% endhighlight %}

###### is_normal

{% highlight bash %}
static inline int is_normal(struct zone *zone)
  作用: 判断 ZONE 是否是 ZONE_NORMAL。
{% endhighlight %}

###### is_normal_idx

{% highlight bash %}
static inline int is_normal_idx(int idx)
  作用: 判断 ZONE 所以对应的 ZONE 是否是 ZONE_NORMAL.
{% endhighlight %}

###### memmap_init_zone

{% highlight bash %}
void __devinit memmap_init_zone(unsigned long size, int nid, unsigned long zone,
                unsigned long start_pfn)
  作用: Buddy 内存管理器初始化 ZONE 分区上的每个 page.
{% endhighlight %}

###### \_\_mod_page_state

{% highlight bash %}
void __mod_page_state(unsigned long offset, unsigned long delta)
  作用: 读取 Buddy 管理器中指定页的使用情况.
{% endhighlight %}

###### next_zone

{% highlight bash %}
static inline struct zone *next_zone(struct zone *zone)
  作用: 从 ZONELIST 中获得下一个可用的 ZONE.
{% endhighlight %}

###### nr_free_buffer_pages

{% highlight bash %}
unsigned int nr_free_buffer_pages(void)
  作用: 读取 Buddy 管理器中 Buffer 中物理页的数量.
{% endhighlight %}

###### nr_free_highpages

{% highlight bash %}
unsigned int nr_free_highpages (void)
  作用: 读取 Buddy 管理器中 ZONE_HIGHMEM 中物理页的数量.
{% endhighlight %}

###### nr_free_pagecache_pages

{% highlight bash %}
unsigned int nr_free_pagecache_pages(void)
  作用: 读取 Buddy 管理器中缓存的物理页数量.
{% endhighlight %}

###### nr_free_pages

{% highlight bash %}
unsigned int nr_free_pages(void)
  作用: 读取 Buddy 管理器中所有 ZONE 中空闲物理页的数量.
{% endhighlight %}

###### nr_free_zone_pages

{% highlight bash %}
static unsigned int nr_free_zone_pages(int offset)
  作用: 读取 Buddy 管理器中所有 ZONE 的物理页数量.
{% endhighlight %}

###### \_\_page_find_buddy

{% highlight bash %}
static inline struct page *
__page_find_buddy(struct page *page, unsigned long page_idx, unsigned int order)
  作用: Buddy 管理器查找 page 对应的 "Buddy" page.
{% endhighlight %}

###### page_is_buddy

{% highlight bash %}
static inline int page_is_buddy(struct page *page, int order)
  作用: Buddy 管理器判断物理页块是否属于 Buddy 管理器.
{% endhighlight %}

###### prep_new_page

{% highlight bash %}
static int prep_new_page(struct page *page, int order)
  作用: Buddy 管理器从 free_area[] 中获得可用物理页块之后，将物理页块进行
        一些初始化.
{% endhighlight %}

###### page_order

{% highlight bash %}
static inline unsigned long page_order(struct page *page)
  作用: 读取 page 的 private 值.
{% endhighlight %}

###### pfn_valid

{% highlight bash %}
static inline int pfn_valid(unsigned long pfn)
  作用: 判断一个 PFN 是否有效.
{% endhighlight %}

###### prep_zero_page

{% highlight bash %}
static inline void prep_zero_page(struct page *page, int order, gfp_t gfp_flags)
  作用: Buddy 分配器将指定的数量的页内容全部清零.
{% endhighlight %}

###### gfp_zone

{% highlight bash %}
static inline int gfp_zone(gfp_t gfp)
  作用: 通过 GFP 标志获得对应的 ZONE 分区.
{% endhighlight %}

###### \_\_read_page_state

{% highlight bash %}
unsigned long __read_page_state(unsigned long offset)
  作用: 读取 Buddy 管理器中指定页的使用情况.
{% endhighlight %}

###### \_\_rmqueue

{% highlight bash %}
static struct page *__rmqueue(struct zone *zone, unsigned int order)
  作用: Buddy 管理器从 free_area[] 链表中获得大块物理页块之后，将拆分之后的
        物理页块插入指定的 free_area[] 链表，剩余的物理页块返回给调用者.
{% endhighlight %}

###### rmqueue_bulk

{% highlight bash %}
static int rmqueue_bulk(struct zone *zone, unsigned int order,
                        unsigned long count, struct list_head *list)
  作用: 从 Buddy 管理器中分配物理页块.
{% endhighlight %}

###### rmv_page_order

{% highlight bash %}
static inline void rmv_page_order(struct page *page)
  作用: 清除 page 的 PG_private 标志，并将 page 的 private 设置为 0.
{% endhighlight %}

###### set_hashdist

{% highlight bash %}
static int __init set_hashdist(char *str)
  作用: 从 cmdline 中解析 "hashdist" 参数，该参数用于设置 VFS 相关的 Hash 表.
{% endhighlight %}

###### set_page_order

{% highlight bash %}
static inline void set_page_order(struct page *page, int order)
  作用: 设置 page 的 private.
{% endhighlight %}

###### set_page_refs

{% highlight bash %}
void set_page_refs(struct page *page, int order)
  作用: 设置 Page 的引用计数.
{% endhighlight %}

###### setup_per_zone_lowmem_reserve

{% highlight bash %}
static void setup_per_zone_lowmem_reserve(void)
  作用: 设置系统中所有 ZONE 的水位线.
{% endhighlight %}

###### setup_per_zone_pages_min

{% highlight bash %}
void setup_per_zone_pages_min(void)
  作用: 设置所有 ZONE 的 pages_low、pages_min 和 pages_high.
{% endhighlight %}

###### show_free_areas

{% highlight bash %}
void show_free_areas(void)
  作用: Buddy 管理器用于打印 ZONE 上所有使用情况.
{% endhighlight %}

###### zoneinfo_show

{% highlight bash %}
static int zoneinfo_show(struct seq_file *m, void *arg)
  作用: 通过 /proc/zoneinfo 打印所有 ZONE 的信息.
{% endhighlight %}

###### zone_idx

{% highlight bash %}
#define zone_idx(zone)          ((zone) - (zone)->zone_pgdat->node_zones)
  作用: 获得 ZONE 对应的 ZONE ID.
{% endhighlight %}

###### zone_init_free_lists

{% highlight bash %}
void zone_init_free_lists(struct pglist_data *pgdat, struct zone *zone,
                                unsigned long size)
  作用: Buddy 管理器初始化每个 ZONE 的 free_area[] 数组.
{% endhighlight %}

###### zone_wait_table_init

{% highlight bash %}
static __devinit
void zone_wait_table_init(struct zone *zone, unsigned long zone_size_pages)
  作用: Buddy 管理器初始化每个 ZONE 的 wait_table.
{% endhighlight %}

###### zone_watermark_ok

{% highlight bash %}
int zone_watermark_ok(struct zone *z, int order, unsigned long mark,
                      int classzone_idx, int alloc_flags)
  作用: Buddy 管理器检查当前 ZONE 的可用物理页是否已经低于 ZONE 的水位线.
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

#### buddy 进阶研究

> - [Buddy-Normal Memory Allocator On Userspace](https://biscuitos.github.io/blog/Memory-Userspace/#E)
>
> - [Buddy-HighMEM Memory Allocator On Userspace](https://biscuitos.github.io/blog/Memory-Userspace/#F)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### buddy 分配器调试

> - [/proc/zoneinfo](#E0000)
>
> - [/proc/buddyinfo](#E0001)
>
> - [BiscuitOS Buddy 内存分配器调试](#C0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------

#### <span id="E0000">/proc/zoneinfo</span>

Linux 内核从 linux 2.6.13 开始向 Proc 文件系统添加了 zoneinfo 节点，用于
获得 ZONE 的所有使用信息，开发者参考下列命令使用:

{% highlight bash %}
cat /proc/zoneinfo
{% endhighlight %}

![](/assets/PDB/RPI/RPI000906.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------

#### <span id="E0001">/proc/buddyinfo</span>

Linux 内核向 Proc 文件系统中添加了 buddyinfo 节点，用于获得每个 NODE 的
free_area[] 各个链表的使用情况, 开发者参考下列命令使用:

{% highlight bash %}
cat /proc/buddyinfo
{% endhighlight %}

![](/assets/PDB/RPI/RPI000907.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
