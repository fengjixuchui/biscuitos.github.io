---
layout: post
title:  "红黑树右旋实践"
date:   2019-05-15 05:30:30 +0800
categories: [HW]
excerpt: TREE 红黑树右旋实践.
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000L.jpg)

> [Github: 红黑树右旋实践](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Rotate/Right_Rotate)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [红黑树右旋](#原理分析)
>
> - [红黑树右旋实践](#实践)
>
> - [红黑树右旋与 2-3 树的关系](#RB23)
>
> - [附录](#附录)

-----------------------------------

# <span id="原理分析">红黑树右旋</span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT100001.gif)

对结点 S 做右旋操作时，假设其左孩子为 E 而不是T.nil, 以 S 到 E 的链为 “支轴” 进
行。使 E 成为该子树新的根结点， S 成为 E 的右孩子，E 的右孩子成为 S 的左孩子。

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000077.png)

如上图，当插入 4 之后，红黑树 5 节点需要进行右旋达到平衡，那么以 5 到 6 的链为
"支轴" 进行。使用 5 节点成为新的根节点， 6 成为 5 的右孩子，4 称为 5 的左
孩子。如下图：

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000078.png)

##### 核心代码实现

为了实现右旋操作，参考内核中的实现进行分析，位于 lib/rbtree.c 文件中，关于右旋的
实现如下：

{% highlight ruby %}
		tmp = gparent->rb_right;
		if (parent != tmp) { /* parent == gparent->rb_left */
			tmp = parent->rb_right;

			gparent->rb_left = tmp;
			parent->rb_right = gparent;
			if (tmp)
				rb_set_parent_color(tmp, gparent, RB_BLACK);
			__rb_rotate_set_parents(gparent, parent, root, RB_RED);
			augment_rotate(gparent, parent);
			break;
		}
{% endhighlight %}

核心代码首先判断 gparent (gparent 为 parent 的父节点) 的左孩子是否存在，对于需要右
旋的部分，gparent 的左孩子是存在的。接着按照右旋的原理，以 gparent 节点到 parent 节
点为支轴进行右旋。此时 parent 的右孩子变成了 gparent 的左孩子，对应的代码就是：
"gparent->rb_left = tmp", gparent 自己变成了 parent 的右孩子, 对应的代码就是：
"parent->rb_right = gparent"。如果此时 tmp 存在，也就是原先 parent 的右孩子存在，
那么，设置 tmp 的父节点为 gparent。接着调用 __rb_rotate_set_parents() 函数修改
gparent 和 parent 之间的关系，代码如下：

{% highlight ruby %}
static inline void
__rb_rotate_set_parents(struct rb_node *old, struct rb_node *new,
		       struct rb_root *root, int color)
{
	struct rb_node *parent = rb_parent(old);
	new->__rb_parent_color = old->__rb_parent_color;
	rb_set_parent_color(old, new, color);
	__rb_change_child(old, new, parent, root);
}
{% endhighlight %}

首先获得 gparent 的父节点，然后将 parent 在红黑树 __rb_parent_color 成员继承
gparent 的 __rb_parent_color. 然后设置 gparent 的 __rb_parent_color 为
parent。最后修改 gparent 原先的父节点的节点信息，函数如下：

{% highlight ruby %}
static inline void
__rb_change_child(struct rb_node *old, struct rb_node *new,
		  struct rb_node *parent, struct rb_root *root)
{
	if (parent) {
		if (parent->rb_left == old)
			parent->rb_left = new;
		else
			parent->rb_right = new;
	} else
		root->rb_node = new;
}
{% endhighlight %}

逻辑很简单，就是判断 gpraent 是原始父节点的左孩子还是右孩子，然后将原始父节点的
左孩子或右孩子指向 parent 节点。如果 gparent 的父节点不存在，那么 gparent 原先是
root 节点，那么就将 root 节点指向 parent 节点。

通过上面的源码，rbtree 已经完成右旋操作，并设置好了各个节点之间的关系，使红黑树再一次
达到平衡。

--------------------------------------------------

# <span id="实践">红黑树右旋实践</span>

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Rotate/Right_Rotate)

开发者可以从上面的链接中获得所有的实践代码，也可以使用如下命令获得：

{% highlight ruby %}
mkdir -p Right_Rotate
cd Right_Rotate
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Rotate/Right_Rotate/Makefile
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Rotate/Right_Rotate/README.md
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Rotate/Right_Rotate/rb_run.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Rotate/Right_Rotate/rbtree.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/rb-tree/Rotate/Right_Rotate/rbtree.h
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

static struct node node0 = { .runtime = 0x6 };
static struct node node1 = { .runtime = 0x5 };
static struct node node2 = { .runtime = 0x4 };

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
rb-tree/Rotate/Right_Rotate$ ./rbtree
0x4 0x5 0x6
{% endhighlight %}

--------------------------------------

#### <span id="运行分析">运行分析</span>

在实践代码中，使用中序遍历了红黑树，开发者可以调试跟踪代码的执行。

--------------------------------------
<span id="RB23"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Z.jpg)

# 红黑树与 2-3 树的关系

> [红黑树与 2-3 树的关系](https://biscuitos.github.io/blog/Tree_2-3-tree/#RB23)

### 红黑树右旋与 2-3 树的关系

右旋转触发的场景：新添加的元素要融合到 3节点的最左边；对应到红黑树中的场景是：
根节点是黑，根节点的左孩子是红，根节点左孩子的左孩子是红；旋转过后，3个节点还是
对应着 2-3 树中的临时 4 节点，所以左右 2 个孩子的颜色都得是红的，红-黑-红，
这就对应到了颜色翻转的情况了，继而进行颜色翻转；

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
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
