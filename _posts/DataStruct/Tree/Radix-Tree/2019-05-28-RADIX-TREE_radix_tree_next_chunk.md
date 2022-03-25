---
layout: post
title:  "radix_tree_next_chunk"
date:   2019-05-29 05:30:30 +0800
categories: [HW]
excerpt: Radix-Tree radix_tree_next_chunk().
tags:
  - Radix-Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: radix_tree_next_chunk](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/API/radix_tree_next_chunk)
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

{% highlight ruby %}
/**
 * radix_tree_next_chunk - find next chunk of slots for iteration
 *
 * @root:       radix tree root
 * @iter:       iterator state
 * @flags:      RADIX_TREE_ITER_* flags and tag index
 * Returns:     pointer to chunk first slot, or NULL if iteration is over
 */
void __rcu **radix_tree_next_chunk(const struct radix_tree_root *root,
                             struct radix_tree_iter *iter, unsigned flags)
{% endhighlight %}

radix_tree_next_chunk() 函数用于在遍历 radix-tree 中获得下一块 slot。由于函数较长，
这里分段进行讲解。参数 root 指向 radix tree 的根节点；参数 iter 指向一个 radix tree
的迭代器 struct radix_tree_iter; flags 参数用于表明标志和索引。

{% highlight ruby %}
        unsigned tag = flags & RADIX_TREE_ITER_TAG_MASK;
        struct radix_tree_node *node, *child;
        unsigned long index, offset, maxindex;

        if ((flags & RADIX_TREE_ITER_TAGGED) && !root_tag_get(root, tag))
                return NULL;

        /*
         * Catch next_index overflow after ~0UL. iter->index never overflows
         * during iterating; it can be zero only at the beginning.
         * And we cannot overflow iter->next_index in a single step,
         * because RADIX_TREE_MAP_SHIFT < BITS_PER_LONG.
         *
         * This condition also used by radix_tree_next_slot() to stop
         * contiguous iterating, and forbid switching to the next chunk.
         */
        index = iter->next_index;
        if (!index && iter->index)
                return NULL;
{% endhighlight %}

函数首先判断 flags 参数是否包含 RADIX_TREE_ITER_TAGGED 以及调用 root_tag_get()
函数是否返回 false，如果两者同时满足就直接返回 NULL; 如果不满足上面的条件，那么
从上一次迭代器中获得上一次最后的 chunk end，然后判断如果 chunk end 为 0 且迭代器
的 index 为 0，则没有叶子节点，那么直接返回 NULL。

{% highlight ruby %}
 restart:
        radix_tree_load_root(root, &child, &maxindex);
        if (index > maxindex)
                return NULL;
        if (!child)
                return NULL;

        if (!radix_tree_is_internal_node(child)) {
                /* Single-slot tree */
                iter->index = index;
                iter->next_index = maxindex + 1;
                iter->tags = 1;
                iter->node = NULL;
                __set_iter_shift(iter, 0);
                return (void __rcu **)&root->rnode;
        }
{% endhighlight %}

每一次迭代的时候，首先调用 radix_tree_load_root() 函数获得 radix tree 的根节点，
以及 radix 支持的最大索引值。接着判断本次查找的 index 值是否大于 radix tree 最大
的索引值，如果大于，那么说明 radix tree 中不包含 index 对应的节点。如果
radix_tree_load_root() 函数获得的 child 为空，那么说明该 radix tree 是空树，
直接返回 NULL。接着函数判断当前的节点是不是内部节点，内部节点就是只包含 slots 指针
数组的节点，而不是最后存储私有 entry 的节点。如果不是，那么说明现在已经找到一个
可用的叶子节点，那么将迭代器的 index 指向当前 index， next_index 指向下一个
maxindex + 1.最后返回根节点。

{% highlight ruby %}
        do {
                node = entry_to_node(child);
                offset = radix_tree_descend(node, &child, index);

                if ((flags & RADIX_TREE_ITER_TAGGED) ?
                                !tag_get(node, tag, offset) : !child) {
                        /* Hole detected */
                        if (flags & RADIX_TREE_ITER_CONTIG)
                                return NULL;

                        if (flags & RADIX_TREE_ITER_TAGGED)
                                offset = radix_tree_find_next_bit(node, tag,
                                                offset + 1);
                        else
                                while (++offset < RADIX_TREE_MAP_SIZE) {
                                        void *slot = rcu_dereference_raw(
                                                        node->slots[offset]);
                                        if (is_sibling_entry(node, slot))
                                                continue;
                                        if (slot)
                                                break;
                                }
                        index &= ~node_maxindex(node);
                        index += offset << node->shift;
                        /* Overflow after ~0UL */
                        if (!index)
                                return NULL;
                        if (offset == RADIX_TREE_MAP_SIZE)
                                goto restart;
                        child = rcu_dereference_raw(node->slots[offset]);
                }

                if (!child)
                        goto restart;
                if (child == RADIX_TREE_RETRY)
                        break;
        } while (radix_tree_is_internal_node(child));
{% endhighlight %}

接下来 do while 循环就是用来从顶层往底层找到一个可用的叶子节点。函数每次循环一次
都会调用 entry_to_node() 函数获得下一级入口地址，然后调用 radix_tree_descend()
函数获得 index 对应的下一层对应的 slot 节点，此时可以获得下一层 slot 的指针，以及
index 在这一层的偏移。此时判断 flags 是不是 RADIX_TREE_ITER_TAGGED，如果不是
那么就判断 slot 是否存在，如果存在，那么说明函数需要继续进入下一层继续查找；如果
slot 不存在，那么表示已经找到一个叶子节点。对于找到一个叶子节点，那么判断 flags，
如果 flags 包含 RADIX_TREE_ITER_CONTIG，那么直接返回 NULL，如果此时 flags 不包含
RADIX_TREE_ITER_TAGGED，那么函数就从这一层的 slots[offset] 开始查找可用的叶子
节点，如果找到，则直接调出 while 循环；如果调用 radix_tree_descend() 下一层
child 存在的话，且 child 不等于 RADIX_TREE_RETRY，那么继续判断当前节点是否是
内部结点，如果是就继续循环；反之退出循环。

{% highlight ruby %}
        /* Update the iterator state */
        iter->index = (index &~ node_maxindex(node)) | (offset << node->shift);
        iter->next_index = (index | node_maxindex(node)) + 1;
        iter->node = node;
        __set_iter_shift(iter, node->shift);

        if (flags & RADIX_TREE_ITER_TAGGED)
                set_iter_tags(iter, node, offset, tag);

        return node->slots + offset;
{% endhighlight %}

当找到一个可用的叶子节点之后，就设置 radix_tree 的迭代器，将 index 成员指向 offset
chunk 之后，next_index 成员指向 chunk 的下一个。将 radix_tree_node 节点存储在
node 成员里，最后返回叶子节点所在的 slots[offset] 地址。

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
	radix_tree_next_chunk(&BiscuitOS_root, &iter, 0);
	printk("Chunk iterator index: %#lx\n", iter.index);

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
+       bool "radix_tree_next_chunk"
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
            [*]radix_tree_next_chunk()
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

{% highlight ruby %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
Radix-Tree: 0x20000
Radix-Tree: 0x30000
Radix-Tree: 0x60000
Radix-Tree: 0x80000
Radix-Tree: 0x90000
Chunk iterator index: 0x20000
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

获得 chunk 内的可以叶子节点。

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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
