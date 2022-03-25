---
layout: post
title:  "BIT_WORD"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP BIT_WORD().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

> [Github: BIT_WORD](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/BIT_WORD)
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
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
{% endhighlight %}

BIT_WORD 宏用于计算 nr 个 bit 占用多少个字，这里字的长度是 BITS_PER_LONG，且
计算出来的字长度比实际少一个，这也是一种向下对齐。参数 nr 代表 bit 的总数。函数
将 nr 的值除以 BITS_PER_LONG 以此向下计算出 nr 个 bit 占了多少个字。

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
	printk("BIT_WORD(0):   %#x\n", BIT_WORD(0));
	printk("BIT_WORD(15):  %#x\n", BIT_WORD(15));
	printk("BIT_WORD(31):  %#x\n", BIT_WORD(31));
	printk("BIT_WORD(32):  %#x\n", BIT_WORD(32));
	printk("BIT_WORD(33):  %#x\n", BIT_WORD(33));
	printk("BIT_WORD(63):  %#x\n", BIT_WORD(63));
	printk("BIT_WORD(64):  %#x\n", BIT_WORD(64));
	printk("BIT_WORD(65):  %#x\n", BIT_WORD(65));
	printk("BIT_WORD(90):  %#x\n", BIT_WORD(90));

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
+       bool "BIT_WORD"
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
            [*]BIT_WORD()
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
BIT_WORD(0):   0x0
BIT_WORD(15):  0x0
BIT_WORD(31):  0x0
BIT_WORD(32):  0x1
BIT_WORD(33):  0x1
BIT_WORD(63):  0x1
BIT_WORD(64):  0x2
BIT_WORD(65):  0x2
BIT_WORD(90):  0x2
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

BIT_WORD() 宏用于计算 nr 个 bit 占用的字数。

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
