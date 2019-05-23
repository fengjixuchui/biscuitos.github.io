---
layout: post
title:  "rbtree_postorder_for_each_entry_safe"
date:   2019-05-23 05:30:30 +0800
categories: [HW]
excerpt: RB-Tree rbtree_postorder_for_each_entry_safe().
tags:
  - RBTREE
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000H.jpg)

> [Github: rbtree_postorder_for_each_entry_safe](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/API/rbtree_postorder_for_each_entry_safe)
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
 * rbtree_postorder_for_each_entry_safe - iterate in post-order over rb_root of
 * given type allowing the backing memory of @pos to be invalidated
 *
 * @pos:        the 'type *' to use as a loop cursor.
 * @n:          another 'type *' to use as temporary storage
 * @root:       'rb_root *' of the rbtree.
 * @field:      the name of the rb_node field within 'type'.
 *
 * rbtree_postorder_for_each_entry_safe() provides a similar guarantee as
 * list_for_each_entry_safe() and allows the iteration to continue independent
 * of changes to @pos by the body of the loop.
 *
 * Note, however, that it cannot handle other modifications that re-order the
 * rbtree it is iterating over. This includes calling rb_erase() on @pos, as
 * rb_erase() may rebalance the tree, causing us to miss some nodes.
 */
#define rbtree_postorder_for_each_entry_safe(pos, n, root, field) \
        for (pos = rb_entry_safe(rb_first_postorder(root), typeof(*pos), field); \
             pos && ({ n = rb_entry_safe(rb_next_postorder(&pos->field), \
                        typeof(*pos), field); 1; }); \
             pos = n)
{% endhighlight %}

rbtree_postorder_for_each_entry_safe() 宏用于安全后序遍历红黑树。参数 pos 和参数
n 都是指向嵌套 rb_node 的结构；参数 root 指向红黑树的根；参数 field 用于指示
rb_node 在嵌套数据结构中的名字。函数首先调用 for 循环，在循环开始的时候，调用
rb_entry_safe() 安全获得第一个嵌套的数据，并也获得下一个嵌套的数据。依次遍历，
直到遍历完所有的节点。

> - [rb_entry_safe 源码分析](https://biscuitos.github.io/blog/Tree_RBTREE_rb_entry_safe/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)

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
 * (C) 2019.05.20 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* header of rbtree */
#include <linux/rbtree.h>
#include <linux/rbtree_augmented.h>

/* rbtree */
struct node {
	struct rb_node node;
	unsigned long runtime;
};

/*
 * RB-Tree
 *
 *                                                        [] Black node
 *                                                        () Red node
 *                    [4]
 *                     |
 *          o----------o----------o
 *          |                     |
 *         (2)                   (7)
 *          |                     |
 *   o------o------o      o-------o-------o
 *   |             |      |               |
 *  [1]           [3]    [5]             [9]
 *                                        |
 *                                o-------o-------o
 *                                |               |
 *                               (8)            (129)
 *
 *
 */
static struct node node0 = { .runtime = 0x1 };
static struct node node1 = { .runtime = 0x2 };
static struct node node2 = { .runtime = 0x3 };
static struct node node3 = { .runtime = 0x5 };
static struct node node4 = { .runtime = 0x4 };
static struct node node5 = { .runtime = 0x7 };
static struct node node6 = { .runtime = 0x8 };
static struct node node7 = { .runtime = 0x9 };
static struct node node8 = { .runtime = 0x129 };

/* root for rbtree */
struct rb_root BiscuitOS_rb = RB_ROOT;

/* Insert private node into rbtree */
static int rbtree_insert(struct rb_root *root, struct node *node)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct node *this = rb_entry(*new, struct node, node);
		int result;

		/* Compare runtime */
		result = this->runtime - node->runtime;

		/* setup parent */
		parent = *new;

		if (result > 0)
			new = &((*new)->rb_left);
		else if (result < 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}

	/* Add new node and reba&node0lance tree */
	rb_link_node(&node->node, parent, new);
	rb_insert_color(&node->node, root);

	return 1;
}

/* Search private node on rbtree */
struct node *rbtree_search(struct rb_root *root, unsigned long runtime)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct node *this = rb_entry(node, struct node, node);
		int result;

		result = this->runtime - runtime;

		if (result > 0)
			node = node->rb_left;
		else if (result < 0)
			node = node->rb_right;
		else
			return this;
	}
	return NULL;
}

static __init int rbtree_demo_init(void)
{
	struct node *np, *n;

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


	/* Postorder iterate over RBtree */
	rbtree_postorder_for_each_entry_safe(np, n, &BiscuitOS_rb, node)
		printk("RB: %#lx ", np->runtime);

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
+       bool "rbtree"
+
+if BISCUITOS_RBTREE
+
+config DEBUG_BISCUITOS_RBTREE
+       bool "rbtree_postorder_for_each_entry_safe"
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
        [*]rbtree
            [*]rbtree_postorder_for_each_entry_safe()
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
RB: 0x1
RB: 0x3
RB: 0x2
RB: 0x5
RB: 0x8
RB: 0x129
RB: 0x9
RB: 0x7
RB: 0x4
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

后序遍历红黑树。

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Red Black Tress](https://biscuitos.github.io/blog/Tree_RBTree/)
>
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
