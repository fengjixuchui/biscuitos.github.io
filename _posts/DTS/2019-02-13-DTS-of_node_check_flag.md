---
layout: post
title:  "of_node_check_flag"
date:   2019-02-22 15:30:30 +0800
categories: [HW]
excerpt: DTS API of_node_check_flag().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_node_check_flag](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_node_check_flag)
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

##### of_node_check_flag

{% highlight c %}
of_node_check_flag
|
|---test_bit
{% endhighlight %}

函数作用：检查节点的某个标志位是否置位。

> 平台： ARM64
>
> linux： 3.10/4.18/5.0

{% highlight c %}
static inline int of_node_check_flag(struct device_node *n, unsigned long flag)
{
    return test_bit(flag, &n->_flags);
}
{% endhighlight %}

参数 n 指向节点；参数 flag 指向需要检查的标志

函数通过调用 test_bit 参数节点的 _flags 是否置位。

-------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，然后通过调用 
of_node_check_flag() 函数检查节点的标志位是否置位，函数定义如下：

{% highlight c %}
static inline int of_node_check_flag(struct device_node *n, unsigned long flag)
{% endhighlight %}

这个函数经常用用于检查节点的某标志位是否置位。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，在 root 节点之下创建一个名为 DTS_demo 的子节点，节点默认打开。具体内容如下：

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

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员
传递。获得驱动对应的节点之后，通过调用 of_node_check_flag() 函数节点的标志位是
否置位，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_node_check_flag
 *
 * static inline int of_node_check_flag(struct device_node *n,
 *                                                 unsigned long flag)
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

    /* Mark device node flags */
    of_node_set_flag(np, OF_DETACHED);
    of_node_set_flag(np, OF_DYNAMIC);

    if (of_node_check_flag(np, OF_DETACHED))
        printk("%s has detached.\n", np->name);

    if (of_node_check_flag(np, OF_DYNAMIC)) {
        printk("%s is dynamic\n", np->name);
        /* Add reference count for device node */
        of_node_get(np);
        /* Declare reference count for device node */
        of_node_put(np);
    }

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
[    3.565645] DTS_demo has detached.
[    3.565649] DTS_demo is dynamic
{% endhighlight %}

--------------------------------------------------

# <span id="附录">附录</span>
