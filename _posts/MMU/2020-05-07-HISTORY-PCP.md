---
layout: post
title:  "PCP Allocator"
date:   2020-05-07 09:37:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [PCP 分配器原理](#A)
>
> - [PCP 分配器使用](#B)
>
> - [PCP 分配器实践](#C)
>
> - [PCP 源码分析](#D)
>
> - [PCP 分配器调试](#E)
>
> - [PCP 分配进阶研究](#F)
>
> - [PCP 时间轴](#G)
>
> - [PCP 历史补丁](#H)
>
> - [PCP API](#K)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### PCP 分配器原理

![](/assets/PDB/RPI/RPI000908.png)

PCP 内存分配器又称为 "Per CPU PageSet Allocator", 顾名思义就是用于从每个
CPU 的物理页集合中分配物理页的内存管理器。PCP 分配器在每个 ZONE 区间上
为每个 CPU 提供了两个物理页链表，每个链表上维护了一个定数量的单个可用物理
页。当系统请求分配当个物理页的时候，内存管理子系统就将这个任务让 PCP 内存
管理器去完成，而不是交给 Buddy 内存管理器.

###### PCP 的由来

PCP 内存管理器和 Buddy 内存管理器都是管理物理内存。它们之间有着千丝万缕的
联系， 这里通过讲述一个例子来讲解内核为什么要多增加一个内存管理器管理物理
内存。

![](/assets/PDB/RPI/RPI000909.png)

当系统向 Buddy 内存分配器请求一个物理页的时候，Buddy 内存分配器就去指定的
ZONE 的 free_area[0] 查找可用物理页，正好 free_area[0] 上包含了可用物理页，
那么 Buddy 分配器将一个物理页从 free_area[0] 链表中移除，并传递给请求者。
整个过程似乎挺顺利, 效率还过得去.

![](/assets/PDB/RPI/RPI000910.png) 


接着系统又向 Buddy 分配器请求一个物理页，此时 Buddy 又去指定的 Zone 的 
free_area[0] 上查找可用的物理页，可是这时 free_area[0] 上没有可用的物理页，
这时 Buddy 管理器就去 free_area[1] 上查找可用的物理页，如果 free_area[1] 上
有可用物理页块，那么将该页块从 free_area[1] 的链表中移除，然后拆分成两块，然
后将其中一块插入到 free_area[0] 的链表上，而将另外一块物理页直接传递给请求者, 
这种情况下，会比前一种情况效率低一些

![](/assets/PDB/RPI/RPI000911.png)

接着系统又向 Buddy 分配器请求一个物理页，此时 Buddy 又去指定的 Zone 的 
free_area[0] 上查找可用的物理页，可此时 free_area[0] 上没有可用的物理页。
Buddy 继续去 free_area[1] 上查找，可此时 free_area[1] 上没有可用的物理页块，
free_area[2] 上也没有，直到 free_area[m] 链表上采用可用物理页块，那么 Buddy
就重复拆分插入炒作，直到获得一个可用物理页，这样的效率明显比第一种低多了.
对于向 Buddy 内存管理器释放一个物理页，同样也存在性能差异:

![](/assets/PDB/RPI/RPI000912.png)

当向系统释放一个物理页的时候，系统首先获得该物理页的 "Buddy" 物理页信息，
检测该 "Buddy" 物理页是否正存储在指定 ZONE 分区的 free_area[0] 链表上，
如果 "Buddy" 物理页不存在，那么直接将释放的物理页插入到指定 ZONE 分区的
free_area[0] 链表上.

![](/assets/PDB/RPI/RPI000913.png)

当向系统释放一个物理页的时候，系统查找该物理页的 "Buddy" 物理页信息，检测
到 "Buddy" 物理页正好存储在指定 ZONE 分区的 free_area[0] 链表上，并且可以
合并，那么 Buddy 管理器就将 "Buddy" 物理页从 free_area[0] 链表中移除，然后
与物理页合并称为两个物理页的物理块。此时 Buddy 管理器又查找新合成物理块的
"Buddy" 物理页是否存在于指定 ZONE 分区的 free_area[1] 链表上，如上图所示，
"Buddy" 物理页正好在 free_area[1] 上，并且可以合并，那么 Buddy 管理器就将
"Buddy" 物理页块从 free_area[1] 链表上移除，并合成新的物理页块，Buddy 管理
器继续重复上面的操作，直到找不到 "Buddy" 物理页块或者已经查找到 
free_area[MAX_ORDER-1] 的链表上了. 这样的情况下释放一个物理页的占用的时间
越来越大，效率越来越低.

基于上面涉及的 Buddy 管理器分配和释放情况，如果系统高频率向 Buddy 管理器
请求一个物理页或者释放一个物理页，那么操作系统整体运行速度会下降很多。于是
内核进行改进:

![](/assets/PDB/RPI/RPI000914.png)

上图就是改进的第一种方案，在 ZONE 分区上维护一个链表，在初始化阶段从 Buddy
分配器上申请多个单个的物理页插入到链表里。当系统申请一个物理页的时候，那么
就从该链表中获得，当系统释放一个物理页的时候，就将物理页插入到链表上。

这样改进的优点是基本形成了 PCP 雏形，能够一定程度上缓解分配单个物理页效率
的问题。但缺点也存在，如果系统某时刻大量申请单个物理页，而很少释放物理页，
那么链表的功能就退化成从 Buddy 分配器上分配单个物理页。而某些时刻系统释放
大量的物理页，而申请少量的物理页，这样会造成链表变的特别大。

![](/assets/PDB/RPI/RPI000915.png)

将方案进行改进之后，ZONE 分区上维护的链表总是保持在一定数量的物理页，
如果系统申请大量单个物理页之后，链表从 Buddy 分配器中分配一定数量物理
页，从而保持链表中物理页的个数。当系统频繁的释放大量的单个物理页，那么
在该链表未达到指定个数的情况下，将其插入到链表，而超过之后则将其归还给
Buddy 分配器。

这样改进优点是维护了这个链表的长度，使其正常生长。但缺点也很明显，当链表
上已经维护了指定数量的物理页之后，还是将物理页归还给 Buddy 管理器。

![](/assets/PDB/RPI/RPI000916.png)

将方案进行改进之后，ZONE 分区上维护两个链表，每个链表上维护指定数量的
物理页，一个称为 "Hot" 链表，用于存储刚被申请不久又被释放的物理页; 另外
一个称为 "Cold" 链表，用于存储被释放一段时间的物理页表。当链表中的物理页
太多的时候，就一次性将多个物理页返回给 Buddy 内存分配器。

这样做的好处就是尽可能让单个物理页的申请留在这两个链表上，而很少从
Buddy 内存分配器中分配或释放单个物理页。当也存在问题，就是遇到 SMP
系统，多个 CPU 从这个链表上分配单个物理页，也会造成效率低下.

![](/assets/PDB/RPI/RPI000917.png)

将方案改进之后，ZONE 分区为每个 CPU 维护一套 "Hot&Cold" 链表，每个 CPU
就可以独立分配和释放单个物理页. 以上只是对 PCP 内存管理器的技术推演，也
存在一些遗漏和不准确的地方，但总体技术变迁一致.

#### PCP 内存管理器介绍

![](/assets/PDB/RPI/RPI000908.png)

通过上面的技术推演之后，大概知道 PCP 内存分配器的作用和意义了。PCP 内存分配
器严格来讲是属于 Buddy 内存分配器，但从功能上又独立与 Buddy 内存分配器，因此
本文将 PCP 独立出来进行介绍。PCP 内存分配器的架构如上图, PCP 内核分配器的数
据存在于每个 ZONE 分区上，struct zone 结构中包含 pageset 成员，其是 
"struct per_cpu_pageset" 结构，该结构定义如下:

{% highlight bash %}
struct per_cpu_pages {
        int count;              /* number of pages in the list */
        int low;                /* low watermark, refill needed */
        int high;               /* high watermark, emptying needed */
        int batch;              /* chunk size for buddy add/remove */
        struct list_head list;  /* the list of pages */
};

struct per_cpu_pageset {
        struct per_cpu_pages pcp[2]; /* 0: hot.  1: cold */
#ifdef CONFIG_NUMA
        unsigned long numa_hit;         /* allocated in intended node */
        unsigned long numa_miss;        /* allocated in non intended node */
        unsigned long numa_foreign;     /* was intended here, hit elsewhere */
        unsigned long interleave_hit;   /* interleaver prefered this zone */
        unsigned long local_node;       /* allocation from local node */
        unsigned long other_node;       /* allocation from other node */
#endif
} ____cacheline_aligned_in_smp;

struct zone {
        /* Fields commonly accessed by the page allocator */
        unsigned long           free_pages;
        unsigned long           pages_min, pages_low, pages_high;
        unsigned long           lowmem_reserve[MAX_NR_ZONES_BS];

#ifdef CONFIG_NUMA
        struct per_cpu_pageset       *pageset[NR_CPUS];
#else
        struct per_cpu_pageset       pageset[NR_CPUS];
#endif

....
}

{% endhighlight %}

PCP 内存分配器之所以和 Buddy 内存分配器连续紧密是因此他们都公用一套代码
函数，在调用公共函数分配物理内存的时候，内存管理子系统会根据分配的物理页
数量选择是从 PCP 内存分配器分配还是从 Buddy 内存管理分配器分配。

每个 ZONE 分区上的 PCP 分配器都使用 struct per_cpu_pages 结构进行维护，
count 成员用于指定 PCP 冷热页链表的物理页数量, low 成员用于标示 PCP 内存
分配器的分配水位线，high 成员用于标示 PCP 内存分配器的释放水位线，batch 
成员用于指定 PCP 分配器与 Buddy 分配器交换物理页的数量.

![](/assets/PDB/RPI/RPI000944.png)

PCP 内存分配器在分配和释放物理页的时候，总会检测当前 ZONE 分区的 PCP 信息。
当 PCP 冷热页链表的数量小于 low 成员的时候，PCP 就向 Buddy 内存分配器分配
指定数量物理页到 PCP 指定的页链表里。当 PCP 内存分配器页链表的物理页数大于 
high 水位线之后，PCP 内存分配器就将页链表上的一定数量的物理页一次性释放给 
Buddy 内存。

{% highlight bash %}
alloc_page(gfp)
alloc_pages(gfp, 0)
free_page()
__free_page()
free_pages(x, 0)
__free_pages(x, 0)
{% endhighlight %}

PCP 内存管理器的函数和 Buddy 内存管理器兼容，可以通过 Buddy 内存分配器提供
的函数从 PCP 内存管理器中分配. 同时也可以通过 Buddy 内存分配器提供的释放
函数将物理页释放到 PCP 内存管理器中.

---------------------------------

###### PCP 的优点

CPU 可以快速从 PCP 管理器中分配或释放单个物理页。

###### PCP 的缺点

为每个 CPU 都要维护一定数量的物理页，有时也会造成浪费.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### PCP 分配器使用

> - [基础用法介绍](#B0000)
>
> - [PCP 从 ZONE_DMA 中分配物理页](#B0001)
>
> - [PCP 从 ZONE_DMA32 中分配物理页](#B0002)
>
> - [PCP 从 ZONE_NORMAL 中分配物理页](#B0003)
>
> - [PCP 从 ZONE_HIGHMEM 中分配物理页](#B0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="B0000">基础用法介绍</span>

与 Buddy 物理内存分配器一样，PCP 分配器也提供了用于分配和释放的相关函数接口:

###### PCP 分配

{% highlight bash %}
alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0001">PCP 从 ZONE_DMA 中分配物理页</span>

![](/assets/PDB/RPI/RPI000918.png)

PCP 从 ZONE_DMA 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/gfp.h>

static int TestCase_alloc_page_from_DMA_PCP(void)
{
        struct page *page;
        void *addr;

        /* allocate page from PCP (DMA) */
        page = alloc_page(GFP_DMA);
        if (!page) {
                printk("%s alloc page failed\n", __func__);
                return -ENOMEM;
        }

        /* Obtain page virtual address */
        addr = page_address(page);
        if (!addr) {
                printk("%s bad page address!\n", __func__);
                return -EINVAL;
        }
        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

        /* free all pages to PCP-hot */
        free_page((unsigned long)addr);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0002">PCP 从 ZONE_DMA32 中分配物理页</span>

![](/assets/PDB/RPI/RPI000919.png)

PCP 从 ZONE_DMA32 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/gfp.h>

static int TestCase_alloc_page_from_DMA32_PCP(void)
{
        struct page *page;
        void *addr;

        /* allocate page from PCP (DMA32) */
        page = alloc_page(GFP_DMA32);
        if (!page) {
                printk("%s alloc page failed\n", __func__);
                return -ENOMEM;
        }

        /* Obtain page virtual address */
        addr = page_address(page);
        if (!addr) {
                printk("%s bad page address!\n", __func__);
                return -EINVAL;
        }
        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

        /* free all pages to PCP-hot */
        free_page((unsigned long)addr);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0003">PCP 从 ZONE_NORMAL 中分配物理页</span>

![](/assets/PDB/RPI/RPI000921.png)

PCP 从 ZONE_NORMAL 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/gfp.h>

static int TestCase_alloc_page_from_NORMAL_PCP(void)
{
        struct page *page;
        void *addr;

        /* allocate page from PCP (NORMAL) */
        page = alloc_page(GFP_KERNEL);
        if (!page) {
                printk("%s alloc page failed\n", __func__);
                return -ENOMEM;
        }

        /* Obtain page virtual address */
        addr = page_address(page);
        if (!addr) {
                printk("%s bad page address!\n", __func__);
                return -EINVAL;
        }
        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

        /* free all pages to PCP-hot */
        free_page((unsigned long)addr);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0004">PCP 从 ZONE_HIGHMEM 中分配物理页</span>

![](/assets/PDB/RPI/RPI000922.png)

PCP 从 ZONE_HIGHMEM 中分配物理内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/highmem.h>

static int TestCase_alloc_page_from_HIGHMEM_PCP(void)
{
        struct page *page;

        /* allocate page from PCP (HIGHMEM) */
        page = alloc_page(__GFP_HIGHMEM);
        if (!page) {
                printk("%s alloc page failed\n", __func__);
                return -ENOMEM;
        }

	/* Mapping ... */

        /* free all pages to PCP-hot */
        __free_page(page);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### PCP 分配器实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)
>
> - [实践建议](/blog/HISTORY-MMU/#C0003)
>
> - [测试建议](#C0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

本实践是基于 BiscuitOS Linux 5.0 ARM32 环境进行搭建，因此开发者首先
准备实践环境，请查看如下文档进行搭建:

> - [BiscuitOS Linux 5.0 ARM32 环境部署](/blog/Linux-5.0-arm32-Usermanual/)

--------------------------------------------

#### <span id="C0001">实践部署</span>

准备好基础开发环境之后，开发者接下来部署项目所需的开发环境。由于项目
支持多个版本的 PCP，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 PCP 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](/assets/PDB/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](/assets/PDB/RPI/RPI000920.png)

选择并进入 "[\*]   PCP(Hot-Cold Page) Allocator  --->" 目录。

![](/assets/PDB/RPI/RPI000923.png)

选择 "[\*]   PCP on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PCP-2.6.12
make download
{% endhighlight %}

![](/assets/PDB/RPI/RPI000924.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](/assets/PDB/RPI/RPI000925.png)

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
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PCP-2.6.12
make kernel
{% endhighlight %}

![](/assets/PDB/RPI/RPI000926.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PCP-2.6.12
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000927.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PCP-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PCP-2.6.12
make run
{% endhighlight %}

![](/assets/PDB/RPI/RPI000928.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_PCP-2.6.12.ko"，打印如上信息，那么
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
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PCP-2.6.12/BiscuitOS_PCP-2.6.12/Makefile
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

然后先向 BiscuitOS 中插入 "BiscuitOS_PCP-2.6.12.ko" 模块，然后再插入
"BiscuitOS_PCP-2.6.12-buddy.ko" 模块。如下:

![](/assets/PDB/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](/assets/PDB/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

Initcall 机制提供了以下函数用于 PCP 调试:

{% highlight bash %}
pcp_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，pcp_initcall_bs() 调用的函数将
在 PCP 分配器初始化完毕之后自动调用。PCP 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PCP-2.6.12/BiscuitOS_PCP-2.6.12/module/pcp/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/PCP/main.o
{% endhighlight %}

PCP 测试代码也包含模块测试，在 Makefile 中打开调试开关:

{% highlight bash %}
obj-m                         += $(MODULE_NAME)-pcp.o
$(MODULE_NAME)-pcp-m          := modules/pcp/module.o
{% endhighlight %}

PCP 模块测试结果如下:

![](/assets/PDB/RPI/RPI000929.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### PCP 历史补丁

> - [PCP Linux 2.6.12](#H-linux-2.6.12)
>
> - [PCP Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [PCP Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [PCP Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [PCP Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [PCP Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [PCP Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [PCP Linux 2.6.13](#H-linux-2.6.13)
>
> - [PCP Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [PCP Linux 2.6.14](#H-linux-2.6.14)
>
> - [PCP Linux 2.6.15](#H-linux-2.6.15)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](/assets/PDB/RPI/RPI000785.JPG)

#### PCP Linux 2.6.12

Linux 2.6.12 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](/assets/PDB/RPI/RPI000786.JPG)

#### PCP Linux 2.6.12.1

Linux 2.6.12.1 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

与 linux 2.6.12 相比，该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](/assets/PDB/RPI/RPI000787.JPG)

#### PCP Linux 2.6.12.2

Linux 2.6.12.2 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

与 linux 2.6.12.1 相比，该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](/assets/PDB/RPI/RPI000788.JPG)

#### PCP Linux 2.6.12.3

Linux 2.6.12.3 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

与 linux 2.6.12.2 相比，该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](/assets/PDB/RPI/RPI000789.JPG)

#### PCP Linux 2.6.12.4

Linux 2.6.12.4 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

与 linux 2.6.12.3 相比，该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](/assets/PDB/RPI/RPI000790.JPG)

#### PCP Linux 2.6.12.5

Linux 2.6.12.5 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

与 linux 2.6.12.4 相比，该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](/assets/PDB/RPI/RPI000791.JPG)

#### PCP Linux 2.6.12.6

Linux 2.6.12.6 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

与 linux 2.6.12.5 相比，该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](/assets/PDB/RPI/RPI000792.JPG)

#### PCP Linux 2.6.13

Linux 2.6.13 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相对上一个版本 linux 2.6.12.6，Buddy 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/page_alloc.c include/linux/gfp.h

2005-06-21 17:14 Christoph Lameter o [PATCH] node local per-cpu-pages
                                     [main] e7c8d5c9955a4d2e88e36b640563f5d6d5aba48a
2005-06-21 17:14 Christoph Lameter o [PATCH] Periodically drain non local pagesets
                                     [main] 4ae7c03943fca73f23bc0cdb938070f41b98101f
2005-06-21 17:15 Christoph Lameter o [PATCH] Reduce size of huge boot per_cpu_pageset
                                     [main] 2caaad41e4aa8f5dd999695b4ddeaa0e7f3912a4
2005-06-22 20:26 Christoph Lameter o [PATCH] boot_pageset must not be freed.
                                     [main] b7c84c6ada2be942eca6722edb2cfaad412cd5de

{% endhighlight %}

![](/assets/PDB/RPI/RPI000930.png)

{% highlight bash %}
git format-patch -1 e7c8d5c9955a4d2e88e36b640563f5d6d5aba48a
vi 0001-PATCH-node-local-per-cpu-pages.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000931.png)

该补丁添加了 NUMA 体系下获得指定 ZONE 分区的 PCP，并在 struct zone 中支持
NUMA 和 UMA 模式的 PCP 数组. start_kernel() 函数中添加了对 
setup_per_cpu_pageset() 函数的调用. NUMA 系统中添加 pageset_table[].
新增 PCP 的 batchsize 计算方法.

{% highlight bash %}
git format-patch -1 4ae7c03943fca73f23bc0cdb938070f41b98101f
vi 0001-PATCH-Periodically-drain-non-local-pagesets.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000932.png)

该补丁用于在 /proc/zoneinfo 打印 ZONE 分区信息时，添加了对 PCP count 数的
打印.

{% highlight bash %}
git format-patch -1 2caaad41e4aa8f5dd999695b4ddeaa0e7f3912a4
vi 0001-PATCH-Reduce-size-of-huge-boot-per_cpu_pageset.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000933.png)

该补丁将 PCP 内存管理器的初始化转移到 setup_pageset() 函数中，并修改了
PCP 页释放时的策略.

{% highlight bash %}
git format-patch -1 b7c84c6ada2be942eca6722edb2cfaad412cd5de
vi 0001-PATCH-boot_pageset-must-not-be-freed.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000934.png)

该补丁修改了 boot_pageset[] 的定义，撤销了 \_\_initdata 的限定. 更多
补丁使用方法，请参考下面文章:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](/assets/PDB/RPI/RPI000793.JPG)

#### PCP Linux 2.6.13.1

Linux 2.6.13.1 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

与 linux 2.6.13 相比，该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](/assets/PDB/RPI/RPI000794.JPG)

#### PCP Linux 2.6.14

Linux 2.6.14 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相对上一个版本 linux 2.6.13，Buddy 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/page_alloc.c include/linux/gfp.h

2005-09-06 15:18 Paul Jackson           o [PATCH] cpusets: formalize intermediate GFP_KERNEL containment
                                          [main] 9bf2229f8817677127a60c177aefce1badd22d7b
2005-10-26 01:58 Magnus Damm            o [PATCH] NUMA: broken per cpu pageset counters
                                          [main] 1c6fe9465941df04a1ad8f009bd6d95b20072a58
{% endhighlight %}

![](/assets/PDB/RPI/RPI000935.png)

{% highlight bash %}
git format-patch -1 9bf2229f8817677127a60c177aefce1badd22d7b
vi 0001-PATCH-cpusets-formalize-intermediate-GFP_KERNEL-cont.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000936.png)

该补丁添加了对 cpuset_zone_allowed() 的支持.

{% highlight bash %}
git format-patch -1 1c6fe9465941df04a1ad8f009bd6d95b20072a58
vi 0001-PATCH-NUMA-broken-per-cpu-pageset-counters.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000937.png)

PCP 内存分配器初始化过程中，将 PCP 清零。更多补丁使用请参考如下文档:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](/assets/PDB/RPI/RPI000795.JPG)

#### PCP Linux 2.6.15

Linux 2.6.12.5 依旧采用 PCP 作为单个物理页申请和释放的内存管理器。

![](/assets/PDB/RPI/RPI000908.png)

###### PCP 分配

{% highlight bash %}
alloc_page()
__alloc_page()
{% endhighlight %}

###### PCP 释放

{% highlight bash %}
free_page()
__free_page()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PCP API](#K)

###### 与项目相关

PCP 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000850.png)

###### 补丁

相对上一个版本 linux 2.6.14，Buddy 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/page_alloc.c include/linux/gfp.h

2005-10-29 18:15 Seth, Rohit            o [PATCH] mm: page_alloc: increase size of per-cpu-pages
                                          [main] ba56e91c940146e99ac694c4c7cd7f2b4aaa565d
2005-10-29 18:15 Seth, Rohit            o [PATCH] mm: set per-cpu-pages lower threshold to zero
                                          [main] e46a5e28c201f703c18b47b108bfddec44f897c4
2005-12-04 13:55 Nick Piggin            o [PATCH] Fix up per-cpu page batch sizes
                                          [main] 0ceaacc9785fedc500e19b024d606a82a23f5372
2005-12-15 09:18 Al Viro                o [PATCH] missing prototype (mm/page_alloc.c)
                                          [main] 78d9955bb06493e7bd78e43dfdc17fb5f1dc59b6
{% endhighlight %}

![](/assets/PDB/RPI/RPI000938.png)

{% highlight bash %}
git format-patch -1 ba56e91c940146e99ac694c4c7cd7f2b4aaa565d
vi 0001-PATCH-mm-page_alloc-increase-size-of-per-cpu-pages.patch
{% endhighlight %}

该补丁修改了 PCP batchsize 的算法.

![](/assets/PDB/RPI/RPI000939.png)

{% highlight bash %}
git format-patch -1 e46a5e28c201f703c18b47b108bfddec44f897c4
vi 0001-PATCH-mm-set-per-cpu-pages-lower-threshold-to-zero.patch
{% endhighlight %}

该补丁在初始化过程中，将 PCP 分配器的 Hot 链表的 low 设置为 0, 将 Cold 页链表
的 batch 设置为 "max(1UL, batch/2)".

![](/assets/PDB/RPI/RPI000940.png)

{% highlight bash %}
git format-patch -1 0ceaacc9785fedc500e19b024d606a82a23f5372
vi 0001-PATCH-Fix-up-per-cpu-page-batch-sizes.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000941.png)

该补丁重新修改了 PCP 内存分配器 batchsize 的算法.

{% highlight bash %}
git format-patch -1 78d9955bb06493e7bd78e43dfdc17fb5f1dc59b6
vi 0001-PATCH-missing-prototype-mm-page_alloc.c.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000942.png)

该补丁修改了 setup_per_cpu_pageset() 函数定义. 更多补丁使用请查看下面文档:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### PCP 历史时间轴

![](/assets/PDB/RPI/RPI000943.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

#### PCP API

###### alloc_page

{% highlight bash %}
#define alloc_page(gfp_mask) alloc_pages(gfp_mask, 0)
  作用: 从 PCP 内存管理器中分配一个物理页.
{% endhighlight %}

###### free_cold_page

{% highlight bash %}
void fastcall free_cold_page(struct page *page)
  作用: 释放一个物理页到 PCP 内存管理指定 ZONE 分区的冷页链表上.
{% endhighlight %}

###### free_hot_cold_page

{% highlight bash %}
static void fastcall free_hot_cold_page(struct page *page, int cold)
  作用: 释放一个物理页到 PCP 的冷热页链表上.
{% endhighlight %}

###### free_hot_page

{% highlight bash %}
void fastcall free_hot_page(struct page *page)
  作用: 释放一个物理页到 PCP 内存管理器指定 ZONE 分区上热页链表上.
{% endhighlight %}

###### \_\_free_page

{% highlight bash %}
#define __free_page(page) __free_pages((page), 0)
  作用: 释放一个物理页到 PCP 内存分配器里.
{% endhighlight %}

###### free_page

{% highlight bash %}
#define free_page(addr) free_pages((addr),0)
  作用: 通过一个物理页的对应的虚拟地址，释放一个物理页到 PCP 内存分配器里.
{% endhighlight %}

###### free_zone_pagesets

{% highlight bash %}
static inline void free_zone_pagesets(int cpu)
  作用: PCP 分配器与指定 CPU 接触绑定，并释放 PCP 占用的资源.
{% endhighlight %}

###### setup_pageset

{% highlight bash %}
inline void setup_pageset(struct per_cpu_pageset *p, unsigned long batch)
  作用: 初始化指定 ZONE 分区上的 PCP 数据.
{% endhighlight %}

###### setup_per_cpu_pageset

{% highlight bash %}
void __init setup_per_cpu_pageset(void)
  作用: 设置每个 CPU 所使用的 PCP 分配器.
{% endhighlight %}

###### zone_batchsize

{% highlight bash %}
static int __devinit zone_batchsize(struct zone *zone)
  作用: PCP 内存管理器计算指定分区上 PCP batchsize.
{% endhighlight %}

###### zone_pcp_init

{% highlight bash %}
static __devinit void zone_pcp_init(struct zone *zone)
  作用: PCP 内存分配器初始化指定 ZONE 分区上的 PCP.
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

#### PCP 进阶研究

> - [用户空间实现一个 PCP 内存分配器](/blog/Memory-Userspace/#G)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

#### PCP 内存分配器调试

> - [/proc/zoneinfo](#E0000)
>
> - [BiscuitOS PCP 内存分配器调试](#C0004)

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

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
