---
layout: post
title:  "of_property_read_u16"
date:   2019-02-24 09:26:30 +0800
categories: [HW]
excerpt: DTS API of_property_read_u16().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_property_read_u16](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_property_read_u16)
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

##### of_property_read_u16

{% highlight c %}
of_property_read_u16
|
|---of_proeprty_read_u16_array
    |
    |---of_find_property_value_of_size
        |
        |---of_find_property
            |
            |---__of_find_property
{% endhighlight %}

函数作用：读取节点中的 16 位整数的值。

> 平台： ARM
>
> linux： 3.10/4.14

{% highlight c %}
static inline int of_property_read_u16(const struct device_node *np,
                       const char *propname,
                       u16 *out_value)
{
    return of_property_read_u16_array(np, propname, out_value, 1);
}
{% endhighlight %}

参数 np 指向当前设备节点；参数 propname 指向 u16 数据所在的属性名字；参数 
out_value 用于存储 u16 数据。

函数直接调用 of_property_read_u16_array() 函数读取 u16 数据。

##### of_property_read_u16_array

{% highlight c %}
/**
* of_property_read_u16_array - Find and read an array of u16 from a property.
*
* @np:        device node from which the property value is to be read.
* @propname:    name of the property to be searched.
* @out_value:    pointer to return value, modified only if return value is 0.
* @sz:        number of array elements to read
*
* Search for a property in a device node and read 16-bit value(s) from
* it. Returns 0 on success, -EINVAL if the property does not exist,
* -ENODATA if property does not have a value, and -EOVERFLOW if the
* property data isn't large enough.
*
* dts entry of array should be like:
*    property = /bits/ 16 <0x5000 0x6000 0x7000>;
*
* The out_value is modified only if a valid u16 value can be decoded.
*/
int of_property_read_u16_array(const struct device_node *np,
            const char *propname, u16 *out_values, size_t sz)
{
    const __be16 *val = of_find_property_value_of_size(np, propname,
                        (sz * sizeof(*out_values)));

    if (IS_ERR(val))
        return PTR_ERR(val);

    while (sz--)
        *out_values++ = be16_to_cpup(val++);
    return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u16_array);
{% endhighlight %}

参数 np 指向当前节点；propbane 参数指向属性的名字；out_values 参数用于存储整数
的指针；

函数首先调用 of_find_property_value_of_size() 函数获得属性对应的属性值。然后进
行判断之后，将每个属性值存储到 out_values 中。

##### of_find_property_value_of_size

{% highlight c %}
/**
* of_find_property_value_of_size
*
* @np:        device node from which the property value is to be read.
* @propname:    name of the property to be searched.
* @len:    requested length of property value
*
* Search for a property in a device node and valid the requested size.
* Returns the property value on success, -EINVAL if the property does not
*  exist, -ENODATA if property does not have a value, and -EOVERFLOW if the
* property data isn't large enough.
*
*/
static void *of_find_property_value_of_size(const struct device_node *np,
            const char *propname, u8 len)
{
    struct property *prop = of_find_property(np, propname, NULL);

    if (!prop)
        return ERR_PTR(-EINVAL);
    if (!prop->value)
        return ERR_PTR(-ENODATA);
    if (len > prop->length)
        return ERR_PTR(-EOVERFLOW);

    return prop->value;
}
{% endhighlight %}

参数 np 指向当前节点；propname 指向属性的名字，len 参数表示属性的长度。

函数首先调用 of_find_property() 函数获得属性，然后分别判断属性，属性的值，以及
属性的长度是否都符合要求。当条件都满足之后，返回属性的值。

##### of_find_property

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

----------------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点默认打开，节点里面包含一个整
形数组的属性，通过调用 of_property_read_u16() 函数获得整形数组属性值，最后打印
属性值，函数定义如下：

{% highlight c %}
static inline int of_property_read_u16(const struct device_node *np,
                       const char *propname,
                       u16 *out_value)
{% endhighlight %}

这个函数经常用用于读取一个 16 位整数属性。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，
在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。节点包含一个整形
数组的属性，具体内容如下：

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
                BiscuitOS-data = <0x10203040 0x50607080>;
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
递。获得驱动对应的节点之后，通过调用 of_property_read_u16() 函数读取整数的属性
值，最后打印整数的值。驱动编写如下：

{% highlight c %}
/*
 * DTS: of_property_read_bool
 *      of_property_read_u8
 *      of_property_read_u16
 *      of_property_read_u32
 *
 * static inline bool of_property_read_bool(const struct device_node *np,
 *                            const char *propname)
 *  
 * static inline int of_property_read_u8(const struct device_node *np,
 *                            const char *propname,
 *                            u8 *out_value)
 *  
 * static inline int of_property_read_u16(const struct device_node *np,
 *                            const char *propname,
 *                            u16 *out_value)
 *  
 * static inline int of_property_read_u32(const struct device_node *np,
 *                            const char *propname,
 *                            u32 *out_value)
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
 *                BiscuitOS-data = <0x10203040 0x50607080>;
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
    bool bool_data;
    u8 u8_data;
    u16 u16_data;
    u32 u32_data;

    printk("DTS demo probe entence.\n");

    /* Read bool data from property */
    bool_data = of_property_read_bool(np, "BiscuitOS-data");
    printk("bool_data: %#x\n", bool_data);

    /* Read u8 data from property */
    of_property_read_u8(np, "BiscuitOS-data", &u8_data);
    printk("u8_data:   %#x\n", u8_data);

    /* Read u16 data from property */
    of_property_read_u16(np, "BiscuitOS-data", &u16_data);
    printk("u16_data:  %#x\n", u16_data);

    /* Read u32 data from property */
    of_property_read_u32(np, "BiscuitOS-data", &u32_data);
    printk("u32_data:  %#x\n", u32_data);

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

{% highlight c %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight c %}
[    3.534323] DTS demo probe entence.
[    3.534359] bool_data: 0x1
[    3.534359] u8_data:   0x10
[    3.534359] u16_data:  0x1020
[    3.534359] u32_data:  0x10203040
{% endhighlight %}

--------------------------------------------

# <span id="附录">附录</span>


