---
layout: post
title:  "__bitmap_equal"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP __bitmap_equal().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __bitmap_equal](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/__bitmap_equal)
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
int __bitmap_equal(const unsigned long *bitmap1,
                const unsigned long *bitmap2, unsigned int bits)
{
        unsigned int k, lim = bits/BITS_PER_LONG;
        for (k = 0; k < lim; ++k)
                if (bitmap1[k] != bitmap2[k])
                        return 0;

        if (bits % BITS_PER_LONG)
                if ((bitmap1[k] ^ bitmap2[k]) & BITMAP_LAST_WORD_MASK(bits))
                        return 0;

        return 1;
}
EXPORT_SYMBOL(__bitmap_equal);
{% endhighlight %}

__bitmap_equal() 函数用于从最低位开始到特定的位，比较两个 bitmap 是否相等。
参数 bitmap1 指向一个 bitmap 的地址；参数 bitmap2 指向一个 bitmap 的地址；
参数 bits 值定从右到左需要比较的位数。函数首先判断 bits 参数包含多少个
BITS_PER_LONG, BITS_PER_LONG 代表在体系中，一个 unsigned long 所占用的位数。
通过计算 bits 所占用 BITS_PER_LONG 之后，直接使用 for 循环，从低位到高位
进行比较，如果比较的过程中遇到不相等的情况，那么直接返回 0. 比较完完整的
BITS_PER_LONG 位数之后，继续比较小于 BITS_PER_LONG 的位数。函数使用
bits 基于 BITS_PER_LONG 求余，然后将两个 bitmap 包含余数的位进行按位异或，
相与之后，与 BITMAP_LAST_WORD_MASK(bits) 进行比较相与操作，如果两个
bitmap 相同，那么异或的结果一定为零；反之为 1，那么函数直接返回 0。

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
	unsigned long bitmap0 = 0x90911016;
	unsigned long bitmap1 = 0x90921016;
	int bits = 16;

	if (__bitmap_equal(&bitmap0, &bitmap1, bits))
		printk("bitmap0: %#lx - bitmap1: %#lx equal bits[0 - %d]\n",
				bitmap0, bitmap1, bits);

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
+       bool "__bitmap_equal"
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
            [*]__bitmap_equal()
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
bitmap0: 0x90911016 - bitmap1: 0x90921016 equal bits[0 - 16]
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

比较两个 bitmap 的 LSB 是否相等。

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
