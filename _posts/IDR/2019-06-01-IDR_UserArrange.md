---
layout: post
title:  "IDR 部署到应用程序中"
date:   2019-06-01 05:30:30 +0800
categories: [HW]
excerpt: IDR 部署到应用程序中().
tags:
  - RBTREE
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: 用户空间 IDR](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/IDR/Basic)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [IDR 部署方法](#IDR 部署方法)
>
> - [IDR 使用方法](#IDR 使用方法)
>
> - [附录](#附录)

-----------------------------------
<span id="IDR 部署方法"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

## IDR 部署方法

BiscuitOS 开源项目提供了一套用户空间使用的 IDR，开发者只要按照使用步骤就可以
轻松将 IDR 部署到开发者自己的项目中。具体步骤如下：

##### 获取 IDR

开发者首先获得 IDR 的源码文件，可以使用如下命令：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDR/Basic/radix.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDR/Basic/radix.h
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDR/Basic/idr.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDR/Basic/idr.h
{% endhighlight %}

通过上面的命令可以获得 IDR 的源代码，其中 idr.c 文件内包含了 IDR 的核心实现，
idr.h 中包含了调用 IDR 的接口, radix.c 文件包含了 radix-tree 的核心实现，
radix.h 中包含了调用 radix-tree 的接口。

------------------------------

<span id="IDR 使用方法"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

## IDR 使用方法

开发者在获得 IDR 的源码之后，参照如下命令将 IDR 编译到自己的项目中，例如：

{% highlight ruby %}
CC=gcc

CFLAGS = -I./

SRC := radix.c idr.c idr_run.c

all: idr

idr: $(SRC)
	@$(CC) $(SRC) $(CFLAGS) -o $@
{% endhighlight %}

例如在上面的 Makefile 脚本中，需要使用 `-I./` 选项，将头文件搜索路径执行当前目录,
接着将 radix.c, idr.c 一同编译到项目中，这样保证了可以在项目中调用 IDR 的接口。接着
要在自己的源码中调用 IDR 的接口，需要在源文件中引用 `idr.h` 头文件。

#### 实际例子

在下面的源文件中，引用了 IDR 的接口，在程序中构建了 IDR，开发者可以
参照这个文件构建，如下：

{% highlight c %}
/*
 * IDR Manual.
 *
 * (C) 2019.06.01 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/* IDR/IDA */
#include <idr.h>

/* private node */
struct node {
	const char *name;
};

/* Root of IDR */
static DEFINE_IDR(BiscuitOS_idr);

/* node set */
static struct node node0 = { .name = "IDA" };
static struct node node1 = { .name = "IDB" };
static struct node node2 = { .name = "IDC" };

/* ID array */
static int idr_array[8];

int main()
{
	struct node *np;
	int id;

	/* preload for idr_alloc() */
	idr_preload(GFP_KERNEL);

	/* Allocate a id from IDR */
	idr_array[0] = idr_alloc_cyclic(&BiscuitOS_idr, &node0, 1,
							INT_MAX, GFP_ATOMIC);
	idr_array[1] = idr_alloc_cyclic(&BiscuitOS_idr, &node1, 1,
							INT_MAX, GFP_ATOMIC);
	idr_array[2] = idr_alloc_cyclic(&BiscuitOS_idr, &node2, 1,
							INT_MAX, GFP_ATOMIC);

	idr_for_each_entry(&BiscuitOS_idr, np, id)
		printf("%s's ID %d\n", np->name, id);

	/* end preload section started with idr_preload() */
	idr_preload_end();
	return 0;
}
{% endhighlight %}

完整实践例子可以查看下面教程：

> [用户空间 IDR 最小实践](https://biscuitos.github.io/blog/IDR/#IDR%20%E5%9C%A8%E5%BA%94%E7%94%A8%E7%A8%8B%E5%BA%8F%E4%B8%AD%E6%9C%80%E5%B0%8F%E5%AE%9E%E8%B7%B5)

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [IDR](https://biscuitos.github.io/blog/IDR/)
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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
