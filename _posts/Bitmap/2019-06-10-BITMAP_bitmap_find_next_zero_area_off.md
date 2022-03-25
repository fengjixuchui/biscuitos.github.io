---
layout: post
title:  "bitmap_find_next_zero_area_off"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP bitmap_find_next_zero_area_off().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

> [Github: bitmap_find_next_zero_area_off](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/bitmap_find_next_zero_area_off)
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
 * bitmap_find_next_zero_area_off - find a contiguous aligned zero area
 * @map: The address to base the search on
 * @size: The bitmap size in bits
 * @start: The bitnumber to start searching at
 * @nr: The number of zeroed bits we're looking for
 * @align_mask: Alignment mask for zero area
 * @align_offset: Alignment offset for zero area.
 *
 * The @align_mask should be one less than a power of 2; the effect is that
 * the bit offset of all zero areas this function finds plus @align_offset
 * is multiple of that power of 2.
 */
unsigned long bitmap_find_next_zero_area_off(unsigned long *map,
                                             unsigned long size,
                                             unsigned long start,
                                             unsigned int nr,
                                             unsigned long align_mask,
                                             unsigned long align_offset)
{
        unsigned long index, end, i;
again:
        index = find_next_zero_bit(map, size, start);

        /* Align allocation */
        index = __ALIGN_MASK(index + align_offset, align_mask) - align_offset;

        end = index + nr;
        if (end > size)
                return end;
        i = find_next_bit(map, end, index);
        if (i < end) {
                start = i + 1;
                goto again;
        }
        return index;
}
EXPORT_SYMBOL(bitmap_find_next_zero_area_off);
{% endhighlight %}

bitmap_find_next_zero_area_off() 函数用于在 bitmap start 处，长度为 size 的
范围内，找到第一块 nr 个 0 bit 的位置。参数 map 指向 bitmap；参数 size 指向
查找的长度；start 参数指向查找的起始位置；参数 nr 指向查找 zero bit 的个数；
采纳数 align_mask 对齐所用的掩码；参数 align_offset 用于对应的偏移。函数
首先使用 find_next_zero_bit() 函数在起始位置位 start，长度为 size 的范围内
找到第一个 zero bit 的位置，接着调用 __ALIGN_MASK() 函数对 index 进行对齐操作。
接着就是将 index + nr 的值存储在 end 里，以 end 作为辩解，调用 find_next_bit()
函数，在范围是 end，起始是 index 的 bitmap 中找到第一个为 1 的位置，并将这个
位置存在 i 中。如果 i 小于 end，那么表示找到的第一个块 zero area 中含有 1，所以
不满足条件，将 start 的位置跳转到 1 bit 之后，继续跳转到 again 处重新执行；如果
i 大于 end，那么表示找到的 zero area 符合条件，那么满足 zero area 的起始位置。

> - [find_next_bit](https://biscuitos.github.io/blog/BITMAP_find_next_bit/)
>
> - [find_next_zero_bit](https://biscuitos.github.io/blog/BITMAP_find_next_zero_bit/)

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
	unsigned int pos;

	/* Find position for first zero area. */
	pos = bitmap_find_next_zero_area_off(&bitmap1, 32, 0, 3, 1, 0);
	printk("Find %#lx first zero area postion is: %d\n", bitmap1, pos);

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
+       bool "bitmap_find_next_zero_area_off"
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
            [*]bitmap_find_next_zero_area_off()
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
Find 0xffff00f1 first zero area postion is: 8
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

找一个 zero area。

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
