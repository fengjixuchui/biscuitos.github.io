---
layout: post
title:  "clear_bit"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP clear_bit().
tags:
  - Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

> [Github: clear_bit](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/clear_bit)
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
static inline void clear_bit(unsigned int nr, volatile unsigned long *p)
{
        p += BIT_WORD(nr);
        atomic_long_andnot(BIT_MASK(nr), (atomic_long_t *)p);
}

#define clear_bit(nr,p)                 ATOMIC_BITOP(clear_bit,nr,p)
{% endhighlight %}

clear_bit() 函数的作用是将指定位清零。参数 nr 在 BITS_PER_LONG 范围内，用于
指定清零的位置；参数 p 指向 bit。clear_bit() 属于 bitops 类中的一个，其实现
与体系有关，上方的函数是通用定义，下方的函数是与体系有关的定义。对于通用的定义，
函数首先计算 nr 位于 bit 的 long 偏移，然后调用 BIT_MASK() 生成对于的掩码，
最后使用 atomic_long_andnot() 函数写原子或操作，设置对应的 bit。对于体系相关
的实现，对 ARM 而言，其实现与下面函数有关：

> - [\_\_\_\_atomic_clear_bit](https://biscuitos.github.io/blog/BITMAP_____atomic_clear_bit)

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
	unsigned long bitmap = 0x1230068;
	unsigned long old = bitmap;
	int ret;

	/* set bit */
	set_bit(9, &bitmap);
	printk("set_bit(9, %#lx): %#lx\n", old, bitmap);

	old = bitmap;
	/* clear bit */
	clear_bit(9, &bitmap);
	printk("clear_bit(9, %#lx): %#lx\n", old, bitmap);

	old = bitmap;
	/* Change bit */
	change_bit(9, &bitmap);
	printk("change_bit(9, %#lx): %#lx\n", old, bitmap);

	old = bitmap;
	/* Set bit and return original value */
	ret = test_and_set_bit(9, &bitmap);
	printk("test_and_set_bit(9, %#lx): %#lx (origin: %d)\n", old,
							bitmap, ret);

	old = bitmap;
	/* Clear bit and return original value */
	ret = test_and_clear_bit(9, &bitmap);
	printk("test_and_clear_bit(9, %#lx): %#lx (origin: %d)\n", old,
							bitmap, ret);

	old = bitmap;
	/* Change bit and return original value */
	ret = test_and_change_bit(9, &bitmap);
	printk("test_and_change_bit(9, %#lx): %#lx (origin: %d)\n",
					old, bitmap, ret);


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
+       bool "clear_bit"
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
            [*]clear_bit()
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
set_bit(9, 0x1230068): 0x1230268
clear_bit(9, 0x1230268): 0x1230068
change_bit(9, 0x1230068): 0x1230268
test_and_set_bit(9, 0x1230268): 0x1230268 (origin: 1)
test_and_clear_bit(9, 0x1230268): 0x1230068 (origin: 1)
test_and_change_bit(9, 0x1230068): 0x1230268 (origin: 0)
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

bitops 清零操作

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
