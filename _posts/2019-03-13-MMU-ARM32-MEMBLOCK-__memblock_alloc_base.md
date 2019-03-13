---
layout: post
title:  "__memblock_alloc_base() 从指定地址之前分配物理内存"
date:   2019-03-13 15:05:30 +0800
categories: [MMU]
excerpt: __memblock_alloc_base() 从指定地址之前分配物理内存.
tags:
  - MMU
---

> [GitHub: MEMBLOCK __memblock_alloc_base()](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/__memblock_alloc_base)
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

函数： __memblock_alloc_base()

功能： 从指定地址之前分配物理内存

{% highlight bash %}
__memblock_alloc_base
|
|---memblock_alloc_base_nid
    |
    |---memblock_alloc_range_nid
    |
    |---memblock_find_in_range_node
    |
    |---memblock_reserve
{% endhighlight %}

##### __memblock_alloc_base

{% highlight c %}
phys_addr_t __init __memblock_alloc_base(phys_addr_t size, phys_addr_t align, phys_addr_t max_addr)
{
	return memblock_alloc_base_nid(size, align, max_addr, NUMA_NO_NODE,
				       MEMBLOCK_NONE);
}
{% endhighlight %}

参数 size 指明要分配物理内存的大小，align 是对齐方式；参数 max_addr
指明最大可分配物理地址。

函数直接调用 memblock_alloc_base_nid() 函数分配所需内存。

##### memblock_alloc_base_nid

{% highlight c %}
phys_addr_t __init memblock_alloc_base_nid(phys_addr_t size,
					phys_addr_t align, phys_addr_t max_addr,
					int nid, enum memblock_flags flags)
{
	return memblock_alloc_range_nid(size, align, 0, max_addr, nid, flags);
}
{% endhighlight %}

参数 size 代表要分配区块的大小，align 表示分配区块的对齐大小；参数 end 代表所
分配区域的终止物理地址；flags参数代表分配内存区块的标志。nid 代表 NUMA 号。

函数直接调用 memblock_alloc_range_nid() 函数，分配一块可用的物理内存，并将
这块内存区加入预留区内。

##### memblock_alloc_range_nid

{% highlight c %}
static phys_addr_t __init memblock_alloc_range_nid(phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end, int nid,
					enum memblock_flags flags)
{
	phys_addr_t found;

	if (!align) {
		/* Can't use WARNs this early in boot on powerpc */
		dump_stack();
		align = SMP_CACHE_BYTES;
	}

	found = memblock_find_in_range_node(size, align, start, end, nid,
					    flags);
	if (found && !memblock_reserve(found, size)) {
		/*
		 * The min_count is set to 0 so that memblock allocations are
		 * never reported as leaks.
		 */
		kmemleak_alloc_phys(found, size, 0, 0);
		return found;
	}
	return 0;
}
{% endhighlight %}

参数 size 代表要分配区块的大小，align 表示分配区块的对齐大小；参数 start
代表所分配区域的起始物理地址；参数 end 代表所分配区域的终止物理地址；flags
参数代表分配内存区块的标志。参数 nid 指向 NUMA 节点号。

函数首先对 align 参数进行检测，如果为零，则警告。接着函数调用
memblock_find_in_range_node() 函数从可用内存区中找一块大小为 size 的物理内存区块，
然后调用 memblock_reseve() 函数在找到的情况下，将这块物理内存区块加入到预留区内。

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

##### memblock_reserve

memblock_reserve() 函数源码分析请看：[memblock_reserve() 源码](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-memblock_reserve/#源码分析)

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

__memblock_alloc_base() 函数用于从指定地址之前分配物理内存，所以本次实践的目的
就是从指定地址之后分配一定长度的物理内存。

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

int bs_debug = 0;

#ifdef CONFIG_DEBUG___MEMBLOCK_ALLOC_BASE
int __init debug___memblock_alloc_base(void)
{
	struct memblock_region *reg;
	phys_addr_t addr;

	/*
	 * Memory Map
	 *
	 *                  memblock.memory
	 * 0    | <---------------------------------> |
	 * +----+-------------------------------------+-----------+
	 * |    |                                     |           |
	 * |    |                                     |           |
	 * |    |                                     |           |
	 * +----+-------------------------------------+-----------+
	 *
	 * Memory Region: [0x60000000, 0xa0000000]
	 */
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
			reg->base + reg->size);

	/*
	 * Memory Map
	 *
	 *                  memblock.memory
	 * 0    | <---------------------------------> |
	 * +----+------------------------------+------+-----------+
	 * |    |                              |      |           |
	 * |    |                              |      |           |
	 * |    |                              |      |           |
	 * +----+------------------------------+------+-----------+
	 *                                     | <--> |
	 *                                      region
	 *
	 * Memory Region: [0x60000000, 0xa0000000]
	 * Found Region:  [0x9ff00000, 0xa0000000]
	 */
	addr = __memblock_alloc_base(0x100000, 0x1000,
					ARCH_LOW_ADDRESS_LIMIT);
	pr_info("Find address: %#x\n", addr);

	/* Dump all reserved region */
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
				reg->base + reg->size);

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
+config DEBUG___MEMBLOCK_ALLOC_BASE
+       bool "__memblock_alloc_base()"
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
            [*]__memblock_alloc_base()
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
+#ifdef CONFIG_DEBUG___MEMBLOCK_ALLOC_BASE
+	extern int bs_debug;
+	extern int debug___memblock_alloc_base(void);
+#endif

 	setup_processor();
 	mdesc = setup_machine_fdt(__atags_pointer);
@@ -1104,6 +1108,10 @@ void __init setup_arch(char **cmdline_p)
 	strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
 	*cmdline_p = cmd_line;

ck_add
+#ifdef CONFIG_DEBUG___MEMBLOCK_ALLOC_BASE
+	debug___memblock_alloc_base();
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
Region: 0x0 - 0x0
Find address: 0x9ff00000
Region: 0x9ff00000 - 0xa0000000
Malformed early option 'earlycon'
Memory policy: Data cache writeback
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在系统启动初期，可用物理内存的地址范围就是真实的物理内存范围，使用
for_each_memblock() 函数遍历预留内存的各个内存区块，代码如下：

{% highlight bash %}
	/*
	 * Memory Map
	 *
	 *                  memblock.memory
	 * 0    | <---------------------------------> |
	 * +----+-------------------------------------+-----------+
	 * |    |                                     |           |
	 * |    |                                     |           |
	 * |    |                                     |           |
	 * +----+-------------------------------------+-----------+
	 *
	 * Memory Region: [0x60000000, 0xa0000000]
	 */
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
			reg->base + reg->size);
{% endhighlight %}

从上面的代码可以确认在未分配之前，系统有多少已分配的内存。接着调用
__memblock_alloc_base() 函数分配一个大小为 0x100000 的内存区块，
代码如下：

{% highlight bash %}
	/*
	 * Memory Map
	 *
	 *                  memblock.memory
	 * 0    | <---------------------------------> |
	 * +----+------------------------------+------+-----------+
	 * |    |                              |      |           |
	 * |    |                              |      |           |
	 * |    |                              |      |           |
	 * +----+------------------------------+------+-----------+
	 *                                     | <--> |
	 *                                      region
	 *
	 * Memory Region: [0x60000000, 0xa0000000]
	 * Found Region:  [0x9ff00000, 0xa0000000]
	 */
	addr = __memblock_alloc_base(0x100000, 0x1000,
					ARCH_LOW_ADDRESS_LIMIT);
	pr_info("Find address: %#x\n", addr);

	/* Dump all reserved region */
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
				reg->base + reg->size);
{% endhighlight %}

分配完之后调用 for_each_memblock() 函数再次遍历预留区内的所有物理内存区块，
以此查看分配情况，运行情况如下图：

{% highlight bash %}
Region: 0x0 - 0x0
Find address: 0x9ff00000
Region: 0x9ff00000 - 0xa0000000
{% endhighlight %}

从运行的结果可以知道，__memblock_alloc_base() 函数找到了一块起始地址为
0x9ff00000, 长度为 0x100000 的内存区，再者，预留区内多了一块内存区块：
[0x9ff00000, 0xa0000000], 因此分配成功。更多原理请看源码分析。
