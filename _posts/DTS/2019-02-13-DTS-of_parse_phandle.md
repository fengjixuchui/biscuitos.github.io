---
layout: post
title:  "of_parse_phandle"
date:   2019-02-22 16:43:30 +0800
categories: [HW]
excerpt: DTS API of_parse_phandle().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_parse_phandle](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_parse_phandle)
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

##### of_parse_phandle

{% highlight c %}
of_parse_phandle
|
|---of_find_node_by_phandle
    |
    |---of_node_get
{% endhighlight %}

函数作用：通过当前节点的 phandle 属性，获得一个 device node 节点。

> 平台： ARM32
> 
> linux： 3.10/4.14

{% highlight c %}
/**
* of_parse_phandle - Resolve a phandle property to a device_node pointer
* @np: Pointer to device node holding phandle property
* @phandle_name: Name of property holding a phandle value
* @index: For properties holding a table of phandles, this is the index into
*         the table
*
* Returns the device_node pointer with refcount incremented.  Use
* of_node_put() on it when done.
*/
struct device_node *of_parse_phandle(const struct device_node *np,
                     const char *phandle_name, int index)
{
    const __be32 *phandle;
    int size;

    phandle = of_get_property(np, phandle_name, &size);
    if ((!phandle) || (size < sizeof(*phandle) * (index + 1)))
        return NULL;

    return of_find_node_by_phandle(be32_to_cpup(phandle + index));
}
EXPORT_SYMBOL(of_parse_phandle);
{% endhighlight %}

参数 np 指向 device_node 节点；phandle_name 参数说明 phandle 的名字；index 
指向 phandle 的索引，通常为 0.

函数首先调用 of_get_property() 函数获得 phandle_name 参数对应的属性值，然后检
查属性值的合法性和有效性。最后调用 of_find_node_by_phandle() 函数和 
be32_to_cpup() 函数找到对应的节点。

##### of_find_node_by_phandle

{% highlight c %}
/**
* of_find_node_by_phandle - Find a node given a phandle
* @handle:    phandle of the node to find
*
* Returns a node pointer with refcount incremented, use
* of_node_put() on it when done.
*/
struct device_node *of_find_node_by_phandle(phandle handle)
{
    struct device_node *np;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    for (np = of_allnodes; np; np = np->allnext)
        if (np->phandle == handle)
            break;
    of_node_get(np);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return np;
}
EXPORT_SYMBOL(of_find_node_by_phandle);
{% endhighlight %}

参数 handle 指向节点中 phandle 的属性值。

函数首先调用 raw_spin_lock_irqsave() 函数加锁。由于 DTB 将所有节点都存放在 
of_allnodes 为表头的单链表里，然后调用 for 循环遍历所有节点。每次遍历一个节点，
如果节点 device_node 的 phandle 成员和遍历到的节点一致，那么就找到 phandle 对
应的节点。接着停止 for 循环，调用 of_node_get() 函数添加节点引用数。最后返回 
device_node 之前调用 raw_spin_unlock_irqrestore() 函数释放锁。

---------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，节点里面包含了两个
子节点，通过调用 of_parse_phandle() 函数获得子节点，最后打印子节点里面的属性
值，函数定义如下：

{% highlight c %}
struct device_node *of_parse_phandle(const struct device_node *np,
                     const char *phandle_name, int index)
{% endhighlight %}

这个函数经常用用于读取节点引用的其他节点。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。具体内容如下：

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
        phy-handle = <&phy0>;

        phy0: phy@0 {
            compatible = "PHYX, BiscuitOS";
            reg = <0>;
        };

        phy1: phy@1 {
            compatible = "PHY, BiscuitOS";
            reg = <1>;
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
递。获得驱动对应的节点之后，通过调用 of_get_property() 函数 phy-phandle 的属性
值，其指向 phy0 节点，然后调用 of_parse_phandle() 函数获得 phy0 的 device_node。
最后打印 phy0 的 compatible 属性值。驱动编写如下：

{% highlight c %}
/*
 * DTS: of_parse_phandle
 *
 * struct device_node *of_parse_phandle(const struct device_node *np,
 *                          const char *phandle_name, int index)
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
 *                phy-handle = <&phy1>;
 *
 *                phy0: phy@0 {
 *                        compatible = "PHYX, BiscuitOS";
 *                        reg = <0>;
 *                };
 *
 *                phy1: phy@1 {
 *                        compatible = "PHY, BiscuitOS";
 *                        reg = <1>;
 *                };
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
    const phandle *ph;
    struct device_node *phy;
    const char *value;

    printk("DTS demo probe entence.\n");

    phy = of_parse_phandle(np, "phy-handle", 0);
    if (!phy) {
        printk("Unable read phy-handle property\n");
        return -1;
    }

    /* Read property from special device node */
    value = of_get_property(phy, "compatible", NULL);
    if (value)
        printk("PHY: %s\n", value);

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
[    3.534323] DTS demo probe entence.
[    3.534359] PHY：PHYX, BiscuitOS
{% endhighlight %}

-------------------------------------------

# <span id="附录">附录</span>
