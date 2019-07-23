---
layout: post
title:  "DTS"
date:   2019-07-22 05:30:30 +0800
categories: [HW]
excerpt: DTS.
tags:
  - DTS
---

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

> [Github: BBBXXX](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/API/BBBXXX)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------


> - [DTS 简介](#A001)
>
> - [ARM 引入 DTS 原因](#A002)
>
> - []


### <span id="A001">DTS 简介</span>

DTS 是为 Linux 提供一种硬件信息的描述方法，以此代替源码中的
硬件编码 (hard code)。DTS 即 Device Tree Source 设备树源码，
Device Tree 是一种描述硬件的数据结构，起源于 OpenFirmware (OF).
在 Linux 2.6 中， ARM 架构的板级硬件细节过多的被硬编码在 arch/arm/plat-xxx
和 arch/arm/mach-xxx (比如板上的 platform 设备，resource，
i2c_board_info, spi_board_info 以及各种硬件的 platform_data）,
这些板级细节代码对内核来讲只不过是垃圾代码。而采用 Device Tree 后，
许多硬件的细节可以直接透过它传递给 Linux，而不再需要在 kernel 中
进行大量的冗余编码。

### <span id="A002">ARM 引入 DTS 的原因</span>

每次正式发布 linux kernel release 之后都会有两周的 merge window，
在这个窗口期间，kernel 各个子系统的维护者都会提交各自的 patch，将测
试稳定的代码请求并入 kernel mainline。每到这个时候，**Linus** 就会比较
繁忙，他需要从各个子系统维护者的分支上获得最新的代码并 merge 到自己的
kernel source tree 中。其中有一个维护者 Tony Lindgren，
OMAP development tree 的维护者，发送了一个邮件给 Linus，请求提交
OMAP 平台代码修改，并给出一些细节描述：

{% highlight bash %}
1. 简单介绍本次修改
2. 关于如何解决 merge conficts
{% endhighlight %}

一切都很正常，也给出足够的信息，然而，这好是这个 pull request
引起了一场针对 ARM Linux 内核代码的争论。也许 Linus 早就对 ARM
相关的代码早就不爽了，ARM 的 merge 工作量不仅较大，而且他认为 ARM
很多的代码都是垃圾，代码里有很多愚蠢的 table，从而导致了冲突。因此，
在处理完 OMAP 的 pull request 之后 （Linus 并非针对 OMAP 平台，
只是 Tony Lindgren 撞在枪口上了），Linus 在邮件中写道：

{% highlight bash %}
Gaah.Guys, this whole ARM thing is a f*ching pain in the ass.
{% endhighlight %}

这件事之后， ARM 社区对 ARM 平台相关的 code 做出了如下规范调整，
这个也正是引入 DTS 的原因，如下：

{% highlight bash %}
1. ARM 的核心代码仍然存储在 arch/arm 目录下

2. ARM Soc core architecture code 存储在 arch/arm 目录下

3. ARM Soc 的周边外设模块驱动存储在 drivers 目录下

4. ARM Soc 的特定代码在 arch/arm/mach-xxx 目录下

5. ARM Soc board specific 代码被移除，由 DeviceTree 机制负责传递硬件拓展和硬件信息
{% endhighlight %}

本质上，DeviceTree 改变原来用 hardcode 方式将 HW 配置信息
嵌入到内核代码的方法，改用 bootloader 传递一个 DB 的形式。
对于嵌入式系统，在系统启动阶段，bootloader 会加载内核并将控
制权转移给内核。

更多 DTS 内核历史邮件请查看如下：

> - [讨论引入 FDT 到嵌入式 linux 平台](#A010)
>
> - [Russell King 反对 DTS 加入到 ARM](#A011)
>
> - [Russell 最终被说服接受 DTS 进入 ARM](#A013)
>
> - [David Gibson 辩护 FDT](#A014)

-----------------------------------------------

## DTB 标准结构设计

DTB 是一个二进制文件，由 dtc 工具将 dts 文件转换而来，用于存储板子硬件
描述信息。板子上电之后，由 u-boot 传递给内核。

##### DTB 文件的作用

{% highlight bash %}
1. 使用减少了内核版本数。比如同一块板子，在外设不同的情况下，使用
   dtb 文件需要编译多个版本的内核。当使用 dtb 文件时同一份 linux
   内核代码可以在多个板子上运行，每个板子可以使用自己的 dtb 文件。

2. 嵌入式中 ，linux 内核启动过程中只要解析 dtb 文件，就能加载对应的模块。

3. 编译 linux 内核时必须选择对应的外设模块，并且 dtb 中包括外设的信息，
   在 linux 内核启动过程中才能自动加载该模块
{% endhighlight %}

### DTB 二进制文件架构

DTB 二进制文件采用一定的标准进行设计规划，开发者只要在程序
中根据 DTB 的标准，就能从 DTB 中获得所需的信息。DTB 的架构
如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000213.png)

DTB 分作如下几个部分：

> - [boot_param_header](#B010)
>
> - [memory reserve map](#B011)
>
> - [device-tree structure](#B012)
>
> - [device-tree strings](#B013)

-------------------------------------------------------

##### <span id="B010">boot_param_header</span>

boot_param_header 部分称为 DTB 的头部，该部分包含了 DTB
二进制的基本信息，其在内核中定义如下：

{% highlight bash %}
/*
 * This is what gets passed to the kernel by prom_init or kexec
 *
 * The dt struct contains the device tree structure, full pathes and
 * property contents. The dt strings contain a separate block with just
 * the strings for the property names, and is fully page aligned and
 * self contained in a page, so that it can be kept around by the kernel,
 * each property name appears only once in this page (cheap compression)
 *
 * the mem_rsvmap contains a map of reserved ranges of physical memory,
 * passing it here instead of in the device-tree itself greatly simplifies
 * the job of everybody. It's just a list of u64 pairs (base/size) that
 * ends when size is 0
 */
struct boot_param_header {
        __be32  magic;                  /* magic word OF_DT_HEADER */
        __be32  totalsize;              /* total size of DT block */
        __be32  off_dt_struct;          /* offset to structure */
        __be32  off_dt_strings;         /* offset to strings */
        __be32  off_mem_rsvmap;         /* offset to memory reserve map */
        __be32  version;                /* format version */
        __be32  last_comp_version;      /* last compatible version */
        /* version 2 fields below */
        __be32  boot_cpuid_phys;        /* Physical CPU id we're booting on */
        /* version 3 fields below */
        __be32  dt_strings_size;        /* size of the DT strings block */
        /* version 17 fields below */
        __be32  dt_struct_size;         /* size of the DT structure block */
};
{% endhighlight %}

开发者可以通过使用二进制工具直接分析一个 DTB 的头部
文件，结合该例子一同分析 DTB 头部的各个成员。例如在 Linux 5.0
上使用一个例子：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/
hexdump vexpress-v2p-ca15_a7.dtb -n 112
{% endhighlight %}

在 Linux 5.0 arm32 体系中，BiscuitOS 使用了 vexpress-v2p-ca15_a7.dtb
作为 DTB，此时使用上面的命令可以获得头文件的二进制内容，如下：

{% highlight bash %}
buddy@BDOS: $ hexdump vexpress-v2p-ca15_a7.dtb -n 112
0000000 0dd0 edfe 0000 4548 0000 3800 0000 6444
0000010 0000 2800 0000 1100 0000 1000 0000 0000
0000020 0000 e103 0000 2c44 0000 0000 0000 0000
0000030 0000 0000 0000 0000 0000 0100 0000 0000
0000040 0000 0300 0000 0d00 0000 0000 3256 2d50
0000050 4143 3531 435f 3741 0000 0000 0000 0300
0000060 0000 0400 0000 0600 0000 4902 0000 0300
{% endhighlight %}

DTB header 包含了 DTB 文件的所有信息，分别代表如下:

--------------------------------------

###### magic

DTB 文件的 MAGIC，用于表明文件属于 DTB，其为 DTB 第一个 4 字节，
由于其定义为 `__be32`, 因此数据采用的大端模式，因此此时 DTB 的
MAGIC 信息为：0xD00DFEED。在 arm/kernel/head-common.S 中有
如下定义：

{% highlight bash %}
#ifdef CONFIG_CPU_BIG_ENDIAN
#define OF_DT_MAGIC 0xd00dfeed
#else
#define OF_DT_MAGIC 0xedfe0dd0 /* 0xd00dfeed in big-endian */
#endif
{% endhighlight %}

因此在采用大端模式的 CPU 中，DTB MAGIC 的宏定义为 0xD00DFEED,
而在小端模式的 CPU 中，DTB MAGIC 的宏定义为 0xEDFE0DD0. 注意！
在 ARM 中 DTB 延续了 POWERPC 的设计，DTB 也采用了大端模式。

---------------------------------------

###### totalsize

标识 DTB 文件的大小, 其为 DTB 第二个 4 字节，由于采用大端模式，
因此 DTB 二进制文件的大小为： 0x00004845. 此时在 Linux 中查看
DTB 文件的大小如下：

{% highlight bash %}
buddy@BDOS:$ ll vexpress-v2p-ca15_a7.dtb
-rw-rw-r-- 1 buddy buddy 18501 7月   8 10:09 vexpress-v2p-ca15_a7.dtb
{% endhighlight %}

-------------------------------------------

###### off_dt_struct

标识 Device-tree structure 在文件中的位置. 其值为 DTB
文件的第三个 4 字节，由于采用大端模式，此时其值为：0x00000038.
解析 DTB 的程序中，只要找到该地址就可以获得 DTS 中节点的起始
地址。

------------------------------------------

###### off_dt_strings

标识 Device-tree strings 在文件中的位置，其值为 DTB
文件的第四个 4 字节，由于采用大端模式，此时其值为：0x00004464.
解析 DTB 的程序中，只要找到该地址就可以获得 DTS 中 string 字符串
存储的起始地址。

---------------------------------

###### off_mem_revmap

标识 reserve memory 在文件中的位置。DTB 二进制文件中也会存在
一段预留的内存区域，其值为 DTB文件的第五个 4 字节，由于采用大
端模式，此时其值为：0x00000028. 解析 DTB 的程序中，只要找到该
地址就可以获得 DTB 二进制文件中的保留内存区。

----------------------------------

###### version

标识 DTB 文件的版本信息。其值为 DTB文件的第六个 4 字节，由于采用大
端模式，此时其值为：0x00000011。

-------------------------------

###### last_comp_version

标识上一个兼容 DTB 版本信息

-------------------------------

###### boot_cpuid_phys

物理 CPU 号

------------------------------

###### dt_strings_size

标识 Device-tree strings 的大小，结合 off_dt_strings 可以
计算出 DTB 文件 device-tree string 的完整信息。

------------------------------

###### dt_struct_size

标识 Device-tree structure 的大小，结合 off_dt_struct 可以
计算出 DTB 文件 device-tree struct 的完整信息。


-------------------------------------------------------

##### <span id="B011">memory reserve map</span>

这个段保存的是一个保留内存映射列表，每个表由一对 64 位的
物理地址和大小组成。其在内核中的定义如下：

{% highlight bash %}
struct fdt_reserve_entry {
    uint64_t address;
    uint64_t size;
};
{% endhighlight %}

-------------------------------------------------------

##### <span id="B012">device-tree structure</span>

这个段保存全部节点的信息，即包括节点的属性又包括节点的子节点，关系如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000214.png)

###### 节点 Node

从上图可知，每个节点或子节点都是以 FDT_BEGIN_NODE 开始，到
FDT_END_NODE 结束。节点中可以嵌套节点，嵌套的节点称为父节点，
被嵌套的节点称为子节点。

{% highlight bash %}
FDT_BEGIN_NODE         .......... 节点 N
|
|---FDT_BEGIN_NODE     .......... 子节点 0
|   |
|   FDT_END_NODE
|
|---FDT_BEGIN_NODE     .......... 子节点 1
|   |
|   FDT_END_NODE
|
FDT_END_NODE
{% endhighlight %}

节点在内核中的定义为：

{% highlight bash %}
struct fdt_node_header {
    uint32_t tag;
    char name[0];
};
{% endhighlight %}

###### 属性 property

从上图可知，属性位于节点内部，并且每个属性以 FDT_PROP 开始，到下一个
FDT_PROP 结束。属性内包含了属性字符串大小，属性字符串在 device-tree strings
中的偏移，以及属性数值等信息。

{% highlight bash %}
FDT_BEGIN_NODE
|
|----FDT_PROP
|    |--- Property string size
|    |--- Property string offset
|    |--- Property value
|    
|----FDT_PROP
|    |--- Property string size
|    |--- Property string offset
|    |--- Property value
|
FDT_END_NODE
{% endhighlight %}

属性在内核中的定义为：

{% highlight bash %}
struct fdt_property {
    uint32_t tag;
    uint32_t len;
    uint32_t nameoff;
    char data[0];
};
{% endhighlight %}

###### 宏定义

宏定义位于：/scripts/dtc/libfdt/fdt/h

{% highlight bash %}
#define FDT_MAGIC    0xd00dfeed    /* 4: version, 4: total size */
#define FDT_TAGSIZE    sizeof(uint32_t)

#define FDT_BEGIN_NODE    0x1        /* Start node: full name */
#define FDT_END_NODE    0x2        /* End node */
#define FDT_PROP    0x3        /* Property: name off,
                       size, content */
#define FDT_NOP        0x4        /* nop */
#define FDT_END        0x9

#define FDT_V1_SIZE    (7*sizeof(uint32_t))
#define FDT_V2_SIZE    (FDT_V1_SIZE + sizeof(uint32_t))
#define FDT_V3_SIZE    (FDT_V2_SIZE + sizeof(uint32_t))
#define FDT_V16_SIZE    FDT_V3_SIZE
#define FDT_V17_SIZE    (FDT_V16_SIZE + sizeof(uint32_t))
{% endhighlight %}

----------------------------------

#### <span id="B013">Device-tree strings</span>

由于某些属性 (比如 compatible) 在大多数节点下都会存在，
为了减少 dtb 文件大小，就需要把这些属性字符串指定一个存储位置即可，
这样每个节点的属性只需找到属性字符串的位置就可以得到那个属性字符串，
所以 dtb 把 device-tree strings 单独列出来存储。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000213.png)

device-tree strings 在 dtb 中的位置由 dtb header 即
boot_param_header 结构的 off_dt_strings 进行指定。




















------------------------------------------------------

##### <span id="A010">讨论引入 FDT 到嵌入式 linux 平台</span>

"Recent" (2009) discussion of "Flattened Device Tree"
work on linux-embedded mailing list:

> - [http://www.mail-archive.com/linux-embedded@vger.kernel.org/msg01721.html](http://www.mail-archive.com/linux-embedded@vger.kernel.org/msg01721.html)

{% highlight bash %}
Re: Representing Embedded Architectures at the Kernel Summit

Grant Likely Tue, 02 Jun 2009 10:30:27 -0700

On Tue, Jun 2, 2009 at 9:22 AM, James Bottomley
<james.bottom...@hansenpartnership.com> wrote:
> Hi All,
>
> We've got to the point where there are simply too many embedded
> architectures to invite all the arch maintainers to the kernel summit.
> So, this year, we thought we'd do embedded via topic driven invitations
> instead.  So what we're looking for is a proposal to discuss the issues
> most affecting embedded architectures, or preview any features affecting
> the main kernel which embedded architectures might need ... or any other
> topics from embedded architectures which might need discussion or
> debate.  If you'd like to do this, could you either reply to this email,
> or send proposals to:
>
> ksummit-2009-disc...@lists.linux-foundation.org


Hi James,

One topic that seems to garner debate is the issue of decoupling the
kernel image from the target platform.  ie. On x86, PowerPC and Sparc
a kernel image will boot on any machine (assuming the needed drivers
are enabled), but this is rarely the case in embedded.  Most embedded
kernels require explicit board support selected at compile time with
no way to produce a generic kernel which will boot on a whole family
of devices, let alone the whole architecture.  Part of this is a
firmware issue, where existing firmware passes very little in the way
of hardware description to the kernel, but part is also not making
available any form of common language for describing the machine.

Embedded PowerPC and Microblaze has tackled this problem with the
"Flattened Device Tree" data format which is derived from the
OpenFirmware specifications, and there is some interest and debate (as
discussed recently on the ARM mailing list) about making flattened
device tree usable by ARM also (which I'm currently
proof-of-concepting).  Josh Boyer has already touched on discussing
flattened device tree support at kernel summit in an email to the
ksummit list last week (quoted below), and I'm wondering if a broader
discussing would be warranted.

I think that in the absence of any established standard like the PC
BIOS/EFI or a real Open Firmware interface, then the kernel should at
least offer a recommended interface so that multiplatform kernels are
possible without explicitly having the machine layout described to it
at compile time.  I know that some of the embedded distros are
interested in such a thing since it gets them away from shipping
separate images for each supported board.  ie. It's really hard to do
a generic live-cd without some form of multiplatform.  FDT is a great
approach, but it probably isn't the only option.  It would be worth
debating.

Cheers,
g.

On Thu, May 28, 2009 at 5:41 PM, Josh Boyer <jwbo...@linux.vnet.ibm.com> wrote:
> Not to hijack this entirely, but I'm wondering if we could tie in some of the
> cross-arch device tree discussions that have been taking place among the ppc,
> sparc, and arm developers.
>
> I know the existing OF code for ppc, sparc, and microblaze could probably use
> some consolidation and getting some of the arch maintainers together in a room
> might prove fruitful.  Particularly if we are going to discuss how to make
> drivers work for device tree and standard platforms alike.
>
> josh
>



--
Grant Likely, B.Sc., P.Eng.
Secret Lab Technologies Ltd.
--
To unsubscribe from this list: send the line "unsubscribe linux-embedded" in
the body of a message to majord...@vger.kernel.org
More majordomo info at  http://vger.kernel.org/majordomo-info.html
{% endhighlight %}

----------------------------------------------------

##### <span id="A011">Russell King 反对 DTS 加入到 ARM</span>

Russell King is against adding support for FDT to the ARM
platform (see whole thread for interesting discussion):

> - [http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/01942.html](http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/01942.html)

{% highlight bash %}
Re: [RFC] [PATCH] Device Tree on ARM platform
From: Russell King
Date: Wed May 27 2009 - 13:48:45 EST

    Next message: Oren Laadan: "[RFC v16][PATCH 40/43] c/r: support semaphore sysv-ipc"
    Previous message: Oren Laadan: "[RFC v16][PATCH 36/43] c/r: support share-memory sysv-ipc"
    In reply to: Mark Brown: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Next in thread: Grant Likely: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Messages sorted by: [ date ] [ thread ] [ subject ] [ author ]

(For whatever reason, I don't have the initial email on this.)

On Wed, May 27, 2009 at 08:27:10AM -0600, Grant Likely wrote:
> On Wed, May 27, 2009 at 1:08 AM, Janboe Ye <yuan-bo.ye@xxxxxxxxxxxx> wrote:
> > Hi, All
> >
> > Currently, ARM linux uses mach-type to figure out platform. But mach-type could not handle variants well and it doesn't tell the kernel about info about attached peripherals.
> >
> > The device-tree used by powerpc and sparc could simplifiy board ports, less platform specific code and simplify device driver code.
> >
> > Please reference to Grant Likely and Josh Boyer's paper, A Symphony of Flavours: Using the device tree to describe embedded hardware , for the detail of device tree.
> >
> > www.kernel.org/doc/ols/2008/ols2008v2-pages-27-38.pdf
> >
> > Signed-off-by: janboe <yuan-bo.ye@xxxxxxxxxxxx>
>
> Heeheehe, This is Fantastic. I'm actually working on this too. Would
> you like to join our efforts?

My position is that I don't like this approach. We have _enough_ of a
problem getting boot loaders to do stuff they should be doing on ARM
platforms, that handing them the ability to define a whole device tree
is just insanely stupid.

For example, it's taken _years_ to get boot loader folk to pass one
correct value to the kernel. It's taken years for boot loaders to
start passing ATAGs to the kernel to describe memory layouts. And even
then there's various buggy platforms which don't do this correctly.

I don't see device trees as being any different - in fact, I see it as
yet another possibility for a crappy interface that lots of people will
get wrong, and then we'll have to carry stupid idiotic fixups in the
kernel for lots of platforms.

The end story is that as far as machine developers are concerned, a
boot loader, once programmed into the device, is immutable. They never
_ever_ want to change it, period.

So no, I see this as a recipe for ugly hacks appearing in the kernel
working around boot loader crappyness, and therefore I'm against it.

--
Russell King
Linux kernel 2.6 ARM Linux - http://www.arm.linux.org.uk/
maintainer of:
--
To unsubscribe from this list: send the line "unsubscribe linux-kernel" in
the body of a message to majordomo@xxxxxxxxxxxxxxx
More majordomo info at http://vger.kernel.org/majordomo-info.html
Please read the FAQ at http://www.tux.org/lkml/
{% endhighlight %}

------------------------------------------------

##### <span id="A013">Russell 最终被说服接受 DTS 进入 ARM</span>

But maybe Russell can be convinced:

> - [http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/03618.html](http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/03618.html)

{% highlight bash %}
Re: [RFC] [PATCH] Device Tree on ARM platform
From: Russell King - ARM Linux
Date: Sun May 31 2009 - 06:54:00 EST

    Next message: Alon Ziv: "Preferred kernel API/ABI for accelerator hardware device driver"
    Previous message: Pekka J Enberg: "[PATCH] slab: document kzfree() zeroing behavior"
    In reply to: Benjamin Herrenschmidt: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Next in thread: Grant Likely: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Messages sorted by: [ date ] [ thread ] [ subject ] [ author ]

On Fri, May 29, 2009 at 10:51:14AM +1000, Benjamin Herrenschmidt wrote:
> On Thu, 2009-05-28 at 17:04 +0200, Sascha Hauer wrote:
> >
> > We normally hide these subtle details behind a baseboard= kernel
> > parameter. I agree with you that it's far better to have a
> > standardized
> > way to specify this. For my taste the oftree is too bloated for this
> > purpose.
>
> Define "bloated" ?
>
> We got a lot of push back on powerpc initially with this exact same
> argument "too bloated" but that was never backed up with facts and
> numbers, and I would mostly say that nobody makes it anymore now
> that it's there and people use it.

It really depends how you look at it.

If you look at the amount of supporting code required for one platform
on ARM, it's typically fairly small - mostly just declaration of data
structures (platform devices, platform device data), and possibly a few
small functions to handle the quirkyness of the platform. That's of the
order of a few K of data and code combined - that's on average about 4K.
Add in the SoC platform device declaration code, and maybe add another
10K.

So, about 14K of code and data.

The OF support code in drivers/of is about 3K. The code posted at the
start of this thread I suspect will be about 4K or so, which gives us a
running total of maybe 7K.

What's now missing is conversion of drivers and the like to be DT
compatible, or creation of the platform devices to translate DT into
platform devices and their associated platform device data. Plus
some way to handle the platforms quirks, which as discussed would be
hard to represent in DT.

So I suspect it's actually marginal whether DT turns out to be larger
than our current approach. So I agree with BenH - I don't think there's
an argument to be made about 'bloat' here.

What /does/ concern me is what I percieve as the need to separate the
platform quirks from the description of the platform - so rather than
having a single file describing the entire platform (eg,
arch/arm/mach-pxa/lubbock.c) we would need:

- a file to create the DT information
- a separate .c file in the kernel containing code to handle the
platforms quirks, or a bunch of new DT drivers to do the same
- ensure both DT and quirks are properly in-sync.

But... we need to see how Grant gets on with his PXA trial before we
can properly assess this.
--
To unsubscribe from this list: send the line "unsubscribe linux-kernel" in
the body of a message to majordomo@xxxxxxxxxxxxxxx
More majordomo info at http://vger.kernel.org/majordomo-info.html
Please read the FAQ at http://www.tux.org/lkml/
{% endhighlight %}

----------------------------------------------------------

##### <span id="A014">David Gibson 辩护 FDT</span>

David Gibson definds FDT：

> - [http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/02304.html](http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/02304.html)

{% highlight bash %}
Re: [RFC] [PATCH] Device Tree on ARM platform
From: David Gibson
Date: Wed May 27 2009 - 23:45:33 EST

    Next message: Rusty Russell: "Re: [my_cpu_ptr 1/5] Introduce my_cpu_ptr()"
    Previous message: David Gibson: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    In reply to: Grant Likely: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Next in thread: Pavel Machek: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Messages sorted by: [ date ] [ thread ] [ subject ] [ author ]

On Wed, May 27, 2009 at 11:52:50AM -0600, Grant Likely wrote:
> On Wed, May 27, 2009 at 11:44 AM, Russell King
> <rmk+lkml@xxxxxxxxxxxxxxxx> wrote:
> > (For whatever reason, I don't have the initial email on this.)
> >
> > On Wed, May 27, 2009 at 08:27:10AM -0600, Grant Likely wrote:
> >> On Wed, May 27, 2009 at 1:08 AM, Janboe Ye <yuan-bo.ye@xxxxxxxxxxxx> wrote:
> >> > Hi, All
> >> >
> >> > Currently, ARM linux uses mach-type to figure out platform. But mach-type could not handle variants well and it doesn't tell the kernel about info about attached peripherals.
> >> >
> >> > The device-tree used by powerpc and sparc could simplifiy board ports, less platform specific code and simplify device driver code.
> >> >
> >> > Please reference to Grant Likely and Josh Boyer's paper, A Symphony of Flavours: Using the device tree to describe embedded hardware , for the detail of device tree.
> >> >
> >> > www.kernel.org/doc/ols/2008/ols2008v2-pages-27-38.pdf
> >> >
> >> > Signed-off-by: janboe <yuan-bo.ye@xxxxxxxxxxxx>
> >>
> >> Heeheehe, This is Fantastic.  I'm actually working on this too.  Would
> >> you like to join our efforts?
> >
> > My position is that I don't like this approach.  We have _enough_ of a
> > problem getting boot loaders to do stuff they should be doing on ARM
> > platforms, that handing them the ability to define a whole device tree
> > is just insanely stupid.
>
> The point of this approach is that the device tree is *not* create by
> firmware. Firmware can pass it in if it is convenient to do so, (ie;
> the device tree blob stored in flash as a separate image) but it
> doesn't have to be and it is not 'owned' by firmware.
>
> It is also true that there is the option for firmware to manipulate
> the .dts, but once again it is not required and it does not replace
> the existing ATAGs.
>
> If a board port does get the device tree wrong; no big deal, we just
> fix it and ship it with the next kernel.

Indeed one of the explicit goals we had in mind in building the
flattened device tree system is that the kernel proper can rely on
having a usable device tree, without requiring that the bootloader /
firmware get all that right.

Firmware can supply a device tre, and if that's sufficiently good to
be usable, that's fine. But alternatively our bootwrapper can use
whatever scraps of information the bootloader does provide to either
pick the right device tree for the platform, tweak it as necessary for
information the bootloader does supply correctly (memory and/or flash
sizes are common examples), or even build a full device tree from
information the firmware supplies in some other form (rare, but
occasionally usefule, e.g. PReP).

We explicitly had the ARM machine number approach in mind as one of
many cases that the devtree mechanism can degenerate to: the
bootwrapper just picks the right canned device tree based on the
machine number. If the bootloader gets the machine number wrong, but
supplies a few other hints that let us work out what the right machine
is, we have logic to pick the device tree based on that. Yes, still a
hack, but at least it's well isolated.

If the firmware does provide a device tree, but it's crap, code to
patch it up to something usable (which could be anything from applying
a couple of tweaks, up to replacing it wholesale with a canned tree
based on one or two properties in the original which let you identify
the machine) is again well isolated.

> > The end story is that as far as machine developers are concerned, a
> > boot loader, once programmed into the device, is immutable.  They never
> > _ever_ want to change it, period.

You're over focusing - as too many people do - on the firmware/kernel
communication aspects of the devtree. Yes, the devtree does open some
interesting possibilities in that area, but as you say, firmware can
never be trusted so the devtree doesn't really bring anything new
(better or worse) here.

What it does bring is a *far* more useful and expressive way of
representing consolidated device information in the kernel than simple
tables. And with the device tree compiler, it also becomes much more
convenient to prepare this information than building C tables. I
encourage you to have a look at the sample device trees in
arch/powerpc/boot/dts and at the device tree compiler code (either in
scripts/dtc where it's just been moved, or from the upstream source at
git://git.jdl.com/software/dtc.git).

I have to agree with DaveM - a lot of people's objections to the
devtree stem from not actually understanding what it does and how it
works.

--
David Gibson | I'll have my music baroque, and my code
david AT gibson.dropbear.id.au | minimalist, thank you. NOT _the_ _other_
| _way_ _around_!
http://www.ozlabs.org/~dgibson
--
To unsubscribe from this list: send the line "unsubscribe linux-kernel" in
the body of a message to majordomo@xxxxxxxxxxxxxxx
More majordomo info at http://vger.kernel.org/majordomo-info.html
Please read the FAQ at http://www.tux.org/lkml/
{% endhighlight %}

--------------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](https://biscuitos.github.io/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [搭建高效的 Linux 开发环境](https://biscuitos.github.io/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)
