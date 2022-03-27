---
layout: post
title:  "将内核双链表部署到应用程序中"
date:   2019-05-27 05:30:30 +0800
categories: [HW]
excerpt: RB-Tree 将内核双链表部署到应用程序中().
tags:
  - RBTREE
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

> [Github: 用户空间双链表](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/list/bindirect-list/Basic)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [双链表部署方法](#双链表部署方法)
>
> - [双链表使用方法](#双链表使用方法)
>
> - [附录](#附录)

-----------------------------------
<span id="双链表部署方法"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

## 双链表部署方法

BiscuitOS 开源项目提供了一套用户空间使用的双链表，开发者只要按照使用步骤就可以
轻松将双链表部署到开发者自己的项目中。具体步骤如下：

##### 获取双链表

开发者首先获得双链表的源码文件，可以使用如下命令：

{% highlight ruby %}
wget https://github.com/BiscuitOS/HardStack/blob/master/Algorithem/list/bindirect-list/Basic/list.h
{% endhighlight %}

通过上面的命令可以获得双链表的源代码，其中 list.h 文件内包含了双链表的核心实现。

------------------------------
<span id="双链表使用方法"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

## 双链表使用方法

开发者在获得双链表的源码之后，参照如下命令将双链表编译到自己的项目中，例如：

{% highlight ruby %}
CC=gcc

CFLAGS = -I./

SRC := list_run.c

all: list

list: $(SRC)
	@$(CC) $(SRC) $(CFLAGS) -o $@
{% endhighlight %}

例如在上面的 Makefile 脚本中，需要使用 `-I./` 选项，将头文件搜索路径执行当前目录,
接着引用 list.h 并一同编译到项目中，这样保证了可以在项目中调用双链表的接口。接着
要在自己的源码中调用双链表的接。

#### 实际例子

在下面的源文件中，引用了双链表的接口，开发者可以参照这个文件构建，如下：

{% highlight ruby %}
/*
 * list Manual.
 *
 * (C) 2019.05.27 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/*
 * bidirect-list
*
 * +-----------+<--o    +-----------+<--o    +-----------+<--o    +-----------+
 * |           |   |    |           |   |    |           |   |    |           |
 * |      prev |   o----| prev      |   o----| prev      |   o----| prev      |
 * | list_head |        | list_head |        | list_head |        | list_head |
 * |      next |---o    |      next |---o    |      next |---o    |      next |
 * |           |   |    |           |   |    |           |   |    |           |
 * +-----------+   o--->+-----------+   o--->+-----------+   o--->+-----------+
 *
 */
/* list */
#include <list.h>

/* private structure */
struct node {
	const char *name;
	struct list_head list;
};

/* Initialize a group node structure */
static struct node node0 = { .name = "BiscuitOS_node0", };
static struct node node1 = { .name = "BiscuitOS_node1", };
static struct node node2 = { .name = "BiscuitOS_node2", };
static struct node node3 = { .name = "BiscuitOS_node3", };
static struct node node4 = { .name = "BiscuitOS_node4", };
static struct node node5 = { .name = "BiscuitOS_node5", };
static struct node node6 = { .name = "BiscuitOS_node6", };

/* Declaration and implement a bindirect-list */
static LIST_HEAD(BiscuitOS_list);

int main()
{
	struct node *np;

	/* add a new entry on back */
	list_add_tail(&node0.list, &BiscuitOS_list);
	list_add_tail(&node1.list, &BiscuitOS_list);
	list_add_tail(&node2.list, &BiscuitOS_list);
	list_add_tail(&node3.list, &BiscuitOS_list);
	list_add_tail(&node4.list, &BiscuitOS_list);
	list_add_tail(&node5.list, &BiscuitOS_list);
	list_add_tail(&node6.list, &BiscuitOS_list);

	/* Traverser all node on bindirect-list */
	list_for_each_entry(np, &BiscuitOS_list, list)
		printf("%s\n", np->name);

	return 0;
}
{% endhighlight %}

完整实践例子可以查看下面教程：

> [用户空间双链表最小实践](/blog/LIST/#%E5%86%85%E6%A0%B8%E5%8F%8C%E9%93%BE%E8%A1%A8%E6%9C%80%E5%B0%8F%E5%AE%9E%E8%B7%B5)

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Bidirect list](/blog/LIST/)
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
