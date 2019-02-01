---
layout: post
title:  "for_each_available_child_of_node"
date:   2019-01-19 08:39:30 +0800
categories: [HW]
excerpt: DTS API for_each_available_child_of_node().
tags:
  - DTS
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/DEV000106.jpg)

> Github: [for_each_available_child_of_node](https://github.com/BiscuitOS/HardStack/tree/master/bus/DTS/kernel/API/for_each_available_child_of_node)
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

##### for_each_available_child_of_node

{% highlight ruby %}
for_each_available_child_of_node
|
|---of_get_next_available_child
    |
    |---raw_spin_lock_irqsave  
    |
    |---raw_spin_unlock_irqrestore
{% endhighlight %}

函数作用：用于遍历节点里的所有可用子节点（status 属性值为 “okay”）。

> 平台： ARM64
>
> linux： 3.10/4.14/5.0

{% highlight ruby %}
#define for_each_available_child_of_node(parent, child) \
    for (child = of_get_next_available_child(parent, NULL); child != NULL; \
         child = of_get_next_available_child(parent, child))
{% endhighlight %}

参数 parent 指向当前节点；child 指向子节点

函数在 for 循环中，调用 of_get_next_available_child(parent，NULL) 函数 parent 
的第一个可用的子节点，只要 child 不为 NULL，循环继续运行。函数继续调用 
of_get_next_available_child(parent, child) 函数获得 child 子节点的下一个可用
子节点，以此达到循环的效果。

##### of_get_next_available_child

{% highlight ruby %}
/**
*    of_get_next_available_child - Find the next available child node
*    @node:    parent node
*    @prev:    previous child of the parent node, or NULL to get first
*
*      This function is like of_get_next_child(), except that it
*      automatically skips any disabled nodes (i.e. status = "disabled").
*/
struct device_node *of_get_next_available_child(const struct device_node *node,
    struct device_node *prev)
{
    struct device_node *next;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    next = prev ? prev->sibling : node->child;
    for (; next; next = next->sibling) {
        if (!__of_device_is_available(next))
            continue;
        if (of_node_get(next))
            break;
    }
    of_node_put(prev);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return next;
}
EXPORT_SYMBOL(of_get_next_available_child);
{% endhighlight %}

参数 node 指向设备节点。prev 参数为前一个子节点

函数首先调用 raw_spin_lock_irqsave() 函数加锁，然后检查 prev 是否存在，如果存
在，则 next 变量指向 prev 节点的 sibling 变量；如果 prev 不存在，那么 next 指
向节点的 child。接着从节点的 sibling 链表中获得子节点，并调用 
__of_device_is_available() 函数检查节点是否可用，然后调用 of_node_get() 获得
节点的父节点，最后返回之前调用 raw_spin_unlock_irqrestore() 函数解锁。返回子
节点。

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
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到
的属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不
为 NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

-----------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过 
for_each_available_child_of_node() 函数遍历节点的所有可用子节点，并打印子节
点的名字，函数定义如下：

{% highlight ruby %}
#define for_each_available_child_of_node(parent, child) \
    for (child = of_get_next_available_child(parent, NULL); child != NULL; \
         child = of_get_next_available_child(parent, child))
{% endhighlight %}

这个函数经常用用于遍历当前节点的所有可用子节点。

#### DTS 文件

由于使用的平台是 ARM64，所以在源码 /arch/arm64/boot/dts 目录下创建一个 DTSI 文
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

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者找到系统默认的 DTS 文
件，比如当前编译使用的 DTS 文件为 XXX.dtsi，然后在 XXX.dtsi 文件开始地方添加如
下内容：

{% highlight ruby %}
#include "DTS_demo.dtsi"
{% endhighlight %}

#### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员
传递。获得驱动对应的节点之后，通过调用 for_each_available_child_of_node() 函数
遍历节点的所有可用子节点，并打印子节点的名字，驱动编写如下：

{% highlight ruby %}
/*
 * DTS: of_get_child_count
 *      for_each_available_child_of_node
 *      for_each_available_of_node
 *
 * static inline int of_get_child_count(const struct device_node *np)
 *
 * #define for_each_available_child_of_node(parent, child) \
 *         for (child = of_get_next_available_child(parent, NULL); child != NULL; \
 *              child = of_get_next_available_child(parent, child))
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
    for_each_available_child_of_node(np, child)
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
make ARCH=arm64
make ARCH=arm64 dtbs
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

---------------------------------

# <span id="附录">附录</span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)



