---
layout: post
title:  "of_find_compatible_node"
date:   2019-02-01 14:43:30 +0800
categories: [HW]
excerpt: DTS API of_find_compatible_node().
tags:
  - DTS
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_find_compatible_node](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_find_compatible_node)
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

##### of_find_compatible_node

{% highlight ruby %}
of_find_compatible_node
|
|---__of_device_is_available
    |
    |---__of_get_property
{% endhighlight %}

函数作用：通过 compatible 属性找到对应的 device node。

> 平台： ARM64
>
> linux： 3.10/4.14

{% highlight ruby %}
/**
*    of_find_compatible_node - Find a node based on type and one of the
*                                tokens in its "compatible" property
*    @from:        The node to start searching from or NULL, the node
*            you pass will not be searched, only the next one
*            will; typically, you pass what the previous call
*            returned. of_node_put() will be called on it
*    @type:        The type string to match "device_type" or NULL to ignore
*    @compatible:    The string to match to one of the tokens in the device
*            "compatible" list.
*
*    Returns a node pointer with refcount incremented, use
*    of_node_put() on it when done.
*/
struct device_node *of_find_compatible_node(struct device_node *from,
    const char *type, const char *compatible)
{
    struct device_node *np;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    np = from ? from->allnext : of_allnodes;
    for (; np; np = np->allnext) {
        if (type
            && !(np->type && (of_node_cmp(np->type, type) == 0)))
            continue;
        if (__of_device_is_compatible(np, compatible) &&
            of_node_get(np))
            break;
    }
    of_node_put(from);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return np;
}
EXPORT_SYMBOL(of_find_compatible_node);
{% endhighlight %}

参数 from 指向开始的 device node； type 指向属性的类型；compatible 指向 
compatible 属性的名字

函数首先调用 raw_spin_lock_irqsave() 函数进行加锁操作，然后判断 from 参数是否
存在，如果不存在，则将 np 指向 of_allnodes 根节点；如果 from 存在，则将 np 指
向 from 的 allnext 成员指向的节点。然后调用 for 循环遍历节点，每遍历一个节点都
会对 type 的 compatible 值进行对比，其中嗲用 __of_device_is_compatible() 函数
检查节点的 compatible 属性是否符合要求。如果符合要求就返回节点；否则返回 NULL。

##### __of_device_is_available

{% highlight ruby %}
/**
*  __of_device_is_available - check if a device is available for use
*
*  @device: Node to check for availability, with locks already held
*
*  Returns 1 if the status property is absent or set to "okay" or "ok",
*  0 otherwise
*/
static int __of_device_is_available(const struct device_node *device)
{
    const char *status;
    int statlen;

    status = __of_get_property(device, "status", &statlen);
    if (status == NULL)
        return 1;

    if (statlen > 0) {
        if (!strcmp(status, "okay") || !strcmp(status, "ok"))
            return 1;
    }

    return 0;
}
{% endhighlight %}

参数 device 指向当前节点

函数首先调用 __of_get_property() 函数获得节点的 “status”属性值。如果属性存在，
并且属性值是 “okay”，那么返回 1；如果属性不存在，也返回 1；其余情况返回 0.

##### __of_get_property

{% highlight ruby %}
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

参数 np 指向设备节点；name 指向属性值；lenp 用于存储属性长度

函数通过调用 __of_find_property() 函数获得属性的值，如果属性值存在，则返回属性
的值；否则返回 NULL。

##### __of_find_property

{% highlight ruby %}
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

由于每个 device_node 包含一个名为 properties 的成员，properties 是一个单链表的
表头，这个单链表包含了该节点的所有属性，函数通过使用 for 循环遍历这个链表，以
此遍历节点所包含的所有属性。每遍历一个属性就会调用 of_prop_cmp() 函数，
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到的
属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不为 
NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

---------------------------------------------

#<span id="实践">实践</span>

实践目的是在 DTS 文件中构建两个私有节点，第一个私有节点默认打开，通过调用 
of_find_compatible_node() 函数找到 compatible 属性对应的节点，定义如下：

{% highlight ruby %}
    struct device_node *of_find_compatible_node(struct device_node *from,
        const char *type, const char *compatible)
{% endhighlight %}

这个函数经常用用于通过 compatible 属性找到对应的节点。

#### DTS 文件

由于使用的平台是 ARM64，所以在源码 /arch/arm64/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开，具体内容如下：

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

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者找到系统默认的 DTS 文
件，比如当前编译使用的 DTS 文件为 XXX.dtsi，然后在 XXX.dtsi 文件开始地方添加
如下内容：

{% highlight ruby %}
#include "DTS_demo.dtsi"
{% endhighlight %}

#### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_find_compatible_node() 函数找到 
compatible 属性对应的节点。驱动编写如下：

{% highlight c %}
/*
 * DTS: of_find_compatible_node
 *
 * struct device_node *of_find_compatible_node(struct device_node *from,
 *         const char *type, const char *compatible)
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
    struct device_node *node;

    printk("DTS probe entence...\n");

    /* find device node via compatible property */
    node = of_find_compatible_node(NULL, NULL, "DTS_demo, BiscuitOS");
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
make ARCH=arm64
make ARCH=arm64 dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight ruby %}
[    3.534323] DTS probe entence...
[    3.534359] Found /DTS_demo
{% endhighlight %}

-------------------------------------------

#<span id="附录">附录</span>
