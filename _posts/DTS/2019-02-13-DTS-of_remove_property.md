---
layout: post
title:  "of_remove_property"
date:   2019-02-24 10:57:30 +0800
categories: [HW]
excerpt: DTS API of_remove_property().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_remove_property](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_remove_property)
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

##### of_remove_property

{% highlight c %}
of_remove_property
|
|---of_property_notify
|
|---proc_device_tree_remove_prop
{% endhighlight %}

> 平台： ARM
>
> linux： 3.10/4.18
>
> 函数作用：将一个属性从当前节点中移除。

##### of_remove_property

{% highlight c %}
/**
* of_remove_property - Remove a property from a node.
*
* Note that we don't actually remove it, since we have given out
* who-knows-how-many pointers to the data using get-property.
* Instead we just move the property to the "dead properties"
* list, so it won't be found any more.
*/
int of_remove_property(struct device_node *np, struct property *prop)
{
    struct property **next;
    unsigned long flags;
    int found = 0;
    int rc;

    rc = of_property_notify(OF_RECONFIG_REMOVE_PROPERTY, np, prop);
    if (rc)
        return rc;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    next = &np->properties;
    while (*next) {
        if (*next == prop) {
            /* found the node */
            *next = prop->next;
            prop->next = np->deadprops;
            np->deadprops = prop;
            found = 1;
            break;
        }
        next = &(*next)->next;
    }
    raw_spin_unlock_irqrestore(&devtree_lock, flags);

    if (!found)
        return -ENODEV;

#ifdef CONFIG_PROC_DEVICETREE
    /* try to remove the proc node as well */
    if (np->pde)
        proc_device_tree_remove_prop(np->pde, prop);
#endif /* CONFIG_PROC_DEVICETREE */

    return 0;
}
{% endhighlight %}

参数 np 指向当前 device_node; prop 指向新的 prop

函数首先调用 of_property_notify() 函数，这里不做过多讲解。然后获得节点的 
properties 属性链表，分别遍历链表中的所有属性，如果遇到属性的名字和节点已有的
属性名字一样，那么就将属性从 properties 链表中移除；如果没有找到指定的属性，那
么移除失败，直接返回 ENODEV。节点的属性都存储在 device_node 的 properties 成员
维护的链表里，of_find_property() 函数就是遍历这个链表，从而找到指定的属性。

{% highlight c %}
Relation: device_node and property
                                                                          +----------+
                                                                          |          |
                                                 +----------+             | property |
                                                 |          |             |    next -|-----------> NULL   
                        +----------+             | property |             |          |
                        |          |             |    next -|------------>+----------+
+-------------+         | property |             |          |
|             |         |    next -|------------>+----------+
| device_node |         |          |
| properties -|-------->+----------+
|             |
+-------------+
{% endhighlight %}

---------------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，然后通过of_remove_property() 函数从
节点里移除一个属性，函数定义如下：

{% highlight c %}
int of_remove_property(struct device_node *np, struct property *prop)
{% endhighlight %}

这个函数经常用用于从节点里移除指定属性。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，在 root 节点之下创建一个名为 DTS_demo 的子节点。具体内容如下：

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
                BiscuitOS-u32 = <0x50607080>;
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
递。获得驱动对应的节点之后，通过调用 of_property_read_u32() 函数从节点的属性 
“BiscuitOS-u32” 中读取原始值，然后调用 of_remove_property() 函数移除 
“BiscuitOS-u32”属性，并通过 of_find_property() 再当前节点中查找 
“BiscuitOS-u32” 属性。如果没有找到，接着调用 of_add_property() 函数添加 
"BiscuitOS-u32" 属性，并读取和打印属性的值，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_remove_property
 *
 * int of_remove_property(struct device_node *np, struct property *prop)
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
 *                BiscuitOS-u32 = <0x50607080>;
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

static u32 u32_data = 0x10203040;

static struct property BiscuitOS_u32 = {
    .name   = "BiscuitOS-u32",
    .length = sizeof(u32),
    .value  = &u32_data,
};

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct property *prop;
    u32 u32_data;
    int rc;

    printk("DTS demo probe entence.\n");

    /* Read legacy property from DT */
    rc = of_property_read_u32(np, "BiscuitOS-u32", &u32_data);
    if (rc == 0)
        printk("Legacy BiscuitOS-u32: %#x\n", be32_to_cpu(u32_data));

    /* Remove speical property from DT */
    prop = of_find_property(np, "BiscuitOS-u32", NULL);
    if (prop)
        of_remove_property(np, prop);

    /* Add a u32 data property into device node */
    of_add_property(np, &BiscuitOS_u32);
    /* Read it from DT */
    rc = of_property_read_u32(np, "BiscuitOS-u32", &u32_data);
    if (rc == 0)
        printk("New BiscuitOS-u32:    %#x\n", be32_to_cpu(u32_data));

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
[    3.587479] DTS demo probe entence.
[    3.587496] Legacy BiscuitOS-u32: 0x50607080
[    3.587508] New BiscuitOS-u32:    0x10203040
{% endhighlight %}

------------------------------------------------

# <span id="附录">附录</span>
