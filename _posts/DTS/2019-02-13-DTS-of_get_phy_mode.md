---
layout: post
title:  "of_get_phy_mode"
date:   2019-02-22 14:04:30 +0800
categories: [HW]
excerpt: DTS API of_get_phy_mode().
tags:
  - DTS
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_get_phy_mode](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_get_phy_mode)
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

##### of_get_phy_mode

{% highlight c %}
of_get_phy_mode
|
|---of_proprty_read_string
   |
   |---of_find_property
       |
       |---of_find_property
           |
           |---__of_find_property
              |
              |---of_prop_cmp
{% endhighlight %}

函数作用：函数用于读取 PHY 的模式。

> 平台： ARM64
>
> linux： 3.10

{% highlight c %}
/**
* of_get_phy_mode - Get phy mode for given device_node
* @np:    Pointer to the given device_node
*
* The function gets phy interface string from property 'phy-mode',
* and return its index in phy_modes table, or errno in error case.
*/
const int of_get_phy_mode(struct device_node *np)
{
    const char *pm;
    int err, i;

    err = of_property_read_string(np, "phy-mode", &pm);
    if (err < 0)
        return err;

    for (i = 0; i < ARRAY_SIZE(phy_modes); i++)
        if (!strcasecmp(pm, phy_modes[i]))
            return i;

    return -ENODEV;
}
EXPORT_SYMBOL_GPL(of_get_phy_mode);
{% endhighlight %}

参数 np 指向一个设备节点。

函数首先调用 of_property_read_string() 函数获得节点中 “phy-mode”属性对应的字符
串，字符串存储在 pm 变量里。如果获取失败，那么直接返回错误值。如果成功获得字符
串之后，就遍历 phy_modes[] 数组，每一次遍历的结果与 “phy-mode”属性值进行对比，
如果两者一致，那么返回字符串在 phy_modes 里的索引。如果没有找到，直接
返回 -ENODEV。

> 平台： ARM64
>
> linux： 4.18/5.0

{% highlight c %}
/**
* of_get_phy_mode - Get phy mode for given device_node
* @np:    Pointer to the given device_node
*
* The function gets phy interface string from property 'phy-mode' or
* 'phy-connection-type', and return its index in phy_modes table, or errno in
* error case.
*/
int of_get_phy_mode(struct device_node *np)
{
    const char *pm;
    int err, i;

    err = of_property_read_string(np, "phy-mode", &pm);
    if (err < 0)
        err = of_property_read_string(np, "phy-connection-type", &pm);
    if (err < 0)
        return err;

    for (i = 0; i < PHY_INTERFACE_MODE_MAX; i++)
        if (!strcasecmp(pm, phy_modes(i)))
            return i;

    return -ENODEV;
}
EXPORT_SYMBOL_GPL(of_get_phy_mode);
{% endhighlight %}

参数 np 指向一个设备节点。

函数首先调用 of_property_read_string() 函数获得节点中 “phy-mode”属性对应的字符
串，字符串存储在 pm 变量里。如果获取失败，那么再调用 of_property_read_string() 
函数，查找节点是否函数 “phy-connection-type”属性，如果有就读取其对应的字符串，
否则直接返回错误值。如果成功获得字符串之后，就遍历 phy_modes[] 数组，每一次遍
历的结果与 “phy-mode”属性值进行对比，如果两者一致，那么返回字符串在 phy_modes 
里的索引。如果没有找到，直接返回 -ENODEV。

##### phy_modes

{% highlight c %}
/**
* It maps 'enum phy_interface_t' found in include/linux/phy.h
* into the device tree binding of 'phy-mode', so that Ethernet
* device driver can get phy interface from device tree.
*/
static const char *phy_modes[] = {
    [PHY_INTERFACE_MODE_NA]        = "",
    [PHY_INTERFACE_MODE_MII]    = "mii",
    [PHY_INTERFACE_MODE_GMII]    = "gmii",
    [PHY_INTERFACE_MODE_SGMII]    = "sgmii",
    [PHY_INTERFACE_MODE_TBI]    = "tbi",
    [PHY_INTERFACE_MODE_RMII]    = "rmii",
    [PHY_INTERFACE_MODE_RGMII]    = "rgmii",
    [PHY_INTERFACE_MODE_RGMII_ID]    = "rgmii-id",
    [PHY_INTERFACE_MODE_RGMII_RXID]    = "rgmii-rxid",
    [PHY_INTERFACE_MODE_RGMII_TXID] = "rgmii-txid",
    [PHY_INTERFACE_MODE_RTBI]    = "rtbi",
    [PHY_INTERFACE_MODE_SMII]    = "smii",
};
{% endhighlight %}

##### of_property_read_string

{% highlight c %}
/**
* of_property_read_string - Find and read a string from a property
* @np:        device node from which the property value is to be read.
* @propname:    name of the property to be searched.
* @out_string:    pointer to null terminated return string, modified only if
*        return value is 0.
*
* Search for a property in a device tree node and retrieve a null
* terminated string value (pointer to data, not a copy). Returns 0 on
* success, -EINVAL if the property does not exist, -ENODATA if property
* does not have a value, and -EILSEQ if the string is not null-terminated
* within the length of the property data.
*
* The out_string pointer is modified only if a valid string can be decoded.
*/
int of_property_read_string(struct device_node *np, const char *propname,
                const char **out_string)
{
    struct property *prop = of_find_property(np, propname, NULL);
    if (!prop)
        return -EINVAL;
    if (!prop->value)
        return -ENODATA;
    if (strnlen(prop->value, prop->length) >= prop->length)
        return -EILSEQ;
    *out_string = prop->value;
    return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_string);
{% endhighlight %}

参数 np 指向一个设备节点；proprname 参数为需要查找属性的名字；参数 out_string 
存储属性的值，这里属性值为字符串。

函数首先调用 of_find_property() 函数获得节点中，属性名字为 propname 的属性，存
储在遍历 prop 中。如果获得属性为 NULL，那么所要查找的属性不存在。如果属性的值
为 NULL，则返回 -ENODATA 表示不正确的数据。然后调用 strnlen() 函数，检查属性值
最为字符串是否合法，不合法就返回 -EILSEQ。如果合法，就将属性的值存储早 
out_string 指针指向的地址，然后返回 0。

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

由于每个 device_node 包含一个名为 properties 的成员，properties 是一个单链表
的表头，这个单链表包含了该节点的所有属性，函数通过使用 for 循环遍历这个链表，
以此遍历节点所包含的所有属性。每遍历一个属性就会调用 of_prop_cmp() 函数，
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到
的属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不
为 NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

---------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含多个属性，其中一个属性
为 "phy-mode"，然后通过of_get_phy_mode() 函数获取 PHY 的模式，函数定义如下：

{% highlight c %}
const int of_get_phy_mode(struct device_node *np)
{% endhighlight %}

这个函数经常用用于读取网卡节点中 PHY 的模式。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，
在 root 节点之下创建一个名为 DTS_demo 的子节点。节点包含一个名为 phy-mode 的属
性，属性值为 "sgmii"。具体内容如下：

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
                phy-mode = "sgmii";
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
传递。获得驱动对应的节点之后，通过调用 of_get_phy_mode() 函数获得 “phy-mode”
属性的值，函数返回一个索引值，然后通过判断这个索引是否和  
PHY_INTERFACE_MODE_SGMII 相等，如果相等，那么就说明 PHY 的模式就是 SGMII，驱动
编写如下：

{% highlight c %}
/*
 * DTS: of_get_phy_mode
 *
 * const int of_get_phy_mode(struct device_node *np)
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
 *                phy-mode = "sgmii";
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
#include <linux/of_net.h>
#include <linux/phy.h>

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int mode;
    
    printk("DTS demo probe entence\n");

    /* Read PHY mode from DTS */
    mode = of_get_phy_mode(np);
    if (mode < 0)
        return mode;

    /* output mode string */
    if (mode == PHY_INTERFACE_MODE_SGMII)
        printk("PHY mode: sgmii\n");

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

{% highlight bash %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight bash %}
[    3.553142] io scheduler cfg registered (default)\
[    3.553439] DTS demo probe entence
[    3.553443] PHY mode: sgmii
{% endhighlight %}

驱动中，phy-mode 这个属性经常放在网卡节点中，作为标识网卡的 MAC 与 PHY 之间使用何种连接模式。例如网卡的节点：

{% highlight c %}
/ {
                ethernet@ff0b0000 {
                        compatible = "cdns,zynqmp-gem", "cdns,gem";
                        status = "disabled";
                        interrupt-parent = <0x4>;
                        interrupts = <0x0 0x39 0x4 0x0 0x39 0x4>;
                        reg = <0x0 0xff0b0000 0x0 0x1000>;
                        clock-names = "pclk", "hclk", "tx_clk", "rx_clk", "tsu_clk";
                        #address-cells = <0x1>;
                        #size-cells = <0x0>;
                        #stream-id-cells = <0x1>;
                        iommus = <0x9 0x874>;
                        power-domains = <0xe>;
                        clocks = <0x3 0x1f 0x3 0x68 0x3 0x2d 0x3 0x31 0x3 0x2c>;
                        phy-mode = "sgmii";
                        xlnx,ptp-enet-clock = <0x0>;
                        fixed-link = <0x2 0x1 0x3e8 0x0 0x0>;
                };
}；
{% endhighlight %}

---------------------------------------------------

# <span id="附录">附录</span>



