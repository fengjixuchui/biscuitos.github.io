---
layout: post
title:  "of_machine_is_compatible"
date:   2019-02-22 14:43:30 +0800
categories: [HW]
excerpt: DTS API of_machine_is_compatible().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_machine_is_compatible](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_machine_is_compatible)
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

##### of_machine_is_compatible

{% highlight c %}
of_machine_is_compatible
|
|---of_device_is_compatible
    |
    |---bus_find_device
        |
        |---of_device_node_match
{% endhighlight %}

函数作用：检查 root 节点的 compatible 是否为指定的值。

> 平台： ARM64
>
> linux： 3.10/4.14/5.0

{% highlight c %}
/**
* of_machine_is_compatible - Test root of device tree for a given compatible value
* @compat: compatible string to look for in root node's compatible property.
*
* Returns true if the root node has the given value in its
* compatible property.
*/
int of_machine_is_compatible(const char *compat)
{
    struct device_node *root;
    int rc = 0;

    root = of_find_node_by_path("/");
    if (root) {
        rc = of_device_is_compatible(root, compat);
        of_node_put(root);
    }
    return rc;
}
EXPORT_SYMBOL(of_machine_is_compatible);
{% endhighlight %}

参数 compat 指向一个字符串，用户匹配 root 节点的 compatible 属性。

函数首先调用 of_find_node_by_path() 函数获得 root 节点，如果 root 节点存在，
那么继续调用 of_device_is_compatible() 函数对比 root 节点 compatible 属性值
是否和 compat 参数一致，如果一致就返回 ture。

##### of_device_is_compatible

{% highlight c %}
/** Checks if the given "compat" string matches one of the strings in
* the device's "compatible" property
*/
int of_device_is_compatible(const struct device_node *device,
        const char *compat)
{
    unsigned long flags;
    int res;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    res = __of_device_is_compatible(device, compat);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return res;
}
EXPORT_SYMBOL(of_device_is_compatible);
{% endhighlight %}

参数 device 指向一个 device node; 参数指向 compatible 字符串。

函数通过调用 __of_device_is_compatible() 函数对应 device_node 的 compatible 
成员是否和参数 compat 一致。如果一致就返回 1；否则返回 0.

##### __of_device_is_compatible

{% highlight c %}
/** Checks if the given "compat" string matches one of the strings in
* the device's "compatible" property
*/
static int __of_device_is_compatible(const struct device_node *device,
                     const char *compat)
{
    const char* cp;
    int cplen, l;

    cp = __of_get_property(device, "compatible", &cplen);
    if (cp == NULL)
        return 0;
    while (cplen > 0) {
        if (of_compat_cmp(cp, compat, strlen(compat)) == 0)
            return 1;
        l = strlen(cp) + 1;
        cp += l;
        cplen -= l;
    }

    return 0;
}
#define of_compat_cmp(s1, s2, l)    strncmp((s1), (s2), (l))
{% endhighlight %}

参数 device 指向一个 device node; 参数指向 compatible 字符串。

函数调用 __of_get_property() 函数获得节点的 “compatible”属性之后，调用 
of_compat_cmp() 函数对比 compatible 属性的值是否和参数一致，如果一致，那么

##### __of_get_property

{% highlight c %}
/*
* Find a property with a given name for a given node
* and return the value.
*/
static const void *__of_get_property(const struct device_node *np,
                     const char *name, int *lenp)
{
    struct property *pp = __of_find_property(np, name, lenp);

    return pp ? pp->value : NULL;
}
{% endhighlight %}

参数 np 指向 device node 节点；参数 name 指向要查找属性的名字；参数 lenp 存
储属性的长度。

函数通过调用 __of_find_property() 函数获得属性的结构、如果属性找到，那么返回
属性的值；否则返回 NULL。

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
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到的
属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不为 
NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

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

函数首先定义了局部变量 np，并指向 of_allnodes 全局 device node 链表的头部，接
着调用 raw_spin_lock_irqsave() 函数加锁，并遍历所有的节点，每一次的遍历到的节
点，如果全路径 full_name 存在，并且与参数 path 一致，那么就找到指定的 device 
node。找到之后调用 raw_spin_unlock_irqrestore() 解锁。

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

------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过 
of_find_device_by_node() 函数找到节点对应的 platform device，函数定义如下：

{% highlight c %}
struct platform_device *of_find_device_by_node(struct device_node *np)
{% endhighlight %}

这个函数经常用用于在子函数中，通过节点找到对应的设备信息。

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
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员
传递。获得驱动对应的节点之后，通过调用 of_machine_is_compatible() 函数检查 DTB 
根节点的 compatible 属性是否为指定的值，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_machine_is_compatible
 *
 * int of_machine_is_compatible(const char *compat)
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
#include <linux/module.h>

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;

    printk("DTS probe entence...\n");

    /* Check root of device tree for a given compatible value */
    if (of_machine_is_compatible("arm,BiscuitOS"))
        printk("Machine is BiscuitOS\n");

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

{% highlight c %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight c %}
[    3.534323] DTS demo probe entence...
[    3.534359] Machine is BiscuitOS
{% endhighlight %}

-----------------------------------------

# <span id="附录">附录</span>


