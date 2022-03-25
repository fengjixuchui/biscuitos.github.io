---
layout: post
title:  "AVL 平衡二叉搜索树"
date:   2019-05-12 05:30:30 +0800
categories: [HW]
excerpt: TREE AVL 树.
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

> [Github: AVL 树](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/binary-tree/Class/AVL)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [AVL 树原理](#原理)
>
> - [AVL 树实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="原理">AVL 树原理</span>

![DTS](/assets/PDB/BiscuitOS/boot/BOOT000074.png)

平衡二叉搜索树，又被称为 AVL 树，且具有以下性质：它是一棵空树或它的左右两个子树的高度
差的绝对值不超过 1，并且左右两个子树都是一棵平衡二叉树。由于普通的二叉查找树会容易失去
"平衡，极端情况下，二叉查找树会退化成线性的链表，导致插入和查找的复杂度下降到 O(n) ，
所以，这也是平衡二叉树设计的初衷。平衡二叉树保持平衡的方法，根据定义，有两个重点，
一是左右两子树的高度差的绝对值不能超过 1，二是左右两子树也是一颗平衡二叉树。下面
通过一个实践进一步认知 AVL 树。

--------------------------------------------------

# <span id="实践">实践</span>

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 binary.c on GitHub](https://github.com/BiscuitOS/HardStack/blob/master/Algorithem/tree/binary-tree/Class/AVL/binary.c)

开发者也可以使用如下命令获得：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/binary-tree/Class/AVL/binary.c
{% endhighlight %}

实践源码具体内容如下：

{% highlight c %}
/*
 * Binary-Tree.
 *
 * (C) 2019.05.12 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/* binary-tree node */
struct binary_node {
	int idx;
	struct binary_node *left;
	struct binary_node *right;
};

/* AVL Tree
 *                               (200)
 *                                 |
 *                 o---------------+---------------o
 *                 |                               |
 *                (7)                            (289)
 *                 |                               |
 *          o------+-----o                         +-----o
 *          |            |                               |
 *         (3)          (9)                            (740)
 *          |            |
 *      o---+            +---o
 *      |                    |
 *     (1)                 (12)
 */
static int AVL_data[] = {
                                  200, 7, 3, 1, -1, -1, -1, 9, -1,
                                  12, -1, -1, 289, -1, 740, -1, -1 };

static int counter = 0;
static int *BinaryTree_data = AVL_data;

/* Preorder Create Binary-tree */
static struct binary_node *Preorder_Create_BinaryTree(struct binary_node *node)
{
	int ch = BinaryTree_data[counter++];

	/* input from terminal */
	if (ch == -1) {
		return NULL;
	} else {
		node =
		   (struct binary_node *)malloc(sizeof(struct binary_node));
		node->idx = ch;

		/* Create left child */
		node->left  = Preorder_Create_BinaryTree(node->left);
		/* Create right child */
		node->right = Preorder_Create_BinaryTree(node->right);
		return node;
	}
}

/* Midd-Traverse Binary-Tree */
static void Middorder_Traverse_BinaryTree(struct binary_node *node)
{
	if (node == NULL) {
		return;
	} else {
		Middorder_Traverse_BinaryTree(node->left);
		printf("%d ", node->idx);
		Middorder_Traverse_BinaryTree(node->right);
	}
}

/* Post-Free BinaryTree */
static void Postorder_Free_BinaryTree(struct binary_node *node)
{
	if (node == NULL) {
		return;
	} else {
		Postorder_Free_BinaryTree(node->left);
		Postorder_Free_BinaryTree(node->right);
		free(node);
		node = NULL;
	}
}

int main()
{
	/* Define binary-tree root */
	struct binary_node *BiscuitOS_root;

	printf("Preorder Create BinaryTree\n");
	BiscuitOS_root = Preorder_Create_BinaryTree(BiscuitOS_root);

	/* Middorder traverse binary-tree */
	printf("Middorder Traverse Binary-Tree:\n");
	Middorder_Traverse_BinaryTree(BiscuitOS_root);
	printf("\n");

	/* Postorder free binary-tree */
	Postorder_Free_BinaryTree(BiscuitOS_root);

	return 0;
}
{% endhighlight %}

--------------------------------------

#### <span id="源码编译">源码编译</span>

将实践源码保存为 binary.c，然后使用如下命令进行编译：

{% highlight ruby %}
gcc bianry.c -o binary
{% endhighlight %}

--------------------------------------

#### <span id="源码运行">源码运行</span>

实践源码的运行很简单，可以使用如下命令，并且运行结果如下：

{% highlight ruby %}
binary-tree/Class/AVL$ ./binary
Preorder Create BinaryTree
Middorder Traverse Binary-Tree:
1 3 7 9 12 200 289 740
{% endhighlight %}

--------------------------------------

#### <span id="运行分析">运行分析</span>

从运行的结果可以看出，在使用中序遍历二叉树的时候，所以的数据都是从小到大排序的，因此
这棵树是一棵 AVL 树。

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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
