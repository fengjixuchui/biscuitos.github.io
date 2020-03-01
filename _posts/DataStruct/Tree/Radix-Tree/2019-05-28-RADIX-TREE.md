---
layout: post
title:  "Radix Tree"
date:   2019-06-03 05:30:30 +0800
categories: [HW]
excerpt: Radix-Tree
tags:
  - Radix-Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: Radix Tree](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [Radix-Tree 原理](#CS00)
>
>   - [Raix Tree 简介](#RaixTree 简介)
>
>   - [内核中的 Radix Tree](#内核中的 Radix Tree)
>
>   - [Radix Tree 的架构原理](#Radix Tree 的架构原理)
>
> - [Radix-Tree 最小实践](#实践)
>
>   - [Radix-tree 内核中最小实践](#Radix-tree 内核中最小实践)
>
>   - [Radix-tree 在应用程序中最小实践](#Radix-tree 在应用程序中最小实践)
>
> - [Radix-Tree 在内核中的应用](#CS01)
>
>   - [Radix-Tree 插入操作](#AD0)
>
>   - [Radix-Tree 查询操作](#AD1)
>
>   - [Radix-Tree 修改操作](#AD2)
>
>   - [Radix-Tree 删除操作](#AD3)
>
>   - [Radix-Tree 遍历操作](#AD4)
>
> - [Radix-tree 在应用程序中部署](https://biscuitos.github.io/blog/Tree_RADIX-TREE_UserArrange/)
>
> - [Radix-tree 进阶研究](https://biscuitos.github.io/blog/Tree_RADIX-TREE_Advance/)
>
> - [使用 Data Structure Visualizations 动态分析 Radix-Tree](https://www.cs.usfca.edu/~galles/visualization/RadixTree.html)
>
> - [Radix-Tree 内核接口函数列表](#LIST)
>
> - [附录](#附录)

-----------------------------------

# <span id="CS00"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

# Radix-Tree 原理

> - [Raix Tree 简介](#RaixTree 简介)
>
> - [内核中的 Radix Tree](#内核中的 Radix Tree)
>
> - [Radix Tree 的架构原理](#Radix Tree 的架构原理)

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000124.png)

### <span id="RaixTree 简介">Raix Tree 简介</span>

Radix Tree 是一种压缩 trie，其中 trie 是一种通过保存关联数组 (associative array)
来提供关键字-值 (key-value) 存储与查找的数据结构。通常关键字是字符串，不过也可以是
其他数据类型。Radix Tree 是针对稀疏的长整型数据查找，能快速且节省空间地完成映射。
借助于 Radix Tree，可以实现对于长整型数据类型的路由。利用 Radix Tree 可以根据一个
长整型 (比如一个长 ID) 快速查找到其对应的对象指针, 这比用 hash 映射来的简单，也更
节省空间，使用 hash 映射 hash 函数难以设计，不恰当的 hash 函数可能增大冲突，或浪费
空间。

Radix tree 是一种多叉搜索树，树的叶子结点是实际的数据条目。每个结点有一个固定的、
2^n 指针指向子结点 (每个指针称为槽 slot，n 为划分的基的大小)

### <span id="内核中的 Radix Tree">内核中的 Radix Tree</span>

Linux 4.20 之前的内核使用 Radix Tree 管理很多内核基础数据结构，其中包括 IDR 机制。
但 Linux 4.20 之后，内核采用新的数据结构 xarray 代替了 Radix Tree。内核关于 Radix
Tree 的源码位于：

{% highlight bash %}
include/linux/radix-tree.h
lib/radix-tree.c
{% endhighlight %}

在 Linux 4.20 之前的内核中，Radix Tree 作为重要的基础数据，内核定义了一下数据结构
对 Radix Tree 进行维护。

###### struct radix_tree_node

{% highlight bash %}
/*
 * @count is the count of every non-NULL element in the ->slots array
 * whether that is an exceptional entry, a retry entry, a user pointer,
 * a sibling entry or a pointer to the next level of the tree.
 * @exceptional is the count of every element in ->slots which is
 * either radix_tree_exceptional_entry() or is a sibling entry for an
 * exceptional entry.
 */
struct radix_tree_node {
        unsigned char   shift;          /* Bits remaining in each slot */
        unsigned char   offset;         /* Slot offset in parent */
        unsigned char   count;          /* Total entry count */
        unsigned char   exceptional;    /* Exceptional entry count */
        struct radix_tree_node *parent;         /* Used when ascending tree */
        struct radix_tree_root *root;           /* The tree we belong to */
        union {
                struct list_head private_list;  /* For tree user */
                struct rcu_head rcu_head;       /* Used when freeing node */
        };
        void __rcu      *slots[RADIX_TREE_MAP_SIZE];
        unsigned long   tags[RADIX_TREE_MAX_TAGS][RADIX_TREE_TAG_LONGS];
};
{% endhighlight %}

Linux 内核中，使用 struct radix_tree_node 数据结构定义了一个 radix-tree 的节点，
节点包含了多个成员，每个成员为构成 radix-tree 起到了关键作用。shift 成员用于指向
当前节点占用所有的偏移；offset 存储该节点在父节点的 slot 的偏移；参数 count 表示
当前节点有多少个 slot 已经被使用；exceptional 表示当前节点有多少个 exceptional
节点；参数 parent 指向父节点；参数 root 指向根节点；参数 slots 是数组，数组的成员
指向下一级的节点；参数 tags 用于标识当前节点包含了指定 tag 的节点数。

###### struct radix_tree_root

{% highlight bash %}
struct radix_tree_root {
        spinlock_t              xa_lock;
        gfp_t                   gfp_mask;
        struct radix_tree_node  __rcu *rnode;
};
{% endhighlight %}

Linux 内核中，使用 struct radix_tree_root 维护 radix-tree 的根节点。xa_lock
是一个自旋锁；参数 gfp_mask 用于标识 radix-tree 的属性以及 radix-tree 节点申请
内核的标识；参数 rnode 指向 radix-tree 的根节点。

###### struct radix_tree_iter

{% highlight bash %}
/**
 * struct radix_tree_iter - radix tree iterator state
 *
 * @index:      index of current slot
 * @next_index: one beyond the last index for this chunk
 * @tags:       bit-mask for tag-iterating
 * @node:       node that contains current slot
 * @shift:      shift for the node that holds our slots
 *
 * This radix tree iterator works in terms of "chunks" of slots.  A chunk is a
 * subinterval of slots contained within one radix tree leaf node.  It is
 * described by a pointer to its first slot and a struct radix_tree_iter
 * which holds the chunk's position in the tree and its size.  For tagged
 * iteration radix_tree_iter also holds the slots' bit-mask for one chosen
 * radix tree tag.
 */
struct radix_tree_iter {
        unsigned long   index;
        unsigned long   next_index;
        unsigned long   tags;
        struct radix_tree_node *node;
#ifdef CONFIG_RADIX_TREE_MULTIORDER
        unsigned int    shift;
#endif
};
{% endhighlight %}

Linux 内核定义了 struct radix_tree_iter 结构用于遍历 radix-tree 中所有的
slots 时候，用于存储每次遍历到的节点。index 成员指明了被遍历到节点的索引值；成员
next_index 成员用于指明下一个节点的索引值；成员 tags 用于标识当前节点的 tag 属性；
成员 node 用于指明遍历到的节点。

### <span id="Radix Tree 的架构原理">Radix Tree 的架构原理</span>

Linux 内核中，使用一个 struct radix_tree_root 结构作为根维护整棵 radix-tree。
每个节点采用 struct radix_tree_node 进行维护。在 radix-tree 中，节点被分作三
类，第一类称为 internal 内部节点，内部节点不存储任何私有数据，主要用于维护下一级
节点的 slots 数组；第二类称为 exceptional 节点，这类节点与 internal 类似，
专门维护 radix-tree 中的下一级 exceptional 节点；第三类就是一般节点，一般节点
的 slots 数组里面存储的就是私有数据，且不包含任何 internal 节点。

Radix-tree 在内核中用于将一个长整型数据与一个指针类型的数据相互对应，对应的原理
就是与长整型的特定字段作为 radix-tree 的特定节点的索引进行布局，因此形成了长整型
数据中多个字段作为 radix-tree 不同层 slots 的入口偏移，如下图：

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000124.png)

如上图所示，存在一个长整型与一棵 radix-tree。linux 从长整型的左到右，依次按特定
长度的域作为 radix-tree 中每一层的索引。例如 radix-tree 的第一层采用了长整型最左边
域的值 8 作为索引，找到第一个 slots 入口，接着用 04 找到下一级 slots 入口，然后
03 再找到下一级的 slots，最后 06 找到最终 slot 的入口地址。通过这样的对应关系，
内核将长整型对应的私有数据存储在最终的 slot 里，这样就实现了一个长整型对应一个指针
的逻辑。

###### Radix-tree 的 tag 原理

Linux 内核中，radix-tree 的根节点使用 struct radix_tree_root 进行维护，其包含
了名为 gfp_mask 的成员，这个成员用于指明 radix-tree 的用途，该成员被拆分做多个
域进行使用，如下图：

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000125.png)

ROOT tag 域用于指明 radixt-tree 的某些属性；GFP tag 域用于指明 radix-tree 在
向内存申请节点时使用的 GFP 标志；IDR tag 域用于指明 Radix tree 是否用于 IDR 机制。

在 struct radix_tree_node 里也存在 tag 成员，该成员是一个二维数组，该二维数组的
作用就是常说的用空间换时间的做法。在 radix-tree 查找特定的 slots 时候，将节点 tag
设置为不同的属性，例如在内核中，tag 经常被设置为 dirty 和 access，也就是 dirty tag
的时候，二维数组的第一位就是一个 BitMap，用于标记该 tag 下有多少 slot 被使用，内核
只需查找 bitmap 的使用情况，不必每个 slots 都进行查找，这将大大增加查找的速度。
具体请看 IDR 机制。struct radix_tree_node 的 tag 定义如下：

{% highlight bash %}
unsigned long   tags[RADIX_TREE_MAX_TAGS][RADIX_TREE_TAG_LONGS];
{% endhighlight %}

Radix-tree 一共支持 RADIX_TREE_MAX_TAGS 中 tag，每种 tag 包含了一个 Bitmap，
其长度为 RADIX_TREE_TAG_LONGS 个 unsigned long，每个 bit 代表一个 slots，
如果有 slot 被使用，那么该 Bitmap 对应的位就被置位；反之被清零。

--------------------------------------------------
<span id="实践"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

## Radix-tree 实践

> - [Radix-tree 内核中最小实践](#Radix-tree 内核中最小实践)
>
> - [Radix-tree 在应用程序中最小实践](#Radix-tree 在应用程序中最小实践)

--------------------------------------
<span id="Radix-tree 内核中最小实践"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000C.jpg)

### Radix-tree 内核中最小实践

> - [驱动源码](#AA驱动源码)
>
> - [驱动安装](#AA驱动安装)
>
> - [驱动配置](#AA驱动配置)
>
> - [驱动编译](#AA驱动编译)
>
> - [驱动运行](#AA驱动运行)
>
> - [驱动分析](#AA驱动分析)

#### <span id="AA驱动源码">驱动源码</span>

> [实践源码 Radix-Tree on GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/API/minix)

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
	struct node *np;

	/* Initialize Radix-tree root */
	INIT_RADIX_TREE(&BiscuitOS_root, GFP_ATOMIC);

	/* Insert node into Radix-tree */
	radix_tree_insert(&BiscuitOS_root, node0.id, &node0);
	radix_tree_insert(&BiscuitOS_root, node1.id, &node1);
	radix_tree_insert(&BiscuitOS_root, node2.id, &node2);
	radix_tree_insert(&BiscuitOS_root, node3.id, &node3);
	radix_tree_insert(&BiscuitOS_root, node4.id, &node4);

	/* search struct node by id */
	np = radix_tree_lookup(&BiscuitOS_root, 0x60000);
	BUG_ON(!np);
	printk("Radix: %s id %#lx\n", np->name, np->id);

	/* Delect a node from radix-tree */
	radix_tree_delete(&BiscuitOS_root, np->id);

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
+config BISCUITOS_RADIX
+       bool "radix tree"
+
+if BISCUITOS_RADIX
+
+config DEBUG_BISCUITOS_RADIX
+       bool "radix tree mini"
+
+endif # BISCUITOS_RADIX
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
+obj-$(CONFIG_BISCUITOS_RADIX)   += radix.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]radix tree
            [*]radix tree mini
{% endhighlight %}

具体过程请参考：

> [Linux 4.19.1 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-4.19.1-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="AA驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 4.19.1 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-4.19.1-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="AA驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 4.19.1 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-4.19.1-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

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

--------------------------------------
<span id="Radix-tree 在应用程序中最小实践"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

### Radix-tree 在应用程序中最小实践

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 Radix-Tree on GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree/Basic)

开发者也可以使用如下命令获得：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/radix-tree/Basic/Makefile
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/radix-tree/Basic/README.md
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/radix-tree/Basic/radix_run.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/radix-tree/Basic/radix.c
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/radix-tree/Basic/radix.h
{% endhighlight %}

实践源码具体内容如下：

{% highlight c %}
/*
 * Radix-Tree Manual.
 *
 * (C) 2019.05.27 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>

/* radix-tree */
#include <radix.h>

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
 *
 */

/* node */
struct node {
	char *name;
	unsigned long id;
};

/* Radix-tree root */
static struct radix_tree_root BiscuitOS_root;

/* Range: [0x00000000, 0x00000040] */
static struct node node0 = { .name = "IDA", .id = 0x21 };
/* Range: [0x00000040, 0x00001000] */
static struct node node1 = { .name = "IDB", .id = 0x876 };
/* Range: [0x00001000, 0x00040000] */
static struct node node2 = { .name = "IDC", .id = 0x321FE };
/* Range: [0x00040000, 0x01000000] */
static struct node node3 = { .name = "IDD", .id = 0x987654 };
/* Range: [0x01000000, 0x40000000] */
static struct node node4 = { .name = "IDE", .id = 0x321FEDCA };

int main()
{
	struct node *np;
	struct radix_tree_iter iter;
	void **slot;

	/* Initialize Radix-tree root */
	INIT_RADIX_TREE(&BiscuitOS_root, GFP_ATOMIC);

	/* Insert node into Radix-tree */
	radix_tree_insert(&BiscuitOS_root, node0.id, &node0);
	radix_tree_insert(&BiscuitOS_root, node1.id, &node1);
	radix_tree_insert(&BiscuitOS_root, node2.id, &node2);
	radix_tree_insert(&BiscuitOS_root, node3.id, &node3);
	radix_tree_insert(&BiscuitOS_root, node4.id, &node4);

	/* Iterate over Radix-tree */
	radix_tree_for_each_slot(slot, &BiscuitOS_root, &iter, 0)
		printf("Index: %#lx\n", iter.index);

	/* search struct node by id */
	np = radix_tree_lookup(&BiscuitOS_root, 0x321FE);
	BUG_ON(!np);
	printf("Radix: %s ID: %lx\n", np->name, np->id);

	/* Delete a node from radix-tree */
	radix_tree_delete(&BiscuitOS_root, node0.id);
	radix_tree_delete(&BiscuitOS_root, node1.id);
	radix_tree_delete(&BiscuitOS_root, node2.id);

	return 0;
}
{% endhighlight %}

--------------------------------------

#### <span id="源码编译">源码编译</span>

使用如下命令进行编译：

{% highlight ruby %}
make clean
make
{% endhighlight %}

--------------------------------------

#### <span id="源码运行">源码运行</span>

实践源码的运行很简单，可以使用如下命令，并且运行结果如下：

{% highlight ruby %}
radix-tree/Basic$ ./radix
Index: 0x21
Index: 0x876
Index: 0x321fe
Index: 0x987654
Index: 0x321fedca
Radix: IDC ID: 321fe
{% endhighlight %}

-----------------------------------

# <span id="CS01"></span>

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

# Radix-Tree 在内核中的应用

> - [Radix-Tree 插入操作](#AD0)
>
> - [Radix-Tree 查询操作](#AD1)
>
> - [Radix-Tree 修改操作](#AD2)
>
> - [Radix-Tree 删除操作](#AD3)
>
> - [Radix-Tree 遍历操作](#AD4)

------------------------------------

#### <span id="AD0">Radix-Tree 插入操作</span>

Radix-tree 提供了一套完整的插入机制，内核通过 index 索引值进行插入。根据
radix-tree 的原理，内核插入 index 到 radix-tree 之后，内核首先判断当前
radix-tree 是否能够存储 index，如果能，那么继续插入；如果不能，那么内核就
增加 radix-tree 树的高度，树的高度一增加，原始的节点的深度也增加，原始的根
节点变成了新根节点的 slot[0] 的孩子。内核在确保 radix-tree 可以存储 index
之后，就将 index 的域从中将特定位置开始，从左到右以此作为索引在 radix-tree
中查找 slots，如下图：

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000124.png)

如上图所示，存在一个长整型与一棵 radix-tree。linux 从长整型的左到右，依次按特定
长度的域作为 radix-tree 中每一层的索引。例如 radix-tree 的第一层采用了长整型最左边
域的值 8 作为索引，找到第一个 slots 入口，接着用 04 找到下一级 slots 入口，然后
03 再找到下一级的 slots，最后 06 找到最终 slot 的入口地址。通过这样的对应关系，
内核将长整型对应的私有数据存储在最终的 slot 里，这样就实现了一个长整型对应一个指针
的逻辑。

内核提供了插入相关的接口函数，开发者可以参考下面的文章进行对应的实践：

> - [\_\_radix_tree_insert](https://biscuitos.github.io/blog/RADIX-TREE___radix_tree_insert/)
>
> - [radix_tree_insert](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_insert/)

------------------------------------

#### <span id="AD1">Radix-Tree 查询操作</span>

Linux 提供了相应的接口用于在 radix-tree 中查找符合条件的节点，节点的 slot 存储
着对应的私有数据。Radix-tree 的查找很将当，就是通过 index 进行查找，radix-tree
将 index 拆分成多个索引，从根节点开始，在每一层节点的 slots 数组里找到指定的
入口地址，然后进入下一层继续查找，知道找到最后一个 slot，如果找到，那么就返回
私有数据；如果没有找到，则返回对应的错误码。如下图：

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000124.png)

Linux 内核为了加快在 radix-tree 中的查找，采用了一种称为空间换时间的做法，
在 radix-tree 查找特定的 slots 时候，将节点 tag
设置为不同的属性，例如在内核中，tag 经常被设置为 dirty 和 access，也就是 dirty tag
的时候，二维数组的第一位就是一个 BitMap，用于标记该 tag 下有多少 slot 被使用，内核
只需查找 bitmap 的使用情况，不必每个 slots 都进行查找，这将大大增加查找的速度。
具体请看 IDR 机制。struct radix_tree_node 的 tag 定义如下：

{% highlight bash %}
unsigned long   tags[RADIX_TREE_MAX_TAGS][RADIX_TREE_TAG_LONGS];
{% endhighlight %}

Radix-tree 一共支持 RADIX_TREE_MAX_TAGS 中 tag，每种 tag 包含了一个 Bitmap，
其长度为 RADIX_TREE_TAG_LONGS 个 unsigned long，每个 bit 代表一个 slots，
如果有 slot 被使用，那么该 Bitmap 对应的位就被置位；反之被清零。基于这种机制，
内核能在 radix-tree 中快速找到所需的节点。

内核提供了查找相关的接口函数，开发者可以通过下面文章进行相关实践：

> - [\_\_radix_tree_lookup](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#___radix_tree_lookup)
>
> - [radix_tree_lookup](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_lookup/)

------------------------------------

#### <span id="AD2">Radix-Tree 修改操作</span>

Linux 内核中，对 radix-tree 的修改操作指的是修改 index 对应的指针私有数据。
由 radix-tree 的原理可以知道，index 在 radix-tree 中对应 slot 中存储私有指针，
因此修改操作就是修改特定的私有指针值。

Linux 内核在以下两种情况下会触发修改操作，一是主动修改私有指针，二是删除操作时
也会修改。修改的逻辑是通过查找操作找到 index 对应的私有指针，然后修改节点对应 slot
的值即可，开发者可以参考如下文章进行相应的实践：

> - [\_\_radix_tree_delete](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#___radix_tree_delete)
>
> - [radix_tree_delete](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_delete/)
>
> - [replace_slot](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#replace_slot)

------------------------------------

#### <span id="AD3">Radix-Tree 删除操作</span>

Linux 内核也提供了 radix-tree 的删除操作。删除一个节点的基本思路是：通过 index
找到对应的节点，然后将其父节点的 slots 入口设置为 NULL，如果删除的节点是一个内部
节点，且该节点没有任何的孩子，那么 radix-tree 就是进行 shrink 操作，减小树的高度。
开发者可以通过下面的文章进行详细了解和实践：

> - [\_\_radix_tree_delete](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#___radix_tree_delete)
>
> - [radix_tree_delete](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_delete/)
>
> - [radix_tree_shrink](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_shrink)

------------------------------------

#### <span id="AD4">Radix-Tree 遍历操作</span>

内核也提供了一套接口用于遍历 radix-tree 树上的所有 slot。内核定义了一个
struct radix_tree_iter 数据结构用于存储每次遍历的数据，每次遍历到一个 slot之后，
都会将必要的信息存储在 struct radix_tree_iter 内，以便便捷使用。开发者
可以参考如下文章进行实践：

> - [radix_tree_for_each_slot](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_for_each_slot/)

-----------------------------------

<span id="LIST"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

## Radix-Tree 内核接口函数列表

> - [all_tag_set](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#all_tag_set)
>
> - [delete_node](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#delete_node)
>
> - [entry_to_node](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#entry_to_node)
>
> - [INIT_RADIX_TREE](https://biscuitos.github.io/blog/RADIX-TREE_INIT_RADIX_TREE/)
>
> - [insert_entries](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#insert_entries)
>
> - [is_idr](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#is_idr)
>
> - [node_maxindex](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#node_maxindex)
>
> - [\_\_radix_tree_create](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#___radix_tree_create)
>
> - [\_\_radix_tree_delete](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#___radix_tree_delete)
>
> - [radix_tree_delete](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_delete/)
>
> - [radix_tree_delete_item](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_delete_item/)
>
> - [radix_tree_descend](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_descend)
>
> - [radix_tree_extend](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_extend)
>
> - [radix_tree_empty](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_empty/)
>
> - [radix_tree_exceptional_entry](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_exceptional_entry/)
>
> - [radix_tree_find_next_bit](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_find_next_bit)
>
> - [radix_tree_for_each_slot](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_for_each_slot/)
>
> - [RADIX_TREE_INIT](https://biscuitos.github.io/blog/RADIX-TREE_RADIX_TREE_INIT/)
>
> - [radix_tree_iter_tag_clear](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_iter_tag_clear)
>
> - [radix_tree_iter_find](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_iter_find)
>
> - [radix_tree_iter_init](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_iter_init/)
>
> - [\_\_radix_tree_insert](https://biscuitos.github.io/blog/RADIX-TREE___radix_tree_insert/)
>
> - [radix_tree_insert](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_insert/)
>
> - [radix_tree_is_internal_node](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_is_internal_node/)
>
> - [radix_tree_load_root](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_load_root)
>
> - [\_\_radix_tree_lookup](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#___radix_tree_lookup)
>
> - [radix_tree_lookup](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_lookup/)
>
> - [radix_tree_next_chunk](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_next_chunk/)
>
> - [radix_tree_next_slot](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_next_slot/)
>
> - [radix_tree_node_alloc](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_node_alloc)
>
> - [radix_tree_node_free](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_node_free)
>
> - [radix_tree_node_rcu_free](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_node_rcu_free)
>
> - [radix_tree_iter_replace](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_iter_replace)
>
> - [\_\_radix_tree_replace](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#__radix_tree_replace)
>
> - [radix_tree_shrink](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_shrink)
>
> - [radix_tree_tagged](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_tagged)
>
> - [replace_slot](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#replace_slot)
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

-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [Radix Tree](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/radix-tree)
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
