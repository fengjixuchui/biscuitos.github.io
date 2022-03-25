---
layout: post
title:  "__iomem"
date:   2019-07-08 05:30:30 +0800
categories: [HW]
excerpt: sparse __iomem().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __iomem](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/sparse/API/__iomem)
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

{% highlight bash %}
#ifdef __CHECKER__
#define __iomem        __attribute__((noderef, address_space(2)))
#endif
{% endhighlight %}

__iomem 宏用于指定指针必须指向设备地址空间。如果指针指向其他空间，比如用户空间，
或者内核空间，那么 sparse 就是警告。__attribute__((address_space(2)))，
是用来修饰一个变量的，而且变量所在的地址空间必须是 2，即 I/O 空间。这里把程
序空间分成了 3 个部分，0 表示 normal space，即普通地址空间，对内核代码来说，
当然就是内核空间地址了。1 表示用户地址空间，这个不用多讲，还有一个 2，
表示是设备地址映射空间，例如硬件设备的寄存器在内核里所映射的地址空间。

--------------------------------------------------

# <span id="实践">实践</span>

> - [驱动源码](#驱动源码)
>
> - [驱动安装](#驱动安装)
>
> - [驱动配置](#驱动配置)
>
> - [驱动编译](#驱动编译)
>
> - [驱动运行](#驱动运行)
>
> - [驱动分析](#驱动分析)

#### <span id="驱动源码">驱动源码</span>

{% highlight c %}
/*
 * Sparse.
 *
 * (C) 2019.07.01 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* sparse macro */
#include <linux/types.h>

static __init int sparse_demo_init(void)
{
	int __iomem *src;
	int dst = 0x10;

	/* points to kernel space */
	src = &dst;

	printk("SRC: %#x\n", *src);

	return 0;
}
device_initcall(sparse_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 sparse.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_SPARSE
+       bool "sparse"
+
+if BISCUITOS_SPARSE
+
+config DEBUG_BISCUITOS_SPARSE
+       bool "__iomem"
+
+endif # BISCUITOS_SPARSE
+
endif # BISCUITOS_DRV
{% endhighlight %}

接着修改 Makefile，请参考如下修改：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Makefile b/drivers/BiscuitOS/Makefile
index 82004c9..9909149 100644
--- a/drivers/BiscuitOS/Makefile
+++ b/drivers/BiscuitOS/Makefile
@@ -1 +1,2 @@
obj-$(CONFIG_BISCUITOS_MISC)        += BiscuitOS_drv.o
+obj-$(CONFIG_BISCUITOS_SPARSE)     += sparse.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]sparse
            [*]__iomem()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

编译结果如下：

{% highlight bash %}
buddy@BDOS:/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/linux/linux$ make ARCH=arm C=2 CROSS_COMPILE=/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- drivers/BiscuitOS/sparse.o -j4
  CHECK   scripts/mod/empty.c
  CALL    scripts/checksyscalls.sh
  CHECK   drivers/BiscuitOS/sparse.c
drivers/BiscuitOS/sparse.c:22:13: warning: incorrect type in assignment (different address spaces)
drivers/BiscuitOS/sparse.c:22:13:    expected int [noderef] <asn:2> *src
drivers/BiscuitOS/sparse.c:22:13:    got int *
drivers/BiscuitOS/sparse.c:24:31: warning: dereference of noderef expression
  CC      drivers/BiscuitOS/sparse.o
{% endhighlight %}

源码中，将 src 指向了内核空间，所以 sparse 报错。

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
SRC: 0x10
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

检查指针是否指向 I/O 空间。

-----------------------------------------------

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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
