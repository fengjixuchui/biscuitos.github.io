---
layout: post
title:  "BITMAP_LAST_WORD_MASK"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP BITMAP_LAST_WORD_MASK().
tags:
  - Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

> [Github: BITMAP_LAST_WORD_MASK](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/BITMAP_LAST_WORD_MASK)
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
#define BITMAP_LAST_WORD_MASK(nbits) (~0UL >> (-(nbits) & (BITS_PER_LONG - 1)))
{% endhighlight %}

BITMAP_LAST_WORD_MASK 宏用于获得含 1 的掩码。参数 nbits 代表从右边起，含有
1 的个数。函数首先将 0UL 取反，以此获得长度为 BITS_PER_LONG 的全 1 值，然后
将该值进行右移，右移的位数为 -nibits，也就说明这样做的结果是从 bit0 向左获得
特定个数个 1. BITMAP_LAST_WORD_MASK(1) 的值就是 0x1, BITMAP_LAST_WORD_MASK(2)
的值就是 0x3.

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
 * Bitmap.
 *
 * (C) 2019.06.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* header of bitmap */
#include <linux/bitmap.h>

static __init int bitmap_demo_init(void)
{
	printk("Bitmap(0):   %#lx\n", BITMAP_LAST_WORD_MASK(0));
	printk("Bitmap(1):   %#lx\n", BITMAP_LAST_WORD_MASK(1));
	printk("Bitmap(2):   %#lx\n", BITMAP_LAST_WORD_MASK(2));
	printk("Bitmap(3):   %#lx\n", BITMAP_LAST_WORD_MASK(3));
	printk("Bitmap(4):   %#lx\n", BITMAP_LAST_WORD_MASK(4));
	printk("Bitmap(5):   %#lx\n", BITMAP_LAST_WORD_MASK(5));
	printk("Bitmap(6):   %#lx\n", BITMAP_LAST_WORD_MASK(6));
	printk("Bitmap(7):   %#lx\n", BITMAP_LAST_WORD_MASK(7));
	printk("Bitmap(8):   %#lx\n", BITMAP_LAST_WORD_MASK(8));
	printk("Bitmap(10):  %#lx\n", BITMAP_LAST_WORD_MASK(10));
	printk("Bitmap(12):  %#lx\n", BITMAP_LAST_WORD_MASK(12));
	printk("Bitmap(16):  %#lx\n", BITMAP_LAST_WORD_MASK(16));
	printk("Bitmap(18):  %#lx\n", BITMAP_LAST_WORD_MASK(18));
	printk("Bitmap(20):  %#lx\n", BITMAP_LAST_WORD_MASK(20));
	printk("Bitmap(22):  %#lx\n", BITMAP_LAST_WORD_MASK(22));
	printk("Bitmap(24):  %#lx\n", BITMAP_LAST_WORD_MASK(24));
	printk("Bitmap(26):  %#lx\n", BITMAP_LAST_WORD_MASK(26));
	printk("Bitmap(27):  %#lx\n", BITMAP_LAST_WORD_MASK(27));
	printk("Bitmap(28):  %#lx\n", BITMAP_LAST_WORD_MASK(28));
	printk("Bitmap(29):  %#lx\n", BITMAP_LAST_WORD_MASK(29));
	printk("Bitmap(30):  %#lx\n", BITMAP_LAST_WORD_MASK(30));
	printk("Bitmap(31):  %#lx\n", BITMAP_LAST_WORD_MASK(31));
	printk("Bitmap(31):  %#lx\n", BITMAP_LAST_WORD_MASK(32));

	return 0;
}
device_initcall(bitmap_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 bitmap.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_BITMAP
+       bool "bitmap"
+
+if BISCUITOS_BITMAP
+
+config DEBUG_BISCUITOS_BITMAP
+       bool "BITMAP_LAST_WORD_MASK"
+
+endif # BISCUITOS_BITMAP
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
obj-$(CONFIG_BISCUITOS_MISC)     += BiscuitOS_drv.o
+obj-$(CONFIG_BISCUITOS_BITMAP)     += bitmap.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]bitmap
            [*]BITMAP_LAST_WORD_MASK()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
Bitmap(0):   0xffffffff
Bitmap(1):   0x1
Bitmap(2):   0x3
Bitmap(3):   0x7
Bitmap(4):   0xf
Bitmap(5):   0x1f
Bitmap(6):   0x3f
Bitmap(7):   0x7f
Bitmap(8):   0xff
Bitmap(10):  0x3ff
Bitmap(12):  0xfff
Bitmap(16):  0xffff
Bitmap(18):  0x3ffff
Bitmap(20):  0xfffff
Bitmap(22):  0x3fffff
Bitmap(24):  0xffffff
Bitmap(26):  0x3ffffff
Bitmap(27):  0x7ffffff
Bitmap(28):  0xfffffff
Bitmap(29):  0x1fffffff
Bitmap(30):  0x3fffffff
Bitmap(31):  0x7fffffff
Bitmap(31):  0xffffffff
input: AT Raw Set 2 keyboard as /devices/platform/smb@4000000/smb@4000000:motherboard/smb@4000000:motherboard:iofpga@7,00000000/10006000.kmi/serio0/input/input0
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

获得全 1 的掩码。

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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
