---
layout: post
title:  "of_translate_address"
date:   2019-02-24 11:21:30 +0800
categories: [HW]
excerpt: DTS API of_translate_address().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_translate_address](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_translate_address)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="源码分析">源码分析</span>

##### of_translate_address

{% highlight c %}
of_translate_address
|
|--__of_translate_address
     |
     |---of_node_get
     |
     |---of_get_parent
     |
     |---of_match_bus
     |
     |---of_translate_one
          |
          |---of_empty_range_quirk
          |
          |---of_read_number
{% endhighlight %}

函数作用：函数用于计算节点地址对应的物理地址。

> 平台： ARM
>
> linux： 3.10/4.18/5.0

{% highlight c %}
u64 of_translate_address(struct device_node *dev, const __be32 *in_addr)
{
    return __of_translate_address(dev, in_addr, "ranges");
}
EXPORT_SYMBOL(of_translate_address);
{% endhighlight %}

函数用于将 reg 属性中的地址值装换成物理地址。

dev 指向 device_node； in_addr 参数指向节点的 reg 中地址值

函数通过直接调用 __of_translate_address() 函数获得地址值对应的物理地址。

##### __of_translate_address

{% highlight c %}
/*
* Translate an address from the device-tree into a CPU physical address,
* this walks up the tree and applies the various bus mappings on the
* way.
*
* Note: We consider that crossing any level with #size-cells == 0 to mean
* that translation is impossible (that is we are not dealing with a value
* that can be mapped to a cpu physical address). This is not really specified
* that way, but this is traditionally the way IBM at least do things
*/
static u64 __of_translate_address(struct device_node *dev,
                  const __be32 *in_addr, const char *rprop)
{
    struct device_node *parent = NULL;
    struct of_bus *bus, *pbus;
    __be32 addr[OF_MAX_ADDR_CELLS];
    int na, ns, pna, pns;
    u64 result = OF_BAD_ADDR;

    pr_debug("OF: ** translation for device %s **\n", dev->full_name);

    /* Increase refcount at current level */
    of_node_get(dev);

    /* Get parent & match bus type */
    parent = of_get_parent(dev);
    if (parent == NULL)
        goto bail;
    bus = of_match_bus(parent);

    /* Count address cells & copy address locally */
    bus->count_cells(dev, &na, &ns);
    if (!OF_CHECK_COUNTS(na, ns)) {
        printk(KERN_ERR "prom_parse: Bad cell count for %s\n",
               dev->full_name);
        goto bail;
    }
    memcpy(addr, in_addr, na * 4);

    pr_debug("OF: bus is %s (na=%d, ns=%d) on %s\n",
        bus->name, na, ns, parent->full_name);
    of_dump_addr("OF: translating address:", addr, na);
{% endhighlight %}

参数 dev 执行一个 device node；in_addr 参数指向一个地址属性值；rprop 指向一个 
ranges 属性名字

函数较长，这里分段解释

函数首先调用 of_get_parent() 函数获得 dev 对应的 parent，如果此时 parent 是
空，那么 dev 就是 root 节点，直接跳转到 bail 分支，并返回；如果父节点存在，
那么调用 of_match_bus() 函数获得 parent 对应的 of_bus 结构，然后调用 of_bus 
的 count_cells 方法计算出节点的 #address-cells 和 #size-cells 所占用的 cells 
数。然后调用 OF_CHECK_COUNTS 宏检查这两个 cells 是否有效，如果无效，则跳转到 
bail 分支并返回；如果有效，则将 in_addr 的值拷贝到 addr 变量里。

{% highlight c %}
    /* Translate */
    for (;;) {
        /* Switch to parent bus */
        of_node_put(dev);
        dev = parent;
        parent = of_get_parent(dev);

        /* If root, we have finished */
        if (parent == NULL) {
            pr_debug("OF: reached root node\n");
            result = of_read_number(addr, na);
            break;
        }

        /* Get new parent bus and counts */
        pbus = of_match_bus(parent);
        pbus->count_cells(dev, &pna, &pns);
        if (!OF_CHECK_COUNTS(pna, pns)) {
            printk(KERN_ERR "prom_parse: Bad cell count for %s\n",
                   dev->full_name);
            break;
        }

        pr_debug("OF: parent bus is %s (na=%d, ns=%d) on %s\n",
            pbus->name, pna, pns, parent->full_name);

        /* Apply bus translation */
        if (of_translate_one(dev, bus, pbus, addr, na, ns, pna, rprop))
            break;

        /* Complete the move up one level */
        na = pna;
        ns = pns;
        bus = pbus;

        of_dump_addr("OF: one level translation:", addr, na);
    }
bail:
    of_node_put(parent);
    of_node_put(dev);

    return result;
}
{% endhighlight %}

接着调用 for 循环进行，每循环一次，函数首先将 dev 指向节点对应的 parent，然后
通过调用 of_get_parent() 函数获得 dev 对应的 parent，如果此时 parent 为空，那
么获得参数 addr 对应的值，并结束循环返回；如果 parent 不为空，那么调用 
of_match_bus() 函数获得 parent 对应的 of_bus, 并调用 of_bus 的 count_cells 方
法获得 parent 的 #address-cells 和 #size-cells 所占 cells 的个数。接着再次调用 
OF_CHECK_COUNTS 宏检查两者的有效性，如果检查不通过，则打印错误信息并结束循环；
如果检查通过，则调用 of_translate_one() 函数进行地址转换，如果转换成功则返回 0

##### of_translate_one

{% highlight c %}
static int of_translate_one(struct device_node *parent, struct of_bus *bus,
                struct of_bus *pbus, __be32 *addr,
                int na, int ns, int pna, const char *rprop)
{
    const __be32 *ranges;
    unsigned int rlen;
    int rone;
    u64 offset = OF_BAD_ADDR;

    /* Normally, an absence of a "ranges" property means we are
     * crossing a non-translatable boundary, and thus the addresses
     * below the current not cannot be converted to CPU physical ones.
     * Unfortunately, while this is very clear in the spec, it's not
     * what Apple understood, and they do have things like /uni-n or
     * /ht nodes with no "ranges" property and a lot of perfectly
     * useable mapped devices below them. Thus we treat the absence of
     * "ranges" as equivalent to an empty "ranges" property which means
     * a 1:1 translation at that level. It's up to the caller not to try
     * to translate addresses that aren't supposed to be translated in
     * the first place. --BenH.
     *
     * As far as we know, this damage only exists on Apple machines, so
     * This code is only enabled on powerpc. --gcl
     */
    ranges = of_get_property(parent, rprop, &rlen);
    if (ranges == NULL && !of_empty_ranges_quirk()) {
        pr_err("OF: no ranges; cannot translate\n");
        return 1;
    }
    if (ranges == NULL || rlen == 0) {
        offset = of_read_number(addr, na);
        memset(addr, 0, pna * 4);
        pr_debug("OF: empty ranges; 1:1 translation\n");
        goto finish;
    }

    pr_debug("OF: walking ranges...\n");

    /* Now walk through the ranges */
    rlen /= 4;
    rone = na + pna + ns;
    for (; rlen >= rone; rlen -= rone, ranges += rone) {
        offset = bus->map(addr, ranges, na, ns, pna);
        if (offset != OF_BAD_ADDR)
            break;
    }
    if (offset == OF_BAD_ADDR) {
        pr_debug("OF: not found !\n");
        return 1;
    }
    memcpy(addr, ranges + na, 4 * pna);

finish:
    of_dump_addr("OF: parent translation for:", addr, pna);
    pr_debug("OF: with offset: %llx\n", (unsigned long long)offset);

    /* Translate it into parent bus space */
    return pbus->translate(addr, offset, pna);
}
{% endhighlight %}

函数首先获得 parent 节点的 ranges 属性，如果获取失败或 
of_empty_ranges_quirk() 返回 0， 则打印错误并返回 1；如果获得 ranges 属性，当
属性长度为 0，那么就读取 addr 的属性值，并将 addr 的值清零，并跳转到 finish 分
支；如果上面的条件都不满足，也就是 parent 的 ranges 属性存在且包含多个值，那么
函数首先计算 ranges 包含 32 位整形数据的个数，并计算一个 ranges 成员所占的 
cells 数，通过代码可知，一个 ranges 成员由三部分构成：

{% highlight c %}
ranges： 本节点 #address-cells ：父节点 #address-cells ：本节点 #size-cells
{% endhighlight %}

所以接下来的 for 循环中，函数首先判断 ranges 属性的长度是否比一个 rone 包含的 
cells 长，如果小于则停止循环；如果大于或等于，说明 ranges 属性至少包含一个成员，
每次循环，函数都会调用 of_bus 的 map 方法，函数将 addr，ranges，na, ns, pna 的
值都传递给 map 方法，不同的 of_bus 采用不同的 map 方法，这里就只讨论 default 
的方法。对于 default 的方法，函数此时会调用 of_bus_default_map() 函数去检查当
前节点的地址是否满足映射要求，如果满足，则返回当前节点的地址与映射基地址之间的
差值，只要返回值不是 OF_BAD_ADDR，那么就直接退出循环。接着 ranges + na 的属性
值拷贝到 addr，也就是每个 ranges 成员的第二部分，这部分是与 parent 地址相关的。
获得以上数据之后，最后在返回前调用的 of_bus 的 translate 返回，这里也制作 
default 的 translate 方法讲，这里就获得对应的映射地址，最后返回该地址

##### of_bus_defaullt_translate

{% highlight c %}
static int of_bus_default_translate(__be32 *addr, u64 offset, int na)
{
    u64 a = of_read_number(addr, na);
    memset(addr, 0, na * 4);
    a += offset;
    if (na > 1)
        addr[na - 2] = cpu_to_be32(a >> 32);
    addr[na - 1] = cpu_to_be32(a & 0xffffffffu);

    return 0;
}
{% endhighlight %}

参数 addr 指向 ranges 属性的 parent 值

函数首先读取 ranges 成员中中间部分的值，然后清空了 addr 值，并加读出来的值加上 
offset 参数，最终读取了 parent 的值存放在 addr 里面

##### of_bus_default_map

{% highlight c %}
static u64 of_bus_default_map(__be32 *addr, const __be32 *range,
        int na, int ns, int pna)
{
    u64 cp, s, da;

    cp = of_read_number(range, na);
    s  = of_read_number(range + na + pna, ns);
    da = of_read_number(addr, na);

    pr_debug("OF: default map, cp=%llx, s=%llx, da=%llx\n",
         (unsigned long long)cp, (unsigned long long)s,
         (unsigned long long)da);

    if (da < cp || da >= (cp + s))
        return OF_BAD_ADDR;
    return da - cp;
}
{% endhighlight %}

函数首先读取 ranges 属性第一个值作为子节点映射的基地址，然后在读取第三个值作为
作为子节点映射的长度。此时函数读取当前节点的地址值，接下来进行检测，只要当前节
点的地址不在 ranges 提供的映射范围内，那么就直接返回 OF_BAD_ADDR 错误；如果符
合映射要求，那么函数返回当前节点地址到 ranges 基地址之间的差值。

##### of_get_address

{% highlight c %}
const __be32 *of_get_address(struct device_node *dev, int index, u64 *size,
            unsigned int *flags)
{
    const __be32 *prop;
    unsigned int psize;
    struct device_node *parent;
    struct of_bus *bus;
    int onesize, i, na, ns;

    /* Get parent & match bus type */
    parent = of_get_parent(dev);
    if (parent == NULL)
        return NULL;
    bus = of_match_bus(parent);
    bus->count_cells(dev, &na, &ns);
    of_node_put(parent);
    if (!OF_CHECK_ADDR_COUNT(na))
        return NULL;

    /* Get "reg" or "assigned-addresses" property */
    prop = of_get_property(dev, bus->addresses, &psize);
    if (prop == NULL)
        return NULL;
    psize /= 4;

    onesize = na + ns;
    for (i = 0; psize >= onesize; psize -= onesize, prop += onesize, i++)
        if (i == index) {
            if (size)
                *size = of_read_number(prop + na, ns);
            if (flags)
                *flags = bus->get_flags(prop);
            return prop;
        }
    return NULL;
}
EXPORT_SYMBOL(of_get_address);
{% endhighlight %}

参数 dev 指向一个 device_node; index 指向第 n 个地址值的索引；size 存储地址
值；flags 存储 flags 标志

函数首先获得节点对应的父节点，如果不存在，那么直接返回 NULL，然后调用 
of_match_bus() 函数获得节点对应的 of_bus 结构，并调用 of_bus 的 count_cell 返
回获得节点对应 bus 的 #address-cells 和 #size-cells 属性值，并调用 
OF_CHECK_ADDR_COUNT 宏检查 #address-cells 是否有效，如果无效，直接返回 NULL；
如果有效，那么函数就调用 of_get_property() 函数获得 ‘reg’或 ‘assigned-address’
属性的值，如果获取失败，则直接返回 NULL；如果获取成功，由于属性的整形值都是由 
32 位数构成的，所以将获得属性长度除以 4 就可以获得属性中包含多少个 32 位整数。
onesize 变量用于标识一个完整的地址和数值所占用的 32 位整形个数。接着调用 for 
循环遍历属性里面的值，如果遍历的次数和 index 参数一直，那么就读出其值并存储到
参数 size 中，此时如果 flags 参数也有效，那么调用 bus 的 get_flags 参数获得属
性的值存储到 flags 参数里。

指的注意的是，对于 reg 属性的结构，其由：“值+地址”构成，且值在前，地址在后，即
当 reg 包含多个值的时候，如下：

{% highlight c %}
/ {
        DTS_demo {
                compatible = "DTS_demo, BiscuitOS";
                status = "okay";
                reg = <0x11223344 0x55667788
                       0x10203040 0x50607080
                       0x99aabbcc 0xddeeff00
                       0x90a0b0c0 0xd0e0f000>;
        };
};

(假设 #address-cells 和 #size-cells 都是 2)
reg 第一个成员的值是：     0x1122334455667788
reg 第一个成员的地址是：  0x1020304050607080
reg 第二个成员的值是：     0x99aabbccddeeff00
reg 第二个成员的地址是：  0x90a0b0c0d0e0f000
{% endhighlight %}

所以 index 参数为 0 的时候，of_get_address() 函数返回 0x1020304050607080

同理 index 参数为 1 的时候，of_get_address() 函数返回 0x90a0b0c0d0e0f000

##### OF_CHECK_ADDR_COUNTS

{% highlight c %}
/* Max address size we deal with */
#define OF_MAX_ADDR_CELLS    4
#define OF_CHECK_ADDR_COUNT(na)    ((na) > 0 && (na) <= OF_MAX_ADDR_CELLS)
{% endhighlight %}

这个宏用于检查节点地址的长度是否满足对应总线的 #address-cells 的要求

##### of_match_bus

{% highlight c %}
static struct of_bus *of_match_bus(struct device_node *np)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(of_busses); i++)
        if (!of_busses[i].match || of_busses[i].match(np))
            return &of_busses[i];
    BUG();
    return NULL;
}
{% endhighlight %}

函数的作用是用于获得节点对应的 of_busses

参数 np 指向一个设备节点

函数首先调用 for 循环遍历 of_busses 数组，数组里存放着内核定义的几种 bus 对应
的 of_bus 结构，内核一共定义了三种 of_bus 结构，分别为 PCI，ISA 和默认 bus。
循环每遍历到一个 of_bus 的时候，都会检查 of_bus 结构是否提供了 match 方法和如
果提供了 match 方法之后运行 match 方法的结果，如果 of_bus 结构不存在 match 方
法，或者运行 match 方法之后返回值为真时，返回这个 of_bus 结构。如果遍历完之后
还是没找到对应的 of_bus 结构，那么返回 NULL。

##### of_busses 

{% highlight c %}
static struct of_bus of_busses[] = {
#ifdef CONFIG_PCI
    /* PCI */
    {
        .name = "pci",
        .addresses = "assigned-addresses",
        .match = of_bus_pci_match,
        .count_cells = of_bus_pci_count_cells,
        .map = of_bus_pci_map,
        .translate = of_bus_pci_translate,
        .get_flags = of_bus_pci_get_flags,
    },
#endif /* CONFIG_PCI */
    /* ISA */
    {
        .name = "isa",
        .addresses = "reg",
        .match = of_bus_isa_match,
        .count_cells = of_bus_isa_count_cells,
        .map = of_bus_isa_map,
        .translate = of_bus_isa_translate,
        .get_flags = of_bus_isa_get_flags,
    },
    /* Default */
    {
        .name = "default",
        .addresses = "reg",
        .match = NULL,
        .count_cells = of_bus_default_count_cells,
        .map = of_bus_default_map,
        .translate = of_bus_default_translate,
        .get_flags = of_bus_default_get_flags,
    },
};
{% endhighlight %}

##### of_get_property

{% highlight c %}
/*
* Find a property with a given name for a given node
* and return the value.
*/
const void *of_get_property(const struct device_node *np, const char *name,
                int *lenp)
{
    struct property *pp = of_find_property(np, name, lenp);

    return pp ? pp->value : NULL;
}
EXPORT_SYMBOL(of_get_property);
{% endhighlight %}

参数 np 指向一个 device node；name 参数为需要查找属性的名字；lenp 参数用于存储
属性的长度。

函数通过调用 of_find_property() 函数获得属性的结构 property，如果获得成功，则
返回属性的值；如果失败则返回 NULL。

##### of_find_property

{% highlight c %}
struct property *of_find_property(const struct device_node *np,
                  const char *name,
                  int *lenp)
{
    struct property *pp;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    pp = __of_find_property(np, name, lenp);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);

    return pp;
}
EXPORT_SYMBOL(of_find_property);
{% endhighlight %}

参数 np 指向一个 device_node；name 参数表示需要查看属性的名字；lenp 用于存储属
性长度。

函数通过调用 raw_spin_lock_irqsave() 函数加锁之后，就直接调用 
__of_find_property() 函数。__of_find_property() 函数用于查找并返回属性名字与 
name 一致的属性。调用完 __of_find_property() 函数之后，调用 
raw_spin_unlock_irqrestore() 函数解锁。最后返回 property 结构。

##### __of_find_property

{% highlight c %}
static struct property *__of_find_property(const struct device_node *np,
                       const char *name, int *lenp)
{
    struct property *pp;

    if (!np)
        return NULL;

    for (pp = np->properties; pp; pp = pp->next) {
        if (of_prop_cmp(pp->name, name) == 0) {
            if (lenp)
                *lenp = pp->length;
            break;
        }
    }

    return pp;
}
{% endhighlight %}

由于每个 device_node 包含一个名为 properties 的成员，properties 是一个单链表
的表头，这个单链表包含了该节点的所有属性，函数通过使用 for 循环遍历这个链表，
以此遍历节点所包含的所有属性。每遍历一个属性就会调用 of_prop_cmp() 函数，
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到
的属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不
为 NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

##### of_read_number

{% highlight c %}
/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 of_read_number(const __be32 *cell, int size)
{
    u64 r = 0;
    while (size--)
        r = (r << 32) | be32_to_cpu(*(cell++));
    return r;
}
{% endhighlight %}

参数 cell 指向数据；size 指向数据 cells 数量。

函数使用 while 循环，每遍历依次，数据长度增加 32 位，并从中读取 32 位数据，最
后以为合并指定长度的数据。

-------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，而且私有节点包含一些属性和一个子节点，
实践中通过 of_translate_address() 函数将子节点的地址转换成物理地址，函数定义
如下：

{% highlight c %}
u64 of_translate_address(struct device_node *dev, const __be32 *in_addr)
{% endhighlight %}

这个函数经常用将节点 reg 属性中的地址转换为物理地址。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点包含了 #address-cells 
属性，其值为 1，以此指定子节点 reg 对应的地址由一个 cells 构成，同理节点也包含
了一个 #size-cells 属性，其值也为 1，这也指明该节点的子节点 reg 属性中 size 占
用一个 cells。节点包含一个 ranges 属性，属性中的每个成员由三部分构成，如下：

{% highlight c %}
ranges-member: mapping_address : parent physical base address : mapping length
{% endhighlight %}

从上面可知，ranges 属性的每个成员由三部分构成，每部分的含义如下：

> mapping_address：这部分表示子节点映射的起始地址，其长度由节点的 
>                  #address-cells 指定
>
> parent physical  base address：这部分表示节点的起始物理地址，其长度由节点
>                  的父节点的 #address-cells 指定
>
> mapping length: 这部分表示子节点能地址映射的长度，其长度由节点的 
>                  #size-cells 指定

从下面的 DTS 文件定义可知，子节点可以映射的地址范围是：0x78000000 到 
0x88000000 (0x78000000 + 0x10000000)，节点的物理基地址就是 0x0000000090000000。
DTS 中也定义了一个子节点 child0，其包含了一个 reg 属性，属性包含多个值，具体
内容如下：

{% highlight c %}
/*
 * DTS Demo Code
 *
 * (C) 2019.01.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or m
 * it under the terms of the GNU General Public License version 2
 * published by the Free Software Foundation.
 */
/ {
        DTS_demo {
                compatible = "DTS_demo, BiscuitOS";
                status = "okay";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0x78000000 0x00000000 0x90000000 0x10000000>;

                child0 {
                        reg = <0x80000000 0x20000000
                               0x30000000 0x40000000>;
                };
        };
};
{% endhighlight %}

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者在 Linux 4.20.8 的源
码中，找到 arch/arm/boot/dts/vexpress-v2p-ca9.dts 文件，然后在文件开始地方添
加如下内容：

{% highlight c %}
#include "DTS_demo.dtsi"
{% endhighlight %}

#### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备
驱动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，首先调用 of_get_child_by_name() 函数获得 “child0”
子节点，然后调用 of_get_address() 函数获得子节点 reg 属性中的第一个地址值，接
着将这个地址值传递给 of_translate_address() 将这个地址转换为物理地址，驱动编
写如下：

{% highlight c %}
/*
 * Device-Tree: of_translate_address
 *
 * u64 of_translate_address(struct device_node *dev, const __be32 *in_addr)
 *
 * (C) 2019.01.01 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Private DTS file: DTS_demo.dtsi
 *
 * / {
 *        DTS_demo {
 *                compatible = "DTS_demo, BiscuitOS";
 *                status = "okay";
 *                #address-cells = <1>;
 *                #size-cells = <1>;
 *                ranges = <0x78000000 0x00000000 0x90000000 0x10000000>;
 *
 *                child0 {
 *                        reg = <0x80000000 0x20000000
 *                               0x30000000 0x40000000>;
 *                };
 *
 *        };
 * };
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_node *child;
    const u32 *regaddr_p;
    u64 addr, regaddr;
    
    printk("DTS demo probe entence.\n");

    /* get child deviec node which named "child0" */
    child = of_get_child_by_name(np, "child0");

    /* get first address from 'reg' property */
    regaddr_p = of_get_address(child, 0, &addr, NULL);
    if (regaddr_p)
        printk("%s's address[0]: %#llx\n", child->name, addr);

    /* Translate address to Physical address */
    regaddr = of_translate_address(child, regaddr_p);
    if (regaddr)
        printk("%s's Physical address: %#llx\n", child->name, regaddr);

    return 0;
}

/* remove platform driver */
static int DTS_demo_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id DTS_demo_of_match[] = {
    { .compatible = "DTS_demo, BiscuitOS", },
    { },
};
MODULE_DEVICE_TABLE(of, DTS_demo_of_match);

/* platform driver information */
static struct platform_driver DTS_demo_driver = {
    .probe  = DTS_demo_probe,
    .remove = DTS_demo_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DEV_NAME, /* Same as device name */
        .of_match_table = DTS_demo_of_match,
    },
};
module_platform_driver(DTS_demo_driver);
{% endhighlight %}

编写好驱动之后，将其编译进内核。编译内核和 dts，如下命令：

{% highlight c %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight c %}
[    3.565634] DTS demo probe entence.
[    3.565648] child0's address[0]: 0x20000000
[    3.565660] child0's physical address: 0x98000000
{% endhighlight %}

通过上面的运行结果和驱动代码可知，child0 子节点 reg 的第一个地址是 0x20000000, 
其映射地址是 0x80000000，在 DTS_demo 节点中，ranges 属性指明了映射的基地址是 
0x78000000, 映射大小为 0x10000000, 所以 child0 节点的映射在 ranges 范围内，所
以可以映射，其偏移为 0x80000000 - 0x78000000 = 0x8000000。最后将这个值加上物
理基地址就是最后的物理地址： 0x90000000 + 0x8000000 = 0x98000000.

------------------------------------------

# <span id="附录">附录</span>
