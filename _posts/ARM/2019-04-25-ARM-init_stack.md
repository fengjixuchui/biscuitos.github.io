---
layout:             post
title:              "内核堆栈"
date:               2019-04-25 08:18:30 +0800
categories:         [MMU]
excerpt:            内核堆栈.
tags:
  - MMU
---

> [GitHub FlameGraph](https://github.com/brendangregg/FlameGraph)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>
>
> Architecture: ARMv7(arm 32) Cortex-A9MP Linux 5.0


## 内核堆栈


## 内核堆栈 init_stack 定义

内核堆栈定义在 arch/arm/kernel/vmlinux.lds.S 里，如下：

{% highlight haskell %}
        _sdata = .;
        RW_DATA_SECTION(L1_CACHE_BYTES, PAGE_SIZE, THREAD_SIZE)
        _edata = .;
{% endhighlight %}

在 vmlinux.lds.S 中定义了内核镜像 vmlinux 中数据段的定义，vmlinux 将堆栈定义
在 _sdata 与 _edata 之间，通过宏 RW_DATA_SECTION 宏定义了堆栈，其定义如下：

{% highlight haskell %}
/*
 * Writeable data.
 * All sections are combined in a single .data section.
 * The sections following CONSTRUCTORS are arranged so their
 * typical alignment matches.
 * A cacheline is typical/always less than a PAGE_SIZE so
 * the sections that has this restriction (or similar)
 * is located before the ones requiring PAGE_SIZE alignment.
 * NOSAVE_DATA starts and ends with a PAGE_SIZE alignment which
 * matches the requirement of PAGE_ALIGNED_DATA.
 *
 * use 0 as page_align if page_aligned data is not used */
#define RW_DATA_SECTION(cacheline, pagealigned, inittask)               \
        . = ALIGN(PAGE_SIZE);                                           \
        .data : AT(ADDR(.data) - LOAD_OFFSET) {                         \
                INIT_TASK_DATA(inittask)                                \
                NOSAVE_DATA                                             \
                PAGE_ALIGNED_DATA(pagealigned)                          \
                CACHELINE_ALIGNED_DATA(cacheline)                       \
                READ_MOSTLY_DATA(cacheline)                             \
                DATA_DATA                                               \
                CONSTRUCTORS                                            \
        }                                                               \
        BUG_TABLE
{% endhighlight %}

从上面看出，vmlinux.lds.S 里定义了内核镜像 vmlinux 的 .data section，其中
INIT_TASK_DATA(inittask) 中定义了堆栈相关的信息，从上面的定义中可以看出
堆栈位于内核镜像的最前部，其定义如下：

{% highlight haskell %}
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

从上面的定义可以看出，内核在此处定义了三个变量 __start_init_task,
init_thread_task, 和 init_stack, 这三个变量的地址都是一样的。INIT_TASK_DATA
使用了 KEEP 关键字在此处将所有输入文件的 .data..init_task section 和
.data..init_thread_info section。接着使用链接脚本的定位符 "." 将虚拟地址
定位为 __start_init_task 的地址加上 THREAD_SIZE, 然后将 __end_init_task
变量指向这个地址，其中 THREAD_SIZE 的定义如下：

{% highlight haskell %}
#define THREAD_SIZE_ORDER       1
#define THREAD_SIZE             (PAGE_SIZE << THREAD_SIZE_ORDER)
{% endhighlight %}

从这里可以看出 THREAD_SIZE 占用了 2 个页，因此可以知道 __start_init_task
到 __end_init_task 之间的大小是 2 个页。此时 init_stack 的内存布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000054.png)
