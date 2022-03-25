---
layout: post
title:  "二叉查找树"
date:   2019-05-12 05:30:30 +0800
categories: [HW]
excerpt: TREE 二叉查找树().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

> [Github: 二叉查找树](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/binary-tree/Class/Binary_Search_Tree)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [二叉查找树原理](#原理)
>
> - [二叉查找树实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="原理">二叉查找树原理</span>

![DTS](/assets/PDB/BiscuitOS/boot/BOOT000073.png)

对于二叉树中任意一个元素, 若其左子树中所有元素的值都小于该元素的值, 并且其右子树中
所有元素的值都大于该元素的值, 这个二叉树就叫做搜索二叉树。二叉搜索树中序遍历后的结
果是从小到大依次排列的, 这也是判断一个二叉树是不是二叉搜索树的依据。二叉查找树具有以下性质

{% highlight ruby %}
1) 若任意节点的左子树不空，则左子树上所有结点的值均小于它的根结点的值；

2) 若任意节点的右子树不空，则右子树上所有结点的值均大于它的根结点的值；

3) 任意节点的左、右子树也分别为二叉查找树；

4) 没有键值相等的节点。
{% endhighlight %}

接下来通过一个
实践进一步认识二叉查找树

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

> [实践源码 binary.c on GitHub](https://github.com/BiscuitOS/HardStack/blob/master/Algorithem/tree/binary-tree/Class/Binary_Search_Tree/binary.c)

开发者也可以使用如下命令获得：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/binary-tree/Class/Binary_Search_Tree/binary.c
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

/* Binary Search Tree
 *                               (200)
 *                                 |
 *                 o---------------+---------------o
 *                 |                               |
 *                (7)                            (289)
 *                 |                               |
 *          o------+-----o                  o------+-----o
 *          |            |                  |            |
 *         (3)          (9)               (220)        (740)
 *          |            |                  |            |
 *      o---+---o    o---+---o          o---+---o    o---+---o
 *      |       |    |       |          |       |    |       |
 *     (1)     (5)  (8)    (12)       (202)   (240)(300)   (791)
 */
static int Binary_Search_Tree_data[] = {
                                   200, 7, 3, 1, -1, -1, 5, -1, -1,
                                   9, 8, -1, -1, 12, -1, -1, 289,
                                   220, 202, -1, -1, 240, -1, -1,
                                   740, 300, -1, -1, 791, -1, -1 };

static int counter = 0;
static int *BinaryTree_data = Binary_Search_Tree_data;

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
binary-tree/Class/Binary_Search_Tree$ ./binary
Preorder Create BinaryTree
Middorder Traverse Binary-Tree:
1 3 5 7 8 9 12 200 202 220 240 289 300 740 791
{% endhighlight %}

--------------------------------------

#### <span id="运行分析">运行分析</span>

从运行的结果可以看出，在使用中序遍历二叉树的时候，所以的数据都是从小到大排序的，因此
这棵树是一棵二叉查找树。

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
