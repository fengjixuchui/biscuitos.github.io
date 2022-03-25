---
layout: post
title:  "of_get_next_child"
date:   2019-02-22 12:08:30 +0800
categories: [HW]
excerpt: DTS API of_get_next_child().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_get_next_child](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_get_next_child)
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

##### of_get_next_child

{% highlight c %}
of_get_next_child
|
|---raw_spin_lock_irqsave
|
|---raw_spin_unlock_irqrestore
{% endhighlight %}

函数作用：用于获得节点的子节点。

> 平台： ARM64
>
> linux： 3.10/4.14/5.0

{% highlight c %}
/**
*    of_get_next_child - Iterate a node childs
*    @node:    parent node
*    @prev:    previous child of the parent node, or NULL to get first
*
*    Returns a node pointer with refcount incremented, use
*    of_node_put() on it when done.
*/
struct device_node *of_get_next_child(const struct device_node *node,
    struct device_node *prev)
{
    struct device_node *next;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    next = prev ? prev->sibling : node->child;
    for (; next; next = next->sibling)
        if (of_node_get(next))
            break;
    of_node_put(prev);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return next;
}
EXPORT_SYMBOL(of_get_next_child);
{% endhighlight %}

参数 node 指向设备节点。prev 参数为前一个子节点

函数首先调用 raw_spin_lock_irqsave() 函数加锁，然后检查 prev 是否存在，如果存
在，则 next 变量指向 prev 节点的 sibling 变量；如果 prev 不存在，那么 next 指
向节点的 child。接着从节点的 sibling 链表中获得子节点，然后调用 of_node_get() 
获得节点的父节点，最后返回之前调用 raw_spin_unlock_irqrestore() 函数解锁。返回
子节点。

-----------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过 
of_get_next_child() 函数获得子节点，并打印节点和子节点的全路径，函数定义如下：

{% highlight c %}
struct device_node *of_get_next_child(const struct device_node *node,
    struct device_node *prev)
{% endhighlight %}

这个函数经常用用于获得节点的子节点。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，
在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。具体内容如下：

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

                child0 {
                        compatible = "Child0, BiscuitOS";
                };

                child1 {
                        compatbile = "Child1, BiscuitOS";
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

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_get_next_child() 函数获得子节点，并打
印节点和子节点的全路径，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_get_parent
 *      of_get_next_parent
 *      of_get_next_child
 *      of_get_next_available_child
 *
 * struct device_node *of_get_parent(const struct device_node *node)
 *
 * struct device_node *of_get_next_parent(struct device_node *node)
 *
 * struct device_node *of_get_next_child(const struct device_node *node,
 *                                       struct device_node *prev)
 *
 * struct device_node *of_get_next_available_child(
 *                                       const struct device_node *node,
 *                                       struct device_node *prev)
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
 *
 *                child0 {
 *                         compatible = "Child0, BiscuitOS";
 *                };
 *
 *                child1 {
 *                         compatbile = "Child1, BiscuitOS";
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
#include <linux/module.h>

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_node *parent;
    struct device_node *nparent;
    struct device_node *child;
    struct device_node *nchild;
  
    printk("DTS probe entence...\n");

    /* Find child */
    child = of_get_next_child(np, NULL);
    if (child)
        printk("\"%s\"'s child: \"%s\"\n", np->full_name, child->full_name);
    else
        printk("The device node doesn't have child\n");

    /* Find next child */
    nchild = of_get_next_available_child(np, child);
    if (nchild)
        printk("\"%s\"'s next child: \"%s\"\n", np->full_name,
                                                nchild->full_name);

    /* find parent */
    parent = of_get_parent(child);
    if (parent)
        printk("\"%s\"'s parent: \"%s\"\n", child->full_name,
                                            parent->full_name);

    /* Lterate to a node's parent */
    nparent = of_get_next_parent(child);
    if (nparent)
        printk("\"%s\"'s next parent: \"%s\"\n", child->full_name,
                                                 parent->full_name);

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
[    3.581300] DTS probe entence...
[    3.581316] "/DTS_demo"'s child: "/DTS_demo/child0"
[    3.581332] "/DTS_demo"'s next child: "/DTS_demo/child1"
[    3.581347] "/DTS_demo/child0"'s parent: "/DTS_demo"
[    3.581362] "/DTS_demo/child0"'s next parent: "/DTS_demo"
{% endhighlight %}

---------------------------------------

# <span id="附录">附录</span>



