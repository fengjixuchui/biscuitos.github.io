---
layout: post
title:  "__radix_tree_insert"
date:   2019-05-29 05:30:30 +0800
categories: [HW]
excerpt: Radix-Tree __radix_tree_insert().
tags:
  - Radix-Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: __radix_tree_insert](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/API/__radix_tree_insert)
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
 *      __radix_tree_insert    -    insert into a radix tree
 *      @root:          radix tree root
 *      @index:         index key
 *      @order:         key covers the 2^order indices around index
 *      @item:          item to insert
 *
 *      Insert an item into the radix tree at position @index.
 */
int __radix_tree_insert(struct radix_tree_root *root, unsigned long index,
                        unsigned order, void *item)
{
        struct radix_tree_node *node;
        void __rcu **slot;
        int error;

        BUG_ON(radix_tree_is_internal_node(item));

        error = __radix_tree_create(root, index, order, &node, &slot);
        if (error)
                return error;

        error = insert_entries(node, slot, item, order, false);
        if (error < 0)
                return error;

        if (node) {
                unsigned offset = get_slot_offset(node, slot);
                BUG_ON(tag_get(node, 0, offset));
                BUG_ON(tag_get(node, 1, offset));
                BUG_ON(tag_get(node, 2, offset));
        } else {
                BUG_ON(root_tags_get(root));
        }

        return 0;
}
EXPORT_SYMBOL(__radix_tree_insert);
{% endhighlight %}

__radix_tree_insert() 函数用于将一个新的 radix_tree_node 添加到 radix-tree。
参数 root 指向 radix tree 的根节点；参数 index 代表新的索引值； item 参数指向私有
数据。函数首先调用 BUG_ON() 与 radix_tree_is_internal_node() 函数确保 item 的
内容不是内部节点，新添加的节点不能是内部节点。函数继续调用 __radix_tree_create() 函数
获得新索引对应的父节点以及 slot 指针，调用该函数会增加树的高度和 index 路径上的节点。
获得父节点和 slot 之后，函数调用 insert_entries() 函数将 item 的值插入到 radix-tree
指定位置。函数继续判断父节点是否存在，如果存在，那么调用 get_slot_offset() 函数，
确保父节点的 tag 域对应的 bit 没有置位。最后返回 0.

> - [radix_tree_is_internal_node](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_is_internal_node/)
>
> - [\_\_radix_tree_create](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#__radix_tree_create)
>
> - [insert_entries](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#insert_entries)
>
> - [tag_get](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#tag_get)

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
static RADIX_TREE(BiscuitOS_root, GFP_ATOMIC);

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

	/* Insert node into Radix-tree */
	__radix_tree_insert(&BiscuitOS_root, node0.id, 0, &node0);
	__radix_tree_insert(&BiscuitOS_root, node1.id, 0, &node1);
	__radix_tree_insert(&BiscuitOS_root, node2.id, 0, &node2);
	__radix_tree_insert(&BiscuitOS_root, node3.id, 0, &node3);
	__radix_tree_insert(&BiscuitOS_root, node4.id, 0, &node4);

	/* Iterate over radix tree slot */
	radix_tree_for_each_slot(slot, &BiscuitOS_root, &iter, 0)
		printk("Radix-Tree: %#lx\n", iter.index);

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
+       bool "__radix_tree_insert"
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
            [*]__radix_tree_insert()
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
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

将一个节点插入到 radix-tree 内。

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
