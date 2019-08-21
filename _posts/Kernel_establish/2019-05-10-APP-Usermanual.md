---
layout: post
title:  "BiscuitOS User Interface"
date:   2019-05-10 14:55:30 +0800
categories: [HW]
excerpt: BiscuitOS Application().
tags:
  - Userland
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Y.jpg)

> [Github: BiscuitOS HardStack](https://github.com/BiscuitOS/HardStack)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录


> - [应用程序移植](#A00)
>
>   - [应用程序移植最小实践](https://biscuitos.github.io/blog/USER_APP_mini/)
>
>   - [快速移植开发者的应用程序](#A02)
>
>   - [完整移植开发者的应用程序](#A03)
>
> - [GNU 项目移植](#B00)
>
>   - [autoconf](https://biscuitos.github.io/blog/USER_GNU_autoconf/)
>
>   - [automake](https://biscuitos.github.io/blog/USER_GNU_automake/)
>
>   - [binutils](https://biscuitos.github.io/blog/USER_GNU_binutils/)
>
>   - [hello](https://biscuitos.github.io/blog/USER_GNU_hello/)
>
> - [动态库移植](#C00)
>
>   - [libtool](https://biscuitos.github.io/blog/USER_DLIB_libtool/)
>
>   - [libuuid](https://biscuitos.github.io/blog/USER_DLIB_libuuid/)
>
>   - [ncurses](https://biscuitos.github.io/blog/USER_DLIB_ncurses/)
>
> - [静态块移植](#D00)
>
> - [BiscuitOS 项目移植](#E00)
>
>   - [binary-tree 二叉树](https://biscuitos.github.io/blog/USER_Algo_binarytree/)
>
>   - [IDA](https://biscuitos.github.io/blog/USER_Algo_IDA/)
>
>   - [IDR](https://biscuitos.github.io/blog/USER_Algo_IDR/)
>
>   - [bind-list 双链表](https://biscuitos.github.io/blog/USER_Algo_bindlist/)
>
>   - [bitmap](https://biscuitos.github.io/blog/USER_Algo_bitmap/)
>
>   - [2-3 tree](https://biscuitos.github.io/blog/USER_Algo_tree23/)
>
>   - [radix tree](https://biscuitos.github.io/blog/USER_Algo_radixtree/)
>
>   - [rbtree](https://biscuitos.github.io/blog/USER_Algo_rbtree/)
>
> - [游戏移植](#F00)
>
>   - [Snake](https://biscuitos.github.io/blog/USER_GAME_snake/)
>
> - [附录](#附录)

-----------------------------------
<span id="A00"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

## 应用程序移植

开发者有时需要 BiscuitOS 上运行 C/C++ 应用程序，因此需要将应用程序通过交叉
编译之后移植到 BiscuitOS 上运行，目前 BiscuitOS 已经完整支持应用程序的移植
与实践，开发者可以通过参考下面的文章了解更多的详情。

> - [应用程序移植最小实践](https://biscuitos.github.io/blog/USER_APP_mini/)
>
> - [快速移植开发者的应用程序](#A02)
>
> - [完整移植开发者的应用程序](#A03)


-----------------------------------
<span id="B00"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000A.jpg)

## GNU 项目移植

目前 BiscuitOS 已经支持多种 GNU 组织的多个开源项目，开发者可以通过下面的
文档进行移植使用。

> - [autoconf](https://biscuitos.github.io/blog/USER_GNU_autoconf/)
>
> - [automake](https://biscuitos.github.io/blog/USER_GNU_automake/)
>
> - [binutils](https://biscuitos.github.io/blog/USER_GNU_binutils/)
>
> - [hello](https://biscuitos.github.io/blog/USER_GNU_hello/)

-----------------------------------
<span id="C00"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000T.jpg)

## 动态库移植

目前 BiscuitOS 已经支持多种动态库，开发者可以通过下面的
文档进行移植使用。

> - [libtool](https://biscuitos.github.io/blog/USER_DLIB_libtool/)
>
> - [libuuid](https://biscuitos.github.io/blog/USER_DLIB_libuuid/)
>
> - [ncurses](https://biscuitos.github.io/blog/USER_DLIB_ncurses/)

-----------------------------------
<span id="E00"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000P.jpg)

## BiscuitOS 项目移植

目前 BiscuitOS 已经支持多种 BiscuitOS 的开源项目，开发者可以通过下面的
文档进行移植使用。

> - [binary-tree 二叉树](https://biscuitos.github.io/blog/USER_Algo_binarytree/)
>
> - [IDA](https://biscuitos.github.io/blog/USER_Algo_IDA/)
>
> - [IDR](https://biscuitos.github.io/blog/USER_Algo_IDR/)
>
> - [bind-list 双链表](https://biscuitos.github.io/blog/USER_Algo_bindlist/)
>
> - [bitmap](https://biscuitos.github.io/blog/USER_Algo_bitmap/)
>
> - [2-3 tree](https://biscuitos.github.io/blog/USER_Algo_tree23/)
>
> - [radix tree](https://biscuitos.github.io/blog/USER_Algo_radixtree/)
>
> - [rbtree](https://biscuitos.github.io/blog/USER_Algo_rbtree/)

-----------------------------------
<span id="F00"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000B.jpg)

## 游戏移植

目前 BiscuitOS 已经支持多种游戏开源项目，开发者可以通过下面的
文档进行移植使用。

> - [Snake](https://biscuitos.github.io/blog/USER_GAME_snake/)

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
