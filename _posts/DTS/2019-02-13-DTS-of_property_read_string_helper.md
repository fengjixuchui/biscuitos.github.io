---
layout: post
title:  "of_property_read_string_helper"
date:   2019-02-22 18:27:30 +0800
categories: [HW]
excerpt: DTS API of_property_read_string_helper().
tags:
  - DTS
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_property_read_string_helper](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_property_read_string_helper)
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

##### of_property_match_string

{% highlight c %}
of_property_match_string
|
|---of_find_property
    |
    |---of_find_property
        |
        |---__of_find_property
            |
            |---of_prop_cmp
{% endhighlight %}

> 平台： ARM64
>
> linux： 3.10/4.18/5.0
>
> 函数作用：检查 string-list 属性中，是否包含特定的字符串。

##### of_property_match_string

{% highlight c %}
/**
* of_property_match_string() - Find string in a list and return index
* @np: pointer to node containing string list property
* @propname: string list property name
* @string: pointer to string to search for in string list
*
* This function searches a string list property and returns the index
* of a specific string value.
*/
int of_property_match_string(struct device_node *np, const char *propname,
                 const char *string)
{
    struct property *prop = of_find_property(np, propname, NULL);
    size_t l;
    int i;
    const char *p, *end;

    if (!prop)
        return -EINVAL;
    if (!prop->value)
        return -ENODATA;

    p = prop->value;
    end = p + prop->length;

    for (i = 0; p < end; i++, p += l) {
        l = strnlen(p, end - p) + 1;
        if (p + l > end)
            return -EILSEQ;
        pr_debug("comparing %s with %s\n", string, p);
        if (strcmp(string, p) == 0)
            return i; /* Found it; return index */
    }
    return -ENODATA;
}
EXPORT_SYMBOL_GPL(of_property_match_string);
{% endhighlight %}

参数 np 指向设备节点；propname 指向属性名字；string 参数表示需要对比的字符串

函数首先调用 of_find_property() 函数获得 propname 对应的属性，然后对获得的属
性和属性值进行有效性检查，检查不过直接返回错误；如果检查通过，接着计算属性的
结束地址后，使用 for 循环遍历属性的值，调用 strcmp() 函数对比字符串是否符合
要求；如果找到符合要求的字符串，直接返回字符串在 string-list 中的索引；否则
返回 -ENODATA。

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

参数 np 指向一个 device_node；name 参数表示需要查看属性的名字；lenp 用于存
储属性长度。

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

由于每个 device_node 包含一个名为 properties 的成员，properties 是一个单链表
的表头，这个单链表包含了该节点的所有属性，函数通过使用 for 循环遍历这个链表，
以此遍历节点所包含的所有属性。每遍历一个属性就会调用 of_prop_cmp() 函数，
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到
的属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不
为 NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

--------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含一个 string-list 属性，
属性里面包含了多个字符串，然后通过of_property_match_string() 函数查找 
string-list 中是否函数 string 参数对应的字符串，函数定义如下：

{% highlight c %}
int of_property_match_string(struct device_node *np, const char *propname,
                 const char *string)
{% endhighlight %}

这个函数经常用用于查找 string-list 属性中是否包含特定的字符串。

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
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员
传递。获得驱动对应的节点之后，通过调用 of_property_match_string() 函数查找 
“BiscuitOS-strings”属性中是否函数 “rootfs”字符串，并打印查询的结果，驱动编写
如下：

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

------------------------------------------

# <span id="附录">附录</span>
