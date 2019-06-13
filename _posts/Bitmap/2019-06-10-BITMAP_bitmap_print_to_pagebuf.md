---
layout: post
title:  "bitmap_print_to_pagebuf"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP bitmap_print_to_pagebuf().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

> [Github: bitmap_print_to_pagebuf](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/bitmap_print_to_pagebuf)
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
 * __bitmap_parse - convert an ASCII hex string into a bitmap.
 * @buf: pointer to buffer containing string.
 * @buflen: buffer size in bytes.  If string is smaller than this
 *    then it must be terminated with a \0.
 * @is_user: location of buffer, 0 indicates kernel space
 * @maskp: pointer to bitmap array that will contain result.
 * @nmaskbits: size of bitmap, in bits.
 *
 * Commas group hex digits into chunks.  Each chunk defines exactly 32
 * bits of the resultant bitmask.  No chunk may specify a value larger
 * than 32 bits (%-EOVERFLOW), and if a chunk specifies a smaller value
/**
 * bitmap_print_to_pagebuf - convert bitmap to list or hex format ASCII string
 * @list: indicates whether the bitmap must be list
 * @buf: page aligned buffer into which string is placed
 * @maskp: pointer to bitmap to convert
 * @nmaskbits: size of bitmap, in bits
 *
 * Output format is a comma-separated list of decimal numbers and
 * ranges if list is specified or hex digits grouped into comma-separated
 * sets of 8 digits/set. Returns the number of characters written to buf.
 *
 * It is assumed that @buf is a pointer into a PAGE_SIZE, page-aligned
 * area and that sufficient storage remains at @buf to accommodate the
 * bitmap_print_to_pagebuf() output. Returns the number of characters
 * actually printed to @buf, excluding terminating '\0'.
 */
int bitmap_print_to_pagebuf(bool list, char *buf, const unsigned long *maskp,
                            int nmaskbits)
{
        ptrdiff_t len = PAGE_SIZE - offset_in_page(buf);

        return list ? scnprintf(buf, len, "%*pbl\n", nmaskbits, maskp) :
                      scnprintf(buf, len, "%*pb\n", nmaskbits, maskp);
}
EXPORT_SYMBOL(bitmap_print_to_pagebuf);
{% endhighlight %}

bitmap_print_to_pagebuf() 函数用于将一个 bitmap 转换成 16 进制形式的字符串。
参数 buf 用于存储生成的字符串；参数 maskp 指向 bitmap；参数 nmaskbits 用于
指定转换的 bitmap 位数。函数调用 scnprintf() 将 bitmap 转换成 ASCII。

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
	unsigned long bitmap = 0x90911016;
	char buf[128];

	/* Cover bitmap to ASCII */
	bitmap_print_to_pagebuf(0, buf, &bitmap, 32);
	printk("Bitmap: %#lx to ASCII: %s", bitmap, buf);

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
+       bool "bitmap_print_to_pagebuf"
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
            [*]bitmap_print_to_pagebuf()
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
Bitmap: 0x90911016 to ASCII: 90911016
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

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
