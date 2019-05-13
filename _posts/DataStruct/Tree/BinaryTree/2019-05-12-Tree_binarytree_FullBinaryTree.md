---
layout: post
title:  "完满二叉树 Full Binary Tree"
date:   2019-05-12 05:30:30 +0800
categories: [HW]
excerpt: TREE 完满二叉树().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000T.jpg)

> [Github: 完满二叉树](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/binary-tree/Class/Full_BinaryTree)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [完满二叉树原理](#原理)
>
> - [完美二叉树实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="原理">完满二叉树原理</span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000072.png)

在二叉树中，所有非叶子结点的度都是 2。接下来通过一个实践进一步认识完美二叉树。

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

> [实践源码 binary.c on GitHub](https://github.com/BiscuitOS/HardStack/blob/master/Algorithem/tree/binary-tree/Class/Full_BinaryTree/binary.c)

开发者也可以使用如下命令获得：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/binary-tree/Class/Full_BinaryTree/binary.c
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

/* Full Binary Tree
 *                               (200)
 *                                 |
 *                 o---------------+---------------o
 *                 |                               |
 *               (143)                           (876)
 *                 |
 *          o------+-----o
 *          |            |
 *        (754)        (386)
 *                       |
 *                   o---+---o
 *                   |       |
 *                  (7)     (9)
 */
static int Full_BinaryTree_data[] = {
                               200, 143, 754, -1, -1, 386, 7, -1, -1,
                               9, -1, -1, 876, -1, -1 };

static int counter = 0;
static int *BinaryTree_data = Full_BinaryTree_data;

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

/* Pre-Traverse Binary-Tree */
static void Preorder_Traverse_BinaryTree(struct binary_node *node)
{
	if (node == NULL) {
		return;
	} else {
		printf("%d ", node->idx);
		/* Traverse left child */
		Preorder_Traverse_BinaryTree(node->left);
		/* Traverse right child */
		Preorder_Traverse_BinaryTree(node->right);
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

/* Post-Traverse Binary-Tree */
static void Postorder_Traverse_BinaryTree(struct binary_node *node)
{
	if (node == NULL) {
		return;
	} else {
		Postorder_Traverse_BinaryTree(node->left);
		Postorder_Traverse_BinaryTree(node->right);
		printf("%d ", node->idx);
	}
}

/* The deep for Binary-Tree */
static int BinaryTree_Deep(struct binary_node *node)
{
	int deep = 0;

	if (node != NULL) {
		int leftdeep  = BinaryTree_Deep(node->left);
		int rightdeep = BinaryTree_Deep(node->right);

		deep = leftdeep >= rightdeep ? leftdeep + 1: leftdeep + 1;
	}
	return deep;
}

/* Leaf counter */
static int BinaryTree_LeafCount(struct binary_node *node)
{
	static int count;

	if (node != NULL) {
		if (node->left == NULL && node->right == NULL)
			count++;

		BinaryTree_LeafCount(node->left);
		BinaryTree_LeafCount(node->right);
	}
	return count;
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

	/* Preoder traverse binary-tree */
	printf("Preorder Traverse Binary-Tree:\n");
	Preorder_Traverse_BinaryTree(BiscuitOS_root);
	printf("\n");

	/* Middorder traverse binary-tree */
	printf("Middorder Traverse Binary-Tree:\n");
	Middorder_Traverse_BinaryTree(BiscuitOS_root);
	printf("\n");

	/* Postorder traverse binary-tree */
	printf("Postorder Traverse Binary-Tree:\n");
	Postorder_Traverse_BinaryTree(BiscuitOS_root);
	printf("\n");

	/* The deep for Binary-Tree */
	printf("The Binary-Tree Deep: %d\n",
				BinaryTree_Deep(BiscuitOS_root));

	/* The leaf number for Binary-Tree */
	printf("The Binary-Tree leaf: %d\n",
				BinaryTree_LeafCount(BiscuitOS_root));

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
binary-tree/Class/Full_BinaryTree$ ./binary
Preorder Create BinaryTree
Preorder Traverse Binary-Tree:
200 143 754 386 7 9 876
Middorder Traverse Binary-Tree:
754 143 7 386 9 200 876
Postorder Traverse Binary-Tree:
754 7 9 386 143 876 200
The Binary-Tree Deep: 3
The Binary-Tree leaf: 4
{% endhighlight %}

--------------------------------------

#### <span id="运行分析">运行分析</span>

从运行的结果可以看出，在使用先序遍历二叉树的时候，二叉树遍历结果和预期一致

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
