---
layout: post
title:  "for_each_node_with_property"
date:   2019-01-20 09:37:30 +0800
categories: [HW]
excerpt: DTS API for_each_node_with_property().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [for_each_node_with_property](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/for_each_node_with_property)
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

##### for_each_node_with_property

{% highlight ruby %}
for_each_node_with_property
|
|---of_find_node_with_property
    |
    |---of_compat_cmp
        |
        |---strcmp
{% endhighlight %}

函数作用：通过属性名找到所有对应的节点。

> 平台： ARM32
>
> linux： 3.10/4.18/5.0

{% highlight ruby %}
#define for_each_node_with_property(dn, prop_name) \
    for (dn = of_find_node_with_property(NULL, prop_name); dn; \
         dn = of_find_node_with_property(dn, prop_name))
{% endhighlight %}

参数 dn 指向 device node；prop_name 参数指向属性的名字

函数首先调用 of_find_node_with_property(NULL, prop_name) 找到第一个符合要求的
节点，如果节点不为 NULL，则继续循环。循环完一次之后，调用 
of_find_node_with_property(dn, prop_name) 函数查找下一个符合要求的节点。

##### of_find_node_with_property

{% highlight ruby %}
/**
*    of_find_node_with_property - Find a node which has a property with
*                                   the given name.
*    @from:        The node to start searching from or NULL, the node
*            you pass will not be searched, only the next one
*            will; typically, you pass what the previous call
*            returned. of_node_put() will be called on it
*    @prop_name:    The name of the property to look for.
*
*    Returns a node pointer with refcount incremented, use
*    of_node_put() on it when done.
*/
struct device_node *of_find_node_with_property(struct device_node *from,
    const char *prop_name)
{
    struct device_node *np;
    struct property *pp;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    np = from ? from->allnext : of_allnodes;
    for (; np; np = np->allnext) {
        for (pp = np->properties; pp; pp = pp->next) {
            if (of_prop_cmp(pp->name, prop_name) == 0) {
                of_node_get(np);
                goto out;
            }
        }
    }
out:
    of_node_put(from);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return np;
}
EXPORT_SYMBOL(of_find_node_with_property);
{% endhighlight %}

参数 from 指向 device node；prop_name 指向属性的名字

函数首先调用 raw_spin_lock_irqsave() 函数进行加锁操作，然后判断 from 是否存
在，如果不存在，则 np 变量指向 of_allnodes 节点；如果 from 存在，则 np 变量
指向 from 的 allnext 指向的节点。接着调用 for 循环遍历节点，每次遍历到的节点
后，再使用 for 循环遍历节点的属性链表 properties，调用 of_prop_name() 函数，
只要属性的名字和参数 prop_name 相同，那么就找到节点，并返回。最后在返回之前
调用 raw_spin_unlock_irqrestore() 函数解锁。

##### of_prop_cmp

{% highlight ruby %}
#define of_prop_cmp(s1, s2)        strcmp((s1), (s2))
{% endhighlight %}

参数 s1,s2 都为字符串；

函数直接调用 strcmp() 函数对比两个字符串是否相同，如果相同则返回 0.

---------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过调用 
for_each_node_with_property() 函数通过属性名找到所有节点，函数定义如下：

{% highlight ruby %}
#define for_each_node_with_property(dn, prop_name) \
    for (dn = of_find_node_with_property(NULL, prop_name); dn; \
         dn = of_find_node_with_property(dn, prop_name))
{% endhighlight %}

这个函数经常用用于通过属性名找所有节点

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建名为 DTS_demo，DTS_demoX，和 DTS_demoY 的子节点，
DTS_demo 节点默认打开。具体内容如下：

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
                BiscuitOs-name = "BiscuitOS0";
                status = "okay";
        };
        DTS_demoX {
                compatible = "DTS_demo, BiscuitOSX";
                BiscuitOs-name = "BiscuitOS1";
                status = "disabled";
        };
        DTS_demoY {
                compatible = "DTS_demo, BiscuitOSY";
                BiscuitOs-name = "BiscuitOS2";
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
递。获得驱动对应的节点之后，通过调用 for_each_node_with_property() 函数通过属
性名字找到所有节点，驱动编写如下：

{% highlight ruby %}
/*
 * DTS: for_each_node_with_property
 *
 * #define for_each_node_with_property(dn, prop_name) \
 *    for (dn = of_find_node_with_property(NULL, prop_name); dn; \
 *         dn = of_find_node_with_property(dn, prop_name))
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
 *                BiscuitOs-name = "BiscuitOS0";
 *                status = "okay";
 *        };
 *        DTS_demoX {
 *                compatible = "DTS_demo, BiscuitOSX";
 *                BiscuitOs-name = "BiscuitOS1";
 *                status = "disabled";
 *        };
 *        DTS_demoY {
 *                compatible = "DTS_demo, BiscuitOSY";
 *                BiscuitOs-name = "BiscuitOS2";
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

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *node;

    printk("DTS probe entence...\n");

    /* find device node via property name */
    for_each_node_with_property(node, "BiscuitOs-name")
        if (node)
            printk("Found %s\n", node->full_name);

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
[    3.565634] DTS demo probe entence...
[    3.565645] Found /DTS_demo
[    3.565649] Found /DTS_demoX
[    3.565656] Found /DTS_demoY
{% endhighlight %}

--------------------------------

# <span id="附录">附录</span>

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
