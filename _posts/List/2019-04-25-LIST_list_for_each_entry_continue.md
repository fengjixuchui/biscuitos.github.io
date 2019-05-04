---
layout: post
title:  "list_for_each_entry_continue"
date:   2019-05-04 14:02:30 +0800
categories: [HW]
excerpt: Bidirect-list list_for_each_entry_continue().
tags:
  - Bidirect-list
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000L.jpg)

> [Github: list_for_each_entry_continue](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/list/bindirect-list/API/list_for_each_entry_continue)
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

{% highlight ruby %}
/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:        the type * to use as a loop cursor.
 * @head:       the head for your list.
 * @member:     the name of the list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define list_for_each_entry_continue(pos, head, member)                 \
        for (pos = list_next_entry(pos, member);                        \
             &pos->member != (head);                                    \
             pos = list_next_entry(pos, member))
{% endhighlight %}

list_for_each_entry_continue() 函数用于从指定入口开始遍历双链表中的入口。该函数与
list_prepare_entry() 配合使用，list_prepare_entry() 函数用于指定入口的地址。
参数 pos 指定入口地址；head 参数用于指定双链表的表头；member 参数用于指定链表在入口结构
中的名字。函数使用 for 循环，调用 list_next_entry() 函数从 pos 入口开始遍历双链表，
并使用 list_next_entry() 函数获得下一个入口。

###### list_next_entry

> [list_next_entry 源码分析](https://biscuitos.github.io/blog/LIST_list_next_entry/)

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
 * bindirect-list
 *
 * (C) 20179.04.25 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * bidirect-list
 *
 * +-----------+<--o    +-----------+<--o    +-----------+<--o    +-----------+
 * |           |   |    |           |   |    |           |   |    |           |
 * |      prev |   o----| prev      |   o----| prev      |   o----| prev      |
 * | list_head |        | list_head |        | list_head |        | list_head |
 * |      next |---o    |      next |---o    |      next |---o    |      next |
 * |           |   |    |           |   |    |           |   |    |           |
 * +-----------+   o--->+-----------+   o--->+-----------+   o--->+-----------+
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* header of list */
#include <linux/list.h>

/* private structure */
struct node {
    const char *name;
    struct list_head list;
};

/* Initialize a group node structure */
static struct node node0 = { .name = "BiscuitOS_node0", };
static struct node node1 = { .name = "BiscuitOS_node1", };
static struct node node2 = { .name = "BiscuitOS_node2", };
static struct node node3 = { .name = "BiscuitOS_node3", };
static struct node node4 = { .name = "BiscuitOS_node4", };
static struct node node5 = { .name = "BiscuitOS_node5", };
static struct node node6 = { .name = "BiscuitOS_node6", };

/* Declaration and implement a bindirect-list */
LIST_HEAD(BiscuitOS_list);

static __init int bindirect_demo_init(void)
{
	struct node *np;
	struct node *np2;

	/* add a new entry on special entry */
	list_add_tail(&node0.list, &BiscuitOS_list);
	list_add_tail(&node1.list, &BiscuitOS_list);
	list_add_tail(&node2.list, &BiscuitOS_list);
	list_add_tail(&node3.list, &BiscuitOS_list);
	list_add_tail(&node4.list, &BiscuitOS_list);
	list_add_tail(&node5.list, &BiscuitOS_list);
	list_add_tail(&node6.list, &BiscuitOS_list);

	printk("BiscuitOS_list:\n");
	/* Traverser all node on bindirect-list */
	list_for_each_entry(np, &BiscuitOS_list, list)
		printk("%s\n", np->name);

	np = &node2;
	/* prepare a pos exist entry. */
	np2 = list_prepare_entry(np, &node0.list, list);
	printk("Prepare exist entry: %s\n", np2->name);

	/* continue iteration over list of given type */
	list_for_each_entry_continue(np2, &BiscuitOS_list, list)
		printk("%s\n", np2->name);

	return 0;
}
device_initcall(bindirect_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 list.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_LIST
+       bool "Bindirect-list"
+
+if BISCUITOS_LIST
+
+config DEBUG_BISCUITOS_LIST
+       bool "list_for_each_entry_continue"
+
+endif # BISCUITOS_LIST
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
+obj-$(CONFIG_BISCUITOS_LIST)    += list.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]Bindirect-list
            [*]list_for_each_entry_continue()
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
ledtrig-cpu: registered to indicate activity on CPUs
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
BiscuitOS_list:
BiscuitOS_node0
BiscuitOS_node1
BiscuitOS_node2
BiscuitOS_node3
BiscuitOS_node4
BiscuitOS_node5
BiscuitOS_node6
Prepare exist entry: BiscuitOS_node2
BiscuitOS_node3
BiscuitOS_node4
BiscuitOS_node5
BiscuitOS_node6
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

与 list_prepare_entry() 配合使用可以从特定位置正序遍历双链表节点对应的入口。

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
