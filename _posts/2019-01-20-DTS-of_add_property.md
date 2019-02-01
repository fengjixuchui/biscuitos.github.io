---
layout: post
title:  "of_add_property"
date:   2019-01-20 09:49:30 +0800
categories: [HW]
excerpt: DTS API of_add_property().
tags:
  - DTS
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_add_property](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_add_property)
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

##### of_add_property

{% highlight ruby %}
of_add_property
|
|---of_property_notify
|
|---proc_device_tree_add_prop
{% endhighlight %}

> 平台： ARM64
>
> linux： 3.10/4.18

函数作用：将一个新的属性插入到当前节点中。

##### of_add_property

{% highlight ruby %}
/**
* of_add_property - Add a property to a node
*/
int of_add_property(struct device_node *np, struct property *prop)
{
    struct property **next;
    unsigned long flags;
    int rc;

    rc = of_property_notify(OF_RECONFIG_ADD_PROPERTY, np, prop);
    if (rc)
        return rc;

    prop->next = NULL;
    raw_spin_lock_irqsave(&devtree_lock, flags);
    next = &np->properties;
    while (*next) {
        if (strcmp(prop->name, (*next)->name) == 0) {
            /* duplicate ! don't insert it */
            raw_spin_unlock_irqrestore(&devtree_lock, flags);
            return -1;
        }
        next = &(*next)->next;
    }
    *next = prop;
    raw_spin_unlock_irqrestore(&devtree_lock, flags);

#ifdef CONFIG_PROC_DEVICETREE
    /* try to add to proc as well if it was initialized */
    if (np->pde)
        proc_device_tree_add_prop(np->pde, prop);
#endif /* CONFIG_PROC_DEVICETREE */

    return 0;
}
{% endhighlight %}

参数 np 指向当前 device_node; prop 指向新的 prop

函数首先调用 of_property_notify() 函数，这里不做过多讲解。然后获得节点的 
properties 属性链表，分别遍历链表中的所有属性，如果遇到新属性的名字和节点已
有的属性名字一样，那么就停止插入，直接返回 -1；如果检查通过，那么就将新属性
插入到节点的 properties 链表的尾部。节点的属性都存储在 device_node 的 
properties 成员维护的链表里，of_find_property() 函数就是遍历这个链表，从而找
到指定的属性。

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

----------------------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，然后通过of_add_property() 函数插入
多个属性到节点里，函数定义如下：

{% highlight ruby %}
int of_add_property(struct device_node *np, struct property *prop)
{% endhighlight %}

这个函数经常用用于插入新的属性到当前节点里。

#### DTS 文件

由于使用的平台是 ARM64，所以在源码 /arch/arm64/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。具体内容如下：

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
递。获得驱动对应的节点之后，通过调用 of_add_property() 函数将不同种类的属性插
入到当前节点，插入完成后再通过 DT 相关的函数读取其值并打印，驱动编写如下：

{% highlight ruby %}
/*
 * DTS: of_add_property
 *
 * int of_add_property(struct device_node *np, struct property *prop)
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

static u8  u8_data  = 0x10;
static u16 u16_data = 0x1020;
static u32 u32_data = 0x10203040;
static u64 u64_data = 0x1020304050607080;
static u32 u32_arr[]   = { 0x10203040, 0x50607080 };

static struct property BiscuitOS_u8 = {
    .name   = "BiscuitOS-u8",
    .length = sizeof(u8),
    .value  = &u8_data,
};

static struct property BiscuitOS_u16 = {
    .name   = "BiscuitOS-u16",
    .length = sizeof(u16),
    .value  = &u16_data,
};

static struct property BiscuitOS_u32 = {
    .name   = "BiscuitOS-u32",
    .length = sizeof(u32),
    .value  = &u32_data,
};

static struct property BiscuitOS_u64 = {
    .name   = "BiscuitOS-u64",
    .length = sizeof(u64),
    .value  = &u64_data,
};

static struct property BiscuitOS_u32arr = {
    .name   = "BiscuitOS-u32arry",
    .length = sizeof(u32) * 2,
    .value  = u32_arr,
};

static struct property BiscuitOS_string = {
    .name   = "BiscuitOS-string",
    .length = 10,
    .value  = "BiscuitOS",
};

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    u8  u8_data;
    u16 u16_data;
    u32 u32_data;
    u64 u64_data;
    u32 u32_array[2];
    const char *string;
    const char *string_arr[2];
    int rc;

    printk("DTS demo probe entence.\n");

    /* Add a u8 data property into device node */
    of_add_property(np, &BiscuitOS_u8);
    /* Read it from DT */
    rc = of_property_read_u8(np, "BiscuitOS-u8", &u8_data);
    if (rc == 0)
        printk("BiscuitOS-u8:  %#x\n", u8_data);

    /* Add a u16 data property into device node */
    of_add_property(np, &BiscuitOS_u16);
    /* Read it from DT */
    rc = of_property_read_u16(np, "BiscuitOS-u16", &u16_data);
    if (rc == 0)
        printk("BiscuitOS-u16: %#x\n", be16_to_cpu(u16_data));

    /* Add a u32 data property into device node */
    of_add_property(np, &BiscuitOS_u32);
    /* Read it from DT */
    rc = of_property_read_u32(np, "BiscuitOS-u32", &u32_data);
    if (rc == 0)
        printk("BiscuitOS-u32: %#x\n", be32_to_cpu(u32_data));

    /* Add a u64 data property into device node */
    of_add_property(np, &BiscuitOS_u64);
    /* Read it from DT */
    rc = of_property_read_u64(np, "BiscuitOS-u64", &u64_data);
    if (rc == 0)
        printk("BiscuitOS-u64: %#llx\n", be64_to_cpu(u64_data));

    /* Add a u32 array property into device node */
    of_add_property(np, &BiscuitOS_u32arr);
    /* Read array from DT */
    rc = of_property_read_u32_array(np, "BiscuitOS-u32arry", u32_array, 2);
    if (rc == 0) {
        printk("BiscuitOS-u32arry[0]: %#x\n", be32_to_cpu(u32_array[0]));
        printk("BiscuitOS-u32arry[1]: %#x\n", be32_to_cpu(u32_array[1]));
    }

    /* Add a string property into device node */
    of_add_property(np, &BiscuitOS_string);
    /* Read string from DT */
    of_property_read_string(np, "BiscuitOS-string", &string);
    printk("BiscuitOS-string: %s\n", string);

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
[    3.587479] DTS demo probe entence.
[    3.587496] BiscuitOS-u8:  0x10
[    3.587508] BiscuitOS-u16: 0x1020
[    3.587521] BiscuitOS-u32: 0x10203040
[    3.587534] BiscuitOS-u64: 0x1020304050607080
[    3.587549] BiscuitOS-u32arry[0]: 0x10203040
[    3.587562] BiscuitOS-u32arry[1]: 0x50607080
[    3.587576] BiscuitOS-string: BiscuitOS
{% endhighlight %}

驱动给出了多种属性的定义以及添加方法，开发者可以根据需求自行选择。

-------------------------------------------------

# <span id="附录">附录</span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)
