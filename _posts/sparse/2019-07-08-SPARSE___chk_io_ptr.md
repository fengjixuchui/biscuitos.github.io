---
layout: post
title:  "__chk_io_ptr"
date:   2019-07-08 05:30:30 +0800
categories: [HW]
excerpt: sparse __chk_io_ptr().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __chk_io_ptr](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/sparse/API/__chk_io_ptr)
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
extern void __chk_io_ptr(const volatile void __iomem *);
#endif
{% endhighlight %}

__chk_io_ptr() 函数用于检查指针是否指向 iomem 空间，如果不指向，
那么 sparse 工具就会报错。

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
	unsigned long __user   *puser;
	unsigned long __kernel *pkernel;
	unsigned long __iomem  *piomem;

	/* Check iomem space pointer */
	__chk_io_ptr(puser);
	__chk_io_ptr(pkernel);
	__chk_io_ptr(piomem);

	puser = NULL;
	pkernel = NULL;
	piomem = NULL;

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
+       bool "__chk_io_ptr"
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
            [*]__chk_io_ptr()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

编译结果如下：

{% highlight bash %}
buddy@BDOS:/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/linux/linux$ make ARCH=ar
m C=2 CROSS_COMPILE=/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- drivers/BiscuitOS/sparse.o -j4
  CHECK   scripts/mod/empty.c
  CALL    scripts/checksyscalls.sh
  CHECK   drivers/BiscuitOS/sparse.c
drivers/BiscuitOS/sparse.c:24:22: warning: incorrect type in argument 1 (different address spaces)
drivers/BiscuitOS/sparse.c:24:22:    expected void const volatile [noderef] <asn:2> *
drivers/BiscuitOS/sparse.c:24:22:    got unsigned long [noderef] <asn:1> *puser
drivers/BiscuitOS/sparse.c:25:22: warning: incorrect type in argument 1 (different address spaces)
drivers/BiscuitOS/sparse.c:25:22:    expected void const volatile [noderef] <asn:2> *
drivers/BiscuitOS/sparse.c:25:22:    got unsigned long *pkernel
  CC      drivers/BiscuitOS/sparse.o
{% endhighlight %}

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

检查指针是否指向 IO 空间。

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

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)
