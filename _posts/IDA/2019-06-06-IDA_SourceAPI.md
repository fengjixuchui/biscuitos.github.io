---
layout: post
title:  "IDA Source code"
date:   2019-06-05 05:30:30 +0800
categories: [HW]
excerpt: IDA Source code().
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000T.jpg)

> [Github: IDA Source code](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/IDA/API)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

-----------------------------------

# <span id="ida_get_new_above">ida_get_new_above</span>

{% highlight bash %}
static int ida_get_new_above(struct ida *ida, int start)
{
        struct radix_tree_root *root = &ida->ida_rt;
        void **slot;
        struct radix_tree_iter iter;
        struct ida_bitmap *bitmap;
        unsigned long index;
        unsigned bit, ebit;
        int new;
{% endhighlight %}

ida_get_new_above() 函数的作用是在 IDA 对应的 radix-tree 中为 bitmap
申请一个 slot，用于存储 bitmap 的值，然后从 bitmap 中找到一个可用的 bit，
并返回 bit 的偏移，以此作为 ID；由于代码太长，分段解析。
参数 ida 指向 IDA 根；参数 start 代表起始 ID 值。函数定义了很多局部变量，
其中 root 指向 IDA radix-tree 的根。

{% highlight bash %}
        index = start / IDA_BITMAP_BITS;
        bit = start % IDA_BITMAP_BITS;
        ebit = bit + RADIX_TREE_EXCEPTIONAL_SHIFT;

        slot = radix_tree_iter_init(&iter, index);
{% endhighlight %}

函数以 start 为基础，计算了 start 在 IDA_BITMAP_BITS 中位置，在 IDA 中，IDA
将 128 bytes 称为一个 chunk，使用 IDA_CHUNK_SIZE 表示，每个 IDA_CHUNK_SIZE 使用
long 的个数称为 IDA_BITMAP_LONGS， IDA 每个 chunk 总共使用的 bits 数为
IDA_BITMAP_BITS。于是函数首先计算 start 在 IDA 中的第几个 chunk，将其值存储在
index 中，然后通过求余运算计算 start 在该 chunk 内的偏移，存储在 bit 里面。
函数将 bit 的值加上 RADIX_TREE_EXCEPTIONAL_SHIFT 计算在 exceptional 节点
内的偏移。接着函数调用 radix_tree_iter_init() 函数初始化了 iter 变量。

{% highlight bash %}
        for (;;) {
                if (slot)
                        slot = radix_tree_next_slot(slot, &iter,
                                                RADIX_TREE_ITER_TAGGED);

                if (!slot) {
                        slot = idr_get_free(root, &iter, GFP_NOWAIT, IDA_MAX);
                        if (IS_ERR(slot)) {
                                if (slot == ERR_PTR(-ENOMEM))
                                        return -EAGAIN;
                                return PTR_ERR(slot);
                        }
                }
                if (iter.index > index) {
                        bit = 0;
                        ebit = RADIX_TREE_EXCEPTIONAL_SHIFT;
                }
{% endhighlight %}

函数接着调用一个死循环，在每次循环中，函数首先判断 slot 是否存在，如果存在，
则调用 radix_tree_next_slot() 获得下一个可用的 slot 入口，并将该入口存储
在 slot 中。如果此时 slot 还未空，那么表示无法获得一个可用的入口，此时调用
idr_get_free() 函数从 radix-tree 中找到一个可用的入口，其中包括增加树的
高度来查找可用的入口。如果此时还是不能找到 slot，那么直接返回错误代码。
如果此时 index 的值比 iter.index 的值还小，那么将 bit 设置为 0， ebit
设置为 RADIX_TREE_EXCEPTIONAL_SHIFT。

{% highlight bash %}
                new = iter.index * IDA_BITMAP_BITS;
                bitmap = *slot;
                if (radix_tree_exception(bitmap)) {
                        unsigned long tmp = (unsigned long)bitmap;

                        ebit = find_next_zero_bit(&tmp, BITS_PER_LONG, ebit);
                        if (ebit < BITS_PER_LONG) {
                                tmp |= 1UL << ebit;
                                *slot = (void *)tmp;
                                return new + ebit -
                                        RADIX_TREE_EXCEPTIONAL_SHIFT;
                        }
                        bitmap = ida_bitmap;
                        if (!bitmap)
                                return -EAGAIN;
                        bitmap->bitmap[0] = tmp >> RADIX_TREE_EXCEPTIONAL_SHIFT;
                        *slot = bitmap;
                }
{% endhighlight %}

函数继续遍历，将 iter.index 的值与 IDA_BITMAP_BITS 的值相乘，存储到 new 变量里，
接着将找到的 slot 指向的内容赋值给 bitmap。函数调用 radix_tree_exception()
函数判断该节点是否是一个 exceptional 节点，如果是，则将 bitmap 的值存储在 tmp 中，
使用 find_next_zero_bit 从 tmp 中找到一个为 0 的 bit。如果此时找到的 bit 小于
BITS_PER_LONG，那么将 ebit 对应的位在 tmp 中置位，然后在将 tmp 的值更新到 slot
中，最后返回 (new+ebit-RADIX_TREE_EXCEPTIONAL_SHIFT) 的值；如果此时 ebit 的
值比 BITS_PER_LONG 还大，那么函数将 bitmap 指向 ida_bitmap. 然后将 tmp 右移
RADIX_TREE_EXCEPTIONAL_SHIFT 的值存储到 bitmap->bitmap[0] 里面。最后将 bitmap
的值更新到 slot 中。

{% highlight bash %}
                if (bitmap) {
                        bit = find_next_zero_bit(bitmap->bitmap,
                                                        IDA_BITMAP_BITS, bit);
                        new += bit;
                        if (new < 0)
                                return -ENOSPC;
                        if (bit == IDA_BITMAP_BITS)
                                continue;

                        __set_bit(bit, bitmap->bitmap);
                        if (bitmap_full(bitmap->bitmap, IDA_BITMAP_BITS))
                                radix_tree_iter_tag_clear(root, &iter,
                                                                IDR_FREE);
                }
{% endhighlight %}

如果此时 bitmap 存在，那么调用 find_next_zero_bit() 在 bitmap->bitmap 中
找到一个 bit 为 0 的位置，然后将 new 的值增加 bit。如果此时 new 小于 0， 那么
直接返回 ENOSPC。如果此时 bit 的值等于 IDA_BITMAP_BITS，那么结束本次循环，继续
下一次循环；如果本次循环没有结束，那么调用 __set_bit() 函数将 bitmap->bitmap
对应的位置位，此时调用 bitmap_full() 函数检查 bitmap->bitmap 的位是置位，那么
函数调用 radix_tree_iter_tag_clear() 函数清除 iter 对应的节点的 tag。

{% highlight bash %}
                } else {
                        new += bit;
                        if (new < 0)
                                return -ENOSPC;
                        if (ebit < BITS_PER_LONG) {
                                bitmap = (void *)((1UL << ebit) |
                                                RADIX_TREE_EXCEPTIONAL_ENTRY);
                                radix_tree_iter_replace(root, &iter, slot,
                                                bitmap);
                                return new;
                        }
                        bitmap = ida_bitmap;
                        if (!bitmap)
                                return -EAGAIN;
                        __set_bit(bit, bitmap->bitmap);
                        radix_tree_iter_replace(root, &iter, slot, bitmap);
                }

                return new;
        }
{% endhighlight %}

如果此时 bitmap 不存在，那么增加 new 的值，如果增加 bit 之后小于 0，那么
返回 ENOSPC 错误码。如果 ebit 此时小于 BITS_PER_LONG，那么表示可以找到
有用的节点，于是将 1 左移 ebit 位，然后与 RADIX_TREE_EXCEPTIONAL_ENTRY
的值相或存储在 bitmap 中，然后调用 radix_tree_iter_replace() 函数替换
slot 的值为 bitmap。最后返回 new；如果此时 ebit 大于 BITS_PER_LONG，
那么将 bitmap 设置为 ida_bitmap, 如果此时 bitmap 不存在，那么返回 EAGAIN
错误码；如果 bitmap 存在，那么调用 __set_bit() 函数将 bitmap 执行为置位，
最后地啊哟红 radix_tree_iter_replace() 函数 slot 的值替换为 bitmap。
直到循环提供，函数返回 new 的值。

-----------------------------------

# <span id="ida_remove">ida_remove</span>

{% highlight bash %}
static void ida_remove(struct ida *ida, int id)
{
        unsigned long index = id / IDA_BITMAP_BITS;
        unsigned offset = id % IDA_BITMAP_BITS;
        struct ida_bitmap *bitmap;
        unsigned long *btmp;
        struct radix_tree_iter iter;
        void __rcu **slot;
{% endhighlight %}

ida_remove() 函数用于从 IDA 中移除 ID。代码比较长，分段解析。参数 IDA
对应着 IDA 的根。id 指向需要移除的 ID。函数定义了一些局部变量，其中 index
对应着 id 位于 IDA 的 chunk，offset 对应着 id 在 chunk 内的偏移。

{% highlight bash %}
        slot = radix_tree_iter_lookup(&ida->ida_rt, &iter, index);
        if (!slot)
                goto err;

        bitmap = rcu_dereference_raw(*slot);
        if (radix_tree_exception(bitmap)) {
                btmp = (unsigned long *)slot;
                offset += RADIX_TREE_EXCEPTIONAL_SHIFT;
                if (offset >= BITS_PER_LONG)
                        goto err;
        } else {
                btmp = bitmap->bitmap;
        }
        if (!test_bit(offset, btmp))
                goto err;
{% endhighlight %}

函数首先调用 radix_tree_iter_lookup() 查找 id 对应的节点，slot 变量存储着
id slot 接口、如果 slot 不存在，那么跳转到 err 处执行；如果 slot 存在，那么
从 slot 中读取 bitmap 的指针。此时调用 radix_tree_exception() 函数判断该
bitmap 是不是 exceptional 节点，如果 slot 的值存储了 IDA 的 bitmap，那么该
slot 存储的值就是一个 exceptional 节点。此时，使用 btmp 存储 slot 指针。
然后将 offset 增加 RADIX_TREE_EXCEPTIONAL_SHIFT, 以此偏移到真正 bitmap 开始
位置。如果此时 offset 已经超过 BITS_PER_LONG 的值，那么也跳转到 err 处
执行；如果 bitmap 不是 exceptional 节点，那么将 btmp 的值指向 bitmap->bitmap,
最后使用 test_bit() 函数检查 id 对应的 bit 是否置位。如果没有，那么直接
跳转到 err 处。

{% highlight bash %}
        __clear_bit(offset, btmp);
        radix_tree_iter_tag_set(&ida->ida_rt, &iter, IDR_FREE);
        if (radix_tree_exception(bitmap)) {
                if (rcu_dereference_raw(*slot) ==
                                        (void *)RADIX_TREE_EXCEPTIONAL_ENTRY)
                        radix_tree_iter_delete(&ida->ida_rt, &iter, slot);
        } else if (bitmap_empty(btmp, IDA_BITMAP_BITS)) {
                kfree(bitmap);
                radix_tree_iter_delete(&ida->ida_rt, &iter, slot);
        }
        return;
 err:
        WARN(1, "ida_free called for id=%d which is not allocated.\n", id);
{% endhighlight %}

函数调用 __clear_bit() 函数清除 id 对应的 bit 位，然后调用
radix_tree_iter_tag_set() 函数设置 ID 对应的节点 tag 为 IDR_FREE。对应之前的
做法，如果 bitmap 是 exceptional 节点，那么将 *slot 设置为
RADIX_TREE_EXCEPTIONAL_ENTRY, 并且调用 radix_tree_iter_delete() 函数
移除对应的入口；如果 bitmap 不是 exceptional 节点，那么直接调用 kfree 释放掉
bitmap，并调用 radix_tree_iter_delete() 函数释放掉指定的节点。最后返回，
至此，ID 已经从 IDA 中移除。

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
