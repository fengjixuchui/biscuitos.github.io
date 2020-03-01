---
layout: post
title:  "for_each_child_of_node"
date:   2019-01-19 09:12:30 +0800
categories: [HW]
excerpt: DTS API for_each_child_of_node().
tags:
  - DTS
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/DEV000106.jpg)

> Github: [for_each_child_of_node](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/for_each_child_of_node)
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

##### for_each_child_of_node

{% highlight ruby %}
for_each_child_of_node
|
|---of_get_next_child
    |
    |---raw_spin_lock_irqsave  
    |
    |---raw_spin_unlock_irqrestore
{% endhighlight %}

函数作用：用于遍历节点里的所有子节点。

> 平台： ARM32
>
> linux： 3.10/4.14/5.0

{% highlight ruby %}
#define for_each_child_of_node(parent, child) \
    for (child = of_get_next_child(parent, NULL); child != NULL; \
         child = of_get_next_child(parent, child))
{% endhighlight %}

参数 parent 指向当前节点；child 指向子节点

函数在 for 循环中，调用 of_get_next_child(parent，NULL) 函数 parent 的第一个
子节点，只要 child 不为 NULL，循环继续运行。函数继续调用 
of_get_next_child(parent, child) 函数获得 child 子节点的下一个子节点，以此达
到循环的效果。

##### of_get_next_child

{% highlight ruby %}
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

函数首先调用 raw_spin_lock_irqsave() 函数加锁，然后检查 prev 是否存在，如果
存在，则 next 变量指向 prev 节点的 sibling 变量；如果 prev 不存在，那么 next 
指向节点的 child。接着从节点的 sibling 链表中获得子节点，然后调用 
of_node_get() 获得节点的父节点，最后返回之前调用 raw_spin_unlock_irqrestore() 
函数解锁。返回子节点。

---------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过 
for_each_child_of_node() 函数遍历节点的所有子节点，并打印子节点的名字，函数
定义如下：

{% highlight ruby %}
#define for_each_child_of_node(parent, child) \
    for (child = of_get_next_child(parent, NULL); child != NULL; \
         child = of_get_next_child(parent, child))
{% endhighlight %}

这个函数经常用用于遍历当前节点的所有子节点。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。具体内容
如下：

{% highlight ruby %}
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
                        status = "disabled";
                };

                child1 {
                        compatbile = "Child1, BiscuitOS";
                        status = "okay";
                };
        };
};
{% endhighlight %}

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者在 Linux 4.20.8 的源码
中，找到 arch/arm/boot/dts/vexpress-v2p-ca9.dts 文件，然后在文件开始地方添
加如下内容：

{% highlight ruby %}
#include "DTS_demo.dtsi"
{% endhighlight %}

#### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备
驱动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动
的 probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 
成员传递。获得驱动对应的节点之后，通过调用 for_each_child_of_node() 函数遍历
节点的所有子节点，并打印子节点的名字，驱动编写如下：

{% highlight ruby %}
/*
 * DTS: of_get_child_count
 *      for_each_child_of_node
 *      for_each_available_of_node
 *
 * static inline int of_get_child_count(const struct device_node *np)
 *
 * #define for_each_child_of_node(parent, child) \
 *         for (child = of_get_next_child(parent, NULL); child != NULL; \
 *              child = of_get_next_child(parent, child))
 *
 * #define for_each_available_child_of_node(parent, child) \
 *     for (child = of_get_next_available_child(parent, NULL); child != NULL; \
 *          child = of_get_next_available_child(parent, child))
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
 *                         status = "disabled";
 *                };
 *
 *                child1 {
 *                         compatbile = "Child1, BiscuitOS";
 *                         status = "okay";
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
    struct device_node *child;
    int count;
  
    printk("DTS probe entence...\n");

    /* Count child number for current device node */
    count = of_get_child_count(np);
    printk("%s has %d children\n", np->name, count);

    printk("%s child:\n", np->name);
    for_each_child_of_node(np, child)
        printk("  \"%s\"\n", child->name);

    printk("%s available child:\n", np->name);
    for_each_available_child_of_node(np, child)
        printk("  \"%s\"\n", child->name);

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

{% highlight ruby %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight ruby %}
[    3.581300] DTS probe entence...
[    3.581316] DTS_demo has 2 children
[    3.581332] DTS_demo child:
[    3.581347]   "child0"
[    3.581362]   "child1"
[    3.581371] DTS_demo available child:
[    3.581378]   "child1"
{% endhighlight %}

----------------------------------

# <span id="附录">附录</span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

