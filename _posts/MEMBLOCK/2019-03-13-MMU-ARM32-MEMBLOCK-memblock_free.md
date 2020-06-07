---
layout: post
title:  "memblock_free() 移除一段预留内存"
date:   2019-03-13 08:07:30 +0800
categories: [MMU]
excerpt: memblock_free() 移除一段预留内存.
tags:
  - MMU
---

> [GitHub: MEMBLOCK memblock_free()](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_free)
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

通过该数据结构，可以知道内存区块起始的物理地址和所占用的体积。内存区块被维护在不同的
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

函数： memblock_free()

功能：从预留内存区中移除一块预留的内存区块，具体调用树如下：

{% highlight bash %}
memblock_free
|
|---memblock_isolate_range
|   |
|   |---for_each_memblock_type
|   |
|   |---memblock_insert_region
|
|---memblock_remove_region
{% endhighlight %}

##### memblock_free

{% highlight c %}
/**
 * memblock_free - free boot memory block
 * @base: phys starting address of the  boot memory block
 * @size: size of the boot memory block in bytes
 *
 * Free boot memory block previously allocated by memblock_alloc_xx() API.
 * The freeing memory will not be released to the buddy allocator.
 */
int __init_memblock memblock_free(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("   memblock_free: [%pa-%pa] %pF\n",
		     &base, &end, (void *)_RET_IP_);

	kmemleak_free_part_phys(base, size);
	return memblock_remove_range(&memblock.reserved, base, size);
}
{% endhighlight %}

参数 base 指向需要移除预留内存的起始物理地址；size 指向需要移除预留内存
的大小。

函数计算出移除物理内存的终止物理地址之后，直接调用 memblock_remove_range()
函数来移除指定的预留内存区。

##### memblock_remove_range

{% highlight c %}
static int __init_memblock memblock_remove_range(struct memblock_type *type,
					  phys_addr_t base, phys_addr_t size)
{
	int start_rgn, end_rgn;
	int i, ret;

	ret = memblock_isolate_range(type, base, size, &start_rgn, &end_rgn);
	if (ret)
		return ret;

	for (i = end_rgn - 1; i >= start_rgn; i--)
		memblock_remove_region(type, i);
	return 0;
}
{% endhighlight %}

参数 type 指明了要从那一块内存区移除物理内存，这里可以是预留区或可用物理内存区；
参数 base 指向需要移除物理内存的起始物理地址；size 指向需要移除物理内存的大小。

要移除的内存区可能与内存区内的内存块存在重叠部分，对于这种情况，函数首先调用
memblock_isolate_range() 函数将要移除的物理内存区从内存区中独立出来，然后
记录这些重叠的内存区的索引号，接着调用 memblock_remove_region() 函数将这些索引
对应的内存区块从内存区中移除。

##### memblock_isolate_range

函数的作用就是将重叠的物理内存区块从内存区独立分割成多块。函数较长，分段解析

{% highlight c %}
/**
 * memblock_isolate_range - isolate given range into disjoint memblocks
 * @type: memblock type to isolate range for
 * @base: base of range to isolate
 * @size: size of range to isolate
 * @start_rgn: out parameter for the start of isolated region
 * @end_rgn: out parameter for the end of isolated region
 *
 * Walk @type and ensure that regions don't cross the boundaries defined by
 * [@base, @base + @size).  Crossing regions are split at the boundaries,
 * which may create at most two more regions.  The index of the first
 * region inside the range is returned in *@start_rgn and end in *@end_rgn.
 *
 * Return:
 * 0 on success, -errno on failure.
 */
static int __init_memblock memblock_isolate_range(struct memblock_type *type,
					phys_addr_t base, phys_addr_t size,
					int *start_rgn, int *end_rgn)
{
	phys_addr_t end = base + memblock_cap_size(base, &size);
	int idx;
	struct memblock_region *rgn;
}
{% endhighlight %}

参数 type 指向一类内存区; base 指向需要移除内存区的起始物理地址；size 指向
需要移除物理内存的大小; start_rgn 用于存储分割之后内存区块在内存区的开始索引；
end_rgn 用于存储分割之后内存区块在内存区的结束索引。

{% highlight c %}
*start_rgn = *end_rgn = 0;

if (!size)
  return 0;

/* we'll create at most two more regions */
while (type->cnt + 2 > type->max)
  if (memblock_double_array(type, base, size) < 0)
    return -ENOMEM;
{% endhighlight %}

由于将重叠部分拆分成新的内存区块会增加内存区块的数量，如果内存区包含的内存区块
数大于一定的值之后，就调用 memblock_double_array() 函数增加内存区的内存区块数。

通过参数 base 和 size 构成的内存区块和在内存区存在的内存区块可能存在多种位置
关系。每种位置关系函数需要进行不同的处理，这里对每种位置关系进行分析，函数首先
调用 for_each_memblock_type() 函数遍历内存区的所用内存区块，并记录正在遍历内
存区块的索引。具体代码如下：

{% highlight c %}
for_each_memblock_type(idx, type, rgn) {
  phys_addr_t rbase = rgn->base;
  phys_addr_t rend = rbase + rgn->size;

  if (rbase >= end)
    break;
  if (rend <= base)
    continue;
{% endhighlight %}

从上面的代码，可以处理四种位置管理，分别为：

> rend <=base

这种情况，要移除的内存区块位于已存在内存区块的后边，或者要移除的物理内存区块
位于已存在内存区块的后面，但与已存在的内存区块相连，如下图：

{% highlight bash %}
rend < base

            System memory                         Remove region
           | <----------> |                         | <-> |
 +---------+--------------+--------+-------------+--+-----+--+
 |         |              |        |             |  |     |  |
 |         |              |        |             |  |     |  |
 |         |              |        |             |  |     |  |
 +---------+--------------+--------+-------------+--+-----+--+
                                   | <---------> |
                                    Pseudo memory


rend == base


            System memory                       Remove region
           | <----------> |                      | <----> |
 +---------+--------------+--------+-------------+--------+--+
 |         |              |        |             |        |  |
 |         |              |        |             |        |  |
 |         |              |        |             |        |  |
 +---------+--------------+--------+-------------+--------+--+
                                   | <---------> |
                                    Pseudo memory
{% endhighlight %}

对于这种情况，函数是不进行处理的，因为要移除的内存区块不与现有的内存区块
存在重叠现象。但后面的内存区块可能与要移除的内存区块重叠，因此直接调用
continue 进行下一个内存区的处理。

> rbase >= end

这种情况下，要移除的内存区块位于已有内存区的前段，可能与现有的内存区块相连，
也可能不相连，如下图：

{% highlight bash %}
rbase > end

            System memory  Remove region
           | <----------> |  |<->|
 +---------+--------------+--+---+-+-------------+-----------+
 |         |              |  |   | |             |           |
 |         |              |  |   | |             |           |
 |         |              |  |   | |             |           |
 +---------+--------------+--+---+-+-------------+-----------+
                                   | <---------> |
                                    Pseudo memory


rbase == end

            System memory   Remove region
           | <----------> |  | <-> |
 +---------+--------------+--+-----+-------------+-----------+
 |         |              |  |     |             |           |
 |         |              |  |     |             |           |
 |         |              |  |     |             |           |
 +---------+--------------+--+-----+-------------+-----------+
                                   | <---------> |
                                    Pseudo memory
{% endhighlight %}

对于这种情况，要移除的内存区块不与已存在的物理内存区块重叠，由于内存区内的
内存区块按低地址到高地址的排列，所以剩下的物理内存区块不能看再与要移除的内
存区块存在重叠，所以函数直接调用 break 结束循环。

{% highlight c %}
if (rbase < base) {
  /*
   * @rgn intersects from below.  Split and continue
   * to process the next region - the new top half.
   */
  rgn->base = base;
  rgn->size -= base - rbase;
  type->total_size -= base - rbase;
  memblock_insert_region(type, idx, rbase, base - rbase,
             memblock_get_region_node(rgn),
             rgn->flags);
}
{% endhighlight %}

接下来函数处理 rbase 小于 base 的情况，对于这种情况，可以简单的归类为要移除的
内存区块和已存在的内存区存在重叠，但要移除的内存区块的起始地址在已存在的内存区块
之后，符合这种情况的位置关系如下图：

{% highlight bash %}
1) rbase < base && rend < end

            System memory                    Remove region
           | <----------> |                 | <---------> |
 +---------+--------------+--------+--------+----+--------+--+
 |         |              |        |        |    |        |  |
 |         |              |        |        |    |        |  |
 |         |              |        |        |    |        |  |
 +---------+--------------+--------+--------+----+--------+--+
                                   | <---------> |
                                    Pseudo memory

2) rbase < base && end < rend

            System memory           Remove region
           | <----------> |           | <-> |
 +---------+--------------+--------+--+-----+----+-----------+
 |         |              |        |  |     |    |           |
 |         |              |        |  |     |    |           |
 |         |              |        |  |     |    |           |
 +---------+--------------+--------+--+-----+----+-----------+
                                   | <---------> |
                                    Pseudo memory
{% endhighlight %}

对于这种情况，函数将已存在的内存区块拆成多块，对于上面的情况 1：
[rbase < base && rend < end]，那么函数会将已存在的内存区块拆做两块内存区块，
其范围是：[rbase, base] 与 [base, remd]，函数将这两个内存块都存在内存区内，
并且进行下一次循环。如果遇到情况 2：[rbase < base && end < rend],那么内存区
会被拆分成三部分，第一部分是：[rbase, base]， 第二部分是：[base, end]， 第三
部分是：[end, rend]，函数会将这三部分都添加到内存区内。上面的情况，idx 对应被
拆分出来的第一个内存区块，所以下一次循环中，将循环新拆分出来的第二个内存区块。

正如代码所示，对于第一个新拆出的内存区块，函数调用 memblock_insert_region()
函数将其添加到内存区，并且 idx 指向这个区；接着将原始的内存区块更新称为第二个
内存区块，并将更新范围。

{% highlight c %}
if (rend > end) {
			/*
			 * @rgn intersects from above.  Split and redo the
			 * current region - the new bottom half.
			 */
			rgn->base = end;
			rgn->size -= end - rbase;
			type->total_size -= end - rbase;
			memblock_insert_region(type, idx--, rbase, end - rbase,
					       memblock_get_region_node(rgn),
					       rgn->flags);
		}
{% endhighlight %}

对于这种情况，函数要移除的物理内存区块与已存在的内存区块存在重合部分，但移除的
物理内存区块的终止地址小于已存在内存区块的终止地址，其位置关系下图：

{% highlight bash %}
1) rbase < base && end < rend

            System memory           Remove region
           | <----------> |           | <-> |
 +---------+--------------+--------+--+-----+----+-----------+
 |         |              |        |  |     |    |           |
 |         |              |        |  |     |    |           |
 |         |              |        |  |     |    |           |
 +---------+--------------+--------+--+-----+----+-----------+
                                   | <---------> |
                                    Pseudo memory

2) end < rend && base < rbase

            System memory     Remove region
           | <----------> |   | <-------> |
 +---------+--------------+---+----+------+------+-----------+
 |         |              |   |    |      |      |           |
 |         |              |   |    |      |      |           |
 |         |              |   |    |      |      |           |
 +---------+--------------+---+----+------+------+-----------+
                                   | <---------> |
                                    Pseudo memory
{% endhighlight %}

对于这两种情况，函数都会将重叠部分拆做两部分，第一部分的范围是：[rbase, end],
第二部分范围是：[end, rend]。函数将第一部分通过函数 memblock_insert_region()
函数插入到内存区内，并且 idx 标记为前一个，这样在下次遍历的时候，就会从第一部分
的内存区开始遍历，第二部分则通过当前内存区调整得来。

{% highlight c %}
			/* @rgn is fully contained, record it */
			if (!*end_rgn)
				*start_rgn = idx;
			*end_rgn = idx + 1;
{% endhighlight %}

最后函数将已存在内存区与要移除的内存区块一样大小的内存区块的索引，存储在
start_rgn 和 end_rgn 中。

##### memblock_remove_range

{% highlight c %}
static void __init_memblock memblock_remove_region(struct memblock_type *type, unsigned long r)
{
	type->total_size -= type->regions[r].size;
	memmove(&type->regions[r], &type->regions[r + 1],
		(type->cnt - (r + 1)) * sizeof(type->regions[r]));
	type->cnt--;

	/* Special case for empty arrays */
	if (type->cnt == 0) {
		WARN_ON(type->total_size != 0);
		type->cnt = 1;
		type->regions[0].base = 0;
		type->regions[0].size = 0;
		type->regions[0].flags = 0;
		memblock_set_region_node(&type->regions[0], MAX_NUMNODES);
	}
}
{% endhighlight %}

参数 type 指向内存类型；参数 r 指向移除的索引值

函数的作用是移除 r 索引对应的内存区块。函数说先调用 memmove 函数将 r 索引
之后的内存区块全部往前挪一个位置，这样 r 索引对应的内存区块就被移除了。如果
移除之后，内存区块不含有任何内存区块，那么就初始化该内存区。

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

memblock_free() 函数的作用是从预留内存区内移除指定大小的预留内存区块，
本次实践的目的就是从预留内存区内移除内存区块。

#### <span id="驱动实践准备">实践准备</span>

由于本次实践是基于 Linux 5.x 的 arm32 系统，所以请先参考 Linux 5.x arm32 开发环境
搭建方法以及重点关注驱动实践一节，请参考下例文章，选择一个 linux 5.x 版本进行实践，后
面内容均基于 linux 5.x 继续讲解，文章链接如下：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

#### <span id="驱动源码">驱动源码</span>

准备好开发环境之后，下一步就是准备实践所用的驱动源码，驱动的源码如下：

{% highlight c %}
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

#ifdef CONFIG_DEBUG_MEMBLOCK_FREE
/*
 * Free an reserved region.
 */
int debug_memblock_free(void)
{
	struct memblock_region *reg;

	/*
	 * Emulate memory
	 *
	 *                   memblock.memory
	 *     | <--------------------------------------> |
	 * +---+----------------------+--------------+----+-------+
	 * |   |                      |              |    |       |
	 * |   |                      |              |    |       |
	 * |   |                      |              |    |       |
	 * +---+----------------------+--------------+----+-------+
	 *                            | <----------> |
	 *                            memblock.reserved
	 *
	 * Memory Region:   [0x60000000, 0x80000000]
	 * Reserved Region: [0x78000000, 0x7a000000]
	 * Free Region:     [0x78000000, 0x79000000]
	 */
	memblock_reserve(0x78000000, 0x2000000);
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
				reg->base + reg->size);

	/*
	 * Processing free
	 *
	 *                   memblock.memory
	 *     | <--------------------------------------> |
	 * +---+--------------------------------+-----+----+-------+
	 * |   |                                |     |    |       |
	 * |   |                                |     |    |       |
	 * |   |                                |     |    |       |
	 * +---+--------------------------------+-----+----+-------+
	 *                                      | <-> |
	 *                                    Remain region
	 *
	 * Memory Region:   [0x60000000, 0x80000000]
	 * Reserved Region: [0x79000000, 0x7a000000]
	 */
	memblock_free(0x78000000, 0x1000000);
	pr_info("Free special region.\n");
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
				reg->base + reg->size);

	/* Clear debug case */
	memblock.reserved.cnt = 1;
	memblock.reserved.total_size = 0;

	return 0;
}
#endif
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

由于这部分驱动涉及到较早的内核启动接管，所以不能直接以模块的形式编入到内核，需要直接
编译进内核，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 memblock.c，然后修改
Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_MEMBLOCK
+       bool "Memblock allocator"
+
+if BISCUITOS_MEMBLOCK
+
+config DEBUG_MEMBLOCK_FREE
+       bool "memblock_free()"
+
+endif # BISCUITOS_MEMBLOCK
+
endif # BISCUITOS_DRV
{% endhighlight %}

接着修改 Makefile，请参考如下修改：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Makefile b/drivers/BiscuitOS/Makefile
index 82004c9..9909149 100644
--- a/drivers/BiscuitOS/Makefile
+++ b/drivers/BiscuitOS/Makefile
@@ -1 +1,2 @@
obj-$(CONFIG_BISCUITOS_MISC)     += BiscuitOS_drv.o
+obj-$(CONFIG_BISCUITOS_MEMBLOCK) += memblock.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]Memblock allocator
            [*]memblock_free()
{% endhighlight %}

具体过程请参考：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

#### <span id="驱动增加调试点">增加调试点</span>

驱动运行还需要在内核的指定位置添加调试点，由于该驱动需要在内核启动阶段就使用，请参考下
面 patch 将源码指定位置添加调试代码：

{% highlight c %}
diff --git a/arch/arm/kernel/setup.c b/arch/arm/kernel/setup.c
index 375b13f7e..fec6919a9 100644
--- a/arch/arm/kernel/setup.c
+++ b/arch/arm/kernel/setup.c
@@ -1073,6 +1073,10 @@ void __init hyp_mode_check(void)
 void __init setup_arch(char **cmdline_p)
 {
 	const struct machine_desc *mdesc;
+#ifdef CONFIG_DEBUG_MEMBLOCK_FREE
+	extern int bs_debug;
+	extern int debug_memblock_free(void);
+#endif

 	setup_processor();
 	mdesc = setup_machine_fdt(__atags_pointer);
@@ -1104,6 +1108,10 @@ void __init setup_arch(char **cmdline_p)
 	strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
 	*cmdline_p = cmd_line;

+#ifdef CONFIG_DEBUG_MEMBLOCK_FREE
+	debug_memblock_free();
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
Region: 0x78000000 - 0x7a000000
Free special region.
Region: 0x79000000 - 0x7a000000
Malformed early option 'earlycon'
Memory policy: Data cache writeback
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在实践过程中，由于 memblock_free() 函数可以从预留区内移除内存区块，那么先
调用 memblock_reserve() 函数构建一段预留内存区，将 [0x78000000, 0x7a000000]
作为一段预留内存区，实践代码如下：

{% highlight c %}
	/*
	 * Emulate memory
	 *
	 *                   memblock.memory
	 *     | <--------------------------------------> |
	 * +---+----------------------+--------------+----+-------+
	 * |   |                      |              |    |       |
	 * |   |                      |              |    |       |
	 * |   |                      |              |    |       |
	 * +---+----------------------+--------------+----+-------+
	 *                            | <----------> |
	 *                            memblock.reserved
	 *
	 * Memory Region:   [0x60000000, 0x80000000]
	 * Reserved Region: [0x78000000, 0x7a000000]
	 * Free Region:     [0x78000000, 0x79000000]
	 */
	memblock_reserve(0x78000000, 0x2000000);
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
				reg->base + reg->size);
{% endhighlight %}

接下来调用 memblock_free() 函数移除 [0x78000000, 0x79000000] 的预留区，
实践代码如下：

{% highlight c %}
	/*
	 * Processing free
	 *
	 *                   memblock.memory
	 *     | <--------------------------------------> |
	 * +---+--------------------------------+-----+----+-------+
	 * |   |                                |     |    |       |
	 * |   |                                |     |    |       |
	 * |   |                                |     |    |       |
	 * +---+--------------------------------+-----+----+-------+
	 *                                      | <-> |
	 *                                    Remain region
	 *
	 * Memory Region:   [0x60000000, 0x80000000]
	 * Reserved Region: [0x79000000, 0x7a000000]
	 */
	memblock_free(0x78000000, 0x1000000);
	pr_info("Free special region.\n");
	for_each_memblock(reserved, reg)
		pr_info("Region: %#x - %#x\n", reg->base,
				reg->base + reg->size);
{% endhighlight %}

运行结果如下：

{% highlight bash %}
Region: 0x78000000 - 0x7a000000
Free special region.
Region: 0x79000000 - 0x7a000000
{% endhighlight %}

调用 memblock_free() 函数之后，0x79000000 到 0x7a000000 内存区块从预留区中移
除了，但并没有将所有的预留区块移除，[0x79000000 - 0x7a000000] 没有被移除。更多
原理分析，请看源码分析部分。

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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
