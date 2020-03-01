---
layout: post
title:  "of_get_address"
date:   2019-02-21 18:40:30 +0800
categories: [HW]
excerpt: DTS API of_get_address().
tags:
  - DTS
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_get_address](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_get_address)
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

##### of_get_address

{% highlight ruby %}
of_get_address
|
|--of_get_parent
|
|---of_match_bus
|
|---of_get_property
|   |
|   |---of_find_property
|       |
|       |---of_find_property
|           |
|           |---__of_find_property
|              |
|              |---of_prop_cmp
|
|---of_read_number
{% endhighlight %}

函数作用：获得节点 reg 属性中的地址值。

> 平台： ARM32
>
> linux： 3.10/4.18/5.0

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
值；flags 存储 flags 标志函数首先获得节点对应的父节点，如果不存在，那么直接返
回 NULL，然后调用 of_match_bus() 函数获得节点对应的 of_bus 结构，并调用 
of_bus 的 count_cell 返回获得节点对应 bus 的 #address-cells 和 #size-cells 属
性值，并调用 OF_CHECK_ADDR_COUNT 宏检查 #address-cells 是否有效，如果无效，直
接返回 NULL；如果有效，那么函数就调用 of_get_property() 函数获得 ‘reg’或 
‘assigned-address’属性的值，如果获取失败，则直接返回 NULL；如果获取成功，由于
属性的整形值都是由 32 位数构成的，所以将获得属性长度除以 4 就可以获得属性中包
含多少个 32 位整数。onesize 变量用于标识一个完整的地址和数值所占用的 32 位整形
个数。接着调用 for 循环遍历属性里面的值，如果遍历的次数和 index 参数一直，那么
就读出其值并存储到参数 size 中，此时如果 flags 参数也有效，那么调用 bus 的 
get_flags 参数获得属性的值存储到 flags 参数里。

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
{% endhighlight %}

(假设 #address-cells 和 #size-cells 都是 2)

> reg 第一个成员的值是：     0x1122334455667788
>
> reg 第一个成员的地址是：  0x1020304050607080
>
> reg 第二个成员的值是：     0x99aabbccddeeff00
>
> reg 第二个成员的地址是：  0x90a0b0c0d0e0f000

所以 index 参数为 0 的时候，of_get_address() 函数返回 0x1020304050607080. 同理 
index 参数为 1 的时候，of_get_address() 函数返回 0x90a0b0c0d0e0f000

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
的 of_bus 结构，内核一共定义了三种 of_bus 结构，分别为 PCI，ISA 和默认 bus。循
环每遍历到一个 of_bus 的时候，都会检查 of_bus 结构是否提供了 match 方法和如果
提供了 match 方法之后运行 match 方法的结果，如果 of_bus 结构不存在 match 方法，
或者运行 match 方法之后返回值为真时，返回这个 of_bus 结构。如果遍历完之后还是
没找到对应的 of_bus 结构，那么返回 NULL。

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

参数 np 指向一个 device node；name 参数为需要查找属性的名字；lenp 参数用于存
储属性的长度。

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

参数 np 指向一个 device_node；name 参数表示需要查看属性的名字；lenp 用于存储
属性长度。

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
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到的
属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不为 
NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

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

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含 reg 属性，然后通过往 
of_get_address() 函数分别获得指定的地址值，函数定义如下：

{% highlight c %}
const __be32 *of_get_address(struct device_node *dev, int index, u64 *size,
            unsigned int *flags)
{% endhighlight %}

这个函数经常用用于读取节点 reg 属性中的地址。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点包含了 reg 属性，属性包含了 2 个值，第一个值的数值是 0x1122334455667788, 并且第一个值的地址是 0x1020304050607080; 第二个值的数值是 0x99aabbccddeeff00，并且第二个值的地址是 0x90a0b0c0d0e0f000。具体内容如下：

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
                reg = <0x11223344 0x55667788
                       0x10203040 0x50607080
                       0x99aabbcc 0xddeeff00
                       0x90a0b0c0 0xd0e0f000>;
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
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传递。获得驱动对应的节点之后，通过调用 of_get_address() 函数读取第一和第二个成员的地址，驱动编写如下：

{% highlight c %}
/*
 * Device-Tree: of_get_address
 *
 * const __be32 *of_get_address(struct device_node *dev, int index,
 *                    u64 *size, unsigned int *flags)
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
 *                reg = <0x11223344 0x55667788
 *                       0x10203040 0x50607080
 *                       0x99aabbcc 0xddeeff00
 *                       0x90a0b0c0 0xd0e0f000>;
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
    const u32 *regaddr_p;
    u64 addr;
    
    printk("DTS demo probe entence.\n");

    /* get first address from 'reg' property */
    regaddr_p = of_get_address(np, 0, &addr, NULL);
    if (regaddr_p)
        printk("%s's address[0]: %#llx\n", np->name, addr);

    /* get second address from 'reg' property */
    regaddr_p = of_get_address(np, 1, &addr, NULL);
    if (regaddr_p)
        printk("%s's address[1]: %#llx\n", np->name, addr);

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
[    3.565648] DTS_demo's address[0]: 0x102030405060780
[    3.565660] DTS_demo's address[1]: 0x90a0b0c0d0e0f00
{% endhighlight %}

-----------------------------------

# <span id="附录">附录</span>





