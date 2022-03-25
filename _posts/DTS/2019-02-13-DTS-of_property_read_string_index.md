---
layout: post
title:  "of_property_read_string_index"
date:   2019-02-22 18:50:30 +0800
categories: [HW]
excerpt: DTS API of_property_read_string_index().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_property_read_string_index](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_property_read_string_index)
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

##### of_property_read_string_index

{% highlight c %}
of_property_read_string_index
|
|---of_property_read_string_helper
    |
    |---of_find_property
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
>
> 函数作用：从 string-list 属性中，读出第 N 个字符串。

##### of_property_read_string_index

{% highlight c %}
/**
* of_property_read_string_index() - Find and read a string from a multiple
* strings property.
* @np:        device node from which the property value is to be read.
* @propname:    name of the property to be searched.
* @index:    index of the string in the list of strings
* @out_string:    pointer to null terminated return string, modified only if
*        return value is 0.
*
* Search for a property in a device tree node and retrieve a null
* terminated string value (pointer to data, not a copy) in the list of strings
* contained in that property.
* Returns 0 on success, -EINVAL if the property does not exist, -ENODATA if
* property does not have a value, and -EILSEQ if the string is not
* null-terminated within the length of the property data.
*
* The out_string pointer is modified only if a valid string can be decoded.
*/
static inline int of_property_read_string_index(struct device_node *np,
                        const char *propname,
                        int index, const char **output)
{
    int rc = of_property_read_string_helper(np, propname, output, 1, index);
    return rc < 0 ? rc : 0;
}
{% endhighlight %}

参数 np 指向设备节点；propname 指向属性名字；output 参数用于存储指定的字符
串；index 用于指定字符串在 string-list 中的索引。

函数直接调用 of_property_read_string_helper() 函数获得多个字符串。

##### of_property_read_string_helper

{% highlight c %}
/**
* of_property_read_string_util() - Utility helper for parsing string properties
* @np:        device node from which the property value is to be read.
* @propname:    name of the property to be searched.
* @out_strs:    output array of string pointers.
* @sz:        number of array elements to read.
* @skip:    Number of strings to skip over at beginning of list.
*
* Don't call this function directly. It is a utility helper for the
* of_property_read_string*() family of functions.
*/
int of_property_read_string_helper(struct device_node *np, const char *propname,
                   const char **out_strs, size_t sz, int skip)
{
    struct property *prop = of_find_property(np, propname, NULL);
    int l = 0, i = 0;
    const char *p, *end;

    if (!prop)
        return -EINVAL;
    if (!prop->value)
        return -ENODATA;
    p = prop->value;
    end = p + prop->length;

    for (i = 0; p < end && (!out_strs || i < skip + sz); i++, p += l) {
        l = strnlen(p, end - p) + 1;
        if (p + l > end)
            return -EILSEQ;
        if (out_strs && i >= skip)
            *out_strs++ = p;
    }
    i -= skip;
    return i <= 0 ? -ENODATA : i;
}
EXPORT_SYMBOL_GPL(of_property_read_string_helper);
{% endhighlight %}

参数 np 指向设备节点；propname 指向属性名字；out_strs 参数用于存储指定的字符
串；sz 参数指定了读取字符串的数量；skip 参数指定了从第几个字符串开始读取。

函数首先调用 of_find_property() 函数获得 propname 对应的属性，然后对获得的属性
和属性值进行有效性检查，检查不过直接返回错误；如果检查通过，接着计算属性的结束
地址后，使用 for 循环遍历属性的值，并且跳过 skip 对应的地址，然后将字符串都存
储在 out_strs 参数里。

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
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到
的属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不
为 NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含一个 string-list 属性，属
性里面包含了多个字符串，然后通过of_property_read_string_index() 函数读取第 N 
个字符串，函数定义如下：

{% highlight c %}
static inline int of_property_read_string_index(struct device_node *np,
                        const char *propname,
                        int index, const char **output)
{% endhighlight %}

这个函数经常用用于读取 string-list 属性中的第 N 个字符。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点包含名为 
BiscutisOS-strings 的属性，属性值为 “uboot”, "kernel", "rootfs", "BiscuitOS" 
四个字符串。具体内容如下：

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
                BiscuitOS-strings = "uboot", "kernel", "rootfs", "BiscuitOS";
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
递。获得驱动对应的节点之后，通过调用 of_property_read_string_index() 函数获得 
“BiscuitOS-strings”属性中第二个字符串，并打印属性值，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_property_match_string
 *      of_property_count_strings
 *      of_property_read_string_index
 *      of_property_read_string_array
 *      of_property_read_string_helper
 *
 * int of_property_match_string(struct device_node *np, const char *propname,
 *                                      const char *string)
 *
 * static inline int of_property_count_strings(struct device_node *np,
 *                                      const char *propname)
 *
 * static inline int of_property_read_string_index(struct device_node *np,
 *                                      const char *propname,
 *                                      int index, const char **output)
 *
 * static inline int of_property_read_string_array(struct device_node *np,
 *                                      const char *propname,
 *                                      const char **out_strs, size_t sz)
 *  
 * int of_property_read_string_helper(struct device_node *np,
 *                                      const char *propname,
 *                                      const char *out_strs,
 *                                      size_t sz, int skip)
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
 *                BiscuitOS-strings = "uboot", "kernel",
 *                                    "rootfs", "BiscuitOS";
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
    const char *string_array[4];
    const char *string;
    int count, index;
    int rc;

    printk("DTS demo probe entence.\n");

    /* Verify property whether contains special string. */
    rc = of_property_match_string(np, "BiscuitOS-strings", "rootfs");
    if (rc < 0)
        printk("BiscuitOS-strings doesn't contain \"rootfs\"\n");
    else
        printk("BiscuitOS-strings[%d] is \"rootfs\"\n", rc);

    /* Count the string on string-list property */
    count = of_property_count_strings(np, "BiscuitOS-strings");
    printk("String count: %#x\n", count);

    /* Read special string on string-list property with index */
    for (index = 0; index < count; index++) {
        rc = of_property_read_string_index(np, "BiscuitOS-strings",
                                                index, &string);
        if (rc < 0) {
            printk("Unable to read BiscuitOS-strings[%d]\n", index);
            continue;
        }
        printk("BiscuitOS-strings[%d]: %s\n", index, string);
    }
    
    /* Read number of strings from string-list property */
    rc = of_property_read_string_array(np, "BiscuitOS-strings",
                                                string_array, count);
    if (rc < 0) {
        printk("Faild to invoke of_property_read_string_array()\n");
        return -1;
    }
    for (index = 0; index < count; index++)
        printk("String_array[%d]: %s\n", index, string_array[index]);

    /* Read a string with index on string-list property (index = 2) */
    rc = of_property_read_string_helper(np, "BiscuitOS-strings",
                                                      &string, 1, 2);
    if (rc < 0)
        printk("Faild to invoke of_property_read_string_helper()!\n");
    else
        printk("BiscuitOS-strings[index=2]: %s\n", string);


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
[    3.580217] DTS demo probe entence.
[    3.580233] BiscuitOS-strings[2] is "rootfs"
[    3.580247] String count: 0x4
[    3.580258] BiscuitOS-strings[0]: uboot
[    3.580271] BiscuitOS-strings[1]: kernel
[    3.580284] BiscuitOS-strings[2]: rootfs
[    3.580297] BiscuitOS-strings[3]: BiscuitOS
[    3.580311] String_array[0]: uboot
[    3.580322] String_array[1]: kernel
[    3.580334] String_array[2]: rootfs
[    3.580346] String_array[3]: BiscuitOS
[    3.580358] BiscuitOS-strings[index=2]: rootfs
{% endhighlight %}

-----------------------------------------

# <span id="附录">附录</span>





