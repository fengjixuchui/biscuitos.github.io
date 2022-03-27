---
layout: post
title:  "__bitmap_parse"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: BITMAP __bitmap_parse().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __bitmap_parse](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/API/__bitmap_parse)
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
 * then leading 0-bits are prepended.  %-EINVAL is returned for illegal
 * characters and for grouping errors such as "1,,5", ",44", "," and "".
 * Leading and trailing whitespace accepted, but not embedded whitespace.
 */
int __bitmap_parse(const char *buf, unsigned int buflen,
                int is_user, unsigned long *maskp,
                int nmaskbits)
{
        int c, old_c, totaldigits, ndigits, nchunks, nbits;
        u32 chunk;
        const char __user __force *ubuf = (const char __user __force *)buf;
{% endhighlight %}

__bitmap_parse() 用于将 16 进制的字符串转换成 bitmap，字符串中可以包含一个或
多个 16 进制字符串。参数 buf 指向字符串；参数 buflen 指定转换字符的数量；参数
is_user 指示是用户空间还是内核空间使用；参数 makep 用于存储转换之后的 bitmap；
参数 nmaskbits 用于指示 bitmap 采用的掩码。由于函数较长，分段解析。字符串
的格式可以是 "abc2343", 也可以是 "23de,876f,786", 但需要注意的是多个 16
进制字符串使用 "," 分开，但与 16 进制之间不存在空格，16 进制的字符串也不含
`0x` 前缀。

{% highlight c %}
        bitmap_zero(maskp, nmaskbits);

        nchunks = nbits = totaldigits = c = 0;
        do {
                chunk = 0;
                ndigits = totaldigits;

                /* Get the next chunk of the bitmap */
                while (buflen) {
                        old_c = c;
                        if (is_user) {
                                if (__get_user(c, ubuf++))
                                        return -EFAULT;
                        }
                        else
                                c = *buf++;
                        buflen--;
                        if (isspace(c))
                                continue;
{% endhighlight %}

函数首先调用 bitmap_zero() 函数将 bitmap 指向的位置的 nmaskbits 进行清零
操作。然后将 nchunks、nbits、totaldigits、c 变量都设置为 0.然后进入循环。
每次循环开始的时候，都是将 chunk 设置为 0，ndigits 设置为 totaldigits。
函数将上一次从 buf 中获得的字符存储在 old_c 中，然后从 buf 读取新的字符
存储在 c 里面，此时减少 buflen 要读取的数量。此时调用 isspace() 函数判断
c 字符是否是一个空格，如果此时是一个空格，那么结束最内层的循环，进入下一次
循环。

{% highlight c %}
                        /*
                         * If the last character was a space and the current
                         * character isn't '\0', we've got embedded whitespace.
                         * This is a no-no, so throw an error.
                         */
                        if (totaldigits && c && isspace(old_c))
                                return -EINVAL;

                        /* A '\0' or a ',' signal the end of the chunk */
                        if (c == '\0' || c == ',')
                                break;

                        if (!isxdigit(c))
                                return -EINVAL;

                        /*
                         * Make sure there are at least 4 free bits in 'chunk'.
                         * If not, this hexdigit will overflow 'chunk', so
                         * throw an error.
                         */
                        if (chunk & ~((1UL << (CHUNKSZ - 4)) - 1))
                                return -EOVERFLOW;

                        chunk = (chunk << 4) | hex_to_bin(c);
                        totaldigits++;
                }
{% endhighlight %}

如果字符不是空格，那么继续检查，如果 totaldigits 为 1，c 也存在，但上一个字符
是空格，那么直接返回 EINVAL，那也就说明了字符串中，空格只能出现在末尾。函数
继续检查，如果此时遇到字符串末尾或者遇到 `,`，那么就结束内存的 while 循环；
反之继续循环，继续调用 isxdigit() 函数判断当前字符是不是 16 进制对应的字符，
如果不是，那么直接返回 EINVAL，因为本函数只处理 16 进制的字符串；如果字符
是 16 进制字符串，那么函数此时检查 chunk 的值，看是否已经移除，CHUNSZ 定义为
32，如果此时没有移除就不返回 EOVERFLOW，函数调用 hex_to_bin() 函数将字符转换
成 BCD 码。最后增加 totaldigit 的值。

{% highlight c %}
                if (ndigits == totaldigits)
                        return -EINVAL;
                if (nchunks == 0 && chunk == 0)
                        continue;

                __bitmap_shift_left(maskp, maskp, CHUNKSZ, nmaskbits);
                *maskp |= chunk;
                nchunks++;
                nbits += (nchunks == 1) ? nbits_to_hold_value(chunk) : CHUNKSZ;
                if (nbits > nmaskbits)
                        return -EOVERFLOW;
        } while (buflen && c == ',');

        return 0;
}
{% endhighlight %}

函数继续检查 ndigits 与 totaldigits 的关系，以此确定是否溢出。函数接着调用
__bitmap_shift_left() 函数将 maskp 对应的 bitmap 进行左移 CHUNKSZ 位，
然后将 maskp 或上 chunk 的值，以此获得一个正确的 bitmap 值。while 循环
只有在 buflen 不为零以及 c 字符为 `,` 的时候继续。

> - [bitmap_zero](/blog/BITMAP_bitmap_zero/)
>
> - [\_\_bitmap_shift_left](/blog/BITMAP___bitmap_shift_left/)

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
	const char *buf = "a2100347,98adef22";
	unsigned long bitmap[10];

	__bitmap_parse(buf, 32, 0, bitmap, 32 * 4);
	printk("%s bitmap: %#lx\n", buf, bitmap[0]);
	printk("%s bitmap: %#lx\n", buf, bitmap[1]);

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
+       bool "__bitmap_parse"
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
            [*]__bitmap_parse()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
a2100347,98adef22 bitmap: 0x98adef22
a2100347,98adef22 bitmap: 0xa2100347
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

将 16 进制字符串转换成 bitmap。

-----------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [搭建高效的 Linux 开发环境](/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
