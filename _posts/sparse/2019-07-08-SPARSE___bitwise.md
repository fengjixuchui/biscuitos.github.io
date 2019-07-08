---
layout: post
title:  "__bitwise"
date:   2019-07-08 05:30:30 +0800
categories: [HW]
excerpt: sparse __bitwise().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __bitwise](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/sparse/API/__bitwise)
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
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#define __bitwise __bitwise__
{% endhighlight %}

bitwise 是用来确保不同位方式类型不会被弄混 (小端模式，大端模式，cpu
尾模式，或者其他), 它提供了非常强的类型检查，如果不同类型之间进行赋值，
即便强制类型转换，Sparse 仍然会发出抱怨。在网络编程里面面，对字节序的
要求非常高，bitwise 可以很好的帮助检查潜在的错误。bitwise 的典型用法
是利用 typedef 定义一个有 bitwise 属性的基类型，之后凡是利用该基类型
声明的变量都将被强制类型检查。

bitwise 在内核中使用 __bitwise, 其最终的定义使用了 GCC 的内嵌属性
`__attribute__((bitwise))`, 虽然 __attribute__((bitwise)) 看起
来是 GCC 的属性声明格式，实际上 GCC 并不会处理这个属性.

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

/* bitwise: big-endian, little-endian */
typedef unsigned int __bitwise bs_t;

static __init int sparse_demo_init(void)
{
	bs_t a = (__force bs_t)0x12345678;
	bs_t b;

#ifdef __LITTLE_ENDIAN
	printk("little-endian original: %#x\n", a);
#else
	printk("big-endian original:    %#x\n", a);
#endif
	/* Cover to little-endian */
	b = (__force bs_t)cpu_to_le32(a);
	printk("%#x to little-endian: %#x\n", a, b);
	/* Cover to big-endian */
	b = (__force uint32_t)cpu_to_be32(a);
	printk("%#x to bit-endian:    %#x\n", a, b);

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
+       bool "__bitwise"
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
            [*]__bitwise()
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
  CC      arch/arm/kernel/asm-offsets.s
  CALL    scripts/checksyscalls.sh
  CHECK   drivers/BiscuitOS/sparse.c
drivers/BiscuitOS/sparse.c:31:27: warning: cast from restricted bs_t
drivers/BiscuitOS/sparse.c:34:31: warning: cast from restricted bs_t
drivers/BiscuitOS/sparse.c:34:31: warning: incorrect type in argument 1 (different base types)
drivers/BiscuitOS/sparse.c:34:31:    expected unsigned int [usertype] val
drivers/BiscuitOS/sparse.c:34:31:    got restricted bs_t [usertype] a
drivers/BiscuitOS/sparse.c:34:31: warning: cast from restricted bs_t
drivers/BiscuitOS/sparse.c:34:31: warning: cast from restricted bs_t
drivers/BiscuitOS/sparse.c:34:31: warning: cast from restricted bs_t
drivers/BiscuitOS/sparse.c:34:31: warning: cast from restricted bs_t
drivers/BiscuitOS/sparse.c:34:11: warning: incorrect type in assignment (different base types)
drivers/BiscuitOS/sparse.c:34:11:    expected restricted bs_t [assigned] [usertype] b
drivers/BiscuitOS/sparse.c:34:11:    got unsigned int [usertype]
  CC      drivers/BiscuitOS/sparse.o
{% endhighlight %}

源码中 b 定义的类型是一个小端，但这里将其赋值给一个大端的数据，sparse
会对这个数据进行警告。

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
little-endian original: 0x12345678
0x12345678 to little-endian: 0x12345678
0x12345678 to bit-endian:    0x78563412
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

大小端数据强制检测。

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
