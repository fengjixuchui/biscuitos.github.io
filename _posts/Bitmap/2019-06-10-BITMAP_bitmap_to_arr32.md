---
layout: post
title:  "bitmap_to_arr32"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP bitmap_to_arr32().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

> [Github: bitmap_to_arr32](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/bitmap_to_arr32)
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
/**
 * bitmap_to_arr32 - copy the contents of bitmap to a u32 array of bits
 *      @buf: array of u32 (in host byte order), the dest bitmap
 *      @bitmap: array of unsigned longs, the source bitmap
 *      @nbits: number of bits in @bitmap
 */
void bitmap_to_arr32(u32 *buf, const unsigned long *bitmap, unsigned int nbits)
{
        unsigned int i, halfwords;

        halfwords = DIV_ROUND_UP(nbits, 32);
        for (i = 0; i < halfwords; i++) {
                buf[i] = (u32) (bitmap[i/2] & UINT_MAX);
                if (++i < halfwords)
                        buf[i] = (u32) (bitmap[i/2] >> 32);
        }

        /* Clear tail bits in last element of array beyond nbits. */
        if (nbits % BITS_PER_LONG)
                buf[halfwords - 1] &= (u32) (UINT_MAX >> ((-nbits) & 31));
}
EXPORT_SYMBOL(bitmap_to_arr32);

#if BITS_PER_LONG == 64
extern void bitmap_to_arr32(u32 *buf, const unsigned long *bitmap,
                                                        unsigned int nbits);
#else
#define bitmap_to_arr32(buf, bitmap, nbits)                     \
        bitmap_copy_clear_tail((unsigned long *) (buf),         \
                        (const unsigned long *) (bitmap), (nbits))
#endif
{% endhighlight %}

bitmap_to_arr32() 函数用于将 bitmap 转换成 32-bit 数组。参数 buf 指向 32-bit
数组；参数 bitmap 指向 bitmap；参数 nbits 指向需要转换的位数。函数的实现与
BITS_PER_LONG 有关，如果在 32-bit 系统中，那么 bitmap_to_arr32() 函数的实现
与 bitmap_copy_clear_tail() 函数有关；如果在 64-bit 系统中，那么函数首先调用
DIV_ROUND_UP() 函数计算 nbits 需要的最小 long 数量。然后将这些 long 变量拆分
成两个 32-bit 的变量存在 buf 数组里。最后如果 nbits 包含小于 BITS_PER_LONG 的
bit，那么将这些 bit 对应的 long 变量的值多余的 bit 清除掉。这样函数就可以将 bitmap
转换成一个 u32 的数组。

> - [bitmap_copy_clear_tail](https://biscuitos.github.io/blog/BITMAP_bitmap_copy_clear_tail)

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
	unsigned long bitmap[] = {0x12345678, 0x234edaff};
	unsigned int array[2];

	/* Cover bitmap into 32-bit array */
	bitmap_to_arr32(array, bitmap, 64);
	printk("Bitmap: %#x-%x\n", array[1], array[0]);

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
+       bool "bitmap_to_arr32"
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
            [*]bitmap_to_arr32()
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
Bitmap: 0x234edaff-12345678
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

bitmap 转换成 32-bit 数组。

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
