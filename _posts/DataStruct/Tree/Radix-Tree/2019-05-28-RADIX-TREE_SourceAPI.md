---
layout: post
title:  "Radix Tree Source code"
date:   2019-05-30 05:30:30 +0800
categories: [HW]
excerpt: Radix-Tree Radix Tree API().
tags:
  - Radix-Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: Radix Tree](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/API)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [entry_to_node](#entry_to_node)
>
> - [附录](#附录)

-----------------------------------

#### <span id="entry_to_node">entry_to_node</span>

{% highlight bash %}
/*
 * The bottom two bits of the slot determine how the remaining bits in the
 * slot are interpreted:
 *
 * 00 - data pointer
 * 01 - internal entry
 * 10 - exceptional entry
 * 11 - this bit combination is currently unused/reserved
 *
 * The internal entry may be a pointer to the next level in the tree, a
 * sibling entry, or an indicator that the entry in this slot has been moved
 * to another location in the tree and the lookup should be restarted.  While
 * NULL fits the 'data pointer' pattern, it means that there is no entry in
 * the tree for this index (no matter what level of the tree it is found at).
 * This means that you cannot store NULL in the tree as a value for the index.
 */
#define RADIX_TREE_ENTRY_MASK           3UL
#define RADIX_TREE_INTERNAL_NODE        1UL

static inline struct radix_tree_node *entry_to_node(void *ptr)
{
        return (void *)((unsigned long)ptr & ~RADIX_TREE_INTERNAL_NODE);
}
{% endhighlight %}

--------------------------------------------------

#### <span id=""></span>

{% highlight bash %}

{% endhighlight %}


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
