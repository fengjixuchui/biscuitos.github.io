---
layout: post
title:  "二叉树"
date:   2019-05-12 05:30:30 +0800
categories: [HW]
excerpt: TREE 二叉树.
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

> [Github: 二叉树](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/API/BBBXXX)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [二叉树原理](#二叉树原理)
>
> - [二叉树最小实践](#二叉树最小实践)
>
> - [二叉树的实现](#二叉树的实现)
>
>   - [二叉树的先、中、后、层序](#二叉树先中后线序)
>
>   - [二叉树结构定义](#二叉树私有结构定义)
>
>   - [二叉树创建](#创建二叉树)
>
>   - [二叉树遍历](#二叉树遍历)
>
>   - [二叉树删除](#二叉树删除)
>
>   - [二叉树统计叶子数](#二叉树统计叶子数)
>
> - [二叉树的分类](#二叉树的分类)
>
>   - [完美二叉树: Perfect Binary Tree](https://biscuitos.github.io/blog/Tree_binarytree_PerfectBinaryTree/)
>
>   - [完全二叉树: Complete Binary Tree](https://biscuitos.github.io/blog/Tree_binarytree_CompleteBinaryTree/)
>
>   - [完满二叉树: Full Binary Tree](https://biscuitos.github.io/blog/Tree_binarytree_FullBinaryTree/)
>
>   - [二叉查找树: Binary Search Tree](https://biscuitos.github.io/blog/Tree_binarytree_Binary_Search_Tree/)
>
>   - [AVL 平衡二叉查找树](https://biscuitos.github.io/blog/Tree_binarytree_AVLTree/)
>
> - [附录](#附录)

-----------------------------------
# <span id="二叉树原理"></span>

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

# 二叉树原理

> - [二叉树基本原理](#二叉树基本原理)
>
> - [二叉树专业术语](#二叉树专业术语)
>
> - [二叉树的性质](#二叉树的性质)
>
> - [二叉树的分类](#二叉树的分类)

-----------------------------------

### <span id="二叉树基本原理">二叉树基本原理</span>

二叉树 (Binary tree) 是一棵树，每个结点至多拥有两棵子树, 即二叉树中不存在度大于 2 的节点。
二叉树的子树有左右之分，其次序不能任意颠倒。

![DTS](/assets/PDB/BiscuitOS/boot/BOOT000071.png)

###  <span id="二叉树专业术语">二叉树专业术语</span>

> 根 Root

树的顶端节点，成为根，如上图中的 200 就是二叉树的根

> 孩子 Child

当远离根 (Root) 的时候，直接连接到另外一个结点的结点被称之为孩子(Child)。每个二叉树节点
可以包含两个孩子，并且两个孩子有左右之分。例如 143 的左孩子是 754，右孩子是 386;740 的
左孩子是 1，右孩子是 5.

> 双亲 Parent

当远离根 (Root) 的时候，直接连接到为孩子的节点成为孩子的双亲，如上图 143 是 754 和 386
的双亲；486 就是 8 和 2 的双亲；200 是 143 和 876 的双亲。

> 兄弟 Sibling

二叉树中具有同一个双亲 (Parent) 的孩子 (Child) 之间互称为兄弟(Sibling)。例如 754 与
386 拥有共同的双亲 143，那么 754 和 386 就是兄弟；例如 7 和 9 就是兄弟。

> 祖先 Ancestor

结点的祖先 (Ancestor) 是从根（Root）到该结点所经分支 (Branch) 上的所有结点。例如 754，
143，200， 6 都是 6 的祖先，这里 754，143，200 都是 6 的真祖先 (proper ancestor)，
6 只是 6 的祖先。

> 子孙 Descendant

二叉树中以某结点为根的子树中的任一结点都称为该结点的子孙 (Descendant)。例如 143 的子孙有
143, 754, 6, 7, 386, 7, 9，上面的子孙中，除了 143 以外，其余都是 143 的
真子孙 (proper descendant )

> 叶子 Leaf

二叉树中没有孩子的结点 (也就是度为 0 的结点) 称为叶子 (Leaf) 或终端结点。例如 6、3、7、9、
8、2、1、5 节点都是叶子节点。

> 分支 Branch

二叉树中至少有一个孩子的结点称为分支 (Branch) 或非终端结点。例如 754、386、486、740 都是
分支。

> 度 Degree

结点所拥有的子树个数称为结点的度(Degree)。二叉树中节点的度不能大于 2。例如 143 节点的度是 2；
7 节点的度是 0；根节点 200 的度是 2.

> 边 Edge

一个结点和另一个结点之间的连接被称之为边(Edge)。

> 层次 Level

二叉树结点的层次 (Level) 从根 (Root) 开始定义起，根为第0层，根的孩子为第1层。
以此类推，若某结点在第 i 层，那么其子树的根就在第 i+1 层。例如第 0 层只有 root 节点；
第 1 层包含节点 143 和 876；第 2 层包含节点 754、386、486、740；第 3 层包含节点 6、3、
7、9、8、2、1、5; 754 节点位于第 2 层，754 的孩子都在第 3 层。

> 结点的高度

二叉树结点的高度是该结点和某个叶子之间存在的最长路径上的边的个数。例如节点 754 到叶子最大
边数为 1，因此节点 754 的高度为 1；例如根节点 200 到叶子最大的边数为 3，因此根节点的高度
为 3.

> 节点的深度

二叉树结点的深度是从树的根结点到该结点的边的个数。例如 143 节点的深度就是 1；再如 8 节点的
深度就是 3.

-----------------------------------

### <span id="二叉树的性质">二叉树的性质</span>

> 性质1

二叉树第 i 层上的结点数目最多为 2{i-1} (i≥1)

> 性质2

深度为 k 的二叉树至多有 2{k}-1 个结点 (k≥1)

> 性质3

包含 n 个结点的二叉树的高度至少为 log2 (n+1)

> 性质4

在任意一棵二叉树中，若终端结点的个数为 n0，度为 2 的结点数为 n2，则 n0 = n2 + 1

-----------------------------------
# <span id="二叉树最小实践"></span>

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

# 二叉树最小实践

二叉树是度不大于 2 的树，并且二叉树的子树有左右之分。本节用于介绍如何最小实践二叉树。
开发者可以按如下步骤进行实践：

> - [源码获取](#源码获取)
>
> - [源码编译](#源码编译)
>
> - [二叉树运行](#二叉树运行)

### <span id="源码获取">源码获取</span>

二叉树实践代码位于 GitHub 上，开发者可以自行下载：

> [BinaryTree Demo Code on Github](https://github.com/BiscuitOS/HardStack/blob/master/Algorithem/tree/binary-tree/Basic/binary.c)

开发者也可以使用相应的命令下载，如下：

{% highlight ruby %}
mkdir BinaryTree
cd BinaryTree
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/binary-tree/Basic/binary.c
{% endhighlight %}

通过两种方式获得的源码 binary.c：

{% highlight ruby %}
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

/* Perfect Binary Tree
 *                               (200)
 *                                 |
 *                 o---------------+---------------o
 *                 |                               |
 *               (143)                           (876)
 *                 |                               |
 *          o------+-----o                  o------+-----o
 *          |            |                  |            |
 *        (754)        (386)              (486)        (740)
 *          |            |                  |            |
 *      o---+---o    o---+---o          o---+---o    o---+---o
 *      |       |    |       |          |       |    |       |
 *     (6)     (3)  (7)     (9)        (8)     (2)  (1)     (5)
 */
static int Perfect_BinaryTree_data[] = {
                                  200, 143, 754, 6, -1, -1, 3, -1, -1,
                                  386, 7, -1, -1, 9, -1, -1, 876, 486,
                                  8, -1, -1, 2, -1, -1, 740, 1, -1, -1,
                                  5, -1, -1 };

static int counter = 0;
static int *BinaryTree_data = Perfect_BinaryTree_data;

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

### <span id="源码编译">源码编译</span>

在获得源码之后，开发者通过上面的命令进行编译并运行，如下：

{% highlight ruby %}
gcc binary.c -o binary
{% endhighlight %}

### <span id="二叉树运行">二叉树运行</span>

运行情况如下：

{% highlight ruby %}
BinaryTree$ ./binary
Preorder Create BinaryTree
Preorder Traverse Binary-Tree:
200 143 754 6 3 386 7 9 876 486 8 2 740 1 5
Middorder Traverse Binary-Tree:
6 754 3 143 7 386 9 200 8 486 2 876 1 740 5
Postorder Traverse Binary-Tree:
6 3 754 7 9 386 143 8 2 486 1 5 740 876 200
The Binary-Tree Deep: 4
The Binary-Tree leaf: 8
{% endhighlight %}

-----------------------------------
# <span id="二叉树的实现"></span>

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

# 二叉树的实现

二叉树的实现有多种方式，这里介绍使用递归实现二叉树。

> - [二叉树的先、中、后、层序](#二叉树先中后线序)
>
> - [二叉树结构定义](#二叉树私有结构定义)
>
> - [二叉树创建](#创建二叉树)
>
> - [二叉树遍历](#二叉树遍历)
>
> - [二叉树删除](#二叉树删除)
>
> - [二叉树统计叶子数](#二叉树统计叶子数)

### <span id="二叉树先中后线序">二二叉树的先、中、后、层序</span>

![DTS](/assets/PDB/BiscuitOS/boot/BOOT000071.png)

上述的实践代码已经运行了最小的二叉树，本节用于讲解上述源码的实现原理。首先需要了解
一下树的先序遍历，中序遍历，后序遍历, 以及线序遍历。

> 二叉树的先序遍历

先序又称为先根，每次遍历到一个节点时的顺序是： 根，左孩子，右孩子。如上面的实践结果
可以知道，先序的结果： 200 143 754 6 3 386 7 9 876 486 8 2 740 1 5

> 二叉树的中序遍历

中序又称为中根，又叫对称序，顺序为： 左孩子，根，右孩子。如上面的实践结果，可以知道
中序的结果： 6 754 3 143 7 386 9 200 8 486 2 876 1 740 5

> 二叉树的后序遍历

后续，即后根，顺序为：左孩子，右孩子，根。如上面的实践结果，可以知道后序的结果：
6 3 754 7 9 386 143 8 2 486 1 5 740 876 200

> 二叉树的线序遍历

线序，即层次，按层次遍历，先遍历第一层，从左到右，再第二层，继续遍历下去。

-----------------------------------

### <span id="二叉树私有结构定义">二叉树结构定义</span>

在实践代码中，定义了一个二叉树节点如下：

{% highlight c %}
/* binary-tree node */
struct binary_node {
	int idx;
	struct binary_node *left;
	struct binary_node *right;
};
{% endhighlight %}

一个典型的二叉树节点必须包含两个指针，两个指针有左右之分，并且指向节点的左右孩子。
节点 binary_node 典型的包含了两个指针，并且这两个指针有左右之分。节点还定义了一个
私有数据，因此，开发者也可以在 binary_node 节点中嵌套私有数据。

-----------------------------------

### <span id="创建二叉树">二叉树创建</span>

二叉树的创建有多种方式，就本实践而言，实践中使用了递归的方式进行二叉树创建。递归
的基本原理就是函数调用函数本身，由于递归存在这个特性，因此在创建二叉树的时候，开发者
可以使用递归进行创建。但递归需要按照一定的规则去递归，不然会形成死循环，接着之前
先序，层序，后序的介绍，可以使用其中一种规则去创建二叉树。(注意！中序是不能创建一棵
唯一的二叉树)。

> - [先序创建二叉树](https://biscuitos.github.io/blog/Tree_binarytree_PreCreate/#%E5%AE%9E%E8%B7%B5%E6%BA%90%E7%A0%81)
>
> - [后序创建二叉树](https://biscuitos.github.io/blog/Tree_binarytree_PostCreate/)
>
> - [层序创建二叉树](#层序创建二叉树)

-----------------------------------

### <span id="二叉树遍历">二叉树遍历</span>

二叉树的遍历方式有很多种，包括先序遍历二叉树，中序遍历二叉树，后序遍历二叉树，以及
层序遍历二叉树。

> - [先序遍历二叉树](https://biscuitos.github.io/blog/Tree_binarytree_PreIterateOver/)
>
> - [中序遍历二叉树](https://biscuitos.github.io/blog/Tree_binarytree_MiddIterateOver/)
>
> - [后序遍历二叉树](https://biscuitos.github.io/blog/Tree_binarytree_PostIterateOver/)
>
> - [层序遍历二叉树](#二叉树遍历)

-----------------------------------

### <span id="二叉树删除">二叉树删除</span>

二叉树的删除应该先删除节点对应的孩子，然后才能删除当前节点，因此开发者需要找到一种
遍历二叉树的方法，该方法先遍历孩子，然后才遍历当前节点。后序遍历满足这个要求，因此
这里使用后序遍历的逻辑去删除所有节点。

> - [后序删除二叉树](https://biscuitos.github.io/blog/Tree_binarytree_PostDelete/)

-----------------------------------

### <span id="二叉树统计叶子数">二叉树统计叶子数</span>

二叉树中没有孩子的结点 (也就是度为 0 的结点) 称为叶子 (Leaf) 或终端结点。因此只要遍历
二叉树的所有的节点，只要节点的度为 0，那么这个节点就是叶子。

> - [统计二叉树的叶子数](https://biscuitos.github.io/blog/Tree_binarytree_Leafcount/)

-----------------------------------
# <span id="二叉树的分类"></span>

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000E.jpg)

# 二叉树的分类

二叉树是节点度不大于 2 的树，根据不同的属性，将二叉树分作不同的类别

> - [完美二叉树: Perfect Binary Tree](https://biscuitos.github.io/blog/Tree_binarytree_PerfectBinaryTree/)
>
> - [完全二叉树: Complete Binary Tree](https://biscuitos.github.io/blog/Tree_binarytree_CompleteBinaryTree/)
>
> - [完满二叉树: Full Binary Tree](https://biscuitos.github.io/blog/Tree_binarytree_FullBinaryTree/)
>
> - [二叉查找树: Binary Search Tree](https://biscuitos.github.io/blog/Tree_binarytree_Binary_Search_Tree/)
>
> - [AVL 平衡二叉查找树](https://biscuitos.github.io/blog/Tree_binarytree_AVLTree/)

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
