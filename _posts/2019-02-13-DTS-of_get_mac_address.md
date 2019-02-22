---
layout: post
title:  "of_get_mac_address"
date:   2019-02-22 11:11:30 +0800
categories: [HW]
excerpt: DTS API of_get_mac_address().
tags:
  - DTS
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_get_mac_address](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_get_mac_address)
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

##### of_get_mac_address

{% highlight c %}
of_get_mac_address
|
|---of_find_property
    |
    |---of_find_property
        |
        |---__of_find_property
           |
           |---of_prop_cmp
{% endhighlight %}

函数作用：函数用于读取网卡的 MAC 地址。

> 平台： ARM32
>
> linux： 3.10

{% highlight c %}
/**
* Search the device tree for the best MAC address to use.  'mac-address' is
* checked first, because that is supposed to contain to "most recent" MAC
* address. If that isn't set, then 'local-mac-address' is checked next,
* because that is the default address.  If that isn't set, then the obsolete
* 'address' is checked, just in case we're using an old device tree.
*
* Note that the 'address' property is supposed to contain a virtual address of
* the register set, but some DTS files have redefined that property to be the
* MAC address.
*
* All-zero MAC addresses are rejected, because those could be properties that
* exist in the device tree, but were not set by U-Boot.  For example, the
* DTS could define 'mac-address' and 'local-mac-address', with zero MAC
* addresses.  Some older U-Boots only initialized 'local-mac-address'.  In
* this case, the real MAC is in 'local-mac-address', and 'mac-address' exists
* but is all zeros.
*/
const void *of_get_mac_address(struct device_node *np)
{
    struct property *pp;

    pp = of_find_property(np, "mac-address", NULL);
    if (pp && (pp->length == 6) && is_valid_ether_addr(pp->value))
        return pp->value;

    pp = of_find_property(np, "local-mac-address", NULL);
    if (pp && (pp->length == 6) && is_valid_ether_addr(pp->value))
        return pp->value;

    pp = of_find_property(np, "address", NULL);
    if (pp && (pp->length == 6) && is_valid_ether_addr(pp->value))
        return pp->value;

    return NULL;
}
EXPORT_SYMBOL(of_get_mac_address);
{% endhighlight %}

参数 np 指向 device node。

函数通过依次调用 of_find_property() 函数，查找节点的所有属性中，是否包含 
"mac-address", "local-mac-address", "address" 三个属性，如果节点同时存在以上两
个或两个以上属性时，函数首先读取 “mac-address” 的属性值。如果 “mac-address”属
性不存的话，就读取 "local-mac-address" 属性；如果两者不存在才读取 "address" 属
性。当成功获得属性之后，首先判断属性的长度是否为 6，然后调用 
is_valid_ether_addr() 函数检测 MAC 地址的有效性，如果按优先级的属性谁先满足这
些条件，那么就使用这个 MAC 地址。如果都不满足，就返回 NULL。

> 平台： ARM32
>
> linux： 4.14/4.18/5.0

{% highlight c %}
/**
* Search the device tree for the best MAC address to use.  'mac-address' is
* checked first, because that is supposed to contain to "most recent" MAC
* address. If that isn't set, then 'local-mac-address' is checked next,
* because that is the default address.  If that isn't set, then the obsolete
* 'address' is checked, just in case we're using an old device tree.
*
* Note that the 'address' property is supposed to contain a virtual address of
* the register set, but some DTS files have redefined that property to be the
* MAC address.
*
* All-zero MAC addresses are rejected, because those could be properties that
* exist in the device tree, but were not set by U-Boot.  For example, the
* DTS could define 'mac-address' and 'local-mac-address', with zero MAC
* addresses.  Some older U-Boots only initialized 'local-mac-address'.  In
* this case, the real MAC is in 'local-mac-address', and 'mac-address' exists
* but is all zeros.
*/
const void *of_get_mac_address(struct device_node *np)
{
    const void *addr;

    addr = of_get_mac_addr(np, "mac-address");
    if (addr)
        return addr;

    addr = of_get_mac_addr(np, "local-mac-address");
    if (addr)
        return addr;

    return of_get_mac_addr(np, "address");
}
EXPORT_SYMBOL(of_get_mac_address);
{% endhighlight %}

参数 np 指向 device node。

函数通过调用 of_get_mac_addr() 函数，按优先级方式从节点中依次读入 
"mac-address", "local-mac-address", 和 "address" 三个属性，只要其中一个属性调
用 of_get_mac_addr() 函数之后，返回值不为零，那么

##### of_get_mac_addr

{% highlight c %}
static const void *of_get_mac_addr(struct device_node *np, const char *name)
{
    struct property *pp = of_find_property(np, name, NULL);

    if (pp && pp->length == ETH_ALEN && is_valid_ether_addr(pp->value))
        return pp->value;
    return NULL;
}
{% endhighlight %}

参数 np 指向 device node；name 标识属性的名字

函数首先调用 of_find_property() 函数和 name 参数获得指定的属性。然后判断属性的
值 (即 MAC 地址) 是否长度符合要求和 MAC 地址是否有效，通过调用 
is_valid_ether_addr() 函数，如果检测通过，则返回属性的名字，即返回 MAC 地址；
如果检测失败，则返回 NULL。

##### is_valid_ether_addr

{% highlight c %}
/**
* is_valid_ether_addr - Determine if the given Ethernet address is valid
* @addr: Pointer to a six-byte array containing the Ethernet address
*
* Check that the Ethernet address (MAC) is not 00:00:00:00:00:00, is not
* a multicast address, and is not FF:FF:FF:FF:FF:FF.
*
* Return true if the address is valid.
*/
static inline bool is_valid_ether_addr(const u8 *addr)
{
    /* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
     * explicitly check for it here. */
    return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}
{% endhighlight %}

函数用于检测 MAC 地址是否为多播地址或零地址。

##### is_multicast_ether_addr

{% highlight c %}
/**
* is_multicast_ether_addr - Determine if the Ethernet address is a multicast.
* @addr: Pointer to a six-byte array containing the Ethernet address
*
* Return true if the address is a multicast address.
* By definition the broadcast address is also a multicast address.
*/
static inline bool is_multicast_ether_addr(const u8 *addr)
{
    return 0x01 & addr[0];
}
{% endhighlight %}

函数用于判断 MAC 地址是否为多播地址。

##### is_zero_ether_addr

{% highlight c %}
/**
* is_zero_ether_addr - Determine if give Ethernet address is all zeros.
* @addr: Pointer to a six-byte array containing the Ethernet address
*
* Return true if the address is all zeroes.
*/
static inline bool is_zero_ether_addr(const u8 *addr)
{
    return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}
{% endhighlight %}

函数用于判断 MAC 地址是否为零

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

----------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建一个私有节点，私有节点包含多个属性，其中有两个属性为
"mac-address" 和 "local-mac-address"，然后通过调用 of_get_mac_address() 函数来
获得 MAC 地址，函数定义如下：

{% highlight c %}
const void *of_get_mac_address(struct device_node *np)
{% endhighlight %}

这个函数经常用用于读取网卡节点的 MAC 地址。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](https://biscuitos.github.io/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，
在 root 节点之下创建一个名为 DTS_demo 的子节点。节点包含名为 mac-address 的属
性，属性值为 00 00 00 00 00 00；节点又包含一个名为 local-mac-address 的属性，
属性值为 01 00 5e 0f 64 c5，是一个多播地址。节点又包含一个名为 address 的属性，
属性值为 00 0a 35 00 b4 c0具体内容如下：

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
                mac-address = [00 00 00 00 00 00];
                local-mac-address = [01 00 5e 0f 64 c5];
                address = [00 0a 35 00 b4 c0];
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
递。获得驱动对应的节点之后，通过调用 of_get_mac_address() 函数获得 MAC 地址的
值并打印，驱动编写如下：

{% highlight c %}
/*
 * DTS: of_get_mac_address
 *
 * const void *of_get_mac_address(struct device_node *np)
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
 *                mac-address = [00 00 00 00 00 00];
 *                local-mac-address = [01 00 5e 0f 64 c5];
 *                address = [00 0a 35 00 b4 c0];
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

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const char *mac;
    
    printk("DTS demo probe entence\n");

    /* obtain MAC address from DTS */
    mac = of_get_mac_address(np);
    if (mac)
        printk("MAC Address: %02x %02x %02x %02x %02x %02x\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[8]);

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
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight bash %}
[    3.553142] io scheduler cfg registered (default)
[    3.553439] DTS demo probe entence
[    3.553443] MAC Address: 00 0a 35 00 b4 c0
{% endhighlight %}

DTS 节点中准备了三个属性 “mac-address” ，“local-mac-address”，和 “address”，
但 “mac-address”属性的值是一个全零的 MAC 地址，显然不符合要求，所以 
of_get_mac_address() 不会返回这个 MAC 地址；“local-mac-address”属性的值是一个
多播地址，也不符号要求，所以 of_get_mac_address() 不会返回这个 MAC 地址。最
后 “address” 属性的值是一个正常的 MAC 地址，所以 of_get_mac_address() 会返回
这个 MAC 地址。

-------------------------------------------

# <span id="附录">附录</span>



