---
layout: post
title:  "将红黑树部署到应用程序中"
date:   2019-05-23 05:30:30 +0800
categories: [HW]
excerpt: RB-Tree 将红黑树部署到应用程序中().
tags:
  - RBTREE
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000H.jpg)

> [Github: 用户空间红黑树](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Basic)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [红黑树部署方法](#红黑树部署方法)
>
> - [红黑树使用方法](#红黑树使用方法)
>
> - [附录](#附录)

-----------------------------------
<span id="红黑树部署方法"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000H.jpg)

## 红黑树部署方法

BiscuitOS 开源项目提供了一套用户空间使用的红黑树，开发者只要按照使用步骤就可以
轻松将红黑树部署到开发者自己的项目中。具体步骤如下：

##### 获取红黑树

开发者首先获得红黑树的源码文件，可以使用如下命令：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Basic/rbtree.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Basic/rbtree.h
{% endhighlight %}

通过上面的命令可以获得红黑树的源代码，其中 rbtree.c 文件内包含了红黑树的核心实现，
rbtree.h 中包含了调用红黑树的接口。

------------------------------
<span id="红黑树使用方法"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000K.jpg)

## 红黑树使用方法

开发者在获得红黑树的源码之后，参照如下命令将红黑树编译到自己的项目中，例如：

{% highlight ruby %}
CC=gcc

CFLAGS = -I./

SRC := rbtree.c rb_run.c

all: rbtree

rbtree: $(SRC)
	@$(CC) $(SRC) $(CFLAGS) -o $@
{% endhighlight %}

例如在上面的 Makefile 脚本中，需要使用 `-I./` 选项，将头文件搜索路径执行当前目录,
接着将 rbtree.c 一同编译到项目中，这样保证了可以在项目中调用红黑树的接口。接着
要在自己的源码中调用红黑树的接口，需要在源文件中引用 `rbtree.h` 头文件。

#### 实际例子

在下面的源文件中，引用了红黑树的接口，在程序中构建了一棵红黑树，开发者可以
参照这个文件构建，如下：

{% highlight ruby %}
/*
 * RB-Tree Manual.
 *
 * (C) 2019.05.14 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/* rbtree */
#include <rbtree.h>

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

/* rbroot */
static struct rb_root BiscuitOS_rb = RB_ROOT;

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

		/*
		 *        (this)
		 *         /  \
		 *        /    \
		 *  (little)   (big)
		 *
		 */
		if (result < 0)
			new = &((*new)->rb_right);
		else if (result > 0)
			new = &((*new)->rb_left);
		else
			return 0;
	}

	/* Add new node and rebalance tree */
	rb_link_node(&node->node, parent, new);
	rb_insert_color(&node->node, root);
}

int main()
{
	struct node *np, *n;
	struct rb_node *node;

	/* Insert rb_node */
	rbtree_insert(&BiscuitOS_rb, &node0);
	rbtree_insert(&BiscuitOS_rb, &node1);
	rbtree_insert(&BiscuitOS_rb, &node2);
	rbtree_insert(&BiscuitOS_rb, &node3);
	rbtree_insert(&BiscuitOS_rb, &node5);
	rbtree_insert(&BiscuitOS_rb, &node6);
	rbtree_insert(&BiscuitOS_rb, &node7);
	rbtree_insert(&BiscuitOS_rb, &node8);

	printf("Iterate over RBTree.\n");
	for (node = rb_first(&BiscuitOS_rb); node; node = rb_next(node))
		printf("%#lx ", rb_entry(node, struct node, node)->runtime);
	printf("\n");

	printf("Iterate over by postorder.\n");
	rbtree_postorder_for_each_entry_safe(np, n, &BiscuitOS_rb, node)
		printf("%#lx ", np->runtime);
	printf("\n");

	return 0;
}
{% endhighlight %}

完整实践例子可以查看下面教程：

> [用户空间红黑树最小实践]()

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
