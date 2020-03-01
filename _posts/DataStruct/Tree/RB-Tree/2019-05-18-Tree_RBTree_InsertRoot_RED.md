---
layout: post
title:  "红黑树插入操作之：插入一个红节点到黑根节点"
date:   2019-05-18 05:30:30 +0800
categories: [HW]
excerpt: TREE 红黑树插入操作之：插入一个红节点到黑根节点.
tags:
  - Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000R.jpg)

> [Github: 插入一个红节点到黑根节点](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case1)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [红黑树插入一个红节点到黑根节点](#原理分析)
>
> - [红黑树插入一个红节点到黑根节点与 2-3 树的关系](#23Tree)
>
> - [插入一个红节点到黑根节点实践](#实践)
>
> - [附录](#附录)

-----------------------------------
<span id="原理分析"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Y.jpg)

# 红黑树插入一个红节点到黑根节点

向一棵只有根节点的红黑树中插入一个红节点，那么该节点可以直接插入到红黑树，成为根节点的左孩子
或者右孩子。注意，每次插入的新节点都是红节点。

###### 核心代码实现

{% highlight ruby %}
static inline void
__rb_insert(struct rb_node *node, struct rb_root *root,
	    bool newleft, struct rb_node **leftmost,
	    void (*augment_rotate)(struct rb_node *old, struct rb_node *new))
{
	struct rb_node *parent = rb_red_parent(node), *gparent, *tmp;

	if (newleft)
		*leftmost = node;

	while (true) {
		/*
		 * Loop invariant: node is red.
		 */
		if (!parent) {
			/*
			 * The inserted node is root. Either this is the
			 * first node, or we recursed at Case 1 below and
			 * are no longer violating 4).
			 */
			rb_set_parent_color(node, NULL, RB_BLACK);
			break;
		}

		/*
		 * If there is a black parent, we are done.
		 * Otherwise, take some corrective action as,
		 * per 4), we don't want a red root or two
		 * consecutive red nodes.
		 */
		if (rb_is_black(parent))
			break;
	}
}
{% endhighlight %}

当调用 __rb_insert() 函数向红黑树插入一个红节点时候，此时红黑树中只有一个根节点，并且
根节点是黑色，所有此时可以直接插入，无需考虑翻转或变色。rb_is_black() 函数用于检查节点
是否为黑节点。

-----------------------------------
<span id="23Tree"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

# 红黑树插入一个红节点到黑根节点与 2-3 树的关系

毕竟红黑树是 2-3 树的一种表现形式，因此插入一个红节点到黑根节点的原理也符合 2-3 树的原理。
黑根节点在 2-3 树中就是一个 2- 节点，因此插入一个红节点之后，红节点与 2- 节点融合成一个 3-
节点，此时 3- 节点符合 2-3 树的要求，所有无需分裂提取。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000092.png)

如上图，在 2-3 树中，新加入的节点与 2- root 节点融合成一个 3- 节点，此时对应的红黑树分
布如上，更多红黑树与 2-3 树的关系请看文档：

> [红黑树与 2-3 树的关系分析](https://biscuitos.github.io/blog/Tree_2-3-tree/)

--------------------------------------------------
<span id="实践"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

# 红黑树插入根节点实践

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case1)

开发者可以从上面的链接中获得所有的实践代码，也可以使用如下命令获得：

{% highlight ruby %}
mkdir -p Insert_ROOT
cd Insert_ROOT
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case1/Makefile
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case1/README.md
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case1/rb_run.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case1/rbtree.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case1/rbtree.h
{% endhighlight %}

实践源码具体内容如下：

{% highlight c %}
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

/* n points to a rb_node */
#define node_entry(n) container_of(n, struct node, node)

static struct node node0 = { .runtime = 0x20 };
static struct node node1 = { .runtime = 0x30 };

/* rbroot */
static struct rb_root BiscuitOS_rb = RB_ROOT;

/* Insert private node into rbtree */
static int rbtree_insert(struct rb_root *root, struct node *node)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct node *this = node_entry(*new);
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

static int count = 20;
/* Middle-order iterate over RB tree */
static void Middorder_IterateOver(struct rb_node *node)
{
	if (!node) {
		return;
	} else {
		Middorder_IterateOver(node->rb_left);
		printf("%#lx ", node_entry(node)->runtime);
		Middorder_IterateOver(node->rb_right);
	}
}

int main()
{
	struct node *np;

	/* Insert rb_node */
	rbtree_insert(&BiscuitOS_rb, &node0);
	rbtree_insert(&BiscuitOS_rb, &node1);
	Middorder_IterateOver(BiscuitOS_rb.rb_node);
	printf("\n");

	return 0;
}
{% endhighlight %}

--------------------------------------

#### <span id="源码编译">源码编译</span>

使用如下命令进行编译：

{% highlight ruby %}
make
{% endhighlight %}

--------------------------------------

#### <span id="源码运行">源码运行</span>

实践源码的运行很简单，可以使用如下命令，并且运行结果如下：

{% highlight ruby %}
rb-tree/Insert/Case0$ ./rbtree
0x20 0x30
{% endhighlight %}

--------------------------------------

#### <span id="运行分析">运行分析</span>

在实践代码中，使用二叉查找树的方法，向红黑树的黑根节点插入红节点，然后调用 rb_insert_color()
红黑树实际插入操作。开发者可以使用调试工具跟踪 rb_insert_color() 的调用过程。

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [红黑树与 2-3 树的关系](https://www.jianshu.com/p/57a0329b2801)
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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
