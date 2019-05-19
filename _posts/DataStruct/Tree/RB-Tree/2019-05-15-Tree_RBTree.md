---
layout: post
title:  "红黑树 Red Black Tree"
date:   2019-05-14 05:30:30 +0800
categories: [HW]
excerpt: TREE 红黑树 Red Black Tree.
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000T.jpg)

> [Github: 红黑树 Red Black Tree](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/binary-tree/Class/Full_BinaryTree)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>


# 目录

> - [红黑树原理](#原理)
>
> - [红黑树最小实践](#实践)
>
> - [红黑树的操作](#操作)
>
> - [附录](#附录)

-----------------------------------
# <span id="原理"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000R.jpg)

> - [红黑树原理](#红黑树原理)
>
> - [红黑树性质](#红黑树性质)
>
> - [红黑树的术语](#红黑树的术语)

------------------------------------------------------

#### <span id="红黑树原理">红黑树原理</span>

红黑树是自平衡的二叉搜索树，是计算机科学中的一种数据结构。平衡是指所有叶子的深度基
本相同(完全相等的情况并不多见, 所以只能趋向于相等)。二叉搜索树是指: 节点最多有两个
儿子，且左子树中所有节点都小于右子树。树中节点有改动时, 通过调整节点顺序（旋转）,
重新给节点染色, 使节点满足某种特殊的性质来保持平衡。旋转和染色过程肯定经过特殊设计
可以高效的完成。红黑树不是完全平衡的二叉树，但能保证搜索操作在 O(log n) 的时间复杂
度内完成（n 是树中节点总数）。

红黑树的插入、删除以及旋转、染色操作都是 O(log n) 的时间复杂度。每个节点只需要用一
位(bit)保存颜色（仅为红、黑两种）属性，除此以外，红黑树不需要保存其他信息，所以红黑
树与普通二叉搜索树（BST）的内存开销基本一样，不会占用太多内存。

##### 红黑树与 2-3 树的关系

红黑树的主要是想对 2-3 查找树进行编码，尤其是对 2-3 查找树中的 3-nodes 节点添加
额外的信息。红黑树中将节点之间的链接分为两种不同类型，红色链接，他用来链接两个
2-nodes 节点来表示一个 3-nodes 节点。黑色链接用来链接普通的 2-3 节点。特别的，
使用红色链接的两个 2-nodes 来表示一个 3-nodes 节点，并且向左倾斜，即一个 2-node
是另一个 2-node 的左子节点。这种做法的好处是查找的时候不用做任何修改，和普通的二叉
查找树相同。

根据以上红黑树与 2-3 树的描述，红黑树定义如下：

{% highlight ruby %}
红黑树是一种具有红色和黑色链接的平衡查找树，同时满足：

1) 红色节点向左倾斜

2) 一个节点不可能有两个红色链接

3) 整个树完全黑色平衡，即从根节点到所以叶子结点的路径上，黑色链接的个数都相同。
{% endhighlight %}

更多 2-3 树与红黑树的关系，请查看文档：

> [2-3 树/2-3-4 树 与红黑树的关系分析](https://biscuitos.github.io/blog/Tree_2-3-tree)

------------------------------------------------------

#### <span id="红黑树性质">红黑树性质</span>

除了二叉树的基本要求外，红黑树必须满足以下几点性质。

{% highlight ruby %}
1) 节点必须是红色或者黑色。

2) 根节点必须是黑色。

3) 叶节点 (NIL) 是黑色的。（NIL 节点无数据，是空节点）

4) 红色节点必须有两个黑色儿子节点。

5) 从任一节点出发到其每个叶子节点的路径，黑色节点的数量是相等的。
{% endhighlight %}

这些约束使红黑树具有这样一个关键属性：从根节点到最远的叶子节点的路径长与到最近的叶子
节点的路径长度相差不会超过 2。 因为红黑树是近似平衡的。另外，插入、删除和查找操作与树
的高度成正比，所以红黑树的最坏情况，效率仍然很高。（不像普通的二叉搜索树那么慢）

解释一下为什么有这样好的效果。注意性质 4 和 性质 5。假设一个红黑树 T，其到叶节点的
最短路径肯定全部是黑色节点（共 B 个），最长路径肯定有相同个黑色节点（性质 5：黑色节
点的数量是相等)，另外会多几个红色节点。性质 4（红色节点必须有两个黑色儿子节点）能保
证不会再现两个连续的红色节点。所以最长的路径长度应该是 2B 个节点，其中 B 个红色，B
个黑色。最短的路径中全部是黑色节点，最长的路径中既有黑色又有红色节点。因为这两个路径
中黑色节点个数是一样的，而且不会出现两个连续的红色节点，所以最长的路径可能会出现红黑
相间的节点。也就是说，树中任意两条路径中的节点数相差不会超过一倍。

#### <span id="红黑树的术语">红黑树的术语</span>

> 黑高 black-height

从某个结点 x 出发（不含该结点）到达一个叶结点的任意一条简单路径上的黑色结点个数称
为该结点的黑高（black-height），记为 bh(x)。红黑树的黑高为其根结点的黑高。

> NIL

红黑树中每个结点包含 5 个属性：color、key、left、right 和 p。如果一个结点没有子结
点或父结点，则该结点相应属性值为 NIL。这些 NIL 被视为指向二叉搜索树的叶结点（外部结
点）的指针，而把带关键字的结点视为树的内部结点。

> 哨兵 T.nil

为了便于处理红黑树代码中的边界条件，使用一个哨兵 T.nil 来代表所有的 NIL：所有的叶
结点和根结点的父结点。

> 旋转

红黑树的旋转是一种能保持二叉搜索树性质的搜索树局部操作。有左旋和右旋两种旋转，通过改
变树中某些结点的颜色以及指针结构来保持对红黑树进行插入和删除操作后的红黑性质.

--------------------------------------------------

# <span id="实践">实践</span>

> - [实践源码](#实践源码)
>
> - [源码编译](#源码编译)
>
> - [源码运行](#源码运行)
>
> - [运行分析](#运行分析)

#### <span id="实践源码">实践源码</span>

> [实践源码 RBTree on GitHub](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Basic)

开发者也可以使用如下命令获得：

{% highlight ruby %}
wget https://raw.githubusercontent.com/BiscuitOS/HardStack/master/Algorithem/tree/binary-tree/Class/Full_BinaryTree/binary.c
{% endhighlight %}

实践源码具体内容如下：

{% highlight c %}

{% endhighlight %}

--------------------------------------

#### <span id="源码编译">源码编译</span>

将实践源码保存为 binary.c，然后使用如下命令进行编译：

{% highlight ruby %}
make clean
make
{% endhighlight %}

--------------------------------------

#### <span id="源码运行">源码运行</span>

实践源码的运行很简单，可以使用如下命令，并且运行结果如下：

{% highlight ruby %}

{% endhighlight %}

--------------------------------------

#### <span id="运行分析">运行分析</span>

从运行的结果可以看出，在使用先序遍历二叉树的时候，二叉树遍历结果和预期一致

-----------------------------------
# <span id="操作"></span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000U.jpg)

# 红黑树的操作

> - [红黑树旋转](#红黑树旋转)
>
> - [红黑树颜色翻转](#红黑树颜色翻转)
>
> - [红黑树颜色翻转](#红黑树颜色翻转)
>
> - [红黑树的插入操作](#红黑树的插入操作)


------------------------------------

### <span id="红黑树旋转">红黑树旋转</span>

红黑树的旋转是一种能保持二叉搜索树性质的搜索树局部操作。有左旋和右旋两种旋转，通过改
变树中某些结点的颜色以及指针结构来保持对红黑树进行插入和删除操作后的红黑性质.

> - [红黑树旋转与 2-3 树的关系](#红黑树旋转与 2-3 树的关系)
>
> - [红黑树左旋](#红黑树左旋)
>
> - [红黑树右旋](#红黑树右旋)

###### <span id="红黑树旋转与 2-3 树的关系">红黑树旋转与 2-3 树的关系</span>

旋转又分为左旋和右旋。通常左旋操作用于将一个向右倾斜的红色链接旋转为向左链接。
右旋操作用于将一个向左倾斜的红色链接旋转为向右链接。对比操作前后，可以看出，该
操作实际上是将红线链接的两个节点中的一个较大的节点移动到根节点上。

------------------------------------

###### <span id=" 红黑树左旋">红黑树左旋</span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT100000.gif)

对结点 E 做左旋操作时，其右孩子为 S 而不是 T.nil，那么以 E 到 S 的链为
"支轴" 进行。使 S 成为该子树新的根结点，E 成为 S 的左孩子，E 的左孩子成为 S 的
右孩子.

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000075.png)

如上图，当插入 6 之后，红黑树 5 节点需要进行左旋达到平衡，那么以 4 到 5 的链为
"支轴" 进行。使用 5 节点成为 6 的新的根节点，4 称为 5 的左孩子，6 称为 5 的右
孩子。如下图：

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000076.png)

> [红黑树左旋实践](https://biscuitos.github.io/blog/Tree_RBTree_LeftRotate/)

------------------------------------

###### </span id="红黑树右旋">红黑树右旋</span>

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT100001.gif)

对结点 S 做右旋操作时，假设其左孩子为 E 而不是T.nil, 以 S 到 E 的链为 “支轴” 进
行。使 E 成为该子树新的根结点， S 成为 E 的右孩子，E 的右孩子成为 S 的左孩子。

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000077.png)

如上图，当插入 4 之后，红黑树 5 节点需要进行右旋达到平衡，那么以 5 到 6 的链为
"支轴" 进行。使用 5 节点成为新的根节点， 6 成为 5 的右孩子，4 称为 5 的左
孩子。如下图：

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000078.png)

> [红黑树右旋实践](https://biscuitos.github.io/blog/Tree_RBTree_RightRotate/)

------------------------------------

### <span id="红黑树颜色翻转">红黑树颜色翻转</span>

当出现一个临时的 4-node 的时候，即一个节点的两个子节点均为红色，此时需要颜色翻转。
颜色翻转的触发场景：

{% highlight ruby %}
1) 向 2-3 树中的 3 节点添加元素，新添加的元素要添加到 3 节点的最右侧.

2) 对应到红黑树中的情形是：新添加的红节点在黑节点右侧，黑节点的左侧还有一个红节点，
   红黑树中的这种形态对应到 2-3 树中就是一个临时的 4 节点；

3) 2-3 树中临时的 4 节点要拆成 3个2 节点，这种形态对应到红黑树中就是 3 个黑节点；

4) 2-3 树在拆成 3 个 2 节点后，要向上融合，3 个 2 节点中间的那个节点是要融合的，
   所以它是红色的；

5) 最后从结果看，从一添加的 “红-黑-红”，到拆完后形成的 “黑-红-黑”，正好形成颜色
   的翻转，即所谓的颜色翻转；
{% endhighlight %}

红黑树中涉及的一些结论：

{% highlight ruby %}
1) 红黑树中，如果一个黑节点左侧没有红色节点的话，它本身就代表 2-3 树中一个单独的 2 节点；

2) 3 个 2 节点对应到红黑树中表示的是这 3 个节点都是黑节点；

3) 2-3 树中，临时的 4 节点在拆完后还要向上融合，融合意味着，2-3 树中临时 4 节点在拆
   完后向上融合的根，在红黑树中是红色的；
{% endhighlight %}

更多颜色翻转实践请看：

> [父节点是祖父的右孩子，引起颜色翻转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_ColorFlips_right/)
>
> [父节点是祖父的左孩子，引起颜色翻转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_ColorFlips_left/)

------------------------------------

### <span id="红黑树的插入操作">红黑树的插入操作</span>

插入操作与二叉搜索树一样，新节点肯定会作为树中的叶子节点的儿子加入（详见二叉搜索树
相关说明），不过为了恢复红黑树性质，还需要做些染色、旋转等调整操作。另外需要注意的
是，红黑树叶子节点是黑色的 NIL节点，所以一般用带有两个黑色 NIL 儿子的新节点直接替换
原先的 NIL 叶子节点，为了方便后续的调整操作，新节点都是默认的红色。

注：插入节点后的调整操作，主要目的是保证树的总体高度不发生改变（使插入点为红色进入
树中); 如果一定要改变树的高度（插入点无法调整为红色），那么所有操作的目的是使树的整
体高度增长 1 个单位，而不是仅某一子树增长 1 个高度。具体如何进行调整要看新节点周围
的节点颜色进行处理。下面是需要注意的几种情况：

{% highlight ruby %}
性质3 (所有的叶子节点都是黑色）不会被破坏，因为叶子节点全部是黑色的 NIL。

性质4 (红色节点的两个儿子必须是黑色）仅在添加一个红色节点时，将黑色节点染成红色时，或
者进行旋转操作时发生改变。

性质5 (从任一节点出发到叶子节点的路径中黑色节点的数量相等）仅在添加黑色节点时，将红色
节点染成黑色时，或者进行旋转操作时发生改变。
{% endhighlight %}

> - [插入根节点](#插入根节点)
>
> - [插入一个红节点到黑根节点](#插入一个红节点到黑根节点)
>
> - [插入一个红节点引起红黑树右旋转](#插入一个红节点引起红黑树右旋转)
>
> - [插入一个红节点引起红黑树左旋转](#插入一个红节点引起红黑树左旋转)
>
> - [插入一个红节点引起红黑树节点颜色翻转](#插入一个红节点引起红黑树节点颜色翻转)

红黑树插入操作相关的实践代码：

> [GitHub 红黑树插入操作](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/tree/rb-tree/Insert)

--------------------------------

##### <span id="插入根节点">插入根节点</span>

当红黑树中没有任何节点的时候，插入的节点作为 root 节点。详细插入实践原理请看如下文章

> [红黑树插入操作之：插入根节点](https://biscuitos.github.io/blog/Tree_RBTree_InsertRoot/)

--------------------------------

##### <span id="插入一个红节点到黑根节点">插入一个红节点到黑根节点</span>

当红黑树中只有根节点，此时根节点称为黑根节点，此时向黑根节点中添加一个红节点 (注意！新
插入到节点都是红节点)，详细插入实践原理请看如下文章：

> [红黑树插入操作之：插入一个红节点到黑根节点](https://biscuitos.github.io/blog/Tree_RBTree_InsertRoot_RED/)

--------------------------------

##### <span id="插入一个红节点引起红黑树右旋转">插入一个红节点引起红黑树右旋转</span>

当插入节点的父节点是祖父的左孩子，当添加一个红孩子作为父节点的左孩子，可能会引起红黑树向左偏移，这时
需要进行红黑树的右旋转，以此达到红黑树的平衡 (注意！新插入到节点都是红节点)。此时也会引起
右旋，详细插入实践原理请看如下文章：

> [父节点是祖父的左孩子，引起的右旋转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_RightRotate/)

当插入节点的父节点是祖父的右孩子，当添加一个红孩子作为父节点的左孩子，可能引起红黑树部分向左偏移，
这时需要进行红黑树的右旋转，以此达到红黑树的平衡，详细插入实践原理请看如下文章：

> [父节点是祖父的右孩子，引起的右旋转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_RightRotate/)

--------------------------------

##### <span id="插入一个红节点引起红黑树左旋转">插入一个红节点引起红黑树左旋转</span>

当插入节点的父节点是祖父的右孩子，当添加一个红孩子作为父节点的右孩子，可能会引起红黑树向右偏移，这时
需要进行红黑树的左旋转，以此达到红黑树的平衡 (注意！新插入到节点都是红节点)。此时也会引起
左旋，详细插入实践原理请看如下文章：

> [父节点是祖父的右孩子，引起的左旋转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_LeftRotate_right/)

当插入节点的父节点是祖父的左孩子，当添加一个红孩子作为父节点的右孩子，可能引起红黑树部分向右偏移，
这时需要进行红黑树的左旋转，以此达到红黑树的平衡，详细插入实践原理请看如下文章：

> [父节点是祖父的左孩子，引起的左旋转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_LeftRotate_left/)

##### <span id="插入一个红节点引起红黑树节点颜色翻转">插入一个红节点引起红黑树节点颜色翻转</span>

当插入节点的父节点是祖父的右孩子，当添加一个红孩子作为父节点的右孩子，其叔叔是红节点，这时
需要进行红黑树节点颜色翻转，以此达到红黑树的平衡 (注意！新插入到节点都是红节点)。此时也会引起
颜色翻转，详细插入实践原理请看如下文章：

> [父节点是祖父的右孩子，引起颜色翻转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_ColorFlips_right/)

当插入节点的父节点是祖父的左孩子，当添加一个红孩子作为父节点的左孩子，其叔叔是红节点，这是需要进行
颜色翻转才能再次达到红黑树平衡，详细插入实践原理请看如下文章：

> [父节点是祖父的左孩子，引起颜色翻转](https://biscuitos.github.io/blog/Tree_RBTree_Insert_ColorFlips_left/)


-----------------------------------------------

# <span id="附录">附录</span>

> [Data Structure Visualizations](https://www.cs.usfca.edu/~galles/visualization/Algorithms.html)
>
> [浅谈算法和数据结构: 平衡查找树之红黑树](https://www.cnblogs.com/yangecnu/p/Introduce-Red-Black-Tree.html)
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
