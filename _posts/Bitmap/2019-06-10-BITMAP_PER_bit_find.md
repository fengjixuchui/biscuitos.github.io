---
layout: post
title:  "find bit 内核性能测试"
date:   2019-06-10 05:30:30 +0800
categories: [HW]
excerpt: find bit 内核性能测试.
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [测试说明](#SC00)
>
> - [测试方法](#SC01)
>
> - [测试分析](#SC02)
>
> - [附录](#附录)

-----------------------------------
<span id="SC00"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

## 测试说明

Linux 提供了用于测试 find bit 性能的接口模块，该模块用于测试 find bit
提供的接口运行时性能。内核以模块的形式给出，源码位于 lib/find_bit_benchmark.c
文件中。内核使用 DECLARE_BITMAP() 定义了两个长度为 320K 的 bitmap，然后
调用如下接口，测试各个接口耗时

{% highlight bash %}
test_find_first_bit() 用于测试 find_first_bit() 函数找到第一个置位位置
                      所花费的时间。

test_find_next_bit() 用于测试 find_next_bit() 函数用于找到下一个置位位置
                      所花费的时间。

test_find_next_zero_bit() 用于测试 find_next_zero_bit() 函数找到下一个
                      清零位置的时间。

test_find_last_bit() 用于测试 find_last_bit() 函数找到最后一个置位位置的
                      时间。

test_find_next_and_bit() 用于测试 find_next_and_bit() 函数找到下一个置位
                      的位置，并相与特定值之后所花费的时间。
{% endhighlight %}

模块的测试逻辑在 find_bit_test() 中实现，其实现逻辑：首先调用
get_random_bytes() 函数给 DECLARE_BITMAP() 定义的两个 320k bitmap 和
bitmap2 赋上随机值。

##### 测试 1

调用 test_find_next_bit()、test_find_next_zero_bit()、
test_find_last_bit() 找到符合要求的 bit 所花费的时间。此时随机
生成的数据比较密集。

##### 测试 2

将 bitmap 和 bitmap2 清零之后，再次赋予随机值之后执行
test_find_next_bit()、test_find_next_zero()、test_find_last_bit()、
test_find_first_bit()、test_find_next_and_bit() 所花费的时间。测试
随机生成的数据比较稀疏。

-----------------------------------
<span id="SC01"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000K.jpg)

## 测试方法

内核将 bit find 测试程序与独立模块的形式给出，开发者可以通过打开相关的宏，
进行 bit find 的测试。开发者按如下步骤：

#### <span id="驱动配置">内核配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
make menuconfig ARCH=arm
{% endhighlight %}

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000136.png)

选择 **Kernel hacking --->**

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000137.png)

选择 **Runtime Testing --->**

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000138.png)

选择 **Test find_bit functions --->**, 选中后保存退出，并重新编译内核。具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">内核编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

-----------------------------------
<span id="SC02"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000L.jpg)

## 测试分析

见 find_bit 模块编译进内核之后，重新编译内核之后，系统启动时候的信息如下：

{% highlight bash %}
squashfs: version 4.0 (2009/01/31) Phillip Lougher
jffs2: version 2.2. (NAND) © 2001-2006 Red Hat, Inc.
9p: Installing v9fs 9p2000 file system support
io scheduler mq-deadline registered
io scheduler kyber registered

Start testing find_bit() with random-filled bitmap
find_next_bit:                 9390000 ns, 163934 iterations
find_next_zero_bit:            8418000 ns, 163747 iterations
find_last_bit:                 8220000 ns, 163933 iterations
find_first_bit:              108544000 ns,  16351 iterations
find_next_and_bit:             6069000 ns,  73764 iterations

Start testing find_bit() with sparse bitmap
find_next_bit:                  180000 ns,    656 iterations
find_next_zero_bit:           16999000 ns, 327025 iterations
find_last_bit:                  107000 ns,    656 iterations
find_first_bit:               44386000 ns,    656 iterations
find_next_and_bit:               96000 ns,      1 iterations
i2c i2c-0: Added multiplexed i2c bus 2
{% endhighlight %}

测试分作两种，两种 bitmap 不同点就是第一种 bitmap 的数据比较集中密集的，
而第二种测试则是数据比较稀疏。通过两种测试产生的数据，可以知道:

##### find_next_bit

数据密集的时候，调用 find_next_bit() 函数总共执行了 163934 次，总共花费
9390000 ns, 平均每次花费 57 ns；数据稀疏的时候，调用 find_next_bit() 函
数总共执行了 656 次，总共花费 180000 ns, 平均每次花费 274 ns。因此
数据越密集，find_next_bit() 函数越容易找到置位的 bit。

##### find_next_zero_bit

数据密集的时候，调用 find_next_zero_bit() 函数总共执行了 163747 次，总共花费
8418000 ns, 平均每次花费 51 ns；数据稀疏的时候，调用 find_next_bit() 函
数总共执行了 327025 次，总共花费 16999000 ns, 平均每次花费 51 ns。因此
数据越密集或稀疏，find_next_zero_bit() 函数找到清零的位耗时一样。

##### find_last_bit

数据密集的时候，调用 find_last_bit() 函数总共执行了 163933 次，总共花费
8220000 ns, 平均每次花费 50 ns；数据稀疏的时候，调用 find_last_bit() 函
数总共执行了 656 次，总共花费 107000 ns, 平均每次花费 163 ns。因此数据越密集，
find_last_bit() 函数越容易找到置位的 bit。

##### find_first_bit

数据密集的时候，调用 find_first_bit() 函数总共执行了 16351 次，总共花费
108544000 ns, 平均每次花费 6638 ns；数据稀疏的时候，调用 find_first_bit() 函
数总共执行了 656 次，总共花费 44386000 ns, 平均每次花费 67661 ns。因此
数据越密集，find_first_bit() 函数越容易找到第一次置位的 bit。

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
