---
layout: post
title:  "of_read_ulong"
date:   2019-02-24 10:50:30 +0800
categories: [HW]
excerpt: DTS API of_read_ulong().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_read_ulong](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_read_ulong)
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

##### of_read_ulong

{% highlight c %}
of_read_ulong
|
|---of_read_number
    |
    |---be32_to_cpu
{% endhighlight %}

函数作用：读取一个属性的值，值长度为 32 为。

> 平台： ARM
>
> linux： 3.10/4.18/5.0

{% highlight c %}
/* Like of_read_number, but we want an unsigned long result */
static inline unsigned long of_read_ulong(const __be32 *cell, int size)
{
    /* toss away upper bits if unsigned long is smaller than u64 */
    return of_read_number(cell, size);
}
{% endhighlight %}

参数 cell 指向属性值的地址；size 指向属性值的第 n 个 ulong 值

函数直接调用 of_read_number() 函数获得指定的属性值。

##### of_read_number

{% highlight c %}
/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 of_read_number(const __be32 *cell, int size)
{
    u64 r = 0;
    while (size--)
        r = (r << 32) | be32_to_cpu(*(cell++));
    return r;
}
{% endhighlight %}

参数 cell 指向数据；size 指向数据 cells 数量。

函数使用 while 循环，每遍历依次，数据长度增加 32 位，并从中读取 32 位数据，最
后以为合并指定长度的数据。

------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含多个属性，然后通过往 
of_read_ulong() 函数获得当前节点整形属性的值，函数定义如下：

{% highlight c %}
static inline unsigned long of_read_ulong(const __be32 *cell, int size)
{% endhighlight %}

这个函数经常用用于读取属性整数值。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，
在 root 节点之下创建一个名为 DTS_demo 的子节点，节点默认打开。节点中包含一个名
为 “BiscuitOS-data” 的属性，属性值很长，具体内容如下：

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
                BiscuitOS-data = <0x10203040 0x50607080
                                  0x90a0b0c0 0xd0e0f000>;
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
传递。获得驱动对应的节点之后，通过调用 of_n_size_cells() 函数获得当前节点
的 #size-cells 属性值，接着调用 of_get_property() 获得 “BiscuitOS-data” 属性
值之后，调用 of_read_ulong() 函数读取属性值，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_read_ulong
 *
 * static inline unsigned long of_read_ulong(const __be32 *cell, int size)
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
 *                BiscuitOS-data = <0x10203040 0x50607080
 *                                  0x90a0b0c0 0xd0e0f000>;
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
    const __be32 *prop;
    u32 data32;

    printk("DTS probe entence...\n");

    /* get proerpty */
    prop = of_get_property(np, "BiscuitOS-data", NULL);

    /* Read 1st-ulong data */
    data32 = of_read_ulong(prop, 1);
    printk("BiscuitOs-data 1st u32 value: %#x\n", data32);

    /* Read 2nd-ulong data */
    data32 = of_read_ulong(prop, 2);
    printk("BiscuitOs-data 2nd u32 value: %#x\n", data32);

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
[    3.565648] BiscuitOS-data 1st u32 value: 0x10203040
[    3.565654] BiscuitOS-data 2nd u32 value: 0x50607080
{% endhighlight %}

------------------------------------

# <span id="附录">附录</span>
