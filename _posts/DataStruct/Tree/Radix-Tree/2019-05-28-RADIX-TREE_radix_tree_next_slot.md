---
layout: post
title:  "radix_tree_next_slot"
date:   2019-05-29 05:30:30 +0800
categories: [HW]
excerpt: Radix-Tree radix_tree_next_slot().
tags:
  - Radix-Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: radix_tree_next_slot](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/API/radix_tree_next_slot)
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
/**
 * radix_tree_next_slot - find next slot in chunk
 *
 * @slot:       pointer to current slot
 * @iter:       pointer to interator state
 * @flags:      RADIX_TREE_ITER_*, should be constant
 * Returns:     pointer to next slot, or NULL if there no more left
 *
 * This function updates @iter->index in the case of a successful lookup.
 * For tagged lookup it also eats @iter->tags.
 *
 * There are several cases where 'slot' can be passed in as NULL to this
 * function.  These cases result from the use of radix_tree_iter_resume() or
 * radix_tree_iter_retry().  In these cases we don't end up dereferencing
 * 'slot' because either:
 * a) we are doing tagged iteration and iter->tags has been set to 0, or
 * b) we are doing non-tagged iteration, and iter->index and iter->next_index
 *    have been set up so that radix_tree_chunk_size() returns 1 or 0.
 */
static __always_inline void __rcu **radix_tree_next_slot(void __rcu **slot,
                                struct radix_tree_iter *iter, unsigned flags)
{% endhighlight %}

radix_tree_next_slot() 函数用于在遍历 radix-tree 过程中，获得下一个可用的 slot。
由于函数比较长，分段进行讲解。参数 slot 指向上一层 slot 指针；参数 iter 指向上一个
slot 的信息；flags 指向节点的标志。

{% highlight bash %}
        if (flags & RADIX_TREE_ITER_TAGGED) {
                iter->tags >>= 1;
                if (unlikely(!iter->tags))
                        return NULL;
                if (likely(iter->tags & 1ul)) {
                        iter->index = __radix_tree_iter_add(iter, 1);
                        slot++;
                        goto found;
                }
                if (!(flags & RADIX_TREE_ITER_CONTIG)) {
                        unsigned offset = __ffs(iter->tags);

                        iter->tags >>= offset++;
                        iter->index = __radix_tree_iter_add(iter, offset);
                        slot += offset;
                        goto found;
                }
        }
{% endhighlight %}

如果节点的 flags 是否是 RADIX_TREE_ITER_TAGGED，如果是，则暂时不分析在纯粹的
radix tree 中。

{% highlight bash %}
        } else {
                long count = radix_tree_chunk_size(iter);

                while (--count > 0) {
                        slot++;
                        iter->index = __radix_tree_iter_add(iter, 1);

                        if (likely(*slot))
                                goto found;
                        if (flags & RADIX_TREE_ITER_CONTIG) {
                                /* forbid switching to the next chunk */
                                iter->next_index = 0;
                                break;
                        }
                }
        }
        return NULL;

 found:
        if (unlikely(radix_tree_is_internal_node(rcu_dereference_raw(*slot))))
                return __radix_tree_next_slot(slot, iter, flags);
        return slot;
{% endhighlight %}

如果节点的标志不是 RADIX_TREE_ITER_TAGGED，那么节点就是纯粹的 radix-tree 节点。
函数调用 radix_tree_chunk_size 获得 chunk 的大小，然后调用 while 循环查找
上一个可用节点之后，在本 chunk 剩下的节点中是否还包含可用的叶子节点，如果找到
对应的 slot 存在，那么表示叶子节点存在，那么就直接跳转到 found 处，如果该节点
也是一个内部节点，则调用 __radix_tree_next_slot() 函数获得一下层的节点；
反之直接返回可用的节点 slot。

> [radix_tree_chunk_size 源码解析]()

--------------------------------------------------

# <span id="实践">实践</span>

> - [驱动源码](#驱动源码)
>
> - [驱动安装](#驱动安装)
>
> - [驱动配置](#驱动配置)
>
> - [驱动编译](#驱动编译)
>
> - [驱动运行](#驱动运行)
>
> - [驱动分析](#驱动分析)

#### <span id="驱动源码">驱动源码</span>

{% highlight c %}
/*
 * Radix tree.
 *
 * (C) 2019.06.01 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>

/* header of radix-tree */
#include <linux/radix-tree.h>

/*
 * Radix-tree                                             RADIX_TREE_MAP: 6
 *                                  (root)
 *                                    |
 *                          o---------o---------o
 *                          |                   |
 *                        (0x0)               (0x2)
 *                          |                   |
 *                  o-------o------o            o---------o
 *                  |              |                      |
 *                (0x0)          (0x2)                  (0x2)
 *                  |              |                      |
 *         o--------o------o       |             o--------o--------o
 *         |               |       |             |        |        |
 *       (0x0)           (0x1)   (0x0)         (0x0)    (0x1)    (0x3)
 *         A               B       C             D        E        F
 *
 * A: 0x00000000
 * B: 0x00000001
 * C: 0x00000080
 * D: 0x00080080
 * E: 0x00080081
 * F: 0x00080083
 */

/* node */
struct node {
	char *name;
	unsigned long id;
};

/* Radix-tree root */
static struct radix_tree_root BiscuitOS_root;

/* node */
static struct node node0 = { .name = "IDA", .id = 0x20000 };
static struct node node1 = { .name = "IDB", .id = 0x60000 };
static struct node node2 = { .name = "IDC", .id = 0x80000 };
static struct node node3 = { .name = "IDD", .id = 0x30000 };
static struct node node4 = { .name = "IDE", .id = 0x90000 };

static __init int radix_demo_init(void)
{
	struct radix_tree_iter iter;
	void __rcu **slot;

	/* Initialize radix-tree root */
	INIT_RADIX_TREE(&BiscuitOS_root, GFP_ATOMIC);

	/* Insert node into Radix-tree */
	radix_tree_insert(&BiscuitOS_root, node0.id, &node0);
	radix_tree_insert(&BiscuitOS_root, node1.id, &node1);
	radix_tree_insert(&BiscuitOS_root, node2.id, &node2);
	radix_tree_insert(&BiscuitOS_root, node3.id, &node3);
	radix_tree_insert(&BiscuitOS_root, node4.id, &node4);

	/* Iterate over radix tree slot */
	radix_tree_for_each_slot(slot, &BiscuitOS_root, &iter, 0) {
		if (radix_tree_is_internal_node(iter.node))
			printk("Node is a internal node.\n");
		printk("Radix-Tree: %#lx\n", iter.index);
	}

	radix_tree_iter_init(&iter, 0);
	slot = radix_tree_next_chunk(&BiscuitOS_root, &iter, 0);
	printk("Chunk iterator index0: %#lx\n", iter.index);
	radix_tree_next_slot(slot, &iter, 0);
	slot = radix_tree_next_chunk(&BiscuitOS_root, &iter, 0);
	printk("Chunk iterator index1: %#lx\n", iter.index);

	return 0;
}
device_initcall(radix_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 radix.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_RADIX_TREE
+       bool "radix-tree"
+
+if BISCUITOS_RADIX_TREE
+
+config DEBUG_BISCUITOS_RADIX_TREE
+       bool "radix_tree_next_slot"
+
+endif # BISCUITOS_RADIX_TREE
+
endif # BISCUITOS_DRV
{% endhighlight %}

接着修改 Makefile，请参考如下修改：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Makefile b/drivers/BiscuitOS/Makefile
index 82004c9..9909149 100644
--- a/drivers/BiscuitOS/Makefile
+++ b/drivers/BiscuitOS/Makefile
@@ -1 +1,2 @@
obj-$(CONFIG_BISCUITOS_MISC)     += BiscuitOS_drv.o
+obj-$(CONFIG_BISCUITOS_RBTREE)  += radix.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]radix-tree
            [*]radix_tree_next_slot()
{% endhighlight %}

具体过程请参考：

> [Linux 4.19 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-4.19-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 4.19 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-4.19-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 4.19 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-4.19-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
Radix-Tree: 0x20000
Radix-Tree: 0x30000
Radix-Tree: 0x60000
Radix-Tree: 0x80000
Radix-Tree: 0x90000
Chunk iterator index0: 0x20000
Chunk iterator index1: 0x30000
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

获得下一个可用的 slot。

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
