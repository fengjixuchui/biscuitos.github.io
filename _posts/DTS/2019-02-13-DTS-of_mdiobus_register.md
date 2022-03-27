---
layout: post
title:  "of_match_node"
date:   2019-02-22 14:51:30 +0800
categories: [HW]
excerpt: DTS API of_match_node().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_match_node](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_match_node)
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

##### of_match_node

函数作用：通过 device_id 列表获得指定的 device node 和 device id。

> 平台： ARM64
> 
> linux： 3.10/4.18/5.0

{% highlight c %}
/**
* of_match_node - Tell if an device_node has a matching of_match structure
*    @matches:    array of of device match structures to search in
*    @node:        the of device structure to match against
*
*    Low level utility function used by device matching.
*/
const struct of_device_id *of_match_node(const struct of_device_id *matches,
                     const struct device_node *node)
{
    const struct of_device_id *match;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    match = __of_match_node(matches, node);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return match;
}
EXPORT_SYMBOL(of_match_node);
{% endhighlight %}

参数 matches 指向 device id 列表；node 指向 device node

函数首先调用 raw_spin_lock_irqsave() 函数加锁，然后调用 __of_match_node() 函数
获得节点对应的 device_id，获得成功之后返回 device_id, 最后返回之前调用 
raw_spin_unlock_irqrestore() 解锁。

##### __of_match_node

{% highlight c %}
static
const struct of_device_id *__of_match_node(const struct of_device_id *matches,
                       const struct device_node *node)
{
    if (!matches)
        return NULL;

    while (matches->name[0] || matches->type[0] || matches->compatible[0]) {
        int match = 1;
        if (matches->name[0])
            match &= node->name
                && !strcmp(matches->name, node->name);
        if (matches->type[0])
            match &= node->type
                && !strcmp(matches->type, node->type);
        if (matches->compatible[0])
            match &= __of_device_is_compatible(node,
                               matches->compatible);
        if (match)
            return matches;
        matches++;
    }
    return NULL;
}
{% endhighlight %}

参数 matches 指向一个 device_id 列表；参数 node 指向一个 device node

函数首先检查 matches 参数的有效性，如果不符合条件，直接返回 NULL。接着函数调用 
while 循环，只要 matches device_id 的 name，type，或者 compatible 成员不为零，
那么 while 循环不会停止。在每次循环中，函数获得 matches 中的一个 device id 结
构，如果 device id 的 name 存在，那么 match 指向节点的名字，同理检查 type 和 
compatible 成员，如果其中一个条件满足，那么就返回 device id。

--------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过调用 
of_match_node() 函数获得节点对应的 device id，函数定义如下：

{% highlight c %}
const struct of_device_id *of_match_node(const struct of_device_id *matches,
                     const struct device_node *node)
{% endhighlight %}

这个函数经常用用于通过 device id 找到对应的 device node。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，在 root 节点之下创建一个名为 DTS_demo 的子节点，节点默认打开。具体内容如下：

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

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备
驱动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_match_node() 函数获得节点对应的 device 
id，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_match_node
 *
 * const struct of_device_id *of_match_node(const struct of_device_id *matches,
 *                            const struct device_node *node)
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

static const struct of_device_id DTS_demo_of_match[] = {
    { .compatible = "DTS_demo, BiscuitOS",  },
    { .compatible = "DTS_demo, BiscuitOSX", },
    { .compatible = "DTS_demo, BiscuitOSM", },
    { },
};
MODULE_DEVICE_TABLE(of, DTS_demo_of_match);

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const struct of_device_id *match;

    printk("DTS probe entence...\n");

    /* Match a device node */
    match = of_match_node(&DTS_demo_of_match, np);
    if (!match) {
        printk("Faild to invoke of_match_node()\n");
        return -1;
    }
    printk("match compatible: %s\n", match->compatible);

    return 0;
}

/* remove platform driver */
static int DTS_demo_remove(struct platform_device *pdev)
{
    return 0;
}

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
[    3.565634] DTS demo probe entence...
[    3.565645] match compatible:: DTS_demo, BiscuitOS
{% endhighlight %}

--------------------------------------

# <span id="附录">附录</span>


