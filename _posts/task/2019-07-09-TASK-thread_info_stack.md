---
layout: post
title:  "thread_info 与内核栈 stack 关系"
date:   2019-07-10 05:30:30 +0800
categories: [HW]
excerpt: thread_info THREAD_INFO().
tags:
  - Tree
---

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

# 目录

> - [thread_info 简介](#A00)
>
> - [thread_union 实现](#A01)
>
> - [thread_info 接口](#A02)
>
> - [0 号进程的 thread_info](#A03)
>
> - [thread_info 实践](#A04)
>
> - [附录](#附录)

-----------------------------------

### <span id="A00">thread_info 简介</span>

thread_info 结构被称为迷你进程描述符，是因为在这个结构中并没有
直接包含与进程相关的字段，而是通过 task 字段指向具体某个进程描
述符。通常这块内存区域的大小是 8KB，也就是两个页的大小（有时候
也使用一个页来存储，即 4KB）。一个进程的内核栈和 thread_info
结构之间的逻辑关系如下图所示：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000192.png)

从上图可知，内核栈是从该内存区域的顶层向下（从高地址到低地址）增
长的，而 thread_info  结构则是从该区域的开始处向上（从低地址到高
地址）增长。内核栈的栈顶地址存储在 esp 寄存器中。所以，当进程从用
户态切换到内核态后，esp 寄存器指向这个区域的末端。

由于一个页大小是 4K，一个页的起始地址都是 4K 的整数倍，即后 12
位都为 0，取得 esp 内核栈栈顶的地址，将其后 12 位取 0，就可以得
到内存区域的起始地址, 该地址即是 thread_info 的地址，通过
thread_info 又可以得到 task_struct 的地址进而得到进程 pid。

------------------------------------------------------------

### <span id="A01">thread_union 实现</span>

thread_info 与内核堆栈在内核中的定义位于: include/linux/sched.h
文件中，定义如下：

{% highlight c %}
union thread_union {
#ifndef CONFIG_ARCH_TASK_STRUCT_ON_STACK
        struct task_struct task;
#endif
#ifndef CONFIG_THREAD_INFO_IN_TASK
        struct thread_info thread_info;
#endif
        unsigned long stack[THREAD_SIZE/sizeof(long)];
};
{% endhighlight %}

上面的定义来自 linux 5.x 源码，内核定义一个联合体 thread_union
用于将内核堆栈和 thread_info 存储在 THREAD_SIZE 大小的空间里，
如果没有启用 CONFIG_ARCH_TASK_STRUCT_ON_STACK，那么联合体还包
含了一个 task_struct 结构。THREAD_SIZE 的定义位于：
arch/arm/include/asm/thread_info.h, 因此该宏的定义与体系有关，
例如 arm 中的定义如下：

{% highlight c %}
#define THREAD_SIZE_ORDER       1
#define THREAD_SIZE             (PAGE_SIZE << THREAD_SIZE_ORDER)
{% endhighlight %}

从上面的定义可知，如果 PAGE_SIZE 的大小是 4K 的话，那么
THREAD_SIZE 就是 8K。

--------------------------------------------------------------

### <span id="A02">thread_info 接口</span>

内核将 thread_info 与内核堆栈放在固定的空间里，内核只要知道堆栈
的位置，那么内核堆栈对应的 thread_info 结构的地址就可以获得。以下
是 linux 5.x 中 arm 的实现过程：

{% highlight c %}
static inline struct thread_info *current_thread_info(void)
{
        return (struct thread_info *)
                (current_stack_pointer & ~(THREAD_SIZE - 1));
}
{% endhighlight %}

arm 在 arch/arm/include/asm/thread_info.h 文件中，定义了
current_thread_info() 函数，该函数用于获得当前内核堆栈对应的
thread_info 结构，从上面的实现可以看出，current_stack_pointer
指向了当前内核堆栈的地址，然后将堆栈的地址按 THREAD_SIZE 对齐，
找到 THREAD_SIZE 的起始地址，以此获得 thread_info 结构。
current_stack_pointer 的定义如下：

{% highlight c %}
/*
 * how to get the current stack pointer in C
 */
register unsigned long current_stack_pointer asm ("sp");
{% endhighlight %}

从上面的定义可以看出，current_stack_pointer 直接返回了 sp 寄存器
的值，也就是堆栈的位置。

### <span id="A03">0 号进程的 thread_info</span>

在 linux 中，0 号进程就是内核启动之后的第一个进程，其通过 init_task
变量指定，其定义在 init/init_task.c 文件里，其中 stack 成员指向了
init_stack 结构。如下：

{% highlight c %}
/*
 * Set up the first task table, touch at your own risk!. Base=0,
 * limit=0x1fffff (=2MB)
 */
struct task_struct init_task
#ifdef CONFIG_ARCH_TASK_STRUCT_ON_STACK
        __init_task_data
#endif
= {
#ifdef CONFIG_THREAD_INFO_IN_TASK
        .thread_info    = INIT_THREAD_INFO(init_task),
        .stack_refcount = ATOMIC_INIT(1),
#endif
        .state          = 0,
        .stack          = init_stack,
        .usage          = ATOMIC_INIT(2),

...
{% endhighlight %}

从上面的定义中，可以找到 0 号进程的 stack 成员指向了
init_stack, 其定义在 include/asm-generic/vmlinux.lds.h
如下：

{% highlight c %}
#define INIT_TASK_DATA(align)                                           \
        . = ALIGN(align);                                               \
        __start_init_task = .;                                          \
        init_thread_union = .;                                          \
        init_stack = .;                                                 \
        KEEP(*(.data..init_task))                                       \
        KEEP(*(.data..init_thread_info))                                \
        . = __start_init_task + THREAD_SIZE;                            \
        __end_init_task = .;
{% endhighlight %}

0 号进程的 stack 定义位于链接脚本里，其中关于 init_stack 的
定义通过 INIT_TASK_DATA 宏进行定义，从上面的定义可以知道，
链接脚本定义了一个链接符号 __start_init_task 指向了堆栈开始的
地址，并且 init_thread_union 与 init_stack 的地址都指向这里，
正如之前讨论的，内核将 0 号进程的 thread_union 结构存放在这里，
并且将 __end_init_task 指向了堆栈的顶部，其大小正好是
THREAD_SIZE.

--------------------------------------------------

# <span id="A04">thread_info 实践</span>

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

[Thread info on GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/task/thread_info/current_thread_info)

{% highlight c %}
/*
 * Thread_info and kernel stack
 *
 * (C) 2019.07.01 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* thread */
#include <linux/sched.h>

static __init int thread_demo_init(void)
{
	struct thread_info *info;
	unsigned long stack;

	/* Obtain current thread_info address */
	info = current_thread_info();

	/* Obtian stack address */
	stack = (__force unsigned long)info;
	stack += THREAD_SIZE;

	printk("thread_info AD: %#lx\n", (__force unsigned long)info);
	printk("stack AD: %#lx\n", current_stack_pointer);
	printk("thread_union end AD: %#lx\n", stack);

	return 0;
}
device_initcall(thread_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 thread_info.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_THREAD_INFO
+       bool "thread_info"
+
+if BISCUITOS_THREAD_INFO
+
+config DEBUG_BISCUITOS_THREAD_INFO
+       bool "THREAD_INFO"
+
+endif # BISCUITOS_THREAD_INFO
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
+obj-$(CONFIG_BISCUITOS_THREAD_INFO)     += thread_info.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]thread_info
            [*]THREAD_INFO()
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
thread_info AD: 0x9e48e000
stack AD: 0x9e48fef8
thread_union end AD: 0x9e490000
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

运行的结果和理论一直，thread_info 与内核堆栈在同一个空间。

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
