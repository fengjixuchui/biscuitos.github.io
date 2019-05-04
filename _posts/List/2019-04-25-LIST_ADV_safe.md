---
layout: post
title:  "内核双链表遍历：带 safe 和不带 safe 的区别"
date:   2019-05-05 06:55:30 +0800
categories: [HW]
excerpt: 内核双链表遍历：带 safe 和不带 safe 的区别.
tags:
  - Bidirect-list
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

> [Github: Bidirect-list](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/list/bindirect-list)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [简介](#简介)
>
> - [原理实践](#原理实践)
>
> - [原理分析](#原理分析)
>
> - [实践总结](#实践总结)
>
> - [附录](#附录)

-----------------------------------

# <span id="简介">简介</span>

Linux 内核提供了一套完整的双链表机制，是开发者可以在内核私有数据结构中轻松
构建双链表。双链表的操作包括很多种，其中遍历双链表是按正序或者倒序的方式遍历
双链表中的节点。Linux 提供的遍历接口如下：

> - [list_entry: 获得节点对应的入口](https://biscuitos.github.io/blog/LIST_list_entry/)
>
> - [list_first_entry: 获得第一个入口](https://biscuitos.github.io/blog/LIST_list_first_entry/)
>
> - [list_last_entry: 获得最后一个入口](https://biscuitos.github.io/blog/LIST_list_last_entry/)
>
> - [list_first_entry_or_null: 获得第一个入口或 NULL](https://biscuitos.github.io/blog/LIST_list_first_entry_or_null/)
>
> - [list_next_entry: 获得下一个入口](https://biscuitos.github.io/blog/LIST_list_next_entry/)
>
> - [list_prev_entry: 获得前一个入口](https://biscuitos.github.io/blog/LIST_list_prev_entry/)
>
> - [list_for_each: 正序遍历所有节点](https://biscuitos.github.io/blog/LIST_list_for_each/)
>
> - [list_for_each_prev: 倒叙遍历所有节点](https://biscuitos.github.io/blog/LIST_list_for_each_prev/)
>
> - [list_for_each_safe: 安全正序遍历所有节点](https://biscuitos.github.io/blog/LIST_list_for_each_safe/)
>
> - [list_for_each_prev_safe: 安全倒叙遍历所有节点](https://biscuitos.github.io/blog/LIST_list_for_each_prev_safe/)
>
> - [list_for_each_entry: 正序遍历所有入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry/)
>
> - [list_for_each_entry_reverse: 倒叙遍历所有入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_reverse/)
>
> - [list_prepare_entry: 获得指定入口](https://biscuitos.github.io/blog/LIST_list_prepare_entry/)
>
> - [list_for_each_entry_continue: 从指定入口开始正序遍历剩余的入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_continue/)
>
> - [list_for_each_entry_continue_reverse: 从指定入口开始倒叙遍历剩余的入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_continue_reverse/)
>
> - [list_for_each_entry_from: 从指定入口正序遍历剩余入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_from/)
>
> - [list_for_each_entry_from_reverse: 从指定入口倒序遍历剩余入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_from_reverse/)
>
> - [list_for_each_entry_safe: 安全正序遍历所有入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_safe/)
>
> - [list_for_each_entry_safe_continue: 安全从指定入口正序遍历剩余入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_safe_continue/)
>
> - [list_for_each_entry_safe_from: 安全从指定入口正序遍历剩余入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_safe_from/)
>
> - [list_for_each_entry_safe_reverse: 安全从指定入口倒序遍历剩余入口](https://biscuitos.github.io/blog/LIST_list_for_each_entry_safe_reverse/)
>
> - [list_safe_reset_next: 安全获得下一个入口](https://biscuitos.github.io/blog/LIST_list_safe_reset_next/)

从上面的各种遍历接口可以看出，双链表可以通过多种方式遍历。但在上面的遍历接口中，
存在一类带 safe 的接口，这类接口和不带 safe 接口有什么区别，以及实际运用场景如何？
例如：

<span id="原理实践"></span>

> - [list_for_each: 正序遍历所有节点](https://biscuitos.github.io/blog/LIST_list_for_each/)
>
> - [list_for_each_safe: 安全正序遍历所有节点](https://biscuitos.github.io/blog/LIST_list_for_each_safe/)

这两个接口的功能都是从链表头开始，正序遍历所有的接口。但 safe 为安全的遍历所有的接口，
那安全在什么地方？首先查看两个接口的定义差别，如下：

{% highlight c %}
#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)


#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)
{% endhighlight %}

从两个接口的定义差别只看出，list_for_each_safe() 的定义多了一个参数 n，这个参数 n
用于缓存下一个节点，其余并没有什么逻辑上的差异。接着可以查看一下两个函数实际的遍历
效果，如下：

> - [list_for_each 实践](https://biscuitos.github.io/blog/LIST_list_for_each/#%E5%AE%9E%E8%B7%B5)
>
> - [list_for_each_safe 实践](https://biscuitos.github.io/blog/LIST_list_for_each_safe/)

从两个函数的遍历效果来看，并未什么差异。接下来分别用两个函数遍历所有的节点，并在
遍历的过程中，每遍历到一个节点，就删除一个节点。测试代码如下：

###### list_for_each 测试代码

{% highlight c %}
/*
 * bindirect-list
 *
 * (C) 20179.04.25 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * bidirect-list
 *
 * +-----------+<--o    +-----------+<--o    +-----------+<--o    +-----------+
 * |           |   |    |           |   |    |           |   |    |           |
 * |      prev |   o----| prev      |   o----| prev      |   o----| prev      |
 * | list_head |        | list_head |        | list_head |        | list_head |
 * |      next |---o    |      next |---o    |      next |---o    |      next |
 * |           |   |    |           |   |    |           |   |    |           |
 * +-----------+   o--->+-----------+   o--->+-----------+   o--->+-----------+
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* header of list */
#include <linux/list.h>

/* private structure */
struct node {
    const char *name;
    struct list_head list;
};

/* Initialize a group node structure */
static struct node node0 = { .name = "BiscuitOS_node0", };
static struct node node1 = { .name = "BiscuitOS_node1", };
static struct node node2 = { .name = "BiscuitOS_node2", };
static struct node node3 = { .name = "BiscuitOS_node3", };
static struct node node4 = { .name = "BiscuitOS_node4", };
static struct node node5 = { .name = "BiscuitOS_node5", };
static struct node node6 = { .name = "BiscuitOS_node6", };

/* Declaration and implement a bindirect-list */
LIST_HEAD(BiscuitOS_list);

static __init int bindirect_demo_init(void)
{
	struct list_head *np;

	/* add a new entry on special entry */
	list_add_tail(&node0.list, &BiscuitOS_list);
	list_add_tail(&node1.list, &BiscuitOS_list);
	list_add_tail(&node2.list, &BiscuitOS_list);
	list_add_tail(&node3.list, &BiscuitOS_list);
	list_add_tail(&node4.list, &BiscuitOS_list);
	list_add_tail(&node5.list, &BiscuitOS_list);
	list_add_tail(&node6.list, &BiscuitOS_list);

	printk("BiscuitOS_list:\n");
	/* Traverser all node on bindirect-list */
	list_for_each(np, &BiscuitOS_list) {
		printk("%s\n", list_entry(np, struct node, list)->name);
    list_del(np);
  }

	return 0;
}
device_initcall(bindirect_demo_init);
{% endhighlight %}

运行上面的驱动，运行结果如下：

{% highlight c %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
BiscuitOS_list:
BiscuitOS_node0
Unable to handle kernel NULL pointer dereference at virtual address 000000fc
pgd = (ptrval)
[000000fc] *pgd=00000000
Internal error: Oops: 5 [#1] SMP ARM
Modules linked in:
CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.0.0 #110
Hardware name: ARM-Versatile Express
PC is at bindirect_demo_init+0xdc/0x114
LR is at bindirect_demo_init+0xe8/0x114
pc : [<80a39870>]    lr : [<80a3987c>]    psr: 00000013
sp : 9e48fee8  ip : 00000000  fp : 80a5483c
r10: 80a638e0  r9 : 000000c3  r8 : 80a39794
r7 : 00000100  r6 : 80b4fedc  r5 : 809406e0  r4 : 00000100
r3 : 80b4fedc  r2 : 80b4fef4  r1 : 00000200  r0 : 0000000f
Flags: nzcv  IRQs on  FIQs on  Mode SVC_32  ISA ARM  Segment none
Control: 10c5387d  Table: 60004059  DAC: 00000051
Process swapper/0 (pid: 1, stack limit = 0x(ptrval))
Stack: (0x9e48fee8 to 0x9e490000)
fee0:                   80b08c08 80b60440 ffffe000 00000000 80a39794 80102e6c
ff00: 000000c3 8013feec 809878dc 8092f400 00000000 00000006 00000006 808dd188
ff20: 00000000 80b08c08 808ec950 808dd1fc 00000000 9efffe49 9efffe52 1e5a15d1
ff40: 00000000 80b60440 00000007 1e5a15d1 80b60440 00000007 80b691c0 80b691c0
ff60: 80a54834 80a010f8 00000006 00000006 00000000 80a00638 807221e0 00000000
ff80: 00000000 00000000 807221e0 00000000 00000000 00000000 00000000 00000000
ffa0: 00000000 807221e8 00000000 801010e8 00000000 00000000 00000000 00000000
ffc0: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
ffe0: 00000000 00000000 00000000 00000000 00000013 00000000 00000000 00000000
[<80a39870>] (bindirect_demo_init) from [<80102e6c>] (do_one_initcall+0x54/0x1fc)
[<80102e6c>] (do_one_initcall) from [<80a010f8>] (kernel_init_freeable+0x2c4/0x360)
[<80a010f8>] (kernel_init_freeable) from [<807221e8>] (kernel_init+0x8/0x10c)
[<807221e8>] (kernel_init) from [<801010e8>] (ret_from_fork+0x14/0x2c)
Exception stack(0x9e48ffb0 to 0x9e48fff8)
ffa0:                                     00000000 00000000 00000000 00000000
ffc0: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
ffe0: 00000000 00000000 00000000 00000000 00000013 00000000
Code: ebdcbbde e5944000 e1540006 0a00000b (e5141004) 
---[ end trace 6ae005fe4fe2c634 ]---
Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b
---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b ]---
random: fast init done
{% endhighlight %}

从上面的运行结果看出，使用 list_for_each() 函数遍历过程中，如果删除节点，那么
就会引起内核的 panic，内核会提示对 NULL 的引用。

###### list_for_each_safe 测试代码

{% highlight c %}
/*
 * bindirect-list
 *
 * (C) 20179.04.25 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * bidirect-list
 *
 * +-----------+<--o    +-----------+<--o    +-----------+<--o    +-----------+
 * |           |   |    |           |   |    |           |   |    |           |
 * |      prev |   o----| prev      |   o----| prev      |   o----| prev      |
 * | list_head |        | list_head |        | list_head |        | list_head |
 * |      next |---o    |      next |---o    |      next |---o    |      next |
 * |           |   |    |           |   |    |           |   |    |           |
 * +-----------+   o--->+-----------+   o--->+-----------+   o--->+-----------+
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>

/* header of list */
#include <linux/list.h>

/* private structure */
struct node {
    const char *name;
    struct list_head list;
};

/* Initialize a group node structure */
static struct node node0 = { .name = "BiscuitOS_node0", };
static struct node node1 = { .name = "BiscuitOS_node1", };
static struct node node2 = { .name = "BiscuitOS_node2", };
static struct node node3 = { .name = "BiscuitOS_node3", };
static struct node node4 = { .name = "BiscuitOS_node4", };
static struct node node5 = { .name = "BiscuitOS_node5", };
static struct node node6 = { .name = "BiscuitOS_node6", };

/* Declaration and implement a bindirect-list */
LIST_HEAD(BiscuitOS_list);

static __init int bindirect_demo_init(void)
{
	struct list_head *np;
	struct list_head *n;

	/* add a new entry on special entry */
	list_add_tail(&node0.list, &BiscuitOS_list);
	list_add_tail(&node1.list, &BiscuitOS_list);
	list_add_tail(&node2.list, &BiscuitOS_list);
	list_add_tail(&node3.list, &BiscuitOS_list);
	list_add_tail(&node4.list, &BiscuitOS_list);
	list_add_tail(&node5.list, &BiscuitOS_list);
	list_add_tail(&node6.list, &BiscuitOS_list);

	printk("BiscuitOS_list:\n");
	/* Traverser all node on bindirect-list */
	list_for_each_safe(np, n, &BiscuitOS_list) {
		printk("%s\n", list_entry(np, struct node, list)->name);
    list_del(np);
  }

	return 0;
}
device_initcall(bindirect_demo_init);
{% endhighlight %}

运行上面的驱动，运行结果如下：

{% highlight c %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
BiscuitOS_list:
BiscuitOS_node0
BiscuitOS_node1
BiscuitOS_node2
BiscuitOS_node3
BiscuitOS_node4
BiscuitOS_node5
BiscuitOS_node6
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

从上面的运行结果看出，使用 list_for_each_safe() 函数遍历过程中，如果删除节点，那么
不会引起内核的 panic。

### <span id="原理分析">原理分析</span>

在上面的实践中，使用 list_for_each() 函数的时候，当每次遍历一个节点的时候，
list_for_each() 函数通过当前节点找到下一个节点，如下：

{% highlight c %}
#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)
{% endhighlight %}

如果是正常的遍历，那么下一个节点可以从当前节点中获得。如果此时将当前节点从链表中
删除之后，此时当前节点的 next 和 prev 指针已经被指向了一个不可用的地址。如果此时
还再使用当前节点去查找下一个节点的会必然会引起内核 panic。因此此时需要使用 safe 类
的接口解决这个问题，正如 list_for_each_safe() 函数定义，如下：

{% highlight c %}
#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)
{% endhighlight %}

每次遍历的时候，函数都会提前将下一个节点缓存在 n 参数里，如果当前节点被删除之后，
下一个节点也可以从 n 参数中获得，这样不会导致内核 panic。

### <span id="实践总结">实践总结</span>

带 safe 类的接口和不带 safe 类的接口在遍历每个节点的时候，并没有差别。但如果在
遍历的时候需要删除当前节点，那么就需要使用带 safe 类的接口。

-----------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [Linux 双链表](https://biscuitos.github.io/blog/LIST_list_head/)
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
