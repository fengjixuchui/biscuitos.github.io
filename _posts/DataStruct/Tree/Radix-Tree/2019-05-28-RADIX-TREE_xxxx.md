---
layout: post
title:  "xxxxxxx"
date:   2019-05-29 05:30:30 +0800
categories: [HW]
excerpt: Radix-Tree BBBXXX().
tags:
  - Radix-Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: BBBXXX](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/API/BBBXXX)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="源码分析">源码分析</span>

{% highlight bash %}

{% endhighlight %}

--------------------------------------------------

> - [all_tag_set](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#all_tag_set)
>
> - [entry_to_node](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#entry_to_node)
>
> - [is_idr](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#is_idr)
>
> - [node_maxindex](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#node_maxindex)
>
> - [radix_tree_exceptional_entry](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_exceptional_entry/)
>
> - [radix_tree_is_internal_node](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_is_internal_node/)
>
> - [radix_tree_load_root](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_load_root)
>
> - [radix_tree_node_alloc](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_node_alloc)
>
> - [root_tag_clear](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#root_tag_clear)
>
> - [root_tag_clear_all](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#root_tag_clear_all)
>
> - [root_tag_get](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#root_tag_get)
>
> - [root_tag_set](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#root_tag_set)
>
> - [shift_maxindex](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#shift_maxindex)
>
> - [tag_clear](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#tag_clear)
>
> - [tag_get](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#tag_get)
>
> - [tag_set](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#tag_set)
>
> - [附录](#附录)

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Radix Tress](https://biscuitos.github.io/blog/Tree_RADIX_TREE/)
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
