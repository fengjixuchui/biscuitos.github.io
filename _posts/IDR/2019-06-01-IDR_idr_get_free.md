---
layout: post
title:  "idr_get_free"
date:   2019-06-01 05:30:30 +0800
categories: [HW]
excerpt: IDR idr_get_free().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000T.jpg)

> [Github: idr_get_free](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/IDR/API/idr_get_free)
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
void __rcu **idr_get_free(struct radix_tree_root *root,
                              struct radix_tree_iter *iter, gfp_t gfp,
                              unsigned long max)
{
        struct radix_tree_node *node = NULL, *child;
        void __rcu **slot = (void __rcu **)&root->rnode;
        unsigned long maxindex, start = iter->next_index;
        unsigned int shift, offset = 0;
{% endhighlight %}

idr_get_free() 函数用于从 radix-tree 中获得一个可用的 slot。由于函数较长，分段解析。
参数 root 指向 radix-tree 的根；参数 iter 存于存储找到的节点 slot；参数 gfp
用于分配内存时使用的标志；参数 max 表示分配的最大 id。函数定义了局部变量，其中
slot 指向 root->rnode。start 变量表示起始的索引，其指向 iter->next_index。

{% highlight bash %}
 grow:
        shift = radix_tree_load_root(root, &child, &maxindex);
        if (!radix_tree_tagged(root, IDR_FREE))
                start = max(start, maxindex + 1);
        if (start > max)
                return ERR_PTR(-ENOSPC);

        if (start > maxindex) {
                int error = radix_tree_extend(root, gfp, start, shift);
                if (error < 0)
                        return ERR_PTR(error);
                shift = error;
                child = rcu_dereference_raw(root->rnode);
        }
{% endhighlight %}

函数首先调用 radix_tree_load_root() 函数获得当前 radix-tree 的根节点，以及支持
的最大索引值，以及使用整形变量的 shift。函数首先判断当前 radix-tree 的 tag 是否
标记为 IDR_FREE，如果没有标记 IDR_FREE，那么表示目前的 radix-tree 没有空间，那么
需要从 radix-tree 支持的最大索引之后分配，因此将 start 设置为 start 和 maxindex + 1
之间最大的一个。如果此时 start 大于参数 max，那么表示目前 radix-tree 不符合
分配条件，直接返回错误码 ENOSPC；如果上面的条件都满足，那么函数继续执行，并判断
start 是否大于 maxindex，如果大于，那么表示目前 radix-tree 不够存储索引，需要
增加树的高度；如果不大于，那么表示目前的 radix-tree 可以存储需求的索引。当需要增加
树的高度时，函数调用 radix_tree_extend() 函数增加树的高度。并将 shift 指向新的
偏移。最后将 child 重新指向了 radix-tree 的根节点。

{% highlight bash %}
        while (shift) {
                shift -= RADIX_TREE_MAP_SHIFT;
                if (child == NULL) {
                        /* Have to add a child node.  */
                        child = radix_tree_node_alloc(gfp, node, root, shift,
                                                        offset, 0, 0);
                        if (!child)
                                return ERR_PTR(-ENOMEM);
                        all_tag_set(child, IDR_FREE);
                        rcu_assign_pointer(*slot, node_to_entry(child));
                        if (node)
                                node->count++;
                } else if (!radix_tree_is_internal_node(child))
                        break;
{% endhighlight %}

函数根据 shift 的值进行循环，每次 shift 遍历的时候，首先判断对应的 slot 是否存在，
如果不存在，那么调用 radix_tree_node_alloc() 函数去分配需求的节点。并将分配到的
内存初始化，设置 tag 为 IDR_TREE。接着将 slot 指向新分配的节点；如果对于的 slot
存在，那么就判断此时是否已经到达 radix-tree 的叶子，如果到达就结束循环；如果没有
到达，则继续遍历。

{% highlight bash %}
                node = entry_to_node(child);
                offset = radix_tree_descend(node, &child, start);
                if (!tag_get(node, IDR_FREE, offset)) {
                        offset = radix_tree_find_next_bit(node, IDR_FREE,
                                                        offset + 1);
                        start = next_index(start, node, offset);
                        if (start > max)
                                return ERR_PTR(-ENOSPC);
                        while (offset == RADIX_TREE_MAP_SIZE) {
                                offset = node->offset + 1;
                                node = node->parent;
                                if (!node)
                                        goto grow;
                                shift = node->shift;
                        }
                        child = rcu_dereference_raw(node->slots[offset]);
                }
                slot = &node->slots[offset];
        }
{% endhighlight %}

当每次遍历的时候，还未找打叶子节点或刚分配一个新的 slot 节点的时候，函数调用
entry_to_node() 函数获得入口对应的节点地址，然后调用 radix_tree_descend()
函数进入下一层节点。在进入下一层之后，函数调用 tag_get() 函数判断当前 offset
对应的节点是否可用，如果不可用，那么函数就调用 radix_tree_find_next_bit()
去查找下一个可用的节点，调用 next_index() 指向下一个可以的节点。如果此时
start 的值大于 max，那么直接返回 ENOSPC，表示此时 radix-tree 不支持这么大
的索引；如果 start 可以支持，那么继续查找可以节点。如果此时 offset 的值与
RADIX_TREE_MAP_SIZE 的值相等，代表一个节点的所有 slot 已经被使用了，那么
将 offset 指向 node->offset 的兄弟节点，此时将 node 指向其父节点，如果父节点
不存在，那么跳转到 grow，从新申请兄弟节点的路径上的节点，并将 shift 指向
此时 node 的 shift，并不断循环。最后在兄弟节点中找到一个可用的 slot，那么
将 child 指向这个 slot。

{% highlight bash %}
        iter->index = start;
        if (node)
                iter->next_index = 1 + min(max, (start | node_maxindex(node)));
        else
                iter->next_index = 1;
        iter->node = node;
        __set_iter_shift(iter, shift);
        set_iter_tags(iter, node, offset, IDR_FREE);

        return slot;
{% endhighlight %}

在上面的循环结束之后，找到一个可用的 slot。此时将 start 的值赋值给 iter->index。
如果此时 node 存在，那么将 iter->next_index 设置为 max 和 start 之间最小的那一个；
如果 node 不存在，那么 iter->next_index 就设置为 1.将 iter->node 指向 node。
接着调用 __set_iter_shift() 函数设置 iter 的 shift 成员，最后调用 set_iter_tags()
设置，返回 slot。

> - [radix_tree_load_root](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_load_root)
>
> - [radix_tree_tagged](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_tagged)
>
> - [radix_tree_extend](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_extend)
>
> - [radix_tree_node_alloc](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_node_alloc)
>
> - [radix_tree_descend](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_descend)
>
> - [radix_tree_is_internal_node](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_is_internal_node/)
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
 * IDR.
 *
 * (C) 2019.06.04 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>

/* header of radix-tree */
#include <linux/idr.h>

/* Root of IDR */
static DEFINE_IDR(BiscuitOS_idr);

static __init int idr_demo_init(void)
{
	struct radix_tree_iter iter;
	int id = 8;

	radix_tree_iter_init(&iter, id);
	idr_get_free(&BiscuitOS_idr.idr_rt, &iter, GFP_ATOMIC, INT_MAX);
	printk("INDEX: %#lx NEXT: %#lx\n", iter.index, iter.next_index);

	return 0;
}
device_initcall(idr_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 atomic.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_IDR
+       bool "IDR"
+
+if BISCUITOS_IDR
+
+config DEBUG_BISCUITOS_IDR
+       bool "idr_get_free"
+
+endif # BISCUITOS_IDR
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
+obj-$(CONFIG_BISCUITOS_IDR)     += idr.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]IDR
            [*]idr_get_free()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
INDEX: 0x8 NEXT: 0x28
input: AT Raw Set 2 keyboard as /devices/platform/smb@4000000/smb@4000000:motherboard/smb@4000000:motherboard:iofpga@7,00000000/10006000.kmi/serio0/input/input0
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

-----------------------------------------------

# <span id="附录">附录</span>

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
