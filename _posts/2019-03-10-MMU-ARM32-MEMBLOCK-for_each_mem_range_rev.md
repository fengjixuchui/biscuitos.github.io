---
layout: post
title:  "for_each_mem_range_rev() 遍历所有可用的物理内存区块"
date:   2019-03-10 11:45:30 +0800
categories: [MMU]
excerpt: for_each_mem_range_rev() 遍历所有可用的物理内存区块.
tags:
  - MMU
---

> [GitHub: for_each_mem_range_rev()](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/Memblock-allocator/API/for_each_mem_range_rev)
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

函数： for_each_mem_range_rev()

功能： 遍历所有可用的物理内存区块

{% highlight bash %}
for_each_mem_range_rev
|
|---__next_mem_range_rev
    |
    |---memblock_get_region_node
    |
    |---memblock_is_hotpluggable
    |
    |---memblock_is_mirror
    |
    |---memblock_is_nomap
{% endhighlight %}

##### for_each_mem_range_rev

{% highlight c %}
/**
 * for_each_mem_range_rev - reverse iterate through memblock areas from
 * type_a and not included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 */
#define for_each_mem_range_rev(i, type_a, type_b, nid, flags,           \
                               p_start, p_end, p_nid)                   \
        for (i = (u64)ULLONG_MAX,                                       \
                     __next_mem_range_rev(&i, nid, flags, type_a, type_b,\
                                          p_start, p_end, p_nid);       \
             i != (u64)ULLONG_MAX;                                      \
             __next_mem_range_rev(&i, nid, flags, type_a, type_b,       \
                                  p_start, p_end, p_nid))

{% endhighlight %}

参数 i 用于循环；type_a 参数指向可用物理内存区块; type_b 参数指向预留物理内存
区块。 nid 指向节点信息；参数 flags 指向内存区标志； p_start 参数用于存储查找
到的内存区块起始地址； p_end 参数用于存储查找到的内存区块的终止地址

函数首先调用 for 循环，将参数 i 设置为 ULLONG_MAX, 以此遍历所有预留区，然后调用
__next_mem_range_rev() 函数查找一块可用的物理内存。每遍历一次，循环都检查 i 的
值，如果 i 不等于 ULLONG_MAX, 那么继续循环。每次调用完 __next_mem_range_rev()
函数之后，i 都会指向前一个预留区。通过循环，每个找到的可用物理内存区块都会将其起始
地址存储到 p_start 参数里，将终止地址存储到 p_end 参数里。直到遍历完所有可用物理
内存区块之后终止循环。

##### __next_mem_range_rev

函数代码较长，分段解析

{% highlight c %}
/**
 * __next_mem_range_rev - generic next function for for_each_*_range_rev()
 *
 * @idx: pointer to u64 loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @type_a: pointer to memblock_type from where the range is taken
 * @type_b: pointer to memblock_type which excludes memory from being taken
 * @out_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @out_nid: ptr to int for nid of the range, can be %NULL
 *
 * Finds the next range from type_a which is not marked as unsuitable
 * in type_b.
 *
 * Reverse of __next_mem_range().
 */
void __init_memblock __next_mem_range_rev(u64 *idx, int nid,
                                          enum memblock_flags flags,
                                          struct memblock_type *type_a,
                                          struct memblock_type *type_b,
                                          phys_addr_t *out_start,
                                          phys_addr_t *out_end, int *out_nid)
{
        int idx_a = *idx & 0xffffffff;
        int idx_b = *idx >> 32;
{% endhighlight %}

参数 idx 是一个 64 位变量，其高 32 位作为 type_a 参数的索引，低 32位作为
type_b 参数的索引。flags 指向内存区的标志；type_a 指向可用物理内存区；
type_b 指向预留内存区；out_start 参数用于存储查找到区域的起始地址；
out_end 参数用于存储找到区域的终止地址; out_nid 指向 nid 域; idx_a 变量
用于存储可用物理内存区块的索引; idx_b 变量用于存储预留区块的索引。

{% highlight c %}
if (WARN_ONCE(nid == MAX_NUMNODES, "Usage of MAX_NUMNODES is deprecated. Use NUMA_NO_NODE instead\n"))
        nid = NUMA_NO_NODE;

if (*idx == (u64)ULLONG_MAX) {
        idx_a = type_a->cnt - 1;
        if (type_b != NULL)
                idx_b = type_b->cnt;
        else
                idx_b = 0;
}
{% endhighlight %}

函数首先检查 nid 参数，如果参数值不是 MAX_NUMNODES， 那么函数就会报错。接着函数
将 idx 参数进行检查，如果参数的值是 ULLONG_MAX,那么就将 idx_a 设置为 type_a
内存区含有的内存区块数，此时，如果 type_b 参数如果存在，那么将 idx_b 设置为
预留区的函数的内存区块数。如果 type_b 参数不存在，那么函数认为 MEMBLOCK 不存在
预留区，可以从有用的物理内存区开始查找，如果 idx_b 设置为 0.

{% highlight c %}
for (; idx_a >= 0; idx_a--) {
        struct memblock_region *m = &type_a->regions[idx_a];

        phys_addr_t m_start = m->base;
        phys_addr_t m_end = m->base + m->size;
        int m_nid = memblock_get_region_node(m);

        /* only memory regions are associated with nodes, check it */
        if (nid != NUMA_NO_NODE && nid != m_nid)
                continue;

        /* skip hotpluggable memory regions if needed */
        if (movable_node_is_enabled() && memblock_is_hotpluggable(m))
                continue;

        /* if we want mirror memory skip non-mirror memory regions */
        if ((flags & MEMBLOCK_MIRROR) && !memblock_is_mirror(m))
                continue;

        /* skip nomap memory unless we were asked for it explicitly */
        if (!(flags & MEMBLOCK_NOMAP) && memblock_is_nomap(m))
                continue;
{% endhighlight %}

这段代码首先使用 for 循环遍历 MEMBLOCK 中可用的物理内存区块，从最后一块开始
遍历，每次遍历一块内存区块，都是用 struct memblock_region 结构体维护，以此
计算出这块内存区块的起始地址和终止地址。接下来进行内存区块的检测，函数是要找到
一块符合要求的的内存区块，内存区块要满足以下几点：

> 1. 内存区块必须属于指定的 node
>
> 2. 内存区块不是热插拔的
>
> 3. 内存区块不是 non-mirror 的
>
> 4. 内存区块不能是 MEMBLOCK_NOMAP 的

只有符合上述的内存区块才是可以继续查找的部分。

{% highlight c %}
        if (!type_b) {
                if (out_start)
                        *out_start = m_start;
                if (out_end)
                        *out_end = m_end;
                if (out_nid)
                        *out_nid = m_nid;
                idx_a--;
                *idx = (u32)idx_a | (u64)idx_b << 32;
                return;
        }
{% endhighlight %}

经过上面的检测之后，如果预留区不存在，那么直接就从可用物理内存区中返回可用的地址，
out_start 参数指向可用物理内存区块的起始地址；out_end 参数指向可用物理内存区块
的终止地址，将 idx_a 指向前一块可用的物理内存块，并更新 idx 参数的值，使其指向
新的索引。最后返回

{% highlight c %}
/* scan areas before each reservation */
for (; idx_b >= 0; idx_b--) {
        struct memblock_region *r;
        phys_addr_t r_start;
        phys_addr_t r_end;

        r = &type_b->regions[idx_b];
        r_start = idx_b ? r[-1].base + r[-1].size : 0;
        r_end = idx_b < type_b->cnt ?
                r->base : PHYS_ADDR_MAX;
        /*
         * if idx_b advanced past idx_a,
         * break out to advance idx_a
         */

        if (r_end <= m_start)
                break;
{% endhighlight %}

如果预留区存在，那么从 idx_b 指向的预留区开始查找。同样使用 memblock_region
结构维护一个预留区块。接下来判断，如果该预留区块的前一块预留区块也存在，那么函数
将上一块预留区块的结束地址到该预留区块的起始地址作为一块可用的物理内存区块；如果
不存在上一块预留区块，那么可用物理内存区块的起始地址设置为 0；如果 idx_b 的索引
不小于预留区所具有的内存区块数，那么将可用物理内存区块的终止地址设置为
PHYS_ADDR_MAX.通过上面的判断可以找到一块可用的物理内存区块范围，如果找到的内存
区块的起始地址大于或等于该预留区块，那么就跳出循环，表示在该可用的物理内存区段中
找不到一块满足条件的内存区块。

{% highlight c %}
/* scan areas before each reservation */
/* if the two regions intersect, we're done */
if (m_end > r_start) {
        if (out_start)
                *out_start = max(m_start, r_start);
        if (out_end)
                *out_end = min(m_end, r_end);
        if (out_nid)
                *out_nid = m_nid;
        if (m_start >= r_start)
                idx_a--;
        else
                idx_b--;
        *idx = (u32)idx_a | (u64)idx_b << 32;
        return;
}
{% endhighlight %}

如果符合之前的条件，那么接下来只要找到的物理内存区块的终止地址比预留区的大，但找到的
物理内存区块的起始地址小于预留区块的终止地址，至少保证可用物理内存区在预留区块之前有
交集。接下来，将 out_start 参数指向最大的起始地址；将 out_end 指向最小的终止地
址，这样处理能确保找到的物理内存区块不与预留区块重叠。接着如果找到的可用物理内存区块
的起始地址大于预留区块的起始地址，那么增加 idx_a 的引用计数，这样可以在循环中指向
下一块可用的物理内存；反之增加 idx_b 的引用计数，这样可以指向下一块预留区块。

{% highlight c %}
/* signal end of iteration */
*idx = ULLONG_MAX;
{% endhighlight %}

如果循环遍历之后，还找不到，那么直接设置 idx 为 ULLONG_MAX.

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

for_each_mem_range_rev() 函数的作用是遍历所有可用的物理内存，
实践的目的就是遍历所有可用物理内存。

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
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>

int bs_debug = 0;

#ifdef CONFIG_DEBUG_FOR_EACH_MEM_RANGE_REV
int debug_for_each_mem_range_rev(void)
{
        enum memblock_flags flags = choose_memblock_flags();
        phys_addr_t start;
        phys_addr_t end;
        u64 idx; /* (memory.cnt << 32) | (reserved.cnt) */

        /*
         * Memory Maps:
         *
         *                     Reserved 0            Reserved 1
         *                   | <------> |          | <------> |
         * +-----------------+----------+----------+----------+------+
         * |                 |          |          |          |      |
         * |                 |          |          |          |      |
         * |                 |          |          |          |      |
         * +-----------------+----------+----------+----------+------+
         *                              | <------> |
         *                               Found area
         *
         * Reserved 0: [0x64000000, 0x64100000]
         * Reserved 1: [0x64300000, 0x64400000]
         */
        memblock_reserve(0x64000000, 0x100000);
        memblock_reserve(0x64300000, 0x100000);

        for_each_mem_range_rev(idx, &memblock.memory, &memblock.reserved,
                        NUMA_NO_NODE, flags, &start, &end, NULL)
                pr_info("Region: [%#x - %#x]\n", start, end);

        /* Clear rservation for debug */
        memblock.reserved.cnt = 1;
        memblock.reserved.total_size = 0;

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
+config DEBUG_FOR_EACH_MEM_RANGE_REV
+       bool "for_each_mem_range_rev()"
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

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，
以打开 CONFIG_BISCUITOS_MEMBLOCK_RESERVE，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]Memblock allocator
            [*]for_each_mem_range_rev()
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
+#ifdef CONFIG_DEBUG_FOR_EACH_MEM_RANGE_REV
+       extern int bs_debug;
+       extern int debug_for_each_mem_range_rev(void);
+#endif

        setup_processor();
        mdesc = setup_machine_fdt(__atags_pointer);
@@ -1104,6 +1108,10 @@ void __init setup_arch(char **cmdline_p)
        strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
        *cmdline_p = cmd_line;

+#ifdef CONFIG_DEBUG_FOR_EACH_MEM_RANGE_REV
+       debug_for_each_mem_range_rev();
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
Region: [0x64400000 - 0xa0000000]
Region: [0x64100000 - 0x64300000]
Region: [0x60000000 - 0x64000000]
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

接着调用 for_each_mem_range_rev() 函数遍历所有的可用物理内存，并将
可用物理内存的起始地址和终止地址都打印出来，代码如下：

{% highlight c %}
for_each_mem_range_rev(idx, &memblock.memory, &memblock.reserved,
                NUMA_NO_NODE, flags, &start, &end, NULL)
        pr_info("Region: [%#x - %#x]\n", start, end);
{% endhighlight %}

通过调用调用 for_each_mem_range_rev() 函数，所有的可用物理内存区块都会
被找到，所以这个函数用于查找可用的物理内存区块。实践中，物理内存区范围是：
[0x60000000, 0xa0000000], 就在这块内存区块，被两个预留区分成了三段，两个
预留区占据了 [0x64000000, 0x64100000] 和 [0x64300000, 0x64400000],
所以剩下的可用物理内存区块为：

> [0x64400000 - 0xa0000000]
>
> [0x64100000 - 0x64300000]
>
> [0x60000000 - 0x64000000]

通过实践可知，分析的可用内存区段和实践获得物理内存区块是一致的。更多
原理请看[for_each_mem_range_rev() 源码分析](#源码分析)
