---
layout: post
title:  "memblock_reserve() Reserved 分配第一个内存区块"
date:   2019-03-06 13:56:30 +0800
categories: [MMU]
excerpt: memblock_reserve() Reserved 分配第一个内存区块.
tags:
  - MMU
---

> [GitHub: BiscuitOS](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/memblock_reserve)
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

函数： memblock_reserve()

功能：将一块物理内存加入到预留内存区内。本文分析的情况是：当原始预留区为空时，加入第一
块内存区块时，memblock_reserve() 会将这块物理内存块放到 regions 链表的头部。具体调
用树如下：

{% highlight bash %}
memblock_reserve
|
|---memblock_reserve
{% endhighlight %}

##### memblock_reserve

{% highlight c %}
int __init_memblock memblock_reserve(phys_addr_t base, phys_addr_t size)
{
    phys_addr_t end = base + size - 1;

    memblock_dbg("memblock_reserve: [%pa-%pa] %pF\n",
             &base, &end, (void *)_RET_IP_);

    return memblock_add_range(&memblock.reserved, base, size, MAX_NUMNODES, 0);
}
{% endhighlight %}

参数 base 指向内存区块的物理基地址，size 参数指定了内存区块的大小

函数首先定义了一个 phys_addr_t 变量 end，其值等于该内存区块最后一个物理地址。接着将
memblock.reserved 预留内存的指针，内存区块基地址 base，内存区块长度 size，以及
MAX_NUMNODES 宏传入 memblock_add_range() 函数中，并且该函数的最后一个参数为 0.

##### memblock_add_range

{% highlight c %}
/**
* memblock_add_range - add new memblock region
* @type: memblock type to add new region into
* @base: base address of the new region
* @size: size of the new region
* @nid: nid of the new region
* @flags: flags of the new region
*
* Add new memblock region [@base, @base + @size) into @type.  The new region
* is allowed to overlap with existing ones - overlaps don't affect already
* existing regions.  @type is guaranteed to be minimal (all neighbouring
* compatible regions are merged) after the addition.
*
* Return:
* 0 on success, -errno on failure.
*/
int __init_memblock memblock_add_range(struct memblock_type *type,
                phys_addr_t base, phys_addr_t size,
                int nid, enum memblock_flags flags)
{
    bool insert = false;
    phys_addr_t obase = base;
    phys_addr_t end = base + memblock_cap_size(base, &size);
    int idx, nr_new;
    struct memblock_region *rgn;

    if (!size)
        return 0;

    /* special case for empty array */
    if (type->regions[0].size == 0) {
        WARN_ON(type->cnt != 1 || type->total_size);
        type->regions[0].base = base;
        type->regions[0].size = size;
        type->regions[0].flags = flags;
        memblock_set_region_node(&type->regions[0], nid);
        type->total_size = size;
        return 0;
    }
repeat:
    /*
     * The following is executed twice.  Once with %false @insert and
     * then with %true.  The first counts the number of regions needed
     * to accommodate the new area.  The second actually inserts them.
     */
    base = obase;
    nr_new = 0;

    for_each_memblock_type(idx, type, rgn) {
        phys_addr_t rbase = rgn->base;
        phys_addr_t rend = rbase + rgn->size;

        if (rbase >= end)
            break;
        if (rend <= base)
            continue;
        /*
         * @rgn overlaps.  If it separates the lower part of new
         * area, insert that portion.
         */
        if (rbase > base) {
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
            WARN_ON(nid != memblock_get_region_node(rgn));
#endif
            WARN_ON(flags != rgn->flags);
            nr_new++;
            if (insert)
                memblock_insert_region(type, idx++, base,
                               rbase - base, nid,
                               flags);
        }
        /* area below @rend is dealt with, forget about it */
        base = min(rend, end);
    }

    /* insert the remaining portion */
    if (base < end) {
        nr_new++;
        if (insert)
            memblock_insert_region(type, idx, base, end - base,
                           nid, flags);
    }

    if (!nr_new)
        return 0;

    /*
     * If this was the first round, resize array and repeat for actual
     * insertions; otherwise, merge and return.
     */
    if (!insert) {
        while (type->cnt + nr_new > type->max)
            if (memblock_double_array(type, obase, size) < 0)
                return -ENOMEM;
        insert = true;
        goto repeat;
    } else {
        memblock_merge_regions(type);
        return 0;
    }
}
{% endhighlight %}

函数的作用是向内存区中添加一块新的内存区块
代码很长，分段解析：

{% highlight c %}
/**
* memblock_add_range - add new memblock region
* @type: memblock type to add new region into
* @base: base address of the new region
* @size: size of the new region
* @nid: nid of the new region
* @flags: flags of the new region
*
* Add new memblock region [@base, @base + @size) into @type.  The new region
* is allowed to overlap with existing ones - overlaps don't affect already
* existing regions.  @type is guaranteed to be minimal (all neighbouring
* compatible regions are merged) after the addition.
*
* Return:
* 0 on success, -errno on failure.
*/
int __init_memblock memblock_add_range(struct memblock_type *type,
                phys_addr_t base, phys_addr_t size,
                int nid, enum memblock_flags flags)
{
    bool insert = false;
    phys_addr_t obase = base;
    phys_addr_t end = base + memblock_cap_size(base, &size);
    int idx, nr_new;
    struct memblock_region *rgn;

    if (!size)
        return 0;
{% endhighlight %}

参数 type 指向了内存区，由上面调用的函数可知，这里指向预留内存区；base 指向


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

memblock_reserve() 函数的作用就是将一块物理内存区块加入到预留物理内存内。由于原始的
预留区内不包含任何内存区块，所以本次实践的目的就是往一个空的预留内存区中加入一块新的内
存区块。在加入前和加入后，都会去遍历整个预留区的所有内存区块。

#### <span id="驱动实践准备">实践准备</span>

由于本次实践是基于 Linux 5.x 的 arm32 系统，所以请先参考 Linux 5.x arm32 开发环境
搭建方法以及重点关注驱动实践一节，因为接下来所讲的内容都和驱动有关，文章链接如下：


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
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>

#ifdef CONFIG_BISCUITOS_MEMBLOCK_RESERVE
/*
* Mark memory as reserved on memblock.reserved regions.
*/
int debug_memblock_reserve(void)
{
        struct memblock_region *reg;

        /* Scan old reserved region */
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /* Reserved memblock region is empty and insert a new region
         *
         * memblock.reserved--->+--------+
         *                      |        |
         *                      | Empty- |
         *                      |        |
         *                      +--------+
         */
        memblock_reserve(0x60000000, 0x200000);
        pr_info("Scan first region:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region which behine and disjunct an exist region:
         *
         * memblock.reserved:
         *
         * rbase                rend         base                  end
         * +--------------------+            +---------------------+
         * |                    |            |                     |
         * |   Exist regions    |            |      new region     |
         * |                    |            |                     |
         * +--------------------+            +---------------------+
         *
         * 1) rend < base
         * 2) rbase: 0x60000000
         *    rend:  0x60200000
         *    base:  0x62000000
         *    end:   0x62200000
         *
         * Processing: Insert/Merge
         *
         * rbase                rend         rbase                 rend
         * +--------------------+            +---------------------+
         * |                    |            |                     |
         * |   Exist regions    |            |    Exist regions    |
         * |                    |            |                     |
         * +--------------------+            +---------------------+
         *
         * 1) rbase: 0x60000000
         *    rend:  0x60200000
         *    base:  0x62000000
         *    end:   0x62200000
         *
         */
        memblock_reserve(0x62000000, 0x200000);
        pr_info("Scan behine and disjunct region:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region which behine and conjunct an exist region:
         *
         * memblock.reserved:
         *                      base
         * rbase                rend                     end
         * +--------------------+------------------------+
         * |                    |                        |
         * |   Exist regions    |       new region       |
         * |                    |                        |
         * +--------------------+------------------------+
         *
         * 1) base == rend
         * 2) rbase: 0x62000000
         *    rend:  0x62200000
         *    base:  0x62200000
         *    end:   0x62400000
         *
         * Processing: Insert/Merge
         *
         * rbase                                         rend
         * +---------------------------------------------+
         * |                                             |
         * |   Exist regions                             |
         * |                                             |
         * +---------------------------------------------+
         *
         * 1) rbase: 0x62000000
         *    rend:  0x62400000
         */
        memblock_reserve(0x62200000, 0x200000);
        pr_info("Scan behine and conjunct region:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region that part of new region contains by exist
         * regions and part of new region behine exist regions:
         *
         * memblock.reserved:
         *
         * rbase                     rend
         * | <---------------------> |
         * +----------------+--------+----------------------+
         * |                |        |                      |
         * | Exist regions  |        |     new region       |
         * |                |        |                      |
         * +----------------+--------+----------------------+
         *                  | <---------------------------> |
         *                  base                            end
         *
         * 1) base > rebase
         * 2) end  > rend
         * 3) base > rend
         * 4) rbase: 0x62000000
         *    rend:  0x62400000
         *    base:  0x62300000
         *    end:   0x62500000
         *
         * Processing: Insert/Merge
         *
         * rbase                                            rend
         * +------------------------------------------------+
         * |                                                |
         * | Exist regions                                  |
         * |                                                |
         * +------------------------------------------------+
         *
         * 1) rbase: 0x62000000
         *    rend:  0x62500000
         */
        memblock_reserve(0x62300000, 0x200000);
        pr_info("Scan behine but contain regions:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region that contain an exist regions which base
         * address is equal to new region. The end address of new region
         * is big than exist regions.
         *
         * memblock.reserved:
         *
         * rbase:             rend
         * | <--------------> |
         * +------------------+-----------------------------+
         * |                  |                             |
         * |  Exist regions   |        new region           |
         * |                  |                             |
         * +------------------+-----------------------------+
         * | <--------------------------------------------> |
         * base                                             end
         *
         * 1) base == rbase
         * 2) rend < end
         * 3) rbase: 0x62000000
         *    rend:  0x62500000
         *    base:  0x62000000
         *    end:   0x62600000
         *
         * Processing: Insert/Merge
         *
         * rbase                                            rend
         * +------------------------------------------------+
         * |                                                |
         * |  Exist regions                                 |
         * |                                                |
         * +------------------------------------------------+
         *
         * 1) rbase: 0x62000000
         *    rend:  0x62600000
         *
         */
        memblock_reserve(0x62000000, 0x600000);
        pr_info("Scan contain but equal regions:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region that contain whole exist regions.
         *
         * memblock.reserved:
         *
         * base             New regions                     end
         * | <--------------------------------------------> |
         * +-----------+--------------------+---------------+
         * |           |                    |               |
         * |           |   Exist regions    |               |
         * |           |                    |               |
         * +-----------+--------------------+---------------+
         *             | <----------------> |
         *             rbase                rend
         *
         * 1) base < rbase
         * 2) rend < end
         * 3) rbase: 0x62000000
         *    rend:  0x62600000
         *    base:  0x61000000
         *    end:   0x63000000
         *
         * Processing: Insert/Merge
         *
         * rbase                                            rend
         * +------------------------------------------------+
         * |                                                |
         * |              Exist Regions                     |
         * |                                                |
         * +------------------------------------------------+
         *
         * 1) rbase: 0x61000000
         *    rend:  0x63000000
         */
        memblock_reserve(0x61000000, 0x2000000);
        pr_info("Scan region which contain exist one:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region into memblock.reserved regions that new region
         * contain by exist regions, but the base address of new regions is
         * big than exist regions, and end address of new regions is equal
         * to exist regions.
         *
         * memblock.reserved:
         *
         * rbase           Exist regions                   rend
         * | <-------------------------------------------> |
         * +-------------------+---------------------------+
         * |                   |                           |
         * |                   |      New region           |
         * |                   |                           |
         * +-------------------+---------------------------+
         *                     | <-----------------------> |
         *                     base                        end
         *
         * 1) end == rend
         * 2) rbase < base
         * 3) rbase: 0x61000000
         *    rend:  0x63000000
         *    base:  0x62000000
         *    end:   0x63000000
         *
         * Processing: Insert/Merge
         *
         * rbase                                           rend
         * +-----------------------------------------------+
         * |                                               |
         * |  Exist regions                                |
         * |                                               |
         * +-----------------------------------------------+
         *
         * 1) rbase: 0x61000000
         *    rend:  0x63000000
         */
        memblock_reserve(0x62000000, 0x1000000);
        pr_info("Scan region which contain new one:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region into memblock.reserved regions that exist region
         * contains new region and the base address of exist region is equal
         * to new, but the end address of exist is big than new.
         *
         * memblock.reserved:
         *
         * rbase              Exist regions               rend
         * | <------------------------------------------> |
         * +----------------+-----------------------------+
         * |                |                             |
         * |   New region   |                             |
         * |                |                             |
         * +----------------+-----------------------------+
         * | <------------> |
         * base             end
         *
         * 1) base == rbase
         * 2) end < rend
         * 3) rbase: 0x61000000
         *    rend:  0x63000000
         *    base:  0x61000000
         *    end:   0x62000000
         *
         * Processing: Insert/Merge
         *
         * rbase                                          rend
         * +----------------------------------------------+
         * |                                              |
         * |  Exist Regions                               |
         * |                                              |
         * +----------------------------------------------+
         *
         * 1) rbase: 0x61000000
         *    rend:  0x63000000
         */
        memblock_reserve(0x61000000, 0x1000000);
        pr_info("Scan region which contain and head of regions:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region into memblock.reserved and new region is
         * same with exist regions.
         *
         * memblock.reserved:
         *
         * rbase            Exist Regions                 rend
         * | <------------------------------------------> |
         * +----------------------------------------------+
         * |                                              |
         * |                                              |
         * |                                              |
         * +----------------------------------------------+
         * | <------------------------------------------> |
         * base             New region                    end
         *
         * 1) rbase = base
         * 2) rend  = end
         * 3) rbase: 0x61000000
         *    rend:  0x63000000
         *    base:  0x61000000
         *    rend:  0x63000000
         *
         * Processing: Insert/Merge
         *
         * rbase                                          rend
         * +----------------------------------------------+
         * |                                              |
         * | Exist regions                                |
         * |                                              |
         * +----------------------------------------------+
         *
         * 1) rbase: 0x61000000
         *    rend:  0x63000000
         */
        memblock_reserve(0x61000000, 0x2000000);
        pr_info("Scan equal region:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region into memblock.reserved regions. The base address
         * of new region is in the front of base address of exist regions,
         * but end address of new region is big then exist.
         *
         * memblock.reserved
         *
         *                 rbase     Exist regions        rend
         *                 | <--------------------------> |
         * +---------------+--------+---------------------+
         * |               |        |                     |
         * |               |        |                     |
         * |               |        |                     |
         * +---------------+--------+---------------------+
         * | <--------------------> |
         * base   New region        end
         *
         * 1) rbase > base
         * 2) rbase < end
         * 3) end < rend
         * 4) rbase: 0x61000000
         *    rend:  0x63000000
         *    base:  0x60f00000
         *    ene:   0x61100000
         *
         * Processing: Insert/Merge
         *
         * rbase                                          rend
         * +----------------------------------------------+
         * |                                              |
         * | Exist regions                                |
         * |                                              |
         * +----------------------------------------------+
         *
         * 1) rbase: 0x60f00000
         *    rend:  0x63000000
         */
        memblock_reserve(0x60f00000, 0x200000);
        pr_info("Scan forware and contain regions:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region into memblock.reserved regions, and new region
         * is in front of exist regions and conjunct with exist regions.
         *
         * memblock.reserved:
         *
         *                        rbase                      rend
         *                        | <----------------------> |
         * +----------------------+--------------------------+
         * |                      |                          |
         * | New region           | Exist regions            |
         * |                      |                          |
         * +----------------------+--------------------------+
         * | <------------------> |
         * base                   end
         *
         * 1) end == rbase
         * 2) rbase: 0x60f00000
         *    rend:  0x63000000
         *    base:  0x60e00000
         *    end:   0x60f00000
         *
         * Processing: Insert/Merge
         *
         * rbase                                             rend
         * +-------------------------------------------------+
         * |                                                 |
         * | Exist regions                                   |
         * |                                                 |
         * +-------------------------------------------------+
         *
         * 1) rbase: 0x60e00000
         *    rend:  0x63000000
         *
         */
        memblock_reserve(0x60e00000, 0x100000);
        pr_info("Scan forward and conjunct regions:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /*
         * Insert a new region into memblock.reserved regions that disjunct
         * with exist regions, and the end address of new regions is more
         * small than exist.
         *
         * memblock.reserved
         *
         * base                    end        rbase               rend
         * +-----------------------+          +-------------------+
         * |                       |          |                   |
         * | New region            |          | Exist regions     |
         * |                       |          |                   |
         * +-----------------------+          +-------------------+
         *
         * 1) end < rbase
         * 2) rbase: 0x60e00000
         *    rend:  0x63000000
         *    base:  0x60a00000
         *    end:   0x60b00000
         */
        memblock_reserve(0x60a00000, 0x100000);
        pr_info("Scan forware and disjunct regions:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /* Clear debug data from memblock.reserved */
        for_each_memblock(reserved, reg) {
                reg->base = 0;
                reg->size = 0;
        }
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
+config BISCUITOS_MEMBLOCK_RESERVE
+       bool "memblock_reserve()"
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

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选  
"Memblock allocator" 和 “memblock_reserve()” 选项，以打开
CONFIG_BISCUITOS_MEMBLOCK_RESERVE，具体过程请参考：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

#### <span id="驱动增加调试点">增加调试点</span>

驱动运行还需要在内核的指定位置添加调试点，由于该驱动需要在内核启动阶段就使用，请参考下
面 patch 将源码指定位置添加调试代码：

{% highlight c %}
diff --git a/arch/arm/kernel/setup.c b/arch/arm/kernel/setup.c
index 375b13f..d36d824 100644
--- a/arch/arm/kernel/setup.c
+++ b/arch/arm/kernel/setup.c
@@ -1074,6 +1074,10 @@ void __init setup_arch(char **cmdline_p)
{
        const struct machine_desc *mdesc;
+#ifdef CONFIG_BISCUITOS_MEMBLOCK_RESERVE
+       extern int debug_memblock_reserve(void);
+#endif
+
        setup_processor();
        mdesc = setup_machine_fdt(__atags_pointer);
        if (!mdesc)
@@ -1104,6 +1108,10 @@ void __init setup_arch(char **cmdline_p)
        strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
        *cmdline_p = cmd_line;
+#ifdef CONFIG_BISCUITOS_MEMBLOCK_RESERVE
+       debug_memblock_reserve();
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
Region [0x0 -- 0x0]
Scan first region:
Region [0x60000000 -- 0x60200000]
Scan behine and disjunct region:
Region [0x60000000 -- 0x60200000]
Region [0x62000000 -- 0x62200000]
Scan behine and conjunct region:
Region [0x60000000 -- 0x60200000]
Region [0x62000000 -- 0x62400000]
Scan behine but contain regions:
Region [0x60000000 -- 0x60200000]
Region [0x62000000 -- 0x62500000]
Scan contain but equal regions:
Region [0x60000000 -- 0x60200000]
Region [0x62000000 -- 0x62600000]
Scan region which contain exist one:
Region [0x60000000 -- 0x60200000]
Region [0x61000000 -- 0x63000000]
Scan region which contain new one:
Region [0x60000000 -- 0x60200000]
Region [0x61000000 -- 0x63000000]
Scan region which contain and head of regions:
Region [0x60000000 -- 0x60200000]
Region [0x61000000 -- 0x63000000]
Scan equal region:
Region [0x60000000 -- 0x60200000]
Region [0x61000000 -- 0x63000000]
Scan forware and contain regions:
Region [0x60000000 -- 0x60200000]
Region [0x60f00000 -- 0x63000000]
Scan forward and conjunct regions:
Region [0x60000000 -- 0x60200000]
Region [0x60e00000 -- 0x63000000]
Scan forware and disjunct regions:
Region [0x60000000 -- 0x60200000]
Region [0x60a00000 -- 0x60b00000]
Region [0x60e00000 -- 0x63000000]
Malformed early option 'earlycon'
Memory policy: Data cache writeback
Reserved memory: created DMA memory pool at 0x4c000000, size 8 MiB
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

当往 MEMBLOCK 的预留区中加入第一个内存区块，MEMBLOCK 分配器会将第一个预留区放到
reserved.regions 链表的头部，并更新相应的数据，测试代码如下：

{% highlight c %}
        /* Scan old reserved region */
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);

        /* Reserved memblock region is empty and insert a new region
         *
         * memblock.reserved--->+--------+
         *                      |        |
         *                      | Empty- |
         *                      |        |
         *                      +--------+
         */
        memblock_reserve(0x60000000, 0x200000);
        pr_info("Scan first region:\n");
        for_each_memblock(reserved, reg)
                pr_info("Region [%#x -- %#x]\n", reg->base,
                        reg->base + reg->size);
{% endhighlight %}

从源码中可知，调用 memblock_serve() 函数，将内存区块 [0x60000000, 0x60200000] 添加
到预留区间的头部，添加完毕之后，调用 for_each_memblock() 函数遍历预留区内的所有内存
区块，运行结果如下：

{% highlight bash %}
Region [0x0 -- 0x0]
Scan first region:
Region [0x60000000 -- 0x60200000]
{% endhighlight %}

从运行的结果可以看出，第一次调用 for_each_memblock() 函数遍历预留区的时候，预留区
为空，所以打印的值都是 0。当添加第一个内存区块之后，再调用 for_each_memblock() 函数
遍历预留区之后，第一个预留区块就是刚刚添加的内存区块： 0x60000000 -- 0x60200000。综
上所述，memblock_reserve() 函数会将第一个预留区存储到 memblock.reserved.regions
的头部。
