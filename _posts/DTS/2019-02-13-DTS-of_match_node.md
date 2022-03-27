---
layout: post
title:  "of_mdiobus_register"
date:   2019-02-22 15:02:30 +0800
categories: [HW]
excerpt: DTS API of_mdiobus_register().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_mdiobus_register](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_mdiobus_register)
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

##### of_mdiobus_register

{% highlight c %}
for_each_compatible_node
|
|---mdiobus_register
|
|---for_each_available_child_of_node
    |
    |---of_get_property 
    | 
    |---irq_of_parse_and_map
    |
    |---get_phy_device
    |
    |---phy_device_register
{% endhighlight %}

函数作用：注册一条 MDIO bus。

> 平台： ARM32
>
> linux： 3.10/4.14

函数代码较长，分段解析

{% highlight c %}
/**
* of_mdiobus_register - Register mii_bus and create PHYs from the device tree
* @mdio: pointer to mii_bus structure
* @np: pointer to device_node of MDIO bus.
*
* This function registers the mii_bus structure and registers a phy_device
* for each child node of @np.
*/
int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np)
{
    struct phy_device *phy;
    struct device_node *child;
    const __be32 *paddr;
    u32 addr;
    bool is_c45, scanphys = false;
    int rc, i, len;

mido 参数指向一条 MDIO bus； np 指向 device node

    /* Mask out all PHYs from auto probing.  Instead the PHYs listed in
     * the device tree are populated after the bus has been registered */
    mdio->phy_mask = ~0;

    /* Clear all the IRQ properties */
    if (mdio->irq)
        for (i=0; i<PHY_MAX_ADDR; i++)
            mdio->irq[i] = PHY_POLL;

    mdio->dev.of_node = np;

    /* Register the MDIO bus */
    rc = mdiobus_register(mdio);
    if (rc)
        return rc;
{% endhighlight %}

函数首先将 MDIO bus 的 phy_mask 设置为 ~0,这样就可以屏蔽所有的 PHY ID，以此自
动探测 PHY device。如果 mdio->irq 存在，清零所有的 IRQ 属性，将 mdio->irq[x] 
设置为 PHY_POLL。接着将 mdio 的 dev.of_node 指向当前的 device node。然后调用 
mdiobus_register() 函数，该函数会将 mdio 注册到 MDIO bus 框架上，并且会遍历探
测这条 MDIO bus 上的 PHY ID，如果 PHY ID 对应的 PHY 设备存在，那么就会为其创建
一个 phy_device 结构，因此，mdiobus_register() 函数成功执行之后，这条 mdio bus 
上的 PHY device 也被创建好了。mdiobus_register() 函数源码解析可以
看 《mdiobus register》

{% highlight c %}
    /* Loop over the child nodes and register a phy_device for each one */
    for_each_available_child_of_node(np, child) {
        /* A PHY must have a reg property in the range [0-31] */
        paddr = of_get_property(child, "reg", &len);
        if (!paddr || len < sizeof(*paddr)) {
            scanphys = true;
            dev_err(&mdio->dev, "%s has invalid PHY address\n",
                child->full_name);
            continue;
        }

        addr = be32_to_cpup(paddr);
        if (addr >= 32) {
            dev_err(&mdio->dev, "%s PHY address %i is too large\n",
                child->full_name, addr);
            continue;
        }

        if (mdio->irq) {
            mdio->irq[addr] = irq_of_parse_and_map(child, 0);
            if (!mdio->irq[addr])
                mdio->irq[addr] = PHY_POLL;
        }

        is_c45 = of_device_is_compatible(child,
                         "ethernet-phy-ieee802.3-c45");
        phy = get_phy_device(mdio, addr, is_c45);

        if (!phy || IS_ERR(phy)) {
            dev_err(&mdio->dev,
                "cannot get PHY at address %i\n",
                addr);
            continue;
        }

        /* Associate the OF node with the device structure so it
         * can be looked up later */
        of_node_get(child);
        phy->dev.of_node = child;

        /* All data is now stored in the phy struct; register it */
        rc = phy_device_register(phy);
        if (rc) {
            phy_device_free(phy);
            of_node_put(child);
            continue;
        }

        dev_dbg(&mdio->dev, "registered phy %s at address %i\n",
            child->name, addr);
    }
{% endhighlight %}

函数首先调用 for_each_avaiable_child_of_node() 函数遍历参数 np 节点的所有可用
子节点，对于可用的子节点，其 status 属性都设置为 “okay”。每次遍历一个可用的子
节点时，先读取子节点的 reg 属性，其值对应 PHY id，然后检查 PHY id 是否有效，
PHY id 的有效范围是 0 到 31，如果属性读取正常，那么调用 be32_to_cpup() 函数读
取 reg 属性的值，然后判断是否超出了 PHY id 的有效范围；如果属性读取失败或属性
长度有误，那么将 scanphys 设置为 true，并结束此次循环，进入下一次循环。如果 
mdio->irq 存在，那么调用 irq_of_parse_and_map() 函数读取中断属性信息，并保存在 
mdio->irq[] 数组内。紧接着函数调用 of_device_is_compatible() 函数检查子节点是
否包含 "ethernet-phy-ieee802.3-c45" 属性，如果包含，那么 MDIO bus 就支持 MDIO 
Clause 45 协议，并将 is_c45 设置为 true；否则将 is_c45 设置为 false。函数有调
用 get_phy_device() 获得 PHY ID 对应的 PHY device，由于之前调用 
midobus_register () 函数的缘故，这条 MDIO bus 上的所有可用的 PHY device 都申请
并初始化成一个 phy_device 设备。如果调用 get_phy_device() 函数之后不能获得子节
点对应的 PHY device，那么将报错并结束这次循环，进入下一次循环；如果 
PHY device 获取正常，那么就将 phy_device 的 dev.of_node 成员指向子节点。最后函
数会调用 phy_device_register() 函数去注册 PHY device，由于之前调用了 
mdiobus_register() 函数，所以 PHY device 已经注册，现在再注册的话会执行失败，
接着函数调用 phy_device_free() 函数和 of_node_put() 函数去解引用这个 
PHY device。

{% highlight c %}
if (!scanphys)
        return 0;

    /* auto scan for PHYs with empty reg property */
    for_each_available_child_of_node(np, child) {
        /* Skip PHYs with reg property set */
        paddr = of_get_property(child, "reg", &len);
        if (paddr)
            continue;

        is_c45 = of_device_is_compatible(child,
                         "ethernet-phy-ieee802.3-c45");

        for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
            /* skip already registered PHYs */
            if (mdio->phy_map[addr])
                continue;

            /* be noisy to encourage people to set reg property */
            dev_info(&mdio->dev, "scan phy %s at address %i\n",
                 child->name, addr);

            phy = get_phy_device(mdio, addr, is_c45);
            if (!phy || IS_ERR(phy))
                continue;

            if (mdio->irq) {
                mdio->irq[addr] =
                    irq_of_parse_and_map(child, 0);
                if (!mdio->irq[addr])
                    mdio->irq[addr] = PHY_POLL;
            }

            /* Associate the OF node with the device structure so it
             * can be looked up later */
            of_node_get(child);
            phy->dev.of_node = child;

            /* All data is now stored in the phy struct;
             * register it */
            rc = phy_device_register(phy);
            if (rc) {
                phy_device_free(phy);
                of_node_put(child);j
                continue;
            }

            dev_info(&mdio->dev, "registered phy %s at address %i\n",
                 child->name, addr);
            break;
        }
    }
{% endhighlight %}

如果子节点不包含 reg 属性，那么函数就会采用自动探测功能。函数首先调用 
for_each_available_child_of_node() 函数遍历所有可用的子节点，调用 
of_get_property() 函数调过可用节点中，不包含 reg 属性的节点。如果子节点中包含
 "ethernet-phy-ieee802.3-c45" 属性，那么这条 MDIO bus 支持 MDIO Clause 45 协议。
函数接着从 0 到 31 遍历所有的 PHY ID，调过已经注册的 PHY ID，然后通过调用 
get_phy_device() 函数获得一个 PHY device，如果 PHY ID 对应的 PHY device 存在，
那么就调用 phy_device_register() 注册这个 PHY device。期间如果 mdio->irq 存在，
那么会调用 irq_of_parse_and_map() 函数从子节点获得中断相关信息。

----------------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建两个私有节点，第一个私有节点默认打开，第二个私有节
点名字为 mdio，其包含两个子节点，phy0 和 phy1。节点 1 通过 phy-handle 属性引用
了节点 2 的 phy1 子节点，然后通过 of_mdiobus_register() 函数，注册 MDIO bus 和 
phy1，最后通过通用 PHY 子系统能够读到这个 PHY 的 ID。定义如下：

{% highlight c %}
int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np)
{% endhighlight %}

这个函数经常用用于通过注册一条新的 MDIO bus，并挂载 MDIO bus 上的 PHY device。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文件，
在 root 节点之下创建名为 DTS_demo，mdio 两个子节点。DTS_demo 节点默认打开，
mdio 节点包含了 phy0 和 phy1 两个子节点，具体内容如下：

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
                phy-handle = <&phy1>;
        };

        mdio {
                compatible = "DTS_mdio, BiscuitOS";
                #address-cells = <0x1>;
                #size-cells = <0x1>;

                phy0: phy@0 {
                        reg = <0x0>;
                };

                phy1: phy@1 {
                        reg = <0x1>;
                        status = "okay";
                };
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
递。获得驱动对应的节点之后，定义并初始化了一条新的 MDIO bus，并且这条 MDIO bus
上挂载了一个 PHY device。驱动中为 MDIO bus 准备了一套 fixup read/write 操作，
驱动在使用 of_mdiobus_register() 函数注册完 MDIO bus 之后，就调用 
get_phy_device() 函数获得 DTS_demo 节点对应的 PHY device，最后打印 PHY device 
ID 寄存器中的值 。驱动编写如下：

{% highlight c %}
/*
 * DTS: of_mdiobus_register
 *
 * int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np)
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
 *                phy-handle = <&phy1>;
 *        };
 *
 *        mdio {
 *                compatible = "DTS_mdio, BiscuitOS";
 *                #address-cells = <0x1>;
 *                #size-cells = <0x1>;
 *
 *                phy0: phy@0 {
 *                        reg = <0x0>;
 *                };
 *
 *                phy1: phy@1 {
 *                        reg = <0x1>;
 *                };
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
#include <linux/errno.h>
#include <linux/of_mdio.h>
#include <linux/mii.h>

/* define name for device and driver */
#define DEV_NAME      "DTS_demo"
#define MII_PHY_ID    0x2
#define PHY_ID        0x1
#define DTS_PHY_ID    0x141

/* Fixup PHY register on page 0 */
static unsigned short fixup_phy_copper_regs[] = {
    0x00,       /* Copper Control */
    0x00,       /* Copper Status */
    0x141,      /* PHY Identifier 1 */
    0x9c0,      /* PHY Identifier 2 */
    0x280,      /* Auto-Negotiation Advertisement Register */
    0x00,       /* Link Partner Ability Register */
    0x00,       /* Auto-Negotiation Expansion Register */
    0x00,       /* Next Page Transmit Register */
    0x00,       /* Link Partner Next Page Register */
    0x00,       /* 1000Base-T Control Register */
    0x00,       /* 1000Base-T Status Register */
    0x00,       
    0x00,
    0x00,
    0x00,
    0x00,       /* Extended Status Register */
    0x00,       /* Copper Specific Control Register 1 */
    0x00,       /* Copper Specific Status Register 1 */
    0x00,       /* Interrupt Enable Register */
    0x00,       /* Copper Specific Status Register 2 */
    0x00,       
    0x00,       /* Receive Error Control Register */
    0x00,       /* Page Address */
    0x00,
    0x00,
    0x00,
    0x00,       /* Copper Specific Control Register 2 */
    0x00,
    0x00,
    0x00,
    0x00,
};

/*
* fixup mdio bus read
*
*  @bus: mdio bus
*  @phy_addr: PHY device ID which range 0 to 31.
*  @reg_num: Register address which range 0 to 31 on MDIO Clause 22.
*
*  Return special register value.
*/
static int DTS_mdio_read(struct mii_bus *bus, int phy_addr, int reg_num)
{
    struct platform_device *pdev = bus->priv;
    struct device_node *np = pdev->dev.of_node;
    const phandle *ph;
    struct device_node *phy;
    int of_phy_id;
    
    /* find PHY handle on current device_node */
    ph = of_get_property(np, "phy-handle", NULL);

    /* Find child node by handle */
    phy = of_find_node_by_phandle(be32_to_cpup(ph));
    if (!phy) {
        printk("Unable to find device child node\n");
        return -EINVAL;
    }

    /* Read PHY ID on MDIO bus */
    of_property_read_u32(phy, "reg", &of_phy_id);
    if (of_phy_id < 0 || of_phy_id > 31) {
        printk("Invalid phy id from DT\n");
        return -EINVAL;
    }
    
    /* Read Special PHY device by PHY ID */
    if (phy_addr != of_phy_id)
        return -EINVAL;

    return fixup_phy_copper_regs[reg_num];
}

/*
* fixup mdio bus write
*
*  @bus: mdio bus
*  @phy_addr: PHY device ID which range 0 to 31.
*  @reg_num: Register address which range 0 to 31 on MDIO Clause 22.
*  @val: value need to write.
*
*  Return 0 always.
*/
static int DTS_mdio_write(struct mii_bus *bus, int phy_addr, int reg_num,
                              u16 val)
{
    /* Forbidding any written */

    return 0;
}

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct mii_bus *bus;
    unsigned short phy_id;
    struct phy_device *phy;

    printk("DTS Probe Entence...\n");

    bus = mdiobus_alloc();
    if (!bus) {
        printk("Unable to allocate memory to mii_bus.\n");
        return -EINVAL;
    }

    bus->name  = "DTS_mdio";
    bus->read  = &DTS_mdio_read;
    bus->write = &DTS_mdio_write;
    snprintf(bus->id, MII_BUS_ID_SIZE, "%s-mii", dev_name(&pdev->dev));
    bus->parent = &pdev->dev;
    bus->priv = pdev;

    platform_set_drvdata(pdev, bus);

    /* Register MDIO bus by DT */
    of_mdiobus_register(bus, np);

    /* MDIO read test */
    phy_id = bus->read(bus, PHY_ID, MII_PHY_ID);
    if (phy_id == DTS_PHY_ID) {
        phy = get_phy_device(bus, PHY_ID, 0);
        if (!phy) {
            printk("Unable to get PHY device.\n");
            return -EINVAL;
        }
        printk("PHY device ID: %#x\n", phy->phy_id);
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
[    3.534323] DTS Probe Entence...
[    3.534359] libphy: DTS_mdio: probe
[    3.534364] PHY device ID: 0x14109c0
{% endhighlight %}

为了成功注册 MDIO bus，驱动中为 MDIO bus 准备了一套 fixup MDIO read/write 操作。
Fixup PHY 俗称假 PHY，代表不是真实的 PHY，实践中 Switch 也是一种假 PHY。驱动中 
fixup MDIO read 的实现逻辑就是从 DT 中读取 DTS_demo 节点 phy-handle 对应节点的 
phy id, 如果这个 phy id 和 MDIO bus read 中的 phy_id 一致时，就读取 
fixup_phy_copper_reg[] 数组里面对应的值，以此默认真实的 MDIO 从 PHY 中读值的
操作。

fixup_phy_copper_reg[] 数组是模拟 PHY 的 copper 页寄存器值，以此模拟一个 PHY 
的基本操作。

{% highlight c %}
/* Fixup PHY register on page 0 */
static unsigned short fixup_phy_copper_regs[] = {
    0x00,       /* Copper Control */
    0x00,       /* Copper Status */
    0x141,      /* PHY Identifier 1 */
    0x9c0,      /* PHY Identifier 2 */
    0x280,      /* Auto-Negotiation Advertisement Register */
    0x00,       /* Link Partner Ability Register */
    0x00,       /* Auto-Negotiation Expansion Register */
    0x00,       /* Next Page Transmit Register */
    0x00,       /* Link Partner Next Page Register */
    0x00,       /* 1000Base-T Control Register */
    0x00,       /* 1000Base-T Status Register */
    0x00,       
    0x00,
    0x00,
    0x00,
    0x00,       /* Extended Status Register */
    0x00,       /* Copper Specific Control Register 1 */
    0x00,       /* Copper Specific Status Register 1 */
    0x00,       /* Interrupt Enable Register */
    0x00,       /* Copper Specific Status Register 2 */
    0x00,       
    0x00,       /* Receive Error Control Register */
    0x00,       /* Page Address */
    0x00,
    0x00,
    0x00,
    0x00,       /* Copper Specific Control Register 2 */
    0x00,
    0x00,
    0x00,
    0x00,
};
{% endhighlight %}

--------------------------------------

# <span id="附录">附录</span>



