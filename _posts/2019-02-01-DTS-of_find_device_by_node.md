---
layout: post
title:  "of_find_device_by_node"
date:   2019-02-01 14:55:30 +0800
categories: [HW]
excerpt: DTS API of_find_device_by_node().
tags:
  - DTS
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_find_device_by_node](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_find_device_by_node)
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

##### of_find_device_by_node

{% highlight ruby %}
of_find_device_by_node
|
|---bus_find_device
    |
    |---of_device_node_match
{% endhighlight %}

函数作用：获得当前节点对应的 platform_device。

> 平台： ARM64
>
> linux： 3.10/4.14

{% highlight ruby %}
/**
* of_find_device_by_node - Find the platform_device associated with a node
* @np: Pointer to device tree node
*
* Returns platform_device pointer, or NULL if not found
*/
struct platform_device *of_find_device_by_node(struct device_node *np)
{
    struct device *dev;

    dev = bus_find_device(&platform_bus_type, NULL, np, of_dev_node_match);
    return dev ? to_platform_device(dev) : NULL;
}
EXPORT_SYMBOL(of_find_device_by_node);
{% endhighlight %}

参数 np 指向一个 device node。

函数通过调用 bus_find_device() 函数遍历 platform_bus_type 总线上的左右设备，如
果设备的 match 函数满足 of_dev_node_match() 函数提出的条件，那么 device 就算找
到了。如果 device 找到就使用 to_platform_device() 函数将 device 转换成 
platform_device；否则返回 NULL。

##### of_dev_node_match

{% highlight ruby %}
static int of_dev_node_match(struct device *dev, void *data)
{
    return dev->of_node == data;
}
{% endhighlight %}

参数 dev 指向总线上的一个 device；data 指向 match 的数据。

函数规定，只要 device 的 of_node 成员和 data 一致，那么判定为 device 找到，返
回 1；否则判定为 device 未找到，返回 0.

函数通过调用 raw_spin_lock_irqsave() 函数加锁之后，就直接调用 
__of_find_property() 函数。__of_find_property() 函数用于查找并返回属性名字与 
name 一致的属性。调用完 __of_find_property() 函数之后，调用 
raw_spin_unlock_irqrestore() 函数解锁。最后返回 property 结构。

--------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过 
of_find_device_by_node() 函数找到节点对应的 platform device，函数定义如下：

{% highlight ruby %}
struct platform_device *of_find_device_by_node(struct device_node *np)
{% endhighlight %}

这个函数经常用用于在子函数中，通过节点找到对应的设备信息。

#### DTS 文件

由于使用的平台是 ARM64，所以在源码 /arch/arm64/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。具体内容如下：

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
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_get_property() 函数获得各个属性的值，
驱动编写如下：

{% highlight c %}
/*
 * DTS: of_find_device_by_node
 *
 * struct platform_device *of_find_device_by_node(struct device_node *np)
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
    struct platform_device *dev;

    printk("DTS demo probe entence.\n");

    dev = of_find_device_by_node(np);
    if (dev)
        printk("Platform device: %s\n", dev_name(&dev->dev));
  
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
make ARCH=arm64
make ARCH=arm64 dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight ruby %}
[    3.534323] DTS demo probe entence.
[    3.534359] Platform device: DTS_demo.25
{% endhighlight %}

---------------------------------------------

# <span id="附录">附录</span>


