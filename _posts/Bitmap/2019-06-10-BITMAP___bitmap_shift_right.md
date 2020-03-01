---
layout: post
title:  "__bitmap_shift_right"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP __bitmap_shift_right().
tags:
  - Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __bitmap_shift_right](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/__bitmap_shift_right)
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
 * __bitmap_shift_right - logical right shift of the bits in a bitmap
 *   @dst : destination bitmap
 *   @src : source bitmap
 *   @shift : shift by this many bits
 *   @nbits : bitmap size, in bits
 *
 * Shifting right (dividing) means moving bits in the MS -> LS bit
 * direction.  Zeros are fed into the vacated MS positions and the
 * LS bits shifted off the bottom are lost.
 */
void __bitmap_shift_right(unsigned long *dst, const unsigned long *src,
                        unsigned shift, unsigned nbits)
{
        unsigned k, lim = BITS_TO_LONGS(nbits);
        unsigned off = shift/BITS_PER_LONG, rem = shift % BITS_PER_LONG;
        unsigned long mask = BITMAP_LAST_WORD_MASK(nbits);
        for (k = 0; off + k < lim; ++k) {
                unsigned long upper, lower;

                /*
                 * If shift is not word aligned, take lower rem bits of
                 * word above and make them the top rem bits of result.
                 */
                if (!rem || off + k + 1 >= lim)
                        upper = 0;
                else {
                        upper = src[off + k + 1];
                        if (off + k + 1 == lim - 1)
                                upper &= mask;
                        upper <<= (BITS_PER_LONG - rem);
                }
                lower = src[off + k];
                if (off + k == lim - 1)
                        lower &= mask;
                lower >>= rem;
                dst[k] = lower | upper;
        }
        if (off)
                memset(&dst[lim - off], 0, off*sizeof(unsigned long));
}
EXPORT_SYMBOL(__bitmap_shift_right);
{% endhighlight %}

__bitmap_shift_right() 函数用于将 bitmap 的 nbits 位进行逻辑右移操作。参数
dst 指向存储右移结果；src 指向 bitmap；参数 shift 代表右移的位数；参数
nbits 指向要操作的范围。函数首先调用 BITS_TO_LONGS() 函数获得要操作 bit 占用
long 变量的个数，然后将 shift 除以 BITS_PER_LONG，以此计算右移多少个完整的
long，同时也通过求余运算计算右移不按 BITS_PER_LONG 对齐的个数。函数调用
BITSMAP_LAST_WORD_MASK 生成从 LSB 开始的全 1 掩码。函数主体是在一个 for
循环中，每次循环，函数都对右移整数部分和余数部分进行处理。如果函数不存在余数，
或者 off+k+1 的值不小于 lim，那么设置 upper 为 0；反之获得高位并与 mask 相与，
以此获得不对齐位的偏移。同理获得整除部分。如果此时 off 存在，那么调用 memset()
对 dst 对应为进行清除操作。

> - [BITS_TO_LONGS](https://biscuitos.github.io/blog/BITMAP_BITS_TO_LONGS/)
>
> - [BITMAP_LAST_WORD_MASK](https://biscuitos.github.io/blog/BITMAP_BITMAP_LAST_WORD_MASK/)

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
	unsigned long dst;
	unsigned long src[] = {0x12345678, 0x9abcdef};

	__bitmap_shift_right(&dst, src, 8, 32);
	printk("DST: %#lx\n", dst);

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
+       bool "__bitmap_shift_right"
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
            [*]__bitmap_shift_right()
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
DST: 0x123456
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

bitmap 右移操作

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
