---
layout: post
title:  "of_find_all_nodes"
date:   2019-02-01 14:31:30 +0800
categories: [HW]
excerpt: DTS API of_find_all_nodes().
tags:
  - DTS
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_find_all_nodes](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_find_all_nodes)
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

##### of_find_all_nodes

{% highlight ruby %}
of_find_all_nodes
|
|---of_node_get
{% endhighlight %}

函数作用：读取当前节点的下一个节点。

> 平台： ARM32
>
> linux： 3.10/4.14

{% highlight ruby %}
/**
* of_find_all_nodes - Get next node in global list
* @prev:    Previous node or NULL to start iteration
*        of_node_put() will be called on it
*
* Returns a node pointer with refcount incremented, use
* of_node_put() on it when done.
*/
struct device_node *of_find_all_nodes(struct device_node *prev)
{
    struct device_node *np;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    np = prev ? prev->allnext : of_allnodes;
    for (; np != NULL; np = np->allnext)
        if (of_node_get(np))
            break;
    of_node_put(prev);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return np;
}
EXPORT_SYMBOL(of_find_all_nodes);
{% endhighlight %}

参数 prev 指向前一个节点。

函数首先调用 raw_spin_lock_irqsave() 函数进行加锁操作，然后判断 prev 节点是否
存在，如果不存在，则将 np 指向 of_allnodes 链表；如果存在，则 np 指向 prev。使
用 for 循环遍历，每次遍历调用 of_node_get() 函数获得使用权，然后调用 
of_node_put() 释放 prev 节点的使用权，最后返回节点之前调用 
raw_spin_unlock_irqrestore() 函数解锁。

---------------------------------------------------

#<span id="实践">实践</span>

实践目的是在 DTS 文件中构建两个私有节点，第一个私有节点默认打开，第二个节点只
包含一个 compatible 属性，通过调用 of_find_all_nodes() 函数获得第一个节点之后
的下一个节点，最后打印节点的 compatible 属性值，函数定义如下：

{% highlight ruby %}
struct device_node *of_find_all_nodes(struct device_node *prev)
{% endhighlight %}

这个函数经常用用于读取当前节点的下一个节点。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。再创建一个名
为 phy1 的节点，具体内容如下：

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
        BiscuitOS-array  = <0x10203040 0x50607080
                                    0x90a0b0c0 0xd0e0f000>;
    };

    phy1: phy@1 {
        compatible = "PHY, BiscuitOS";
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
递。获得驱动对应的节点之后，通过调用 of_find_all_nodes() 函数获得下一个节点，
最后打印节点的 compatible 属性值。驱动编写如下：

{% highlight c %}
/*
 * DTS: of_find_all_nodes
 *
 * struct device_node *of_find_all_nodes(struct device_node *prev)
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
 *
 *        phy1: phy@1 {
 *                compatible = "PHY, BiscuitOS";
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
    struct device_node *npp;
    const char *comp;

    printk("DTS demo probe entence.\n");

    /* Find next device node */
    npp = of_find_all_nodes(np);
    if (!npp) {
        printk("Unable to get next device_node.\n");
        return -1;
    }
    
    /* Read compatible property */
    comp = of_get_property(npp, "compatible", NULL);
    if (!comp) {
        printk("Unable to read compatible.\n");
        return -1;
    }
    printk("%s compatible: %s\n", npp->name, comp);

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

{% highlight ruby %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight ruby %}
[    3.534323] DTS demo probe entence.
[    3.534359] phy compatible: PHY, BiscuitOS
{% endhighlight %}

-----------------------------------------------

#<span id="附录">附录</span>






