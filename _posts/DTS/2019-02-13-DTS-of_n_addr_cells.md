---
layout: post
title:  "of_n_addr_cells"
date:   2019-02-22 15:19:30 +0800
categories: [HW]
excerpt: DTS API of_n_addr_cells().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_n_addr_cells](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_n_addr_cells)
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

##### of_n_addr_cells

{% highlight c %}
of_n_addr_cells
|
|---of_get_property
    |
    |---of_find_property
        |
        |---of_find_property
            |
            |---__of_find_property
               |
               |---of_prop_cmp
{% endhighlight %}

函数作用：获得节点 #address-cells 属性的值，其值表示节点中，地址的长度。
当 #address-cells 为 1 时，表示节点中的地址长度为 32 位；当 #address-cells 
为 2 时，表示节点中的地址长度为 64 位。

> 平台： ARM64
>
> linux： 3.10/4.18/5.0

{% highlight c %}
int of_n_addr_cells(struct device_node *np)
{
    const __be32 *ip;

    do {
        if (np->parent)
            np = np->parent;
        ip = of_get_property(np, "#address-cells", NULL);
        if (ip)
            return be32_to_cpup(ip);
    } while (np->parent);
    /* No #address-cells property for the root node */
    return OF_ROOT_NODE_ADDR_CELLS_DEFAULT;
}
EXPORT_SYMBOL(of_n_addr_cells);
{% endhighlight %}

参数 np 指向 device_node。

函数首先确认节点是否包含父节点，如果包含，则局部变量指向父节点。然后调用 
of_get_proerpty() 函数获得 np 节点的 #address-cells 属性，如果属性存在，则调
用 be32_to_cpup() 函数返回属性值；如果属性不存在，在使用 while 循环查找 np 对
应的父节点，直到找到 #address-cells 属性 为止。

##### of_get_property

{% highlight c %}
/*
* Find a property with a given name for a given node
* and return the value.
*/
const void *of_get_property(const struct device_node *np, const char *name,
                int *lenp)
{
    struct property *pp = of_find_property(np, name, lenp);

    return pp ? pp->value : NULL;
}
EXPORT_SYMBOL(of_get_property);
{% endhighlight %}

参数 np 指向一个 device node；name 参数为需要查找属性的名字；lenp 参数用于存
储属性的长度。

函数通过调用 of_find_property() 函数获得属性的结构 property，如果获得成功，则
返回属性的值；如果失败则返回 NULL。

#### of_find_property

{% highlight c %}
struct property *of_find_property(const struct device_node *np,
                  const char *name,
                  int *lenp)
{
    struct property *pp;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    pp = __of_find_property(np, name, lenp);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);

    return pp;
}
EXPORT_SYMBOL(of_find_property);
{% endhighlight %}

参数 np 指向一个 device_node；name 参数表示需要查看属性的名字；lenp 用于存储
属性长度。

函数通过调用 raw_spin_lock_irqsave() 函数加锁之后，就直接调用 
__of_find_property() 函数。__of_find_property() 函数用于查找并返回属性名字与 
name 一致的属性。调用完 __of_find_property() 函数之后，调用 
raw_spin_unlock_irqrestore() 函数解锁。最后返回 property 结构。

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

由于每个 device_node 包含一个名为 properties 的成员，properties 是一个单链表的
表头，这个单链表包含了该节点的所有属性，函数通过使用 for 循环遍历这个链表，以
此遍历节点所包含的所有属性。每遍历一个属性就会调用 of_prop_cmp() 函数，
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到的
属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不为 
NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

----------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含多个属性，然后通过往 
of_n_addr_cells() 函数获得当前节点 #address-cells 的值，函数定义如下：

{% highlight c %}
int of_n_addr_cells(struct device_node *np)
{% endhighlight %}

这个函数经常用用于计算节点或子节点地址长度。

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
 * This program is free software; you can redistribute it and/or m
 * it under the terms of the GNU General Public License version 2
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
递。获得驱动对应的节点之后，通过调用 of_n_addr_cells() 函数获得当前节点
的 #address-cells 属性值，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_n_addr_cells
 *      of_n_size_cells
 *
 * int of_n_addr_cells(struct device_node *np)
 *
 * int of_n_size_cells(struct device_node *np)
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
    int cells;

    printk("DTS probe entence...\n");

    /* Get #address-cells number */
    cells = of_n_addr_cells(np);
    printk("%s #address-cells: %#x\n", np->name, cells);

    /* Get #size-cells number */
    cells = of_n_size_cells(np);
    printk("%s #size-cells:    %#x\n", np->name, cells);

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
[    3.565634] DTS demo probe entence
[    3.565648] DTS_demo #address-cells: 0x2
[    3.565660] DTS_demo #size-cells:    0x2
{% endhighlight %}

------------------------------------------------

# <span id="附录">附录</span>




