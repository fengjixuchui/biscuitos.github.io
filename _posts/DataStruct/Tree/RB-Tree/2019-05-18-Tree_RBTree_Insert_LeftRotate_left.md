---
layout: post
title:  "红黑树左旋：父节点是祖父的左孩子，引起的左旋转"
date:   2019-05-18 05:30:30 +0800
categories: [HW]
excerpt: TREE 红黑树左旋：父节点是祖父的左孩子，引起的左旋转.
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000R.jpg)

> [Github: 插入一个红节点引起左旋转](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case4)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [红黑树插入一个红节点引起左旋转原理](#原理分析)
>
> - [红黑树插入一个红节点引起左旋转与 2-3 树的关系](#23Tree)
>
> - [红黑树插入一个红节点引起左旋转实践](#实践)
>
> - [附录](#附录)

-----------------------------------
<span id="原理分析"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT100000.gif)

对结点 E 做左旋操作时，其右孩子为 S 而不是 T.nil，那么以 E 到 S 的链为
"支轴" 进行。使 S 成为该子树新的根结点，E 成为 S 的左孩子，E 的左孩子成为 S 的
右孩子.

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000075.png)

如上图，当插入 6 之后，红黑树 5 节点需要进行左旋达到平衡，那么以 4 到 5 的链为
"支轴" 进行。使用 5 节点称为 6 的新的根节点，4 称为 5 的左孩子，6 称为 5 的右
孩子。如下图：

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000076.png)

##### 核心代码实现

为了实现左旋操作，参考内核中的实现进行分析，位于 lib/rbtree.c 文件中，关于左旋的
实现如下：

{% highlight ruby %}
tmp = gparent->rb_right;
if (parent != tmp) { /* parent == gparent->rb_left */
        tmp = parent->rb_right;
        if (node == tmp) {
                /*
                 * Case 2 - node's uncle is black and node is
                 * the parent's right child (left rotate at
                 * parent).
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in volation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                tmp = node->rb_left;
                parent->rb_right = tmp;
                node->rb_left = parent;
                if (tmp)
                        rb_set_parent_color(tmp, parent,
                                            RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->rb_right;
        }
{% endhighlight %}

核心代码首先确定了父节点是祖父节点的左孩子，而且新插入的节点是父节点的右孩子。代码
首先获得新插入节点的左孩子，使用 tmp 指向，然后将父节点的右孩子指向 tmp。接着将
插入节点的左孩子指向父节点，这样形成了基本的左旋转。如果 tmp 真实存在，那么将其父亲设置
为 parnet，并将自己设置为黑色，然后调用 rb_set_parent_color() 函数设置 parent
的颜色为红色，此时左旋已经完成，但是由于此时父节点与插入的节点都是红色，需要向上继续
旋转，但此处只分析左旋转操作，因此不继续分析下去。此时红黑树是不平衡的。

-----------------------------------
<span id="23Tree"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

### 红黑树插入一个红节点引起左旋转与 2-3 树的关系

毕竟红黑树是 2-3 树的一种表现形式，因此插入一个红节点到引起红黑树右旋转的原理也符合 2-3 树
的原理。由于父节点是祖父节点的左孩子，那么祖父节点是一个 2- 节点。父节点此时是一个红节点，
在向其右侧添加一个红节点，那么此时父节点和子节点都是红节点，与父节点和父节点的左孩子构成了一个
零时的 4- 节点，此时 4- 节点的颜色不对，需要对节点进行分裂，提取插入的节点为父节点，将 4-
节点拆分成 3 个 2- 节点，此时父节点变成了插入节点的子节点的左孩子，插入节点的左孩子变成了
父节点的右节点，此时完成了以此左旋转操作。此时父节点，插入的新节点与祖父节点构成了一个零时的
4- 节点，因此需要进行分裂操作，由于本文只分析左旋做，因此不继续讨论下去，如下图。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000096.png)

如上图，在插入之前，p0 与 n1 构成一个 3- 节点，但是由于 n0 红节点的插入，因此 n1 与 n0
之间不平衡，因此需要从 n1 与 n0 开始，构成一个 4- 零时节点，此时进行分裂操作，提取 n0
节点作为父节点，n1 作为 n1 的左孩子，n0 的左孩子变成 n1 的右孩子。此时从红黑树的角度完成
了一次左旋转操作。此时 n1,n0,p0 构成了一个 4- 节点，需要在进行分裂、提取、合并，但由于这里
只讨论左旋转，因此不继续讨论下去。更多红黑树与 2-3 树的关系请看文档：

> [红黑树与 2-3 树的关系分析](https://biscuitos.github.io/blog/Tree_2-3-tree/)

--------------------------------------------------
<span id="实践"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000D.jpg)

# 红黑树插入一个红节点引起左旋转实践

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case4)

开发者可以从上面的链接中获得所有的实践代码，也可以使用如下命令获得：

{% highlight ruby %}
mkdir -p Insert_ROOT
cd Insert_ROOT
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case4/Makefile
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case4/README.md
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case4/rb_run.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case4/rbtree.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case4/rbtree.h
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
static struct node node1 = { .runtime = 0x15 };
static struct node node2 = { .runtime = 0x18 };

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
rb-tree/Insert$ ./rbtree
0x15 0x18 0x20
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
