---
layout: post
title:  "Bit 部署到应用程序中"
date:   2019-06-01 05:30:30 +0800
categories: [HW]
excerpt: Bit 部署到应用程序中().
tags:
  - Bit
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: 用户空间 Bit](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/bitmap/Basic)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [Bit 部署方法](#Bit 部署方法)
>
> - [Bit 使用方法](#Bit 使用方法)
>
> - [附录](#附录)

-----------------------------------

<span id="Bit 部署方法"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

## Bit 部署方法

BiscuitOS 开源项目提供了一套用户空间使用的 Bit，开发者只要按照使用步骤就可以
轻松将 Bit 部署到开发者自己的项目中。具体步骤如下：

##### 获取 Bit

开发者首先获得 Bit 的源码文件，可以使用如下命令：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/bitmap/Basic/bitmap.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/bitmap/Basic/bitmap.h
{% endhighlight %}

通过上面的命令可以获得 Bit 的源代码，其中 bitmap.c 文件内包含了 Bitmap、bitops、
bitmask、bit-find 的核心实现，bitmap.h 中包含了调用 Bit 的接口以及相关的宏。

------------------------------

<span id="Bit 使用方法"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

## Bit 使用方法

开发者在获得 Bit 的源码之后，参照如下命令将 Bit 编译到自己的项目中，例如：

{% highlight ruby %}
# SPDX-License-Identifier: GPL-2.0
CC=gcc

# Architecture
ARCH := $(shell getconf LONG_BIT)

CFLAGS = -I./

SRC := bitmap.c bitmap_run.c

# Config

ifeq ($(ARCH),64)
CFLAGS += -DCONFIG_64BIT
endif

all: bitmap

bitmap: $(SRC)
	@$(CC) $(SRC) $(CFLAGS) -o $@

clean:
	@rm -rf *.o bitmap > /dev/null
{% endhighlight %}

例如在上面的 Makefile 脚本中，需要使用 `-I./` 选项，将头文件搜索路径执行当前目录,
接着将 bitmap.c 一同编译到项目中，这样保证了可以在项目中调用 Bit 的接口。接着
要在自己的源码中调用 Bit 的接口，需要在源文件中引用 `bitmap.h` 头文件。

#### 实际例子

在下面的源文件中，引用了 Bit 的接口，在程序中构建了 Bit，开发者可以
参照这个文件构建，如下：

{% highlight c %}
/*
 * Bitmap Manual.
 *
 * (C) 2019.06.10 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/* bitmap header */
#include <bitmap.h>

int main()
{
	unsigned long bitmap[2] = {0};
	u64 map = 0x123456789abcdef;

	/* Cover u64 to bitmap */
	bitmap_from_u64(bitmap, map);
	printf("%#llx cover to [0]%#lx [1]%#lx\n", map, bitmap[0], bitmap[1]);

	return 0;
}
{% endhighlight %}

完整实践例子可以查看下面教程：

> [用户空间 Bit 最小实践](https://biscuitos.github.io/blog/BITMAP/#B1)

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Bit](https://biscuitos.github.io/blog/BITMAP/)
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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
