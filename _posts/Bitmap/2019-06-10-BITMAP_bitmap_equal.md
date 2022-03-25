---
layout: post
title:  "bitmap_equal"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP bitmap_equal().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

> [Github: bitmap_equal](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/bitmap_equal)
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
static inline int bitmap_equal(const unsigned long *src1,
                        const unsigned long *src2, unsigned int nbits)
{
        if (small_const_nbits(nbits))
                return !((*src1 ^ *src2) & BITMAP_LAST_WORD_MASK(nbits));
        if (__builtin_constant_p(nbits & BITMAP_MEM_MASK) &&
            IS_ALIGNED(nbits, BITMAP_MEM_ALIGNMENT))
                return !memcmp(src1, src2, nbits / 8);
        return __bitmap_equal(src1, src2, nbits);
}

{% endhighlight %}

bitmap_equal() 函数用于对比两个 bitmap 指定位数内是否相等，如果相等则返回 1，
不相等则返回 0。参数 src1 指向第一个 bitmap；参数 src2 指向第二个 bitmap；
参数 nbits 指向需要对比的位数。函数首先调用 small_const_nbits() 函数判断
nbits 是否符合大于 0 小于 BITS_PER_LONG，且为常数的要求，如果符合，则将
src1 和 src2 进行异或运算并与 BITMAP_LAST_WORD_MASK() 生成的掩码相与，以此
确定指定的位数是否相等；反之函数调用 __builtin_constant_p() 函数判断 nbits
指定的位数内是否对齐，已经为常数，如果满足，那么直接调用 memcmp() 函数
进行两个 bitmap 的对比；如果上面的条件都不满足，那么函数直接调用
__bitmap_equal() 函数进行对比。以此计算两个 bitmap 在指定位数内是否相等。

> - [BITMAP_LAST_WORD_MASK](https://biscuitos.github.io/blog/BITMAP_BITMAP_LAST_WORD_MASK)

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
	unsigned long bitmap0 = 0xff45;
	unsigned long bitmap1 = 0x7845;

	/* Check bitmap whether equal */
	if (bitmap_equal(&bitmap0, &bitmap1, 8))
		printk("%#lx equal %#lx through 0 to 7.\n",
						bitmap0, bitmap1);

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
+       bool "bitmap_equal"
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
            [*]bitmap_equal()
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
0xff45 equal 0x7845 through 0 to 7.
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

检查 bitmap 是否相等

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
