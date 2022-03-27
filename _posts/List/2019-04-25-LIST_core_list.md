---
layout: post
title:  "Linux 内核核心双链表"
date:   2019-04-26 07:38:30 +0800
categories: [HW]
excerpt: Linux 内核核心双链表.
tags:
  - Bidirect-list
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

> [Github: Bidirect-list core list](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/list/bindirect-list/core)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [简介](#LIST)
>
> - [附录](#附录)

-----------------------------------

# <span id="LIST">简介</span>

Linux 内核提供了双链表运用在各种数据结构中，为数据结构之间构建更紧密的联系。本文
主要介绍内核中主要的双链表。如下表：

> - [alias_prop: 维护 DTS 所有别名](/blog/LIST_alias_prop/)
>
> - early_platform_driver_list: 早期 platform driver 链表
>
> - early_platform_device_list: 早期 platform device 链表

-----------------------------------------------

# <span id="附录">附录</span>

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
