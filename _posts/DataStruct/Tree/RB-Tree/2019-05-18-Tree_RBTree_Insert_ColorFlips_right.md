---
layout: post
title:  "父节点是祖父的右孩子，引起颜色翻转"
date:   2019-05-18 05:30:30 +0800
categories: [HW]
excerpt: TREE 父节点是祖父的右孩子，引起颜色翻转.
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000R.jpg)

> [Github: 插入一个红节点引起颜色翻转](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case6)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [红黑树插入一个红节点引起颜色翻转原理](#原理分析)
>
> - [红黑树插入一个红节点引起颜色翻转与 2-3 树的关系](#23Tree)
>
> - [红黑树插入一个红节点引起颜色翻转实践](#实践)
>
> - [附录](#附录)

-----------------------------------
<span id="原理分析"></span>

## 红黑树插入一个红节点引起颜色翻转

父节点是祖父节点的右孩子，新插入一个红节点，与父节点和祖父节点构成一个
临时的 4-node 的时候，即一个节点的两个子节点均为红色，此时需要颜色翻转。
颜色翻转的触发场景：

{% highlight ruby %}
1) 向 2-3 树中的 3 节点添加元素，新添加的元素要添加到 3 节点的最右侧.

2) 对应到红黑树中的情形是：新添加的红节点在黑节点右侧，黑节点的左侧还有一个红节点，
   红黑树中的这种形态对应到 2-3 树中就是一个临时的 4 节点；

3) 2-3 树中临时的 4 节点要拆成 3个2 节点，这种形态对应到红黑树中就是 3 个黑节点；

4) 2-3 树在拆成 3 个 2 节点后，要向上融合，3 个 2 节点中间的那个节点是要融合的，
   所以它是红色的；

5) 最后从结果看，从一添加的 “红-黑-红”，到拆完后形成的 “黑-红-黑”，正好形成颜色
   的翻转，即所谓的颜色翻转；
{% endhighlight %}

红黑树中涉及的一些结论：

{% highlight ruby %}
1) 红黑树中，如果一个黑节点左侧没有红色节点的话，它本身就代表 2-3 树中一个单独的 2 节点；

2) 3 个 2 节点对应到红黑树中表示的是这 3 个节点都是黑节点；

3) 2-3 树中，临时的 4 节点在拆完后还要向上融合，融合意味着，2-3 树中临时 4 节点在拆
   完后向上融合的根，在红黑树中是红色的；
{% endhighlight %}

##### 核心代码实现

为了实现上面讨论的颜色翻转操作，参考内核中的实现进行分析，位于 lib/rbtree.c 文件中，
实现如下：

{% highlight ruby %}
tmp = gparent->rb_right;
if (parent == tmp) { /* parent == gparent->rb_right */
        tmp = gparent->rb_left;
        if (tmp && rb_is_red(tmp)) {
                /* Case 1 - color flips
                 *
                 *       G            g
                 *      / \          / \
                 *     U   P  -->   U   P
                 *          \            \
                 *           n            n
                 *
                 * However, since g's parent might be reg, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
        }
{% endhighlight %}

首先确认父节点是祖父节点的右孩子，新插入的节点是父节点的右孩子，叔叔节点是红节点，但插入
新节点到父节点的右边，此时由于不满足红黑树的性质，需要将父节点和叔叔节点都设置为黑色。
然后将祖父节点设置为红色节点。此时颜色翻转已经完成。此时由于祖父节点还是一个红节点，需要
继续与更上层的节点进行操作，但本文只讨论颜色翻转，因此这里不继续讨论下去。

-----------------------------------
<span id="23Tree"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

### 红黑树插入一个红节点引起颜色翻转与 2-3 树的关系

毕竟红黑树是 2-3 树的一种表现形式，因此插入一个红节点到引起红黑树颜色翻转原理也符合 2-3 树
的原理。由于父节点是祖父节点的右孩子，那么祖父节点是一个 2- 节点。父节点此时是一个黑节点，
叔叔节点也是一个红色节点，但插入红节点之后，祖父节点，叔叔节点与父节点一同组成了一个 4- 节点，
但这个 4- 节点的颜色是 "红-黑-红"，因此此时需要对父节点向上融合，因此父节点的颜色设置
为红色，两个孩子节点设置为黑节点，这样一个 4- 节点被拆成了三个 2- 节点，此时颜色翻转完成，
但是由于父节点还是一个红节点，需要等待与更上一层的节点进行融合。由于本文之讲解颜色翻转，
因此不继续讨论下去，如下图。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000097.png)

如上图，在为添加红节点之前，p0 节点是一个 2- 节点，当添加一个红节点 n0 的时候，此时
n1、p0、n2 节点构成了一个零时的 4- 节点，需要分裂和提取操作，将一个 4- 节点分裂成 3 个
2- 节点，因此将 n1 与 n2 节点设置为黑色节点，由于 p0 节点需要继续向上融合，因此
需要将 p0 设置为红色。此时颜色翻转完成。更多红黑树与 2-3 树的关系请看文档：

> [红黑树与 2-3 树的关系分析](https://biscuitos.github.io/blog/Tree_2-3-tree/)

--------------------------------------------------
<span id="实践"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000D.jpg)

# 红黑树插入一个红节点引起颜色翻转实践

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert/Case6)

开发者可以从上面的链接中获得所有的实践代码，也可以使用如下命令获得：

{% highlight ruby %}
mkdir -p Insert_ROOT
cd Insert_ROOT
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case6/Makefile
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case6/README.md
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case6/rb_run.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case6/rbtree.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Insert/Case6/rbtree.h
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
static struct node node1 = { .runtime = 0x10 };
static struct node node2 = { .runtime = 0x30 };
static struct node node3 = { .runtime = 0x40 };

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
	rbtree_insert(&BiscuitOS_rb, &node3);
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
0x10 0x20 0x30 0x40
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
