---
layout: post
title:  "DTS 中使用 Interrupt"
date:   2018-11-14 16:33:30 +0800
categories: [HW]
excerpt: DTS 中使用 Interrupt.
tags:
  - DTS
  - Interrupt
---

>**Architecture: ARM**

>**Kernel: Linux 4.4**

# DTS 中 interrupt 描述

### Client 节点中添加中断信息


在节点中添加中断至少包含一个 interrupts 属性，或者 interrupts-extended 属性，
或者两者。如果两者都存在，节点优先使用 interrupts-extended 属性，而 
interrupts 属性此时只是为兼容软件而不做其他用途。这些属性都可以包含多个中断标
识。

interrupts 属性需要指定一个中断父节点，中断父节点使用 interrupt-parent 指定，
如下：

{% highlight ruby %}
Example：
    interrupt-parent = <&int1>;
    interrupts = <5 0>, <6 0>;
{% endhighlight %}

interrupt-parent 属性用于指定用于接收和路由的中断控制器，每个中断控制器在 DTS 
中会由标签，用于指向 phandle，在上里中，interrupt-parent 指向 int1 中断控制器,
int1 是中断控制器的标签，也是执行中断控制器的 phandle。如果节点中没有
interrupt-parent 属性，那么该节点的 interrupts 属性的父节点和根节点的 
interrupt-parent 一样。

当一个节点需要引用多个中断控制器中的中断时，可以使用 interrupts-extended 属性。
interrupts-extended 的每个入口包含了中断控制器的 phandle 和中断标识。
interrupts-extended 只有节点需要多个中断控制器的中断时才使用，其他均不适用。

{% highlight ruby %}
Example：
    interrupts-extended = <&intc1 5 1>, <&intc2 1 0>;
{% endhighlight %}

### 中断控制器节点中添加中断信息

一个中断控制器节点必须含有 interrupt-controller 属性，该属性是一个 bool 属性，
其值必须为空。也就是说，只要节点中含有 interrupt-controller 属性，那么这个节点
就是中断控制器。中断控制器节点必须含有 #interrupt-cells 属性，该属性由于定义节
点中中断标识的 cell 数量。以下是三种 cell 中断表示的例子

##### One cell

中断控制器中，#interrupt-cells 属性设置为 1，此时节点中的中断标识只含有一个 
cell，该 cell 用于指明终中断控制器内和偏移。

{% highlight ruby %}
Example：
    vic: interrupt@10140000 {
        compatible = "arm,versatile-vic";
        interrupt-controller;
        #interrupt-cells = <1>;
        reg = <0x10140000 0x1000>;
    };

    sic: intc@10003000 {
        compatible = "arm,versatile-sic";
        interrupt-controller;
        #interrupt-cells = <1>;
        reg = <0x10003000 0x1000>;
        interrupt-parent = <&vic>;
        interrupts = <31>; /* Cascaded to vic */
    };
{% endhighlight %}

本例中 vic，sic 都是中断控制器的节点，其中 vic 控制器定义了 cell 数为 1，那么
所有以它为父节点的节点，中断标识只含有一个 cell。如上 sic 也是一个中断控制器，
其以 vic 为父节点，所有 sic 的 interrupts 属性就只能含有一个 cell，sic 使用的
中断在 vic 中的偏移是 31.

##### Two Cell

中断控制器中，#interrupt-cell 属性设置为 2，此时节点中的中断标识含有两个 cell，
第一个 cell 用于指明控制器内的偏移。第二个 cell 用于指明触发方式和级别，具体如
下：

{% highlight ruby %}
-bits[3:0] 触发类型和级别标志：

    1 = low-to-high edge triggered
    2 = high-to-low edge triggered
    4 = active high level-sensitive
    8 = active low level-sensitive
{% endhighlight %}

{% highlight ruby %}
Example:
    i2c@7000c000 {
        gpioext: gpio-adnp@41 {
            compatible = "ad,gpio-adnp";
            reg = <0x41>;
            intrrupt-parent = <&gpio>;
            interrupts = <160 1>;
            gpio-controller;
            #gpio-cells = <1>;
            interrupt-controller;
            #interrupt-cells = <2>;
            nr-gpios = <64>;
        };
        sx8634@2b {
            compatible = "smtc,sx8634";
            reg = <0x2b>;
            interrupt-parent = <&gpioext>;
            interrupts = <3 0x8>;
            #address-cells = <1>;
            #size-cells = <0>;
            threshold = <0x40>;
            sensitivity = <7>;
        };
    };
{% endhighlight %}

-------------------------------------------------------

# ARM General Interrupt Controller (GIC) on DTS

![GIC](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/INT000001.png)

## GIC

GIC (Generic Interrupt Controller) 是 ARM 提供的一个通用的中断控制器，GIC 通过
AMBA (Advanced Microcontroller Bus Architecture) 片上总线连接到一个或者多个
ARM 处理器上. ARM SMP 多核心结构通常带有一个 GIC, 以此提供 CPU 独占中断 (PPI)
和 CPU 共享中断 (SGI).

## DTS 中的 GIC

主 GIC 直接设计和 CPU 在一起，这类 GIC 具有典型的 PPI 和 SGI 中断。次级的 GIC
级联在主 GIC 上，向主 GIC 上报中断，但没有 PPI 和 SGI 中断。典型的 gic 节点描
述如下：

{% highlight ruby %}
Example：
    gic: interrupt-controller@d000 {
        compatible = "arm,cortex-a9-gic";
        interrupt-controller;
        #interrupt-cells = <3>;
        #size-cells = <0>;
        reg = <0xd000 0x1000>, 
              <0xc100 0x1000>;
    };
{% endhighlight %}

GIC 主节点的 compatible 属性应该属于为以下其中之一的值：

{% highlight ruby %}
- compatible:
    "arm,arm1176jzf-devchip-gic"
    "arm,arm11mp-gic"
    "arm,cortex-a15-gic"
    "arm,cortex-a7-gic"
    "arm,cortex-a9-gic"
    "arm,gic-400"
    "arm,pl390"
    "brcm,brahma-b15-gic"
    "qcom,msm-8660-qgic"
    "qcom,msm-qgic2"
{% endhighlight %}

interrupt-controller 属性用于指明这个节点是中断控制器，interrupt-controller 是
一个 bool 值，其值为空。

**#interrupt-cells** 属性由于指明中断源中 cell 的数量，GIC 中 interrupt-cells
的值为 3。每个 cell 的含义如下：

第一个 cell 代表中断类型。 0 是 SPI 中断，1 是 PPI 中断。
第二个 cell 代表不同类型中断的中断号。SPI 中断号的范围从 0 到 987； PPI 中断号
从 0 到 15。
第三个 cell 代表中断标识，各标识含义如下：

{% highlight ruby %}
    bits[3:0] 代表触发类型
        1 = 上升沿 IRQ_TYPE_EDGE_RISING
        2 = 下降沿 IRQ_TYPE_EDGE_FALLING
        4 = 高电平有效 IRQ_TYPE_LEVEL_HGIH
        8 = 低电平有效 IRQ_TYPE_LEVEL_LOW
    bits[15:8] PPI 中断 CPU 掩码
{% endhighlight %}

对于 GIC，它可以管理 4 种类型的中断：

#### 外设中断 (Peripheral interrupt)

根据目标 CPU 的不同，外设的中断可以分为 PPI (Private Peripheral Interrupt) 和 
SPI (Shared Peripheral Interrupt)。 PPI 只能分配给一个确定的处理器，而 SPI 可
以有 Distributor 将中断分配给一组处理器中的一个进行处理。外设类型的中断一般通
过一个 interrupt request line 的硬件信号线连接到中断控制器，可能是电平触发的 
(Level-sensitive)， 也可能是边缘触发的 (Edge-triggered)。

#### 软件触发的中断 (SGI, software-generated interrupt)

软件可以通过写 GICD_SGIR 寄存器来触发一个中断事件，这样的中断，可以用于处理器
之间的通信。

#### 虚拟中断 (Virtual Interrupt) 和 Maintenance Interrupt

这两种中断和本文无关。


在 DTS 中，外设的 interrupt type 有两种，一种是 SPI，另外一种是 PPI。SGI 用于
处理器之间的通信，和外设无关。GIC 最大支持 1020 个 HW interrupt ID, 具体的 ID 
分配情况如下：

##### ID0 ~ ID31

用于分发到一个特定的处理器的 interrupt。 标识这些 interrupt 不能仅仅依靠 ID，
因为各个 interrupt source 都用同样的 ID0 ~ ID31 来标识，因此识别这些 
interrupt 需要 interrupt ID + CPU interface number。ID0 ~ ID15 用于 SGI，
ID16 ~ ID31 用于 PPI。PPI 类型的中断会送到指定的 processor 上，和其他的 
process 无关。SGI 是通过写 GICD_SGIR 寄存器而触发的中断。 Distributor 通过 
processor source ID，中断 ID 和 target processor ID 来唯一标识一个 SGI。

##### ID32 ~ ID1019

用于 SPI。


具体如何解析中断标识？这个需要限定在一定的上下文中，不同的 interrupt controller
会有不同的解释。因此，对于一个包含多个 interrupt controller 的系统，每个 
interrupt controller 及其相连的外设组成一个 interrupt domain，各个外设的 
interrupt 标识只能在属于它的那个 interrupt domain 中得到解析。#interrupt-cells
定义了 interrupt domain 中，用多少个 cell 来描述一个外设的 interrupt 标识。

-------------------------------------------------

# 驱动中从 DTS 获取中断信息

我们在 DTS 中创建一个新的节点，节点中包含了一个中断，中断使用 GPIO 11，而且 
GPIO 11 属于 GPIO0 控制器，所以可以按如下编写节点的属性

{% highlight ruby %}
demo: demo2@2 {
    compatible = "demo,demo_intr";
    reg = <2>;
    interrupt-parent = <&gpio0>;
    interrupts = <11 2>;
    status = "okay";
};
{% endhighlight %}

如上所示，interrupt-parent 指向 gpio0 控制器，由于中断使用的 gpio 11 在 gpio0 
控制器中的偏移值为 11，所以 interrupts 属性的第一个 cells 值设置为 11，第二个
cells 根据需求自行设置，其值可以为 1，2，4，8.

编写 DTS 中的节点信息之后，开发者在驱动中使用如下函数以及头文件，以此从 dts 中
获得 interrupt 的信息：

{% highlight ruby %}
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>

/* Obtain interrupt ID from DTS */
int irq = of_irq_get(np, 0);

/* Register Interrupt */
int ret = request_threaded_irq(irq, NULL, intr_handler, IRQF_ONESHOT | IRQ_TRIGGER_FALLING, "demo", NULL);

/* Free interrupt */
free_irq(irq, NULL);

/* Interrupt handler */
static irqreturn_t intr_handler(int irq, void *dev_id)
{
    return IRQ_HANDLED;
}
{% endhighlight %}

of_irq_get() 函数可以从 dts 中获得对应节点的中断号。获得中断号之后，可以根据实
际需求选择中断注册函数，并且编写对应的中断处理函数。使用完中断记得释放中断给其
他贡献该中断的资源。

完整的驱动程序如下：

{% highlight ruby %}
/*
* Interrupt on DTS
*
* (C) 2018.11.14 <buddy.zhang@aliyun.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

/*
*        demo: demo2@2 {
*                compatible = "demo,demo_intr";
*                reg = <2>;
*                interrupt-parent = <&gpio0>;
*                interrupts = <11 2>;
*                status = "okay";
*        };
*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

/* define name for device and driver */
#define DEV_NAME "demo_intr"
static int irq;

static irqreturn_t demo_irq_handler(int irq, void *dev_id)
{

    printk("Hello World\n");

    return IRQ_HANDLED;
}

/* probe platform driver */
static int demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int ret;
    
    /* Obtain interrupt ID from DTS */
    irq = of_irq_get(np, 0);

    /* Request irq handler */
    ret = request_threaded_irq(irq, NULL, demo_irq_handler,
                    IRQF_ONESHOT | IRQF_TRIGGER_FALLING, "demo", NULL);
    if (ret) {
        printk("Failed to acquire irq %d\n", irq);
        return -EINVAL;
    }

    return 0;
}

/* remove platform driver */
static int demo_remove(struct platform_device *pdev)
{
    /* Release Interrupt */
    free_irq(irq, NULL);

    return 0;
}

static const struct of_device_id demo_of_match[] = {
    { .compatible = "demo,demo_intr", },
    { },
};
MODULE_DEVICE_TABLE(of, demo_of_match);

/* platform driver information */
static struct platform_driver demo_driver = {
    .probe  = demo_probe,
    .remove = demo_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DEV_NAME, 
        .of_match_table = demo_of_match,
    },
};

module_platform_driver(demo_driver);
{% endhighlight %}

------------------------------------------------------

# 实例 一： Marvell A385 中使用 DTS

项目中需要使用一个 MCP2515 SPI 转 CAN 模块，在这个模块中，需要使用一根独立的中
断。于是，硬件设计上将 GPIO 11 连接到 MCP2515 的中断上。在 DTS 中按如下配置 
MCP2515

{% highlight ruby %}
gpio0: gpio@18140 {
    compatible = "marvell,orion-gpio";
    reg = <0x18100 0x40>;
    ngpios = <2>;
    gpio-controller;
    #gpio-cells = <2>;
    interrupt-controller;
    #interrupt-cells = <2>;
    interrupts = <GIC_SPI 53 IRQ_TYPE_LEVEL_HIGH>,
                 <GIC_SPI 54 IRQ_TYPE_LEVEL_HIGH>,
                               <GIC_SPI 55 IRQ_TYPE_LEVEL_HIGH>,
                               <GIC_SPI 56 IRQ_TYPE_LEVEL_HIGH>;
};

spi@10600 {
    mcp2515can: can@2 {
        compatible = "microchip,mcp2515";
        status = "okay";
        reg = <2>;
        spi-max-frequency = <10000000>;
        interrupt-parent = <&gpio0>;
        interrupts = <11 2>;
    };
}；
{% endhighlight %}

对应的硬件手册如下：

![A385](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/INT000002.png)

GPIO0 控制器使用了 53， 54， 55， 56 四条 SPI 中断，而且在 GPIO0 的 DTS 中，
 **#interrupt-cells** 为 2， 所以其他节点的 interrupt-parent 指向 gpio0 的 
interrupt-specify 就含有两个 cells。

通过上面的分析，MCP2515 的中断连接到 GPIO 11， 而 GPIO 11 连接到 GPIO0 控制器
上，所以 MCP2515 节点的 interrupt-parent 属性就指向 gpio0。 由于GPIO 11 在 
GPIO0 控制器中的偏移为 11，因此 MCP2515 的 interrupts 属性的第一个 cells 的值
为 11. 同理如果使用 GPIO 12 作为中断，那么 interrupts 属性的第一个 cells 就要
设置为 12. 至于 MCP2515 节点 interrupts 属性的第二个 cells，可根据实际驱动需要
的电压或触发方式进行配置，其值可以为 1， 2， 4， 8. 最终 MCP2515 节点配置如下：

{% highlight ruby %}
spi@10600 {
    mcp2515can: can@2 {
        compatible = "microchip,mcp2515";
        status = "okay";
        reg = <2>;
        spi-max-frequency = <10000000>;
        interrupt-parent = <&gpio0>;
        interrupts = <11 2>;
    };
}；
{% endhighlight %}

通过上面的配置，SPI 总线在遍历总线上的设备时候，会自动检测中断的有效性。

**NOTE**

如果在上面配置之后，如果实际驱动注册中断的时候，抛出如下错误：

{% highlight ruby %}
genirq: Setting trigger mode 4 for irq 59 failed (mvebu_gpio_irq_set_type+0x0/0xf0)
{% endhighlight %}

这个错误说明：在中断检测时，该中断对应的引脚被设置为 output 模式，这样会导致中
断注册失败。对于这个问题，开发者有两个方法解决：

>1）找出让改引脚变为 output 模式的源码，进行修改

>2）手动在源码中添加代码，调用 gpio_direction_input() 函数设置为 input模式。

## 附录

**Interrupt dts:** 

>Kernel/Documentation/devicetree/bindings/interrupt-controller/interrupts.txt
