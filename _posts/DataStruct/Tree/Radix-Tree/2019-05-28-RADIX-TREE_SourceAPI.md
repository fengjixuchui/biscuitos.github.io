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

entry_to_node() 函数的作用是将一个 radix tree 的入口转换成一个 radix_tree_node。
在 radix tree 中，如果不是最后一层树叶，那么该 radix_tree_node 就是一个内部节点，
内部节点会在指向节点的指针的低位或上 RADIX_TREE_INTERNAL_NODE 标志，以此判断一个
节点是内部节点还是一般叶子。entry_to_node() 函数就是通过该原理，将一个内部节点的
RADIX_TREE_INTERNAL_NODE 标志清除，以此获得指向这个节点的指针。

--------------------------------------------------

#### <span id="radix_tree_load_root">radix_tree_load_root</span>

{% highlight bash %}
static unsigned radix_tree_load_root(const struct radix_tree_root *root,
                struct radix_tree_node **nodep, unsigned long *maxindex)
{
        struct radix_tree_node *node = rcu_dereference_raw(root->rnode);

        *nodep = node;

        if (likely(radix_tree_is_internal_node(node))) {
                node = entry_to_node(node);
                *maxindex = node_maxindex(node);
                return node->shift + RADIX_TREE_MAP_SHIFT;
        }

        *maxindex = 0;
        return 0;
}
{% endhighlight %}

radix_tree_load_root() 函数用于获得 radix-tree 的根节点。radix-tree 使用
struct radix_tree_node 结构表示一个 radix-tree 节点，使用 struct radix_tree_root
结构表示 radix-tree 的根节点，并包含成员 rnode 指向根节点的 radix-tree 节点。

radix_tree_load_root() 函数首先获得根节点的 rnode 成员，然后判断 rnode 是不是内部
节点，如果此时 radix-tree 不是空树，那么 rnode 就是一个内部节点，接着调用
entry_to_node() 函数获得指向 rnode 对应的 radix-tree 节点的指针。然后调用
node_maxindex() 函数获得当前节点对应的最大索引值，并存储在 maxindex。最后
返回当前节点的 (node->shift + RADIX_TREE_MAP_SHIFT) 以此获得当前内部节点支持
最大的索引偏移；如果此时 radix-tree 是空树，那么将 maxindex 设置为 0，并返回 0，
告诉当前内部节点的 shift 值。

> [radix_tree_is_internal_node](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_is_internal_node/)
>
> [entry_to_node](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#entry_to_node)
>
> [node_maxindex](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#node_maxindex)

--------------------------------------------------

#### <span id="shift_maxindex">shift_maxindex</span>

{% highlight bash %}
#ifndef RADIX_TREE_MAP_SHIFT
#define RADIX_TREE_MAP_SHIFT    (CONFIG_BASE_SMALL ? 4 : 6)
#endif

#define RADIX_TREE_MAP_SIZE     (1UL << RADIX_TREE_MAP_SHIFT)

/*
 * The maximum index which can be stored in a radix tree
 */
static inline unsigned long shift_maxindex(unsigned int shift)
{
        return (RADIX_TREE_MAP_SIZE << shift) - 1;
}
{% endhighlight %}

shift_maxindex() 函数用于获得 radix-tree 每一层最大的索引值。例如 RADIX_TREE_MAP_SHIFT
为 6，那么每个内部节点能包含 2^6 个 slots， radix-tree 从根节点开始，第一层包含 1
个节点，节点就包含 64 个 slots，到了 radix-tree 的第二层，最多能包含 64 个内部节点，
每个节点包含 64 个 slots，那么第二层的最大索引值就是 (64 * 64 - 1), 其计算又为
(64 << 6 - 1)：同理第三层最大的索引值就是 (64 << 12 - 1), 因此 radix-tree 就
使用 shift_maxindex() 函数获得每一层最大的索引值，因此 shift 的值应该是 0,6,12,18...
 6 的整数倍。

--------------------------------------------------

#### <span id="node_maxindex">node_maxindex</span>

{% highlight bash %}
static inline unsigned long node_maxindex(const struct radix_tree_node *node)
{
        return shift_maxindex(node->shift);
}
{% endhighlight %}

node_maxindex() 函数用于获得当前节点支持的最大索引值。

> - [shift_maxindex](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#shift_maxindex)

--------------------------------------------------

#### <span id="root_tag_set">root_tag_set</span>

{% highlight bash %}
/* The top bits of gfp_mask are used to store the root tags */
#define ROOT_TAG_SHIFT  (__GFP_BITS_SHIFT)

static inline void root_tag_set(struct radix_tree_root *root, unsigned tag)
{
        root->gfp_mask |= (__force gfp_t)(1 << (tag + ROOT_TAG_SHIFT));
}
{% endhighlight %}

root_tag_set() 函数用于设置 radix-tree 的 tag。在 struct radix_tree_root 结构中，
存在成员 gfp_mask, 该成员被 radix-tree 分作三个区段，最低四字节用于存储 IDR 标志，
也就是标记该 radix_tree 是不是用于 IDR 机制。[23:4] 字段包含了用于从内核申请内存
的标志；[31:24] 字段用于设置 root 的 tag。root_tag_set() 函数用于设置 root tag
区域值。

--------------------------------------------------

#### <span id="root_tag_clear">root_tag_clear</span>

{% highlight bash %}
/* The top bits of gfp_mask are used to store the root tags */
#define ROOT_TAG_SHIFT  (__GFP_BITS_SHIFT)

static inline void root_tag_clear(struct radix_tree_root *root, unsigned tag)
{
        root->gfp_mask &= (__force gfp_t)~(1 << (tag + ROOT_TAG_SHIFT));
}
{% endhighlight %}

root_tag_clear() 函数用于清除 radix-tree 的 root tag 字段中特定的 tag。在
struct radix_tree_root 结构中，存在成员 gfp_mask, 该成员被 radix-tree 分作三
个区段，最低四字节用于存储 IDR 标志，也就是标记该 radix_tree 是不是用于 IDR 机
制。[23:4] 字段包含了用于从内核申请内存的标志；[31:24] 字段用于设置 root 的 tag。
因此 root_tag_clear() 用于清除 [31:24] 字段中特定的 tag；

--------------------------------------------------

#### <span id="root_tag_clear_all">root_tag_clear_all</span>

{% highlight bash %}
/* The top bits of gfp_mask are used to store the root tags */
#define ROOT_TAG_SHIFT  (__GFP_BITS_SHIFT)

static inline void root_tag_clear_all(struct radix_tree_root *root)
{
        root->gfp_mask &= (1 << ROOT_TAG_SHIFT) - 1;
}
{% endhighlight %}

root_tag_clear_all() 用于清除 radix_tree 的 root tag 字段。在
struct radix_tree_root 结构中，存在成员 gfp_mask, 该成员被 radix-tree 分作三
个区段，最低四字节用于存储 IDR 标志，也就是标记该 radix_tree 是不是用于 IDR 机
制。[23:4] 字段包含了用于从内核申请内存的标志；[31:24] 字段用于设置 root 的 tag。
因此 root_tag_clear() 用于清除 [31:24] 字段。

--------------------------------------------------

#### <span id="root_tag_get">root_tag_get</span>

{% highlight bash %}
/* The top bits of gfp_mask are used to store the root tags */
#define ROOT_TAG_SHIFT  (__GFP_BITS_SHIFT)

static inline int root_tag_get(const struct radix_tree_root *root, unsigned tag)
{
        return (__force int)root->gfp_mask & (1 << (tag + ROOT_TAG_SHIFT));
}
{% endhighlight %}

root_tag_get() 函数用于获得 radix-tree 树 root tag 字段中特定的 tag 值。在
struct radix_tree_root 结构中，存在成员 gfp_mask, 该成员被 radix-tree 分作三
个区段，最低四字节用于存储 IDR 标志，也就是标记该 radix_tree 是不是用于 IDR 机
制。[23:4] 字段包含了用于从内核申请内存的标志；[31:24] 字段用于设置 root 的 tag。
因此 root_tag_get() 用于获得 [31:24] 字段中特定的 tag。

--------------------------------------------------

#### <span id="is_idr">is_idr</span>

{% highlight bash %}
/* The IDR tag is stored in the low bits of the GFP flags */
#define ROOT_IS_IDR     ((__force gfp_t)4)

static inline bool is_idr(const struct radix_tree_root *root)
{
        return !!(root->gfp_mask & ROOT_IS_IDR);
}
{% endhighlight %}

is_idr() 函数用于判断当前 radix-tree 是否用于 IDR 机制。在 struct radix_tree_root 结构中，
存在成员 gfp_mask, 该成员被 radix-tree 分作三个区段，最低四字节用于存储 IDR 标志，
也就是标记该 radix_tree 是不是用于 IDR 机制。因此通过判断 gfp_mask 的最低 4 bit
的值判断当前 radix-tree 是不是用于 IDR 机制。

--------------------------------------------------

#### <span id="radix_tree_node_alloc">radix_tree_node_alloc</span>

{% highlight bash %}
/*
 * This assumes that the caller has performed appropriate preallocation, and
 * that the caller has pinned this thread of control to the current CPU.
 */
static struct radix_tree_node *
radix_tree_node_alloc(gfp_t gfp_mask, struct radix_tree_node *parent,
                        struct radix_tree_root *root,
                        unsigned int shift, unsigned int offset,
                        unsigned int count, unsigned int exceptional)
{
        struct radix_tree_node *ret = NULL;

        /*
         * Preload code isn't irq safe and it doesn't make sense to use
         * preloading during an interrupt anyway as all the allocations have
         * to be atomic. So just do normal allocation when in interrupt.
         */
        if (!gfpflags_allow_blocking(gfp_mask) && !in_interrupt()) {
                struct radix_tree_preload *rtp;

                /*
                 * Even if the caller has preloaded, try to allocate from the
                 * cache first for the new node to get accounted to the memory
                 * cgroup.
                 */
                ret = kmem_cache_alloc(radix_tree_node_cachep,
                                       gfp_mask | __GFP_NOWARN);
                if (ret)
                        goto out;

                /*
                 * Provided the caller has preloaded here, we will always
                 * succeed in getting a node here (and never reach
                 * kmem_cache_alloc)
                 */
                rtp = this_cpu_ptr(&radix_tree_preloads);
                if (rtp->nr) {
                        ret = rtp->nodes;
                        rtp->nodes = ret->parent;
                        rtp->nr--;
                }
                /*
                 * Update the allocation stack trace as this is more useful
                 * for debugging.
                 */
                kmemleak_update_trace(ret);
                goto out;
        }
        ret = kmem_cache_alloc(radix_tree_node_cachep, gfp_mask);
out:
        BUG_ON(radix_tree_is_internal_node(ret));
        if (ret) {
                ret->shift = shift;
                ret->offset = offset;
                ret->count = count;
                ret->exceptional = exceptional;
                ret->parent = parent;
                ret->root = root;
        }
        return ret;
}
{% endhighlight %}

radix_tree_node_alloc() 函数用于从系统分配内存给新的 radix_tree_node, 并且
初始化这个新的节点。由于 struct radix_tree_node 结构经常分配和释放，内核就预先
分配了 radix_tree_node 的缓存，直接调用 kmem_cache_alloc() 函数就可以获得
需要的 radix_tre_node 对应大小的内存。如果分配成功，那么就初始化这个节点。

--------------------------------------------------

#### <span id="tag_set">tag_set</span>

{% highlight bash %}
static inline void tag_set(struct radix_tree_node *node, unsigned int tag,
                int offset)
{
        __set_bit(offset, node->tags[tag]);
}
{% endhighlight %}

tag_set() 函数用于设置 struct radix_tree_node 结构 tags 成员 tag 域的指定 bit。
Linux radix 树的独特特征是标签扩展。radix_tree_node tags 有 2 个标签，分别表示打开
和关闭。这些标签是结点结构的成员 tags，该标签用一个位图实现，在加锁的情况下设置和清
除。每个标签基本上是一个在 radix 树顶层上的位图序号。标签与群查询一起用来在给定的范
围内查找给定标签集的页。标签在每结点位图中维护，在每个给定的级别上，可以判定下一个级
别是否至少有一个标签集。在结点结构 radix_tree_node 中，tags 域是一个二维数组，每个
slot 用 2 位标识，这是一个典型的用空间换时间的应用。tags 域用于记录该结点下面的子结
点有没有相应的标志位。目前 RADIX_TREE_MAX_TAGS 为 2，表示只记录两个标志，其中
tags[0] 为 PAGE_CACHE_DIRTY，tags[1] 为 PAGE_CACHE_WRITEBACK。如果当前节点的
tags[0] 值为 1，那么它的子树节点就存在 PAGE_CACHE_DIRTY 节点，否则这个子树分枝就
不存在着这样的节点，就不必再查找这个子树了。比如在查找 PG_dirty 的页面时，就不需要
遍历整个树，而可以跳过那些 tags[0] 为 0 值的子树，这样就提高了查找效率。“加标签查询”
是返回有指定标签集 radix 树条目的查询，可以查询设置了标签的条目。加标签查询可以并行
执行，但它们需要通过设置或清除标签从操作上互斥。

因此 tag_set() 函数就是设置 radix_tree_node 结构中 tags[tag] 的 offset bit。

--------------------------------------------------

#### <span id="tag_clear">tag_clear</span>

{% highlight bash %}
static inline void tag_clear(struct radix_tree_node *node, unsigned int tag,
                int offset)
{
        __clear_bit(offset, node->tags[tag]);
}
{% endhighlight %}

tag_clear() 函数用于清除 struct radix_tree_node 的 tags 中特定的位。
Linux radix 树的独特特征是标签扩展。radix_tree_node tags 有 2 个标签，分别表示打开
和关闭。这些标签是结点结构的成员 tags，该标签用一个位图实现，在加锁的情况下设置和清
除。每个标签基本上是一个在 radix 树顶层上的位图序号。标签与群查询一起用来在给定的范
围内查找给定标签集的页。标签在每结点位图中维护，在每个给定的级别上，可以判定下一个级
别是否至少有一个标签集。在结点结构 radix_tree_node 中，tags 域是一个二维数组，每个
slot 用 2 位标识，这是一个典型的用空间换时间的应用。tags 域用于记录该结点下面的子结
点有没有相应的标志位。目前 RADIX_TREE_MAX_TAGS 为 2，表示只记录两个标志，其中
tags[0] 为 PAGE_CACHE_DIRTY，tags[1] 为 PAGE_CACHE_WRITEBACK。如果当前节点的
tags[0] 值为 1，那么它的子树节点就存在 PAGE_CACHE_DIRTY 节点，否则这个子树分枝就
不存在着这样的节点，就不必再查找这个子树了。比如在查找 PG_dirty 的页面时，就不需要
遍历整个树，而可以跳过那些 tags[0] 为 0 值的子树，这样就提高了查找效率。“加标签查询”
是返回有指定标签集 radix 树条目的查询，可以查询设置了标签的条目。加标签查询可以并行
执行，但它们需要通过设置或清除标签从操作上互斥。

因此 tag_clear() 函数就是清除 radix_tree_node 结构中 tags[tag] 的 offset bit。

--------------------------------------------------

#### <span id="tag_get">tag_get</span>

{% highlight bash %}
static inline int tag_get(const struct radix_tree_node *node, unsigned int tag,
                int offset)
{
        return test_bit(offset, node->tags[tag]);
}
{% endhighlight %}

tag_get() 函数用于从 struct radix_tree_node 的 tags 成员中读取特定的位信息。
Linux radix 树的独特特征是标签扩展。radix_tree_node tags 有 2 个标签，分别表示打开
和关闭。这些标签是结点结构的成员 tags，该标签用一个位图实现，在加锁的情况下设置和清
除。每个标签基本上是一个在 radix 树顶层上的位图序号。标签与群查询一起用来在给定的范
围内查找给定标签集的页。标签在每结点位图中维护，在每个给定的级别上，可以判定下一个级
别是否至少有一个标签集。在结点结构 radix_tree_node 中，tags 域是一个二维数组，每个
slot 用 2 位标识，这是一个典型的用空间换时间的应用。tags 域用于记录该结点下面的子结
点有没有相应的标志位。目前 RADIX_TREE_MAX_TAGS 为 2，表示只记录两个标志，其中
tags[0] 为 PAGE_CACHE_DIRTY，tags[1] 为 PAGE_CACHE_WRITEBACK。如果当前节点的
tags[0] 值为 1，那么它的子树节点就存在 PAGE_CACHE_DIRTY 节点，否则这个子树分枝就
不存在着这样的节点，就不必再查找这个子树了。比如在查找 PG_dirty 的页面时，就不需要
遍历整个树，而可以跳过那些 tags[0] 为 0 值的子树，这样就提高了查找效率。“加标签查询”
是返回有指定标签集 radix 树条目的查询，可以查询设置了标签的条目。加标签查询可以并行
执行，但它们需要通过设置或清除标签从操作上互斥。

因此 tag_get() 函数就是获取 radix_tree_node 结构中 tags[tag] 的 offset bit。

--------------------------------------------------

#### <span id="all_tag_set">all_tag_set</span>

{% highlight bash %}
static inline void all_tag_set(struct radix_tree_node *node, unsigned int tag)
{
        bitmap_fill(node->tags[tag], RADIX_TREE_MAP_SIZE);
}
{% endhighlight %}

all_tag_set() 用于将 struct radix_tree_node 的 tags 中特定 tag 对应的位图
全部位置位。Linux radix 树的独特特征是标签扩展。radix_tree_node tags 有 2 个标签，分别表示打开
和关闭。这些标签是结点结构的成员 tags，该标签用一个位图实现，在加锁的情况下设置和清
除。每个标签基本上是一个在 radix 树顶层上的位图序号。标签与群查询一起用来在给定的范
围内查找给定标签集的页。标签在每结点位图中维护，在每个给定的级别上，可以判定下一个级
别是否至少有一个标签集。在结点结构 radix_tree_node 中，tags 域是一个二维数组，每个
slot 用 2 位标识，这是一个典型的用空间换时间的应用。tags 域用于记录该结点下面的子结
点有没有相应的标志位。目前 RADIX_TREE_MAX_TAGS 为 2，表示只记录两个标志，其中
tags[0] 为 PAGE_CACHE_DIRTY，tags[1] 为 PAGE_CACHE_WRITEBACK。如果当前节点的
tags[0] 值为 1，那么它的子树节点就存在 PAGE_CACHE_DIRTY 节点，否则这个子树分枝就
不存在着这样的节点，就不必再查找这个子树了。比如在查找 PG_dirty 的页面时，就不需要
遍历整个树，而可以跳过那些 tags[0] 为 0 值的子树，这样就提高了查找效率。“加标签查询”
是返回有指定标签集 radix 树条目的查询，可以查询设置了标签的条目。加标签查询可以并行
执行，但它们需要通过设置或清除标签从操作上互斥。

--------------------------------------------------

#### <span id="radix_tree_extend">radix_tree_extend</span>

{% highlight bash %}
/*
 *      Extend a radix tree so it can store key @index.
 */
static int radix_tree_extend(struct radix_tree_root *root, gfp_t gfp,
                                unsigned long index, unsigned int shift)
{% endhighlight %}

radix_tree_extend() 函数用于当 radix-tree 需要存储一个比现有最大 index 还大
的索引时，需要使用该函数扩展 radix-tree 树。参数 root 指向 radix-tree 的根节点，
index 参数代表需要增加的索引；shift 代表偏移值。由于代码比较长，分段解析：

{% highlight bash %}
        void *entry;
        unsigned int maxshift;
        int tag;

        /* Figure out what the shift should be.  */
        maxshift = shift;
        while (index > shift_maxindex(maxshift))
                maxshift += RADIX_TREE_MAP_SHIFT;

        entry = rcu_dereference_raw(root->rnode);
        if (!entry && (!is_idr(root) || root_tag_get(root, IDR_FREE)))
                goto out;
{% endhighlight %}

函数首先将参数 shift 存储在局部变量 maxshift 里。然后调用 shift_maxindex() 函数
获得 maxshift 对应的最大索引值，然后调用 while 循环找到一个可以存下 index 的索引
范围。shift_maxindex() 函数每调用一次，maxshift 增大 RADIX_TREE_MAP_SHIFT,
这里的作用就是一次查询存下这个 index 需要几层节点。接着将根节点对应的 rnode
赋值给 entry，此时如果 entry 为空，即 radix_tree 为空树，并且该 radix_tree
不是用于 IDR，那么直接跳转到 out 处执行，或者 root 包含了 IDR_FREE 标志，也
要跳转到 out 处执行。如果不满足上面的条件，那么就指向下面代码：

{% highlight bash %}
                struct radix_tree_node *node = radix_tree_node_alloc(gfp, NULL,
                                                        root, shift, 0, 1, 0);
                if (!node)
                        return -ENOMEM;

                if (is_idr(root)) {
                        all_tag_set(node, IDR_FREE);
                        if (!root_tag_get(root, IDR_FREE)) {
                                tag_clear(node, IDR_FREE, 0);
                                root_tag_set(root, IDR_FREE);
                        }
                } else {
                        /* Propagate the aggregated tag info to the new child */
                        for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++) {
                                if (root_tag_get(root, tag))
                                        tag_set(node, tag, 0);
                        }
                }
{% endhighlight %}

函数首先调用 radix_tree_node_alloc() 从缓存中分配一个新的 struct radix_tree_node,
如果 node 为空，则分配失败。如果 radix-tree 是用于 IDR 机制，那么就调用
all_tag_set() 函数将该 node 的 tags[IDR_FREE] 全部置位，并判断根节点的 gfp_mask
成员是否不包含 IDR_FREE，如果不包含，那么就调用 tag_clear, 那么就将
node->tag[IDR_FREE] 对应根节点的 bit 清零，然后调用 root_tag_set() 函数
向根节点添加 IDR_FREE 标志；反之如果 radix-tree 不是用于 IDR 机制，那么就使用
for 循环，调用 root_tag_get() 函数判断 root 的 gfp_mask 是否包含了特定的 tag，
如果包含就将对于的 tag 的 0bit 置位.

{% highlight bash %}
                BUG_ON(shift > BITS_PER_LONG);
                if (radix_tree_is_internal_node(entry)) {
                        entry_to_node(entry)->parent = node;
                } else if (radix_tree_exceptional_entry(entry)) {
                        /* Moving an exceptional root->rnode to a node */
                        node->exceptional = 1;
                }
                /*
                 * entry was already in the radix tree, so we do not need
                 * rcu_assign_pointer here
                 */
                node->slots[0] = (void __rcu *)entry;
                entry = node_to_entry(node);
                rcu_assign_pointer(root->rnode, entry);
                shift += RADIX_TREE_MAP_SHIFT;
        } while (shift <= maxshift);
{% endhighlight %}

在每一循环中，如果更节点是内部节点，首先判断 shift 是否已经超过了 BITS_PER_LONG,
如果超过则发出警告。函数判断 entry 是内部节点还是 exceptional 节点，如果是内部节
点，那么将 entry 的 parent 指向新申请的节点；如果是 exceptional 节点，那么就将
节点的 exceptional 设置为 1. 判断之后要做的就是将 radix tree 增加一层，函数
将原始的 entry 设置为新节点的 slot[0] 指向的值，然后将 entry 指向当前节点对应的
入口。替换根节点的 rnode 为新添加的 node，并且 shift 增加 RADIX_TREE_MAP_SHIFT。
如果 shift 的值还小于 maxshift，那么就需要在增加 radix-tree 树的高度。

{% highlight bash %}
out:
        return maxshift + RADIX_TREE_MAP_SHIFT;
}
 {% endhighlight %}

循环完毕之后，将 maxshift + RADIX_TREE_MAP_SHIFT 的值返回。

> - [shift_maxindex](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#shift_maxindex)
>
> - [is_idr](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#is_idr)
>
> - [root_tag_get](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#root_tag_get)
>
> - [all_tag_set](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#all_tag_set)
>
> - [root_tag_set](https://biscuitos.github.io/blog/RADIX-TREE_SourceAPI/#root_tag_set)
>
> - [radix_tree_is_internal_node](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_is_internal_node/)
>
> - [radix_tree_exceptional_entry](https://biscuitos.github.io/blog/RADIX-TREE_radix_tree_exceptional_entry/)
>

--------------------------------------------------

#### <span id=""></span>

{% highlight bash %}

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
