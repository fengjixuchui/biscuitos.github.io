---
layout: post
title:  "of_get_cpu_node"
date:   2019-02-22 10:03:30 +0800
categories: [HW]
excerpt: DTS API of_get_cpu_node().
tags:
  - DTS
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_get_cpu_node](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_get_cpu_node)
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

##### of_get_cpu_node

{% highlight c %}
of_match_node
|
|---__of_match_node
    |
    |---__of_device_is_compatible
{% endhighlight %}

函数作用：获得当前节点对应的 platform_device。

> 平台： ARM32
>
> linux： 3.10/4.14/5.0

{% highlight c %}
/**
* of_get_cpu_node - Get device node associated with the given logical CPU
*
* @cpu: CPU number(logical index) for which device node is required
* @thread: if not NULL, local thread number within the physical core is
*          returned
*
* The main purpose of this function is to retrieve the device node for the
* given logical CPU index. It should be used to initialize the of_node in
* cpu device. Once of_node in cpu device is populated, all the further
* references can use that instead.
*
* CPU logical to physical index mapping is architecture specific and is built
* before booting secondary cores. This function uses arch_match_cpu_phys_id
* which can be overridden by architecture specific implementation.
*
* Returns a node pointer for the logical cpu if found, else NULL.
*/
struct device_node *of_get_cpu_node(int cpu, unsigned int *thread)
{
    struct device_node *cpun, *cpus;

    cpus = of_find_node_by_path("/cpus");
    if (!cpus) {
        pr_warn("Missing cpus node, bailing out\n");
        return NULL;
    }

    for_each_child_of_node(cpus, cpun) {
        if (of_node_cmp(cpun->type, "cpu"))
            continue;
        /* Check for non-standard "ibm,ppc-interrupt-server#s" property
         * for thread ids on PowerPC. If it doesn't exist fallback to
         * standard "reg" property.
         */
        if (IS_ENABLED(CONFIG_PPC) &&
            __of_find_n_match_cpu_property(cpun,
                "ibm,ppc-interrupt-server#s", cpu, thread))
            return cpun;
        if (__of_find_n_match_cpu_property(cpun, "reg", cpu, thread))
            return cpun;
    }
    return NULL;
}
EXPORT_SYMBOL(of_get_cpu_node);
{% endhighlight %}

参数 cpu 指向 local cpu id；thread 存储线程数

函数首先调用 of_find_node_by_path() 函数找到 “/cpus”路径对应的节点，然后调用 
for_each_child_of_node() 函数遍历节点的所有子节点。每遍历到一个子节点，都会检
查节点的名字是否是 “cpu”，如果不是则继续下一次遍历；如果是，则调用 
__of_find_n_match_cpu_property() 函数，检查子节点的 “reg” 参数是否符合特定的
要求，如果符合就返回这个子节点。

##### __of_find_n_match_cpu_property

{% highlight c %}
/**
* Checks if the given "prop_name" property holds the physical id of the
* core/thread corresponding to the logical cpu 'cpu'. If 'thread' is not
* NULL, local thread number within the core is returned in it.
*/
static bool __of_find_n_match_cpu_property(struct device_node *cpun,
            const char *prop_name, int cpu, unsigned int *thread)
{
    const __be32 *cell;
    int ac, prop_len, tid;
    u64 hwid;

    ac = of_n_addr_cells(cpun);
    cell = of_get_property(cpun, prop_name, &prop_len);
    if (!cell)
        return false;
    prop_len /= sizeof(*cell);
    for (tid = 0; tid < prop_len; tid++) {
        hwid = of_read_number(cell, ac);
        if (arch_match_cpu_phys_id(cpu, hwid)) {
            if (thread)
                *thread = tid;
            return true;
        }
        cell += ac;
    }
    return false;
}
{% endhighlight %}

参数 cpun 指向特定的节点；prop_name 指向属性名字；cpu 参数执行 cpu id；thread 
参数存储 thread 数量。

函数首先调用 of_n_addr_cells() 函数获得节点 #address-cells 的值，以此表示地址
值的长度。然后调用 of_get_property() 函数获得 cpun 节点中 prop_name 属性的值，
如果属性值不存在，则直接返回 false；如果属性值存在，就调用 of_read_number() 函
数读取属性值，这里就是读取 cpu id 的值。接着调用 arch_match_cpu_phys_id() 函数
检查 cpu id 的值是否符合要求，符合则返回 true；不符合就继续读取属性的下一个值。

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

函数使用 while 循环，每遍历依次，数据长度增加 32 位，并从中读取 32 位数据，
最后以为合并指定长度的数据。

##### of_find_node_by_path

{% highlight c %}
/**
*    of_find_node_by_path - Find a node matching a full OF path
*    @path:    The full path to match
*
*    Returns a node pointer with refcount incremented, use
*    of_node_put() on it when done.
*/
struct device_node *of_find_node_by_path(const char *path)
{
    struct device_node *np = of_allnodes;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    for (; np; np = np->allnext) {
        if (np->full_name && (of_node_cmp(np->full_name, path) == 0)
            && of_node_get(np))
            break;
    }
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return np;
}
EXPORT_SYMBOL(of_find_node_by_path);
{% endhighlight %}

参数 path 指向一个绝对路径。

函数首先定义了局部变量 np，并指向 of_allnodes 全局 device node 链表的头部，
接着调用 raw_spin_lock_irqsave() 函数加锁，并遍历所有的节点，每一次的遍历到的
节点，如果全路径 full_name 存在，并且与参数 path 一致，那么就找到指定的 
device node。找到之后调用 raw_spin_unlock_irqrestore() 解锁。

##### of_allnodes

{% highlight c %}
device_node: all node single list of_allnodes

+-------------+         +-------------+         +-------------+         +-------------+         +-------------+         
|             |         |             |         |             |         |             |         |             |         
| device_node |         | device_node |         | device_node |         | device_node |         | device_node |         
|             |         |             |         |             |         |             |         |             |       allnextpp
|    allnext -|-------->|    allnext -|-------->|    allnext -|-------->|    allnext -|-------->|    allnext -|------------------->
|             |         |             |         |             |         |             |         |             |         
+-------------+         +-------------+         +-------------+         +-------------+         +-------------+         
{% endhighlight %}

DTS 机制中，将从 DTB 解析出来的所有节点都维护在一个单链表中，链表的表头是 
of_allnodes，device_node 通过 allnext 成员指向下一个 device_node.

##### of_n_addr_cells

{% highlight c %}
int of_n_addr_cells(struct device_node *np)
{
    const __be32 *ip;

    do {
        if (np->parent)
            np = np->parent;
        ip = of_get_property(np, "#address-cells", NULL);
        if (ip)
            return be32_to_cpup(ip);
    } while (np->parent);
    /* No #address-cells property for the root node */
    return OF_ROOT_NODE_ADDR_CELLS_DEFAULT;
}
EXPORT_SYMBOL(of_n_addr_cells);
{% endhighlight %}

参数 np 指向 device_node。

函数首先确认节点是否包含父节点，如果包含，则局部变量指向父节点。然后调用 
of_get_proerpty() 函数获得 np 节点的 #address-cells 属性，如果属性存在，则调用 
be32_to_cpup() 函数返回属性值；如果属性不存在，在使用 while 循环查找 np 对应的
父节点，直到找到 #address-cells 属性 为止。

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

函数通过调用 of_find_property() 函数获得属性的结构 property，如果获得成功，
则返回属性的值；如果失败则返回 NULL。

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
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到
的属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不
为 NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

-----------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过 
of_get_cpu_node() 函数找到 cpu 节点，函数定义如下：

{% highlight c %}
struct device_node *of_get_cpu_node(int cpu, unsigned int *thread)
{% endhighlight %}

这个函数经常用用于获得 local cpu 节点。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。具体内容如下：

{% highlight c %}
/*
 * DTS Demo Code
 *
 * (C) 2019.01.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/ {
        DTS_demo {
                compatible = "DTS_demo, BiscuitOS";
                status = "okay";
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

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_get_cpu_node() 函数获得节点对应的 
compatible 属性并打印，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_get_cpu_node
 *
 * struct device_node *of_get_cpu_node(int cpu, unsigned int *thread)
 *
 * (C) 2019.01.11 BuddyZhang1 <buddy.zhang@aliyun.com>
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
 *        };
 * };
 *
 * On Core dtsi:
 *
 * include "DTS_demo.dtsi"
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of_platform.h>

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const struct of_device_id *match;
    struct device_node *cpu_node;
    const char *comp = NULL;

    printk("DTS probe entence...\n");

    /* Read local CPU 0 */
    cpu_node = of_get_cpu_node(0, NULL);
    if (cpu_node) {
        printk("CPU0: %s\n", cpu_node->name);
        of_property_read_string(cpu_node, "compatible", &comp);
        if (comp)
            printk("  Compatible: %s\n", comp);
    }

    /* Read local CPU 1 */
    cpu_node = of_get_cpu_node(1, NULL);
    if (cpu_node) {
        printk("CPU1: %s\n", cpu_node->name);
        of_property_read_string(cpu_node, "compatible", &comp);
        if (comp)
            printk("  Compatible: %s\n", comp);
    }

    /* Read local CPU 2 */
    cpu_node = of_get_cpu_node(2, NULL);
    if (cpu_node) {
        printk("CPU2: %s\n", cpu_node->name);
        of_property_read_string(cpu_node, "compatible", &comp);
        if (comp)
            printk("  Compatible: %s\n", comp);
    }

    /* Read local CPU 3 */
    cpu_node = of_get_cpu_node(3, NULL);
    if (cpu_node) {
        printk("CPU3: %s\n", cpu_node->name);
        of_property_read_string(cpu_node, "compatible", &comp);
        if (comp)
            printk("  Compatible: %s\n", comp);
    }

    return 0;
}

/* remove platform driver */
static int DTS_demo_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id DTS_demo_of_match[] = {
    { .compatible = "DTS_demo, BiscuitOS",  },
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

{% highlight bash %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight bash %}
[    3.534323] DTS demo probe entence.
[    3.534359] CPU0: cpu
[    3.534359]   Compatible: arm,cortex-a53
[    3.534359] CPU1: cpu
[    3.534359]   Compatible: arm,cortex-a53
[    3.534359] CPU2: cpu
[    3.534359]   Compatible: arm,cortex-a53
[    3.534359] CPU3: cpu
[    3.534359]   Compatible: arm,cortex-a53
{% endhighlight %}

--------------------------------------------

# <span id="附录">附录</span>
