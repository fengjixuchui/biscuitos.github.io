---
layout: post
title:  "memblock_find_in_range() 在指定的区间内查找一块可用的物理内存"
date:   2019-03-11 15:30:30 +0800
categories: [MMU]
excerpt: memblock_find_in_range() 在指定的区间内查找一块可用的物理内存.
tags:
  - MMU
---

> [GitHub: for_each_mem_range()](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_find_in_range)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> [原理](#原理)
>
> [源码分析](#源码分析)
>
> [实践](#实践)
>
> [附录](#附录)

---------------------------------------------------

# <span id="原理">原理</span>

#### MEMBLOCK 内存分配器原理

MEMBLOCK 内存分配器作为 arm32 早期的内存管理器，维护了两种内存。第一种内存是系统可用
的物理内存，即系统实际含有的物理内存，其值从 DTS 中进行配置，并通过 uboot 实际探测之
后传入到内核。第二种内存是内核预留给操作系统的内存，这部分内存作为特殊功能使用，不能作
为共享内存使用。MEMBLOCK 内存分配器基础框架如下：

{% highlight bash %}
MEMBLOCK


                                         struct memblock_region
                       struct            +------+------+--------+------+
                       memblock_type     |      |      |        |      |
                       +----------+      | Reg0 | Reg1 | ...    | Regn |
                       |          |      |      |      |        |      |
                       | regions -|----->+------+------+--------+------+
                       | cnt      |      [memblock_memory_init_regions]
                       |          |
 struct           o--->+----------+
 memblock         |
 +-----------+    |
 |           |    |
 | memory   -|----o
 | reserved -|----o
 |           |    |                      struct memblock_region
 +-----------+    |    struct            +------+------+--------+------+
                  |    memblock_type     |      |      |        |      |
                  o--->+----------+      | Reg0 | Reg1 | ...    | Regn |
                       |          |      |      |      |        |      |
                       | regions -|----->+------+------+--------+------+
                       | cnt      |      [memblock_reserved_init_regions]
                       |          |
                       +----------+
{% endhighlight %}

从上面的逻辑图可以知道，MEMBLOCK 分配器使用一个 struct memblock 结构维护着两种内存，
其中成员 memory 维护着可用物理内存区域；成员 reserved 维护着操作系统预留的内存区域。
每个区域使用数据结构 struct memblock_type 进行管理，其成员 regions 负责维护该类型内
存的所有内存区，每个内存区使用数据结构 struct memblock_region 进行维护。

MEMBLOCK 分配器的主体是使用数据结构 struct memblock 进行维护，定义如下：

{% highlight c %}
/**
* struct memblock - memblock allocator metadata
* @bottom_up: is bottom up direction?
* @current_limit: physical address of the current allocation limit
* @memory: usabe memory regions
* @reserved: reserved memory regions
* @physmem: all physical memory
*/
struct memblock {
        bool bottom_up;  /* is bottom up direction? */
        phys_addr_t current_limit;
        struct memblock_type memory;
        struct memblock_type reserved;
#ifdef CONFIG_HAVE_MEMBLOCK_PHYS_MAP
        struct memblock_type physmem;
#endif
};
{% endhighlight %}

从上面的数据可知，struct memblock 通过数据结构 struct memblock_type 维护了初期的两
种内存，可用物理内存维护在 memory 成员里，操作系统预留的内存维护在 reserved 里。
current_limit 成员用于指定当前 MEMBLOCK 分配器在上限。在 linux 5.0 arm32 中，内核
实例化了一个 struct memblock 结构，以供 MEMBLOCK 进行内存的管理，其定义如下：

{% highlight c %}
struct memblock memblock __initdata_memblock = {
        .memory.regions         = memblock_memory_init_regions,
        .memory.cnt             = 1,    /* empty dummy entry */
        .memory.max             = INIT_MEMBLOCK_REGIONS,
        .memory.name            = "memory",

        .reserved.regions       = memblock_reserved_init_regions,
        .reserved.cnt           = 1,    /* empty dummy entry */
        .reserved.max           = INIT_MEMBLOCK_RESERVED_REGIONS,
        .reserved.name          = "reserved",

#ifdef CONFIG_HAVE_MEMBLOCK_PHYS_MAP
        .physmem.regions        = memblock_physmem_init_regions,
        .physmem.cnt            = 1,    /* empty dummy entry */
        .physmem.max            = INIT_PHYSMEM_REGIONS,
        .physmem.name           = "physmem",
#endif

        .bottom_up              = false,
        .current_limit          = MEMBLOCK_ALLOC_ANYWHERE,
};
{% endhighlight %}

从上面的数据中可以看出，对于可用物理内存，其名字设定为 “memory”，初始状态系统下，可用
物理物理的所有内存区块都维护在 memblock_memory_init_regions 上，当前情况下，可用物
理内存区只包含一个内存区块，然而可用物理内存可管理 INIT_MEMBLOCK_REGIONS 个内存区
块；同理，对于预留内存，其名字设定为 “reserved”，初始状态下，预留物理内存的所有区块
都维护在 memblock_reserved_init_regions 上，当前情况下，预留物理内存区只包含一个内
存区块，然而预留内存区可以维护管理 INIT_MEMBLOCK_RESERVED_REGIONS 个内存区块。

MEMBLOCK 分配器中，使用数据结构 struct memblock_type 管理不同类型的内存，其定义
如下：

{% highlight c %}
/**
* struct memblock_type - collection of memory regions of certain type
* @cnt: number of regions
* @max: size of the allocated array
* @total_size: size of all regions
* @regions: array of regions
* @name: the memory type symbolic name
*/
struct memblock_type {
        unsigned long cnt;
        unsigned long max;
        phys_addr_t total_size;
        struct memblock_region *regions;
        char *name;
};
{% endhighlight %}

通过 struct memblock_type 结构，可以获得当前类型的物理内存管理内存区块的数量 cnt，
最大可管理内存区块的数量 max，已经管理内存区块的总体积 total_size，以及指向内存区块
的指针 regions

{% highlight bash %}
                  struct memblock_region
struct            +------+------+--------+------+
memblock_type     |      |      |        |      |
+----------+      | Reg0 | Reg1 | ...    | Regn |
|          |      |      |      |        |      |
| regions -|----->+------+------+--------+------+
| cnt      |      [memblock_memory_init_regions]
|          |
+----------+
{% endhighlight %}

数据结构 struct memblock_region 维护一块内存区块，其定义为：

{% highlight c %}
/**
* struct memblock_region - represents a memory region
* @base: physical address of the region
* @size: size of the region
* @flags: memory region attributes
* @nid: NUMA node id
*/
struct memblock_region {
        phys_addr_t base;
        phys_addr_t size;
        enum memblock_flags flags;
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
        int nid;
#endif
};
{% endhighlight %}

通过改数据结构，可以知道内存区块起始的物理地址和所占用的体积。内存区块被维护在不同的
struct memblock_type 的 regions 链表上，这是一个由数组构成的链表，链表通过每个区块
的基地址的大小，从小到大的排列。每个内存区块代表的内存区不能与本链表中的其他内存区块相
互重叠，可以相连。内核初始定义了两个内存区块数组，如下：

{% highlight c %}
static struct memblock_region memblock_memory_init_regions[INIT_MEMBLOCK_REGIONS] __initdata_memblock;
static struct memblock_region memblock_reserved_init_regions[INIT_MEMBLOCK_RESERVED_REGIONS] __initdata_memblock
{% endhighlight %}

#### MEMBLOCK 内存分配器提供的逻辑

MEMBLOCK 通过上面的数据结构管理 arm32 早期的物理内存，使操作系统能够分配或回收可用的
物理内存，也可以将指定的物理内存预留给操作系统。通过这样的逻辑操作，早期的物理的内存得
到有效的管理，防止内存泄露和内存分配失败等问题。

--------------------------------------------------

# <span id="源码分析">源码分析</span>

> Arch： arm32
>
> Version： Linux 5.x

函数： memblock_find_in_range()

功能： 在指定的区间内查找一块可用的物理内存

{% highlight bash %}
memblock_find_in_range
|
|---memblock_find_in_range_node
    |
    |---memblock_bottom_up
    |
    |---__memblock_find_range_bottom_up
    |   |
    |   |---for_each_mem_range
    |
    |---__memblock_find_range_top_down
       |
       |---for_each_mem_range_rev
{% endhighlight %}

##### memblock_find_in_range

{% highlight c %}
/**
 * memblock_find_in_range - find free area in given range
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 *
 * Find @size free area aligned to @align in the specified range.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
phys_addr_t __init_memblock memblock_find_in_range(phys_addr_t start,
					phys_addr_t end, phys_addr_t size,
					phys_addr_t align)
{
	phys_addr_t ret;
	enum memblock_flags flags = choose_memblock_flags();

again:
	ret = memblock_find_in_range_node(size, align, start, end,
					    NUMA_NO_NODE, flags);

	if (!ret && (flags & MEMBLOCK_MIRROR)) {
		pr_warn("Could not allocate %pap bytes of mirrored memory\n",
			&size);
		flags &= ~MEMBLOCK_MIRROR;
		goto again;
	}

	return ret;
}
{% endhighlight %}

参数 start 查找区域的起始地址；参数 end 查找区域的结束地址；参数 size 表示
要查找可用物理内存的大小； align 参数用于对齐操作。

函数调用 memblock_find_in_range_node() 函数在指定的节点和指定的区域内找到
一块 size 大小的可用物理内存块，并将起始地址存储在 ret 中。如果 ret 为 0，
那么该标志无法获得想要的物理内存，改变物理内存标志，然后跳到 again 继续查找。

##### memblock_find_in_range_node

代码较长，分段解析

{% highlight c %}
/**
 * memblock_find_in_range_node - find free area in given range and node
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @flags: pick from blocks based on memory attributes
 *
 * Find @size free area aligned to @align in the specified range and node.
 *
 * When allocation direction is bottom-up, the @start should be greater
 * than the end of the kernel image. Otherwise, it will be trimmed. The
 * reason is that we want the bottom-up allocation just near the kernel
 * image so it is highly likely that the allocated memory and the kernel
 * will reside in the same node.
 *
 * If bottom-up allocation failed, will try to allocate memory top-down.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
phys_addr_t __init_memblock memblock_find_in_range_node(phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end, int nid,
					enum memblock_flags flags)
{
	phys_addr_t kernel_end, ret;
{% endhighlight %}

参数 size 代表需要查找内存的大小；align 代表对齐方式；start 代表需要查找
内存区域的起始地址；end 参数代表需要查找内存区域的终止地址；nid 代表 NUMA
节点信息；flags 代表内存区的标志。

{% highlight c %}
/* pump up @end */
if (end == MEMBLOCK_ALLOC_ACCESSIBLE ||
    end == MEMBLOCK_ALLOC_KASAN)
  end = memblock.current_limit;

/* avoid allocating the first page */
start = max_t(phys_addr_t, start, PAGE_SIZE);
end = max(start, end);
kernel_end = __pa_symbol(_end);
{% endhighlight %}

函数首先对 end 参数进行检测，只要参数属于 MEMBLOCK_ALLOC_ACCESSIBLE
或 MEMBLOCK_ALLOC_KASAN 中的一种，那么函数就会将 end 参数设置为
MEMBLOCK 最大限制地址。接着函数调用 max_t() 函数和 max() 函数对
start 参数和 end 参数进行简单的处理。最后通过 __pa_symbol() 函数获得
kernel 镜像的终止物理地址。

{% highlight c %}
/*
 * try bottom-up allocation only when bottom-up mode
 * is set and @end is above the kernel image.
 */
if (memblock_bottom_up() && end > kernel_end) {
  phys_addr_t bottom_up_start;

  /* make sure we will allocate above the kernel */
  bottom_up_start = max(start, kernel_end);

  /* ok, try bottom-up allocation first */
  ret = __memblock_find_range_bottom_up(bottom_up_start, end,
                size, align, nid, flags);
  if (ret)
    return ret;

  /*
   * we always limit bottom-up allocation above the kernel,
   * but top-down allocation doesn't have the limit, so
   * retrying top-down allocation may succeed when bottom-up
   * allocation failed.
   *
   * bottom-up allocation is expected to be fail very rarely,
   * so we use WARN_ONCE() here to see the stack trace if
   * fail happens.
   */
  WARN_ONCE(IS_ENABLED(CONFIG_MEMORY_HOTREMOVE),
      "memblock: bottom-up allocation failed, memory hotremove may be affected\n");
}
{% endhighlight %}

接下来，参数做了一个判断，如果 memblock_bottom_up() 函数返回 true，表示
MEMBLOCK 支持从低向上的分配，以及查找的终止地址大于内核的终止物理地址，
那么函数将执行从低地址开始查找符合要求的内存区块。采用这种分配的一定要
从 kernel 的终止地址之后开始分配，接着调用 __memblock_find_range_bottom_up()
函数进行分配，如果分配成功，则返回获得的起始地址。由于 bottom-top 的分配
要从 kernel 结束地址之后开始分配，但 top-down 分配则没有这个限制，所以
bottom-top 的分配很容易失败，所以当分配失败之后，函数会调用 WARN_ONCE 进行
提示。

{% highlight c %}
return __memblock_find_range_top_down(start, end, size, align, nid,
					      flags);
{% endhighlight %}

如果函数没有采用 bottom-up 的分配方式，那么函数就采用 top-down 的方式进行
分配，函数调用 __memblock_find_range_top_down() 函数，并直接返回查找到的
值。

##### __memblock_find_range_top_down

{% highlight c %}
/**
 * __memblock_find_range_top_down - find free area utility, in top-down
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @flags: pick from blocks based on memory attributes
 *
 * Utility called from memblock_find_in_range_node(), find free area top-down.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
static phys_addr_t __init_memblock
__memblock_find_range_top_down(phys_addr_t start, phys_addr_t end,
			       phys_addr_t size, phys_addr_t align, int nid,
			       enum memblock_flags flags)
{
	phys_addr_t this_start, this_end, cand;
	u64 i;

	for_each_free_mem_range_reverse(i, nid, flags, &this_start, &this_end,
					NULL) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);

		if (this_end < size)
			continue;

		cand = round_down(this_end - size, align);
		if (cand >= this_start)
			return cand;
	}

	return 0;
}
{% endhighlight %}

参数 start 指向一块内存区块的起始地址；end 指向一块内存区块的终止地址；
size 指向查找内存区块的大小；align 对齐方式；nid 指向 NUMA 节点；
flags 所查找内存区块的标志。

函数的作用就是从顶部到底部遍历所有可用的物理内存，每遍历到的物理内存区块中，
只要从该内存区块的顶部开始能找到大小为 size 的内存区块，那么就找到所需要的
内存区块，如果没有找到继续遍历。函数 clamp 的左右就是让第一个参数位于第二个
参数到第三个参数之间，所以可以看到，所遍历到的内存区块的起始地址要位于所查找
的范围之内。如果找到就调用 round_down() 函数从找到的顶部找一块 size 大小
的内存，最后将符合条件的地址返回。for_each_free_mem_range_reverse() 函数
源码分析请看： [for_each_free_mem_range_reverse() 源码](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-for_each_free_mem_range_reverse/#源码分析)

##### __memblock_find_range_bottom_up

{% highlight c %}
/**
 * __memblock_find_range_bottom_up - find free area utility in bottom-up
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @flags: pick from blocks based on memory attributes
 *
 * Utility called from memblock_find_in_range_node(), find free area bottom-up.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
static phys_addr_t __init_memblock
__memblock_find_range_bottom_up(phys_addr_t start, phys_addr_t end,
				phys_addr_t size, phys_addr_t align, int nid,
				enum memblock_flags flags)
{
	phys_addr_t this_start, this_end, cand;
	u64 i;

	for_each_free_mem_range(i, nid, flags, &this_start, &this_end, NULL) {
		this_start = clamp(this_start, start, end);
		this_end = clamp(this_end, start, end);

		cand = round_up(this_start, align);
		if (cand < this_end && this_end - cand >= size)
			return cand;
	}

	return 0;
}
{% endhighlight %}

参数 start 指向一块内存区块的起始地址；end 指向一块内存区块的终止地址；
size 指向查找内存区块的大小；align 对齐方式；nid 指向 NUMA 节点；
flags 所查找内存区块的标志。

函数的作用是正序遍历所有的可用物理内存，每遍历到一块物理内存，都会从起始地址
开始，检查能否找到一块大小为 size 的内存区块。如果内找到就返回这个地址，
如果不能找到，继续循环。函数每次遍历到一个物理内存区块时，都会调用 clamp()
函数，以确保要查找的范围在遍历到的内存区内。如果找到，那么调用 round_up()
函数从找到的内存区底部到顶部，大小为 size 的内存区块。如果找到，那么就
返回这个地址；如果没找到，那么继续遍历可用物理内存区块。最后找到符合要求的
地址，for_each_free_mem_range() 函数源码分析请看：[for_each_free_mem_range() 源码分析](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-for_each_free_mem_range/#源码分析)


---------------------------------------------

# <span id="实践">实践</span>

> [实践目的](#驱动实践目的)
>
> [实践准备](#驱动实践准备)
>
> [驱动源码](#驱动源码)
>
> [驱动安装](#驱动安装)
>
> [驱动配置](#驱动配置)
>
> [驱动编译](#驱动编译)
>
> [增加调试点](#驱动增加调试点)
>
> [驱动运行](#驱动运行)
>
> [驱动分析](#驱动分析)

#### <span id="驱动实践目的">实践目的</span>

memblock_find_in_range() 函数用于从指定的内存区域获得可用的物理
内存，所以本次实践的目的就是从不同的内存区域中获得可用的物理内存。

#### <span id="驱动实践准备">实践准备</span>

由于本次实践是基于 Linux 5.x 的 arm32 系统，所以请先参考 Linux 5.x arm32
开发环境搭建方法以及重点关注驱动实践一节，请参考下例文章，选择一个 linux 5.x
版本进行实践，后面内容均基于 linux 5.x 继续讲解，文章链接如下：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

#### <span id="驱动源码">驱动源码</span>

准备好开发环境之后，下一步就是准备实践所用的驱动源码，驱动的源码如下：

{% highlight bash %}
/*
 * memblock allocator
 *
 * (C) 2019.03.05 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * MEMBLOCK
 *
 *
 *                                         struct memblock_region
 *                       struct            +------+------+--------+------+
 *                       memblock_type     |      |      |        |      |
 *                       +----------+      | Reg0 | Reg1 | ...    | Regn |
 *                       |          |      |      |      |        |      |
 *                       | regions -|----->+------+------+--------+------+
 *                       | cnt      |      [memblock_memory_init_regions]
 *                       |          |
 * struct           o--->+----------+
 * memblock         |
 * +-----------+    |
 * |           |    |
 * | memory   -|----o
 * | reserved -|----o
 * |           |    |                      struct memblock_region
 * +-----------+    |    struct            +------+------+--------+------+
 *                  |    memblock_type     |      |      |        |      |
 *                  o--->+----------+      | Reg0 | Reg1 | ...    | Regn |
 *                       |          |      |      |      |        |      |
 *                       | regions -|----->+------+------+--------+------+
 *                       | cnt      |      [memblock_reserved_init_regions]
 *                       |          |
 *                       +----------+
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>

#include <asm/sections.h>

int bs_debug = 0;

#ifdef CONFIG_DEBUG_MEMBLOCK_FIND_IN_RANGE
/*
 * Mark memory as reserved on memblock.reserved regions.
 */
int debug_memblock_find_in_range(void)
{
	struct memblock_region *reg;
	phys_addr_t kernel_end;
	phys_addr_t addr;
	bool bottom_up;

	/*
	 * Memory Maps:
	 *
	 *                     Reserved 0            Reserved 1
	 *          _end     | <------> |          | <------> |
	 * +--+-----+--------+----------+----------+----------+------+
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * +--+-----+--------+----------+----------+----------+------+
	 *    | <-> |
	 *    Kernel
	 *
	 * Reserved 0: [0x64000000, 0x64100000]
	 * Reserved 1: [0x64300000, 0x64400000]
	 */
	memblock_reserve(0x64000000, 0x100000);
	memblock_reserve(0x64300000, 0x100000);
	kernel_end = __pa_symbol(_end);
	bottom_up = memblock_bottom_up();

	for_each_memblock(memory, reg)
		pr_info("Memory-Region:   %#x - %#x\n", reg->base,
				reg->base + reg->size);
	for_each_memblock(reserved, reg)
		pr_info("Reserved-Region: %#x - %#x\n", reg->base,
				reg->base + reg->size);
	pr_info("Kernel End addr: %#x\n", kernel_end);

	/*
	 * Top-down allocate:
	 *
	 *                     Reserved 0            Reserved 1
	 *          _end     | <------> |          | <------> |
	 * +--+-----+--------+----------+----------+----------+------+
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * +--+-----+--------+----------+----------+----------+------+
	 *    | <-> |                                            <---|
	 *    Kernel
	 *
	 * Reserved 0: [0x64000000, 0x64100000]
         * Reserved 1: [0x64300000, 0x64400000]
	 *
	 * Find free are in given range and node, memblock will allocate
	 * memory in Top-down direction.
	 */
	memblock_set_bottom_up(false);
	addr = (phys_addr_t)memblock_find_in_range(
			MEMBLOCK_LOW_LIMIT, ARCH_LOW_ADDRESS_LIMIT,
			0x100000, 0x1000);
	pr_info("Top-down Addre:  %#x\n", addr);

	/*
	 * Bottom-up allocate:
	 *
	 *                     Reserved 0            Reserved 1
	 *          _end     | <------> |          | <------> |
	 * +--+-----+--------+----------+----------+----------+------+
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * +--+-----+--------+----------+----------+----------+------+
	 *    | <-> |---->
	 *    Kernel
	 *
	 * Reserved 0: [0x64000000, 0x64100000]
         * Reserved 1: [0x64300000, 0x64400000]
	 *
	 * Find free are in given range and node, memblock will allocate
	 * memory in bottom-up direction.
	 */
	memblock_set_bottom_up(true);
	addr = (phys_addr_t)memblock_find_in_range(
			MEMBLOCK_LOW_LIMIT, ARCH_LOW_ADDRESS_LIMIT,
			0x100000, 0x1000);
	pr_info("Bottom-up Addr:  %#x\n", addr);

	/*
	 * Special area allocate:
	 *
	 *                     Reserved 0            Reserved 1
	 *          _end     | <------> |          | <------> |
	 * +--+-----+--------+----------+----------+----------+------+
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * |  |     |        |          |          |          |      |
	 * +--+-----+--------+----------+----------+----------+------+
	 *    | <-> |                   |---->
	 *    Kernel
	 *
	 * Reserved 0: [0x64000000, 0x64100000]
	 * Reserved 1: [0x64300000, 0x64400000]
	 *
	 * Find free are in given range and node, memblock will allocate
	 * memory in bottom-up from special address.
	 */
	memblock_set_bottom_up(true);
	addr = (phys_addr_t)memblock_find_in_range(
			0x64100000, 0x64300000, 0x100000, 0x1000);
	pr_info("Speical Address: %#x\n", addr);

	/* Clear debug case */
	memblock.reserved.cnt = 1;
	memblock.reserved.total_size = 0;
	memblock_set_bottom_up(bottom_up);

	return 0;
}
#endif
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

由于这部分驱动涉及到较早的内核启动接管，所以不能直接以模块的形式编入到内核，
需要直接编译进内核，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为
memblock.c，然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index cca538e38..c4c2edcab 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
 config BISCUITOS_MISC
     bool "BiscuitOS misc driver"

+config MEMBLOCK_ALLOCATOR
+       bool "MEMBLOCK allocator"
+
+if MEMBLOCK_ALLOCATOR
+
+config DEBUG_MEMBLOCK_FIND_IN_RANGE
+       bool "memblock_find_in_range()"
+
+endif # MEMBLOCK_ALLOCATOR
+
 endif # BISCUITOS_DRV
{% endhighlight %}

接着修改 Makefile，请参考如下修改：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Makefile b/drivers/BiscuitOS/Makefile
index 82004c9a2..1e4052a4b 100644
--- a/drivers/BiscuitOS/Makefile
+++ b/drivers/BiscuitOS/Makefile
@@ -1 +1,2 @@
 obj-$(CONFIG_BISCUITOS_MISC)     += BiscuitOS_drv.o
+obj-$(CONFIG_MEMBLOCK_ALLOCATOR) += memblock.o
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]Memblock allocator
            [*]memblock_find_in_range()
{% endhighlight %}

具体过程请参考：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

#### <span id="驱动增加调试点">增加调试点</span>

驱动运行还需要在内核的指定位置添加调试点，由于该驱动需要在内核启动阶段就使
用，请参考下面 patch 将源码指定位置添加调试代码：

{% highlight bash %}
diff --git a/arch/arm/kernel/setup.c b/arch/arm/kernel/setup.c
index 375b13f7e..fec6919a9 100644
--- a/arch/arm/kernel/setup.c
+++ b/arch/arm/kernel/setup.c
@@ -1073,6 +1073,10 @@ void __init hyp_mode_check(void)
 void __init setup_arch(char **cmdline_p)
 {
 	const struct machine_desc *mdesc;
+#ifdef CONFIG_DEBUG_MEMBLOCK_FIND_IN_RANGE
+	extern int bs_debug;
+	extern int debug_memblock_find_in_range(void);
+#endif

 	setup_processor();
 	mdesc = setup_machine_fdt(__atags_pointer);
@@ -1104,6 +1108,10 @@ void __init setup_arch(char **cmdline_p)
 	strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
 	*cmdline_p = cmd_line;

+#ifdef CONFIG_DEBUG_MEMBLOCK_FIND_IN_RANGE
+	debug_memblock_find_in_range();
+#endif
+
 	early_fixmap_init();
 	early_ioremap_init();
{% endhighlight %}

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

驱动运行的结果如下：

{% highlight bash %}
CPU: ARMv7 Processor [410fc090] revision 0 (ARMv7), cr=10c5387d
CPU: PIPT / VIPT nonaliasing data cache, VIPT nonaliasing instruction cache
OF: fdt: Machine model: V2P-CA9
Memory-Region:   0x60000000 - 0xa0000000
Reserved-Region: 0x64000000 - 0x64100000
Reserved-Region: 0x64300000 - 0x64400000
Kernel End addr: 0x60b95258
Top-down Addre:  0x9ff00000
Bottom-up Addr:  0x60b96000
Speical Address: 0x64100000
Malformed early option 'earlycon'
Memory policy: Data cache writeback
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在驱动中，首先调用 memblock_reserve() 函数增加两块预留区块，这两块预留区块的
范围是： [0x64000000, 0x64100000] 和 [0x64300000, 0x64400000]。系统启动
过程中，从 DTS 中获得一块可用物理内存区的范围是： [0x60000000, 0xa0000000]。
于是调试之前，内存布局如下：

{% highlight bash %}
Memory Maps:

                     Reserved 0            Reserved 1
                   | <------> |          | <------> |
 +-----------------+----------+----------+----------+------+
 |                 |          |          |          |      |
 |                 |          |          |          |      |
 |                 |          |          |          |      |
 +-----------------+----------+----------+----------+------+
                              | <------> |


 Reserved 0: [0x64000000, 0x64100000]
 Reserved 1: [0x64300000, 0x64400000]
{% endhighlight %}

接着调用 for_each_mem_range() 函数遍历所有的可用物理内存，并将
可用物理内存的起始地址和终止地址都打印出来，并调用 __pa_symbol() 函数
获得内核代码段结束的物理地址，代码如下：

{% highlight c %}
memblock_reserve(0x64000000, 0x100000);
memblock_reserve(0x64300000, 0x100000);
kernel_end = __pa_symbol(_end);
bottom_up = memblock_bottom_up();

for_each_memblock(memory, reg)
  pr_info("Memory-Region:   %#x - %#x\n", reg->base,
      reg->base + reg->size);
for_each_memblock(reserved, reg)
  pr_info("Reserved-Region: %#x - %#x\n", reg->base,
      reg->base + reg->size);
pr_info("Kernel End addr: %#x\n", kernel_end);
{% endhighlight %}

接下来，从可用内存区的顶部开始，向下查找一块大小为 size 的物理内存块，
查找的范围是从 MEMBLOCK_LOW_LIMIT 到 ARCH_LOW_ADDRESS_LIMIT，具体代码如下：

{% highlight bash %}
/*
 * Top-down allocate:
 *
 *                     Reserved 0            Reserved 1
 *          _end     | <------> |          | <------> |
 * +--+-----+--------+----------+----------+----------+------+
 * |  |     |        |          |          |          |      |
 * |  |     |        |          |          |          |      |
 * |  |     |        |          |          |          |      |
 * +--+-----+--------+----------+----------+----------+------+
 *    | <-> |                                            <---|
 *    Kernel
 *
 * Reserved 0: [0x64000000, 0x64100000]
       * Reserved 1: [0x64300000, 0x64400000]
 *
 * Find free are in given range and node, memblock will allocate
 * memory in Top-down direction.
 */
 memblock_set_bottom_up(false);
 addr = (phys_addr_t)memblock_find_in_range(
     MEMBLOCK_LOW_LIMIT, ARCH_LOW_ADDRESS_LIMIT,
     0x100000, 0x1000);
 pr_info("Top-down Addre:  %#x\n", addr);
{% endhighlight %}

首先调用 memblock_set_bottom_up() 函数，传入 false 值之后，MEMBLOCK
就不会从底部开始，而是从顶部开始分配内存。接着调用了 memblock_find_in_range()
函数，传入值之后，将找到的地址存储到 addr 里。运行结果如下：

{% highlight bash %}
Memory-Region:   0x60000000 - 0xa0000000
Reserved-Region: 0x64000000 - 0x64100000
Reserved-Region: 0x64300000 - 0x64400000
Kernel End addr: 0x60b95258
Top-down Addre:  0x9ff00000
{% endhighlight %}

从运行结果可以看出，内核物理内存空间范围是: [0x60000000 - 0xa0000000],
所以从物理内存的顶部找到了一块 0x100000 的可用物理内存。其地址是：
[0x9ff00000 - 0xa0000000]。

接下来从底部开始到顶部找一块可用物理内存，代码如下：

{% highlight bash %}
/*
 * Bottom-up allocate:
 *
 *                     Reserved 0            Reserved 1
 *          _end     | <------> |          | <------> |
 * +--+-----+--------+----------+----------+----------+------+
 * |  |     |        |          |          |          |      |
 * |  |     |        |          |          |          |      |
 * |  |     |        |          |          |          |      |
 * +--+-----+--------+----------+----------+----------+------+
 *    | <-> |---->
 *    Kernel
 *
 * Reserved 0: [0x64000000, 0x64100000]
 * Reserved 1: [0x64300000, 0x64400000]
 *
 * Find free are in given range and node, memblock will allocate
 * memory in bottom-up direction.
 */
 memblock_set_bottom_up(true);
 addr = (phys_addr_t)memblock_find_in_range(
     MEMBLOCK_LOW_LIMIT, ARCH_LOW_ADDRESS_LIMIT,
     0x100000, 0x1000);
 pr_info("Bottom-up Addr:  %#x\n", addr);
{% endhighlight %}

函数同样先调用 memblock_set_bottom_up() 函数，传入 true 值，这样 MEMBLOCK
就支持从底部往顶部方向分配可用物理内存，然后调用 memblock_find_in_range()
函数返回找到的地址，存储到 addr 中，运行结果如下：

{% highlight bash %}
Memory-Region:   0x60000000 - 0xa0000000
Reserved-Region: 0x64000000 - 0x64100000
Reserved-Region: 0x64300000 - 0x64400000
Kernel End addr: 0x60b95258
Bottom-up Addr:  0x60b96000
{% endhighlight %}

由源码分析可知，MEMBLOCK 从 kernel 结束的物理位置开始之后的物理内存进行分配，从上
面运行结果可以知道，kernel 的终止物理地址是 0x60b95258，所以找到的第一块可用的物理
内存就是：[0x60b96000 -- 0x60ba6000]

最后，可以指定在特定的物理内存内找指定大小的可用物理内存块，具体的代码如下：

{% highlight bash %}
/*
 * Special area allocate:
 *
 *                     Reserved 0            Reserved 1
 *          _end     | <------> |          | <------> |
 * +--+-----+--------+----------+----------+----------+------+
 * |  |     |        |          |          |          |      |
 * |  |     |        |          |          |          |      |
 * |  |     |        |          |          |          |      |
 * +--+-----+--------+----------+----------+----------+------+
 *    | <-> |                   |---->
 *    Kernel
 *
 * Reserved 0: [0x64000000, 0x64100000]
       * Reserved 1: [0x64300000, 0x64400000]
 *
 * Find free are in given range and node, memblock will allocate
 * memory in bottom-up from special address.
 */
 memblock_set_bottom_up(true);
 addr = (phys_addr_t)memblock_find_in_range(
     0x64100000, 0x64300000, 0x100000, 0x1000);
 pr_info("Speical Address: %#x\n", addr);
{% endhighlight %}

首先也是调用 memblock_set_bottom_up() 函数，选择是从底部还是从顶部查找
可用内存的。这里选择 true，那么就从底部开始查找，所查找的范围是：
[0x64100000 -- 0x64300000], 查找大小为 0x100000 的物理内存块，运行结果如下：

{% highlight bash %}
Memory-Region:   0x60000000 - 0xa0000000
Reserved-Region: 0x64000000 - 0x64100000
Reserved-Region: 0x64300000 - 0x64400000
Kernel End addr: 0x60b95258
Speical Address: 0x64100000
{% endhighlight %}

从上图运行可知，[0x64100000 -- 0x64300000] 区域在两个预留区之间，是一块可用的
物理内存区块，那么函数就从这块可用物理内存的底部开始分配，所以获得区间是：
[0x64100000 -- 0x64200000].

更多实践原理，请查看 [memblock_find_in_range() 源码分析](#源码分析)

-----------------------------------------------

# <span id="附录">附录</span>

> [MEMBLOCK 内存分配器](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-index/)
>
> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](https://biscuitos.github.io/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [搭建高效的 Linux 开发环境](https://biscuitos.github.io/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
