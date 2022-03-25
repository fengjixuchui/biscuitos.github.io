---
layout: post
title:  "of_prop_next_u32"
date:   2019-02-24 10:37:30 +0800
categories: [HW]
excerpt: DTS API of_prop_next_u32().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_prop_next_u32](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_prop_next_u32)
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

##### of_prop_next_u32

{% highlight c %}
of_prop_next_u32
|
|---be32_to_cpup
{% endhighlight %}

函数作用：读取整形属性中多个整形值。

> 平台： ARM
>
> linux： 3.10/4.18/5.0

{% highlight c %}
const __be32 *of_prop_next_u32(struct property *prop, const __be32 *cur,
                   u32 *pu)
{
    const void *curv = cur;

    if (!prop)
        return NULL;

    if (!cur) {
        curv = prop->value;
        goto out_val;
    }

    curv += sizeof(*cur);
    if (curv >= prop->value + prop->length)
        return NULL;

out_val:
    *pu = be32_to_cpup(curv);
    return curv;
}
EXPORT_SYMBOL_GPL(of_prop_next_u32);
{% endhighlight %}

参数 prop 指向属性；cur 参数指向属性值地址；pu 参数用于存储属性值。

函数首先对 prop 和 cur 变量进行有效性检测，其中，如果 cur 为空的话，就讲 cur 
指向属性的值。接着将 curv 的值加上 u32 长度之后获得新的地址，并检查这个新地址
时候超出属性的长度。最后使用 be32_to_cpuc() 函数获得一个正确格式的值存储到 pu 
参数中。

-------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含 “BiscuitOS-name”属性，属
性包含多个 u32 值，然后通过of_prop_next_u32() 函数读取指定的值，函数定义如下：

{% highlight c %}
const __be32 *of_prop_next_u32(struct property *prop, const __be32 *cur,
                   u32 *pu)
{% endhighlight %}

这个函数经常用用于循环读出属性中的 u32 值。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，
在 root 节点之下创建一个名为 DTS_demo 的子节点，节点默认打开。具体内容如下：

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

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_find_property() 函数获得 
"BiscuitOS-name" 属性，然后通过 of_prop_next_u32() 函数循环读出属性中的值，
驱动编写如下：

{% highlight c %}
/*
 * DTS: of_prop_next_u32
 *
 * const __be32 *of_prop_next_u32(struct property *prop, const __be32 *cur,
 *                                                       u32 *pu)
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
 *                BiscuitOs-data = <0x10203040 0x50607080
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
    struct property *prop;
    const __be32 *lane = NULL;
    u32 data;
    int i;
  
    printk("DTS probe entence...\n");
   
    /* get property from device node */
    prop = of_find_property(np, "BiscuitOS-data", NULL);
    if (!prop)
        return -1;

    for (i = 0; i < 4; i++) {
        lane = of_prop_next_u32(prop, lane, &data);
        printk("Data[%d]: %#x\n", i, data);
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
[    3.565648] Data[0]: 0x10203040
[    3.565660] Data[1]: 0x50607080
[    3.565669] Data[2]: 0x90a0b0c0
[    3.565673] Dtat[3]: 0xd0e0f000
{% endhighlight %}

----------------------------------------------------

# <span id="附录">附录</span>
