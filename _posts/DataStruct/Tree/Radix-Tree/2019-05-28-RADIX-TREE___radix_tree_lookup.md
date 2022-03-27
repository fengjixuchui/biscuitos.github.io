---
layout: post
title:  "__radix_tree_lookup"
date:   2019-05-29 05:30:30 +0800
categories: [HW]
excerpt: Radix-Tree __radix_tree_lookup().
tags:
  - Radix-Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: __radix_tree_lookup](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/API/__radix_tree_lookup)
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
 *      __radix_tree_lookup     -       lookup an item in a radix tree
 *      @root:          radix tree root
 *      @index:         index key
 *      @nodep:         returns node
 *      @slotp:         returns slot
 *
 *      Lookup and return the item at position @index in the radix
 *      tree @root.
 *
 *      Until there is more than one item in the tree, no nodes are
 *      allocated and @root->rnode is used as a direct slot instead of
 *      pointing to a node, in which case *@nodep will be NULL.
 */
void *__radix_tree_lookup(const struct radix_tree_root *root,
                          unsigned long index, struct radix_tree_node **nodep,
                          void __rcu ***slotp)
{
        struct radix_tree_node *node, *parent;
        unsigned long maxindex;
        void __rcu **slot;
{% endhighlight %}

__radix_tree_lookup() 函数用于通过索引值查找对应的私有数据。由于代码较长，分段解析。
参数 root 指向根节点；index 参数代表需要查找的值；参数 nodep 指向开始查找的父节点；
slotp 指向开始查找的 slot。

{% highlight bash %}
restart:
       parent = NULL;
       slot = (void __rcu **)&root->rnode;
       radix_tree_load_root(root, &node, &maxindex);
       if (index > maxindex)
               return NULL;
{% endhighlight %}

函数首先获得根节点，并获得当前 radix tree 支持的最大索引数，如果需要查找的索引值大于
最大索引值，那么表示当前 radix-tree 中并不包含要查找的数据。

{% highlight bash %}
       while (radix_tree_is_internal_node(node)) {
               unsigned offset;

               if (node == RADIX_TREE_RETRY)
                       goto restart;
               parent = entry_to_node(node);
               offset = radix_tree_descend(parent, &node, index);
               slot = parent->slots + offset;
       }
{% endhighlight %}

函数接着调用了一个 while 循环，从根节点开始查找，调用 radix_tree_is_internal_node()
函数判断当前遍历的到的父节点是否是内部节点。在 radix tree 内，只要不是最后的叶子节点，
节点被成为内部节点。如果父节点的是 RADIX_TREE_RETRY，那么函数跳转到 restart 处继续
执行；如果不是，那么函数继续调用 entry_to_node() 获得父节点的指针后，调用
radix_tree_descend() 函数进入下一层的入口，slot 指向新一层的 offset 对应的入口。
循环最后，会找到 index 参数对应的 slot 入口。

{% highlight bash %}
if (nodep)
        *nodep = parent;
if (slotp)
        *slotp = slot;
return node;
{% endhighlight %}

最后将 index 参数在 radix tree 的父节点以及指向 slot 的指针返回。至此获得 index 对应
的 slot 入口，该 slot 入口指向就是 index 参数对应的私有数据。如果此时 slot 不存在，
那么就代表 radix tree 中不存在 index 参数对应的私有数据。

> - [radix_tree_load_root](/blog/RADIX-TREE_SourceAPI/#radix_tree_load_root)
>
> - [radix_tree_is_internal_node](/blog/RADIX-TREE_radix_tree_is_internal_node/)
>
> - [entry_to_node](/blog/RADIX-TREE_SourceAPI/#entry_to_node)
>
> - [radix_tree_descend](/blog/RADIX-TREE_SourceAPI/#radix_tree_descend)

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
	struct node *np;

	/* Insert node into Radix-tree */
	__radix_tree_insert(&BiscuitOS_root, node0.id, 0, &node0);
	__radix_tree_insert(&BiscuitOS_root, node1.id, 0, &node1);
	__radix_tree_insert(&BiscuitOS_root, node2.id, 0, &node2);
	__radix_tree_insert(&BiscuitOS_root, node3.id, 0, &node3);
	__radix_tree_insert(&BiscuitOS_root, node4.id, 0, &node4);

	/* Iterate over radix tree slot */
	radix_tree_for_each_slot(slot, &BiscuitOS_root, &iter, 0)
		printk("Radix-Tree: %#lx\n", iter.index);

	/* search struct node by id */
	np = __radix_tree_lookup(&BiscuitOS_root, 0x60000, NULL, NULL);
	BUG_ON(!np);
	printk("Radix: %s id %#lx\n", np->name, np->id);

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
+       bool "__radix_tree_lookup"
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
            [*]__radix_tree_lookup()
{% endhighlight %}

具体过程请参考：

> [Linux 4.19 开发环境搭建 -- 驱动配置](/blog/Linux-4.19-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 4.19 开发环境搭建 -- 驱动编译](/blog/Linux-4.19-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 4.19 开发环境搭建 -- 驱动运行](/blog/Linux-4.19-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
Radix-Tree: 0x20000
Radix-Tree: 0x30000
Radix-Tree: 0x60000
Radix-Tree: 0x80000
Radix-Tree: 0x90000
Radix: IDB id 0x60000
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在 radix tree 中查找指定的节点。

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Radix Tress](/blog/Tree_RADIX_TREE/)
>
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
