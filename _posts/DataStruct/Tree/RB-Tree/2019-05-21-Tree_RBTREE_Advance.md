---
layout: post
title:  "红黑树进阶研究"
date:   2019-05-23 05:30:30 +0800
categories: [HW]
excerpt: RB-Tree 红黑树进阶研究().
tags:
  - RBTREE
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [红黑树应用场景](#红黑树应用场景)
>
> - [内核中红黑树应用分析](#内核中红黑树应用分析)
>
> - [附录](#附录)

-----------------------------------
<span id="红黑树应用场景"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

## 红黑树应用场景

红黑树可以运用到以下场景

> 散列表的冲突处理

map 的实现，底层一般会采用红黑树，在节点多的时候效率高。在节点少的时候，可用链表方式。

> 动态插入、删除和查询较多的场景

-----------------------------------
<span id="内核中红黑树应用分析"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

## 内核中红黑树应用分析

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Red Black Tress](https://biscuitos.github.io/blog/Tree_RBTree/)
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
