---
layout: post
title:  "CMA 布局策略"
date:   2019-12-20 09:23:30 +0800
categories: [HW]
excerpt: CMA Memory.
tags:
  - [CMA]
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [CMA 布局简介](#A0)
>
> - [CMA 内核部署](#C0)
>
>   - [CMA 内核宏详解](#C01)
>
>   - [CMA 方案配置](#C0)
>
>     - [CMA 方案配置之 DTS](#C02)
>
>     - [CMA 方案配置之 CMDLINE](#C03)
>
>     - [CMA 方案配置之 Kbuild](#C04)
>
> - [CMA 布局研究](#E0)
>
> - [CMA 布局实践](#B0)
>
> - [附录](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### CMA 布局简介

![](/assets/PDB/HK/HK000169.png)

随着科技不断的迭代，视频编解码、AI 等热门应用不断应用到 linux 中，
这类应用有一个共同特点就是需要快速的数据交互过程。Linux 提供了用户
程序直接访问物理内存的办法，让这类应用能够快速的在物理内存上读写
数据。由于业务需求不断扩张，应用程序对连续物理内存的需求越来越大，
与此同时，Linux 推出了 CMA 内存分配器，开发者可以使用 CMA 内存
分配器分配大块连续物理内存。理想是美好的，但现实是残酷的，由于
物理内存是有限的，开发者不可能将 CMA 物理内存无线扩大，又由于
CMA 基于 buddy 构建，Linux 内核既要确保本身预留合适的物理内存
空间，也要满足 CMA 的分配需求。因此 CMA 布局对系统工程师来说是
不小的挑战。本文重点介绍如何在一个平台上布局 CMA。更多 CMA
描述请参考:

> - [CMA 分配器详解](/blog/CMA/#A0)

----------------------------------

<span id="C0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

#### CMA 内核部署

> - [CMA 内核宏详解](#C01)
>
> - [CMA 方案配置](#C0)
>
>   - [CMA 方案配置之 DTS](#C02)
>
>   - [CMA 方案配置之 CMDLINE](#C03)
>
>   - [CMA 方案配置之 Kbuild](#C04)

-------------------------------------

<span id="C01"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

###### CMA 内核宏详解

为了在内核中使用 CMA，需要在内核中启用和配置对应的 CMA 选项，
开发者可以参考 CMA 相关的宏:

> - [CONFIG_CMA](#C010)
>
> - [CONFIG_CMA_AREAS](#C011)
>
> - [CONFIG_DMA_CMA](#C012)
>
> - [CONFIG_CMA_SIZE_MBYTES](#C013)
>
> - [CONFIG_CMA_SIZE_SEL_MBYTES](#C014)
>
> - [CONFIG_CMA_SIZE_SEL_PERCENTAGE](#C015)
>
> - [CONFIG_CMA_SIZE_PERCENTAGE](#C016)
>
> - [CONFIG_CMA_SIZE_SEL_MIN](#C017)
>
> - [CONFIG_CMA_SIZE_SEL_MAX](#C018)
>
> - [CONFIG_CMA_ALIGNMENT](#C019)

--------------------------------------

###### <span id="C010">CONFIG_CMA</span>

![](/assets/PDB/HK/HK000082.png)

在内核配置时，将该宏设置为 "Y" 以此在内核运行过程中启用 CMA 内存
分配器.

------------------------------------------------

###### <span id="C011">CONFIG_CMA_AREAS</span>

![](/assets/PDB/HK/HK000083.png)

该宏用于设置 CMA 支持的区域数。CMA分配器支持多块相互隔离的 CMA
区域，以便指定驱动使用特定的 CMA 区块。默认值为 7.

----------------------------------------------

###### <span id="C012">CONFIG_DMA_CMA</span>

![](/assets/PDB/HK/HK000085.png)

该宏用于启用 DMA 连续物理内存分配器，其分配的物理内存也是 CMA
内存。

----------------------------------------------

###### <span id="C013">CONFIG_CMA_SIZE_MBYTES</span>

![](/assets/PDB/HK/HK000084.png)

该宏与 CONFIG_CMA_SIZE_SEL_MBYTES 宏配合使用，但内核启用 
CONFIG_CMA_SIZE_SEL_MBYTES 宏之后，可以通过配置
CONFIG_CMA_SIZE_MBYTES 的值来设置 CMA 的大小。

---------------------------------------

###### <span id="C014">CONFIG_CMA_SIZE_SEL_MBYTES</span>

![](/assets/PDB/HK/HK000086.png)

该宏与 CONFIG_CMA_SIZE_MBYTES 宏配合使用，该宏用于指明 CONFIG_CMA_SIZE_MBYTES
的粒度大小为 MBytes。当启用该宏之后，内核通过配置 CONFIG_CMA_SIZE_MBYTES 的
大小来设置 CMA 的大小.

---------------------------------------

###### <span id="C015">CONFIG_CMA_SIZE_SEL_PERCENTAGE</span>

![](/assets/PDB/HK/HK000087.png)

该宏与 CONFIG_CMA_SIZE_PERCENTAGE 配合使用，用于配置 CMA 占物理内存的百
分比。当内核启用 CONFIG_CMA_SIZE_SEL_PERCENTAGE 宏之后，就可以通过是
配置 CONFIG_CMA_SIZE_PERCENTAGE 百分比来设置 CMA 的大小。

--------------------------------------

###### <span id="C016">CONFIG_CMA_SIZE_PERCENTAGE</span>

![](/assets/PDB/HK/HK000088.png)

该宏与 CONFIG_CMA_SIZE_SEL_PERCENTAGE 配合使用，用于设置 CMA 占
物理内核的百分比。当内核启用 CONFIG_CMA_SIZE_SEL_PERCENTAGE 宏之后，
内核就可以通过配置 CONFIG_CMA_SIZE_PERCENTAGE 的百分比来设置 CMA
物理内存的大小。

--------------------------------------

###### <span id="C017">CONFIG_CMA_SIZE_SEL_MIN</span>

![](/assets/PDB/HK/HK000089.png)

该宏与 CONFIG_CMA_SIZE_MBYTES、CONFIG_CMA_SIZE_PERCENTAGE 配合使用，
内核可以同时通过 CONFIG_CMA_SIZE_MBYTES 配置指定大小的 CMA，又可用
通过 CONFIG_CMA_SIZE_PERCENTAGE 百分比的方式配置 CMA 大小，此时系统
根据 CMA_SIZE_SEL_MIN 宏选用两者中最小的配置为最终 CMA 的大小.

--------------------------------------

###### <span id="C018">CONFIG_CMA_SIZE_SEL_MAX</span>

![](/assets/PDB/HK/HK000090.png)

该宏与 CONFIG_CMA_SIZE_MBYTES、CONFIG_CMA_SIZE_PERCENTAGE 配合使用，
内核可以同时通过 CONFIG_CMA_SIZE_MBYTES 配置指定大小的 CMA，又可用
通过 CONFIG_CMA_SIZE_PERCENTAGE 百分比的方式配置 CMA 大小，此时系统
根据 CMA_SIZE_SEL_MAX 宏选用两者中最大的配置为最终 CMA 的大小.

--------------------------------------

###### <span id="C019">CONFIG_CMA_ALIGNMENT</span>

![](/assets/PDB/HK/HK000091.png)

该宏用于 DMA 方式申请的连续物理内存的对齐方式。

-------------------------------------

<span id="C02"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

###### CMA 方案配置之 DTS

> - [方法简介](#C020)
>
> - [方法实践](#C021)
>
> - [方案优缺点](#C022)

------------------------------

###### <span id="C020">方法简介</span>

在支持 DTS 平台，内核支持从 DTS 中获得 CMA 分配器配置信息。内核同时支持
多种方案配置 CMA，DTS 方案优先级最高。如果内核同时开启了多种 CMA 配置
方案，内核优先采用 DTS 方案而忽略其他方案。为了实现 DTS 方案，内核首先
需要支持 DTS，因此下列宏在内核配置阶段必须开启:

{% highlight bash %}
CONFIG_DTC=y
CONFIG_OF=y
CONFIG_OF_RESERVED_MEM=y
CONFIG_OF_FLATTREE=y
CONFIG_OF_EARLY_FLATTREE=y
CONFIG_OF_KOBJ=y
CONFIG_OF_ADDRESS=y
{% endhighlight %}

![](/assets/PDB/HK/HK000155.png)

CMA 相关的配置位于 DTS 的 "/reserved-memory" 节点内，如果需要向系统
中添加一个 CMA 区域，可以在该节点中创建一个子节点进行描述，子节点的
属性有一定的要求，目前支持两种模板:

{% highlight bash %}
                linux,cma {
                        compatible = "shared-dma-pool";
                        reusable;
                        size = <0x00800000>; /* 8M */
                        alloc-ranges = <0x69000000 0x00800000>;
                        linux,cma-default;
                };
{% endhighlight %}

第一种模板如上: 1) 节点的名字内核会解析为 CMA 区域的名字，上例中，节点
的名字为 "linux,cma", 那么该 CMA 区域的名字就是 "linux,cma".
2) compatible 属性的属性值必须为 "shared-dma-pool". 3) 节点必须包含
"reusable" 属性而不应该包含 "no-map" 属性，"reusable" 属性用于说明
预留区的页是可以映射建立页表的，而 "no-map" 属性则表示预留区的页不
建立页表，不建立页表的结果就是虚拟内存无法映射这段物理内存。4) 节点
包含了 size 属性之后就不能包含 reg 属性，size 属性用于指明 CMA 区域
的长度，size 使用的 cells 数应该从 "/reserved-memory" 的 "#size-cells"
获得，如果 "/reserved-memory" 的 "#size-cells" 为 1，那么使用一个 32
位整数就能表示, 正如上面的例子，分配 8M，那么 size 属性的属性值就是
"0x00800000"；如果 "#size-celss" 为 2，那么使用两个 32 位整数才能
表示，同样要分配 8MB 的物理内存，size 属性的属性值就是 
"0x0 0x00800000". 5) alloc-ranges 属性用于指明内核分配连续物理内存
的范围，其由基地址和长度两个部分组成，正如上例表示，内核会从
0x69000000 初开始，长度为 0x00800000 的空间内找到一块大小为 size
属性对应的连续物理内存。这里同样要注意，地址部分需要使用多少个 32
位整数表示依赖于 "/reserved-memory" 的 "#address-cells" 的值决定。
同理长度部分的使用的 32 位整数也依赖于 "/reserved-memory" 的
"#size-cells" 的值决定。6) 如果要使用节点对应的区域作为 CMA 分配器
默认使用的区域，那么必须包含 "linux,cma-default" 属性，否则可能
该区域不会被作为 CMA 分配器默认使用的区域。

{% highlight bash %}
                BiscuitOS_CMA: cma@69800000 {
                        compatible = "shared-dma-pool";
                        reg = <0x69800000 0x00800000>; /* 8M */
                        reusable;
			linux,cma-default;
                };
{% endhighlight %}

第二种模板如上: 1) 节点的名字就是 CMA 区域的名字，例如上面节点的名字
是 BiscuitOS_CMA, CMA 区域的名字就是 BiscuitOS_CMA. 第二种模板中，
必须使用 name@address 部分指明该节点的名字，其中 name 部分名为 "cma",
address 的值为节点 reg 属性的属性值，并且用 16 进制表示。2) 
"compatible" 属性的属性值必须是 "shared-dma-pool". 3) 模板二中必须
包含 reg 属性，reg 属性包含地址和长度两部分，地址部分的长度通过
"/reserved-memory" 的 "#address-cells" 属性值决定. 长度部分的长度通
过 "/reserved-memory" 的 "#size-cells" 属性值决定。例如上例子中，
地址部分为 0x69800000, 长度部分为 0x00800000，内核解析这部分属性
之后，内核会从 0x69800000 开始查找长度为 0x00800000 的连续物理内存。
4) 节点必须包含 "reusable" 属性而不应该包含 "no-map" 属性，"reusable"
属性用于说明预留区的页是可以映射建立页表的，而 "no-map" 属性则表示预
留区的页不建立页表，不建立页表的结果就是虚拟看见物理使用这段物理内存。
5) 如果要使用节点对应的区域作为 CMA 分配器默认使用的区域，那么必须包
含 "linux,cma-default" 属性，否则可能该区域不会被作为 CMA 分配器默认
使用的区域。

至此内核就可以通过 DTS 方案配置 CMA 分配器，DTS 方案源码初始化解析
请参考下文:

![](/assets/PDB/HK/HK000149.png)

> - [CMA 方案配置之 DTS 源码解析](/blog/CMA/#E0111)

<span id="C022"></span>

###### DTS 方案优点

DTS 方案是最灵活的，同时支持名字、起始地址、长度和终止地址的配置，以及
对齐方式。最关键的是 DTS 可以配置多个 CMA 区域。

###### DTS 方案缺点

DTS 方案的缺点就是在不支持的 DTS 的平台无法使用该方案.

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

------------------------------

###### <span id="C021">方法实践</span>

本例基于 BiscuitOS linux 5.0 进行实践，BiscuitOS linux 5.0 开发环境
部署请参考下文:

> - [BiscuitOS linux 5.0 arm32 开发环境部署](/blog/Linux-5.0-arm32-Usermanual/)

准备好开发环境之后，开发者可以参考下列步骤实践 DTS 方案:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
vi arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

![](/assets/PDB/HK/HK000155.png)

如上图，在 "/reserved-memory" 中添加了两个节点 "linux,cma" 和 
"BiscuitOS_CMA", 其大小都是 8MB。其中 CMA 分配器默认使用 "linux,cma"
分区。接着重新编译内核，并运行 BiscuitOS, 参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- -j4
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000156.png)

内核启动过程中，输出 CMA 区域初始化信息，从上图 log 可以看出，内核
一共新建了两个 CMA 区域，长度都是 8MB。

![](/assets/PDB/HK/HK000157.png)

BiscuitOS 运行之后，查看 "/proc/meminfo" CMA 统计信息，上图中可以看出
CMA 总共 16384KB，即 16MB，目前系统可用的 CMA 物理内存为 14592KB. 由
内核策略可以知道，CMA 的总数也为 "MemTotal" 的一部分。

-------------------------------------

<span id="C03"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

###### CMA 方案配置之 CMDLINE

> - [方法简介](#C030)
>
> - [方法实践](#C031)
>
> - [方案优缺点](#C032)

------------------------------

###### <span id="C030">方法简介</span>

内核支持通过 CMDLINE 的方式配置 CMA 分配器，内核可以通过 Kbuid
传入 CMDLINE 参数也可以通过 uboot 传入 CMDLINE 参数。由于内核同时
支持多种方案配置 CMA 内存分配器，所有内核对这些方案有不同的优先级，
CMDLINE 方案的优先级仅仅低于 DTS，因此当系统没有使用 DTS 方案配置
CMA 的话，如果此时 CMDLINE 方案存在，那么内核就采用 CMDLINE 方案
配置 CMA 分配器。CMDLINE 方案中，配置 CMA 的格式如下:

{% highlight c %}
CMDLINE="... cma=8M"
CMDLINE="... cma=8M@0x69000000"
CMDLINE="... cma=8M@0x69000000-0x69800000"
{% endhighlight %}

CMDLINE 中支持使用关键字 "cma=" 来指定 CMA 分区的长度，如果长度之后
紧跟 "@" 符合和一个地址，那么代表 CMA 区域的起始地址。如果其后还紧跟
"-" 符号和一个地址，那么代表 CMA 区域可达到的最大地址。内核会在
起始地址到终止地址范围内从末端找到符合要求的连续物理内存，从末端开始
找的特点是由于此阶段内核使用 MEMBLOCK 分配器进行分配，因此这里的行为
与 MEMBLOCK 有关，也就是说如果此时 MEMBLOCK 采用从低端到顶端分配的策略，
那么内核会从指定范围的低部开始向高端方向查找符合要求的内存区块。
具体的说明可以参考:

> - [linux/Documentation/admin-guide/kernel-parameters.txt](https://elixir.bootlin.com/linux/v5.0/source/Documentation/admin-guide/kernel-parameters.txt)

CMDLINE 方案内核初始化源码分析请参考下列文档:

![](/assets/PDB/HK/HK000151.png)

> - [CMA 方案配置之 CMDLINE 源码解析](/blog/CMA/#E0112)

<span id="C032"></span>

###### CMDLINE 方案优点

简单便捷创建一个新的 CMA 区域，既可以通过 uboot 传递参数，也可以通过
内核配置的 CMDLINE 传递参数，除了不能设置 CMA 区域的名字，CMDLINE 方法
便捷设置 CMA 区域的起始地址，长度和最大可用地址。

###### CMDLINE 方案缺点

不能设置 CMA 区域的名字，只能创建一个 CMA 区域。不能设置 CMA 区域的对齐
方式。优先级低于 DTS 方案。

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

------------------------------

###### <span id="C031">方法实践</span>

本例基于 BiscuitOS linux 5.0 进行实践，BiscuitOS linux 5.0 开发环境
部署请参考下文:

> - [BiscuitOS linux 5.0 arm32 开发环境部署](/blog/Linux-5.0-arm32-Usermanual/)

准备好开发环境之后，本次实践模拟 uboot 传递 CMDLINE 方式配置 CMA。
开发者首先向 uboot 中添加传入的 CMDLINE 参数，以便添加 cma 配置，
在 BiscuitOS 中，uboot 的 CMDLINE 参数位于 RunBiscuitOS.sh 中，请
参考下面步骤进行实践:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32
vi RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000158.png)

在 RunBiscuitOS.sh 文件中，找到 CMDLINE 变量，向其添加参数
"cma=4M@0x69000000-0x69800000", 这将模拟 uboot 传递 CMDLINE
给内核，其参数函数是需要创建一个长度为 4M，起始地址为 0x690000000,
最大可用物理地址是 0x69800000 的区域内分配连续的物理内存作为
一块新的 cma 区域。添加完毕之后运行 BiscuitOS, 参考下列命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000159.png)

从上图可以看到系统从 0x69400000 处开始分配一个 4M 的物理内存，
终止地址是 0x69800000. 从实践可以知道系统会在 0x69000000 到 
0x69800000 的区域内按 4M 对齐的方式，从末端开始找一块符合要求
的区域，该性质由 MEMBLOCK 从高向低分配的特点决定。

![](/assets/PDB/HK/HK000160.png)

BiscuitOS 运行之后，可以通过查看 "/proc/meminfo" 获得 CMA 分配器区域
的信息，如上图系统 CMA 总内存为 4M, 当前可用的 CMA 内存为 2304KB.

-------------------------------------

<span id="C04"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

###### CMA 方案配置之 Kbuild

> - [方法简介](#C040)
>
> - [方法实践](#C041)
>
> - [方案优缺点](#C042)

------------------------------

###### <span id="C040">方法简介</span>

内核支持通过 Kbuild 来配置 CMA 分配器区域，所以 Kbuild 方案就是使用
内核配置的方法，通过宏来描述 CMA 区域的信息，内核在初始化阶段读取这些
信息构建符合要求的 CMA 区域。由于内核同时支持多种配置 CMA 的方案，但
只有一种方案有效，因此内核按优先级选用 CMA 配置方案。Kbuild 方案是
优先级最低的，基本可以理解为系统默认的方案，一旦有其他方案，Kbuild
方案就会被启用，因此如果要使用 Kbuild 方案，那么其他方案就不能使用。
与 Kbuild 配置相关的宏如下，宏具体的描述可以查看下列文章:

> - [CONFIG_CMA_AREAS 配置 CMA 分配器区域数](#C011)
>
> - [CONFIG_CMA_SIZE_MBYTES 配置 CMA 的大小](#C013)
>
> - [CONFIG_CMA_SIZE_SEL_MBYTES 按 MB 配置 CMA 区域大小](#C014)
>
> - [CONFIG_CMA_SIZE_SEL_PERCENTAGE 按物理内存的百分比配置 CMA 区域大小](#C015)
>
> - [CONFIG_CMA_SIZE_PERCENTAGE](#C016)
>
> - [CONFIG_CMA_SIZE_SEL_MIN 优先选择最小的 CMA 体积](#C017)
>
> - [CONFIG_CMA_SIZE_SEL_MAX 优先选择最大的 CMA 体积](#C018)
>
> - [CONFIG_CMA_ALIGNMENT CMA 对齐大小](#C019)

Kbuild 通过设置 CMA 大小来配置 CMA，Kbuild 为了提供不同的使用场景，提供
了 4 种配置逻辑: 1) 按 MB 直接配置. 2) 按可用物理内存的百分比进行配置. 
3) 按最小体积进行配置. 5) 按最大体积进行配置。无论使用那种配置，内核
在启动过程中，内核都会通过解析相关宏获得一个 CMA 长度，那么系统就
从 MEMBLOCK 中分配指定长度的区域给 CMA。Kbuild 方案源码分析参考如下:

![](/assets/PDB/HK/HK000152.png)

> - [CMA 配置方案之 Kbuild 源码分析](/blog/CMA/#E0113)

<span id="C042"></span>

###### Kbuild 方案优点

提供了内核基础 CMA 配置方法，并提供了几种配置方法使用不同的使用场景，
简单便捷。

###### Kbuild 方案缺点

不能指定 CMA 区域的起始地址和终止地址。如果存在其他方案，该方案
会被系统自动弃用。不能配置 CMA 区域的名字。只能创建一个 CMA 区域。
Kbuild 方案优先级最低，容易被其他方案取代。

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

------------------------------

###### <span id="C041">方法实践</span>

本例基于 BiscuitOS linux 5.0 进行实践，BiscuitOS linux 5.0 开发环境
部署请参考下文:

> - [BiscuitOS linux 5.0 arm32 开发环境部署](/blog/Linux-5.0-arm32-Usermanual/)

准备好开发环境之后，开发者可以参考下列步骤实践 Kbuild 方案:

{% highlight c %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make menuconfig ARCH=arm
{% endhighlight %}

![](/assets/PDB/HK/HK000093.png)

选择 "Memory Management options --->" 并进入下一级，

![](/assets/PDB/HK/HK000094.png)

选择 "Contiguous Memory Allocator", 并确保 "Maximum count of the CMA areas"
为 7. 返回到上一级。

![](/assets/PDB/HK/HK000095.png)

选择 "Device Drivers" 并进入下一级菜单。

![](/assets/PDB/HK/HK000096.png)

选择 "Generic Driver Options" 并进入下一级菜单。

![](/assets/PDB/HK/HK000097.png)

选择 "DMA Contiguous Memory Allocator", 接下来优先选择 
"Selected region size" 选项。

![](/assets/PDB/HK/HK000098.png)

此处开发者可以根据需求进行选择，如果选择 "Use mega bytes value only",
那么可以按 MBytes 配置 CMA; 如果选择 "Use percentage value only"
那么按百分比进行 CMA 配置; 如果选择 "Use lower value (minimum)"
那么选择最小的 CMA 配置; 如果选择 "Use higher value (maximum)",
那么选择最大的 CMA 配置。根据配置信息请参考下面内容:

> - [CMA 内核配置宏](#C01)

例如选择 "Use mega bytes value only" 之后配置 CMA 为 128MBytes，如
下图:

![](/assets/PDB/HK/HK000099.png)

配置完毕之后保存退出，并重新编译内核和运行 BiscuitOS，开发者
可以参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- -j4
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000161.png)

BiscuitOS 启动过程中，系统 log 中可以看到内核根据 Kbuild 配置信息
创建了长度为 128MB 的 CMA 区域。

![](/assets/PDB/HK/HK000162.png)

#BiscuitOS 运行之后，通过查看 "/proc/meminfo" 查看 CMA 的信息，如上图，
CMA 总内存数是 131072KB, 当前可用 CMA 为 129280KB.

----------------------------------

<span id="E0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Z.jpg)

#### CMA 布局策略

CMA 分配器用于分配连续的物理内存，CMA 问题的本质就是如何规划系统的
物理内存。CMA 分配器从 CMA 区域中分配连续的物理内存，从上面的章节
已经知道如何在内核中配置并添加一块新的 CMA 区域。本节将重点研究一个
问题: CMA 设置多大才合理 ? 这回到了 CMA 的本质，这个问题就是问如何
规划系统的连续物理内存。本节就详细介绍 CMA 连续物理内存规划的原理。 

###### 系统物理内存

第一个比较重要的是获得系统物理内存的范围，只有知道系统 RAM 的布局
之后，才可以规划 CMA 区域。系统 RAM 信息可以通过 "/proc/iomem" 获得，
开发者可以参考下面命令:

{% highlight bash %}
cat /proc/iomem
{% endhighlight %}

![](/assets/PDB/HK/HK000163.png)

执行上面的命令之后，可以看到系统物理地址占用情况，其中找到 "System RAM",
其代表系统物理内存的起始物理地址和终止物理地址。有的系统包含两块系统 RAM，
从上图看出系统 RAM 的地址范围是: "60000000-9fffffff" 长度为 1G。

###### MEMBLOCK 可用物理内存

第二个比较重要的依据是系统申请物理内存并创建 CMA 区域处理 MEMBLCOK
分配器管理阶段，该阶段的一个特点就是 MEMBLOCK 将系统物理内存分作
三类，第一类是可用物理内存，也就是当时系统某个部分需要物理内存，可以
通过 MEMBLOCK 提供的接口进行分配。第二类是预留内存，这部分物理内存
给特定功能使用，比如用于存储内核的代码和数据段，存储一些重要数据逻辑
等。第三类就是系统不希望虚拟空间看到的物理内存，这部分物理内存未来
将不能建立页表，虚拟地址无法访问。内核启动到该阶段之后，从不同的
策略获得 CMA 区域的长度等信息之后，就会向 MEMBLOCK 分配器从可用
物理内存中分配指定长度的物理内存，分配成功之后，MEMBLOCK 会将这部分
物理内存加入到 MEMBLOCK 的预留区，因此此刻 MEMBLOCK 内存分配可用
物理内存区的使用情况对 CMA 区域的布局起到最关键作用，因此根据
这个重要信息，开发者可以查看系统 MEMBLOCK 分配器预留区的使用情况
来规划 CMA 区域。开发者可以使用如下命令查看 MEMBLOCK 预留区的使用
情况:

{% highlight bash %}
cat /sys/kernel/debug/memblock/reserved
{% endhighlight %}

![](/assets/PDB/HK/HK000164.png)

从上一条可以知道，系统 RAM 的范围是 "60000000-9fffffff", 从上图获得
的信息可以知道，在系统物理内存中 "0x60004000-0x6800bfd9" 以及
"0x9f6e3000-9fffffff" 基本被占用，再根据 CMA 连续物理内存对齐的要求，
因此可用的物理内存范围是: "0x68100000-0x9f600000"，总计 885MB。因此
现在可知 CMA 区域最大长度为 885MB.

###### 长度，基地址，名字

CMA 分配器可以设置 CMA 区域的起始基地址，名字和长度。综合上面两点，
最后开发者可以使用下列方案进行 CMA 配置.

如果要设置 CMA 区域的起始地址和名字以及长度，只能通过 DTS 方案进行
部署，具体实现过程请查看下列文章:

> - [CMA 方案配置之 DTS](#C02)

如果要设置起始地址以及长度，可以通过 DTS 和 CMALINE 方案进行部署，
具体实现过程请看下列文章:

> - [CMA 方案配置之 DTS](#C02)
>
> - [CMA 方案配置之 CMDLINE](#C03)

如果只设置 CMA 区域的长度，三种方案都可以进行部署，具体实现过程
请看下列文章:

> - [CMA 方案配置之 DTS](#C02)
>
> - [CMA 方案配置之 CMDLINE](#C03)
>
> - [CMA 方案配置之 Kbuild](#C04)

----------------------------------

<span id="B0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

#### CMA 布局实践

> - [实践部署准备](#B00)
>
> - [CMA 区域配置](#B01)
>
> - [CMA 运行查看](#B02)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="B00">实践部署准备</span>

本例基于 BiscuitOS linux 5.0 进行实践，BiscuitOS linux 5.0 开发环境
部署请参考下文:

> - [BiscuitOS linux 5.0 arm32 开发环境部署](/blog/Linux-5.0-arm32-Usermanual/)

在布局 CMA 之前，需要先运行一下 BiscuitOS 获得 RAM 信息和 MEMBLOCK
分配器预留区信息。使用下面命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
cat /proc/iomem
{% endhighlight %}

![](/assets/PDB/HK/HK000163.png)

执行上面的命令之后，可以看到系统物理地址占用情况，其中找到 "System RAM",
其代表系统物理内存的起始物理地址和终止物理地址。有的系统包含两块系统 RAM，
从上图看出系统 RAM 的地址范围是: "60000000-9fffffff" 长度为 1G。因此
只要 CMA 区域在这个范围内都是可行的。接着执行下面命令:

{% highlight bash %}
cat /sys/kernel/debug/memblock/reserved
{% endhighlight %}

![](/assets/PDB/HK/HK000164.png)

从上一条可以知道，系统 RAM 的范围是 "60000000-9fffffff", 从上图获得
的信息可以知道，在系统物理内存中 "0x60004000-0x6800bfd9" 以及
"0x9f6e3000-9fffffff" 基本被占用，再根据 CMA 连续物理内存对齐的要求，
因此可用的物理内存范围是: "0x68100000-0x9f600000"，总计 885MB。因此
现在可知 CMA 区域最大长度为 885MB.

因此结合上面两个信息，决定从 0x69000000 开始分配长度为 864M 的空间，
这里值得注意长度必须按 8MB 对齐。这里对结束地址有做要求，因此结束地
址正好是 0x9F000000, 并且需要将这块 CMA 区域的名字设置为 
"BiscuitOS_CMA", 因此采用 DTS 方案。

---------------------------------------

###### <span id="B01">CMA 区域配置</span>

结合上面的分析采用了 DTS 方案，因此使用如下命令在 DTS 中进行配置:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
vi arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

![](/assets/PDB/HK/HK000165.png)

在 "/reserved-memory" 节点下创建了 "BiscuitOS_CMA" 子节点，
采用 reg 属性的节点配置，由于 "/reserved-memory" 节点的 
"#address-cells" 属性值为 1， 因此这里将 reg 属性地址域设置为
0x69000000，同理 "/reserved-memory" 节点的 "#size-cells" 
属性值为 1，因此 reg 属性的长度域设置为 0x36000000, 切记
长度域的值一定要按 8MB 进行对齐。接着需要添加 "reusable" 
属性，由于只有一个 CMA 域，因此不用添加 "linux,cma-default"
的前提下，CMA 分配器默认使用这个区域分配连续物理内存。
DTS 配置完毕之后，重新编译内核并运行 BiscuitOS，参考下面命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- -j4
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

---------------------------------------

###### <span id="B02">CMA 运行查看</span>

BiscuitOS 启动时可以从 log 中看到 CMA 区域初始化信息，如下:

![](/assets/PDB/HK/HK000166.png)

系统从 0x69000000 开始，分配了长度为 864MiB 的物理内存作为 CMA 区域。

![](/assets/PDB/HK/HK000167.png)

BiscuitOS 运行之后，可以查看 "/proc/meminfo" 获得 CMA 的信息，如
上图所示，系统 CMA 连续物理内存总数为 884736KB, 当前可用 CMA 连续
物理内存为 884736KB。

![](/assets/PDB/HK/HK000168.png)

从上图可以看出 0x69000000 到 0x9effffff 已经被加入到 MEMBLOCK 预留区内。
通过上面的数据可以获得 CMA 分配器的新 CMA 区域已经布局成功。

-----------------------------------------------

# <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 赞赏一下吧 🙂

![](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
