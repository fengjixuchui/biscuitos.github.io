---
layout: post
title:  "Radix-Tree 部署到应用程序中"
date:   2019-05-23 05:30:30 +0800
categories: [HW]
excerpt: RB-Tree Radix-Tree 部署到应用程序中().
tags:
  - RBTREE
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: 用户空间 Radix-Tree](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/Basic)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [Radix-Tree 部署方法](#Radix-Tree 部署方法)
>
> - [Radix-Tree 使用方法](#Radix-Tree 使用方法)
>
> - [附录](#附录)

-----------------------------------
<span id="Radix-Tree 部署方法"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000P.jpg)

## Radix-Tree 部署方法

BiscuitOS 开源项目提供了一套用户空间使用的 Radix-Tree，开发者只要按照使用步骤就可以
轻松将 Radix-Tree 部署到开发者自己的项目中。具体步骤如下：

##### 获取 Radix-Tree

开发者首先获得 Radix-Tree 的源码文件，可以使用如下命令：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/radix-tree/Basic/radix.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/radix-tree/Basic/radix.h
{% endhighlight %}

通过上面的命令可以获得 Radix-Tree 的源代码，其中 radix.c 文件内包含了 Radix-Tree 的核心实现，
radix.h 中包含了调用 Radix-Tree 的接口。

------------------------------

<span id="Radix-Tree使用方法"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000K.jpg)

## Radix-Tree 使用方法

开发者在获得 Radix-Tree 的源码之后，参照如下命令将 Radix-Tree 编译到自己的项目中，例如：

{% highlight ruby %}
CC=gcc

CFLAGS = -I./

SRC := radix.c radix_run.c

all: radix

radix: $(SRC)
	@$(CC) $(SRC) $(CFLAGS) -o $@
{% endhighlight %}

例如在上面的 Makefile 脚本中，需要使用 `-I./` 选项，将头文件搜索路径执行当前目录,
接着将 radix.c 一同编译到项目中，这样保证了可以在项目中调用 Radix-Tree 的接口。接着
要在自己的源码中调用 Radix-Tree 的接口，需要在源文件中引用 `radix.h` 头文件。

#### 实际例子

在下面的源文件中，引用了 Radix-Tree 的接口，在程序中构建了一棵 Radix-Tree，开发者可以
参照这个文件构建，如下：

{% highlight ruby %}
/*
 * Radix-Tree Manual.
 *
 * (C) 2019.05.27 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/* radix-tree */
#include <radix.h>

/*
 * Radix-tree                                             RADIX_TREE_MAP: 6
 *                                  (root)
 *                                    |
 *                          o---------o---------o
 *                          |                   |
 *                        (0x0)               (0x2)
 *                          |                   |
 *                  o-------o------o            o---------o
 *                  |              |                      |
 *                (0x0)          (0x2)                  (0x2)
 *                  |              |                      |
 *         o--------o------o       |             o--------o--------o
 *         |               |       |             |        |        |
 *       (0x0)           (0x1)   (0x0)         (0x0)    (0x1)    (0x3)
 *         A               B       C             D        E        F
 *
 * A: 0x00000000
 * B: 0x00000001
 * C: 0x00000080
 * D: 0x00080080
 * E: 0x00080081
 * F: 0x00080083
 *
 */

/* node */
struct node {
	char *name;
	unsigned long id;
};

/* Radix-tree root */
static struct radix_tree_root BiscuitOS_root;

/* Range: [0x00000000, 0x00000040] */
static struct node node0 = { .name = "IDA", .id = 0x21 };
/* Range: [0x00000040, 0x00001000] */
static struct node node1 = { .name = "IDB", .id = 0x876 };
/* Range: [0x00001000, 0x00040000] */
static struct node node2 = { .name = "IDC", .id = 0x321FE };
/* Range: [0x00040000, 0x01000000] */
static struct node node3 = { .name = "IDD", .id = 0x987654 };
/* Range: [0x01000000, 0x40000000] */
static struct node node4 = { .name = "IDE", .id = 0x321FEDCA };

int main()
{
	struct node *np;
	struct radix_tree_iter iter;
	void **slot;

	/* Initialize Radix-tree root */
	INIT_RADIX_TREE(&BiscuitOS_root, GFP_ATOMIC);

	/* Insert node into Radix-tree */
	radix_tree_insert(&BiscuitOS_root, node0.id, &node0);
	radix_tree_insert(&BiscuitOS_root, node1.id, &node1);
	radix_tree_insert(&BiscuitOS_root, node2.id, &node2);
	radix_tree_insert(&BiscuitOS_root, node3.id, &node3);
	radix_tree_insert(&BiscuitOS_root, node4.id, &node4);

	/* Iterate over Radix-tree */
	radix_tree_for_each_slot(slot, &BiscuitOS_root, &iter, 0)
		printf("Index: %#lx\n", iter.index);

	/* search struct node by id */
	np = radix_tree_lookup(&BiscuitOS_root, 0x321FE);
	BUG_ON(!np);
	printf("Radix: %s ID: %lx\n", np->name, np->id);

	/* Delete a node from radix-tree */
	radix_tree_delete(&BiscuitOS_root, node0.id);
	radix_tree_delete(&BiscuitOS_root, node1.id);
	radix_tree_delete(&BiscuitOS_root, node2.id);

	return 0;
}
{% endhighlight %}

完整实践例子可以查看下面教程：

> [用户空间 Radix-Tree 最小实践](https://biscuitos.github.io/blog/Tree_RBTree/#%E7%BA%A2%E9%BB%91%E6%A0%91%E5%9C%A8%E5%BA%94%E7%94%A8%E7%A8%8B%E5%BA%8F%E4%B8%AD%E6%9C%80%E5%B0%8F%E5%AE%9E%E8%B7%B5)

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Radix Tree](https://biscuitos.github.io/blog/RADIX-TREE/)
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
