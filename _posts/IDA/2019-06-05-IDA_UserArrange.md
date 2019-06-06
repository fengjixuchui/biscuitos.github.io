---
layout: post
title:  "IDA 部署到应用程序中"
date:   2019-06-01 05:30:30 +0800
categories: [HW]
excerpt: IDA 部署到应用程序中().
tags:
  - RBTREE
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: 用户空间 IDA](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/IDA/Basic)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [IDA 部署方法](#IDA 部署方法)
>
> - [IDA 使用方法](#IDA 使用方法)
>
> - [附录](#附录)

-----------------------------------
<span id="IDA 部署方法"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000P.jpg)

## IDA 部署方法

BiscuitOS 开源项目提供了一套用户空间使用的 IDA，开发者只要按照使用步骤就可以
轻松将 IDA 部署到开发者自己的项目中。具体步骤如下：

##### 获取 IDA

开发者首先获得 IDA 的源码文件，可以使用如下命令：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDA/Basic/radix.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDA/Basic/radix.h
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDA/Basic/ida.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/IDA/Basic/ida.h
{% endhighlight %}

通过上面的命令可以获得 IDA 的源代码，其中 ida.c 文件内包含了 IDA 的核心实现，
ida.h 中包含了调用 IDA 的接口, radix.c 文件包含了 radix-tree 的核心实现，
radix.h 中包含了调用 radix-tree 的接口。

------------------------------

<span id="IDA 使用方法"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000K.jpg)

## IDA 使用方法

开发者在获得 IDA 的源码之后，参照如下命令将 IDA 编译到自己的项目中，例如：

{% highlight ruby %}
CC=gcc

CFLAGS = -I./

SRC := radix.c ida.c ida_run.c

all: ida

ida: $(SRC)
	@$(CC) $(SRC) $(CFLAGS) -o $@
{% endhighlight %}

例如在上面的 Makefile 脚本中，需要使用 `-I./` 选项，将头文件搜索路径执行当前目录,
接着将 radix.c, ida.c 一同编译到项目中，这样保证了可以在项目中调用 IDA 的接口。接着
要在自己的源码中调用 IDA 的接口，需要在源文件中引用 `ida.h` 头文件。

#### 实际例子

在下面的源文件中，引用了 IDA 的接口，在程序中构建了 IDA，开发者可以
参照这个文件构建，如下：

{% highlight c %}
/*
 * IDA Manual.
 *
 * (C) 2019.06.03 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/* IDR/IDA */
#include <ida.h>

/* Root of IDA */
static DEFINE_IDA(BiscuitOS_ida);

int main()
{
	unsigned long i;
	int id;

	for (i = 0; i < 1000; i++) {
		/* Allocate an unused ID */
		id = ida_alloc(&BiscuitOS_ida, GFP_KERNEL);
		printf("IDA-ID: %d\n", id);
	}


	/* Release an allocated ID */
	ida_free(&BiscuitOS_ida, id);

	return 0;
}
{% endhighlight %}

完整实践例子可以查看下面教程：

> [用户空间 IDA 最小实践](https://biscuitos.github.io/blog/IDA/#IDA%20%E5%9C%A8%E5%BA%94%E7%94%A8%E7%A8%8B%E5%BA%8F%E4%B8%AD%E6%9C%80%E5%B0%8F%E5%AE%9E%E8%B7%B5)

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [IDA](https://biscuitos.github.io/blog/IDA/)
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
