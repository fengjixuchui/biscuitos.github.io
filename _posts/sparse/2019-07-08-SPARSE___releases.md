---
layout: post
title:  "__releases"
date:   2019-07-08 05:30:30 +0800
categories: [HW]
excerpt: sparse __releases().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

> [Github: __releases](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/sparse/API/__releases)
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
#ifdef __CHECKER__
#define __releases(x)  __attribute__((context(x,1,0)))
#endif
{% endhighlight %}

__releases() 宏与锁有关，检查锁在使用前确保锁为 1，然后将其设置为 0.
__releases() 与 __acquires() 成对使用，用于检查在某个函数中，是否
正确的上锁和解锁操作。在实际使用中，__releases(x) 和__acquires(x) 必须成对
使用才算合法，否则编译时会有 sparse 工具报错， 而 spin_lock 和 spin_unlock
正需要这样的检查，也必须成对使用。所以正是利用了这个 "属性"，可以在内核编译期
间发现 spin_lock 和 spin_unlock 未配对使用的情况。而在实际编译后运行的代码
中，是不用 __attribute__((context()) 这个东东的。 该 "属性" 仅用于代码的静
态检查，用于编译阶段。

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
 * Sparse.
 *
 * (C) 2019.07.01 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* sparse macro */
#include <linux/types.h>
/* spinlock */
#include <linux/spinlock.h>

struct node {
	spinlock_t lock;
};

/* lock with __releases() and __releases() */
static inline void bs_connect(struct node *np)
	__releases(np->lock)
	__acquires(np->lock)
{
	spin_lock(&np->lock);
}

/* unlock with __releases() and  __releases() */
static inline void bs_disconnect(struct node *np)
	__releases(np->lock)
	__acquires(np->lock)
{
	spin_unlock(&np->lock);
}

static __init int sparse_demo_init(void)
{
	struct node n0;

	/* init spinlock */
	spin_lock_init(&n0.lock);

	/* lock */
	bs_connect(&n0);

	/* unlock */
	bs_disconnect(&n0);

	return 0;
}
device_initcall(sparse_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 sparse.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_SPARSE
+       bool "sparse"
+
+if BISCUITOS_SPARSE
+
+config DEBUG_BISCUITOS_SPARSE
+       bool "__releases"
+
+endif # BISCUITOS_SPARSE
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
obj-$(CONFIG_BISCUITOS_MISC)        += BiscuitOS_drv.o
+obj-$(CONFIG_BISCUITOS_SPARSE)     += sparse.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]sparse
            [*]__releases()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

编译结果如下：

{% highlight bash %}
buddy@BDOS:/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/linux/linux$ make ARCH=ar
m C=2 CROSS_COMPILE=/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- drivers/BiscuitOS/sparse.o -j4
  CHECK   scripts/mod/empty.c
  CALL    scripts/checksyscalls.sh
  CHECK   drivers/BiscuitOS/sparse.c
drivers/BiscuitOS/sparse.c:39:19: warning: context imbalance in 'sparse_demo_init' - unexpected unlock
  CC      drivers/BiscuitOS/sparse.
{% endhighlight %}

如果在上诉的代码中，将 bs_unlock() 或者 bs_lock() 函数缺少一个，
那么 sparse 就会报错。

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9

{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

锁成对使用检查。

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
