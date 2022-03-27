---
layout: post
title:  "for_each_matching_node_and_match"
date:   2019-01-20 09:24:30 +0800
categories: [HW]
excerpt: DTS API for_each_matching_node_and_match().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [for_each_matching_node_and_match](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/for_each_matching_node_and_match)
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

##### of_find_matching_node_and_match

函数作用：通过 device_id 列表获得指定的 device node 和 device id。

> 平台： ARM32
>
> linux： 3.10/4.18/5.0

{% highlight ruby %}
#define for_each_matching_node_and_match(dn, matches, match) \
    for (dn = of_find_matching_node_and_match(NULL, matches, match); \
         dn; dn = of_find_matching_node_and_match(dn, matches, match))
{% endhighlight %}

参数 dn 指向 device node；参数 matches 指向 device id list；参数 match 是一个
二级指针，用于存储 device id

函数首先调用 of_find_matching_node_and_match(NULL, matches, match) 函数获得第
一个 device id 对应的 device node 和 device id，device node 存储在 dn 中，只
要 dn 不为 NULL，那么循环继续。接着调用 of_find_matching_node_and_match(dn, 
matches, match) 函数获得下一个 device id 对应的 device node 和 device id，
device node 存储在 dn 中。

##### of_find_matching_node_and_match

{% highlight ruby %}
/**
*    of_find_matching_node_and_match - Find a node based on an of_device_id
*                      match table.
*    @from:        The node to start searching from or NULL, the node
*            you pass will not be searched, only the next one
*            will; typically, you pass what the previous call
*            returned. of_node_put() will be called on it
*    @matches:    array of of device match structures to search in
*    @match        Updated to point at the matches entry which matched
*
*    Returns a node pointer with refcount incremented, use
*    of_node_put() on it when done.
*/
struct device_node *of_find_matching_node_and_match(struct device_node *from,
                    const struct of_device_id *matches,
                    const struct of_device_id **match)
{
    struct device_node *np;
    const struct of_device_id *m;
    unsigned long flags;

    if (match)
        *match = NULL;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    np = from ? from->allnext : of_allnodes;
    for (; np; np = np->allnext) {
        m = __of_match_node(matches, np);
        if (m && of_node_get(np)) {
            if (match)
                *match = m;
            break;
        }
    }
    of_node_put(from);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return np;
}
EXPORT_SYMBOL(of_find_matching_node_and_match);
{% endhighlight %}

参数 from 指向开始查找的 device node；matches 指向一个已知的 device_id 列表；
match 是一个二级指针，用于存储找到的 device_id.

函数首先调用 raw_spin_lock_irqsave() 函数加锁，然后判断 from 参数是否存在，如
果存在，则将 np 变量指向 from 的 allnext 成员；如果 from 不存在，则将 np 变量
指向 of_allnodes 根节点。然后使用 for 循环遍历每一个节点。在每次遍历中，函数会
将获得的节点和 matches 参数传递给 __of_match_node() 函数，以此找到 matches 中
描述的节点。当找到后就返回这个节点，并将对应的 device_id 存储在 match 参数里。
最后返回之前调用 raw_spin_unlock_irqrestore() 函数解锁。

##### __of_match_node

{% highlight ruby %}
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

函数首先检查 matches 参数的有效性，如果不符合条件，直接返回 NULL。接着函数调
用 while 循环，只要 matches device_id 的 name，type，或者 compatible 成员不为
零，那么 while 循环不会停止。在每次循环中，函数获得 matches 中的一个 
device id 结构，如果 device id 的 name 存在，那么 match 指向节点的名字，同理
检查 type 和 compatible 成员，如果其中一个条件满足，那么就返回 device id。

----------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过调用 
for_each_matching_node_and_match() 函数获得所有节点和对应的 device id，函数
定义如下：

{% highlight ruby %}
#define for_each_matching_node_and_match(dn, matches, match) \
    for (dn = of_find_matching_node_and_match(NULL, matches, match); \
         dn; dn = of_find_matching_node_and_match(dn, matches, match))
{% endhighlight %}

这个函数经常用用于通过 device id list 找到对应的 device node 和 device id。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 
文件，在 root 节点之下创建一个名为 DTS_demo 的子节点，节点默认打开。具体内容
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
        };
        DTS_demoX {
                compatible = "DTS_demo, BiscuitOSX";
                status = "disabled";
        };
        DTS_demoY {
                compatible = "DTS_demo, BiscuitOSY";
                status = "disabled";
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
递。获得驱动对应的节点之后，通过调用 for_each_matching_node_and_match() 函数获
得节点和对应的 device id，驱动编写如下：

{% highlight ruby %}
/*
 * DTS: for_each_matching_node_and_match
 *
 * #define for_each_matching_node_and_match(dn,matches,match) \
 *    for(dn = of_find_matching_node_and_match(NULL,matches,match);dn;\
 *        dn = of_find_matching_node_and_match(dn,matches,match))
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
 *        DTS_demoX {
 *                compatible = "DTS_demo, BiscuitOSX";
 *                status = "disabled";
 *        };
 *        DTS_demoY {
 *                compatible = "DTS_demo, BiscuitOSY";
 *                status = "disabled";
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

static const struct of_device_id DTS_demo_of_match[] = {
    { .compatible = "DTS_demo, BiscuitOS",  },
    { .compatible = "DTS_demo, BiscuitOSX",  },
    { .compatible = "DTS_demo, BiscuitOSY",  },
    { },
};
MODULE_DEVICE_TABLE(of, DTS_demo_of_match);

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *node;
    const struct of_device_id *match;

    printk("DTS probe entence...\n");

    /* find all device nodes via device node id */
    for_each_matching_node_and_match(node, DTS_demo_of_match, &match) {
        if (node)
            printk("Matching %s\n", node->name);
        if (match)
            printk("device_id compatible: %s\n", match->compatible);
    }

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

{% highlight ruby %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight ruby %}
[    3.565634] DTS demo probe entence...
[    3.565645] Matching DTS_demo
[    3.565645] device_id compatible: DTS_demo, BiscuitOS
[    3.565645] Matching DTS_demoX
[    3.565645] device_id compatible: DTS_demo, BiscuitOSX
[    3.565645] Matching DTS_demoY
[    3.565645] device_id compatible: DTS_demo, BiscuitOSY
{% endhighlight %}

-------------------------------------------

# <span id="附录">附录</span>

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
