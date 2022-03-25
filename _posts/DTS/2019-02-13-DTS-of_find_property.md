---
layout: post
title:  "of_find_property"
date:   2019-02-13 08:20:30 +0800
categories: [HW]
excerpt: DTS API of_find_property().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_find_property](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_find_property)
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

##### of_find_property

{% highlight ruby %}
of_find_property
|
|---of_find_property
    |
    |---__of_find_property
       |
       |---of_prop_cmp
{% endhighlight %}

> 平台： ARM32
>
> linux： 3.10/4.18/5.0

函数作用：查找当前节点中的某个属性，属性有 name 参数指定，并返回属性的长度。

##### of_find_property

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
性长度。

函数通过调用 raw_spin_lock_irqsave() 函数加锁之后，就直接调用 
__of_find_property() 函数。__of_find_property() 函数用于查找并返回属性名字与 
name 一致的属性。调用完 __of_find_property() 函数之后，调用 
raw_spin_unlock_irqrestore() 函数解锁。最后返回 property 结构。

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

-----------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含多个属性，然后通过往 
of_find_property() 函数中传入指定名字，来获得属性的结构，函数定义如下：

{% highlight ruby %}
struct property *of_find_property(const struct device_node *np,
                  const char *name,
                  int *lenp);
{% endhighlight %}

这个函数经常用用于通过属性的名字查找当前节点中指定的属性。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点包含名为 BiscutisOS_int 
的属性，属性值为 0x10203040；节点又包含一个名为 BiscuitOS_name 的属性，属性值为 
"BiscuitOS"。具体内容如下：

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
                BiscuitOS_int = <0x10203040>;
                BiscuitOS_name = "BiscuitOS";
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
递。获得驱动对应的节点之后，通过调用 of_find_property() 函数获得 
“BiscuitOS_int”属性和 “BiscuitOS_name”属性，并打印属性值，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_find_property
 *
 * struct property *of_find_property(const struct device_node *np,
 *                         const char *name, int *lenp)
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
 *                BiscuitOS_int = <0x10203040>;
 *                BiscuitOS_name = "BiscuitOS";
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
#include <linux/of_fdt.h>

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct property *pp;
    int len;
    
    printk("DTS demo probe entence\n");

    /* find int property on current device-node */
    pp = of_find_property(np, "BiscuitOS_int", &len);
    printk("Property: %s\n", pp->name);
    printk("Value: %#x\n", of_read_number(pp->value, len / 4));
    printk("Length: %d\n", len);

    /* Find string property on current device-node */
    pp = of_find_property(np, "BiscuitOS_name", NULL);
    printk("Property: %s\n", pp->name);
    printk("Value: %s\n", pp->value);

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
[    3.553142] io scheduler cfg registered (default)\
[    3.553439] DTS demo probe entence
[    3.553443] Property: BiscuitOS_int
[    3.553449] Value: 0x10203040
[    3.553454] Length: 4
[    3.553459] Property: BiscuitOS_name
[    3.553464] Value: BiscuitOS
{% endhighlight %}

驱动中，获得 device_node 之后，节点的属性都存储在 device_node 的 properties 成
员维护的链表里，of_find_property() 函数就是遍历这个链表，从而找到指定的属性。

{% highlight ruby %}
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

-----------------------------------------

# <span id="附录">附录</span>
