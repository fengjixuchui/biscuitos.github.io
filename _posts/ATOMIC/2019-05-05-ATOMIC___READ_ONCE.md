---
layout: post
title:  "__READ_ONCE"
date:   2019-05-05 14:55:30 +0800
categories: [HW]
excerpt: ATOMIC __READ_ONCE().
tags:
  - ATOMIC
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

> [Github: __READ_ONCE](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/atomic/API/__READ_ONCE)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>
>
> Architecture: ARMv7 Cortex A9-MP

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="源码分析">源码分析</span>

{% highlight ruby %}
#define __READ_ONCE(x, check)                                           \
({                                                                      \
        union { typeof(x) __val; char __c[1]; } __u;                    \
        if (check)                                                      \
                __read_once_size(&(x), __u.__c, sizeof(x));             \
        else                                                            \
                __read_once_size_nocheck(&(x), __u.__c, sizeof(x));     \
        smp_read_barrier_depends(); /* Enforce dependency ordering from x */ \
        __u.__val;                                                      \
})
{% endhighlight %}

__READ_ONCE() 函数用于从内存中读取一个变量，并支持检测。参数 x 指向需要读取内存
变量；check 参数用于指明 x 是否需要检查。函数首先定义一个联合体 __u，__u 包含两个
成员 __val 和 __c 共同使用这个联合体。函数首先检查 check 是否为真，如果为真，则
代表在从内存中读取数据的时候，需要检查；如果 check 为假，则代表在从内存中读取数据
的时候，不需要检查。__read_once_size() 和 __read_once_size_nocheck() 都是从
内存中读取数据到缓存 __u.__c 中。接着调用 smp_read_barrier_depends() 函数强制
依赖顺序，对于 ARMv7 平台，没有具体作用，最后将 __u.__val 返回。整个函数确保数据
都是从内存中读取。而不是从缓存，cache 或者寄存器中读取。

###### __read_once_size

> [\_\_read_once_size 源码分析](https://biscuitos.github.io/blog/ATOMIC___read_once_size/)

###### __read_once_size_nocheck

> [\_\_read_once_size_nocheck 源码分析](https://biscuitos.github.io/blog/ATOMIC___read_once_size_nocheck/)

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
 * atomic
 *
 * (C) 2019.05.05 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* READ_ONCE/WRITE_ONCE */
#include <linux/compiler.h>

static __init int atomic_demo_init(void)
{
        volatile char ch = 'A';
        char cb;

        /* Read from memory not cache nor register */
        cb = __READ_ONCE(ch, 1);

        printk("cb: %c\n", cb);

        return 0;
}
device_initcall(atomic_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 atomic.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_ATOMIC
+       bool "atomic"
+
+if BISCUITOS_ATOMIC
+
+config DEBUG_BISCUITOS_ATOMIC
+       bool "__READ_ONCE"
+
+endif # BISCUITOS_ATOMIC
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
+obj-$(CONFIG_BISCUITOS_ATOMIC)  += atomic.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]atomic
            [*]__READ_ONCE()
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

{% highlight ruby %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
cb: A
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

函数确保数据都是从内存中读取。

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
