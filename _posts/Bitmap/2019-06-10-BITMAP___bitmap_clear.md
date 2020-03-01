---
layout: post
title:  "__bitmap_clear"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP __bitmap_clear().
tags:
  - Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __bitmap_clear](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/__bitmap_clear)
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
void __bitmap_clear(unsigned long *map, unsigned int start, int len)
{
        unsigned long *p = map + BIT_WORD(start);
        const unsigned int size = start + len;
        int bits_to_clear = BITS_PER_LONG - (start % BITS_PER_LONG);
        unsigned long mask_to_clear = BITMAP_FIRST_WORD_MASK(start);

        while (len - bits_to_clear >= 0) {
                *p &= ~mask_to_clear;
                len -= bits_to_clear;
                bits_to_clear = BITS_PER_LONG;
                mask_to_clear = ~0UL;
                p++;
        }
        if (len) {
                mask_to_clear &= BITMAP_LAST_WORD_MASK(size);
                *p &= ~mask_to_clear;
        }
}
EXPORT_SYMBOL(__bitmap_clear);
{% endhighlight %}

__bitmap_clear() 用于清除 bitmap 指定范围内，长度为 len 的 bits。参数 map
指向一个 bitmap；参数 start 指向开始清除 bit 的位置；参数 len 指向需要清除
bit 的个数。函数首先通过调用 BIT_WORD 计算出 start 位于 map 的 long 偏移。
定义局部变量 bits_to_clear 计算在一个完整的 long 变量里，剩余的 bit 数。
最后定义局部变量 mask_to_clear 调用 BITMAP_FIRST_WORD_MASK() 生成 start
对应的掩码。函数接着调用 while 循环处理 start 的偏移已经大于 long 所包含
bits 数，如果 start 超过包含一个 long 包含的 bit 数之后，那么就将完整的
long 对应的 bits 数都清零，然后循环到下一个 long；如果循环结束，此时如果 len
的值不为零，那么意味着包含不足 long bits 的 bit，那么将 mask_to_clear
的值与 BITMAP_LAST_WORD_MASK() 相与，最后取反与 p 对应的值相与，以此
获得清零的位。

> - [BITMAP_FIRST_WORD_MASK](https://biscuitos.github.io/blog/BITMAP_BITMAP_FIRST_WORD_MASK/)
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
	unsigned long bitmap1 = 0xffff00f1;
	unsigned long bitmap2 = bitmap1;

	/* clear special bits */
	__bitmap_clear(&bitmap1, 4, 4);
	printk("%#lx clear 4 bit: %#lx\n", bitmap2, bitmap1);

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
+       bool "__bitmap_clear"
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
            [*]__bitmap_clear()
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
0xffff00f1 clear 4 bit: 0xffff0001
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

bitmap 清零操作。

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
