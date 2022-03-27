---
layout: post
title:  "Bootmem 内存分配器"
date:   2020-05-07 09:35:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [Bootmem 分配器原理](#A)
>
> - [Bootmem 分配器使用](#B)
>
> - [Bootmem 分配器实践](#C)
>
> - [Bootmem 源码分析](#D)
>
> - [Bootmem 分配器调试](#E)
>
> - [Bootmem 分配进阶研究](#F)
>
>   - [Bootmem 分配器在 NUMA NODE 中的分配优先级](#F01)
>
>   - [Bootmem 内存分配器分配的粒度研究](#F02) 
>
>   - [Bootmem 内存分配器分配策略研究](#F05)
>
>   - [Bootmem 内存分配器回收策略研究](#F06)
>
>   - [Bootmem 内存分配器与 CMDLINE 研究](#F03)
>
>   - [Bootmem Allocator on ARM Architecture](#DX0)
>
>   - [Bootmem Allocator on i386 Architecture](#DX1)
>
>   - [Bootmem Allocator on X64 Architecutre](#DX2)
>
> - [Bootmem 时间轴](#G)
>
> - [Bootmem 历史补丁](#H)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### Bootmem 分配器原理

![](/assets/PDB/RPI/RPI000775.png)

Bootmem 分配器是 Linux boot-time 阶段管理物理内存，并提供物理内存分配和回收的分配器。其作为内核初始化过程中第一个真正意义上的内存分配器，为内核早期的初始化活动提供了物理内存的分配和回收，以及为 Buddy 分配器的创建提供了基础，Bootmem 分配器将自己管理的物理内存移交给 Buddy 分配器之后，其使命已经完成，内核正式启用 Buddy 分配器管理系统物理内存。

Bootmem 分配器的实现不是很复杂，在 boot-time 阶段，不同架构的内核通过各种机制获得物理内存布局的信息，内核基于这些信息构建 Bootmem 内存分配器，譬如 I386/x64 架构在 boot-time 阶段，E820 内存管理器从 BIOS 获得系统内存布局信息之后，将 max_low_pfn 之前的所有低端物理内存让 Bootmem 分配器管理; 在 ARM 架构中，内核通过 ATAG 机制从 uboot 中获得物理内存信息，内核将这些信息传递给 Bootmem 分配器进行管理。因此从不同的架构可以看出，内核都是将一部分而不是全部物理内存传递给 Bootmem 分配器进行管理。

Bootmem 分配器在获得这些物理内存信息之后，将物理内存按 PAGE_SIZE 的大小划分成不同的物理内存区域，每个物理内存区域用一个 bit 进行表示，以此建立一个 bitmap。Bootmem 分配器以此 bitmap 为基础管理物理内存区域，如果一块物理内存区域已经预留或分配了，那么 Bootmem 分配器将这块物理内存区域对应的 bit 置位; 反之如果一块物理内存区域可以使用或分配，那么 Bootmem 分配器将这块物理内存区域对应的 bit 清零. Bootmem 分配器就是利用 Bitmap 机制完成了复杂的分配与回收功能。具体过程可以查看:

> [Bootmem 分配器源码分析](#D)

Bootmem 分配器提供了一套完整的 API 用于系统初始化阶段物理内存的分配与回收。Bootmem 分配器能够实现不同粒度的内存分配，小到 1 个字节大到多个物理页，因此在 boot-time 阶段，各子系统可以根据需要调用不同接口分配内存，对于 Bootmem 分配器提供的接口的使用可以参考:

> [Bootmem 分配器使用](#B)

![](/assets/PDB/HK/HK000515.png)

在支持 NUMA 或者 UMA 的架构中，每个 NODE 上都维护着一个 struct bootmem_data 的数据结构，该结构用于描述 Bootmem 分配器管理的一段物理内存区域，其中 node_min_pfn 表示这段物理内存区域的起始物理页帧，node_low_pfn 则表示这段物理内存区域的最大低端物理内存页帧号，node_bootmem_map 则指向 bitmap。Bootmem 分配器正式通过该数据结构管理物理内存区域，在多 NUMA NODE 的架构中，每个 NODE 上都会维护一个 struct bootmem_data 的数据结构，因此 Bootmem 分配器在不同的架构中，提供了一些 NODE 优先级策略，以便获得所需的物理内存。具体可以参考:

> [Bootmem 分配器在 NUMA NODE 中的分配优先级](#F01)

![](/assets/PDB/RPI/RPI000777.png)

Bootmem 分配器为了实现不同粒度的分配与回收，因此也提供了不同的分配策略与回收策略，以保证功能的正确性。Bootmem 分配器在分配的时候, 首先在 bitmap 中查找上次一次分配之后可用的 bit，因此这就提形成了递增式分配，分配器一直从比上一次分配更高的地址进行分配。当 Bootmem 分配器分配到物理内存区域的末尾之后，Bootmem 分配器重新调整方向，从物理内存低地址开始查找可用的 bit，并分配对应的物理内存区域. 分配器由于提供不同的粒度的分配，但 bitmap 是按 PAGE_SIZE 进行管理，因此 Bootmem 分配在没有分配完一个物理页之前，该页对应的 bit 一致清零，直到这个物理页全部分配完或者下一次分配从这个物理页中的某部分分配横跨到后面的物理页，这时对应的 bit 才会被清零，具体过程可以查看:

> [> Bootmem 内存分配器分配的粒度研究](#F02)
>
> [> Bootmem 内存分配器分配策略研究](#F05)
>
> [> Bootmem 内存分配器回收策略研究](#F06)

Bootmem 内存分配器作为早期版本内核 boot-time 阶段的内存分配器被使用，但由于自身一些问题，比如随着管理物理内存变得，Bootmem 分配器 bitmap 占用的物理内存越来越大等问题，最终被 MEMBLOCK 分配器取代. 虽然被取代，但其在 Linux 发展历史上的作用不可忽视，并且其简单有效的内存管理策略也值得开发者在特定需求的内存分配器开发提供宝贵的经验。以下是各个架构放弃使用 Bootmem 分配的版本信息:

* ARM 架构在 2.6.36 之前默认使用 Bootmem 分配器
* I386 架构在 2.6.37 之前默认使用 Bootmem 分配器
* X86 架构在 2.6.37 之前默认使用 Bootmem 分配器
* 主线内核在 4.20 之后将 Bootmem 分配器源码从内核源码树中移除

想了解更多 Bootmem 分配器的历史以及历史补丁，请查看:

> [> Bootmem 时间轴](#G)
>
> [> Bootmem 历史补丁](#H)


---------------------------------

###### Bootmem 的优点

Bootmem 结构简单，使用 bitmap 对内存进行简单的管理，在 Linux 启动的早期确实不需要很复杂的内存管理器，只要完成简单的物理内存分配和预留就可以。

###### Bootmem 的缺点

Bootmem 的缺点就是会对所有的物理内存在 bitmap 中建立对应的 bit，这样在超级大物理内存的平台上将要花费很多时间进行创建，并且创建之后并不使用 bitmap 中的大部分。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### Bootmem 分配器使用

> - [分配一段物理内存 (Boot stage)](#B000)
>
> - [预留一段物理内存](#B001)
>
> - [从低端内存分配物理内存](#B002)
>
> - [从高端内存分配物理内存](#B003)
>
> - [从指定物理内存区域内分配物理内存](#B004)
>
> - [从指定的 NUMA NODE 上分配物理内存](#B005)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------

<span id="B000"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 分配一段物理内存 (Boot stage)

Bootmem 内存分配器在 Boot stage 阶段提供了丰富的接口用于不同场景下的物理内存分配，其支持从低端物理内存分配物理内存，也可以从高端内存分配物理内存，也可以从指定范围内分配物理内存，开发者可以参考如下接口进行内核 Boot Stage 阶段的内存分配:

> [\_\_alloc_bootmem](#D30017)
>
> [alloc_bootmem](#D30018)
>
> [\_\_alloc_bootmem_low](#D30027)
>
> [alloc_bootmem_low](#D30028)
>
> [alloc_bootmem_low_pages](#D30029)
>
> [alloc_bootmem_low_pages_node](#D30030)
>
> [\_\_alloc_bootmem_low_node](#D30043)
>
> [\_\_alloc_bootmem_node](#D30023)
>
> [\_\_alloc_bootmem_node_high](#D30041)
>
> [\_\_alloc_bootmem_node_nopanic](#D30042)
>
> [alloc_bootmem_node](#D30024)
>
> [alloc_bootmem_pages](#D30020)
>
> [alloc_bootmem_pages_node](#D30025)
>
> [alloc_bootmem_pages_node_nopanic](#D30026)
>
> [alloc_bootmem_pages_nopanic](#D30021)
>
> [\_\_alloc_bootmem_nopanic](#D30040)
>
> [alloc_bootmem_nopanic](#D30019)

同理 Bootmem 内存分配器也提供了丰富的接口用于释放已经分配的物理内存，开发者可以参考如下接口进行物理内存的释放:

> [free_bootmem](#D30038)
>
> [free_bootmem_node](#D30009)

Bootmem 内存分配器分配、使用，以及释放一段物理内存可以参考如下代码: (该代码试用于 Boot Stage 阶段，这里 Boot Stage 阶段特指 start_kernel()->mm_init() 函数之前的阶段)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
	char *buffer = NULL;
	int size = 0x20;

	/* Alloc memory from Bootmem */
	buffer = (char *)alloc_bootmem(size);
	if (!buffer) {
		printk("ERROR: Alloc memory failed.\n");
		return -ENOMEM;
	}

	/* Use */
	sprintf(buffer, "BiscuitOS-%s", "Buddy");
	printk("=> %s\n", buffer);

	/* Free memory */
	free_bootmem(__pa(buffer), size);

	return 0;
}
{% endhighlight %}

在上面例子中，调用 alloc_bootmem() 函数从 Bootmem 分配器中分配长度为 0x20 的物理内存，并返回物理内存对应的虚拟地址，然后使用这段内存存储一段字符串，并使用 printk 打印这段字符串。使用完毕之后，函数调用 free_bootmem() 将这段物理内存释放回 Bootmem 分配器。如果将这个函数放到 start_kernel() 函数里面 mm_init() 的前面进行调用，那么运行结果如下:

![](/assets/PDB/HK/HK000556.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------

<span id="B001"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 预留一段物理内存

内核在初始化过程中，需要将一些特定的物理内存区域进行预留，以便其他分配器无法使用这些物理内存，这些预留的物理内存经常用于存储系统重要数据和特定功能，因此 Bootmem 分配器作为内核初始化阶段的前端内存分配器，必须提供提供物理内存预留功能。Bootmem 分配器提供了以下接口用于物理内存的预留:

> [reserve_bootmem](#D30039)
>
> [reserve_bootmem_node](#D30010)

开发者可以参考这段代码在 Boot Stage 阶段进行物理内存的预留: (该代码试用于 Boot Stage 阶段，这里 Boot Stage 阶段特指 start_kernel()->mm_init() 函数之前的阶段)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
	unsigned long Reserve_addr = 0x6000000;
	unsigned long Reserve_size = 0x1000000;

	/* Reserved Physical Memory */
	reserve_bootmem(Reserve_addr, Reserve_size, BOOTMEM_DEFAULT);
}
{% endhighlight %}

上面的例子中，函数调用 reserve_bootmem() 函数将 0x6000000 - 0x6100000 物理内存区域进行预留，Bootmem 标记为预留之后，这段物理内存将不会转移给 Buddy 内存分配器进行管理，因此系统运行过程中一直保存预留状态. 如果将这个函数放到 start_kernel() 函数里面 mm_init() 的前面进行调用，那么可以查看添加前和添加后系统可用物理内存统计信息:

![](/assets/PDB/HK/HK000557.png)

上图为未添加预留前，可用物理内存为 256784 KiB.

![](/assets/PDB/HK/HK000558.png)

上图是运行了预留代码的，可用物理内存为 240400 KiB, 相比预留前，可用内存减少了 16384 KiB，即减少了 0x1000000. 因此使用 reserve_bootmem() 预留的物理内存区域不会转移给 Buddy 内存分配器，会一致在系统中预留。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------

<span id="B002"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 从低端内存分配物理内存

内核在不同的体系结构中，将物理内存划分成不同的区域。例如将内核空间虚拟地址和物理地址直接映射的区域称为低端内存和 DMA 内存，而将高于直接映射物理内存区域称为高端内存区域，高端内存区域的物理地址和虚拟地址通过动态映射建立联系。有些功能模块必须从低端内存区分配物理内存，因此 Bootmem 分配器提供了一套接口用于从低端内存分配指定数量的物理内存，如下接口:

> [\_\_alloc_bootmem_low](#D30027)
>
> [alloc_bootmem_low](#D30028)
>
> [alloc_bootmem_low_pages](#D30029)
>
> [alloc_bootmem_low_pages_node](#D30030)
>
> [\_\_alloc_bootmem_low_node](#D30043)

开发者可以参考这段代码在 Boot Stage 阶段从低端内存区分配物理内存: (该代码试用于 Boot Stage 阶段，这里 Boot Stage 阶段特指 start_kernel()->mm_init() 函数之前的阶段)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
	char *buffer;
	int size = 0x20;

	/* Alloc memory from LOW-Memory */
	buffer = (char *)alloc_bootmem_low(size);
	if (!buffer) {
		printk("ERROR: Can't alloc memory from LOW-Memory.\n");
		return -ENOMEM;
	}

	/* Use */
	sprintf(buffer, "BiscuitOS-%s", "Buddy");
	printk("=> %s\n", buffer);
	printk("Phys: %#lx -- Highmem: %#lx\n", __pa(buffer), __pa(high_memory));

	/* Free memory */
	free_bootmem(__pa(buffer), size);

	return 0;
}
{% endhighlight %}

在这个例子中，函数首先调用 alloc_bootmem_low() 函数从低端内存区分配了长度为 size 的物理内存，并返回对应的虚拟地址。然后将一个字符串存储在这段内存区域，接着使用 printk 打印这段字符串，以及打印了这段内存区域的物理地址以及高端内存的起始物理地址。如果将这个函数放到 start_kernel() 函数里面 mm_init() 的前面进行调用
，那么运行结果如下:

![](/assets/PDB/HK/HK000559.png)

从运行的结果可以看出，函数已经成功分配内存，通过打印的物理地址可以看出，分配的物理内存来自低端内存区域, 此环境中高端内存区域的起始物理地址是 0x10000000.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------

<span id="B003"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 从高端内存分配物理内存

内核在不同的体系结构中，将物理内存划分成不同的区域。例如将内核空间虚拟地址和物理地址直接映射的区域称为低端内存和 DMA 内存，而将高于直接映射物理内存区域称为高端内存区域，高端内存区域的物理地址和虚拟地址通过动态映射建立联系。有些功能模块必须从高端内存区分配物理内存，因此 Bootmem 分配器提供了一套接口用于从高端内存分配指定数量的物理内存，如下接口:

> [\_\_alloc_bootmem_node_high](#D30041)

开发者可以参考这段代码在 Boot Stage 阶段从高端内存区分配物理内存: (该代码试用于 Boot Stage 阶段，这里 Boot Stage 阶段特指 start_kernel()->mm_init() 函数之前的阶
段)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer;
        int size = 0x20;

        /* Alloc memory from High-Memory */
        buffer = (char *)__alloc_bootmem_node_high(NODE_DATA(0), size, 
					PAGE_SIZE, __pa(MAX_DMA_ADDRESS));
        if (!buffer) {
                printk("ERROR: Can't alloc memory from High-Memory.\n");
                return -ENOMEM;
        }

        /* Use */
        sprintf(buffer, "BiscuitOS-%s", "Buddy");
        printk("=> %s\n", buffer);
        printk("Phys: %#lx -- Highmem: %#lx\n", __pa(buffer), __pa(high_memory));

        /* Free memory */
        free_bootmem_node(NODE_DATA(0), __pa(buffer), size);

        return 0;
}
{% endhighlight %}

在这个例子中，首先系统要支持 CONFIG_HIGHMEM 宏，否则函数还是只会从 Low-Memory 中分配内存。函数首先调用 \_\_alloc_bootmem_node_high() 函数从高端内存区分配了长度为 size 的物理内存，并返回对应的虚拟地址。然后将一个字符串存储在这段内存区域，接着使用 printk 打印这段字符串，以及打印了这段内存区域的物理地址以及高端内存的起始物理地址。如果将这个函数放到 start_kernel() 函数里面 mm_init() 的前面进行调用
，那么运行结果如下:

![](/assets/PDB/HK/HK000559.png)

从运行的结果可以看出，函数已经成功分配内存，通过打印的物理地址可以看出，分配的物
理内存来自低端内存区域, 此环境中高端内存区域的起始物理地址是 0x10000000.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------

<span id="B004"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 从指定物理内存区域分配物理内存

内核有的功能模块限定只能从指定的物理内存范围分配物理内存，因此 Bootmem 分配器提供了一套接口用于指定物理内存范围内分配指定数量的物理内存，如下接口:

> [\_\_alloc_bootmem](#D30017)
>
> [\_\_alloc_bootmem_node](#D30023)
>
> [\_\_alloc_bootmem_node_nopanic](#D30042)
>
> [\_\_alloc_bootmem_nopanic](#D30040)

以上函数都有一个共同的特点，这些函数都提供了 goal 参数，该参数用于指明 Bootmem 分配器从该地址开始查找可用物理内存，因此可以使用这些函数从指定的内存区域分配物理内存。开发者可以参考这段代码在 Boot Stage 阶段从指定内存区域上分配物理内存: (该代码试用于 Boot Stage 阶段，这里 Boot Stage 阶段特指 start_kernel()->mm_init() 函数之前的阶段)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer;
        int size = 0x20;
	unsigned long range_start = 0x6200000;

        /* Alloc memory from Special memory area */
        buffer = (char *)__alloc_bootmem(size, PAGE_SIZE, range_start); 
        if (!buffer) {
                printk("ERROR: Can't alloc memory from %#lx.\n", range_start);
                return -ENOMEM;
        }

        /* Use */
        sprintf(buffer, "BiscuitOS-%s", "Buddy");
        printk("=> %s\n", buffer);
        printk("Phys: %#lx on from %#lx\n", __pa(buffer), range_start); 

        /* Free memory */
        free_bootmem(__pa(buffer), size);

        return 0;
}
{% endhighlight %}

在这里例子中，调用 \_\_alloc_bootmem() 函数从 range_start 指向的物理地址开始查找物理内存，直到找到合适的物理内存为止，否则返回 NULL. 分配成功之后，在这段内存上存储一段字符串，接着打印这段字符串、内存对应的物理地址，以及起始物理地址，以此确认是否从指定的物理地址开始分配。使用完毕之后调用 free_bootmem() 函数进行释放。如果将这个函数放到 start_kernel() 函数里面 mm_init() 的前面进行调用，那么运行结果如下:

![](/assets/PDB/HK/HK000560.png)

从运行结果来看，函数已经从指定的物理地址开始查找可用物理内存，因此与预期相符.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------

<span id="B005"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 从指定的 NUMA NODE 上分配物理内存

在支持 NUMA 的架构中，系统中一般包含不止一个 NODE，内核有的功能模块限定只能从指定的 NODE 上分配物理内存，因此 Bootmem 分配器提供了一套接口用于从指定 NODE 上分配物理内存，如下接口:

> [alloc_bootmem_low_pages_node](#D30030)
>
> [\_\_alloc_bootmem_low_node](#D30043)
>
> [\_\_alloc_bootmem_node](#D30023)
>
> [\_\_alloc_bootmem_node_high](#D30041)
>
> [\_\_alloc_bootmem_node_nopanic](#D30042)
>
> [alloc_bootmem_node](#D30024)
>
> [alloc_bootmem_pages_node](#D30025)
>
> [alloc_bootmem_pages_node_nopanic](#D30026)

以上函数都有一个共同的特点，这些函数比一般的 Bootmem 分配器函数多了 pg_data_t 参数，该参数用于指定 NUMA NODE 的信息，因此可以使用这些函数从指定的内存区域分配物理，因此开发者可以使用这些函数从指定的 NODE 上分配内存。开发者可以参考这段代码在 Boot Stage 阶段从指定内存区域上分配物理内存: (该代码试用于 Boot Stage 阶段，这里 Boot Stage 阶段特指 start_kernel()->mm_init() 函数之前的阶段)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer;
        int size = 0x20;

        /* Alloc memory from Specify NUMA NODE */
        buffer = (char *)alloc_bootmem_node(NODE_DATA(0), size); 
        if (!buffer) {
                printk("ERROR: Can't alloc memory from Specify NUMA NODE.\n");
                return -ENOMEM;
        }

        /* Use */
        sprintf(buffer, "BiscuitOS-%s", "Buddy");
        printk("=> %s\n", buffer);

        /* Free memory */
        free_bootmem_node(NODE_DATA(0), __pa(buffer), size);
 
        return 0;
}
{% endhighlight %}

在这个例子中，调用 alloc_bootmem_node() 函数从指定的 NUMA NODE 中分配物理内存，其中 NODE_DATA() 函数可以提供 pg_data_t 变量。分配成功之后，函数将字符串存储到这段内存上，并使用 printk 打印字符串。使用完毕之后，调用 free_bootmem_node() 函数释放这段内存. 如果将这个函数放到 start_kernel() 函数里面 mm_init() 的前面进行调用，那么运行结果如下:

![](/assets/PDB/HK/HK000561.png)

从运行的结果可以看出，函数已经成功分配内存，通过打印的物理地址可以看出，分配的物
理内存来自低端内存区域, 此环境中高端内存区域的起始物理地址是 0x10000000.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### Bootmem 分配器实践

Bootmem 分配器的实践分作以下几种类型的实践，首先是在特定内核版本的实践，其次是 Bootmem 历史项目的实践，开发者可以根据需要进行选择:

> [> 标准 Bootmem 内存分配器实践](#C1)
>
> [> Bootmem 历史项目实践](#C2)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

#### 标准 Bootmem 内存分配器实践

> - [实践准备](#C1000)
>
> - [实践部署](#C1001)
>
> - [实践执行](#C1002)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C1000">实践准备</span>

本实践支持 "i386/x86/ARM" 架构，开发者可以根据需要自行选择架构进行实践。另外对于内核版本的选择，开发者请参考如下信息:

* ARM 架构在 2.6.36 之前默认使用 Bootmem 分配器
* I386 架构在 2.6.37 之前默认使用 Bootmem 分配器
* X86 架构在 2.6.37 之前默认使用 Bootmem 分配器

因此开发者在选择架构之后，内核版本应该选择小于默认使用 Bootmem 分配器的版本。本节以 "i386 架构 2.6.36 版本内核" 进行讲解. 开发者首先进行环境的搭建:

> [> BiscuitOS Linux 2.x i386 Usermanual](/blog/Linux-2.x-i386-Usermanual/#header)
>
> [> BiscuitOS Linux 2.x x86_64 Usermanual](/blog/Linux-2.x-x86_64-Usermanual/)
>
> [> BiscuitOS Linux 2.6.10 - 2.6.11 arm32 Usermanual](/blog/Linux-2.6.10-arm32-Usermanual/)
>
> [> BiscuitOS Linux 2.6.12 - 2.6.33 arm32 Usermanual](/blog/Linux-2.6.12-arm32-Usermanual/)
>
> [> BiscuitOS Linux 2.6.34 - 2.6.39 arm32 Usermanual](/blog/Linux-2.6.34-arm32-Usermanual/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C1001">实践部署</span>

在部署完毕开发环境之后，开发者应该确保一下内核宏是关闭的，参考如下:

{% highlight bash %}
cd BiscuitOS/output/linux-2.6.36-i386/linux/linux
make ARCH=i386 menuconfig

  Processor type and features  --->
    [ ] Disable Bootmem code
{% endhighlight %}

开发者一定要确认上面的选项是关闭的，确认修改之后重新编译内核使用。如果涉及修改 CMDLINE，那么开发者可以参考如下:

{% highlight bash %}
BiscuitOS/output/linux-2.6.36-i386/RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000384.png)

如上图，RunBiscuitOS.sh 脚本中 CMDLINE 变量用于存储内核启动时候使用的 CMDLINE
信息，开发者可以将自定义的 CMDLINE 参数写入该变量里，内核启动自动加载作为
系统启动 CMDLINE. 值得注意的是脚本里面的 CMDLINE 变量通过字符串的方式存储
系统使用的 CMDLINE，因此特殊字符需要转换，例如 CMDLINE 参数中包含 "$" 符号的，
需要在特殊符号前面加 "\\" 进行转译.

Bootmem 分配器相关的代码使用请在 start_kernel()->setup_arch() 函数之后，start_kernel()->mm_init() 函数之前进行使用，这个阶段 Bootmem 分配器正常工作.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C1002">实践执行</span>

环境部署完毕之后，开发者可以直接运行 BiscuitOS, 此时 boot-time 阶段使用的就是 Bootmem 分配器. 运行的情况，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-2.6.36-i386/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

#### Bootmem 历史项目实践

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

> [BiscuitOS Linux 5.0 ARM32 环境部署](/blog/Linux-5.0-arm32-Usermanual/)

--------------------------------------------

#### <span id="C0001">实践部署</span>

准备好基础开发环境之后，开发者接下来部署项目所需的开发环境。由于项目
支持多个版本的 Bootmem，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 Bootmem 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](/assets/PDB/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](/assets/PDB/RPI/RPI000778.png)

选择并进入 "[\*]   Bootmem Allocator  --->" 目录。

![](/assets/PDB/RPI/RPI000779.png)

选择 "[\*]   bootmem on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make download
{% endhighlight %}

![](/assets/PDB/RPI/RPI000780.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](/assets/PDB/RPI/RPI000781.png)

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
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

添加完毕之后，使用如下命令更新 DTS:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make kernel
{% endhighlight %}

![](/assets/PDB/RPI/RPI000782.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000783.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make run
{% endhighlight %}

![](/assets/PDB/RPI/RPI000784.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_bootmem-2.6.12.ko"，打印如上信息，那么
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
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12/BiscuitOS_bootmem-2.6.12/Makefile
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

然后先向 BiscuitOS 中插入 "BiscuitOS_bootmem-2.6.12.ko" 模块，然后再插入
"BiscuitOS_bootmem-2.6.12-buddy.ko" 模块。如下:

![](/assets/PDB/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](/assets/PDB/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

由于 bootmem 内存分配器只存在于内存管理子系统的早期，因此只能在内存管理
早期进行测试，Initcall 机制提供了以下函数用于 bootmem 调试:

{% highlight bash %}
bootmem_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，bootmem_initcall_bs() 调用的函数将
在 bootmem 分配器初始化完毕之后自动调用，因此可用此法调试 bootmem。
bootmem 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12/BiscuitOS_bootmem-2.6.12/module/bootmem/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/bootmem/main.o
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### Bootmem 历史补丁

> - [Bootmem Linux 2.6.12](#H-linux-2.6.12)
>
> - [Bootmem Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [Bootmem Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [Bootmem Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [Bootmem Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [Bootmem Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [Bootmem Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [Bootmem Linux 2.6.13](#H-linux-2.6.13)
>
> - [Bootmem Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [Bootmem Linux 2.6.14](#H-linux-2.6.14)
>
> - [Bootmem Linux 2.6.15](#H-linux-2.6.15)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](/assets/PDB/RPI/RPI000785.JPG)

#### Bootmem Linux 2.6.12

Linux 2.6.12 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](/assets/PDB/RPI/RPI000786.JPG)

#### Bootmem Linux 2.6.12.1

Linux 2.6.12.1 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](/assets/PDB/RPI/RPI000787.JPG)

#### Bootmem Linux 2.6.12.2

Linux 2.6.12.2 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.1，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](/assets/PDB/RPI/RPI000788.JPG)

#### Bootmem Linux 2.6.12.3

Linux 2.6.12.3 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.2，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](/assets/PDB/RPI/RPI000789.JPG)

#### Bootmem Linux 2.6.12.4

Linux 2.6.12.4 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.3，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](/assets/PDB/RPI/RPI000790.JPG)

#### Bootmem Linux 2.6.12.5

Linux 2.6.12.5 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.4，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](/assets/PDB/RPI/RPI000791.JPG)

#### Bootmem Linux 2.6.12.6

Linux 2.6.12.6 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.5，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](/assets/PDB/RPI/RPI000792.JPG)

#### Bootmem Linux 2.6.13

Linux 2.6.13 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.6，bootmem 内存分配器提交了四个补丁。如下:

{% highlight bash %}
tig mm/bootmem.c include/linux/bootmem.h

2005-06-23 00:07 Dave Hansen    o [PATCH] sparsemem base: simple NUMA remap space allocator
                                  [main] 6f167ec721108c9282d54424516a12c805e3c306 - commit 4 of 5
2005-06-23 00:07 Andy Whitcroft o [PATCH] sparsemem memory model
                                  [main] d41dee369bff3b9dcb6328d4d822926c28cc2594 - commit 3 of 5
2005-06-25 14:58 Vivek Goyal    o [PATCH] kdump: Retrieve saved max pfn
                                  [main] 92aa63a5a1bf2e7b0c79e6716d24b76dbbdcf951 - commit 2 of 5
2005-06-25 14:59 Nick Wilson    o [PATCH] Use ALIGN to remove duplicate code
                                  [main] 8c0e33c133021ee241e9d51255b9fb18eb34ef0e - commit 1 of 5
{% endhighlight %}

![](/assets/PDB/RPI/RPI000796.png)

{% highlight bash %}
git format-patch -1 6f167ec721108c9282d54424516a12
vi 0001-PATCH-sparsemem-base-simple-NUMA-remap-space-allocat.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000797.png)

对应 bootmem 内存分配器来说，这个补丁添加了在不支持 CONFIG_HAVE_ARCH_ALLOC_REMAP
的情况下，提供 "alloc_remap()" 函数的实现.

{% highlight bash %}
git format-patch -1 d41dee369bff3b9dcb6328d4d822926c28cc259
vi 0001-PATCH-sparsemem-memory-model.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000809.png)

该 PATCH 添加了 bootmem 内存分配器支持早期的 PFN 和 page 之间的转换。

{% highlight bash %}
git format-patch -1 92aa63a5a1bf2e7b0c79e6716d24b76dbbdcf951
vi 0001-PATCH-kdump-Retrieve-saved-max-pfn.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000798.png)

该补丁通过描述可以知道在 i386 体系结构中，如果启用 CONFIG_CRASH_DUMP 宏，
那么会在 bootmem.c 中定义一个变量 "saved_max_pfn", 这个全局变量用于存储
系统中最大物理帧的值，以便当 max_pfn 变量被修改之后，"saved_max_pfn" 还可以
基础标志系统使用的最大物理帧，这样将有效确保用户对物理内存的操作不会超过
"saved_max_pfn" 之外.

{% highlight bash %}
git format-patch -1 8c0e33c133021ee241e9d51255b9fb18eb34ef0e
vi 0001-PATCH-Use-ALIGN-to-remove-duplicate-code.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000799.png)

这个补丁只是将 bootmem 内存分配中对齐操作全部替换成了 ALIGN() 函数。
更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](/assets/PDB/RPI/RPI000793.JPG)

#### Bootmem Linux 2.6.13.1

Linux 2.6.13.1 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.13，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](/assets/PDB/RPI/RPI000794.JPG)

#### Bootmem Linux 2.6.14

Linux 2.6.14 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
alloc_bootmem_limit()
__alloc_bootmem_limit()
alloc_bootmem_low_limit()
__alloc_bootmem_limit()
alloc_bootmem_node_limit()
alloc_bootmem_pages_node_limit()
alloc_bootmem_low_pages_node_limit()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.13.1，bootmem 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/bootmem.c include/linux/bootmem.h

2005-09-12 18:49 Andi Kleen     o [PATCH] x86-64: Reverse order of bootmem lists
                                  [main] 5d3d0f7704ed0bc7eaca0501eeae3e5da1ea6c87 - commit 3 of 8
2005-09-30 12:38 Linus Torvalds o Revert "x86-64: Reverse order of bootmem lists"
                                  [main] 6e3254c4e2927c117044a02acf5f5b56e1373053 - commit 2 of 8
2005-10-19 15:52 Yasunori Goto  o [PATCH] swiotlb: make sure initial DMA allocations really are in DMA memory
                                  [main] 281dd25cdc0d6903929b79183816d151ea626341 - commit 1 of 8
{% endhighlight %}

![](/assets/PDB/RPI/RPI000800.png)

{% highlight bash %}
git format-patch -1 5d3d0f7704ed0bc7eaca0501eeae3e5da1ea6c87
vi 0001-PATCH-x86-64-Reverse-order-of-bootmem-lists.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000801.png)

补丁修改了 bootmem 分配器的分配顺序，从 node 0 开始而不是从最后一个 node 开始。

{% highlight bash %}
git format-patch -1 6e3254c4e2927c117044a02acf5f5b56e1373053
vi 0001-Revert-x86-64-Reverse-order-of-bootmem-lists.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000802.png)

{% highlight bash %}
git format-patch -1 281dd25cdc0d6903929b79183816d151ea626341
vi 0001-PATCH-swiotlb-make-sure-initial-DMA-allocations-real.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000803.png)

该补丁中，由于对 bootmem 分配的内存进行了限制，因此产生了一套待限制的
分配函数，以便在使用 bootmem 分配时能够限制范围。bootmem 提供了这些函数
的具体实现过程。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](/assets/PDB/RPI/RPI000795.JPG)

#### Bootmem Linux 2.6.15

Linux 2.6.15 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](/assets/PDB/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
alloc_bootmem_limit()
__alloc_bootmem_limit()
alloc_bootmem_low_limit()
__alloc_bootmem_limit()
alloc_bootmem_node_limit()
alloc_bootmem_pages_node_limit()
alloc_bootmem_low_pages_node_limit()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](/assets/PDB/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.14，bootmem 内存分配器增加了两个补丁，如下:

{% highlight bash %}
tig mm/bootmem.c include/linux/bootmem.h

2005-12-12 00:37 Haren Myneni   o [PATCH] fix in __alloc_bootmem_core() when there is no free page in first node's memory
                                  [main] 66d43e98ea6ff291cd4e524386bfb99105feb180 - commit 1 of 10
2005-10-29 18:16 Nick Piggin    o [PATCH] core remove PageReserved
                                  [main] b5810039a54e5babf428e9a1e89fc1940fabff11 - commit 2 of 10
{% endhighlight %}

![](/assets/PDB/RPI/RPI000805.png)

{% highlight bash %}
git format-patch -1 b5810039a54e5babf428e9a1e89fc1940fabff11
vi 0001-PATCH-core-remove-PageReserved.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000806.png)

补丁在 free_all_bootmem_core() 函数中添加了将 struct page 的引用计数设置为 0.
该函数的主要用途就是将 bootmem 管理的可用物理页传递给 buddy 内存分配器。

{% highlight bash %}
git format-patch -1 66d43e98ea6ff291cd4e524386bfb99105feb180
vi 0001-PATCH-fix-in-__alloc_bootmem_core-when-there-is-no-f.patch
{% endhighlight %}

![](/assets/PDB/RPI/RPI000807.png)

这个补丁用于解决当第一个 node 没有可用的物理内存之后，bootmem 内存分配器
应该去下一个 node 中查找. 在 \_\_alloc_bootmem_core() 函数中，restart_scan
继续查找可用的物理内存，当当前 node 没有可用的物理内存之后，即条件满足就
跳出循环，继续去下一个 node 中查找。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### Bootmem 历史时间轴

![](/assets/PDB/RPI/RPI000808.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

#### Bootmem 分配器源码分析

> - [Bootmem 分配器架构](#D)
>
>   - [Bootmem Allocator on ARM Architecture](#DX0)
>
>   - [Bootmem Allocator on i386 Architecture](#DX1)
>
>   - [Bootmem Allocator on X86 Architecture](#DX2)
>
> - [Bootmem 分配器重要数据](#D101)
>
>   - [bdata_list](#D100)
>
> - [Bootmem 分配器数据结构](#D201)
>
>   - [bootmem_data_t/struct bootmem_data](#D200)
>
> - [Bootmem 分配器 API](#D30044)
>
>   - [align_idx](#D30011)
>
>   - [align_off](#D30013)
>
>   - [alloc_arch_preferred_bootmem](#D30014)
>
>   - [\_\_alloc_bootmem](#D30017)
>
>   - [\_\_\_alloc_bootmem](#D30016)
>
>   - [alloc_bootmem](#D30018)
>
>   - [alloc_bootmem_core](#D30012)
>
>   - [\_\_alloc_bootmem_low](#D30027)
>
>   - [alloc_bootmem_low](#D30028)
>
>   - [alloc_bootmem_low_pages](#D30029)
>
>   - [alloc_bootmem_low_pages_node](#D30030)
>
>   - [\_\_alloc_bootmem_low_node](#D30043)
>
>   - [\_\_alloc_bootmem_node](#D30023)
>
>   - [\_\_\_alloc_bootmem_node](#D30022)
>
>   - [\_\_alloc_bootmem_node_high](#D30041)
>
>   - [\_\_alloc_bootmem_node_nopanic](#D30042)
>
>   - [alloc_bootmem_node](#D30024)
>
>   - [alloc_bootmem_pages](#D30020)
>
>   - [alloc_bootmem_pages_node](#D30025)
>
>   - [alloc_bootmem_pages_node_nopanic](#D30026)
>
>   - [alloc_bootmem_pages_nopanic](#D30021)
>
>   - [\_\__alloc_bootmem_nopanic](#D30015)
>
>   - [\_\_alloc_bootmem_nopanic](#D30040)
>
>   - [alloc_bootmem_nopanic](#D30019)
>
>   - [bootmap_bootmap_pages](#D30001)
>
>   - [bootmap_bytes](#D30000)
>
>   - [bootmem_debug_setup](#D30032)
>
>   - [\_\_free](#D30005)
>
>   - [free_all_bootmem](#D30036)
>
>   - [free_all_bootmem_core](#D30034)
>
>   - [free_all_bootmem_node](#D30035)
>
>   - [free_bootmem](#D30038)
>
>   - [free_bootmem_late](#D30033)
>
>   - [free_bootmem_node](#D30009)
>
>   - [\_\_free_pages_bootmem](#D30032)
>
>   - [init_bootmem_core](#D30003)
>
>   - [init_bootmem_node](#D30004)
>
>   - [link_bootmem](#D30002)
>
>   - [mark_bootmem](#D30037)
>
>   - [mark_bootmem_node](#D30008)
>
>   - [\_\_reserve](#D30007)
>
>   - [reserve_bootmem](#D30039)
>
>   - [reserve_bootmem_node](#D30010)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D3000X">X</span>

![](/assets/PDB/HK/HK000.png)

{% highlight bash %}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="DX2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Z.jpg)

#### Bootmem Allocator on X64 Architecture

Linux 2.6.36 及其以前，x64 体系结构的初始化阶段通过 E820 从 BIOS 中获得内存布局相关的信息，并将使用 E820 内存管理器进行维护，以此为基础构建内核早期的内存信息。i386 架构将这些信息在内核初始化过程中传递给 Bootmem 内存分配器。以此构建内核早期的内存管理器，为 Buddy 内存分配器的构建提供基础，最终 Bootmem 分配器将管理的物理内存全部移交给 Buddy 内存管理器进行维护，Bootmem 分配器完成自己的使命，其代码逻辑如下:

![](/assets/PDB/HK/HK000578.png)

> - [initmem_init](#DX200)
>
>   - [bootmem_bootmap_pages](#D30001)
>
>   - [init_bootmem_node](#D30004)
>
>   - [free_bootmem_with_active_regions](#DX201)
> 
>     - [free_bootmem_node](#D30009)
>
> - [mm_init](#DX204)
>
>   - [mem_init](#DX205)
>
>     - [numa_free_all_bootmem](#DX206)
>
>     - [free_all_bootmem](#D30036)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

<span id="DX200"></span>

![](/assets/PDB/HK/HK000579.png)

X64 架构的 setup_arch() 函数通过调用 initmem_init() 函数用于初始化 Bootmem 内存分配器。函数首先调用 bootmem_bootmap_pages() 函数将一个物理页对应一个 bit 的方式计算出从 0 到 end_pfn 对于的物理内存区域需要多少 bit，以此计算出这些 bit 占用的物理内存页数量。接着函数调用 find_e820_area() 函数从 E820 管理器中找到一块合适的物理内存用于构建 bitmap。如果成功找到，那么函数在 584 行调用 reserve_early() 函数将这段物理内存标记为预留，以防止被其他分配出去。函数接着调用 init_bootmem_node() 函数进行 Bootmem 分配器的构建，此时 Bootmem 分配器将物理内存区域都标记为不可用。此时函数在 588 行调用 e820_register_active_regions() 函数将将 start_pfn 到 end_pfn 的区域标记为可用内存区域，那么函数调用 free_bootmem_with_active_regions() 函数将 E820 激活的可用物理内存区域在 Bootmem 分配器中标记可用，标记结束之后，Bootmem 内存分配器就可以开始分配内存。

> [bootmem_bootmap_pages](#D30001)
>
> [init_bootmem_node](#D30004)

<span id="DX201"></span>

![](/assets/PDB/HK/HK000576.png)

函数在 3622 行调用 for_each_active_range_idx_in_nid() 函数遍历当前系统中可用的物理内存区。在遍历每个内存区域的时候，函数只处理低端物理内存区，对于高端内存区直接选择跳过。函数接着调用 free_bootmem_node() 函数将对于低端内存区域在 Bootmem 分配器中标记为可用。

> [free_bootmem_node](#D30009)

<span id="DX204"></span>

![](/assets/PDB/HK/HK000554.png)

start_kernel 初始化完与体系相关的代码之后，在 mm_init() 函数中准备构建 buddy 内>存分配器，mm_init() 函数调用 mem_init() 函数进行 buddy 内存分配器的创建。

<span id="DX205"></span>

![](/assets/PDB/HK/HK000580.png)

在 X64 架构中，mem_init() 函数在支持 NUMA 的情况下，调用 numa_free_all_bootmem() 函数将 Bootmem 分配器的管理的物理内存移交给 Buddy 内存分配器; 而在不开 NUMA 的架构中，函数直接调用 free_all_bootmem() 将 Bootmem 分配器管理的物理内存移交给 Buddy 分配器进行管理.

> [free_all_bootmem](#D30036)

<span id="DX206"></span>

![](/assets/PDB/HK/HK000581.png)

在支持 NUMA 的情况下，函数通过调用 numa_free_all_bootmem() 函数将 Bootmem 分配器管理的物理内存移交给 Buddy 分配器进行管理。函数在 703 行调用 for_each_online_node() 函数遍历 NUMA 的所有 NODE，每当遍历到一个 NODE，函数调用 free_all_bootmem_node() 函数将 NODE 里对应的物理内存从 Bootmem 分配器移交给 Buddy 内存分配器进行管理。

> [free_all_bootmem_node](#D30035)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="DX1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

#### Bootmem Allocator on I386 Architecture

Linux 2.6.36 及其以前，i386 体系结构的初始化阶段通过 E820 从 BIOS 中获得内存布局相关的信息，并将使用 E820 内存管理器进行维护，以此为基础构建内核早期的内存信息。i386 架构将这些信息在内核初始化过程中传递给 Bootmem 内存分配器。以此构建内核早期的内存管理器，为 Buddy 内存分配器的构建提供基础，最终 Bootmem 分配器将管理的物理内存全部移交给 Buddy 内存管理器进行维护，Bootmem 分配器完成自己的使命，其代码逻辑如下:

![](/assets/PDB/HK/HK000572.png)

> - [initmem_init](#DX100)
>
>   - [setup_bootmem_allocator](#DX101)
>
>     - [setup_node_bootmem](#DX108)
> 
>     - [init_bootmem_node](#DX102)
>
>     - [free_bootmem_with_active_region](#DX103)
>
> - [mm_init](#DX104)
>
>   - [mem_init](#DX105)
>
>     - [free_all_bootmem](#)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

<span id="DX100"></span>

![](/assets/PDB/HK/HK000573.png)

i386 架构的 setup_arch() 函数通过调用 initmem_init() 函数用于初始化 Bootmem 内存分配器。函数在 711 - 726 行计算出了高端内存的起始虚拟地址为 high_memory, 以及系统中包含物理页的个数 num_physpages. 计算完这些数据之后，函数在 735 行调用 setup_bootmem_allocator() 函数进行 Bootmem 分配器的初始化工作.

<span id="DX101"></span>

![](/assets/PDB/HK/HK000574.png)

在支持 Bootmem 分配器的架构中，CONFIG_NO_BOOTMEM 宏是不打开的，因此函数在 783 行调用 bootmem_bootmap_pages() 函数将每个物理页长度的物理内存空间定义为一个 bit，然后计算 max_low_pfn 对应的物理内存空间一共占用了多少个 bit，并且这些 bit 占用的内存为多少个物理页，max_low_pfn 表示低端物理内存总共包含多少个物理页帧. 因此从这里可以看出 Bootmem 分配器在 i386 架构中是不维护高端物理内存的。函数接着在 784 行调用 find_e820_area() 函数从 E820 管理的内存区域中找出一块可用的内存用于存储 Bootmem 的 bitmap。函数接着调用 reserve_early() 函数将 bitmap 占用的内存标记为预留。函数接着在 791 - 793 行打印了内存相关的信息。函数在 796 行调用 for_each_online_node() 函数遍历系统可用的 NODE，在每次遍历过程中计算每个 NODE 的起始物理页帧和结束物理页帧，并且每次遍历过程中，函数只计算每个 NODE 的低端内存的起始物理页帧和终止物理页帧，高端内存选择跳过，接着函数将起始物理页帧和结束物理页帧传递给 setup_node_bootmem() 函数初始化 Bootmem 分配器。Bootmem 分配器初始化完毕之后，函数在 815 行将 after_bootmem 设置为 1.以此告诉系统 Bootmem 分配器可以分配内存。

> [bootmem_bootmap_pages](#D30001)

<span id="DX108"></span>

![](/assets/PDB/HK/HK000575.png)

函数在 762 行调用 init_bootmem_node() 函数将指定的物理内存区域加入到 Bootmem 内存分配器中进行管理，但加入进去的物理内存区全部标记为不可用。然后函数在 769 行调用 free_bootmem_with_active_region() 函数用于将系统可用的物理内存在 Bootmem 内存分配器中标记为可用，最后返回这段物理内存在 bitmap 中的结束位置。

> [init_bootmem_node](#D30004)

<span id="DX103"></span>

![](/assets/PDB/HK/HK000576.png)

函数在 3622 行调用 for_each_active_range_idx_in_nid() 函数遍历当前系统中可用的物理内存区。在遍历每个内存区域的时候，函数只处理低端物理内存区，对于高端内存区直接选择跳过。函数接着调用 free_bootmem_node() 函数将对于低端内存区域在 Bootmem 分配器中标记为可用。

> [free_bootmem_node](#D30035)

<span id="DX104"></span>

![](/assets/PDB/HK/HK000554.png)

start_kernel 初始化完与体系相关的代码之后，在 mm_init() 函数中准备构建 buddy 内存分配器，mm_init() 函数调用 mem_init() 函数进行 buddy 内存分配器的创建。

<span id="DX105"></span>

![](/assets/PDB/HK/HK000577.png)

mem_init() 函数直接在 878 行调用 free_all_bootmem() 函数将 Bootmem 分配器管理的低端物理内存全部移交给 Buddy 内存分配器进行管理。至此 Bootmem 分配器完成使命。

> [free_all_bootmem](#D30036)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="DX0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

#### Bootmem Allocator on ARM Architecture

Linux 2.6.35 及其以前，ARM 体系结构的初始阶段，通过 ATAG 从 uboot 获得内存相关的信息，并将其存储在 meminfo 数据结构中进行维护，以此构成早期的内存信息，ARM 体系将这些内存信息在初始化过程中传递给 Bootmem 内存分配器，以此构建内核早期的内存管理器，为 Buddy 内存分配器的创建提供了基础，最终 Bootmem 分配器将管理的物理内存全部移交给 Buddy 内存管理器进行维护，Bootmem 分配器完成自己的使命其代码逻辑如下:

![](/assets/PDB/HK/HK000542.png)

> - [parse_tag_mem32](#DX000)
>
>   - [arm_add_memory](#DX001)
>
> - parse_early_param
>
>   - [early_mem](#DX002)
>
> - [paging_init](#DX003)
>
>   - sanity_check_meminfo
>
>   - [bootmem_init](#DX005)
>
>     - find_node_limits
>
>     - [bootmem_init_node](#DX007)
>
>     - [reserve_node_zero](#DX008)
>
>     - [bootmem_free_node](#DX009)
>
> - [mm_init](#DX010)
>
>   - [mem_init](#DX011)
>
>     - [free_all_bootmem_node](#D30035)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

<span id="DX000"></span>

![](/assets/PDB/HK/HK000543.png)

ARM 在初始化过程中，首先通过 ATAG 机制从 uboot 获得物理内存布局的信息，然后通过 parse_tag_mem32 函数解析出来，并将这些物理内存信息存储在 meminfo.bank 里面, arm_add_memory() 函数完成实际的添加动作.

<span id="DX001"></span>

![](/assets/PDB/HK/HK000544.png)

arm_add_memory() 函数主要负责将参数对应的内存区域加入到系统维护的 meminfo 数据结构里，参数 start 和 size 指明了添加内存区域的信息。函数首先在 390 行获得一个新的 struct membank 内存区间，meminfo.nr_banks 用于统计当前 meminfo 中一共维护 struct membank 的数量，如果 meminfo.nr_banks 超过了 NR_BANKS 的值，那么 ARM 认为系统认为 meminfo 不能在维护更多的内存信息，这回丢弃掉一些内存信息，这将导致系统重要资源的缺失，未来一定会引起问题，所以直接返回错误。如果检测通过，那么函数将内存区域的信息存储到 struct membank 数据结构中。函数接着检测该 struct meminfo，如果发现 size 成员等于 0 或者 node 成员超过了 MAX_NUMNODES 的值，那么系统直接报错; 如果没有检测出问题，那么函数更新 meminfo.nr_banks 的统计，以上便完成了 meminfo 数据的创建.

<span id="DX002"></span>

ARM 内核继续初始化，接着调用 parse_early_param() 函数解析内核早期的参数，如果 CMDLINE 中包含 "mem=" 的信息，那么函数就会调用 early_mem() 函数.

![](/assets/PDB/HK/HK000545.png)

CMDLINE 中的 "mem=" 参数用于设置系统可用物理内存范围，其通过设置内存的起始地址和内存长度来进行设置。如果 CMDLINE 中包含了该字段，那么函数 early_mem() 函数将通过 early_param() 函数调用。函数首先在 433 行进行判断，如果系统支持 CMDLINE 中的 "mem=" 字段，那么内核将不再使用 ATAG 中传递的内存信息，而是只采用 CMDLINE 传递的内存信息作为系统内存信息。函数首先判断 usermem 的值是否为 0，也就代码了该 "mem=" 信息是否第一次使用，因为在 CMDLINE 中 "mem=" 的字段是可以多次使用，因此如果 "mem=" 字段是第一次解析，那么函数将 usermem 设置为 1，并将 meminfo.nr_banks 的值设置为 1，这样的操作直接将从 ATAG 获得的内存信息直接丢弃. 函数在 438 - 441 行将 "mem=" 字段中的内存信息解析出来，然后将其传递给 arm_add_memory() 函数将对应的内存区域信息添加到 meminfo 数据结构中。

<span id="DX003"></span>

![](/assets/PDB/HK/HK000546.png)

ARM 初始化到 paging_init() 函数之后，首先在 1063 行调用 sort() 函数将 meminfo 中的 bank 按起始物理物理地址从低到高进行排序，接着函数在 1066 行调用 sanity_check_meminfo() 函数检测. 检测完毕之后，函数调用 bootmem_init() 函数进行 Bootmem 内存分配器的初始化。

<span id="DX005"></span>

![](/assets/PDB/HK/HK000547.png)

bootmem_init() 函数首先获得 meminfo 数据结构，然后在 372 行调用 check_initr() 函数检测 INITRD 相关的信息，接着函数将 max_low 和 max_high 都设置为 0.

![](/assets/PDB/HK/HK000548.png)

bootmem_init() 函数在 379 行调用 for_each_node() 函数遍历所有的 NODE. 在遍历每个 NODE 过程中，函数首先在 382 行调用 find_node_limits() 函数找出该 NODE 中最小的起始物理页帧并存储在 min 变量里，找出最大的物理页帧且不是高端物理内存，那么将最大物理页帧存储在 node_low 和 node_high 中，如果是高端物理内存，那么最大物理页帧存储在 node_high 中，而 node_low 则不更新. 函数在 384 - 387 行将该 NODE 最大的物理页帧信息存储在 max 里，而该 NODE 最小的物理页帧号则存储在 min 里。函数在 393 行检测 node_low 是否为 0，如果此时为 0，那么表示物理内存全部都是高端物理内存，那么 Bootmem 分配器不管理高端内存，那么直接忽略这部分内存。

![](/assets/PDB/HK/HK000549.png)

函数获得该 NODE 最小物理页帧和低端物理内存最大物理页帧之后，调用 bootmem_init_node() 函数进行 ARM 架构的 Bootmem 内存初始化，其实现如下:

<span id="DX007"></span>

![](/assets/PDB/HK/HK000550.png)

在 bootmem_init_node() 函数中，函数在 237 行调用 bootmem_bootmap_pages() 计算出全部的物理页帧，每个物理页帧占用一个 bit，总共需要占用多少个物理页，并将结果存储在 boot_pages 里，函数接着调用 find_bootmap_pfn() 函数对比内核的 "\_end" 对应物理地址之后的可用物理地址，也就是 find_bootmap_pfn() 函数确保 Bootmem 分配器管理的物理内存在内核镜像占用的内存之后。函数接着调用 node_set_online() 将当前 NODE 上线，并将 pgdat 指向了当前 NODE。函数在 246 行调用 init_bootmem_node() 函数完成 Bootmem 分配器对该物理内存区域的初始化，Bootmem 分配器在初始化这段物理内存过程中，将对应的物理内存全部标记为不可用，只建立了相应的 bitmap，以及将该物理内存区域，具体函数实现可参照如下:

> [init_bootmem_node 详解](#D30004)

Bootmem 分配器初始化完毕之后，在 248 行函数调用 for_each_nodebank() 函数遍历当前 NODE 的所有 membank，只要遍历到的 membank 不是高端物理内存区，那么函数将对应的物理内存区域通过 free_bootmem_node() 函数释放为可用的物理内存区域。函数遍历完毕之后，函数调用 reserve_bootmem_node() 函数将 bitmap 对应的物理内存区域标记为 Reserved，以免 Bootmem 将其分配给调用者使用。

![](/assets/PDB/HK/HK000549.png)

回到 bootmem_init 函数，bootmem_init_node() 函数执行完毕之后，函数在 400 行进行判断，如果当前是 NODE 0，那么函数会调用 reserve_node_zero() 函数将 NODE 0 上特定区域进行预留，其中包含如下:

<span id="DX008"></span>

![](/assets/PDB/HK/HK000551.png)

函数首先在 840 - 846 行通过 CONFIG_XIP_KERNEL 宏的情况，将 "\_data - \_end" 或者 "\_stext - \_end" 的区域进行预留，即将内核本省占用的物理内存在 Bootmem 内存分配中进行预留。接着函数在 852 - 853 行，函数将 swapper_pg_dir 对应的全局页目录在 bootmem 分配器中进行预留，接下来函数将体系相关的区域也进行了预留操作。

![](/assets/PDB/HK/HK000549.png)

回到 bootmem_init 函数，函数如果检测到当前 NODE 和 INITRD 所在的 NODE 是同一个 NODE，那么函数调用 bootmem_reserve_initrd() 函数将 INITRD 所占用的物理内存进行预留。函数在 413 行调用 arm_memory_present() 函数对支持稀疏内存模型的架构进行初始化，如果当前内存模型是平坦内存模型的话，那么函数不做任何操作。

![](/assets/PDB/HK/HK000552.png)

在函数 bootmem_init() 中，函数在 419 行调用 sparse_init() 函数以便在支持稀疏内存模型的架构上构建稀疏内存的基础信息，该函数会从 Bootmem 内存分配器中分配、使用并释放内存。函数接着调用 for_each_node() 遍历所有的 NODE，并在每次遍历中调用 bootmem_free_node() 函数构建 ZONE 的基础信息. 函数在 429 行将全局的 high_memory 变量设置为 max_low 对应的虚拟地址，因此 high_momory 指向了高端内存起始的虚拟地址. max_low_pfn 则指向低端内存的最大物理页帧号.

<span id="DX009"></span>

![](/assets/PDB/HK/HK000553.png)

在 bootmem_free_node() 函数中，函数首先在 288 行调用 find_node_limits() 函数获得当前 NODE 的最大物理页帧、最小物理页帧，以及低端内存最大物理页帧的信息。函数接着在 293 行将 zone_size 对应的数据清零，然后将 "zone_size[0]" 的值设置为当前 NODE 低端物理内存的页帧数。如果当前系统支持高端物理内存，那么函数将高端内存信息存储在 zone_size[] 数组的 ZONE_HIGHMEM 成员里，接着函数将 zone_size[] 数组的信息拷贝到 zhole_size[] 数组里，以便计算内存空洞。函数在 310 行调用 for_each_nodebank() 函数 zhole_size[] 中的数据减去 membank 中物理内存的数量，以此计算出空洞的大小。接着函数调用 arch_adjust_zones() 函数调整了 Zone 相关的信息，

<span id="DX010"></span>

![](/assets/PDB/HK/HK000554.png)

start_kernel 初始化完与体系相关的代码之后，在 mm_init() 函数中准备构建 buddy 内存分配器，mm_init() 函数调用 mem_init() 函数进行 buddy 内存分配器的创建。

<span id="DX011"></span>

![](/assets/PDB/HK/HK000555.png)

mem_init() 函数在 537 行调用 for_each_online_node() 函数遍历所有的 NODE，并且将遍历到的 NODE 进行检测，检测当前 NODE 是否维护物理页，也就是 node_spanned_pages 的值，如果 node_spanned_pages 不为零，那么表示该 NODE 上有物理页，那么函数调用 free_all_bootmem_node() 函数将对应的物理页转移给 Buddy 内存分配器进行管理，至此 Bootmem 分配器完成其初始化阶段的使命。

> [free_all_bootmem_node 详解](#D30035)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30043">\_\_alloc_bootmem_low_node</span>

![](/assets/PDB/HK/HK000541.png)

\_\_alloc_bootmem_low_node() 函数用于从指定的低端物理内存中分配物理内存。pgdat 参数指向指定的 NODE，size 参数指明分配的长度，align 参数指明对齐的方式，goal 参数指明从指定地址分配物理内存。

函数首先在 991 行检测 SLAB 分配器是否可用，如果可用，那么直接调用 kzalloc_node() 函数进行实际的分配; 反之函数调用 \_\_\_alloc_bootmem_node() 函数进行分配，并将分配的上限设置为 ARCH_LOW_ADDRESS_LIMIT。最后返回分配对应的虚拟地址。

> [\_\_\_alloc_bootmem_node 详解](#D30022)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30042">\_\_alloc_bootmem_node_nopanic</span>

![](/assets/PDB/HK/HK000540.png)

\_\_alloc_bootmem_node_nopanic() 函数用于 Bootmem 分配器从指定的物理内存区域分配物理内存，如果分配失败不会引起 panic。参数 pgdat 指向指定的物理内存区域，参数 size 指明分配的长度，align 参数指明对齐方式，参数 goal 指明从指定的物理地址开始分配。

函数首先在 929 行检测 SLAB 是否可以使用，如果可以使用，直接调用 kzalloc_node() 函数进行分配; 反之 SLAB 不能使用，那么函数首先调用 alloc_arch_preferred_bootmem() 函数从当前 NODE 推荐的物理内存区域上分配物理内存，如果分配成功直接返回对应的虚拟地址; 反之如果分配失败，那么函数继续调用 alloc_bootmem_core() 函数从指定的物理内存区域上分配物理内存，如果分配成功则直接返回对应的虚拟地址; 反之如果分配失败，那么函数继续调用 \_\_alloc_bootmem_nopanic() 函数进行分配，如果分配成功则返回对应的虚拟地址，如果分配失败不会引起 panic.

> [alloc_arch_preferred_bootmem 详解](#D30014)
>
> [alloc_bootmem_core 详解](#D30012)
>
> [\_\_alloc_bootmem_nopanic 详解](#D30040)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30041">\_\_alloc_bootmem_node_high</span>

![](/assets/PDB/HK/HK000539.png)

\_\_alloc_bootmem_node_high() 函数用于从 MAX_DMA32_PFN 对应的物理地址之后进行分配。参数 pgdat 指向 NODE，size 参数指明分配的长度，参数 align 指明对齐的方式，goal 参数指明期盼从指定物理地址开始分配.

如果没有定义 MAX_DMA32_PFN 宏，那么系统直接调用 \_\_alloc_bootmem_node() 函数进行分配指定长度的物理内存; 反之如果定义 MAX_DMA32_PFN 宏，那么函数首先在 862 行进行检测 SLAB 分配器是否可用，如果可用就调用 kzalloc_node() 函数进行分配; 反之如果 SLAB 分配器不可用，那么函数计算当前 NODE 应该结束的物理页帧号。如果该 NODE 结束页帧号大于 MAX_DMA32_PFN 之后 128M 对应的物理页帧，且 goal 参数对应的页帧小于 MAX_DMA32_PFN, 那么函数将 new_goal 设置为 MAX_DMA32_PFN 对应的物理地址，并调用 alloc_bootmem_core() 函数直接从 new_goal 对应的地址开始分配, 最后分配成功返回对应的虚拟地址.

> [alloc_bootmem_core 详解](#D30012)
>
> [\_\_alloc_bootmem_node 详解](#D30023)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30040">\_\_alloc_bootmem_nopanic</span>

![](/assets/PDB/HK/HK000538.png)

\_\_alloc_bootmem_nopanic() 函数的作用是从 Bootmem 中分配物理内存，如果分配失败不会触发 panic。参数 size 指明分配的长度，align 参数指明对齐方式，goal 参数指明期盼从该地址开始进行分配. 函数通过调用 \_\_\_alloc_bootmem_nopanic() 函数完成实际的分配操作.

> [\_\__alloc_bootmem_nopanic 详解](#D30015)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30039">reserve_bootmem</span>

![](/assets/PDB/HK/HK000537.png)

reserve_bootmem() 函数用于将一段物理内存区域标记为预留。参数 addr 和 size 指明了需要标记的物理内存范围，flags 参数指明了标记时候的标志。函数在 521 - 522 行获得需要标记内存区域的起始页帧号和终止页帧号，并在 524 行调用 mark_bootmem() 函数进行实际的标记操作.

> [mark_bootmem 详解](#D30037)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30038">free_bootmem</span>

![](/assets/PDB/HK/HK000536.png)

free_bootmem() 函数用于在 Bootmem 分配器中将一段物理内存标记为可用. 参数 addr 和 size 指向需要标记的空间。函数在 468 - 469 行获得需要标记内存区域的起始物理页帧和终止物理页帧，并在 471 行调用 mark_bootmem() 函数将对应的区域在 Bootmem 分配内存标记为可用。

> [mark_bootmem](#D30037)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30037">mark_bootmem</span>

![](/assets/PDB/HK/HK000535.png)

mark_bootmem() 函数用于标记一段物理内存区域，参数 start 和 end 指明一段物理内存区域，reserve 参数用于指明标记的方式，flags 参数用于标记的标志。mark_bootmem() 函数可以将一段物理内存区域标记为可用，也可以标记为预留。

函数在 396 行将 pos 设置为 start，并在其后调用 list_for_each_entry() 函数遍历 bdata_list 上的所有物理内存区域，每当遍历一个物理内存区域的时候，函数会在 401 - 402 行进行检测，如果 pos 小于当前物理内存区域的起始地址，或者 pos 不小于当前物理内存区域的终止物理地址，那么函数此时如果检测到 pos 不等于 start，那么通过 BUG_ON() 函数报错，否则跳转到下一次物理内存区域的遍历。继续本次遍历，函数在 407 行将 max 指向了当前物理内存结束物理地址和 end 参数之间最小的一个，以确保范围的有效性。接着函数在 409 行调用 mark_bootmem_node() 函数标记 Bootmem 内存分配器中指定的物理内存区域。标记完毕之后，函数在 410 行进行检测，如果 reserve 为 1，但 err 也唯一，那么表示之前的标记过程中出现错误，那么函数将撤回之前的标记，于是在 411 - 412 行，函数调用 mark_bootmem() 函数将以及标记过的物理内存区域由预留状态转换为可用物理内存状态, 并返回错误值; 反之标记成功之后，函数在 415 行进行检测，如果此时 max 的值等于 end，那么表示没有更多的物理内存可以标记了，直接返回 0; 反之函数将 pos 指向了当前物理内存区域的结束地址，并执行下一次循环。如果以上循环结束都没有返回，那么函数在 419 行调用 BUG() 函数进行报错.

> [bdata_list 详解](#D100)
>
> [mark_bootmem 详解](#D30037)
>
> [mark_bootmem_node 详解](#D30008)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30036">free_all_bootmem</span>

![](/assets/PDB/HK/HK000534.png)

free_all_bootmem() 函数用于将 Bootmem 分配器管理的可用物理内存转移给 Buddy 内存分配器. 函数在 319 行调用 list_for_each_entry() 函数遍历 bdata_list 链表上的所有物理内存区域，每次遍历一个内存区域，函数会调用 free_all_bootmem_core() 函数将该物理内存区域上可用的物理内存转移给 Buddy 内存分配器，转移完毕后并将转移的数量更新到 total_pages 变量里. 函数完成自身功能之后，返回 total_pages。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30035">free_all_bootmem_node</span>

![](/assets/PDB/HK/HK000533.png)

free_all_bootmem_node() 函数用于 Bootmem 分配器将指定 NODE 上的物理内存转移给 Buddy 内存分配器。参数 pgdat 指向指定的 NODE。函数首先调用 register_page_bootmem_info_node() 函数将对应的 NODE 信息添加到内存热插拔机制里面，然后调用 free_all_bootmem_core() 函数将物理内存转移给 Buddy 内存分配器.

> [free_all_bootmem_core](#D30034)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30034">free_all_bootmem_core</span>

![](/assets/PDB/HK/HK000531.png)

free_all_bootmem_core() 函数的作用是 Bootmem 分配器将其管理的一段物理内存全部移交给 Buddy 内存分配器进行管理。参数 bdata 指向一个段物理内存区域。

函数在 226 行首先接触该物理内存区是否包含 bitmap，如果没有包含指定的 bitmap，那么直接返回 0. 接着函数在 229 - 230 行获得该物理内存区域的起始物理页帧号和终止物理页帧号。函数在 236 行进行了对齐检测，如果起始页帧按机器位宽对齐，那么 Bootmem 分配器就可以按 bulk 的方式将批量的物理内存转移给 Buddy 内存分配器; 反之只能将没有对齐的部分按单独的物理页帧转移给 Buddy 分配器. 函数在 244 - 245 行获得物理内存区域对应的 bitmap，以及起始地址在 bitmap 中的索引，函数在 246 行将 idx 处开始的 BITS_PER_LONG 个 bit 值全部取反。函数在 248 行开始判断，如果 aligned 为真，也就是 start 对应的物理页帧已经按机器的位宽进行对齐了，可以按批量方式进行转移。此时如果 vec 的 BITS_PER_LONG 全为 1，那么对应的物理页帧全部可用。如果此时 start 加上 BITS_PER_LONG 的值没有达到 end，以此表示这个范围是可以操作的范围，因此函数执行 249 - 252 行的逻辑。函数首先获得 BITS_PER_LONG 对应的阶数，然后调用 \_\_free_pages_bootmem() 函数将对应的物理页一次性转移到 Bootmem 分配器里，并更新 count 的值; 反之如果上面的三个条件有其中一个条件不满足，那么代表 BITS_PER_LONG 个物理页帧中有部分物理页帧是不能转移给 Buddy 物理内存管理的，因此函数执行 254 - 263 行逻辑，函数首先在 256 行使用 while 循环，只要 vec 为真，且 off 小于 BITS_PER_LONG, 那么就遍历，每次遍历到一个 bit 时候，函数首先判断 vec 是否为 1，如果为 1 代表该 bit 对应的物理页帧是可以转移给 Buddy 分配器的，那么函数此时调用 pfn_to_page() 函数获得对应的 struct page 结构，并调用 \_\_free_pages_bootmem() 函数将对应的物理页帧转移给 Buddy 内存分配器，最后更新 count 的计数，并向右移 vec 和 off 指向下一个 bit。 处理完以上两种情况之后，函数将 start 的值增加 BITS_PER_LONG, 以此进行下一次循环或结束循环。

![](/assets/PDB/HK/HK000532.png)

函数转移完可用的物理页之后，函数在 269 行调用 virt_to_page() 函数获得物理内存区域 bitmap 对应的 struct page. 接着函数通过 270 - 271 计算出当前物理内存区域对应的 bitmap 占用的物理内存页的数量, 然后更新到 count 变量里，最后使用 while 循环将 bitmap 占用的物理页通过 \_\_free_pages_bootmem() 函数转移到 Buddy 内存分配器里面，至此函数完成其功能并返回释放物理页的数量.

> [\_\_free_pages_bootmem 详解](#D30032)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30033">free_bootmem_late</span>

![](/assets/PDB/HK/HK000530.png)

free_bootmem_late() 函数的作用是将 Bootmem 分配管理的一段物理内存区域移交给 Buddy 内存分配器管理。参数 addr 和 size 指明内存区域的范围。函数首先在 164 - 165 行分别获得物理内存区域的起始页帧号和终止页帧号，并在 167 行使用 for 循环，以此遍历设计的物理页帧，没遍历一个物理页帧，函数调用 \_\_free_pages_bootmem() 函数将该物理页帧移交给 Buddy 分配器进行管理，移交完毕之后更新 totoalram_pages 的数量.

> [\_\_free_pages_bootmem](#D30032)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30032">\_\_free_pages_bootmem</span>

![](/assets/PDB/HK/HK000529.png)

\_\_free_pages_bootmem() 函数用于将 Bootmem 分配器所管理的物理内存转移给 Buddy 内存分配器。参数 page 指向一个物理页，order 参数指明包含物理页的数量.

函数在 690-694 行用于将一个物理页转移到 Buddy 内存分配器，在转移一个物理页的时候，函数在 691 行首先调用 \_\_ClearPageReserved() 函数将 struct page 从 Reserved 状态转换为可用状态. 接着调用 set_page_count() 函数将 struct page 的使用计数设置为 0，代表 struct page 没有被使用. 函数在 639 行继续调用 set_page_refcounted() 将 struct page 的 \_count 引用计数设置为 1. 最后调用 \_\_free_page() 函数将物理页加入到 Buddy 分配器中.

函数在 696 - 709 行将 order 为 5 的物理页转移到 Buddy 分配器中。函数首先在 698 行预加载 struct page 数据结构，然后使用 for 循环，将 order 为 5 里面的所有 struct page 都遍历一遍，在每次遍历过程中，函数在 702 - 703 行预先加载下一个 struct page 数据结构，然后在 703 行调用 \_\_ClearPageReserved() 函数将 struct page 由 Reserved 状态切换为可使用的状态，接着在 705 行调用 set_page_count() 函数将 struct page 的使用计数设置为 0. 遍历完 order 为 5 中的所有 struct page 之后，函数在 708 行调用 set_page_refcounted() 将首 struct page 的 \_count 引用计数设置为 1，最后调用 \_\_free_pages() 将 struct page 插入到 Buddy 分配器里面.

该函数就是 Buddy 分配器创建的核心实现，也是 Bootmem 分配器与 Buddy 分配器之间交流的重要实现.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)


---------------------------------------------

#### <span id="D30032">bootmem_debug_setup</span>

![](/assets/PDB/HK/HK000528.png)

bootmem_debug_setup() 函数用于从 CMDLINE 中解析 "bootmem_debug" 参数，以此确认是否开启或关闭 Bootmem 分配器的 debug 功能。如果 CMDLINE 参数中包含 "bootmem_debug"，那么 Bootmem 分配器开始 debug 功能，否则关闭 debug 功能.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30030">alloc_bootmem_low_pages_node</span>

{% highlight bash %}
#define alloc_bootmem_low_pages_node(pgdat, x) \
        __alloc_bootmem_low_node(pgdat, x, PAGE_SIZE, 0)
{% endhighlight %}

alloc_bootmem_low_pages_node() 函数的作用是 Bootmem 分配从低端内存指定的 NODE 上分配一定数量的物理内存。函数通过调用 \_\_alloc_bootmem_low() 函数实现，并将分
配的起始地址设置为 0，以及对齐方式设置为 PAGE_SIZE 方式对齐.

> [\_\_alloc_bootmem_low 详解](#D30027)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30029">alloc_bootmem_low_pages</span>

{% highlight bash %}
#define alloc_bootmem_low_pages(x) \   
        __alloc_bootmem_low(x, PAGE_SIZE, 0)
{% endhighlight %}

alloc_bootmem_low_pages() 函数用于 Bootmem 分配器从低端内存分配指定数量的物理页。参数 x 指明物理页的个数。函数通过调用 \_\_alloc_bootmem_low() 函数实现，并将分配的起始地址设置为 0，以及对齐方式设置为 PAGE_SIZE 方式对齐.

> [\_\_alloc_bootmem_low 详解](#D30027)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30028">alloc_bootmem_low</span>

{% highlight bash %}
#define alloc_bootmem_low(x) \
        __alloc_bootmem_low(x, SMP_CACHE_BYTES, 0)
{% endhighlight %}

alloc_bootmem_low() 函数的作用是 Bootmem 从低端内存中分配物理内存。参数 x 指明分配内存的长度。函数通过调用 \_\_alloc_bootmem_low() 函数实现，并将分配的起始地址设置为 0，以及对齐方式设置为 SMP_CACHE_BYTES 方式对齐.

> [\_\_alloc_bootmem_low 详解](#D30027)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30027">\_\_alloc_bootmem_low</span>

![](/assets/PDB/HK/HK000527.png)

\_\_alloc_bootmem_low() 函数的作用是 Bootmem 分配器从低端内存分配物理内存。参数 size 指明分配物理内存的长度，align 表示对齐方式，goal 表示期盼开始分配的物理地址. 函数通过调用 \_\_\_alloc_bootmem() 函数实现，并将 ARCH_LOW_ADDRESS_LIMIT 的值作为最大物理地址限制。

> [\_\_\_alloc_bootmem 详解](#D30016)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30026">alloc_bootmem_pages_node_nopanic</span>

{% highlight bash %}
#define alloc_bootmem_pages_node_nopanic(pgdat, x) \
        __alloc_bootmem_node_nopanic(pgdat, x, PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
{% endhighlight %}

alloc_bootmem_pages_node_nopanic() 函数的作用是 Bootmem 分配器从指定的 NODE 上分配一定数量的物理内存，如果分配失败不会触发 panic。参数 pgdat 指向指定的 NODE，参数 x 指明分配物理页的数量. 函数通过调用 \_\_alloc_bootmem_node_nopanic() 函数实现，并将对齐方式设置为 PAGE_SIZE, 最大物理地址设置为 MAX_DMA_ADDRESS 对应的物理地址.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30025">alloc_bootmem_pages_node</span>

{% highlight bash %}
#define alloc_bootmem_pages_node(pgdat, x) \
        __alloc_bootmem_node(pgdat, x, PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
{% endhighlight %}

alloc_bootmem_pages_node() 函数用于从指定的 NODE 上分配一定数量的物理页。参数 pgdat 指向指定的 NODE，x 参数指明分配物理页的个数. 函数通过调用 \_\_alloc_bootmem_node() 函数完成实际的内存分配, 并将对齐方式设置为 PAGE_SIZE, 最大物理地址设置为 MAX_DMA_ADDRESS 对应的物理地址.

> [\_\_alloc_bootmem_node 详解](#D30023)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30024">alloc_bootmem_node</span>

{% highlight bash %}
#define alloc_bootmem_node(pgdat, x) \
        __alloc_bootmem_node(pgdat, x, SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))
{% endhighlight %}

alloc_bootmem_node() 函数的作用就是从指定的 NODE 上分配指定长度的物理内存，并返回对应的虚拟地址。参数 x 指明分配内存的长度。函数通过调用 \_\_alloc_bootmem_node() 函数实现实际的分配动作, 并以 SMP_CACHE_BYTES 为对齐方式，MAX_DMA_ADDRESS 对应的物理地址作为最大的分配限制.

> [\_\_alloc_bootmem_node 详解](#D30023)

从运行的结果可以看出，函数从指定的 NUMA NODE 0 上分配物理内存，以预期相符.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30023">\_\_alloc_bootmem_node</span>

![](/assets/PDB/HK/HK000526.png)

\_\_alloc_bootmem_node() 函数用于从 Bootmem 分配器中指定的 node 上分配内存，并返回对应的虚拟地址。参数 pgdat 指向指定的 NODE，size 参数指明分配内存的大小，参数 align 指明对齐的方式，goal 参数指明期盼从指定的物理地址开始分配。

函数首先在 838 行进行检测，如果此时 SLAB 内存分配器已经可以使用，那么整个系统环境应该是一个比较稳定的时候，直接调用 kzalloc_node() 函数从 SLAB 中分配内存; 反之如果此时还在内核初始化早期，SLAB 分配器还不存在，那么函数就调用 \_\_\_alloc_bootmem_node() 函数从指定 NODE 上分配物理内存, 并返回对应的虚拟地址.

> [\_\_\_alloc_bootmem_node 详解](#D30022)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30022">\_\_\_alloc_bootmem_node</span>

![](/assets/PDB/HK/HK000525.png)

\_\_\_alloc_bootmem_node() 函数用于 Bootmem 分配器从指定的 node 中分配物理内存，并返回对应的虚拟地址。参数 bdata 指向指定的物理内存区域，参数 size 指明分配内存的长度，align 参数指明对齐方式，goal 参数指明期盼从指定物理地址开始分配，参数 limit 则指明分配的最大物理地址限制.

函数首先在 806 行调用 alloc_arch_preferred_bootmem() 函数从体系推荐的物理内存区域进行分配，如果分配成功，则返回对应的虚拟地址; 反之如果分配失败，则调用 alloc_bootmem_core() 函数从 Bootmem 分配器中指定的物理内存区域分配内存，如果分配成功则返回对应的虚拟地址; 反之如果分配还失败，那么函数在 814 行调用 \_\_\_alloc_bootmem() 函数再次从 Bootmem 内存分配器中尽可能的分配物理内存，并返回对应的虚拟地址.

> [alloc_arch_preferred_bootmem 详解](#D30014)
>
> [alloc_bootmem_core 详解](#D30012)
>
> [\_\_\_alloc_bootmem 详解](#D30016)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30021">alloc_bootmem_pages_nopanic</span>

{% highlight bash %}
#define alloc_bootmem_pages_nopanic(x) \
        __alloc_bootmem_nopanic(x, PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
{% endhighlight %}

alloc_bootmem_pages_nopanic() 函数用于从 Bootmem 分配器中分配指定数量的物理页，如果分配失败会触发 panic。参数 x 代表物理页的个数。函数通过调用 \_\_alloc_bootmem_nopanic() 函数完成实际的分配工作，并设置对齐方式为 PAGE_SIZE, 以及最大物理地址为 MAX_DMA_ADDRESS 对应的物理地址.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30020">alloc_bootmem_pages</span>

{% highlight bash %}
#define alloc_bootmem_pages(x) \
        __alloc_bootmem(x, PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
{% endhighlight %}

alloc_bootmem_pages() 函数用于从 Bootmem 分配器中分配指定长度的物理页，并返回对应的虚拟地址。参数 x 代表分配物理页的数量. 函数通过调用 \_\_alloc_bootmem() 函数实现，并传入了 PAGE_SIZE 作为对齐方式，以及 MAX_DMA_ADDRESS 对应的物理地址作为最大物理地址.

> [\_\_alloc_bootmem 详解](#D30017)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30019">alloc_bootmem_nopanic</span>

{% highlight bash %}
#define alloc_bootmem_nopanic(x) \
	__alloc_bootmem_nopanic(x, SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))
{% endhighlight %}

alloc_bootmem_nopanic() 函数用于从 Bootmem 分配器中分配指定长度的物理内存，并返回对应的虚拟地址，如果分配失败不会触发 panic。参数 x 指明分配内存的长度。函数调用 \_\_alloc_bootmem_nopanic() 函数实现，并且将对齐方式设置为 SMAP_CACHE_BYTES, 最大物理内存设置为 MAX_DMA_ADDRESS 对应的物理地址.

> [\_\__alloc_bootmem_nopanic 详解](#D30015)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30018">alloc_bootmem</span>

{% highlight bash %}
#define alloc_bootmem(x) \
	__alloc_bootmem(x, SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))
{% endhighlight %}

alloc_bootmem() 函数用于从 Bootmem 分配器中分配指定长度的内存。参数 x 表示分配内存的大小。函数通过向 \_\_alloc_bootmem() 函数传入 SMP_CACHE_BYTES 对齐方式，并且最大物物理地址设置为 MAX_DMA_ADDRESS 对应的物理地址，以此分配内存.

> [\_\_alloc_bootmem 详解](#D30017)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30017">\_\_alloc_bootmem</span>

![](/assets/PDB/HK/HK000524.png)

\_\_alloc_bootmem() 函数用于从 Bootmem 分配中分配指定长度的物理内存，并返回对应的虚拟地址。参数 size 指明分配内存的长度，align 参数指明对齐的方式，参数 goal 指明期盼分配的起始物理地址。

函数在 790 将 limit 设置为 0，并在 796 行调用 \_\_\_alloc_bootmem() 函数进行实际的物理内存分配, 最终返回对应的虚拟地址.

> [\_\_\_alloc_bootmem 详解](#D30016)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30016">\_\_\_alloc_bootmem</span>

![](/assets/PDB/HK/HK000523.png)

\_\_\_alloc_bootmem() 函数用于从 Bootmem 分配器中分配指定长度的内存。size 参数指明分配内存的大小，align 参数指明对齐方式，goal 参数则指明期盼从该物理地址进行分配，limit 参数指明分配的最大物理地址限制.

函数在 762 行调用 \_\__alloc_bootmem_nopanic() 函数进行实际的物理内存分配，如果分配成功，那么函数直接返回分配的地址; 反之如果分配失败，那么函数打印相关的信息，并触发 panic() 返回 NULL.

> [\_\__alloc_bootmem_nopanic 详解](#D30015)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30015">\_\__alloc_bootmem_nopanic</span>

![](/assets/PDB/HK/HK000522.png)

\_\__alloc_bootmem_nopanic() 函数用于从 Bootmem 分配器中分配指定长度的内存，并且内存分配失败不会引起 panic(). 参数 size 指明分配的长度，align 指明对齐方式，goal 参数指明预期分配的起始物理地址，limit 参数指明分配的最大限制。

函数在 690 行调用 alloc_arch_preferred_bootmem() 函数从 Bootmem 分配器中分配指定长度的物理内存，如果分配成功，那么函数直接返回; 如果分配不成功，那么函数就调用 list_for_each_entry 函数遍历 Bootmem 分配器维护的所有物理内存空间，在遍历过程中，如果 goal 参数不为零，且当前物理内存区域的终止物理页帧小于或等于 goal 对应的物理页帧号，那么函数跳过这个物理内存区域，继续遍历下一个物理内存区域。函数继续遍历，如果 limit 参数存在，且当前物理内存区域的起始物理页帧号大于 limit 对应的物理页帧号，那么函数直接跳出循环; 如果以上两个检测条件都满足，那么函数找到一个合适的物理内存区域，那么调用 alloc_bootmem_core() 函数，Bootmem 分配器就从指定的物理内存区域中分配物理内存. 分配成功则直接返回; 分配失败则继续查找其他物理内存区域进行分配。函数如果有机会指定到 705 行，那么此时检测到 goal 不为 0，那么让 Bootmem 分配器不再从指定物理内存分配内存，只要能分配物理内存就行，那么将 goal 设置为 0，跳转到 restart 继续分配. 如果以上的努力都不行，那么函数直接返回 NULL，那代表找不到物理内存.

> [alloc_bootmem_core](#D30012)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30014">alloc_arch_preferred_bootmem</span>

![](/assets/PDB/HK/HK000521.png)

alloc_arch_preferred_bootmem() 函数用于从 bootmem 中分配指定长度的物理内存。参数 bdata 指向物理内存区域，size 指明分配的长度，align 参数指明对齐的方式，goal 参数指明期望开始分配的位置，limit 指明最大分配限制。

函数在 663 行至 664 行对当前的环境进行检测，如果 slab_is_available() 函数返回真，那么当前 SLAB 分配器可用，直接通过 kzalloc() 函数进行分配; 反之该阶段处于内核初始化早期， SLAB 分配器还不可用，且 CONFIG_HAVE_ARCH_BOOTMEM 宏存在，那么在 670 行调用 bootmem_arch_preferred_node() 函数去查找一个符合要求的物理内存区域，如果该区域找到，那么函数在 673 行调用 alloc_bootmem_core() 函数从物理内存区域中分配指定长度的物理内存; 反之如果 CONFIG_HAVE_ARCH_BOOTMEM 宏没有定义，那么函数直接返回 NULL。

> [alloc_bootmem_core 详解](#D30012)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30013">align_off</span>

![](/assets/PDB/HK/HK000516.png)

align_off() 函数用于按物理地址宽度进行对齐。参数 bdata 指向指定的物理内存区域，off 参数指向一段物理地址偏移，align 参数指明对齐的方式。

函数在 545 行调用 PFN_PHYS 函数获得物理内存区域起始物理地址，然后在 549 行，函数首先获得物理偏移加上起始物理地址之后一个物理地址，然后将该物理地址按 align 的方式进行偏移，偏移之后的值再减去物理区域的起始物理地址，这样就可以获得对齐之后的物理地址偏移.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30012">alloc_bootmem_core</span>

![](/assets/PDB/HK/HK000510.png)

alloc_bootmem_core() 函数的作用是从 bootmem 分配器中分配指定长度的物理内存，并返回对应的虚拟地址。alloc_bootmem_core() 函数是 bootmem 分配器分配的核心实现。bdata 指向一个物理内存区域, size 指明需要分配物理内存的长度，align 分配时的对齐方式，goal 参数指明期望从指定的位置开始分配，limit 参数指明最大可分配的极限.

函数在 563 - 565 行对参数进行检测，确保分配的长度值有效，对齐方式不越界，以及 goal 加上 size 的值没有超过当前物理内存区域的限制。函数在 567 行检测以确保当前的物理内存区域的 bitmap 存在.

![](/assets/PDB/HK/HK000511.png)

函数在 570 行获得该物理内存区域的起始页帧号，571 行则获得该物理内存区域的最大物理页帧号。函数在 573 - 574 行将 goal 参数和 limit 参数都按 PAGE_SHIFT 向右移位，以此获得对应的页帧偏移。函数在 576 - 577 行检测了 limit 是否越界，以及当前物理区域最大页帧号是否超过的 limit 限制，如果超过，那么将 max 设置为最大限制值。函数在 578 - 579 行对 max 和 min 值进行检测，以此判断 max 是否越界。检测完毕之后，函数接着在 581 行通过 max() 函数，在参数 align 和 1UL 之间选择最大的对齐方式。函数在 583 - 586 行如果判断到 goal 存在，且 goal 在 min 和 max 之间，那么 goal 值有效，bootmem 分配器可以从 goal 处开始分配，那么将 start 指向按 step 对齐之后的 goal 处; 反之那么 bootmem 分配器只能从 min 处开始分配，那么将 start 指向按 step 对齐之后的 min 处. 函数在 588 行出计算了 start 在 bitmap 中的索引值，以及计算了当前物理内存区域的 bitmap 支持的最大索引值。 

![](/assets/PDB/HK/HK000512.png)

经过上面的代码处理之后，bootmem 分配器针对当前的物理内存区域布局如上图。从上图可以看到 sidx 和 midx 已经在 bitmap 中指定了一块可以分配的区间，接下来 bootmem 分配器将从 sidx 处开始实际的分配操作.

![](/assets/PDB/HK/HK000513.png)

函数此时检测 sidx 索引值是否小于当前物理内存区域上一次分配的结束索引值，如果小于，那么函数将在 597 行将 sidx 调整为上一次分配结束的索引值。因此可以看出 bootmem 分配器优先从上一次分配之后开始进行查找，而不是重头开始找。

![](/assets/PDB/HK/HK000514.png)

函数开始实际的分配操作，首先在 605 行，函数通过调用 find_next_zero_bit() 函数从当前物理内存区对应的 bitmap 的 sidx 处开始到 midx 结束的范围内找出第一个清零的位，将找到的位置存储在 sidx 变量里，接着函数在 606 行将找到的 sidx 值进行对齐，然后将 sidx 的值加上 size 之后以便获得此次分配的范围。此时通过 bitmap 找到第一个空间为 sidx 到 eidx，接着函数在 609 行进行检测，如果 sidx 大于当前 bitmap 支持的最大索引值，即 bitmap 在指定范围内没有空闲的 bit，或者 eidx 大于 midx，即 bitmap 在指定范围内没有指定数量的空闲 bit，那么函数跳出循环; 如果没有跳出循环，那么说明在 bitmap 中找到了一块合适的区域, 如下图 "range" 部分:

![](/assets/PDB/HK/HK000515.png)

函数在找到一块 "可用的范围" 之后，在 612 - 618 行，函数使用 for 循环以此遍历范围内的每一个 bit，并调用 test_bit() 函数检测遍历到的 bit 是否已经被 "使用" (置位)，如果已经置位，那么函数将 sidx 重定位到置位的位置，重定位时采用将遍历到的位置按 step 对齐后的位置，如果此时对齐后的位置与遍历的位置重合，那么需要将 sidx 重新定位到下一个对齐点上，直接将 sidx 加上对齐长度，最后跳转到 find_block 处重新查找; 放置如果该区域的 bit 全部 "可用" (清零)，那么函数继续执行接下来的逻辑.

![](/assets/PDB/HK/HK000517.png)

函数在 620 行进行判断，如果此时物理内存区域的 last_end_off 存在，即上一次分配时对应的结束物理地址，如果该结束物理地址没有按页对齐，且其下一个物理页帧就是 sidx，那么函数将 start_off 指向 last_end_off 按 align 对齐后的结果; 反之则将 start_off 指向了 sidx 对应的物理地址. 函数接着在 626 行进行检测，如果此时 start_off 对应的页帧号小于 sidx，那么 merge 设置为 true, 反之为 false。接着函数在 627 行将 end_off 指向 start_off 加上 size 之后的物理地址，至此已经获得一段可用的物理内存区域。函数在 629 行将 end_off 对应的结束物理地址存储在物理内存区域的 last_end_off,以此表示上一次分配的结束物理地址，接着在 630 行将 end_off 对应的物理页帧号存储在物理内存区域的 hint_idx 成员里，以此表示上一次分配的结束物理地址在 bitmap 中的索引。

![](/assets/PDB/HK/HK000518.png)

查找到合适的物理内存区域之后，函数调用 \_\_reserve() 函数将对应的物理内存区域进行预留，以此表示这些物理内存区域已经从 bootmem 分配器中分配出去。函数接着在 639 行调用 phys_to_virt() 和 PFN_PHYS() 函数获得对应区域的起始虚拟地址，并存储在 region 中，并调用 memset() 函数将 region 到 region+size 的内存空间清零，最终将虚拟地址返回给调用者.

![](/assets/PDB/HK/HK000519.png)

如果函数没有在指定物理内存区域的 bitmap 中找到可用的区域，那么函数跳出 while() 循环，函数在 650 行检测 fallback 是否为真，如果为真，那么函数从 bitmap 的 fallback 处重新进行查找。如果实在找不到，那么函数最终返回 NULL.

> [bootmem_data_t/struct bootmem_data 详解](#D200)
>
> [align_idx 详解](#D30011)
>
> [align_off 详解](#D30013)
>
> [\_\_reserve 详解](#D30007)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30011">align_idx</span>

![](/assets/PDB/HK/HK000509.png)

align_idx() 函数用于对索引进行对齐操作。操作 bdata 指向 bootmem 的指定物理区间，参数 idx 为该物理区间对应的 bitmap 的索引，参数 step 为对齐方式。

函数在 532 行获得该物理内存区域的起始物理页帧号，然后通过起始页帧号加上索引值获得索引对应的物理页帧号，并将其按 step 参数进行对齐，对齐之后的结果减去物理区间的起始值，这样就获得一个对齐之后的索引值.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30010">reserve_bootmem_node</span>

![](/assets/PDB/HK/HK000508.png)

reserve_bootmem_node() 函数用于将指定 NUMA NODE 上的一段物理内存标记为预留。参数 pgdat 指向 NUMA NODE，参数 physaddr 和 size 指明物理内存的范围，flags 指明了标记时的参数.

函数在 495 - 496 行计算出物理页对应的物理页帧号，并调用 mark_bootmem_node() 函数进行实际的标记工作，经过 mark_bootmem_node() 的标记，bootmem 内存分配器可能预留这段物理内存。

> [makr_bootmem_node 详解](#D30008)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30009">free_bootmem_node</span>

![](/assets/PDB/HK/HK000507.png)

free_bootmem_node() 函数用于将指定 NUMA NODE 的一段物理页标记为可用状态. 参数 pgdat 指向指定的 NUMA NODE。physaddr 和 size 参数指定了需要标记物理页的范围.

函数在 443 - 444 行计算出物理页对应的物理页帧信息，然后在 446 行调用 mark_bootmem_node() 函数进行实际的标记操作. 标记完毕之后，这段物理内存在 bootmem 分配器中可用.

> [makr_bootmem_node 详解](#D30008)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30008">makr_bootmem_node</span>

![](/assets/PDB/HK/HK000506.png)

makr_bootmem_node() 函数的作用是标记一段 bootmem 内存分配器的物理内存为可用或者预留. 参数 bdata 指向指定的物理内存区域，start 和 end 参数用于指明需要标记的区间范围，reserve 参数用于指明标记为预留还是可用，flags 参数则指明标记时候的标志.

函数在 377-378 行首先确认需要标记的区间位于 bdata 维护的区间范围内，接着函数在 380-381 行分别计算出需要标记的区间在 bdata 对应 bitmap 中的索引位置。接着函数在 383 行判断 reserve 的值，如果为真，那么需要将区域标记为预留，如果为假，则将区域标记为可用。当标记为预留的时候，函数调用 \_\_reserve() 函数进行实际的预留操作; 当标记为可用的时候，函数调用 \_\_free() 函数进行实际的释放操作.

> [\_\_free 详解](#D30005)
>
> [\_\_reserve 详解](#D30007)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30007">\_\_reserve</span>

![](/assets/PDB/HK/HK000505.png)

\_\_reserve() 函数用于将 bootmem_data_t 指向的一段物理内存在 bitmap 中的所有 bit 都置位，以此让 bootmem 分配器知道这段物理内存已经被预留. 参数 bdata 指向 bootmem 分配器维护的一段物理内存区域，参数 sidx 和 eidx 指明需要预留的物理内存在 bitmap 中的位置， flags 参数用于指定预留的标志。

函数在 348 行从 flags 参数中提取 BOOTMEM_EXCLUSIVE 标志信息，并存储到 exclusive 变量里。函数在 356 行调用 for 循环以 sidx 为起始索引，eidx 为终止索引遍历 bdata 对应的 bitmap 空间，函数在 357 行调用 test_and_set_bit() 函数将对应的 bit 都置位，但如果此时 exclusive 变量为真，那么函数调用 \_\_free() 函数将已经置位的 bit 又清零，并返回 EBUSY 错误，以此告诉 bootmem 分配器，独享的物理内存不能预留; 反之如果 exclusive 为假，那么函数会将区间内的所有 bits 都置位，以此预留这段物理内存.

> [\_\_free 详解](#D30005)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30005">\_\_free</span>

![](/assets/PDB/HK/HK000504.png)

\_\_free() 函数用于将 bootmem_data_t 指向的一段物理内存在 bitmap 中的所有 bit 都清零，以此让 bootmem 分配器知道这段物理内存是空闲可用的. 参数 bdata 指向 bootmem 分配器维护的一段物理内存区域，参数 sidx 和 edix 指向在 bitmap 需要清零的范围.

函数在 336 - 337 行判断该区域的 hint_idx 是否已经超过了 sidx 参数，如果超过将 hint_idx 设置为 sidx. 函数在 339 行调用 for 循环，从 sidx 索引开始，到 eidx 索引结束，调用 test_and_clear_bit() 函数将遍历到的 bit 在 bdata 对应的 bitmap 中清零。如果清零过程中遇到失败的情况，那么函数调用 BUG() 函数报错. 清零完毕之后。bit 对应的物理页帧在 Bootmem 分配器中就可以使用.

> [bootmem_data_t/struct bootmem_data 详解](#D200)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30004">init_bootmem_node</span>

![](/assets/PDB/HK/HK000503.png)

init_bootmem_node() 函数的作用是向 bootmem 内存分配中注册一段物理内存区域。参数 pgdat 指向系统的 pg_data_t 数据结构，freepfn 用于指向一块可用的内存区域，参数 start_pfn 和 endpfn 用于指定一段物理内存区域. 函数通过调用 init_bootmem_core() 函数进行实际的插入工作。

> [init_bootmem_core 详解](#D30003)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30003">init_bootmem_core</span>

![](/assets/PDB/HK/HK000502.png)

init_bootmem_core() 函数的作用是初始化用于维护某段物理内存区域的 bootmem_data_t 元数据。参数 bdata 对应 bootmem_data_t 数据结构，参数 mapstart 用于指向存储 bitmap 的物理页帧号，参数 start 指向某段物理内存区域的起始地址，参数 end 指向某段物理内存区域的终止地址。

函数在 102 行将 bdata 的 node_bootmem_map 指向了系统分配给 bitmap 的虚拟地址，此处的 bitmap 用于管理参数 start 到 end 之间的物理内存区域，期间每个物理页占用 bitmap 的一个 bit。函数在 103 和 104 行将物理内存的起始地址和结束地址分别存储在了 bootmem_data_t 的 node_min_pfn 和 node_low_pfn 成员里. 接着函数在 105 行调用 link_bootmem() 函数将参数 bdata 对应的物理内存区域插入到 bootmem 分配器的 bdata_list 链表里。函数在 111 行调用 bootmap_bytes() 函数计算出参数 start 到 end 之间占用的物理内存以 1 个 bit 为单位，总共占用了多少个 bytes 的数据。计算完毕之后，函数在 112 行将该物理内存区域对应的 bitmap 全部 bit 设置为 1，以此先将该物理内存区域的页帧在 bootmem 内存分配器中进行预留. 最后函数返回了该物理内存区域占用 bitmap 的长度.

> [bootmap_bytes 详解](#D30000)
>
> [link_bootmem 详解](#D30002)
>
> [bootmem_data_t/struct bootmem_data 详解](#D200)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D100">bdata_list</span>

{% highlight bash %}
static struct list_head bdata_list __initdata = LIST_HEAD_INIT(bdata_list);
{% endhighlight %}

bdata_list 定义为一个双链表的数据结构，用于管理 Bootmem 分配器的全部 bootmem_data_t 数据。在 Bootmem 分配器中，包含了很多物理内存区域，每个物理内存区域使用一个 bootmem_data_t 数据结构进行维护。Bootmem 分配器使用 bdata_list 双链表将所有的物理内存区域都维护起来，并按物理内存区域的起始物理页帧从小到大的顺序在 bdata_list 双链表中进行维护.

> [bootmem_data_t/struct bootmem_data 详解](#200)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D200">bootmem_data_t/struct bootmem_data</span>

![](/assets/PDB/HK/HK000520.png)

![](/assets/PDB/HK/HK000501.png)

在内核初始化阶段，物理内存可能是一整块的物理内存，也可能是分离的多块物理内存，Botomem 分配器为了统一管理这些物理内存区域，那么使用了 bootmem_data_t 数据结构，该数据结构用于管理一块物理内存区域，有了 bootmem_data_t 数据结构，bootmem 分配器基于这些物理内存区域进行构建和管理该阶段的所有物理内存。

* node_min_pfn 成员用于表示该物理内存区域的起始物理页帧号
* node_low_pfn 成员用于表示该物理内存区域的终止物理页帧号
* node_bootmem_map 成员用于指向物理内存区域对应的 bitmap
* last_end_off 成员用于指明上一次分配的结束物理地址
* hint_idx 成员用于指明上一次分配的结束地址在 bitmap 中的索引值.
* list 成员则用于插入到 bootmem 分配器的 bdata_list 链表上


![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30003">link_bootmem</span>

![](/assets/PDB/HK/HK000500.png)

link_bootmem() 函数的作用是将一个新的 bootmem_data_t 数据结构按一定的顺序插入到 bdata_list 链表里。参数指向一个 bootmem_data_t 数据结构。在 bootmem 分配器中，一段物理内存区域使用 bootmem_data_t 进行维护，函数在 83 行通过调用 list_for_each() 函数遍历 bdata_list 链表上的所有 bootmem_data_t 数据结构，并在 86 行处调用 list_entry() 函数获得链表节点对应的 bootmem_data_t 数据结构，此时通过对比遍历到的 bootmem_data_t 对应的 node_min_pfn 是否比参数 bdata 对应的 node_min_pfn 大，如果大就直接 break 出循环，这样做的目的能让 bdata 参数对应的物理内存区域按从小到大的顺序插入到 bdata_list 链表中。最后函数在 90 行处调用 list_add_tail() 函数将参数 bdata 对应的数据结构插入到了 bdata_list 链表里.

> [bootmem_data_t/struct bootmem_data 详解](#200)
>
> [bdata_list 详解](#D100)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30001">bootmap_bootmap_pages</span>

![](/assets/PDB/HK/HK000499.png)

bootmap_bootmap_pages() 函数用于计算指定数量的物理页在 Bootmem 分配器中使用 bitmap 进行管理需要使用多少的物理页。 参数 pages 指明需要计算的物理页数量。函数首先通过调用 bootmap_bytes() 计算出指定数量的物理页需要使用多少个 byte 进行管理，然后计算这些 byte 占用了多少个物理页的内存.

> [bootmap_bytes 详解](#D30000)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D30000">bootmap_bytes</span>

![](/assets/PDB/HK/HK000498.png)

bootmap_bytes() 函数用于计算指定数量的 pages 至少需要多少个字节，并返回按 long 对齐的结果。参数 pages 指明需要计算的物理页数量。在 Bootmem 分配器中，其使用 bitmap 管理所有的物理页，且一个物理页在 bitmap 中占用一个 bit。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F03"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

#### Bootmem 内存分配器与 CMDLINE 研究

在 Linux 的 CMDLINE 机制中，Linux 提供了 "mem=" 字段用于传递物理内存布局信息。内核初始化阶段，内核可以通过 E820 (X86/X64) 或者 ATAG (ARM) 中获得物理内存布局信息，这些信息用于构建 Bootmem 分配器。如果此时系统从 CMDLINE 中解析到 "mem=" 字段，那么系统优先采用 CMDLINE 中 "mem=" 提供的物理内存布局信息作为唯一的信息，且传递给 Bootmem 分配器用于其初始化，因此 CMDLINE 中 "mem=" 字段会影响`Bootmem 内存分配器的行为，"mem=" 的使用如下:

{% highlight bash %}
mem=size@addr
{% endhighlight %}

addr 参数指定一段物理内存区域的起始物理地址，size 参数指明这段物理内存区域的长度，内核支持 CMDLINE 中同时存在多个 "mem=" 字段，内核解析 "mem=" 字段的逻辑如下:

![](/assets/PDB/HK/HK000545.png)

CMDLINE 中的 “mem=” 参数用于设置系统可用物理内存范围，其通过设置内存的起始地址和内存长度来进行设置。如果 CMDLINE 中包含了该字段，那么函数 early_mem() 函数将通过 early_param() 函数调用。函数首先在 433 行进行判断，如果系统支持 CMDLINE 中的 “mem=” 字段，那么内核将不再使用 ATAG 中传递的内存信息，而是只采用 CMDLINE 传递的内存信息作为系统内存信息。函数首先判断 usermem 的值是否为 0，也就代码了该 “mem=” 信息是否第一次使用，因为在 CMDLINE 中 “mem=” 的字段是可以多次使用，因此如果 “mem=” 字段是第一次解析，那么函数将 usermem 设置为 1，并将 meminfo.nr_banks 的值设置为 1，这样的操作直接将从 ATAG 获得的内存信息直接丢弃. 函数在 438 - 441 行将 “mem=” 字段中的内存信息解析出来，然后将其传递给 arm_add_memory() 函数将对应的内存区域信息添加到 meminfo 数据结构中。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### 实践验证 [ARM]

基于本文实践章节，在 ARM 架构中, 内核可以通过 ATAG 机制从 Uboot 中获得物理内存的信息，也可以从 CMDLINE 中获得物理内存的信息，因此当使用 CMDLINE 获得物理内存信息时，内核会丢弃从 ATAG 中获得物理内存信息，而采用 CMDLINE 中获得的物理内存信息。基于上面的结论，在 RunBiscuitOS.sh 文件中的 CMDLINE 里添加如下参数:

{% highlight bash %}
CMDLINE="earlycon root=/dev/ram0 rw rootfstype=${FS_TYPE} console=ttyAMA0 init=/linuxrc loglevel=8 mem=96M@0 mem=96M@0x8000000"
{% endhighlight %}

![](/assets/PDB/HK/HK000563.png)

在当前系统中，真实的物理内存从 0x00000000 到 0x10000000, 总共 256 MiB，基于上面的修改，系统可见的物理内存将变成 "0x00000000 - 0x6000000" 与 "0x800000 - 0xe000000" 两端物理内存区域，那么 "0x6000000 - 0x8000000" 则变为内存 Hole，系统无法使用这段内存区域。在 BiscuitOS 上运行的实际效果如下图:

![](/assets/PDB/HK/HK000562.png)

从实际运行情况来看，系统识别两块物理内存，其中第一块物理内存的范围从 0x00000000 到 0x05FFFFFF, 其长度为 96 MiB，第二块的范围从 0x08000000 到 0x0dFFFFFF, 其长度为 96 MiB。然而 0x06000000 到 0x07FFFFFF 之间的内存区域系统无法感知到，因此这段区域称为了 "Hole", 而 Uboot 通过 ATAG 传递的内存信息指出，真实的物理内存范围是从 0x00000000 到 0x10000000. 从上面的运行结果可以得出，在 ARM 架构中，内核优先采用 CMDLINE 传递的物理内存信息，如果 CMDLINE 没有传递内存相关的信息，那么内核采用 Uboot 传递的内存信息。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### CMDLINE 的用途

通过上面的实践分析，CMDLINE 传递物理内存信息的用途可以总结为一下:

###### 建立系统预留区

CMDLINE 中的 "mem=" 字段可以传递一块或者多块可用的物理内存区域，那么可以利用这个特性跳过特定区域，这些被跳过的区域系统物理识别和使用，因此形成了内存空洞，即 "Memory Hole"。Hole 的存在满足一些特定功能的需求，例如需要申请一块独立且内存无法使用的内存供特定模块或者功能使用; 又或者说将一块物理内存预留起来，等某个时刻或某个任务到来之后，内核需要将这块预留的内存进行热插拔机制插入到内核，以便动态扩充系统内存的长度。

CMDLINE 机制中，被 "mem=" 字段跳过的物理内存区域不能在 Bootmem 分配器中使用，这段区域在 Bootmem 分配器中一直保持预留，因此这段内存区域不会转移到 Buddy 内存分配器，最终内核无法使用这段物理内存。

###### 布局系统物理内存

CMDLINE 中的 "mem=" 字段最原始的用法，用于向系统传递一块或多块可用物理内存系统，内核根据这些信息构建 Bootmem 分配器，而且通过 CMDLINE "mem=" 传递物理内存信息机制优先级高于其他机制，因此可以忽略不同平台上五花八门的物理内存布局信息获取方式，采用内核提供的统一方式即可设置系统可用物理内存。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F02"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

#### Bootmem 内存分配器分配的粒度研究

Bootmem 分配器提供了一套完整的接口用于分配物理内存，这些接口可以分配不同粒度的物理内存，比如按字节进行分配，或者按页进行分配，因此本文研究的核心是 Bootmem 如何支持不同粒度的物理内存分配和回收。

> [小粒度内存分配](#F020)
>
> [大粒度内存分配](#F021)
>
> [最小粒度内存分配](#F022)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="F020">小粒度内存分配</span>

小粒度的内存分配一般指内存长度小于 PAGE_SIZE 的分配，这类型的分配用于给一些数据结构分配内存。首先基于这个问题做一些实践，实践办法参照如下文档, 使用如下测试代码:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
	char *buffer = NULL;
	int size = 0x20;
	int count = 20;

	while (count--) {
		/* Alloc memory from Bootmem */
		buffer = (char *)alloc_bootmem(size);
		if (!buffer) {
			printk("ERROR: Alloc memory failed.\n");
			return -ENOMEM;
		}

		/* Dup Physical Address */
		printk("Physial-Address: %#lx\n", __pa(buffer));
	}

	return 0;
}
{% endhighlight %}

将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中运行，运行的情况如下:

![](/assets/PDB/HK/HK000564.png)

从实际的运行结果可以看出，物理地址之间的距离都是 0x20，这与预期期盼相符，因此可以得出 Bootmem 分配器支持小粒度的内存分配。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

###### <span id="F021">大粒度内存分配</span>

大粒度的内存分配一般指内存长度大于 PAGE_SIZE 的内存分配。接下来继续使用代码进行测试，这次从 Bootmem 分配器中按物理页进行分配，实践办法参照如下文档，使用如下测试代码:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer = NULL;
        int size = 0x20 * PAGE_SIZE;
	int count = 5;

	while (count--) {
		/* Alloc memory from Bootmem */
		buffer = (char *)alloc_bootmem_pages(size);
		if (!buffer) {
			printk("ERROR: Alloc memory failed.\n");
			return -ENOMEM;
		}
		/* Dup Physical Address */
		printk("Physical-Address: %#lx\n", __pa(buffer));
	}

        return 0;
}
{% endhighlight %}

将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中
运行，运行的情况如下:

![](/assets/PDB/HK/HK000565.png)

从 BiscuitOS 中实际运行的结果可以看出，Bootmem 分配器总是可以分配出长度为 "0x20 * PAGE_SIZE" 的物理内存，并且物理地址之间的间隔也是 0x20000. 因此可以得出的结论是 Bootmem 内存分配器支持大粒度的内存分配。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------

###### <span id="F022">最小粒度内存分配</span>

Bootmem 分配器支持小粒度的内存分配，那么 Bootmem 支持多小的内存分配呢？首先最小粒度这里指的是一个字节，小于一个字节的分配无研究意义。接着使用 Bootmem 内存分配器分配一个字节，如果分配成功，那么 Bootmem 分配器可以分配最小粒度认为是一个字节; 反之如果失败，那么 Bootmem 分配器支持的最小分配粒度是多少呢？ 接下来继续使用代码进行测试，这次从 Bootmem 分配器中连续分配一个字节，实践办法参照如下文档，使用如下测试代码:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer = NULL;
        int size = 1;
        int count = 20;

        while (count--) {
                /* Alloc memory from Bootmem */
                buffer = (char *)__alloc_bootmem(size, 1, 0);
                if (!buffer) {
                        printk("ERROR: Alloc memory failed.\n");
                        return -ENOMEM;
                }

                /* Dup Physical Address */
                printk("Physial-Address: %#lx\n", __pa(buffer));
        }

        return 0;
}
{% endhighlight %}

将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中
运行，运行的情况如下:

![](/assets/PDB/HK/HK000566.png)

从运行的结果可以看出，Bootmem 分配器是支持一个字节的内存分配。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F05"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000D.jpg)

#### Bootmem 内存分配器分配策略研究

Bootmem 分配器主要用于 boot-time 阶段内存分配，这个阶段系统大部分功能都未初始化，Bootmem 分配器为这个阶段的功能模块的初始化提供了基础的内存分配回收，当内核初始化到一定阶段之后，Bootmem 分配器就会将所管理的物理内存转移给 Buddy 分配器，待转移完毕之后，Buddy 分配器正式接管物理内存，Bootmem 分配器完成自己的使命。boot-time 阶段消耗内存比较少，在 i386 架构上，Bootmem 分配器甚至只管理低端物理内存，而在有的架构中则低端和高端物理内存都管理，并且 Bootmem 分配器提供不同粒度的内存分配，因此本节用于研究 Bootmem 分配器的分配策略/逻辑.

> [> 回旋式分配/递增式分配](#F033)
>
> [> Bootmem 分配器分配策略 0](#F036)
>
> [> Bootmem 分配器分配策略 1](#F037)
>
> [> Bootmem 分配器分配策略 2](#F038)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

###### <span id="F033">回旋式分配/递增式分配</span>

所谓回旋式分配指的是刚从 Bootmem 分配器中分配一段物理内存，然后直接释放掉这段物理内存，接着又分配同样长度的物理内存，一直循环下去; 递增式分配指刚从 Bootmem 分配器中分配物理内存，然后直接释放，接着又从 Bootmem 内存分配器中分配同样长度的物理内存。如果 Bootmem 分配器存在回旋分配，那么分配到的地址都是同一个物理地址; 反之如果 Bootmem 分配器存在递增式分配，那么分配到的物理地址都是新的物理地址. 接下来继续使用代码进行测试，这次从 Bootmem 分配器中重复分配内存又释放内存，实践办法参照如下文档，使用如下测试代码:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer = NULL;
        int size = 0x20;
        int count = 20;

        while (count--) {
                /* Alloc memory from Bootmem */
                buffer = (char *)alloc_bootmem(size);
                if (!buffer) {
                        printk("ERROR: Alloc memory failed.\n");
                        return -ENOMEM;
                }

                /* Dup Physical Address */
                printk("Physial-Address: %#lx\n", __pa(buffer));

		/* Free memory */
		free_bootmem(__pa(buffer), size);
        }

        return 0;
}
{% endhighlight %}

将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中
运行，运行的情况如下:

![](/assets/PDB/HK/HK000567.png)

从运行的结果可以看出，新分配的物理地址都是新的地址，而且都是递增的，因此 Bootmem 分配器不存在回旋式分配机制，而是存在递增式分配机制.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="F036">Bootmem 分配器分配策略 0</span>

Bootmem 分配器分配的核心代码是 "alloc_bootmem_core()" 函数，其函数逻辑分析如下:

> [alloc_bootmem_core() 函数详解](#D30012)

只有当一个物理页被全部分配了，这个物理页才会在 Bootmem 内存分配器中标记为已分配，否则一直都是可用状态. 针对第一个结论，可以使用如下代码进行验证, 实践办法参照如下文档，使用如下测试代码:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer = NULL;
        int size = 0x200;
        int count = 10;

        while (count--) {
		pg_data_t *pgdat = NODE_DATA(0);
		struct bootmem_data *bdata = pgdat->bdata;
		unsigned long *bitmap = bdata->node_bootmem_map;
		unsigned long sidx, midx;

                /* Alloc memory from Bootmem */
                buffer = (char *)alloc_bootmem_node(pgdat, size);
                if (!buffer) {
                        printk("ERROR: Alloc memory failed.\n");
                        return -ENOMEM;
                }

                /* Dup Physical Address */
                printk("Physial-Address: %#lx\n", __pa(buffer));
		/* Check Bitmap */
		sidx = PFN_DOWN(__pa(buffer)) - bdata->node_min_pfn;
		midx = bdata->node_low_pfn - bdata->node_min_pfn;
		printk("ZeroBit: %#lx\n", 
				find_next_zero_bit(bitmap, midx, sidx));

                /* Don't Free memory */
        }

        return 0;
}
{% endhighlight %}

将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中运行，运行的情况如下:

![](/assets/PDB/HK/HK000568.png)

从运行结果可以来看, 当 0x51e000 这个物理页没有分配完，那么 Bootmem 分配器就一直从这个物理页分配，这个物理页对应的 bit 一直处于清零状态; 当分配到 0x51ef40 的时候，如果在从这个物理页分配 0x200 的物理内存时，该物理页已经没有剩余这么多的物理内存，那么函数将一部分从这个物理页中分配，剩余部分从下一个物理页中分配，并将分配满的物理页对应的 bit 标记置位，因此下一个分配的物理地址是 0x51f140, 且下一个可用的清零 bit 为 0x520. 以上的实践和结论符合预期。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------------------

###### <span id="F037">Bootmem 分配器分配策略 1</span>

Bootmem 分配器分配的核心代码是 "alloc_bootmem_core()" 函数，其函数逻辑分析如下:

> [alloc_bootmem_core() 函数详解](#D30012)

如果一次分配因为对齐原因跳过了当前可分配的物理页，那么下一次分配时，Bootmem 分配器会从下一个新的物理内存页中分配内存. 对于这个结论，可以使用如下代码进行验证, 实践办法参照如下文档，使用如下测试代码:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer = NULL;
        int size = 0x20;
	int page_size = 0x200;
        int count = 10;

        while (count--) {
                pg_data_t *pgdat = NODE_DATA(0);
                struct bootmem_data *bdata = pgdat->bdata;
                unsigned long *bitmap = bdata->node_bootmem_map;
                unsigned long sidx, midx;

                /* Alloc memory from Bootmem */
		if (count % 2)
                	buffer = (char *)alloc_bootmem_node(pgdat, size);
		else
                	buffer = (char *)alloc_bootmem_pages_node(pgdat, page_size);
                if (!buffer) {
                        printk("ERROR: Alloc memory failed.\n");
                        return -ENOMEM;
                }

                /* Dup Physical Address */
                printk("Physial-Address: %#lx\n", __pa(buffer));
                /* Check Bitmap */
                sidx = PFN_DOWN(__pa(buffer)) - bdata->node_min_pfn;
                midx = bdata->node_low_pfn - bdata->node_min_pfn;
                printk("ZeroBit: %#lx\n",
                                find_next_zero_bit(bitmap, midx, sidx));

                /* Don't Free memory */
        }

        return 0;
}
{% endhighlight %}

本例子的代码逻辑很简单，就是交替从 Bootmem 分配器中分配物理内存，第一次分配 0x20，第二次按物理页对齐分配 0x200，依次交替进行分配。每次分配完毕之后，函数都会获得当前 Bootmem 分配器中下一个可用物理页在 bitmap 中的索引。将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中
运行，运行的情况如下:

![](/assets/PDB/HK/HK000569.png)

从实际的运行结果可以看出，当第一次从 Bootmem 中分配 0x20 的物理内存，Bootmem 分配器分配物理地址是 0x51e140, 第二次则按页对齐之后进行分配，分配的长度为 0x200, 其地址为 0x51f000, 符合要求，第三次进行分配时，由于上一次分配的结束地址是 0x51f200, 因此 Bootmem 分配器从这个地址开始分配，那么第三次分配到的物理内存地址就是 0x51f200, 那么 Bootmem 分配器就跳过了第一次分配剩余的物理内存。通过分析，符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

###### <span id="F038">Bootmem 分配器分配策略 2</span>

Bootmem 分配器分配的核心代码是 "alloc_bootmem_core()" 函数，其函数逻辑分析如下:

> [alloc_bootmem_core() 函数详解](#D30012)

Bootmem 内存分配器只能增量式分配，不支持迂回式分配. 对于这个结论，可以使用如下代码进行验证, 实践办法参照如下文档，使用如下测试代码:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer;
        int size = 0x20;
	unsigned long range_start = 0x9200000;

        /* Alloc memory from Special memory area */
        buffer = (char *)__alloc_bootmem(size, PAGE_SIZE, range_start); 
        if (!buffer) {
                printk("ERROR: Can't alloc memory from %#lx.\n", range_start);
                return -ENOMEM;
        }

	/* Dup Physical Address */
	printk("Physical-Address: %#lx\n", __pa(buffer));

	range_start = 0x5000000;
        /* Alloc memory from Special memory area */
        buffer = (char *)__alloc_bootmem(size, PAGE_SIZE, range_start); 
        if (!buffer) {
                printk("ERROR: Can't alloc memory from %#lx.\n", range_start);
                return -ENOMEM;
        }

	/* Dup Physical Address */
	printk("Physical-Address: %#lx\n", __pa(buffer));

	range_start = 0xFFFFF00;
        /* Alloc memory from Special memory area */
        buffer = (char *)__alloc_bootmem(size, 2 * PAGE_SIZE, range_start); 
        if (!buffer) {
                printk("ERROR: Can't alloc memory from %#lx.\n", range_start);
                return -ENOMEM;
        }

	/* Dup Physical Address */
	printk("Physical-Address: %#lx\n", __pa(buffer));

        return 0;
}
{% endhighlight %}

本例子的代码逻辑很简单，第一次分配让 Bootmem 分配器从 0x92000000 开始分配 0x20 的物理内存，第二次则让 Bootmem 分配器从 0x5000000 处开始分配问题内存，对于第三次，假定当前物理地址最大值是 0x10000000, 那么从 0xFFFFF00 之后分配两个物理页，以便观察 Bootmem 分配器会不会从物理内存区域的起始地址开始分配，都查看三次分配之后的物理内存地址。将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中运行，运行的情况如下:

![](/assets/PDB/HK/HK000570.png)

从实际的运行结果可以看出，第一次分配从指定的 0x9200000 分配物理内存，但第二次分配则从 0x9201000 处分配，这正好是第一次分配之后按 PAGE_SIZE 对齐的地址，因此 Bootmem 内存分配器只能从上一次分配之后继续分配, 因此分配的物理地址就是 0x9201000. 对应第三次分配，系统物理内存最大物理地址是 0x10000000, 但从 0xFFFFF00 之后分配 2 个物理页之后已经超过最大物理内存地址了，但 Bootmem 分配器没有采取迂回的方式从物理内存区域的起始地址开始分配，而是直接从上一次分配的地方开始分配，于是第三次的分配物理地址为 0x9202000。结果符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F06"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

#### Bootmem 分配器回收逻辑研究

Bootmem 分配器也提供了内存回收的功能，并提供相应的接口便可回收。Bootmem 分配器回收的核心代码是 "free_bootmem_node()/free_bootmem()" 函数，两者的实现逻辑类似，其中 free_bootmem() 代码逻辑如下:

> [free_bootmem](#D30038)

由 free_bootmem() 函数的实现决定了 Bootmem 的回收逻辑以及策略，因此本节重点研究一下 Bootmem 分配器的回收策略:

> [> Bootmem 分配器无真正意义上的回收机制](#F061)
>
> [> Bootmem 分配器回收机制引发的问题](#F062)

---------------------------------

###### <span id="F061">Bootmem 分配器无真正意义上的回收机制</span>

Bootmem 分配器支持递增式分配方式，即当分配一块物理内存紧接着释放掉这块物理内存，那么再次分配物理内存的时候，Bootmem 不会再提供这块物理内存，而是提供下一块物理内存。为了验证这个结论，可以使用如下代码进行验证:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
	char *buffer = NULL;
	int size = 0x20;
	int count = 20;

	while (count--) {
		/* Alloc memory from Bootmem */
		buffer = (char *)alloc_bootmem(size);
		if (!buffer) {
			printk("ERROR: Alloc memory failed.\n");
			return -ENOMEM;
		}

		/* Dup Physical Address */
		printk("Physical-%d: %#lx\n", count, __pa(buffer));

		/* Free memory */
		free_bootmem(__pa(buffer), size);
	}

        return 0;
}
{% endhighlight %}

本例子的代码逻辑很简单，申请一个物理内存之后马上释放，再申请再释放，以此观察分配到的物理内存地址。将这段代码插入到 start_kernel() -> mm_init() 函数之前进行调用，并在 BiscuitOS 中运行，运行的情况如下:

![](/assets/PDB/HK/HK000582.png)

从实践结果来看，每次分配都是从新的一个物理地址上分配，而不从刚释放的地址上分配，这将导致一个问题，Bootmem 分配器的回收机制没有像正常的内存分配器一样具有回收功能，这也是因为 Bootmem 分配器任务的特殊，它只需提供 Buddy 分配器之前的物理内存分配，而且 boot-time 阶段分配内存比较少，Bootmem 分配器无需花太多的代码和精力去实现真正意义上的回收功能，其次大多是使用 Bootmem 分配器的预留功能，如果使用分配功能，分配之后的内存一直都使用，直到内核关机才不使用，因此这也是 Bootmem 分配器没有一个完善的回收功能。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

###### <span id="F062">Bootmem 分配器回收机制引发的问题</span>

通过上面一节的讨论，可以直到 Bootmem 分配器没有真正意义的回收机制，也就是回收的内存不能再次使用，并且 Bootmem 分配器分配时不能迂回分配，也就是当高地址的物理内存分配完毕之后，Bootmem 分配器不会掉头去分配低地址的物理内存，这是一个很值得注意的问题，那么先用一个实际例子作为讲解，代码如下:

> [Bootmem 内存分配器中分配一段物理内存](#B000)

{% highlight c %}
#include <linux/bootmem.h>

static int BiscuitOS_Demo(void)
{
        char *buffer;
        int size = 0x20;
        unsigned long top_address = 0x9200000 - 0x100;

        /* Alloc memory from Special memory area */
        buffer = (char *)__alloc_bootmem(size, PAGE_SIZE, top_address);
        if (!buffer) {
		printk("Can't alloc memory.\n");
               	return -ENOMEM;
       	}
	free_bootmem(__pa(buffer), size);

	/* Loop allocate and free */
	while (1) {
        	/* Alloc memory from Special memory area */
        	buffer = (char *)alloc_bootmem(size);
        	if (!buffer) {
			printk("Can't alloc memory.\n");
                	return -ENOMEM;
        	}

        	/* Dup Physical Address */
        	printk("Physical-Address: %#lx\n", __pa(buffer));

		/* Free Address */
		free_bootmem(__pa(buffer), size);
	}

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

#### Bootmem 分配器调试

内核 CMDLINE 提供了 "bootmem_debug" 字段用于用于开启 Bootmem 分配器的 debug 功能，开发者可以参考实践章节在 CMDLINE 中添加 "boootmem_debug" 字段进行调试。例如开启之后的效果如下:

![](/assets/PDB/HK/HK000583.png)

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
