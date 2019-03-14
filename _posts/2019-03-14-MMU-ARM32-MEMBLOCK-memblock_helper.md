---
layout:             post
title:              "从 MEMBLOCK 获得信息"
date:               2019-03-13 18:20:30 +0800
categories:         [MMU]
excerpt:            从 MEMBLOCK 获得信息.
tags:
  - MMU
---

> [GitHub:          MEMBLOCK information helper()](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/)
>
> Email:            BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> [原理](#原理)
>
> [信息接口](#信息接口)
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
* @bottom_up:       is bottom up direction?
* @current_limit:   physical address of the current allocation limit
* @memory:          usabe memory regions
* @reserved:        reserved memory regions
* @physmem:         all physical memory
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
* @cnt:             number of regions
* @max:             size of the allocated array
* @total_size:      size of all regions
* @regions:         array of regions
* @name:            the memory type symbolic name
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
* @base:            physical address of the region
* @size:            size of the region
* @flags:           memory region attributes
* @nid:             NUMA node id
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

# <span id="信息接口">信息接口</span>

MEMBLOCK 提供了很多 API 以此获得 MEMBLOCK 的信息，这些信息包括多种多样，具体 API
如下：

> - [memblock_phys_mem_size: 获得可用物理内存的总体积](#memblock_phys_mem_siz)
>
> - [memblock_reserved_size: 获得预留区内存的总体积](#memblock_reserved_size)
>
> - [memblock_start_of_DRAM: 获得 DRAM 的起始物理地址](#memblock_start_of_DRAM)
>
> - [memblock_end_of_DRAM: 获得 DRAM 的结束物理地址](#memblock_end_of_DRAM)
>
> - [memblock_is_reserved: 检查某个物理地址是否位于预留区内](#memblock_is_reserved)
>
> - [memblock_is_memory: 检查某个物理地址是否位于可用内存区](#memblock_is_memory)
>
> - [memblock_is_region_memory: 检查某段内存区是否属于可用内存区](#memblock_is_region_memory)
>
> - [memblock_is_region_reserved: 检查某段内存区块是否属于预留区](#memblock_is_region_reserved)
>
> - [memblock_get_current_limit: 获得 MEMBLOCK 的 limit](#memblock_get_current_limit)
>
> - [memblock_set_current_limit: 设置 MEMBLOCK 的 limit](#memblock_set_current_limit)
>
> - [memblock_is_hotpluggable: 检查内存区块是否支持热插拔](#memblock_is_hotpluggable)
>
> - [memblock_is_mirror: 检查内存区块是否支持 mirror](#memblock_is_mirror)
>
> - [memblock_is_nomap: 检查内存区块是否支持 nomap](#memblock_is_nomap)
>
> - [memblock_get_region_node: 获得内存区块的 NUMA 号](#memblock_get_region_node)
>
> - [memblock_set_region_node: 设置内存区块的 NUMA 号](#memblock_set_region_node)
>
> - [memblock_bottom_up: 获得 MEMBLOCK 分配的方向](#memblock_bottom_up)
>
> - [memblock_set_bottom_up: 设置 MEMBLOCK 分配的方向](#memblock_set_bottom_up)


--------------------------------------------------

# <span id="源码分析">源码分析</span>

> Arch： arm32
>
> Version： Linux 5.x

##### <span id="memblock_phys_mem_size">memblock_phys_mem_size</span>

{% highlight c %}
phys_addr_t __init_memblock memblock_phys_mem_size(void)
{
	return memblock.memory.total_size;
}
{% endhighlight %}

函数的作用是获得可用物理内存的总体积。函数直接返回 memblock.memory 的
total_size， total_size 成员存储该内存区的体积大小。

[GitHub: memblock_phys_mem_size](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_phys_mem_size)

##### <span id="memblock_reserved_size">memblock_reserved_size</span>

{% highlight c %}
phys_addr_t __init_memblock memblock_reserved_size(void)
{
	return memblock.reserved.total_size;
}
{% endhighlight %}

函数的作用是获得预留区内存的总体积。函数直接返回 memblock.reserved 的
total_size， total_size 成员存储该内存区的体积大小。

[GitHub: memblock_reserved_size](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_reserved_size)

##### <span id="memblock_start_of_DRAM">memblock_start_of_DRAM</span>

{% highlight c %}
/* lowest address */
phys_addr_t __init_memblock memblock_start_of_DRAM(void)
{
	return memblock.memory.regions[0].base;
}
{% endhighlight %}

函数的作用是获得 DRAM 的起始地址。DRAM 的起始地址就是 memblock.memory 内存区
第一个内存区块的起始物理地址。函数直接返回 memblock.memory 的
regions[0].base， regions[0].base 成员存储 DRAM 的起始物理地址。

[GitHub: memblock_start_of_DRAM](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_start_of_DRAM)

##### <span id="memblock_end_of_DRAM">memblock_end_of_DRAM</span>

{% highlight c %}
phys_addr_t __init_memblock memblock_end_of_DRAM(void)
{
	int idx = memblock.memory.cnt - 1;

	return (memblock.memory.regions[idx].base + memblock.memory.regions[idx].size);
}
{% endhighlight %}

函数的作用是获得 DRAM 的终止地址。DRAM 的终止地址就是 memblock.memory 内存区
最后一个内存区块的终止物理地址。最后一个内存区的索引是 memblock.memory.cnt - 1，
所以这个索引对应的内存区的终止地址就是 DRAM 的结束地址。

[GitHub: memblock_end_of_DRAM](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_end_of_DRAM)

##### <span id="memblock_is_reserved">memblock_is_reserved</span>

{% highlight c %}
bool __init_memblock memblock_is_reserved(phys_addr_t addr)
{
	return memblock_search(&memblock.reserved, addr) != -1;
}
{% endhighlight %}

函数的作用是检查某个物理地址是否属于预留区。参数 addr 指向要检查的地址。
函数调用 memblock_search() 函数进行地址检查。

[GitHub: memblock_is_reserved](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_is_reserved)

##### <span id="memblock_is_memory">memblock_is_memory</span>

{% highlight c %}
bool __init_memblock memblock_is_memory(phys_addr_t addr)
{
	return memblock_search(&memblock.memory, addr) != -1;
}
{% endhighlight %}

函数的作用是检查某个物理地址是否属于可用内存区。参数 addr 指向要检查的地址。
函数调用 memblock_search() 函数进行地址检查。

[GitHub: memblock_is_memory](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_is_memory)

##### <span id="memblock_is_region_memory">memblock_is_region_memory</span>

{% highlight c %}
/**
 * memblock_is_region_memory - check if a region is a subset of memory
 * @base: base of region to check
 * @size: size of region to check
 *
 * Check if the region [@base, @base + @size) is a subset of a memory block.
 *
 * Return:
 * 0 if false, non-zero if true
 */
bool __init_memblock memblock_is_region_memory(phys_addr_t base, phys_addr_t size)
{
	int idx = memblock_search(&memblock.memory, base);
	phys_addr_t end = base + memblock_cap_size(base, &size);

	if (idx == -1)
		return false;
	return (memblock.memory.regions[idx].base +
		 memblock.memory.regions[idx].size) >= end;
}
{% endhighlight %}

函数的作用是检查某段内存区是否属于可用内存区。参数 base 需要检查的区段的起始物理
地址。参数 size 需要检查区段的长度。函数调用 memblock_search() 函数对 base 参
数进行检查，memblock_search() 函数返回 base 地址在可用内存区的索引，接着计算出
需要检查区段的终止地址，最后通过检查终止地址是否在 idx 对应的内存区段之内，如果
在表示这段内存区块在可用物理内存区内；如果不在，则表示这段内存区块不属于可用物理
内存区段。

[GitHub: memblock_is_region_memory](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_is_region_memory)

##### <span id="memblock_is_region_reserved">memblock_is_region_reserved</span>

{% highlight c %}
/**
 * memblock_is_region_reserved - check if a region intersects reserved memory
 * @base: base of region to check
 * @size: size of region to check
 *
 * Check if the region [@base, @base + @size) intersects a reserved
 * memory block.
 *
 * Return:
 * True if they intersect, false if not.
 */
bool __init_memblock memblock_is_region_reserved(phys_addr_t base, phys_addr_t size)
{
	memblock_cap_size(base, &size);
	return memblock_overlaps_region(&memblock.reserved, base, size);
}
{% endhighlight %}

函数的作用是检查某段内存区是否属于预留区。参数 base 需要检查的区段的起始物理
地址。参数 size 需要检查区段的长度。函数调用 memblock_cap_size() 函数对 size
参数进行处理之后，传递给 memblock_overlaps_region() 函数检查 base 和 size
对应的内存区是否属于预留区。

[GitHub: memblock_is_region_reserved](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_is_region_reserved)

##### <span id="memblock_get_current_limit">memblock_get_current_limit</span>

{% highlight c %}
phys_addr_t __init_memblock memblock_get_current_limit(void)
{
	return memblock.current_limit;
}
{% endhighlight %}

函数的作用就是返回 MEMBLOCK 的当前 limit。

[GitHub: memblock_get_current_limit](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_get_current_limit)

##### <span id="memblock_set_current_limit">memblock_set_current_limit</span>

{% highlight c %}
void __init_memblock memblock_set_current_limit(phys_addr_t limit)
{
	memblock.current_limit = limit;
}
{% endhighlight %}

函数的作用就是设置 MEMBLOCK 的当前 limit。

[GitHub: memblock_set_current_limit](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_set_current_limit)

##### <span id="memblock_is_hotpluggable">memblock_is_hotpluggable</span>

{% highlight c %}
static inline bool memblock_is_hotpluggable(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_HOTPLUG;
}
{% endhighlight %}

函数的作用就是检查特定的内存区块是否支持热插拔。函数之间检查内存区块的 flags
是否支持 MEMBLOCK_HOTPLUG。

[GitHub: memblock_is_hotpluggable](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_is_hotpluggable)

##### <span id="memblock_is_mirror">memblock_is_mirror</span>

{% highlight c %}
static inline bool memblock_is_mirror(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_MIRROR;
}
{% endhighlight %}

函数的作用就是检查特定的内存区块是否支持 mirror。函数之间检查内存区块的 flags
是否支持 MEMBLOCK_MIRROR。

[GitHub: memblock_is_mirror](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_is_mirror)

##### <span id="memblock_is_nomap">memblock_is_nomap</span>

{% highlight c %}
static inline bool memblock_is_nomap(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_NOMAP;
}
{% endhighlight %}

函数的作用就是检查特定的内存区块是否支持 mirror。函数之间检查内存区块的 flags
是否支持 MEMBLOCK_MIRROR。

[GitHub: memblock_is_nomap](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_is_nomap)

##### <span id="memblock_get_region_node">memblock_get_region_node</span>

{% highlight c %}
static inline int memblock_get_region_node(const struct memblock_region *r)
{
	return r->nid;
}
{% endhighlight %}

函数的作用就是获得特定内存区块的 NUMA 号。参数 r 指向特定的内存区块。内存
区块的 NUMA 号存储在 nid 成员里，所以函数直接返回 nid 的值。

[GitHub: memblock_get_region_node](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_get_region_node)

##### <span id="memblock_set_region_node">memblock_set_region_node</span>

{% highlight c %}
static inline void memblock_set_region_node(struct memblock_region *r, int nid)
{
	r->nid = nid;
}
{% endhighlight %}

函数的作用就是设置特定内存区块的 NUMA 号。参数 r 指向特定的内存区块。内存
区块的 NUMA 号存储在 nid 成员里，所以函数直接设置 nid 的值。

[GitHub: memblock_set_region_node](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_set_region_node)

##### <span id="memblock_bottom_up">memblock_bottom_up</span>

{% highlight c %}
/*
 * Check if the allocation direction is bottom-up or not.
 * if this is true, that said, memblock will allocate memory
 * in bottom-up direction.
 */
static inline bool memblock_bottom_up(void)
{
	return memblock.bottom_up;
}
{% endhighlight %}

函数的作用就是获得 MEMBLOCK 分配的方向。 MEMBLOCK 支持从顶部往底部方向分配内存，
也支持从底部往顶部分配内存。当 memblock_bottom_up() 函数返回 true 表示从底部往
顶部分配内存，也就是 bottom-up; 当 memblock_bottom_up() 函数返回 false 表示从
顶部往底部分配内存，就是 top-down 方式。

[GitHub: memblock_bottom_up](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_bottom_up)

##### <span id="memblock_set_bottom_up">memblock_set_bottom_up</span>

{% highlight c %}
/*
 * Set the allocation direction to bottom-up or top-down.
 */
static inline void __init memblock_set_bottom_up(bool enable)
{
	memblock.bottom_up = enable;
}
{% endhighlight %}

函数的作用就是设置 MEMBLOCK 分配的方向。 MEMBLOCK 支持从顶部往底部方向分配内存，
也支持从底部往顶部分配内存。当 enable 参数为 true， 表示从底部往顶部分配内存，
也就是 bottom-up; 当 enable 参数为 false 表示从顶部往底部分配内存，就是
top-down 方式。

[GitHub: memblock_set_bottom_up](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_set_bottom_up)

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

MEMBLOCK 提供了很多接口函数用于设置 MEMBLOCK 分配器的属性，以及读取 MEMBLOCK
分配器的相关信息，本次实践的目的就是使用这些接口函数。

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

#ifdef CONFIG_DEBUG_MEMBLOCK_HELPER
int __init debug_memblock_helper(void)
{
	struct memblock_region *reg;
	phys_addr_t size;
	phys_addr_t addr;
	phys_addr_t limit;
	bool state;
	int nid;

	/*
	 * Emulate memory
	 *
	 *                      memblock.memory
	 * 0     | <----------------------------------------> |
	 * +-----+---------+-------+----------+-------+-------+----+
	 * |     |         |       |          |       |       |    |
	 * |     |         |       |          |       |       |    |
	 * |     |         |       |          |       |       |    |
	 * +-----+---------+-------+----------+-------+-------+----+
	 *                 | <---> |          | <---> |
	 *                 Reserved 0         Reserved 1
	 *
	 * Memroy Region:   [0x60000000, 0xa0000000]
	 * Reserved Region: [0x80000000, 0x8d000000]
	 * Reserved Region: [0x90000000, 0x92000000]
	 */
	memblock_reserve(0x80000000, 0xd000000);
	memblock_reserve(0x90000000, 0x2000000);
	pr_info("Memory Regions:\n");
	for_each_memblock(memory, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
					reg->base + reg->size);
	pr_info("Reserved Regions:\n");
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
					reg->base + reg->size);

	/* Obtain memblock.memory total size */
	size = memblock_phys_mem_size();
	pr_info("Phyiscal Memory total size: %#x\n", size);

	/* Obtain memblock.reserved total size */
	size = memblock_reserved_size();
	pr_info("Reserved Memory total size: %#x\n", size);

	/* Obtain Start physical address of DRAM */
	addr = memblock_start_of_DRAM();
	pr_info("Start address of DRAM:      %#x\n", addr);

	/* Obtain End of physical address of DRAM */
	addr = memblock_end_of_DRAM();
	pr_info("End address of DRAM:        %#x\n", addr);

	/* Check address is memblock.reserved */
	addr = 0x81000000; /* Assume address in memblock.reserved */
	state = memblock_is_reserved(addr);
	if (state)
		pr_info("Address: %#x in reserved.\n", addr);

	/* Check address in memblock.memory */
	addr = 0x62000000; /* Assume address in memblock.memory */
	state = memblock_is_memory(addr);
	if (state)
		pr_info("Address: %#x in memory.\n", addr);

	/* Check region in memblock.memory */
	addr = 0x62000000;
	size = 0x100000; /* Assume [0x62000000, 0x62100000] in memory */
	state = memblock_is_region_memory(addr, size);
	if (state)
		pr_info("Region: [%#x - %#x] in memblock.memory.\n",
				addr, addr + size);

	/* Check region in memblock.reserved */
	addr = 0x80000000;
	size = 0x100000; /* Assume [0x80000000, 0x80100000] in reserved */
	state = memblock_is_region_reserved(addr, size);
	if (state)
		pr_info("Region: [%#x - %#x] in memblock.reserved.\n",
				addr, addr + size);

	/* Obtain current limit for memblock */
	limit = memblock_get_current_limit();
	pr_info("MEMBLOCK current_limit: %#x\n", limit);

	/* Set up current_limit for MEMBLOCK */
	memblock_set_current_limit(limit);

	/* Check memblock regions is hotpluggable */
	state = memblock_is_hotpluggable(&memblock.memory.regions[0]);
	if (state)
		pr_info("MEMBLOCK memory.regions[0] is hotpluggable.\n");
	else
		pr_info("MEMBLOCK memory.regions[0] is not hotpluggable.\n");

	/* Check memblock regions is mirror */
	state = memblock_is_mirror(&memblock.memory.regions[0]);
	if (state)
		pr_info("MEMBLOCK memory.regions[0] is mirror.\n");
	else
		pr_info("MEMBLOCK memory.regions[0] is not mirror.\n");

	/* Check memblock regions is nomap */
	state = memblock_is_nomap(&memblock.memory.regions[0]);
	if (state)
		pr_info("MEMBLOCK memory.regions[0] is nomap.\n");
	else
		pr_info("MEMBLOCK memory.regions[0] is not nomap.\n");

	/* Check region nid information */
	nid = memblock_get_region_node(&memblock.memory.regions[0]);
	pr_info("MEMBLOCK memory.regions[0] nid: %#x\n", nid);
	/* Set up region nid */
	memblock_set_region_node(&memblock.memory.regions[0], nid);

	/* Obtian MEMBLOCK allocator direction */
	state = memblock_bottom_up();
	pr_info("MEMBLOCK direction: %s", state ? "bottom-up" : "top-down");
	/* Set up MEMBLOCK allocate direction */
	memblock_set_bottom_up(state);

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
+config DEBUG_MEMBLOCK_PHYS_ALLOC_TRY_NID
+       bool "memblock_phys_alloc_try_nid()"
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
            [*]memblock_helper()
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
+#ifdef CONFIG_DEBUG_MEMBLOCK_HELPER
+	extern int bs_debug;
+	extern int debug_memblock_helper(void);
+#endif

 	setup_processor();
 	mdesc = setup_machine_fdt(__atags_pointer);
@@ -1104,6 +1108,10 @@ void __init setup_arch(char **cmdline_p)
 	strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
 	*cmdline_p = cmd_line;

ck_add
+#ifdef CONFIG_DEBUG_MEMBLOCK_HELPER
+	debug_memblock_helper();
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
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Phyiscal Memory total size: 0x40000000
Reserved Memory total size: 0xf000000
Start address of DRAM:      0x60000000
End address of DRAM:        0xa0000000
Address: 0x81000000 in reserved.
Address: 0x62000000 in memory.
Region: [0x62000000 - 0x62100000] in memblock.memory.
Region: [0x80000000 - 0x80100000] in memblock.reserved.
MEMBLOCK current_limit: 0xffffffff
MEMBLOCK memory.regions[0] is not hotpluggable.
MEMBLOCK memory.regions[0] is not mirror.
MEMBLOCK memory.regions[0] is not nomap.
MEMBLOCK memory.regions[0] nid: 0x0
MEMBLOCK direction: top-down
Malformed early option 'earlycon'
Memory policy: Data cache writeback
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

为了能让实践更具有说明性，在实践代码中添加了两块预留区，分别是：
[0x80000000, 0x8d000000] 和 [0x90000000, 0x92000000], 并调用
for_each_memblock() 函数遍历可用内存区和预留区，具体实践代码如下：

{% highlight bash %}
	/*
	 * Emulate memory
	 *
	 *                      memblock.memory
	 * 0     | <----------------------------------------> |
	 * +-----+---------+-------+----------+-------+-------+----+
	 * |     |         |       |          |       |       |    |
	 * |     |         |       |          |       |       |    |
	 * |     |         |       |          |       |       |    |
	 * +-----+---------+-------+----------+-------+-------+----+
	 *                 | <---> |          | <---> |
	 *                 Reserved 0         Reserved 1
	 *
	 * Memroy Region:   [0x60000000, 0xa0000000]
	 * Reserved Region: [0x80000000, 0x8d000000]
	 * Reserved Region: [0x90000000, 0x92000000]
	 */
	memblock_reserve(0x80000000, 0xd000000);
	memblock_reserve(0x90000000, 0x2000000);
	pr_info("Memory Regions:\n");
	for_each_memblock(memory, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
					reg->base + reg->size);
	pr_info("Reserved Regions:\n");
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
					reg->base + reg->size);
{% endhighlight %}

运行结果如下：

{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
{% endhighlight %}

接着调用 memblock_phys_mem_siz() 函数获得当前物理内存的总体积，代码如下：

{% highlight bash %}
	/* Obtain memblock.memory total size */
	size = memblock_phys_mem_size();
	pr_info("Phyiscal Memory total size: %#x\n", size);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Phyiscal Memory total size: 0x40000000
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_reserved_size() 函数
获得预留区大小。具体实践代码如下：

{% highlight bash %}
	/* Obtain memblock.reserved total size */
	size = memblock_reserved_size();
	pr_info("Reserved Memory total size: %#x\n", size);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Reserved Memory total size: 0xf000000
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_start_of_DRAM() 函数
获得 DRAM 的起始物理地址。具体实践代码如下：

{% highlight bash %}
	/* Obtain Start physical address of DRAM */
	addr = memblock_start_of_DRAM();
	pr_info("Start address of DRAM:      %#x\n", addr);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Start address of DRAM:      0x60000000
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_end_of_DRAM() 函数
获得 DRAM 的终止物理地址。具体实践代码如下：

{% highlight bash %}
	/* Obtain End of physical address of DRAM */
	addr = memblock_end_of_DRAM();
	pr_info("End address of DRAM:        %#x\n", addr);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
End address of DRAM:        0xa0000000
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_is_reserved() 函数
检查 0x81000000 地址是否位于预留区内。具体实践代码如下：

{% highlight bash %}
	/* Check address is memblock.reserved */
	addr = 0x81000000; /* Assume address in memblock.reserved */
	state = memblock_is_reserved(addr);
	if (state)
		pr_info("Address: %#x in reserved.\n", addr);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Address: 0x81000000 in reserved.
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_is_memory() 函数
检查 0x62000000 地址是否位于可用内存区。具体实践代码如下：

{% highlight bash %}
	/* Check address in memblock.memory */
	addr = 0x62000000; /* Assume address in memblock.memory */
	state = memblock_is_memory(addr);
	if (state)
		pr_info("Address: %#x in memory.\n", addr);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Address: 0x62000000 in memory.
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_is_region_reserved() 函数
检查 [0x80000000, 0x80100000] 是否属于预留区。具体实践代码如下：

{% highlight bash %}
	/* Check region in memblock.reserved */
	addr = 0x80000000;
	size = 0x100000; /* Assume [0x80000000, 0x80100000] in reserved */
	state = memblock_is_region_reserved(addr, size);
	if (state)
		pr_info("Region: [%#x - %#x] in memblock.reserved.\n",
				addr, addr + size);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Region: [0x80000000 - 0x80100000] in memblock.reserved.
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_is_region_memory() 函数
检查 [0x62000000, 0x62100000] 是否属于可用内存区。具体实践代码如下：

{% highlight bash %}
	/* Check region in memblock.memory */
	addr = 0x62000000;
	size = 0x100000; /* Assume [0x62000000, 0x62100000] in memory */
	state = memblock_is_region_memory(addr, size);
	if (state)
		pr_info("Region: [%#x - %#x] in memblock.memory.\n",
				addr, addr + size);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
Region: [0x62000000 - 0x62100000] in memblock.memory.
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_get_current_limit() 函数
检查获得 MEMBLOCK 当前 limit，以及调用 memblock_set_current_limit()
函数设置 MEMBLOCK 当前 limit。具体实践代码如下：

{% highlight bash %}
	/* Obtain current limit for memblock */
	limit = memblock_get_current_limit();
	pr_info("MEMBLOCK current_limit: %#x\n", limit);

	/* Set up current_limit for MEMBLOCK */
	memblock_set_current_limit(limit);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
MEMBLOCK current_limit: 0xffffffff
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_is_hotpluggable()
函数，以此检查 memory.regions[0] 是否属于热插拔。具体实践代码如下：

{% highlight bash %}
	/* Check memblock regions is hotpluggable */
	state = memblock_is_hotpluggable(&memblock.memory.regions[0]);
	if (state)
		pr_info("MEMBLOCK memory.regions[0] is hotpluggable.\n");
	else
		pr_info("MEMBLOCK memory.regions[0] is not hotpluggable.\n");
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
MEMBLOCK memory.regions[0] is not hotpluggable.
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_is_mirror()
函数，以此检查 memory.regions[0] 是否属于 mirror。具体实践代码如下：

{% highlight bash %}
	/* Check memblock regions is mirror */
	state = memblock_is_mirror(&memblock.memory.regions[0]);
	if (state)
		pr_info("MEMBLOCK memory.regions[0] is mirror.\n");
	else
		pr_info("MEMBLOCK memory.regions[0] is not mirror.\n");
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
MEMBLOCK memory.regions[0] is not mirror.
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_is_nomap()
函数，以此检查 memory.regions[0] 是否属于 mirror。具体实践代码如下：

{% highlight bash %}
	/* Check memblock regions is nomap */
	state = memblock_is_nomap(&memblock.memory.regions[0]);
	if (state)
		pr_info("MEMBLOCK memory.regions[0] is nomap.\n");
	else
		pr_info("MEMBLOCK memory.regions[0] is not nomap.\n");
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
MEMBLOCK memory.regions[0] is not nomap.
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_get_region_node()
函数和 memblock_set_region_node() 函数获得和设置特定内存区块的 NUMA 号，
具体实践代码如下：

{% highlight bash %}
	/* Check region nid information */
	nid = memblock_get_region_node(&memblock.memory.regions[0]);
	pr_info("MEMBLOCK memory.regions[0] nid: %#x\n", nid);
	/* Set up region nid */
	memblock_set_region_node(&memblock.memory.regions[0], nid);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
MEMBLOCK memory.regions[0] nid: 0x0
{% endhighlight %}

运行结果和实际真实值一致。接下来调用 memblock_bottom_up()
函数和 memblock_set_bottom_up() 函数获得和设置 MEMBLOCK 的
分配方向，具体实践代码如下：

{% highlight bash %}
	/* Obtian MEMBLOCK allocator direction */
	state = memblock_bottom_up();
	pr_info("MEMBLOCK direction: %s", state ? "bottom-up" : "top-down");
	/* Set up MEMBLOCK allocate direction */
	memblock_set_bottom_up(state);
{% endhighlight %}

运行结果如下：
{% highlight bash %}
Memory Regions:
Region: 0x60000000 - 0xa0000000
Reserved Regions:
Region: 0x80000000 - 0x8d000000
Region: 0x90000000 - 0x92000000
MEMBLOCK direction: top-down
{% endhighlight %}
