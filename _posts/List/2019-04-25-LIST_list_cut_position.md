---
layout: post
title:  "list_cut_position"
date:   2019-04-26 14:02:30 +0800
categories: [HW]
excerpt: Bidirect-list list_cut_position().
tags:
  - Bidirect-list
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000L.jpg)

> [Github: list_cut_position](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/list/bindirect-list/API/list_cut_position)
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
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *      and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void list_cut_position(struct list_head *list,
                struct list_head *head, struct list_head *entry)
{
        if (list_empty(head))
                return;
        if (list_is_singular(head) &&
                (head->next != entry && head != entry))
                return;
        if (entry == head)
                INIT_LIST_HEAD(list);
        else
                __list_cut_position(list, head, entry);
}
{% endhighlight %}

list_cut_position() 函数用于将一个链表切成两个链表。参数 list 指向一个新的链表，
参数 head 指向被拆开的链表 (原始链表)，entry 参数指向拆开的位置。函数首先调用
list_empty() 函数确定 head 链表是不是空链表，如果是则直接返回；如果不是空链表，
则调用 list_is_singular() 函数确定 head 链表是不是单一节点的链表，如果是则判断
head 链表的 next 不指向 entry 参数，并且 head 不是 entry，那么直接返回；反之
不是，则判断 entry 与 head 的关系，如果 entry 就是 head，那么代表新链表是空链表，
那么直接调用 INIT_LIST_HEAD() 函数初始化 list 链表；反之调用 __list_cut_position()
将 head 链表拆成两段，第一个节点到 entry 节点的链表使用 list 指定，entry 到最后一个
节点通过 head 指向。

###### list_is_singular

> [list_is_singular 源码](https://biscuitos.github.io/blog/LIST_list_is_singular/)

###### INIT_LIST_HEAD

> [INIT_LIST_HEAD 源码](https://biscuitos.github.io/blog/LIST_INIT_LIST_HEAD/)

###### __list_cut_position

> [\_\_list_cut_position 源码](https://biscuitos.github.io/blog/LIST___list_cut_position/)

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
LIST_HEAD(BiscuitOS_blist);

static __init int bindirect_demo_init(void)
{
	struct node *np;

	/* add a new entry on special entry */
	list_add_tail(&node0.list, &BiscuitOS_list);
	list_add_tail(&node1.list, &BiscuitOS_list);
	list_add_tail(&node2.list, &BiscuitOS_list);
	list_add_tail(&node3.list, &BiscuitOS_list);
	list_add_tail(&node4.list, &BiscuitOS_list);
	list_add_tail(&node5.list, &BiscuitOS_list);
	list_add_tail(&node6.list, &BiscuitOS_list);

	/* Cut BiscuitOS_list and cat into BiscuitOS_blist */
	list_cut_position(&BiscuitOS_blist, &BiscuitOS_list, &node3.list);

	printk("BiscuitOS_list:\n");
	/* Traverser all node on bindirect-list */
	list_for_each_entry(np, &BiscuitOS_list, list)
		printk("%s\n", np->name);

	printk("BiscuitOS_blist:\n");
	/* Traverser all node on bindirect-list */
	list_for_each_entry(np, &BiscuitOS_blist, list)
		printk("%s\n", np->name);

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
+       bool "list_cut_position"
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
            [*]list_cut_position()
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
BiscuitOS_list:
BiscuitOS_node4
BiscuitOS_node5
BiscuitOS_node6
BiscuitOS_blist:
BiscuitOS_node0
BiscuitOS_node1
BiscuitOS_node2
BiscuitOS_node3
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在需要将一个链表拆分成两个链表的时候，可以使用 list_cut_position() 函数完成功能。

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
