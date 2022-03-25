---
layout: post
title:  "idr_alloc_u32"
date:   2019-06-01 05:30:30 +0800
categories: [HW]
excerpt: IDR idr_alloc_u32().
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

> [Github: idr_alloc_u32](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/IDR/API/idr_alloc_u32)
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
int idr_alloc_u32(struct idr *idr, void *ptr, u32 *nextid,
                        unsigned long max, gfp_t gfp)
{
        struct radix_tree_iter iter;
        void __rcu **slot;
        unsigned int base = idr->idr_base;
        unsigned int id = *nextid;

        if (WARN_ON_ONCE(radix_tree_is_internal_node(ptr)))
                return -EINVAL;
        if (WARN_ON_ONCE(!(idr->idr_rt.gfp_mask & ROOT_IS_IDR)))
                idr->idr_rt.gfp_mask |= IDR_RT_MARKER;

        id = (id < base) ? 0 : id - base;
        radix_tree_iter_init(&iter, id);
        slot = idr_get_free(&idr->idr_rt, &iter, gfp, max - base);
        if (IS_ERR(slot))
                return PTR_ERR(slot);

        *nextid = iter.index + base;
        /* there is a memory barrier inside radix_tree_iter_replace() */
        radix_tree_iter_replace(&idr->idr_rt, &iter, slot, ptr);
        radix_tree_iter_tag_clear(&idr->idr_rt, &iter, IDR_FREE);

        return 0;
}
{% endhighlight %}

idr_alloc_u32() 函数用于从 IDR 中获得一个 ID，并将 ID 与参数 ptr 进行绑定。
参数 idr 指向 idr 根节点；参数 ptr 指向与 ID 绑定的指针；参数 nextid 指向
下一个 id 的指针；参数 max 指向最大 id 值；gfp 参数指向申请节点时的内存标识。
函数首先检查 ptr 参数是不是 radix tree 的一个内部节点，如果是则发出警告并
返回 EINVAL；如果 idr 对应的 radix-tree 的 ROOT_IS_IDR 标志没有置位，那么
函数就设置 idr 对应 radix-tree 的 tag 标志，增加 IDR_RT_MARKER 标识，这样
radix-tree 保留给 IDR 使用。函数节点判断 id 与 base 之间的关系，如果 id
小于 base，那么 id 的值设置为 0；反之设置为 id - base 的值。接着函数调用
radix_tree_iter_init() 初始化了一个 struct radix_tree_iter 结构，该结构
用于存储当前节点的信息和下一个节点的索引。接着调用 idr_get_free() 函数
从 radix-tree 中找到一个空闲符合条件的 slot，并肩 slot 对应的节点的信息
存储在 iter 结构里。函数继续将 iter.index 与 base 的和赋值给 nextid。
接着调用 radix_tree_iter_replace() 去替换 iter 对应的 slot 的值，将 slot
的值替换成 ptr，最后调用 radix_tree_iter_tag_clear() 清除相关的 tag，至此，
从 IDR 中获得一个可用的 slot 存储 ID 绑定的指针。

> - [radix_tree_is_internal_node](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_is_internal_node/)
>
> - [idr_get_free](https://biscuitos.github.io/blog/IDR_idr_get_free/)
>
> - [radix_tree_iter_replace](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_iter_replace)
>
> - [radix_tree_iter_tag_clear](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#radix_tree_iter_tag_clear)

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

/* private node */
struct node {
	const char *name;
};

/* Root of IDR */
static DEFINE_IDR(BiscuitOS_idr);

/* node set */
static struct node node0 = { .name = "IDA" };
static struct node node1 = { .name = "IDB" };
static struct node node2 = { .name = "IDC" };
static struct node node3 = { .name = "IDD" };
static struct node node4 = { .name = "IDE" };

/* ID array */
#define IDR_ARRAY_SIZE	5
static int idr_array[IDR_ARRAY_SIZE];

static __init int idr_demo_init(void)
{
	struct node *np;
	int id = 0;

	/* proload for idr_alloc */
	idr_preload(GFP_KERNEL);

	/* Allocate a id from IDR */
	idr_array[0] = idr_alloc_u32(&BiscuitOS_idr, &node0, &id,
							INT_MAX, GFP_ATOMIC);
	idr_array[1] = idr_alloc_u32(&BiscuitOS_idr, &node1, &id,
							INT_MAX, GFP_ATOMIC);
	idr_array[2] = idr_alloc_u32(&BiscuitOS_idr, &node2, &id,
							INT_MAX, GFP_ATOMIC);
	idr_array[3] = idr_alloc_u32(&BiscuitOS_idr, &node3, &id,
							INT_MAX, GFP_ATOMIC);
	idr_array[4] = idr_alloc_u32(&BiscuitOS_idr, &node4, &id,
							INT_MAX, GFP_ATOMIC);

	/* Interate over all slots */
	idr_for_each_entry(&BiscuitOS_idr, np, id)
		printk("%s's ID %d\n", np->name, id);

	/* end preload section started with idr_preload() */
	idr_preload_end();

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
+       bool "idr_alloc_u32"
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
            [*]idr_alloc_u32()
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
IDA's ID 0
IDB's ID 1
IDC's ID 2
IDD's ID 3
IDE's ID 4
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

将指针绑定到 ID。

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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
