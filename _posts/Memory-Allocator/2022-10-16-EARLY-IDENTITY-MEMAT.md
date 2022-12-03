---
layout: post
title:  "Early Identity-Mapping Memory Allocator"
date:   2022-10-16 12:02:00 +0800
categories: [MMU]
excerpt: Early IO/RESV-MEM Remap.
tags:
  - Boot-Time Allocator
  - Linux 6.0+
  - BiscuitOS DOC 3.0+
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [Early Identity-Mapping 内存分配器原理](#A)
>
> - [Early Identity-Mapping 内存分配器实践](#B)
>
> - [Early Identity-Mapping 内存分配器使用](#C)
>
> - [Early Identity-Mapping API 合集](#D)
>
> - [Early Identity-Mapping 内存世界地图](#F)
>
> - Early Identity-Mapping 应用场景
>
> - Early Identity-Mapping 分配器 BUG 合集
>
> - Early Identity-Mapping 分配器进阶研究

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### Early Identity-Mapping 内存分配器原理

![](/assets/PDB/HK/TH001990.png)
