---
layout: post
title:  "of_device_is_available"
date:   2019-01-21 17:56:30 +0800
categories: [HW]
excerpt: DTS API of_device_is_available().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_device_is_available](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_device_is_available)
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

##### of_device_is_available

{% highlight ruby %}
of_device_is_available
|
|---__of_device_is_available
    |
    |---__of_get_property
{% endhighlight %}

函数作用：读取当前节点的下一个节点。

> 平台： ARM32
>
> linux： 3.10/4.14

{% highlight ruby %}
/**
*  of_device_is_available - check if a device is available for use
*
*  @device: Node to check for availability
*
*  Returns 1 if the status property is absent or set to "okay" or "ok",
*  0 otherwise
*/
int of_device_is_available(const struct device_node *device)
{
    unsigned long flags;
    int res;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    res = __of_device_is_available(device);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return res;

}
EXPORT_SYMBOL(of_device_is_available);
{% endhighlight %}

参数 prev 指向当前一个节点。

函数首先调用 raw_spin_lock_irqsave() 函数进行加锁操作，然后调用 
__of_device_is_available() 函数获得节点是否可用的信息，最后返回可用信息之前调用
raw_spin_unlock_irqrestore() 函数解锁。

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

-------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建两个私有节点，第一个私有节点默认打开，通过调用 
of_device_is_available() 函数检查节点是否可用，函数定义如下：

{% highlight ruby %}
int of_device_is_available(const struct device_node *device)
{% endhighlight %}

这个函数经常用用于判断节点是否可用。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

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
递。获得驱动对应的节点之后，通过调用 of_device_is_available() 函数判断当前节点
是否可用。驱动编写如下：

{% highlight ruby %}
/*
 * DTS: of_device_is_avaiable
 *
 * int of_device_is_available(const struct device_node *device)
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
    struct device_node *npp;
    const char *comp;

    printk("DTS demo probe entence.\n");

    if (of_device_is_available(np)) {
        printk("%s device is available\n", np->name);
    }

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
[    3.534359] DTS_demo device is available
{% endhighlight %}

------------------------------------------------

# <span id="附录">附录</span>

