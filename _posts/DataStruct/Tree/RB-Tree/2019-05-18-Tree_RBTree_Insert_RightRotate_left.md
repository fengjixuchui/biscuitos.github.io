---
layout: post
title:  "红黑树右旋：父节点是祖父的右孩子，引起的右旋转"
date:   2019-05-18 05:30:30 +0800
categories: [HW]
excerpt: TREE 红黑树插入操作之：父节点是祖父的右孩子，引起的右旋转.
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

> [Github: 插入一个红节点引起右旋转](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case7)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [红黑树插入一个红节点引起右旋转原理](#原理分析)
>
> - [红黑树插入一个红节点引起右旋转与 2-3 树的关系](#23Tree)
>
> - [红黑树插入一个红节点引起右旋转实践](#实践)
>
> - [附录](#附录)

-----------------------------------
<span id="原理分析"></span>

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000Y.jpg)

# 红黑树插入一个红节点引起右旋转原理

![DTS](/assets/PDB/BiscuitOS/boot/BOOT100001.gif)

对结点 S 做右旋操作时，假设其左孩子为 E 而不是T.nil, 以 S 到 E 的链为 “支轴” 进
行。使 E 成为该子树新的根结点， S 成为 E 的右孩子，E 的右孩子成为 S 的左孩子。

![DTS](/assets/PDB/BiscuitOS/boot/BOOT000077.png)

如上图，当插入 4 之后，红黑树 5 节点需要进行右旋达到平衡，那么以 5 到 6 的链为
"支轴" 进行。使用 5 节点成为新的根节点， 6 成为 5 的右孩子，4 称为 5 的左
孩子。如下图：

![DTS](/assets/PDB/BiscuitOS/boot/BOOT000078.png)

##### 核心代码实现

为了实现右旋操作，参考内核中的实现进行分析，位于 lib/rbtree.c 文件中，关于右旋的
实现如下：

{% highlight ruby %}
tmp = parent->rb_left;
if (node == tmp) {
        /* Case 2 - right rotate at parent
         *
         *      G             G
         *     / \           / \
         *    U   p  -->    U   n
         *       /               \
         *      n                 p
         *
         * This still leaves us in violation of 4), the
         * continuation into Case 3 will fix that.
         */
        tmp = node->rb_right;
        parent->rb_left = tmp;
        node->rb_right = parent;
        if (tmp)
                rb_set_parent_color(tmp, parent,
                                        RB_BLACK);
        rb_set_parent_color(parent, node, RB_RED);
        augment_rotate(parent, node);
        parent = node;
        tmp = node->rb_left;
{% endhighlight %}

核心代码如上，此时父节点是祖父节点的右节点，新加入的红节点是父节点的左孩子。首先将 tmp
指向插入节点的右孩子，然后进行右旋转操作，将父节点的左孩子指向 tmp，然后插入节点的右孩子
指向父节点，如果插入节点的右孩子存在，那么将其父节点指向 parent，并设置为黑色，最后将
node 节点设置为红节点，以此向上融合。进过上面的操作，只能暂时然红黑树局部达到平衡，由于
此时新的父节点已经变成红色，需要父节点继续和祖父节点进行融合或者分离操作。

-----------------------------------
<span id="23Tree"></span>

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

### 红黑树插入一个红节点引起右旋转与 2-3 树的关系

毕竟红黑树是 2-3 树的一种表现形式，因此插入一个红节点到引起红黑树右旋转的原理也符合 2-3 树
的原理。由于父节点是祖父节点的右孩子，那么祖父节点是一个 2- 节点。父节点此时是一个红节点，
在向其左侧添加一个红节点，那么此时父节点和子节点都是红节点，不符合 2-3 树的要求，引起需要
对此处进行分裂，分裂的逻辑就是提起左孩子作为父节点，右边的节点成为父节点的右孩子，从红黑树
角度来看，就是一次右旋操作。

![](/assets/PDB/BiscuitOS/boot/BOOT000094.png)

如上图，在 2-3 树中，祖父节点 p0 是一个黑节点，n0 是其右孩子，且是一个红孩子，此时插入 n1
节点，此时 n1 也是一个红孩子，那么 n1 和 n0 构成一个零时的节点，这个节点都是红色，不符合
2-3 树的要求，因此需要分裂成两个 2- 节点，因此将节点右旋转，让 n1 成为父节点，n0 成为 n1
的右孩子，n1 的右孩子在右旋转过程中，变成了 n0 的左孩子，并且是一个 2- 节点，因此是一个
黑节点。经过右旋转之后，形成新的布局，此时并未满足 2-3 树的要求，需要将 n1 节点继续和 p0
节点做左旋转操作，但是本文只介绍右旋转，因此这里不继续讨论。更多红黑树与 2-3 树
的关系请看文档：

> [红黑树与 2-3 树的关系分析](/blog/Tree_2-3-tree/)

--------------------------------------------------
<span id="实践"></span>

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000D.jpg)

# 红黑树插入一个红节点引起右旋转实践

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case7)

开发者可以从上面的链接中获得所有的实践代码，也可以使用如下命令获得：

{% highlight ruby %}
mkdir -p Insert_ROOT
cd Insert_ROOT
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case7/Makefile
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case7/README.md
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case7/rb_run.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case7/rbtree.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case7/rbtree.h
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
static struct node node2 = { .runtime = 0x25 };

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
	rbtree_insert(&BiscuitOS_rb, &node2);
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
0x20 0x25 0x30
{% endhighlight %}

--------------------------------------

#### <span id="运行分析">运行分析</span>

在实践代码中，使用二叉查找树的方法，向红黑树插入红节点，然后调用 rb_insert_color()
红黑树实际插入操作。开发者可以使用调试工具跟踪 rb_insert_color() 的调用过程。

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [红黑树与 2-3 树的关系](https://www.jianshu.com/p/57a0329b2801)
>
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
