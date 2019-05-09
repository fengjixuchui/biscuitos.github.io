---
layout: post
title:  "rb_parent"
date:   2019-05-09 08:36:30 +0800
categories: [HW]
excerpt: RBTREE rb_parent().
tags:
  - RBTREE
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000S.jpg)

> [Github: rb_parent](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/rbtree/API/rb_parent)
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
#define rb_parent(r)   ((struct rb_node *)((r)->__rb_parent_color & ~3))
{% endhighlight %}

rb_parent() 函数用于获得 rb_node 的父节点。在 struct rb_node 结构里，成员
__rb_parent_color 即包含了父节点的地址，也包含了节点颜色，节点颜色位于
__rb_parent_color 成员的低 2 位，所以要获得父节点的信息，需要先去掉颜色位。

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
 * rbtree
 *
 * (C) 2019.05.08 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* header of rbtree */
#include <linux/rbtree.h>

/* rbtree */
struct node {
	struct rb_node node;
	unsigned long runtime;
};

static struct node node0 = { .runtime = 0x20 };
static struct node node1 = { .runtime = 0x40 };
static struct node node2 = { .runtime = 0x60 };
static struct node node3 = { .runtime = 0x10 };
static struct node node4 = { .runtime = 0x01 };
static struct node node5 = { .runtime = 0x53 };
static struct node node6 = { .runtime = 0x24 };
static struct node node7 = { .runtime = 0x89 };
static struct node node8 = { .runtime = 0x56 };
static struct node node9 = { .runtime = 0x43 };

/* root for rbtree */
struct rb_root BiscuitOS_rb = RB_ROOT;

/* Insert private node into rbtree */
static int rbtree_insert(struct rb_root *root, struct node *node)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct node *this = container_of(*new, struct node, node);
		int result;

		/* Compare runtime */
		result = this->runtime - node->runtime;

		/* setup parent */
		parent = *new;

		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}

	/* Add new node and rebalance tree */
	rb_link_node(&node->node, parent, new);
	rb_insert_color(&node->node, root);

	return 1;
}

/* Search private node on rbtree */
struct node *rbtree_search(struct rb_root *root, unsigned long runtime)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct node *this = container_of(node, struct node, node);
		int result;

		result = this->runtime - runtime;

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return this;
	}
	return NULL;
}

static __init int rbtree_demo_init(void)
{
	struct rb_node *np;
	struct node *this;

	/* Insert rb_node */
	rbtree_insert(&BiscuitOS_rb, &node0);
	rbtree_insert(&BiscuitOS_rb, &node1);
	rbtree_insert(&BiscuitOS_rb, &node2);
	rbtree_insert(&BiscuitOS_rb, &node3);
	rbtree_insert(&BiscuitOS_rb, &node4);
	rbtree_insert(&BiscuitOS_rb, &node5);
	rbtree_insert(&BiscuitOS_rb, &node6);
	rbtree_insert(&BiscuitOS_rb, &node7);
	rbtree_insert(&BiscuitOS_rb, &node8);
	rbtree_insert(&BiscuitOS_rb, &node9);

	/* Traverser all node on rbtree */
	for (np = rb_first(&BiscuitOS_rb); np; np = rb_next(np))
		printk("RB: %#lx\n", rb_entry(np, struct node, node)->runtime);

	/* Search node by runtime */
	this = rbtree_search(&BiscuitOS_rb, 0x53);
	if (this) {
		struct rb_node *parent;

		/* Obtain rb_node's parent */
		parent = rb_parent(&this->node);
		if (parent)
			printk("%#lx's parent is %#lx\n", this->runtime,
				rb_entry(parent, struct node, node)->runtime);
		else
			printk("illegae child\n");

	} else
		printk("Invalid data on rbtree\n");

	/* delete rb_node */
	rb_erase(&node0.node, &BiscuitOS_rb);
	rb_erase(&node3.node, &BiscuitOS_rb);
	rb_erase(&node4.node, &BiscuitOS_rb);
	rb_erase(&node6.node, &BiscuitOS_rb);
	printk("Remove: %#lx %#lx %#lx %#lx\n", node0.runtime, node3.runtime,
				node4.runtime, node6.runtime);

	printk("Traverser all node again\n");
	/* Traverser all node again */
	for (np = rb_first(&BiscuitOS_rb); np; np = rb_next(np))
		printk("RB: %#lx\n", rb_entry(np, struct node, node)->runtime);

	return 0;
}
device_initcall(rbtree_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 rbtree.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_RBTREE
+       bool "RBTREE"
+
+if BISCUITOS_RBTREE
+
+config DEBUG_BISCUITOS_RBTREE
+       bool "rb_parent"
+
+endif # BISCUITOS_RBTREE
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
+obj-$(CONFIG_BISCUITOS_RBTREE)  += rbtree.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]RBTREE
            [*]rb_parent()
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
RB: 0x89
RB: 0x60
RB: 0x56
RB: 0x53
RB: 0x43
RB: 0x40
RB: 0x24
RB: 0x20
RB: 0x10
RB: 0x1
0x53's parent is 0x60
Remove: 0x20 0x10 0x1 0x24
Traverser all node again
RB: 0x89
RB: 0x60
RB: 0x56
RB: 0x53
RB: 0x43
RB: 0x40
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在某些需求中，需要获得 rb_node 对应的父节点信息。

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
