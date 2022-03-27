---
layout: post
title:  "of_find_node_by_path"
date:   2019-02-13 07:48:30 +0800
categories: [HW]
excerpt: DTS API of_find_node_by_path().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_find_node_by_path](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_find_node_by_path)
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

##### of_find_node_by_path

{% highlight ruby %}
of_find_node_by_path
|
|---raw_spin_lock_irqsave
|
|---of_node_cmp
|
|---of_node_get
|
|---raw_spin_unlock_irqrestore
{% endhighlight %}

函数作用：通过绝对路径获得 device node。

> 平台： ARM
>
> linux： 3.10/4.14/5.0

{% highlight ruby %}
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

函数首先定义了局部变量 np，并指向 of_allnodes 全局 device node 链表的头部，接
着调用 raw_spin_lock_irqsave() 函数加锁，并遍历所有的节点，每一次的遍历到的节
点，如果全路径 full_name 存在，并且与参数 path 一致，那么就找到指定的 
device node。找到之后调用 raw_spin_unlock_irqrestore() 解锁。

##### of_allnodes

device_node: all node single list of_allnodes

{% highlight ruby %}
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

----------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过 
of_find_node_by_path() 函数全路径对应的节点，函数定义如下：

{% highlight ruby %}
struct device_node *of_find_node_by_path(const char *path)
{% endhighlight %}

这个函数经常用用于通过全路径找到对应的节点。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。具体内容如下：

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
        };
        
};
{% endhighlight %}

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者在 Linux 4.20.8 的源
码中，找到 arch/arm/boot/dts/vexpress-v2p-ca9.dts 文件，然后在文件开始地方添
加如下内容：

{% highlight ruby %}
#include "DTS_demo.dtsi"
{% endhighlight %}

#### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_find_node_by_path() 函数找到指定的节点，
并打印 compatible 属性，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_find_node_by_path
 *
 * struct device_node *of_find_node_by_path(const char *path)
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
    struct device_node *node;
    const char *comp = NULL;

    printk("DTS probe entence...\n");

    /* find device by path */
    node = of_find_node_by_path("/DTS_demo");
    if (node) {
        of_property_read_string(node, "compatible", &comp);
        if (comp)
            printk("%s compatible: %s\n", node->name, comp);
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

{% highlight ruby %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight ruby %}
[    3.534323] DTS demo probe entence.
[    3.534359] DTS_demo compatible: DTS_demo, BiscuitOS
{% endhighlight %}

-------------------------------------

# <span id="附录">附录</span>
