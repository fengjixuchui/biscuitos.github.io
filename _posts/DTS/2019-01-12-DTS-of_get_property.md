---
layout: post
title:  "of_get_property"
date:   2019-01-12 18:28:30 +0800
categories: [HW]
excerpt: DTS API of_get_property().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_get_property](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_get_property)
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

#### of_get_property

{% highlight ruby %}
of_get_property
|
|---of_find_property
    |
    |---of_find_property
        |
        |---__of_find_property
           |
           |---of_prop_cmp
{% endhighlight %}

函数作用：获得当前节点的属性值。

> 平台： ARM32
>
> linux： 3.10/4.18/5.0

{% highlight ruby %}
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
储属性的长度。函数通过调用 of_find_property() 函数获得属性的结构 property，
如果获得成功，则返回属性的值；如果失败则返回 NULL。

#### of_find_property

{% highlight ruby %}
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

参数 np 指向一个 device_node；name 参数表示需要查看属性的名字；lenp 用于存储属
性长度。函数通过调用 raw_spin_lock_irqsave() 函数加锁之后，就直接调
用 __of_find_property() 函数。__of_find_property() 函数用于查找并返回属性名字
与 name 一致的属性。调用完 __of_find_property() 函数之后，调
用 raw_spin_unlock_irqrestore() 函数解锁。最后返回 property 结构。

#### __of_find_property

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

由于每个 device_node 包含一个名为 properties 的成员，properties 是一个单链表
的表头，这个单链表包含了该节点的所有属性，函数通过使用 for 循环遍历这个链表，
以此遍历节点所包含的所有属性。每遍历一个属性就会调用 of_prop_cmp() 函数，
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到
的属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不
为 NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

----------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含多个属性，然后通过往 
of_get_property() 函数分别获得属性的值，函数定义如下：

{% highlight ruby %}
const void *of_get_property(const struct device_node *np, const char *name,
                int *lenp)
{% endhighlight %}

这个函数经常用用于通过属性的名字查找当前节点中指定属性的值。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 
文件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点包含名为 
BiscutisOS_name 的属性，属性值为 “BiscuitOS”；节点又包含一个名为 
BiscuitOS_int 的属性，属性值为 0x10203040；节点还包含一个名为 
BiscuitOS_mult 的属性，其值由四个 32 位值组成，分别为 0x10203040 0x50607080 
0x90a0b0c0 0xd0e0f000；最后节点还包含一个不含属性值的属性，其属性名为 
"BiscuitOS_leg"。具体内容如下：

{% highlight ruby %}
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
                BiscuitOS_name = "BiscuitOS";
                BiscuitOS_int  = <0x10203040>;
                BiscuitOS_mult = <0x10203040 0x50607080
                                  0x90a0b0c0 0xd0e0f000>;
                BiscuitOS_leg;
        };

};
{% endhighlight %}

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者在 Linux 4.20.8 的源码中，
找到 arch/arm/boot/dts/vexpress-v2p-ca9.dts 文件，然后在文件开始地方添
加如下内容：

{% highlight ruby %}
#include "DTS_demo.dtsi"
{% endhighlight %}

### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备
驱动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动
的 probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 
成员传递。获得驱动对应的节点之后，通过调用 of_get_property() 函数获得各个属
性的值，驱动编写如下：

{% highlight ruby %}
/*
 * DTS: of_get_property
 *
 * const void *of_get_property(const struct device_node *np, const char *name)
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
 *                BiscuitOS_name = "BiscuitOS";
 *                BiscuitOS_int  = <0x10203040>;
 *                BiscuitOS_mult = <0x10203040 0x50607080
 *                                  0x90a0b0c0 0xd0e0f000>;
 *                BiscuitOS_leg;
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
    const __be32 *prop;
    int len;
    
    printk("DTS demo probe entence\n");

    /* Obtain string property from device node */
    prop = of_get_property(np, "BiscuitOS_name", &len);
    if (prop) {
        printk("Property: BiscuitOS_name\n");
        printk("String: %s\n", prop);
    }

    /* Obtain int property from device node */
    prop = of_get_property(np, "BiscuitOS_int", &len);
    if (prop) {
        int value;

        value = be32_to_cpup(prop);
        printk("Property: BiscuitOS_int\n");
        printk("Value: %#x\n", value);
    }

    /* Obtain multi-int property from device node */
    prop = of_get_property(np, "BiscuitOS_mult", NULL);
    if (prop) {
        unsigned long value;

        printk("Property: BiscuitOS_mult\n");
        /* #cells 0 value */
        value = of_read_ulong(prop, of_n_addr_cells(np));
        printk("#cell 0: %#llx\n", value);

        /* #cells 1 value */
        prop += of_n_addr_cells(np);
        value = of_read_ulong(prop, of_n_addr_cells(np));
        printk("#cell 1: %#llx\n", value);
    }

    /* Obtain empty-value property on device node */
    prop = of_get_property(np, "BiscuitOS_leg", NULL);
    if (prop)
        printk("BiscuitOS_leg property exist.\n");
    else
        printk("BiscuitOS_leg property doesn't exist.\n");
   
  
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
[    3.565634] DTS demo probe entence
[    3.565648] Property: BiscuitOS_name
[    3.565660] String: BiscuitOS
[    3.565671] Property: BiscuitOS_int
[    3.565683] Value: 0x10203040
[    3.565693] Property: BiscuitOS_mult
[    3.565707] #cell 0: 0x1020304050607080
[    3.565719] #cell 1: 0x90a0b0c0d0e0f000
[    3.565732] BiscuitOS_leg property exist.
{% endhighlight %}

驱动中，通过 of_get_property() 获得属性的之后，需要进行不同的处理才能获得正
确的属性值，这里总结属性值的处理方法。

#### String 属性值

通过 of_get_property() 函数获得属性值后，这个值就是一个字符串，内核可以直接
使用这个字符串。

{% highlight ruby %}
/* Obtain string property from device node */
prop = of_get_property(np, "BiscuitOS_name", &len);
if (prop) {
    printk("Property: BiscuitOS_name\n");
    printk("String: %s\n", prop);
}
{% endhighlight %}

#### 单整形属性值

通过 of_get_property() 函数获得属性值后，需要使用 be32_to_cpup() 函数获得一个
小端的整形数据

{% highlight ruby %}
/* Obtain int property from device node */
prop = of_get_property(np, "BiscuitOS_int", &len);
if (prop) {
    int value;

    value = be32_to_cpup(prop);
    printk("Property: BiscuitOS_int\n");
    printk("Value: %#x\n", value);
}
{% endhighlight %}

#### 多个整形属性值

通过 of_get_property() 函数获得属性值之后，需要使用 of_read_ulong() 函数和 
of_n_addr_cells() 函数获得其中一个整形值。然后通过在 of_get_property() 返回
值的基础上，增加 of_n_addr_cells() 之后，在调用 of_read_ulong() 函数和 
of_n_addr_cells() 函数获得下一个整形值。

{% highlight ruby %}
/* Obtain multi-int property from device node */
prop = of_get_property(np, "BiscuitOS_mult", NULL);
if (prop) {
    unsigned long value;

    printk("Property: BiscuitOS_mult\n");
    /* #cells 0 value */
    value = of_read_ulong(prop, of_n_addr_cells(np));
    printk("#cell 0: %#llx\n", value);

    /* #cells 1 value */
    prop += of_n_addr_cells(np);
    value = of_read_ulong(prop, of_n_addr_cells(np));
    printk("#cell 1: %#llx\n", value);
}
{% endhighlight %}

#### 空属性值

有的属性不包含属性值，但需要判断节点是否包含这个属性，所以使用 
of_get_property() 函数的返回值判断该属性是否存在，如果存在返回非 NULL 值；如果
不存在，则返回 NULL 值。

{% highlight ruby %}
/* Obtain empty-value property on device node */
prop = of_get_property(np, "BiscuitOS_leg", NULL);
if (prop)
    printk("BiscuitOS_leg property exist.\n");
else
    printk("BiscuitOS_leg property doesn't exist.\n");
{% endhighlight %}

------------------------------------

# <span id="附录">附录</span>

