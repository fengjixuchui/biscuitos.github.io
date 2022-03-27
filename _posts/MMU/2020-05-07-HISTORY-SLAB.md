---
layout: post
title:  "SLAB Allocator"
date:   2020-05-07 09:39:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [SLAB 分配器原理](#A)
>
> - [SLAB 分配器使用](#B)
>
> - [SLAB 分配器实践](#C)
>
> - [SLAB 源码分析](#D)
>
> - [SLAB 分配器调试](#E)
>
> - [SLAB 分配器进阶研究](#F)
>
> - [SLAB 时间轴](#G)
>
> - [SLAB 历史补丁](#H)
>
> - [SLAB API](#K)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### SLAB 分配器原理

![](/assets/PDB/HK/HK000226.png)

在 Linux 内核虚拟地址空间，存在一段将虚拟地址直接与物理地址映射的区域，在这段
区域内物理地址是整块连续的，虚拟地址也是整块连续的，Linux 称这块区域为线性
映射区。Linux 在初始化阶段为这块区域创建相应的页表，由于物理地址和虚拟地址的
连续性，只要知道其中某段虚拟地址与物理地址的映射关系，系统就可以通过简单的线性
计算, 然后就可以推断出该区域内其他虚拟地址与物理地址之间的映射关系. 例如在上图
中 PAGE_OFFSET 到 highmem 之间的虚拟内存区域就是线性区，在通常的体系架构里，
线性区与之对应的物理区域为 ZONE_DMA/ZONE_DMA32 与 ZONE_NORMAL.

![](/assets/PDB/RPI/RPI000735.png)

由于架构的差异，系统可能只存在 ZONE_NORMAL 的物理分区，有的可能也包含了
ZONE_DMA 分区，但不论物理地址如何划分，线性区一般都是将第一块内存起始地址物理
地址与内核起始虚拟地址处开始映射整个 ZONE_DMA/ZONE_DMA32 和 ZONE_NORMAL 分区.
在 SLAB 分配器未出现之前，系统只存在 Buddy 分配器，如果系统想要从线性区分配
内存 (虚拟内核+物理内存)，那么 Buddy 分配器只能提供按物理页为单位进行分配，
因此内核只需要几十个字节的内存，那么 Buddy 分配器只能提供一个物理页大小的内存，
分配这个物理页之后，只有几十个字节供给请求者使用，剩余的内存其他进程是不可以
使用的，也就是剩余的内存完全浪费了，只有请求者使用完几十个字节之后，将这段内存
归还给 Buddy 分配器，那么这个物理页对应的内存才能供系统其他地方使用。设想一下，
如果系统很多地方需要申请几十个字节的内存，那么系统内存资源一会就会被浪费完，
因此系统需要一个能够分配小粒度内存的分配器，以此满足系统的需求。

基于上面的需求，内核提供了一些或好或坏的解决方案，在一般的操作系统文献中都有
讲解，例如 Tan07. 此类提议之一，所谓 slab 分配器，证明对许多类工作负荷都是
高效。SLAB 分配器由 Sun 公司的一个雇员 Jeff Bonwick 在 Solaris 2.4 中设计并
实现的。由于他公开了其方法 Bon94, 因此也可以为 Linux 实现一个版本.

SLAB 分配器的出现不仅解决了小粒度内存分配的问题，而且还提供了面向对象的分配
机制。Linux 内核经常高频率分配和使用一些数据结构，由于分配太频繁，如果此时分配
速率太慢的话会影响系统的性能，因此 SLAB 面向这些数据结构设计了高速缓存，内核
可以从这些高速缓存上快速获得需要的内存，当内核使用完毕之后可以将这些内存再归
还给高速缓存，以便其他申请者使用。这样的设计很大程度上提高了分配速率和系统速度，
也从一定程度上减少了系统内存的浪费。由于系统 CACHE 的存在，高速缓存中的内存
很容易命中，这也在一定程度上加速了系统运行的速率.

------------------------------------

#### SLAB 分配器实现

![](/assets/PDB/RPI/TB000001.png)

> - [SLAB 分配器术语](#A0001)

SLAB 分配器的实现相对于其他分配器有一些复杂，但由于其严密的组织架构，逻辑上
SLAB 分配器是很明朗的。SLAB 分配器为高速缓存对象构建了本地高速缓存、共享高速
缓存、slab 链表等结构，以满足快速内存分配, 更提供了 slab 着色策略，以便缓存
对象能够在 CACHE 中进行命中.

![](/assets/PDB/RPI/TB000008.png)

SLAB 分配器从 Buddy 分配器中分配一定数量的物理内存，这些物理内存对应的虚拟
地址位于线性映射区，因此物理地址和虚拟地址的页表已经建立好了。SLAB 分配器
将获得内存 (这里指物理内存和对应的虚拟内存) 分作两部分，其一部分用于管理
这段内存，另外一部分内存划分为分配对象大小的内存块。SLAB 分配器将这样的组织
结构称为 slab，管理数据部分称为 slab 管理数据，另一部分的每一个内存块称为
缓存对象，slab 是 SLAB 分配器管理的基本单位，而缓存对象是 SLAB 分配器分配
的基本单位. 在 slab 中，slab 使用 kmem_bufctl 数组将所有可用缓存维护成一个
单向链表.

![](/assets/PDB/RPI/TB000005.png)

SLAB 分配器使用 struct kmem_cache 维护一个高速缓存，并且为每个 CPU 构建了一个
本地高速缓存，通过 array 成员进行指定，本地高速缓存使用缓存栈维护一定数量的
可用缓存对象，每个 CPU 可用从本地高速缓存上快速获得一个可用的缓存对象，并且
缓存栈使用 FILO 的模式，可以让刚刚释放的缓存对象再次被使用，这大大增加了缓存
对象在 CACHE 中的命中率。

高速缓存创建了一个共享高速缓存，用户维护来自各个 CPU 的本地高速缓存释放的缓存
对象，当某个本地高速缓存的缓存栈上没有可用缓存对象时，那么该本地高速缓存就会
去共享高速缓存上获一定数量的高速缓存，当本地高速缓存上维护缓存对象数量超过了
上限，那么本地高速缓存把缓存对象释放为共享高速缓存进行维护。共享缓存的存在解决
了本地共享高速缓存之间不能共享缓存对象的问题，其次共享缓存对象的存在在一定
程度上加速缓存的分配，因此本地高速缓存没有可用缓存对象的时候，不用去 slab 链表
上直接查找缓存对象。

高速缓存从 Buddy 分配器获得内存之后，将其组织成 slab 进行管理，slab 中维护
一定数量的缓存对象。高速缓存在每个 NODE 上创建一个 slab 链表用于维护高速缓存
的 slab，slab 链表一共包含三种链表，slabs_partial 链表上维护的 slab 中包含
部分可用缓存对象; slabs_full 链表上维护的 slab 中包含的缓存对象已经全部分配;
slabs_free 链表上维护的 slab 中的缓存对象全部可用。

![](/assets/PDB/RPI/TB000109.png)

因此 SLAB 分配缓存对象的优先级如上图，首先从 CPU 对应的本地高速缓存上分配，
如果没有那么从当前 NODE 的共享缓存上进行分配，如果没有那么从当前 NODE 的
slab 链表中分配，如果没有那么从 Buddy 分配器中分配。高速缓存通过层层加速，
最终利于分配的速率.

![](/assets/PDB/RPI/TB000010.png)

SLAB 分配器将所有的高速缓存维护在 cache_chain 链表中，以便统一管理.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### SLAB 的优点

SLAB 分配器能够满足小粒度的内存分配。提供了通用高速缓存，满足了通用内存的分配。
通过 SLAB 着色能够很好的增加缓存对象在 CACHE 中的命中，以此提高了系统速度.

###### SLAB 的缺点

SLAB 分配器对每个高速缓存都带来了巨大的管理数据开销，以及日常维护分配器的巨大
开销.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="A0001"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### SLAB 分配器术语

> - [高速缓存](#A0001)
>
> - [slab](#A0002)
>
> - [缓存对象](#A0003)
>
> - [本地高速缓存](#A0004)
>
> - [共享高速缓存](#A0005)
>
> - [通用/专用高速缓存](#A0010)
>
> - [slab 链表](#A0006)
>
> - [本地缓存栈](#A0007)
>
> - [kmem_bufctl 数组](#A0008)
>
> - [slab 着色](#A0009)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0010">通用/专用高速缓存</span>

内核中经常分配不规则长度的小粒度的内存，SLAB 为了加速这类型内存的分配，专门
创建了一系列的高速缓存，这些高速缓存包含了基础的长度，能够满足大部分的分配需求。

{% highlight c %}
# .name  .name_dma
size-32                 size-32(DMA)
size-64                 size-64(DMA)
size-96                 size-96(DMA)
size-128                size-128(DMA)
size-192                size-192(DMA)
size-256                size-256(DMA)
size-512                size-512(DMA)
size-1024               size-1024(DMA)
size-2048               size-2048(DMA)
size-4096               size-4096(DMA)
size-8192               size-8192(DMA)
size-16384              size-16384(DMA)
size-32768              size-32768(DMA)
size-65536              size-65536(DMA)
size-131072             size-131072(DMA)
size-262144             size-262144(DMA)
size-524288             size-524288(DMA)
size-1048576            size-1048576(DMA)
size-2097152            size-2097152(DMA)
size-4194304            size-4194304(DMA)
{% endhighlight %}

内核可以直接使用 kmalloc() 相关的函数进行分配.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0001">高速缓存</span>

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器不仅可以为通用长度的小粒度内存分配内存，也可以提供面向对象的小粒度
内存分配. 系统经常高频率分配和使用某些数据结构，SLAB 分配器为这些数据结构创建
了一个类似与缓存池的东西，系统可以从这个缓存池中快速获得指定大小的内存，也可以
将使用完毕的内存再放到这个缓存池里，以供其他申请者使用。SLAB 将这个缓存池称为
高速缓存。

SLAB 分配器使用 struct kmem_cache 维护一个高速缓存，高速缓存管理一定的内存，
并将这些内存划分为指定对象大小的内存块，高速缓存为系统提供了一套快速获得指定
大小内存块的机制，同时也包含了一套回收指定内存块的机制，以此让高速缓存为系统
所需的功能.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0002">slab</span>

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器使用高速缓存管理一定数量的内存，并向系统提供了申请和释放指定大小
内存块的服务。SLAB 分配器所需要的内存首先从 Buddy 分配器中分配一定数量的物理
内存，然后获得这些物理内存对应的虚拟内存，再将这些虚拟内存组织成带有一定管理
功能的数据块，SLAB 分配器称这些数据块为 slab. slab 的组成如下:

![](/assets/PDB/RPI/TB000008.png)

slab 分配两个部分，第一个部分就是 slab 管理数据区域，该区域包含了管理 slab 和
缓存对象的数据结构。第二个部分就是缓存对象区域，包含了多个缓存对象. slab 是 SLAB
分配器管理的基本单位. 高速缓存就是从 slab 上获得可用的缓存对象，然后将这些缓存
对象分配给申请者，当申请者使用完毕之后，再将缓存对象返回给 slab。

![](/assets/PDB/RPI/TB000021.png)

slab 的管理数据可以位于 slab 内部，也可以位于 slab 外部, 如上面两张图. 无论
slab 管理数据位于 slab 内部还是 slab 外部，其由一个 struct slab 数据结构和
一个 kmem_bufctl 数组进行维护。struct slab 数据结构中维护了 slab 基础信息，
包括 slab 中可用缓存对象的数量、第一个可用缓存对象的位置等信息。kmem_bufct 数
组则是一个 kmem_bufctl_t 数组，数组一共包含了与 slab 中缓存对象数量相等的
成员，并且 kmem_bufctl 数组中的每个成员都与 slab 中的缓存对象一一对应。
kmem_bufctl 数组用于指向指定缓存对象的下一个可用缓存对象位置.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0003">缓存对象</span>

![](/assets/PDB/RPI/TB000008.png)

在 SLAB 中，高速缓存通过 slab 管理可用内存，SLAB 分配器将 slab 管理可用内存
划分为指定对象大小的内存块，称这些内存块为缓存对象。缓存对象是 SLAB 分配的基础
单位. 每个缓存对象是与高速缓存指向的对象同样大小，因此高速缓存通过维护多个可用
的缓存对象，以便供系统分配调用。缓存对象可以维护在 slab 中，也可以维护在本地高
速缓存或者共享缓存里.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0004">本地高速缓存</span>

![](/assets/PDB/RPI/TB000001.png)

高速缓存的初衷不仅是为了分配小粒度的内存，而且是为了加快内存的分配，因此
SLAB 分配器觉得每次去遍历 slab 链表已获得可用的缓存对象，这样的分配速度存在
颠簸 (有时能从 slab 链表中快速找到，有时找的很慢，有时找不到还要从 Buddy 分配
器中分配内存后，重新制作 slab), 并且在 SMP 系统中，多个 CPU 同时遍历 slab 链表，
以此获得想要的缓存对象，这回导致 CPU 等待，这样的结果已经背离了初衷，于是
SLAB 分配器为了防止分配时的颠簸，以及多个 CPU 同时从高速缓存中分配内存，那么
SLAB 分配器提出了为每个 CPU 创建本地高速缓存，并在本地高速缓存上使用缓存栈
提前维护一定数量的可用缓存对象，当缓存栈上的缓存不够时，可用从 slab 链表上一
次性获得多个缓存对象。这样就解决了上面遇到的问题.

![](/assets/PDB/RPI/TB000005.png)

本地高速函数通过 struct kmem_cache 的 array 数组进行指定，使用 struct 
cache_array 结构进行维护.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0005">共享高速缓存</span>

![](/assets/PDB/RPI/TB000001.png)

在 SMP 架构中，SLAB 分配器为每个 CPU 提供了一个本地高速缓存，本地高速缓存使用
一个缓存栈维护一定数量的可用缓存对象。但每个本地缓存都会占用多个可用缓存对象，
如果其中一个本地缓存上没有可用缓存对象而其他本地高速缓存上还有很多缓存对象，
那么该本地缓存只好去 slab 链表中查找可用缓存，这在一定程度上造成了颠簸。为了
让本地高速缓存可以使用其他本地高速缓存，但又不打破本地这个特性，SLAB 分配器
为高速缓存创建了一个共享高速缓存。共享高速缓存的组织方式与本地高速缓存一样，
使用 struct cache_array 数据结构进行维护，其也通过一个缓存栈维护来自所有本地
高速缓存释放的可用缓存对象. 因此当本地高速缓存没有可用缓存对象的时候，不是直接
去 slab 链表中查找，而是去共享高速缓存中进行查找。指定注意的是，如果高速缓存
对象的缓存对象长度大于 PAGE_SIZE, 或者在 UP 架构上不存在共享高速缓存.

高速缓存使用 struct Kmem_cache 进行管理，其成员 nodelists 里通过 shared 成员
进行指定.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0006">slab 链表</span>

![](/assets/PDB/RPI/TB000001.png)

高速缓存从 Buddy 分配器中分配一定数量的物理内存，然后将这些物理内存对应的内存
通过 slab 管理起来，然后高速缓存将 slab 维护在 slab 链表上，slab 链表使用
struct kmem_list3 数据结构进行维护，slab 链表一共存在三种链表，分别是 slabs_full
链表，该链表上的 slab 里全部缓存对象已经分配; slabs_partial 链表，该链表上的
slab 里包含部分可用的缓存对象; slabs_free 链表，该链表上的 slab 里的缓存对象
全部可用.

当高速缓存的本地高速缓存和共享高速缓存上没有可用的缓存对象时，高速缓存就会
在 slab 链表的 slabs_partial 和 slabs_free 上进行查找，查找之后再将其放回到
本地高速缓存。如果本地高速缓存上维护缓存对象的数量超过了上限，那么本地高速
缓存将缓存对象释放回共享高速缓存，如果此时共享高速缓存维护缓存对象数量超过
上限，那么缓存对象会被释放会 slab 链表里。

如果从 slabs_free 的 slab 上分配缓存，那么该 slab 可能会从 slabs_free 中移除
并插入到 slabs_partial 或者 slabs_full 链表上。当释放一定数据的缓存对象回
slabs_partial 中的 slab，那么该 slab 可能会从 slabs_partial 链表中移除插入到
slabs_free 链表里.

高速缓存通过 struct kmem_cache 进行维护，其成员 nodelists 指向每个 NODE 的
slab 链表，因此高速缓存为每个 NODE 准备了一套 slab 链表.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0007">本地缓存栈</span>

高速缓存为每个 CPU 创建了本地高速缓存，以加快 CPU 快速分配缓存对象. 本地高速
缓存使用缓存栈维护一定数量的可用缓存对象。本地高速缓存使用 struct cache_array
数据结构进行维护，其 avail 成员指明缓存栈上可用缓存对象的数量，以及指向了缓存
栈的栈顶. limit 成员用于指定缓存栈上最大维护可用缓存对象的数量，batchcount 
成员用于指定一次性从共享缓存或 slab 中获得可用缓存的数量，或者一次性释放到
共享缓存或者 slab 缓存对象的数量. entry 成员是一个零数组，用于指向多个可用
缓存对象。

缓存栈采用 FILO 的模式，即 SLAB 分配器刚释放的缓存会被一下次分配使用，这样
很大程度上保持了缓存对象能够保持在 CACHE 中. 其二是当释放一定数量的可用缓存
对象回共享高速函数或者 slab 时，总是将缓存栈栈底的多个缓存对象释放，尽量保持
缓存对象在 CACHE 中命中.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0008">kmem_bufctl 数组</span>

![](/assets/PDB/RPI/TB000007.png)

在 slab 中，使用 slab 管理数据管理这 slab 上可用缓存对象的分配和回收。slab 管理
数据包含了 struct slab 和 kmem_bufctl 数组。kmem_bufctl 数组是由 kmem_bufctl_t
类型的成员构成，kmem_bufctl 数组一共包含了 slab 中维护缓存对象的数量.

kmem_bufctl 中的成员与 slab 中的缓存对象一一对应，成员用于描述对应缓存对象
的下一个可用缓存对象，这样就会在 slab 中构成一个单项的可用缓存链表，最后
一个对象使用 BUFCTL_END 结尾.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

###### <span id="A0009">slab 着色</span>

![](/assets/PDB/RPI/TB000007.png)

SLAB 分配器为了提供缓存对象在 CACHE 中的命中效率提出一种解决方案，这里的着色
和颜色没有一点关系。slab 着色的意思是让 slab 的缓存对象能够加载到 CACHE 中的
不同 cache line 中. 这样能最大程度让 cache 命中。SLAB 着色首先计算出 slab 从
Buddy 内存分配的内存中，减去管理数据和缓存对象之后剩余的内存空间，再将剩余的
内存空间除以一定的对齐模式，那么就可以算出高速缓存能够着色的范围。当计算出
高速缓存的着色范围之后，每次从 Buddy 中分配内存之后，在组织 slab 的过程中，
都会从内存开始处预留一定的空间，这部分空间就称为 slab 着色.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### SLAB 分配器使用

> - [基础用法介绍](#B0000)
>
> - [高速缓存使用](#B0001)
>
> - [通用高速缓存使用](#B0002)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="B0000">基础用法介绍</span>

SLAB 分配器提供了用于分配和释放虚拟内存的相关函数接口:

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0001">高速缓存使用</span>

![](/assets/PDB/RPI/TB000001.png)

高速缓存的使用包括了高速缓存的创建与销毁，以及从高速缓存中分配和释放缓存
对象.

{% highlight c %}
#include <linux/mm.h>
#include <linux/slab.h>

struct node_default {
        unsigned long index;
        unsigned long offset;
        char *name;
};

static int TestCase_kmem_cache_func(void)
{
        struct kmem_cache *BiscuitOS_cache;
        struct node_default *np;

        /* kmem cache create */
        BiscuitOS_cache = kmem_cache_create("BiscuitOS-cache",
                                sizeof(struct node_default),
                                0,
                                SLAB_HWCACHE_ALIGN,
                                NULL,
                                NULL);
        if (!BiscuitOS_cache) {
                printk("%s kmem_cache_create failed.\n", __func__);
                return -ENOMEM;
        }

        /* alloc from cache */
        np = kmem_cache_alloc(BiscuitOS_cache, SLAB_ATOMIC);
        if (!np) {
                printk("%s kmem_cache_alloc failed.\n", __func__);
                goto out_alloc;
        }

        np->index = 0x98;
        printk("%s INDEX: %#lx\n", __func__, np->index);

        /* free to cache */
        kmem_cache_free(BiscuitOS_cache, np);

out_alloc:
        /* kmem cache destroy */
        kmem_cache_destroy(BiscuitOS_cache);
        return 0;
}

{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0002">通用高速缓存使用</span>

{% highlight c %}
# .name  .name_dma
size-32                 size-32(DMA)
size-64                 size-64(DMA)
size-96                 size-96(DMA)
size-128                size-128(DMA)
size-192                size-192(DMA)
size-256                size-256(DMA)
size-512                size-512(DMA)
size-1024               size-1024(DMA)
size-2048               size-2048(DMA)
size-4096               size-4096(DMA)
size-8192               size-8192(DMA)
size-16384              size-16384(DMA)
size-32768              size-32768(DMA)
size-65536              size-65536(DMA)
size-131072             size-131072(DMA)
size-262144             size-262144(DMA)
size-524288             size-524288(DMA)
size-1048576            size-1048576(DMA)
size-2097152            size-2097152(DMA)
size-4194304            size-4194304(DMA)
{% endhighlight %}

SLAB 分配器为内核提供了上表的通用高速缓存，以满足特定内存分配需求. SLAB
提供了通用高速缓存的分配和释放函数, 如下:

{% highlight c %}
#include <linux/mm.h>
#include <linux/slab.h>

struct node_default {
        unsigned long index;
        unsigned long offset;
        char *name;
};

static int TestCase_kmalloc(void)
{
        struct node_default *np;

        /* allocate */
        np = (struct node_default *)kmalloc(sizeof(struct node_default),
                                                               GFP_KERNEL);
        if (!np) {
                printk("%s kmalloc failed.\n", __func__);
                return -ENOMEM;
        }

        np->index = 0x50;
        np->offset = 0x60;
        printk("%s INDEX: %#lx\n", __func__, np->index);

        /* Free */
        kfree(np);

        return 0;
}
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)


------------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### SLAB 分配器实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)
>
> - [实践建议](/blog/HISTORY-MMU/#C0003)
>
> - [测试建议](#C0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

本实践是基于 BiscuitOS Linux 5.0 ARM32 环境进行搭建，因此开发者首先
准备实践环境，请查看如下文档进行搭建:

> - [BiscuitOS Linux 5.0 ARM32 环境部署](/blog/Linux-5.0-arm32-Usermanual/)

--------------------------------------------

#### <span id="C0001">实践部署</span>

准备好基础开发环境之后，开发者接下来部署项目所需的开发环境。由于项目
支持多个版本的 SLAB，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 SLAB 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](/assets/PDB/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](/assets/PDB/RPI/TB000110.png)

选择并进入 "[\*]   SLAB Allocator  --->" 目录。

![](/assets/PDB/RPI/TB000111.png)

选择 "[\*]   SLAB on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make download
{% endhighlight %}

![](/assets/PDB/RPI/TB000112.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](/assets/PDB/RPI/TB000113.png)

arch 目录下包含内存初始化早期，与体系结构相关的处理部分。mm 目录下面包含
了与各个内存分配器和内存管理行为相关的代码。init 目录下是整个模块的初始化
入口流程。modules 目录下包含了内存分配器的使用例程和测试代码. fs 目录下
包含了内存管理信息输出到文件系统的相关实现。入口函数是 init/main.c 的
start_kernel()。

如果你是第一次使用这个项目，需要修改 DTS 的内容。如果不是可以跳到下一节。
开发者参考源码目录里面的 "BiscuitOS.dts" 文件，将文件中描述的内容添加
到系统的 DTS 里面，"BiscuitOS.dts" 里的内容用来从系统中预留 100MB 的物理
内存供项目使用，具体如下:

![](/assets/PDB/RPI/RPI000738.png)

开发者将 "BiscuitOS.dts" 的内容添加到:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

添加完毕之后，使用如下命令更新 DTS:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make kernel
{% endhighlight %}

![](/assets/PDB/RPI/TB000114.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make
{% endhighlight %}

![](/assets/PDB/RPI/TB000115.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make run
{% endhighlight %}

![](/assets/PDB/RPI/TB000116.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_SLAB-2.6.12.ko"，打印如上信息，那么
BiscuitOS Memory Manager Unit History 项目的内存管理子系统已经可以使用。

--------------------------------------

###### <span id="C0004">测试建议</span>

BiscuitOS Memory Manager Unit History 项目提供了大量的测试用例用于测试
不同内存分配器的功能。结合项目提供的 initcall 机制，项目将测试用例分作
两类，第一类类似于内核源码树内编译，也就是同 MMU 子系统一同编译的源码。
第二类类似于模块编译，是在 MMU 模块加载之后独立加载的模块。以上两种方案
皆在最大程度的测试内存管理器的功能。

要在项目中使用以上两种测试代码，开发者可以通过项目提供的 Makefile 进行
配置。以 linux 2.6.12 为例， Makefile 的位置如下:

{% highlight bash %}
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12/BiscuitOS_SLAB-2.6.12/Makefile
{% endhighlight %}

![](/assets/PDB/RPI/RPI000771.png)

Makefile 内提供了两种方案的编译开关，例如需要使用打开 buddy 内存管理器的
源码树内部调试功能，需要保证 Makefile 内下面语句不被注释:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/buddy/main.o
{% endhighlight %}

如果要关闭 buddy 内存管理器的源码树内部调试功能，可以将其注释:

{% highlight bash %}
# $(MODULE_NAME)-m                += modules/buddy/main.o
{% endhighlight %}

同理，需要打开 buddy 模块测试功能，可以参照下面的代码:

{% highlight bash %}
obj-m                             += $(MODULE_NAME)-buddy.o
$(MODULE_NAME)-buddy-m            := modules/buddy/module.o
{% endhighlight %}

如果不需要 buddy 模块测试功能，可以参考下面代码, 将其注释:

{% highlight bash %}
# obj-m                             += $(MODULE_NAME)-buddy.o
# $(MODULE_NAME)-buddy-m            := modules/buddy/module.o
{% endhighlight %}

在上面的例子中，例如打开了 buddy 的模块调试功能，重新编译模块并在 BiscuitOS
上运行，如下图，可以在 "lib/module/5.0.0/extra/" 目录下看到两个模块:

![](/assets/PDB/RPI/RPI000772.png)

然后先向 BiscuitOS 中插入 "BiscuitOS_SLAB-2.6.12.ko" 模块，然后再插入
"BiscuitOS_SLAB-2.6.12-buddy.ko" 模块。如下:

![](/assets/PDB/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](/assets/PDB/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

Initcall 机制提供了以下函数用于 SLAB 调试:

{% highlight bash %}
fixmap_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，fixmap_initcall_bs() 调用的函数将
在 SLAB 分配器初始化完毕之后自动调用。SLAB 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12/BiscuitOS_SLAB-2.6.12/module/fixmap/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/fixmap/main.o
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### SLAB 历史补丁

> - [SLAB Linux 2.6.12](#H-linux-2.6.12)
>
> - [SLAB Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [SLAB Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [SLAB Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [SLAB Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [SLAB Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [SLAB Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [SLAB Linux 2.6.13](#H-linux-2.6.13)
>
> - [SLAB Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [SLAB Linux 2.6.14](#H-linux-2.6.14)
>
> - [SLAB Linux 2.6.15](#H-linux-2.6.15)
>
> - [SLAB Linux 2.6.16](#H-linux-2.6.16)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](/assets/PDB/RPI/RPI000785.JPG)

#### SLAB Linux 2.6.12

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来. 并
产生了 2 个补丁:

{% highlight bash %}
tig mm/slab.h include/linux/slab.h

2005-05-01 08:58 Manfred Spraul   o [PATCH] add kmalloc_node, inline cleanup
                                    [main] 97e2bde47f886a317909c8a8f9bd2fcd8ce2f0b0
2005-05-01 08:59 Paul E. McKenney o [PATCH] Change synchronize_kernel to _rcu and _sched
                                    [main] fbd568a3e61a7decb8a754ad952aaa5b5c82e9e5
{% endhighlight %}

![](/assets/PDB/RPI/TB000117.png)

{% highlight bash %}
git format-patch -1 97e2bde47f886a317909c8a8f9bd2fcd8ce2f0b0
vi 0001-PATCH-add-kmalloc_node-inline-cleanup.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000118.png)

该补丁的作用是增加了 kmem_cache_alloc_node() 和 kmalloc_node() 函数，使其
在 NUMA 架构中能从指定的 NODE 上分配内存。并增加了 \_\_find_general_cachep()
函数，然后将 kmem_find_general_cachep() 函数导出. 更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](/assets/PDB/RPI/RPI000786.JPG)

#### SLAB Linux 2.6.12.1

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.12, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](/assets/PDB/RPI/RPI000787.JPG)

#### SLAB Linux 2.6.12.2

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.12.1, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](/assets/PDB/RPI/RPI000788.JPG)

#### SLAB Linux 2.6.12.3

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.12.2, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](/assets/PDB/RPI/RPI000789.JPG)

#### SLAB Linux 2.6.12.4

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.12.3, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](/assets/PDB/RPI/RPI000790.JPG)

#### SLAB Linux 2.6.12.5

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.12.4, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](/assets/PDB/RPI/RPI000791.JPG)

#### SLAB Linux 2.6.12.6

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.12.5, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](/assets/PDB/RPI/RPI000792.JPG)

#### SLAB Linux 2.6.13

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.12.6, 该版本并产生了多个补丁。

{% highlight bash %}
tig mm/slab.c include/linux/slab.h

2005-06-18 22:46 Arnaldo Carvalho de Melo o [SLAB] Introduce kmem_cache_name
                                            [main] 1944972d3bb651474a5021c9da8d0166ae19f1eb
2005-06-21 17:14 Christoph Lameter        o [PATCH] Periodically drain non local pagesets
                                            [main] 4ae7c03943fca73f23bc0cdb938070f41b98101f
2005-06-23 00:09 Paulo Marques            o [PATCH] create a kstrdup library function
                                            [main] 543537bd922692bc978e2e356fcd8bfc9c2ee7d5
2005-07-06 10:47 Christoph Lameter        o [PATCH] Fix broken kmalloc_node in rc1/rc2
                                            [main] 83b78bd2d31f12d7d9317d9802a1996a7bd8a6f2
2005-07-07 17:56 Alexey Dobriyan          o [PATCH] propagate __nocast annotations
                                            [main] 0db925af1db5f3dfe1691c35b39496e2baaff9c9
2005-07-27 11:43 Alexey Dobriyan          o [PATCH] Really __nocast-annotate kmalloc_node()
                                            [main] c10b873695c6a1de0d8ebab40b525575ca576683
{% endhighlight %}

![](/assets/PDB/RPI/TB000119.png)

{% highlight bash %}
git format-patch -1 1944972d3bb651474a5021c9da8d0166ae19f1eb
vi 0001-SLAB-Introduce-kmem_cache_name.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000120.png)

该补丁用户获得高速缓存的名字.

{% highlight bash %}
git format-patch -1 543537bd922692bc978e2e356fcd8bfc9c2ee7d5
vi 0001-PATCH-create-a-kstrdup-library-function.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000121.png)

该补丁提供了 kstrdup() 接口，该接口用于为指定字符串分配内存空间，并将原始数据
拷贝到新的内存里.

{% highlight bash %}
git format-patch -1 83b78bd2d31f12d7d9317d9802a1996a7bd8a6f2
vi 0001-PATCH-Fix-broken-kmalloc_node-in-rc1-rc2.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000122.png)

该补丁用于当使用 kmem_cache_alloc_node() 分配缓存对象时，如果 nodeid 设置为
-1，那么高速缓存从当前节点进行分配缓存对象. 更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](/assets/PDB/RPI/RPI000793.JPG)

#### SLAB Linux 2.6.13.1

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.13, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](/assets/PDB/RPI/RPI000794.JPG)

#### SLAB Linux 2.6.14

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.13.1, 该版本并产生了多个补丁。

{% highlight bash %}
tig mm/slab.c include/linux/slab.h

2005-09-03 15:54 Martin Hicks             o [PATCH] vm: slab.c spelling correction
                                            [main] 0abf40c1ac3f25d264c019e1cfe155d590defb87
2005-09-03 15:55 Kyle Moffett             o [PATCH] sab: consolidate kmem_bufctl_t
                                            [main] fa5b08d5f818063d18433194f20359ef2ae50254
2005-09-03 15:55 Eric Dumazet             o [PATCH] mm/slab.c: prefetchw the start of new allocated objects
                                            [main] 34342e863c3143640c031760140d640a06c6a5f8
2005-09-03 15:55 Manfred Spraul           o [PATCH] slab: removes local_irq_save()/local_irq_restore() pair
                                            [main] 00e145b6d59a16dd7740197a18f7abdb3af004a9
2005-09-06 15:18 Pekka J Enberg           o [PATCH] introduce and use kzalloc
                                            [main] dd3927105b6f65afb7dac17682172cdfb86d3f00
2005-09-09 13:03 Christoph Lameter        o [PATCH] Numa-aware slab allocator V5
                                            [main] e498be7dafd72fd68848c1eef1575aa7c5d658df
2005-09-09 13:10 Pekka Enberg             o [PATCH] update kfree, vfree, and vunmap kerneldoc
                                            [main] 80e93effce55044c5a7fa96e8b313640a80bd4e9
2005-09-10 00:26 Victor Fusco             o [PATCH] mm/slab: fix sparse warnings
                                            [main] b2d550736f8b2186b8ef7e206d0bfbfec2238ae8
2005-09-14 12:17 Alok Kataria             o [PATCH] Fix slab BUG_ON() triggered by change in array cache size
                                            [main] c7e43c78ae4d8630c418ce3495787b995e61a580
2005-09-22 21:43 Ivan Kokshaysky          o [PATCH] slab: alpha inlining fix
                                            [main] 7243cc05bafdda4c4de77cba00cf87666bd237f7
2005-09-22 21:44 Christoph Lameter        o [PATCH] slab: fix handling of pages from foreign NUMA nodes
                                            [main] ff69416e6323fe9d38c42a06ebdefeb58bbe9336
2005-09-22 21:44 Christoph Lameter        o [PATCH] __kmalloc: Generate BUG if size requested is too large.
                                            [main] eafb42707b21beb42bba4eae7b742f837ee9d2e0
2005-09-23 13:24 Andrew Morton            o [PATCH] revert oversized kmalloc check
                                            [main] dbdb90450059e17e8e005ebd3ce0a1fd6008a0c8
2005-09-27 21:45 Alok N Kataria           o [PATCH] kmalloc_node IRQ safety fix
                                            [main] 5c382300876f2337f7b945c159ffcaf285f296ea
2005-10-07 07:46 Al Viro                  o [PATCH] gfp flags annotations - part 1
                                            [main] dd0fc66fb33cd610bc1a5db8a5e232d34879b4d7
{% endhighlight %}

![](/assets/PDB/RPI/TB000123.png)

{% highlight bash %}
git format-patch -1 0abf40c1ac3f25d264c019e1cfe155d590defb87
vi 0001-PATCH-vm-slab.c-spelling-correction.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000124.png)

该补丁修改了拼写错误.

{% highlight bash %}
git format-patch -1 fa5b08d5f818063d18433194f20359ef2ae50254
vi 0001-PATCH-sab-consolidate-kmem_bufctl_t.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000125.png)

该补丁用于将 kmem_bufctl_t 的定义迁移到 slab.c 中定义.

{% highlight bash %}
git format-patch -1 34342e863c3143640c031760140d640a06c6a5f8
vi 0001-PATCH-mm-slab.c-prefetchw-the-start-of-new-allocated.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000126.png)

该补丁用于从高速缓存中分配一个缓存对象时调用 prefetchw() 预取该对象.

{% highlight bash %}
git format-patch -1 00e145b6d59a16dd7740197a18f7abdb3af004a9
vi 0001-PATCH-slab-removes-local_irq_save-local_irq_restore-.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000127.png)

该补丁用于在获得高速缓存的缓存对象大小时，移除了 local_irq_save() 和 
local_irq_restore().

{% highlight bash %}
git format-patch -1 dd3927105b6f65afb7dac17682172cdfb86d3f00
vi 0001-PATCH-introduce-and-use-kzalloc.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000128.png)

该补丁将 kcalloc() 函数替换成 kzalloc() 函数.

{% highlight bash %}
git format-patch -1 e498be7dafd72fd68848c1eef1575aa7c5d658df
vi 0001-PATCH-Numa-aware-slab-allocator-V5.patch
{% endhighlight %}

该补丁用于支持本地高速缓存的缓存栈，以及支持 NUMA 架构共享高速缓存，并且多个
NODE 之间共享缓存对象.

{% highlight bash %}
git format-patch -1 c7e43c78ae4d8630c418ce3495787b995e61a580
vi 0001-PATCH-Fix-slab-BUG_ON-triggered-by-change-in-array-c.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000129.png)

该补丁修复了当查找 struct cache_array 对应的通用高速缓存，如果高速缓存
不存在，函数应该调用 BUG_ON() 报错.

{% highlight bash %}
git format-patch -1 ff69416e6323fe9d38c42a06ebdefeb58bbe9336
vi 0001-PATCH-slab-fix-handling-of-pages-from-foreign-NUMA-n.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000130.png)

该补丁用于当释放缓存对象回 slab 时，增加了对 NODE 的标识.

{% highlight bash %}
git format-patch -1 eafb42707b21beb42bba4eae7b742f837ee9d2e0
vi 0001-PATCH-__kmalloc-Generate-BUG-if-size-requested-is-to.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000131.png)

该补丁用于当使用 \_\_kmalloc() 分配超级大内存的时候，如果找不到对于的高速
缓存，那么函数调用 BUG_ON() 进行报错.

{% highlight bash %}
git format-patch -1 dbdb90450059e17e8e005ebd3ce0a1fd6008a0c8
vi 0001-PATCH-revert-oversized-kmalloc-check.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000132.png)

该补丁将 BUG_ON() 修改为原先的处理逻辑.

{% highlight bash %}
git format-patch -1 5c382300876f2337f7b945c159ffcaf285f296ea
vi 0001-PATCH-kmalloc_node-IRQ-safety-fix.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000133.png)

该补丁用于 kmem_cache_alloc_node() 函数在不同 NODE 上使用 \_\_cache_alloc()
函数分配. 更多补丁使用请参考下文:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](/assets/PDB/RPI/RPI000795.JPG)

#### SLAB Linux 2.6.15

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.12 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.14, 该版本并产生了多个补丁。

{% highlight bash %}
tig mm/slab.c include/linux/slab.h

2005-10-21 03:18 Al Viro                  o [PATCH] gfp_t: mm/* (easy parts)
                                            [main] 6daa0e28627abf362138244a620a821a9027d816
2005-10-29 18:15 Christoph Lameter        o [PATCH] slab: add additional debugging to detect slabs from the wrong node
                                            [main] 09ad4bbc3a5c93316d7f4ffc0c310d9cbb28c2f0
2005-11-07 00:58 Andrew Morton            o [PATCH] slab: don't BUG on duplicated cache
                                            [main] 4f12bb4f7715f418a9c80f89447948790f476958
2005-11-07 00:58 Pekka J Enberg           o [PATCH] mm: rename kmem_cache_s to kmem_cache
                                            [main] 2109a2d1b175dfcffbfdac693bdbe4c4ab62f11f
2005-11-07 00:58 Manfred Spraul           o [PATCH] slab: Use same schedule timeout for all cpus in cache_reap
                                            [main] cd61ef6268ac52d3dfa5626d1e0306a91b3b2608
2005-11-07 01:01 Randy Dunlap             o [PATCH] more kernel-doc cleanups, additions
                                            [main] 1e5d533142c1c178a31d4cc81837eb078f9269bc
2005-11-08 16:44 Adrian Bunk              o mm/slab.c: fix a comment typo
                                            [main] dc6f3f276e2b4cbc1563def8fb39373a45db84ac
2005-11-13 16:06 Pekka Enberg             o [PATCH] slab: convert cache to page mapping macros
                                            [main] 065d41cb269e9debb18c6d5052e4de1088ae3d8f
2005-11-13 16:06 Christoph Lameter        o [PATCH] slab: remove alloc_pages() calls
                                            [main] 50c85a19e7b3928b5b5188524c44ffcbacdd4e35
{% endhighlight %}

![](/assets/PDB/RPI/TB000134.png)

{% highlight bash %}
git format-patch -1 4f12bb4f7715f418a9c80f89447948790f476958
vi 0001-PATCH-slab-don-t-BUG-on-duplicated-cache.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000136.png)

该补丁用于解决复杂高速缓存名字时候出现的 BUG.

{% highlight bash %}
git format-patch -1 2109a2d1b175dfcffbfdac693bdbe4c4ab62f11f
vi 0001-PATCH-mm-rename-kmem_cache_s-to-kmem_cache.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000137.png)

该补丁将 struct kmem_cache_s 修改为 struct kmem_cache.

{% highlight bash %}
git format-patch -1 065d41cb269e9debb18c6d5052e4de1088ae3d8f
vi 0001-PATCH-slab-convert-cache-to-page-mapping-macros.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000138.png)

该补丁提供了 page_set_cache()、page_set_slab() 函数等，用于 slab、page 与高速
缓存之间绑定.

{% highlight bash %}
git format-patch -1 50c85a19e7b3928b5b5188524c44ffcbacdd4e35
vi 0001-PATCH-slab-remove-alloc_pages-calls.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000139.png)

该补丁用于提供从 Buddy 分配器获得物理内存的统一接口. 更多补丁使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.16"></span>

![](/assets/PDB/RPI/TB000156.JPG)

#### SLAB Linux 2.6.16

![](/assets/PDB/RPI/TB000007.png)

Linux 2.6.16 采用 SLAB 分配器分配小粒度内存。

###### 高速缓存 

{% highlight bash %}
kmem_cache_create
kmem_cache_alloc
kmem_cache_alloc_notrace
kmem_cache_destroy
kmem_cache_free
kmem_cache_name
kmem_cache_zalloc
{% endhighlight %}

###### 通用高速缓存

{% highlight bash %}
kzalloc
kmalloc
kmalloc_node
kfree
{% endhighlight %}

###### 转换函数

{% highlight bash %}
virt_to_slab
page_get_slab
page_get_cache
page_set_cache
page_set_slab
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB Allocator API](#K)

###### 与项目相关

项目中虚拟内存布局如下:

![](/assets/PDB/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x90000000 到 0x94400000.

###### 补丁

相对于前一个版本 linux 2.6.15, 该版本并产生了多个补丁。

{% highlight bash %}
tig mm/slab.c include/linux/slab.h

2006-01-08 01:00 Pekka Enberg             o [PATCH] slab: remove unused align parameter from alloc_percpu
                                            [main] f9f7500521b25dbf1aba476b81230489ad8e2c4b
2006-01-08 01:00 Pekka Enberg             o [PATCH] slab: extract slabinfo header printing to separate function
                                            [main] 85289f98ddc13f6cea82c59d6ff78f9d205dfccc
2006-01-08 01:00 Pekka Enberg             o [PATCH] slab: extract slab order calculation to separate function
                                            [main] 4d268eba1187ef66844a6a33b9431e5d0dadd4ad
2006-01-08 01:00 Pekka Enberg             o [PATCH] slab: fix code formatting
                                            [main] b28a02de8c70d41d6b6ba8911e83ed3ccf2e13f8
2006-01-08 01:00 Tobias Klauser           o [PATCH] mm: clean up local variables
                                            [main] cd105df4590c89837a1c300843238148cfef9b5f
2006-01-08 01:01 Matt Mackall             o [PATCH] slob: introduce mm/util.c for shared functions
                                            [main] 30992c97ae9d01b17374fbfab76a869fb4bba500
2006-01-08 01:01 Matt Mackall             o [PATCH] slob: introduce the SLOB allocator
                                            [main] 10cef6029502915bdb3cf0821d425cf9dc30c817
2006-01-09 15:59 Ingo Molnar              o [PATCH] mutex subsystem, more debugging code
                                            [main] de5097c2e73f826302cd8957c225b3725e0c7553
2006-01-11 14:41 David Woodhouse          o [PATCH] fix/simplify mutex debugging code
                                            [main] a4fc7ab1d065a9dd89ed0e74439ef87d4a16e980
2006-01-18 17:42 Ingo Molnar              o [PATCH] sem2mutex: mm/slab.c
                                            [main] fc0abb1451c64c79ac80665d5ba74450ce274e4d
2006-01-18 17:42 Christoph Lameter        o [PATCH] NUMA policies in the slab allocator V2
                                            [main] dc85da15d42b0efc792b0f5eab774dc5dbc1ceec
2006-01-18 17:42 Christoph Lameter        o [PATCH] mm: optimize numa policy handling in slab allocator
                                            [main] 86c562a9d6683063e071692fe14e0a18e64ee1be
2006-02-01 03:05 Benjamin LaHaise         o [PATCH] Use 32 bit division in slab_put_obj()
                                            [main] 9884fd8df195fe48d4e1be2279b419be96127cae
2006-02-01 03:05 Manfred Spraul           o [PATCH] slab: distinguish between object and buffer size
                                            [main] 3dafccf22751429e69b6266636cf3acf45b48075
2006-02-01 03:05 Christoph Lameter        o [PATCH] slab: minor cleanup to kmem_cache_alloc_node
                                            [main] 18f820f655ce93b1e4d9b48fc6fcafc64157c6bc
2006-02-01 03:05 Steven Rostedt           o [PATCH] slab: have index_of bug at compile time
                                            [main] 5ec8a847bb8ae2ba6395cfb7cb4bfdc78ada82ed
2006-02-01 03:05 Steven Rostedt           o [PATCH] slab: cache_estimate cleanup
                                            [main] fbaccacff1f17c65ae0972085368a7ec75be6062
2006-02-01 03:05 Matthew Dobson           o [PATCH] slab: extract slab_destroy_objs()
                                            [main] 12dd36faec5d3bd96da84fa8f76efecc632930ab
2006-02-01 03:05 Matthew Dobson           o [PATCH] slab: extract slab_{put|get}_obj
                                            [main] 78d382d77c84229d031431931bf6490d5da6ab86
2006-02-01 03:05 Pekka Enberg             o [PATCH] slab: reduce inlining
                                            [main] 5295a74cc0bcf1291686eb734ccb06baa3d55c1a
2006-02-01 03:05 Pekka Enberg             o [PATCH] slab: extract virt_to_{cache|slab}
                                            [main] 6ed5eb2211204224799b2821656bbbfde26ef200
2006-02-01 03:05 Pekka Enberg             o [PATCH] slab: rename ac_data to cpu_cache_get
                                            [main] 9a2dba4b4912b493070cbc170629fdbf440b01d7
2006-02-01 03:05 Pekka Enberg             o [PATCH] slab: replace kmem_cache_t with struct kmem_cache
                                            [main] 343e0d7a93951e35065fdb5e3dd61aece0ec6b3c
2006-02-01 03:05 Pekka Enberg             o [PATCH] slab: fix kzalloc and kstrdup caller report for CONFIG_DEBUG_SLAB
                                            [main] 7fd6b1413082c303613fc137aca9a004740cacf0
2006-02-01 03:05 Randy.Dunlap             o [PATCH] mm/slab: add kernel-doc for one function
                                            [main] a70773ddb96b74c7afe5a5bc859ba45e3d02899e
2006-02-01 03:05 Randy Dunlap             o [PATCH] slab: fix sparse warning
                                            [main] ee13d785eac1fbe7e79ecca77bf7e902734a0b30
2006-02-04 23:27 Ravikiran G Thirumalai   o [PATCH] NUMA slab locking fixes: move color_next to l3
                                            [main] 2e1217cf96b54d3b2d0162930608159e73507fbf
2006-02-04 23:27 Ravikiran G Thirumalai   o [PATCH] NUMA slab locking fixes: irq disabling from cahep->spinlock to l3 lock
                                            [main] ca3b9b91735316f0ec7f01976f85842e0bfe5c6e
2006-02-04 23:27 Ravikiran G Thirumalai   o [PATCH] NUMA slab locking fixes: fix cpu down and up locking
                                            [main] 4484ebf12bdb0ebcdc6e8951243cbab3d7f6f4c1
2006-02-05 11:26 Linus Torvalds           o mm/slab.c (non-NUMA): Fix compile warning and clean up code
                                            [main] 7a21ef6fe902ac0ad53b45af6851ae5ec3a64299
2006-02-10 01:51 Ravikiran G Thirumalai   o [PATCH] slab: Avoid deadlock at kmem_cache_create/kmem_cache_destroy
                                            [main] f0188f47482efdbd2e005103bb4f0224a835dfad
2006-03-06 12:10 Linus Torvalds           o Fix "check_slabp" printout size calculation
                                            [main] 264132bc62fe071d0ff378c1103bae9d33212f10
2006-03-06 17:44 Linus Torvalds           o slab: clarify and fix calculate_slab_order()
                                            [main] 9888e6fa7b68d9c8cc2c162a90979825ab45150a
2006-03-08 10:33 Linus Torvalds           o slab: fix calculate_slab_order() for SLAB_RECLAIM_ACCOUNT
                                            [main] f78bb8ad482267b92c122f0e37a7dce69c880247
2006-03-07 21:55 Jack Steiner             o [PATCH] slab: allocate larger cache_cache if order 0 fails
                                            [main] 07ed76b2a085a31f427c2a912a562627947dc7de
2006-03-09 17:33 Christoph Lameter        o [PATCH] slab: Node rotor for freeing alien caches and remote per cpu pages.
                                            [main] 8fce4d8e3b9e3cf47cc8afeb6077e22ab795d989
{% endhighlight %}

![](/assets/PDB/RPI/TB000140.png)

{% highlight bash %}
git format-patch -1 85289f98ddc13f6cea82c59d6ff78f9d205dfccc
vi 0001-PATCH-slab-extract-slabinfo-header-printing-to-separ.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000141.png)

该补丁修改了 print_slabinfo_header() 函数的显示方法.

{% highlight bash %}
git format-patch -1 4d268eba1187ef66844a6a33b9431e5d0dadd4ad
vi 0001-PATCH-slab-extract-slab-order-calculation-to-separat.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000142.png)

该补丁添加了 calculate_slab_order() 函数用于计算 slab 占用物理页的数量.

{% highlight bash %}
git format-patch -1 b28a02de8c70d41d6b6ba8911e83ed3ccf2e13f8
vi 0001-PATCH-slab-fix-code-formatting.patch 
{% endhighlight %}

该补丁修改了 mm/slab.c 的代码风格.

{% highlight bash %}
git format-patch -1 30992c97ae9d01b17374fbfab76a869fb4bba500
vi 0001-PATCH-slob-introduce-mm-util.c-for-shared-functions.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000143.png)

该补丁将 kzalloc() 函数和 kstrdup() 函数移动到 mm/util.c.

{% highlight bash %}
git format-patch -1 10cef6029502915bdb3cf0821d425cf9dc30c817
vi 0001-PATCH-slob-introduce-the-SLOB-allocator.patch 
{% endhighlight %}

该补丁增加了对 SLOB 分配器的支持.

{% highlight bash %}
git format-patch -1 fc0abb1451c64c79ac80665d5ba74450ce274e4d
vi 0001-PATCH-sem2mutex-mm-slab.c.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000144.png)

该补丁用于将 up/down 换成了互斥锁.

{% highlight bash %}
git format-patch -1 9884fd8df195fe48d4e1be2279b419be96127cae
vi 0001-PATCH-Use-32-bit-division-in-slab_put_obj.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000145.png)

该补丁限定了 slab 中包含的缓存对象数用 32 位表示.

{% highlight bash %}
git format-patch -1 3dafccf22751429e69b6266636cf3acf45b48075
vi 0001-PATCH-slab-distinguish-between-object-and-buffer-siz.patch
{% endhighlight %}

该补丁采用了 buffer_size 维护高速缓存对象的长度.

{% highlight bash %}
git format-patch -1 5ec8a847bb8ae2ba6395cfb7cb4bfdc78ada82ed
vi 0001-PATCH-slab-have-index_of-bug-at-compile-time.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000146.png)

该补丁在 index_of() 函数中添加了 \_\_bad_size().

{% highlight bash %}
git format-patch -1 fbaccacff1f17c65ae0972085368a7ec75be6062
vi 0001-PATCH-slab-cache_estimate-cleanup.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000147.png)

该补丁用于在 cache_estimate() 函数中支持外部 slab 管理数据的计算.

{% highlight bash %}
git format-patch -1 12dd36faec5d3bd96da84fa8f76efecc632930ab
vi 0001-PATCH-slab-extract-slab_destroy_objs.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000148.png)

该补丁用于增加函数 slab_destroy_objs().

{% highlight bash %}
git format-patch -1 78d382d77c84229d031431931bf6490d5da6ab86
vi 0001-PATCH-slab-extract-slab_-put-get-_obj.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000149.png)

该补丁用于提供 slab_get_obj() 函数和 slab_put_obj() 函数.

{% highlight bash %}
git format-patch -1 6ed5eb2211204224799b2821656bbbfde26ef200
vi 0001-PATCH-slab-extract-virt_to_-cache-slab.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000150.png)

该补丁提供了 virt_to_cache() 函数和 virt_to_slab() 函数.

{% highlight bash %}
git format-patch -1 9a2dba4b4912b493070cbc170629fdbf440b01d7
vi 0001-PATCH-slab-rename-ac_data-to-cpu_cache_get.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000151.png)

该补丁提供了 cpu_get_cache() 函数用于获得本地高速缓存..

{% highlight bash %}
git format-patch -1 343e0d7a93951e35065fdb5e3dd61aece0ec6b3c
vi 0001-PATCH-slab-replace-kmem_cache_t-with-struct-kmem_cac.patch
{% endhighlight %}

该补丁用于将 kmem_cache_t 替换成 struct kmem_cache..

{% highlight bash %}
git format-patch -1 2e1217cf96b54d3b2d0162930608159e73507fbf
vi 0001-PATCH-NUMA-slab-locking-fixes-move-color_next-to-l3.patch 
{% endhighlight %}

![](/assets/PDB/RPI/TB000152.png)

该补丁修正了着色的起始地址，从 0 开始着色.

{% highlight bash %}
git format-patch -1 9888e6fa7b68d9c8cc2c162a90979825ab45150a
vi 0001-slab-clarify-and-fix-calculate_slab_order.patch
{% endhighlight %}

![](/assets/PDB/RPI/TB000153.png)

该补丁修改 calculate_slab_order() 函数，是其支持从 order 0 开始计算从 Buddy
分配器中获得物理页. 更多补丁使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](/blog/HISTORY-MMU/#C00033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### SLAB 历史时间轴

![](/assets/PDB/RPI/TB000154.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

#### SLAB API

###### 数据结构

> - [struct array_cache](#K0002000)
>
> - [struct cache_names](#K0002001)
>
> - [struct cache_sizes](#K0002002)
>
> - [struct ccupdate_struct](#K0002003)
>
> - [struct kmem_cache](#K0002004)
>
> - [struct kmem_list3](#K0002005)
>
> - [struct slab](#K0002006)
>
> - [struct slab_rcu](#K0002007)
>
> - [kmem_bufctl_t](#K0002008)
>
> - [struct arraycache_init](#K0002009)

##### Function API

> - [alloc_arraycache](#K0001000)
>
> - [alloc_kmemlist](#K0001001)
>
> - [alloc_slabmgmt](#K0001002)
>
> - [\_\_\_\_cache_alloc](#K0001003)
>
> - [\_\_cache_alloc](#K0001004)
>
> - [cache_alloc_refill](#K0001005)
>
> - [cache_estimate](#K0001006)
>
> - [cache_flusharray](#K0001007)
>
> - [\_\_cache_free](#K0001008)
>
> - [cache_grow](#K0001009)
>
> - [cache_init_objs](#K0001010)
>
> - [\_\_cache_shrink](#K0001011)
>
> - [calculate_slab_order](#K0001012)
>
> - [cpu_cache_get](#K0001013)
>
> - [\_\_do_cache_alloc](#K0001014)
>
> - [do_ccupdate_local](#K0001015)
>
> - [do_drain](#K0001016)
>
> - [\_\_do_kmalloc](#K0001017)
>
> - [do_tune_cpucache](#K0001018)
>
> - [drain_array](#K0001019)
>
> - [drain_cpu_caches](#K0001020)
>
> - [drain_freelist](#K0001021)
>
> - [enable_cpucache](#K0001022)
>
> - [\_\_find_general_cachep](#K0001023)
>
> - [free_block](#K0001024)
>
> - [index_to_obj](#K0001025)
>
> - [init_list](#K0001026)
>
> - [kfree](#K0001027)
>
> - [\_\_kmalloc](#K0001028)
>
> - [kmalloc](#K0001029)
>
> - [kmalloc_node](#K0001030)
>
> - [kmem_cache_alloc](#K0001031)
>
> - [kmem_cache_alloc_notrace](#K0001032)
>
> - [kmem_cache_create](#K0001033)
>
> - [\_\_kmem_cache_destroy](#K0001034)
>
> - [kmem_cache_destroy](#K0001035)
>
> - [kmem_cache_free](#K0001036)
>
> - [kmem_cache_init](#K0001037)
>
> - [kmem_cache_init_late](#K0001038)
>
> - [kmem_cache_name](#K0001039)
>
> - [kmem_cache_shrink](#K0001040)
>
> - [kmem_cache_size](#K0001041)
>
> - [kmem_cache_zalloc](#K0001042)
>
> - [kmem_find_general_cachep](#K0001043)
>
> - [kmem_freepages](#K0001044)
>
> - [kmem_getpages](#K0001045)
>
> - [kmem_list3_init](#K0001046)
>
> - [kmem_rcu_free](#K0001047)
>
> - [kzalloc](#K0001048)
>
> - [MAKE_ALL_LISTS](#K0001049)
>
> - [MAKE_LIST](#K0001050)
>
> - [obj_size](#K0001051)
>
> - [obj_to_index](#K0001052)
>
> - [page_get_cache](#K0001053)
>
> - [page_get_slab](#K0001054)
>
> - [page_set_cache](#K0001055)
>
> - [page_set_slab](#K0001056)
>
> - [set_up_list3s](#K0001057)
>
> - [setup_cpu_cache](#K0001058)
>
> - [slab_bufctl](#K0001059)
>
> - [slab_destroy](#K0001060)
>
> - [slab_get_obj](#K0001061)
>
> - [slab_is_available](#K0001062)
>
> - [slab_map_pages](#K0001063)
>
> - [slab_mgmt_size](#K0001064)
>
> - [slab_put_obj](#K0001065)
>
> - [virt_to_slab](#K0001066)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="K0002009">struct arraycache_init</span>

> - [struct arraycache_init Linux 2.6.34 版本源码解析](#D0107)

---------------------------------------------

###### <span id="K0002008">kmem_bufctl_t</span>

> - [kmem_bufctl_t Linux 2.6.34 版本源码解析](#D0106)

---------------------------------------------

###### <span id="K0002007">struct slab_rcu</span>

> - [struct slab_rcu Linux 2.6.34 版本源码解析](#D030063)

---------------------------------------------

###### <span id="K0002006">struct slab</span>

> - [struct slab Linux 2.6.34 版本源码解析](#D0102)

---------------------------------------------

###### <span id="K0002005">struct kmem_list3</span>

> - [struct kmem_list3 Linux 2.6.34 版本源码解析](#D0100)

---------------------------------------------

###### <span id="K0002004">struct kmem_cache</span>

> - [struct kmem_cache Linux 2.6.34 版本源码解析](#D0103)

---------------------------------------------

###### <span id="K0002003">struct ccupdate_struct</span>

> - [struct ccupdate_struct Linux 2.6.34 版本源码解析](#D0108)

---------------------------------------------

###### <span id="K0002002">struct cache_sizes</span>

> - [struct cache_sizes Linux 2.6.34 版本源码解析](#D0104)

---------------------------------------------

###### <span id="K0002001">struct cache_names</span>

> - [struct cache_names Linux 2.6.34 版本源码解析](#D0105)

---------------------------------------------

###### <span id="K0002000">struct array_cache</span>

> - [struct array_cache Linux 2.6.34 版本源码解析](#D0101)

---------------------------------------------

###### <span id="K0001066">virt_to_slab</span>

> - [virt_to_slab Linux 2.6.34 版本源码解析](#D030054)

---------------------------------------------

###### <span id="K0001065">slab_put_obj</span>

> - [slab_put_obj Linux 2.6.34 版本源码解析](#D030047)

---------------------------------------------

###### <span id="K0001064">slab_mgmt_size</span>

> - [slab_mgmt_size Linux 2.6.34 版本源码解析](#D030003)

---------------------------------------------

###### <span id="K0001063">slab_map_pages</span>

> - [slab_map_pages Linux 2.6.34 版本源码解析](#D030014)

---------------------------------------------

###### <span id="K0001062">slab_is_available</span>

> - [slab_is_available Linux 2.6.34 版本源码解析](#D030038)

---------------------------------------------

###### <span id="K0001061">slab_get_obj</span>

> - [slab_get_obj Linux 2.6.34 版本源码解析](#D030005)

---------------------------------------------

###### <span id="K0001060">slab_destroy</span>

> - [slab_destroy Linux 2.6.34 版本源码解析](#D030052)

---------------------------------------------

###### <span id="K0001059">slab_bufctl</span>

> - [slab_bufctl Linux 2.6.34 版本源码解析](#D030007)

---------------------------------------------

###### <span id="K0001058">setup_cpu_cache</span>

> - [setup_cpu_cache Linux 2.6.34 版本源码解析](#D030037)

---------------------------------------------

###### <span id="K0001057">set_up_list3s</span>

> - [set_up_list3s Linux 2.6.34 版本源码解析](#D030001)

---------------------------------------------

###### <span id="K0001056">page_set_slab</span>

> - [page_set_slab Linux 2.6.34 版本源码解析](#D030013)

---------------------------------------------

###### <span id="K0001055">page_set_cache</span>

> - [page_set_cache Linux 2.6.34 版本源码解析](#D030010)

---------------------------------------------

###### <span id="K0001054">page_get_slab</span>

> - [page_get_slab Linux 2.6.34 版本源码解析](#D030012)

---------------------------------------------

###### <span id="K0001053">page_get_cache</span>

> - [page_get_cache Linux 2.6.34 版本源码解析](#D030011)

---------------------------------------------

###### <span id="K0001052">obj_to_index</span>

> - [obj_to_index Linux 2.6.34 版本源码解析](#D030048)

---------------------------------------------

###### <span id="K0001051">obj_size</span>

> - [obj_size Linux 2.6.34 版本源码解析](#D030065)

---------------------------------------------

###### <span id="K0001050">MAKE_LIST</span>

> - [MAKE_LIST Linux 2.6.34 版本源码解析](#D030042)

---------------------------------------------

###### <span id="K0001049">MAKE_ALL_LISTS</span>

> - [MAKE_ALL_LISTS Linux 2.6.34 版本源码解析](#D030043)

---------------------------------------------

###### <span id="K0001048">kzalloc</span>

> - [kzalloc Linux 2.6.34 版本源码解析](#D030035)

---------------------------------------------

###### <span id="K0001047">kmem_rcu_free</span>

> - [kmem_rcu_free Linux 2.6.34 版本源码解析](#D030050)

---------------------------------------------

###### <span id="K0001046">kmem_list3_init</span>

> - [kmem_list3_init Linux 2.6.34 版本源码解析](#D030000)

---------------------------------------------

###### <span id="K0001045">kmem_getpages</span>

> - [kmem_getpages Linux 2.6.34 版本源码解析](#D030008)

---------------------------------------------

###### <span id="K0001044">kmem_freepages</span>

> - [kmem_freepages Linux 2.6.34 版本源码解析](#D030049)

---------------------------------------------

###### <span id="K0001043">kmem_find_general_cachep</span>

> - [kmem_find_general_cachep Linux 2.6.34 版本源码解析](#D030025)

---------------------------------------------

###### <span id="K0001042">kmem_cache_zalloc</span>

> - [kmem_cache_zalloc Linux 2.6.34 版本源码解析](#D030022)

---------------------------------------------

###### <span id="K0001041">kmem_cache_size</span>

> - [kmem_cache_size Linux 2.6.34 版本源码解析](#D030066)

---------------------------------------------

###### <span id="K0001040">kmem_cache_shrink</span>

> - [kmem_cache_shrink Linux 2.6.34 版本源码解析](#D030064)

---------------------------------------------

###### <span id="K0001039">kmem_cache_name</span>

> - [kmem_cache_name Linux 2.6.34 版本源码解析](#D030067)

---------------------------------------------

###### <span id="K0001038">kmem_cache_init_late</span>

> - [kmem_cache_init_late Linux 2.6.34 版本源码解析](#D030044)

---------------------------------------------

###### <span id="K0001037">kmem_cache_init</span>

> - [kmem_cache_init Linux 2.6.34 版本源码解析](#D030040)

---------------------------------------------

###### <span id="K0001036">kmem_cache_free</span>

> - [kmem_cache_free Linux 2.6.34 版本源码解析](#D030051)

---------------------------------------------

###### <span id="K0001035">kmem_cache_destroy</span>

> - [kmem_cache_destroy Linux 2.6.34 版本源码解析](#D030062)

---------------------------------------------

###### <span id="K0001034">\_\_kmem_cache_destroy</span>

> - [\_\_kmem_cache_destroy Linux 2.6.34 版本源码解析](#D030057)

---------------------------------------------

###### <span id="K0001033">kmem_cache_create</span>

> - [kmem_cache_create Linux 2.6.34 版本源码解析](#D030039)

---------------------------------------------

###### <span id="K0001032">kmem_cache_alloc_notrace</span>

> - [kmem_cache_alloc_notrace Linux 2.6.34 版本源码解析](#D030029)

---------------------------------------------

###### <span id="K0001031">kmem_cache_alloc</span>

> - [kmem_cache_alloc Linux 2.6.34 版本源码解析](#D030021)

---------------------------------------------

###### <span id="K0001030">kmalloc_node</span>

> - [kmalloc_node Linux 2.6.34 版本源码解析](#D030030)

---------------------------------------------

###### <span id="K0001029">kmalloc</span>

> - [kmalloc Linux 2.6.34 版本源码解析](#D030028)

---------------------------------------------

###### <span id="K0001028">\_\_kmalloc</span>

> - [\_\_kmalloc Linux 2.6.34 版本源码解析](#D030027)

---------------------------------------------

###### <span id="K0001027">kfree</span>

> - [kfree Linux 2.6.34 版本源码解析](#D030055)

---------------------------------------------

###### <span id="K0001026">init_list</span>

> - [init_list Linux 2.6.34 版本源码解析](#D030041)

---------------------------------------------

###### <span id="K0001025">index_to_obj</span>

> - [index_to_obj Linux 2.6.34 版本源码解析](#D030006)

---------------------------------------------

###### <span id="K0001024">free_block</span>

> - [free_block Linux 2.6.34 版本源码解析](#D030053)

---------------------------------------------

###### <span id="K0001023">\_\_find_general_cachep</span>

> - [\_\_find_general_cachep Linux 2.6.34 版本源码解析](#D030024)

---------------------------------------------

###### <span id="K0001022">enable_cpucache</span>

> - [enable_cpucache Linux 2.6.34 版本源码解析](#D030036)

---------------------------------------------

###### <span id="K0001021">drain_freelist</span>

> - [drain_freelist Linux 2.6.34 版本源码解析](#D030056)

---------------------------------------------

###### <span id="K0001020">drain_cpu_caches</span>

> - [drain_cpu_caches Linux 2.6.34 版本源码解析](#D030060)

---------------------------------------------

###### <span id="K0001019">drain_array</span>

> - [drain_array Linux 2.6.34 版本源码解析](#D030059)

---------------------------------------------

###### <span id="K0001018">do_tune_cpucache</span>

> - [do_tune_cpucache Linux 2.6.34 版本源码解析](#D030033)

---------------------------------------------

###### <span id="K0001017">\_\_do_kmalloc</span>

> - [\_\_do_kmalloc Linux 2.6.34 版本源码解析](#D030026)

---------------------------------------------

###### <span id="K0001016">do_drain</span>

> - [do_drain Linux 2.6.34 版本源码解析](#D030058)

---------------------------------------------

###### <span id="K0001015">do_ccupdate_local</span>

> - [do_ccupdate_local Linux 2.6.34 版本源码解析](#D030034)

---------------------------------------------

###### <span id="K0001014">\_\_do_cache_alloc</span>

> - [\_\_do_cache_alloc Linux 2.6.34 版本源码解析](#D030019)

---------------------------------------------

###### <span id="K0001013">cpu_cache_get</span>

> - [cpu_cache_get Linux 2.6.34 版本源码解析](#D030004)

---------------------------------------------

###### <span id="K0001012">calculate_slab_order</span>

> - [calculate_slab_order Linux 2.6.34 版本源码解析](#D030023)

---------------------------------------------

###### <span id="K0001011">\_\_cache_shrink</span>

> - [\_\_cache_shrink Linux 2.6.34 版本源码解析](#D030061)

---------------------------------------------

###### <span id="K0001010">cache_init_objs</span>

> - [cache_init_objs Linux 2.6.34 版本源码解析](#D030015)

---------------------------------------------

###### <span id="K0001009">cache_grow</span>

> - [cache_grow Linux 2.6.34 版本源码解析](#D030016)

---------------------------------------------

###### <span id="K0001008">\_\_cache_free</span>

> - [\_\_cache_free Linux 2.6.34 版本源码解析](#D030045)

---------------------------------------------

###### <span id="K0001007">cache_flusharray</span>

> - [cache_flusharray Linux 2.6.34 版本源码解析](#D030046)

---------------------------------------------

###### <span id="K0001006">cache_estimate</span>

> - [cache_estimate Linux 2.6.34 版本源码解析](#D030002)

---------------------------------------------

###### <span id="K0001005">cache_alloc_refill</span>

> - [cache_alloc_refill Linux 2.6.34 版本源码解析](#D030017)

---------------------------------------------

###### <span id="K0001004">\_\_cache_alloc</span>

> - [\_\_cache_alloc Linux 2.6.34 版本源码解析](#D030020)

---------------------------------------------

###### <span id="K0001003">\_\_\_\_cache_alloc</span>

> - [ Linux 2.6.34 版本源码解析](\_\_\_\_cache_alloc)

---------------------------------------------

###### <span id="K0001002">alloc_slabmgmt</span>

> - [alloc_slabmgmt Linux 2.6.34 版本源码解析](#D030009)

---------------------------------------------

###### <span id="K0001001">alloc_kmemlist</span>

> - [alloc_kmemlist Linux 2.6.34 版本源码解析](#D030032)

---------------------------------------------

###### <span id="K0001000">alloc_arraycache</span>

> - [alloc_arraycache Linux 2.6.34 版本源码解析](#D030031)

---------------------------------------------

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

#### SLAB 进阶研究


![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

#### SLAB 内存分配器调试

> - [/proc/slabinfo](#E0000)
>
> - [BiscuitOS SLAB 内存分配器调试](#C0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------

#### <span id="E0000">/proc/slabinfo</span>

Linux 内核向文件系统添加了 slabinfo 节点，用于获得 slab 的所有使用信息，开发
者参考下列命令使用:

{% highlight bash %}
cat /proc/slabinfo
{% endhighlight %}

![](/assets/PDB/RPI/TB000155.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### SLAB 源码分析

> - [Linux 2.6.34](#D0)
>
> - [Linux 3.0 (Filling...)](#D1)
>
> - [Linux 4.1 (Filling...)](#D2)
>
> - [Linux 5.0 (Filling...)](#D3)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="D0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Z.jpg)

#### Linux 2.6.34

> - [逻辑解析](#D02)
>
> - 数据结构
>
>   - [struct array_cache](#D0101)
>
>   - [struct cache_names](#D0105)
>
>   - [struct cache_sizes](#D0104)
>
>   - [struct ccupdate_struct](#D0108)
>
>   - [struct kmem_cache](#D0103)
>
>   - [struct kmem_list3](#D0100)
>
>   - [struct slab](#D0102)
>
>   - [struct slab_rcu](#D030063)
>
>   - [kmem_bufctl_t](#D0106)
>
>   - [struct arraycache_init](#D0107)
>
> - 函数解析
>
>   - [alloc_arraycache](#D030031)
>
>   - [alloc_kmemlist](#D030032)
>
>   - [alloc_slabmgmt](#D030009)
>
>   - [\_\_\_\_cache_alloc](#D030018)
>
>   - [\_\_cache_alloc](#D030020)
>
>   - [cache_alloc_refill](#D030017)
>
>   - [cache_estimate](#D030002)
>
>   - [cache_flusharray](#D030046)
>
>   - [\_\_cache_free](#D030045)
>
>   - [cache_grow](#D030016)
>
>   - [cache_init_objs](#D030015)
>
>   - [\_\_cache_shrink](#D030061)
>
>   - [calculate_slab_order](#D030023)
>
>   - [cpu_cache_get](#D030004)
>
>   - [\_\_do_cache_alloc](#D030019)
>
>   - [do_ccupdate_local](#D030034)
>
>   - [do_drain](#D030058)
>
>   - [\_\_do_kmalloc](#D030026)
>
>   - [do_tune_cpucache](#D030033)
>
>   - [drain_array](#D030059)
>
>   - [drain_cpu_caches](#D030060)
>
>   - [drain_freelist](#D030056)
>
>   - [enable_cpucache](#D030036)
>
>   - [\_\_find_general_cachep](#D030024)
>
>   - [free_block](#D030053)
>
>   - [index_to_obj](#D030006)
>
>   - [init_list](#D030041)
>
>   - [kfree](#D030055)
>
>   - [\_\_kmalloc](#D030027)
>
>   - [kmalloc](#D030028)
>
>   - [kmalloc_node](#D030030)
>
>   - [kmem_cache_alloc](#D030021)
>
>   - [kmem_cache_alloc_notrace](#D030029)
>
>   - [kmem_cache_create](#D030039)
>
>   - [\_\_kmem_cache_destroy](#D030057)
>
>   - [kmem_cache_destroy](#D030062)
>
>   - [kmem_cache_free](#D030051)
>
>   - [kmem_cache_init](#D030040)
>
>   - [kmem_cache_init_late](#D030044)
>
>   - [kmem_cache_name](#D030067)
>
>   - [kmem_cache_shrink](#D030064)
>
>   - [kmem_cache_size](#D030066)
>
>   - [kmem_cache_zalloc](#D030022)
>
>   - [kmem_find_general_cachep](#D030025)
>
>   - [kmem_freepages](#D030049)
>
>   - [kmem_getpages](#D030008)
>
>   - [kmem_list3_init](#D030000)
>
>   - [kmem_rcu_free](#D030050)
>
>   - [kzalloc](#D030035)
>
>   - [MAKE_ALL_LIST](#D030043)
>
>   - [MAKE_LIST](#D030042)
>
>   - [obj_size](#D030065)
>
>   - [obj_to_index](#D030048)
>
>   - [page_get_cache](#D030011)
>
>   - [page_get_slab](#D030012)
>
>   - [page_set_cache](#D030010)
>
>   - [page_set_slab](#D030013)
>
>   - [set_up_list3s](#D030001)
>
>   - [setup_cpu_cache](#D030037)
>
>   - [slab_bufctl](#D030007)
>
>   - [slab_destroy](#D030052)
>
>   - [slab_get_obj](#D030005)
>
>   - [slab_is_available](#D030038)
>
>   - [slab_map_pages](#D030014)
>
>   - [slab_mgmt_size](#D030003)
>
>   - [slab_put_obj](#D030047)
>
>   - [virt_to_slab](#D030054)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="D02"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 逻辑解析

在 Linux 2.6.x 内核中，SLAB 分配器的初始化位于 Buddy 分配器之后、PERCPU 分配
器之前，因此 SLAB 分配器可以使用完整的 Buddy 分配器功能。

![](/assets/PDB/RPI/TB000108.png)

内核初始化阶段，通过调用 kmem_cache_init() 函数进行 SLAB 分配器的初始化。在
SLAB 初始化过程中，SLAB 需要使用从 SLAB 中分配的内存，因此 SLAB 的初始化就
出现了 "是先有蛋还是先有鸡" 的问题，为了解决这个问题，SLAB 分配器将 SLAB 分配
的初始化分作了五个阶段，如下:

![](/assets/PDB/RPI/TB000059.png)

###### NONE 阶段

SLAB 分配器初始化第一阶段，位于该阶段的 SLAB 分配器只包含了静态数据
cache_cache、initkmem_list3 以及 initarray_cache。SLAB 分配器在这个阶段
利用这些静态数据构建 SLAB 分配器的第一个高速缓存 cache_cache. cache_cache
高速缓存是为其他高速缓存提供 struct kmem_cache 对象。因此其作为 SLAB 根基
是不允许有任何失败的.

###### PARTIAL_AC 阶段

SLAB 分配器的第二个阶段，位于该阶段的 SLAB 分配器已经可以分配 struct kmem_cache
对象了，此阶段 SLAB 分配器通过获得 struct cache_array 的长度，以此构建一个
与该长度匹配的通用高速缓存，然后 SLAB 分配器可以使用这个高速缓存为 SLAB 分配器
提供 struct cache_array 对象，struct cache_array 用于维护高速缓存的本地高速
缓存，由于此阶段系统只使用一个 CPU 运行.

###### PARTIAL_L3 阶段

SLAB 分配器初始化第三阶段，位于该阶段的 SLAB 分配器已经具有 struct kmem_cache
和 struct cache_array 高速缓存。该阶段 SLAB 分配器获得 struct kmem_list3 的
长度之后，以此构建一个与该程度匹配的通用高速缓存，然后 SLAB 分配器就可以提供
struct kmem_list3 对象。struct kmem_list3 用于维护高速缓存的 slab 链表.

###### EARLY 阶段

SLAB 分配器初始化第四阶段，这个阶段 SLAB 分配器构建了通用高速缓存，并能为
系统提供大部分的 SLAB 分配器功能。只是该阶段系统只用 boot CPU 运行，并且
SLAB CPU 热插拔等功能还为支持. 该阶段 SLAB 分配器将原先的静态数据完全
替换成 SLAB 分配器分配器的内存，并将数据迁移到新的内存上.

###### FULL 阶段

SLAB 分配器初始化完毕。位于该阶段的 SLAB 分配器已经是一个完全体，并支持所有的
SLAB 分配器功能.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030067">kmem_cache_name</span>

![](/assets/PDB/RPI/TB000107.png)

kmem_cache_name() 函数用于获得高速缓存的名字. 参数 cachep 指向高速缓存。SLAB
分配器使用 struct kmem_cache 维护一个高速缓存，其中 name 成员用于指明高速缓存
的名字.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030066">kmem_cache_size</span>

![](/assets/PDB/RPI/TB000106.png)

kmem_cache_size() 函数用于获得缓存对象的长度。参数 cachep 指向高速缓存.
函数通过 obj_size() 函数获得缓存对象长度.

> [obj_size](#D030065)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030065">obj_size</span>

![](/assets/PDB/RPI/TB000105.png)

obj_size() 宏用于获得高速缓存的缓存对象长度. SLAB 分配器使用 struct kmem_cache
维护一个高速缓存，其中 buffer_size 用于指明缓存对象的大小.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030064">kmem_cache_shrink</span>

![](/assets/PDB/RPI/TB000104.png)

kmem_cache_shrink() 函数用于收缩高速缓存，将高速缓存的缓存对象释放。参数
cachep 指向高速缓存。函数首先进行基础检测，确保高速缓存存在，且不再中断中
执行该函数. 然后函数上互斥锁，并调用 \_\_cache_shrink() 函数进行实际的收缩
工作，完成收缩任务之后接触互斥锁.

> [\_\_cache_shrink](#D030061)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030063">struct slab_rcu</span>

![](/assets/PDB/RPI/TB000103.png)

struct slab_rcu 数据结构用于销毁一个 slab 时候，辅助 SLAB 分配器使用 RCU 方式
将 slab 进行销毁. head 成员用于 RCU 相关的数据，cachep 指向 slab 对应的高速
缓存，addr 指向 slab 对应的虚拟地址.

> [slab_destroy](#D030052)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030062">kmem_cache_destroy</span>

![](/assets/PDB/RPI/TB000102.png)

kmem_cache_destroy() 函数用于销毁一个高速缓存。参数 cachep 指向高速缓存.

![](/assets/PDB/RPI/TB000001.png)

当 SLAB 分配器需要销毁一个高速缓存的时候，SLAB 分配器需要将其本地高速缓存、
共享高速缓存，以及 slab 等占用的内存释放回 Buddy 分配器. 函数在 2546 行做了
基础检测，确保高速缓存存在，且调用该函数时不能位于中断中. 接着函数给
cache_chain 链表上互斥锁，然后将高速缓存从 cache_chain 链表中移除. 接着函数
在 2555 行调用 \_\_cache_shrink() 函数将高速缓存对应的本地高速缓存和共享缓存
以及 slab 占用的内存都释放回 Buddy 分配器，如果有缓存对象无法释放，那么函数
此时报错，并将高速缓存插入 cache_chain 链表，解除互斥锁，最后直接返回; 反之
如果 \_\_cache_shrink() 函数已经将所有缓存对象占用的内存释放回 Buddy 分配器，
那么函数调用 \_\_kmem_cache_destroy() 函数将自身释放回 cache_cache 高速缓存.

> [\_\_cache_shrink](#D030061)
>
> [\_\_kmem_cache_destroy](#D030057)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030061">\_\_cache_shrink</span>

![](/assets/PDB/RPI/TB000101.png)

\_\_cache_shrink() 函数用于收缩高速缓存. cachep 参数指向高速缓存. SLAB 分配
器提供了该函数用于缩减高速缓存占用的内存数量. 函数 2491 行调用 
drain_cpu_caches() 函数将高速缓存对应的本地缓存和共享高速缓存中的可用缓存释放
回 slab，然后函数在 2494 行遍历所有的 NODE，以便在每次遍历过程中获得对应的 slab
链表，如果检测 slab 链表存在，那么函数调用 drain_freelist() 函数将其 slab 占用
的内存释放回 Buddy 分配器. 如果 slab 链表中，slabs_full 或者 slabs_partial 不为
空，那么 ret 变量加一，最后如果 ret 不为零，那么返回 1，如果 ret 为零，代表
高速缓存还维护一定数量的 slab.

> [drain_cpu_caches](#D030060)
>
> [drain_freelist](#D030056)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030060">drain_cpu_caches</span>

![](/assets/PDB/RPI/TB000100.png)

drain_cpu_caches() 函数的作用是将本地高速缓存和共享高速缓存上的可用缓存对象
归还给 slab。参数 cachep 指向高速缓存.

函数 2429 行通过调用 do_drain() 函数将本地高速缓存释放会 slab。函数 2437 行
到 2441 行，行数遍历所有的 NODE，并将对应的共享高速缓存的缓存对象归还给 slab.

> [do_drain](#D030058)
>
> [drain_array](#D030059)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030059">drain_array</span>

![](/assets/PDB/RPI/TB000099.png)

drain_array() 函数用于从本地高速缓存上释放一定数量的缓存对象。参数 cachep 指向
高速缓存，l3 指向高速缓存的 slab 链表，ac 参数指向本地高速缓存，force 参数指明
是否全部释放, node 指明 NODE 信息.

![](/assets/PDB/RPI/TB000001.png)

高速缓存针对每个 CPU 设置了一个本地高速缓存，本地高速缓存通过一个缓存栈维护
一定数量的可用缓存对象，drain_array() 函数用于从这个缓存栈上释放一定数量的
可用缓存对象回 slab. 函数首先确认本地高速缓存存在以及缓存栈上维护一定数量的
缓存对象，接着函数如果探测到本地高速缓存已经被使用过，且 force 参数为 0，那么
函数仅仅将本地高速缓存的 touched 设置为 0; 反之那么函数检测到缓存栈上是否一定
数量的可用缓存对象，那么函数在 4019 行确认需要释放缓存对象的数量，如果 force
为真，那么将缓存栈上的所有缓存对象释放回 slab，反之将上限的 30% 返回到 slab。
如果返回的数量大于缓存栈上可用缓存对象的数量，那么返回缓存栈上 50% 的可用缓存
对象到 slab。于是在 4022 行函数调用 free_block() 函数将这些可用缓存返回给共享
高速缓存或则 slab，最后为了保持函数栈先进后出的特点，调用 memmove() 函数将
剩余的所有缓存对象整体移动待缓存栈的底部.

> [free_block](#D030053)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030058">do_drain</span>

![](/assets/PDB/RPI/TB000098.png)

do_drain() 函数用于释放一个本地高速缓存. 参数 arg 指向一个高速缓存. SLAB
分配器为了加速缓存对象的分配，SLAB 分配器为每个 CPU 创建了本地高速缓存。
本地高速缓存使用缓存栈维护一定数量的可用缓存对象，在 do_drain() 函数里，
函数会将缓存栈内的所有缓存对象全部释放给 slab。

函数在 2417 行调用 cpu_cache_get() 函数获得对应的本地缓存，然后使用
free_block() 函数将所有的缓存对象释放给 slab，最后将本地高速缓存的可用
缓存对象设置为 0.

> [cpu_cache_get](#D030004)
>
> [free_block](#D030053)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030057">\_\_kmem_cache_destroy</span>

![](/assets/PDB/RPI/TB000097.png)

\_\_kmem_cache_destroy() 函数用于释放高速缓存的本地缓存、slab 链表，以及
高速缓存本身. 参数 cachep 指向高速缓存.

函数在 1929 行调用 for_each_online_cpu() 函数遍历所有在线的 CPU，并调用 kfree
释放了所有本地高速缓存. 函数在 1933 行调用 for_each_online_node() 函数遍历
所有的 NODE 节点，并在每次遍历中释放高速缓存的 slab 链表以及共享高速缓存.
函数最后在 1941 行调用 kmem_cache_free() 函数释放了高速缓存本身.

> [kfree](#D030055)
>
> [kmem_cache_free](#D030051)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030056">drain_freelist</span>

![](/assets/PDB/RPI/TB000096.png)

drain_freelist() 函数用于从高速缓存 slabs_free 链表上移除指定数量的 slab
归还给 Buddy 分配器. cache 参数指向高速缓存, 参数 l3 指向 slab 链表，tofree
参数指明需要释放 slab 的数量.

函数在 2458 行使用 while 循环，只要 nr_freed 小于 tofree，且 slabs_free 链表
不为空，那么循环一直进行。在每次循环中，函数在 2461 行获得 slabs_free 链表最
后一个 slab，将 slab 从 slabs_free 链表上移除，并更新 slab 链表中可用缓存
对象的数量。函数接着在 2478 行调用 slab_destroy() 函数将 slab 占用的内存归还
给 Buddy 分配器. 最后返回释放 slab 的数量.

> [slab_destroy](#D030052)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030055">kfree</span>

![](/assets/PDB/RPI/TB000095.png)

kfree() 函数用于释放一段虚拟地址回 SLAB 分配器. 参数 objp 指向需要释放的虚拟
地址. 除去一些 debug 函数，函数通过 \_\_cache_free() 函数释放指定的内存.

> [\_\_cache_free](#D030045)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030046">cache_flusharray</span>

![](/assets/PDB/RPI/TB000086.png)

cache_flusharray() 函数用于将本地高速缓存里的批量缓存对象归还给共享高速缓存
或者 slab. 参数 cachep 指向高速缓存，ac 指向本地高速缓存.

![](/assets/PDB/RPI/TB000001.png)

函数在 3485 行获得释放缓存对象的数量，其可以从本地高速缓存的 batchcount 中获得。
3490 行获得高速缓存的 slab 链表，如果此时 l3 shared 不为零，及共享高速缓存
存在，那么函数执行 3493 行到 3502 行代码逻辑，在这段代码逻辑中，函数首先确认
共享高速缓存中维护可用缓存对象数量是否已经达到上限，如果没有达到上限，那么
函数将批量的缓存对象插入到共享高速缓存的缓存栈上，插入完毕之后调转到 free_done；
如果共享高速缓存已经无法容纳缓存对象了，那么直接跳转到 3504 行继续执行.
函数在 3505 行调用 free_block() 函数将批量的可用缓存对象归还给 slab，最后更新
本地高速缓存可用缓存对象的数量，并将栈定部分的缓存对象整体向栈底移动.

> [free_block](#D030053)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030054">virt_to_slab</span>

![](/assets/PDB/RPI/TB000094.png)

virt_to_slab() 函数用于通过缓存对象的虚拟地址获得对应的 slab。参数 obj 指向
缓存对象的虚拟地址. 函数首先调用 virt_to_head_page() 函数获得虚拟地址对应的
物理页，然后通过调用 page_get_slab() 函数获得物理页对应的 slab。

> [page_get_slab](#D030012)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030053">free_block</span>

![](/assets/PDB/RPI/TB000093.png)

free_block() 函数用于释放批量的缓存对象会 slab 链表里。参数 cachep 指向高速
缓存，参数 objpp 指向缓存所在为的位置，参数 nr_objects 指明释放缓存的数量，
参数 node 指明 NODE 信息.

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器为高速缓存在每个 NODE 上使用 kmem_list3 维护了所有的 slab，其通过
维护 3 种链表，将不同条件的 slab 插入到指定的 slab 链表中，其中 slabs_partial
链表上维护的 slab 包含了一定数量可用的缓存对象; slabs_full 链表上维护的 slab
的所有缓存对象已经分配; slabs_free 链表上维护的 slab 上的所有缓存对象都未分配。

函数在 3441 行遍历所有需要释放的缓存对象，每次遍历中，函数在 3445 行通过调用
virt_to_slab() 函数获得缓释放缓存对象对应的 slab，并结合 node 信息获得高速缓存
对应的 slab 链表。函数在 3450 行调用 slab_put_obj() 函数将释放对象在 slab 中
标记为可分配使用的, 并更新 slab 链表上可用缓存对象的数量。函数在 3456 行继续
检测当前 slab 的所有缓存对象是否都没有在使用，如果 slab 所有缓存对象都没有在
使用，那么函数继续检测当前 slab 链表上可用缓存对象数量是否已经超过 slab 链表
的上限，如果超过，那么 SLAB 分配器将该 slab 摧毁并释放会 Buddy 分配器，函数
3458 行更新 slab 链表上可用缓存对象数量，然后调用 slab_destory() 函数摧毁
slab; 如果该 slab 上所有缓存对象可用，当 slab 链表的可用缓存对象数量没有达到
上限，那么函数将该 slab 从插入到 slabs_free 链表里; 如果 slab 的在使用的缓存
数量不为零，那么将该 slab 插入到 slabs_partial 链表上.

> [virt_to_slab](#D030054)
>
> [slab_put_obj](#D030047)
>
> [slab_destroy](#D030052)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030052">slab_destroy</span>

![](/assets/PDB/RPI/TB000092.png)

slab_destroy() 函数用于释放一个 slab。参数 cachep 用于指向高速缓存，slabp 
指向即将释放的 slab. SLAB 分配器可以采用两种方式 slab，第一种是使用 RCU 的
方式释放 slab，第二种则是直接释放 slab。

函数在 1907 行首先获得 slab 占用物理页块的首地址。然后函数会的检测高速缓存
的标志中是否包含 SLAB_DESTROY_BY_RCU，如果包含，那么函数在使用 slab 时候需要
按 RCU 方式释放，1911 行到 1916 行函数使用数据结构 struct slab_rcu 辅助 SLAB
分配器使用 RCU 方式进行释放，函数将高速缓存和 slab 以及首地址存储在 slab_rcu
中，然后调用 call_rcu() 函数和 kmem_rcu_free() 在合适的情况下释放 slab; 反之
如果高速缓存不支持 SLAB_DESTROY_BY_RCU，那么函数通过 1918 行到 1921 行进行
slab 释放，函数通过调用 kmem_freepages() 函数将 slab 占用的物理页全部归还给
Buddy 分配器，如果此时 slab 的管理数据位于 slab 外部，那么函数继续调用
kmem_cache_free() 将 slab 管理数据释放回对应的高速缓存.

> [kmem_rcu_free](#D030050)
>
> [kmem_freepages](#D030049)
>
> [kmem_cache_free](#D030051)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030051">kmem_cache_free</span>

![](/assets/PDB/RPI/TB000091.png)

kmem_cache_free() 函数用于释放一个缓存对象. 参数 cachep 指向高速缓存，参数
objp 指向需要释放的缓存对象.

函数在 3753 行通过调用 \_\_cache_free() 函数释放一个缓存对象.

> [\_\_cache_free](#D030045)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030050">kmem_rcu_free</span>

![](/assets/PDB/RPI/TB000090.png)

kmem_rcu_free() 函数的作用是使用 RCU 的方式释放 slab. 参数 head 指向 struct
slab_rcu 结构体. 函数在 1684 行获得 slab 对应的高速缓存，并且 slab 对应的
地址位于 slab_rcu 的 addr 成员里。于是函数在 1686 行调用 kmem_freepages()
函数释放了 slab 占用的物理页。1687 行通过调用 OFF_SLAB() 函数检测 slab 的管理
数据是否位于 slab 外部，如果 slab 管理数据位于 slab 外部，那么函数调用 
kmem_cache_free() 函数，将 slab 管理数据释放会对应的高速缓存。

> [kmem_freepages](#D030049)
>
> [kmem_cache_free](#D030051)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030049">kmem_freepages</span>

![](/assets/PDB/RPI/TB000089.png)

kmem_freepages() 函数用于将 slab 占用的物理页释放会 Buddy 分配器. 参数 cachep
指向高速缓存，addr 指向物理页对应的虚拟地址. 当 SLAB 分配器为高速缓存创建 slab
的时候就会从 Buddy 分配器分配一定数量的物理页，当 SLAB 分配器根据需求，将高速
缓存的 slab 占用的内存释放回 Buddy 内核分配器。函数 1660 行调用 virt_to_page()
函数获得 addr 参数对应的物理页，如果高速缓存为 slab 分配物理页时支持
SLAB_RECLAIM_ACCOUNT 标志，也就代表申请的物理页支持回收，因此在释放的时候，函数
调用 sub_zone_page_state() 函数更新物理页对应分区可回收页的数量; 反之如果分配
时物理页不支持 SLAB_RECLAIM_ACCOUNT 标志，也就是物理页都是不可回收的，因此在
释放的时候，函数调用 sub_zone_page_state() 函数更新物理页对应分区不可回收页的
数量. 函数在 1671 行到 1675 行调用 \_\_ClearPageSlab() 函数清楚物理页的 PG_slab
标志。最后函数在 1678 行调用 free_pages() 函数将对应的物理页归还给 Buddy 分配器.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030048">obj_to_index</span>

![](/assets/PDB/RPI/TB000088.png)

obj_to_index() 函数用于获得缓存对象在 slab kmem_bufctl 数组中的索引。参数
cache 指向高速缓存，slab 参数执行那个对应的 slab，obj 参数指向缓存对象.

![](/assets/PDB/RPI/TB000008.png)

在 slab 中，slab 管理数据 kmem_bufctl 数组管理所有缓存对象的使用情况，struct
slab 管理数据的 s_mem 指向了缓存对象在 slab 中的起始地址. 函数 563 行通过减法
计算出缓存对象在 slab 缓存对象区的偏移，然后调用 reciprocal_divide() 函数计算
出了缓存对象在 kmem_bufctl 数组中的偏移.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030047">slab_put_obj</span>

![](/assets/PDB/RPI/TB000087.png)

slab_put_obj() 函数的作用是将一个缓存对象释放会 slab。参数 cachep 指向高速
缓存，slabp 参数指向 slab，参数 objp 指向缓存对象，参数 nodeid 指明了 NODE
信息.

![](/assets/PDB/RPI/TB000008.png)

在 slab 中，slab 使用 kmem_bufctl 数组管理每一个可用对象的使用情况，函数 2597 
行调用 obj_to_index() 函数获得释放缓存对象在 kmem_bufctl 数组中的索引, 然后
将对应 kmem_bufctl 对应成员的值设置为当前 slab 第一个可用缓存索引值，然后修改
slab 管理数据，将 slab 第一个可用缓存对象的索引 free 设置为 objp 对应的索引
值，最后将 inuse 减一，以此表示 slab 中正在使用的缓存对象减少一个.

> [obj_to_index](#D030048)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030045">\_\_cache_free</span>

![](/assets/PDB/RPI/TB000085.png)

\_\_cache_free() 函数用于释放一个缓存对象。参数 cachep 指向高速缓存，参数 objp
指向缓存对象.

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 维护一个高速缓存，SLAB 为了加速缓存对象的
分配，为每个 CPU 分配了本地高速缓存，通过 array 数组指向本地高速缓存。本地
高速缓存通过 struct cache_array 数据结构进行维护，其通过使用一个缓存栈维护
一定数量的缓存对象，当 SLAB 分配器分配或释放缓存对象时，不是直接与 slab 进行
交互，而是与本地高速缓存进行交互。

函数 3536 行调用 cpu_cache_get() 函数获得高速缓存对应的本地缓存。在本地高速
缓存栈上，struct array_cache 的 avail 指向缓存栈的栈顶，在将一个缓存对象释放
会本地缓存的时候，如果缓存栈中可用的缓存对象大于本地缓存容纳的最大可用缓存数
时，那么函数执行 3559 行到 3561 行的代码逻辑，调用 cache_flusharray() 函数将
一定数量的可用缓存对象归还给 slab，再将释放的缓存对象插入到缓存栈上; 反之如果
缓存栈上维护的可用缓存对象数量没有超过本地高速缓存维护的最大可用缓存对象数量，
那么函数直接将释放的缓存对象插入缓存栈的栈顶.

> [cache_flusharray](#D030046)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030044">kmem_cache_init_late</span>

![](/assets/PDB/RPI/TB000084.png)

kmem_cache_init_late() 函数用于完成 SLAB 分配器初始化后期. 在内核初始化到
一定阶段之后，内核已经支持 CPU 热插拔以及多 CPU 模式，此时 SLAB 更新所有
高速缓存的本地高速缓存. 

函数 1565 行为遍历 cache_chain 链表加上互斥锁 cache_chain_mutex. 1566 行函数
调用 list_for_each_entry() 函数遍历 cache_chain 链表上的所有高速缓存，并在
每次遍历过程中，调用 enable_cpucahe() 函数更新高速缓存的本地高速缓存。由于
该阶段内核已经支持 CPU 热插拔，那么有的 CPU 可以离线但本地高速缓存还没有释放，
那么 SLAB 分配器需要更新所有高速缓存的本地高速缓存，以此让离线 CPU 未释放的
本地高速缓存释放掉. 遍历完毕之后调用调用 mutex_unlock() 函数接触 cache_chain
的互斥锁。将 g_cpucache_up 设置为 FULL, 以此表示 SLAB 能完整使用.

> [enable_cpucache](#D030036)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030043">MAKE_ALL_LISTS</span>

![](/assets/PDB/RPI/TB000083.png)

MAKE_ALL_LISTS() 函数用于将高速缓存的 slab 链表的成员迁移到新的链表上。
参数 cachep 指向高速缓存，参数 ptr 指向新的链表，nodeid 参数用于指明 NODE 
信息. 在高速缓存中，为每个 NODE 维护了一套 slab 链表，slab 链表包含了 3 种链
表，分别是 slabs_partial、slabs_free 以及 slabs_full。函数通过调用 
MAKE_LIST() 函数将高速缓存原始的 slab 链表中的成员迁移到新的 slab 链表中.

> [MAKE_LIST](#D030042)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030042">MAKE_LIST</span>

![](/assets/PDB/RPI/TB000082.png)

MAKE_LIST() 函数用于初始化新的 slab 链表，并将原始高速缓存的指定 slab 链表
上的成员迁移到新的 slab 链表上. 参数 cachep 指向高速缓存，listp 指向新的链表，
slab 指明 slab 链表的类型，nodeid 参数用于指定 NODE 信息。

函数在 367 行初始化了 listp 对应的链表，然后调用 list_splice() 函数将高速缓存
nodeid 阶段对应的 slab 链表上的成员迁移到 listp 上面.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030041">init_list</span>

![](/assets/PDB/RPI/TB000081.png)

init_list() 用于创建一个新的 struct kmem_list3 数据结构，并将高速缓存原始的
kmem_list3 slab 链表数据迁移到新的 kmem_list3 上，并将高速缓存的 slab 链表
指向新的 kmem_list3。参数 cachep 指向高速缓存，参数 list 指向 slab 链表，参数
nodeid 指明 NODE 信息.

函数在 1342 行调用 kmalloc_node() 函数从指定的节点上分配内存，并在 1345 行将
高速缓存原始的 slab 链表信息迁移到 ptr 对应的内存上，然后初始化了对应的 
list_lock 锁，接着调用 MAKE_ALL_LISTS() 函数将 slab 链表上的数据全部迁移
到 ptr 对应的链表上，最后更新高速缓存的 slab 链表指向 ptr.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030040">kmem_cache_init</span>

![](/assets/PDB/RPI/TB000073.png)

kmem_cache_init() 函数用于初始化 SLAB 分配器. SLAB 分配器的初始化是一个 
"鸡和蛋" 问题，因此 SLAB 分配器为解决这个问题将初始化过程分作了几个阶段，
通过 g_cpucache_up 变量进行表示，g_cpucache_up 的取值如下:

![](/assets/PDB/RPI/TB000059.png)

当 g_cpucache_up 为 NONE 的时候，SLAB 分配器只能提供 cache_cache 一个高速
缓存，在这个阶段，SLAB 分配器需要通过 cache_cache 和静态数据构建 cache_array
高速缓存，当 cache_array 高速缓存构建完毕之后，SLAB 分配器进入 PARTIAL_AC
阶段，在该阶段中，SLAB 分配器基于已经存储的静态数据和几个高速缓存，构建
kmem_list3 高速缓存，该缓存构建完毕之后，SLAB 分配器位于 PARTIAL_L3 缓存。
此时 SLAB 基本可以使用 SLAB 分配器大部分功能，于是将之前使用的静态数据通过
高速缓存进行分配，并将静态数据迁移到新分配的内存，完成之后 SLAB 进入 EARLY
阶段，此时 SLAB 基本已经可以使用所有 SLAB 分配器功能，但此时由于高速缓存只能
外为 boot CPU 分配本地高速缓存。等内核进行调用 kmem_cache_init_late() 函数
对 SLAB 分配进行最后的初始化之后，SLAB 分配器进入 FULL 阶段，SLAB 分配器正式
初始化完毕.

![](/assets/PDB/RPI/TB000074.png)

SLAB 分配器初始化最早的阶段，函数调用 kmem_list3_init() 函数和 set_up_list3s()
函数为 cache_cache 高速缓存的创建构建 slab 链表. 此时 slab 链表相关的 kmem_list3
数据来自静态数据 initkmem_list3.

> [kmem_list3_init](#D030000)
>
> [set_up_list3s](#D030001)

![](/assets/PDB/RPI/TB000075.png)

SLAB 分配器首先初始化高速缓存全局链表 cahce_chain, 然后将 cache_cache 插入
到该链表上，然后 1426 行计算了 cache_cache 着色长度，并存储在高速缓存的 
colour_off 成员里，1427 行为 cache_cache 高速缓存设置本地高速缓存，其使用
静态数据 initarray_cache 提供的内存进行创建. 1428 行函数通过静态数据 
initkmem_list3 创建了 cache_cache 的 slab 链表. 1434 行计算了 struct kmem_cache
高速缓存的对象长度. 函数在 1439 行对高速缓存的对象长度进行了对齐操作.

![](/assets/PDB/RPI/TB000076.png)

函数在 1444 行使用 for 循环来确认 Buddy 分配器需要提供物理页的数量，以便
cache_cache 构建 slab。在每次循环中，函数调用 cache_estimate() 函数计算
cache_cache 高速缓存使用的 slab 占用物理页的数量，已经 slab 中维护缓存对象的
数量，最后还计算了 slab 浪费的内存长度。在每次循环中，如果当前物理页满足 slab
的构建要求，那么跳出循环。如果循环结束，Buddy 分配器提供的物理页还不能满足
cache_cache 的 slab 构建，那么 SLAB 直接调用 BUG_ON() 报错并提供初始化. 如果
找到 slab 构建的正确数据，那么将 slab 占用物理页的数据存储在 cache_cache 高速
缓存的 gfporder 成员里。1452 行函数通过 slab 浪费的内存除以着色长度，以此计算
了 cache_cache 的着色范围.最后将 slab 管理数据的长度存储在 cache_cache 的
slab_size 成员里.

> [cache_estimate](#D030002)

![](/assets/PDB/RPI/TB000077.png)

SLAB 分配器为了加速内核中通用的长度的内存分配速度，为系统提供了通用高速缓存，
这些通用高速缓存的对象的长度为 2 的幂次，这些高速缓存也称为匿名高速缓存。
SLAB 分配器初始化到图中的阶段时，通过计算 struct cache_array 的长度，建立对应
的通用高速缓存，通过调用 kmem_cache_create() 函数进行创建。函数在 1472 行进行
检测，用于判断 struct cache_array 与 struct kmem_list3 数据结构是否相同，如果
不同，那么 SLAB 分配器也会调用 kmem_cache_create() 函数为 struct kmem_list3
数据结构长度创建对应的通用高速缓存。创建完毕之后，将 slab_early_init 设置为 0.

> [kmem_cache_create](#D030039)

![](/assets/PDB/RPI/TB000078.png)

SLAB 分配器接下来为系统通用高速缓存创建对应的高速缓存，如果 CONFIG_ZONE_DMA
宏打开，那么也为 DMA 通用高速缓存创建高速缓存.

> [kmem_cache_create](#D030039)

![](/assets/PDB/RPI/TB000079.png)

SLAB 初始化到这个阶段，由于原始 cache_cache 的本地高速缓存通过静态数据
initarray_cache 进行维护。此时 struct cache_array 对应的通用高速缓存已经
可以使用，因此函数在 1514 行调用 kmalloc() 函数为 ptr 指针分配内存，并在 1517
行将 cache_cache 对应的本地高速缓存数据迁移到 ptr 对应的内存上，接着初始化了
ptr 对应的数据，其中包括 lock 锁的初始化，接着将 cache_cache 的本地高速缓存
指向了 ptr。函数继续调用 kmalloc() 分配内存，且 ptr 指向新分配的内存. 函数在
1530 行将 struct cache_array 对应的通用高速缓存的本地缓存数据迁移到 ptr 指向
的内存，然后将该通用高速缓存的本地缓存指向 ptr。至此 SLAB 分配器中原始的静态
本地缓存占用的内存全部替换成 SLAB 提供的内存.

> [kmalloc](#D030028)

![](/assets/PDB/RPI/TB000080.png)

SLAB 初始化到这个阶段，SLAB 分配器准备将静态的 slab 链表数据更新为 SLAB 提供
的内存. 函数在 1544 行调用 for_each_online_node() 函数遍历系统所有在线的 NODE，
在每次遍历中，函数将 cache_cache 高速缓存和 struct cache_array、
struct kmem_list3 对应的 slab 链表占用的内存全部替换成 SLAB 分配的内存。通过
init_list() 函数实现. 遍历完毕之后，SLAB 分配器将 g_cpucache_up 设置为 EARLY，
以此表示 SLAB 分配器的可以正常使用.

> [init_list](#D030041)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030039">kmem_cache_create</span>

![](/assets/PDB/RPI/TB000060.png)

kmem_cache_create() 函数用于创建一个高速缓存. 参数 name 指明高速缓存的名字;
参数 size 指明高速缓存对象的长度; 参数 align 指明高速缓存对象的对齐方式; 
参数 flags 包含了高速缓存创建标志; 参数 ctor 指向了高速缓存对象的构建函数.

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 表示一个高速缓存，其包含了高速缓存的基础信息，
其中也包含了高速缓存的本地缓存、共享缓存，以及 slab 链表等。高速缓存的创建过程
就是用于填充和创建高速缓存所需的数据.

![](/assets/PDB/RPI/TB000061.png)

函数首先进行基础的检测，其中包括高速缓存的名字是否存在、此时是否处在中断中、
高速缓存对象的长度是否小于 BYTES_PER_WORD 或者大于 KMALLOC_MAX_SIZE, 只要
以上条件其一满足，那么 SLAB 分配器认为这是非法的，直接内核 BUG.

![](/assets/PDB/RPI/TB000062.png)

函数 2128 行调用 slab_is_available() 函数检测当前的 SLAB 分配器是否可用，
如果可用，那么如果此时系统支持 CPU 热插拔，那么函数调用 get_online_cpus()
获得可用的 CPU，然后调用 mutex_lock() 上互斥锁. 函数在 2133 行处调用函数
list_for_each_entry() 函数遍历所有的高速缓存.

![](/assets/PDB/RPI/TB000010.png)

SLAB 分配器将所有的高速缓存维护在 cache_chain 链表上，然后函数遍历所有的
高速缓存，如果遍历到的高速缓存的名字与即将创建的高速缓存名字一致，那么 SLAB
分配器不允许名字相同的高速缓存存在，因此调用 dump_stack() 报错并跳转到 oops。

![](/assets/PDB/RPI/TB000063.png)

函数 2182 行到 2231 行，函数根据高速缓存创建时提供的标志进行对齐相关的操作，
最终会将对齐的结果存储在 ralign 变量里.

![](/assets/PDB/RPI/TB000064.png)

函数在 2240 行继续判断当前 SLAB 分配器已经正常使用，如果正常使用，那么 SLAB
分配器从 Buddy 分配器分配的物理内存使用 GFP_KERNEL 标志进行分配; 反之如果 
SLAB 分配器还不能完整使用，那么从 Buddy 分配器分配物理内存时使用 GFP_NOWAIT
标志，以便让分配不能失败.

![](/assets/PDB/RPI/TB000065.png)

函数 2246 行调用 kmem_cache_zalloc() 函数为高速缓存分配一个新的 
struct kmem_cache. 如果分配失败，则跳转到 oops。

> [kmem_cache_zalloc](#D030022)

![](/assets/PDB/RPI/TB000066.png)

函数 2287 行到 2293 行用于判断 slab 默认的管理数据是位于 slab 内部还是外部。
判断的条件之一是 slab_early_init 是否为零，这也说明了 SLAB 初始化完毕之前，
slab 的管理数据默认只能待在 slab 内部. 函数判断如果 slab 管理数据位于 slab
外部，必须满足这几个条件: 其一高速缓存对象的长度不小于 "PAGE_SIZE >> 3",
其二 SLAB 分配器已经过了基础初始化阶段，其三是 flags 标志不能包含 
SLAB_NOLEAKTRACE. 只有同时满足上面三个条件，那么 slab 的管理数据才能位于
slab 外部. 如果条件都满足了，那么函数将 CFLAGS_OFF_SLAB 标志添加到 flags 参数
里面.

![](/assets/PDB/RPI/TB000067.png)

函数 2295 行获得高速缓存对象对齐之后的长度，并在 2297 行调用 
calculate_slab_order() 函数计算出高速缓存每个 slab 维护缓存对象的数量，然后
计算每个 slab 占用物理页的数量，并计算出每个 slab 剩余的内存数量. 函数在
2306 行计算除了 slab 管理数据的长度，并存储在 slab_size 变量中.

> [calculate_slab_order](#D030023)

![](/assets/PDB/RPI/TB000068.png)

函数在 2313 行检测 slab 的管理数据是否位于 slab 外面，同时也检测 slab 剩余
的内存是否比 slab 管理数据大。如果高速缓存的 flags 参数包含了 CFLAGS_OFF_SLAB
标志，即表示想让 slab 管理数据位于 slab 外部，当如果此时 slab 管理数据的长度
小于等于 slab 剩余的内存，那么 SLAB 分配器不会让该高速缓存的 slab 管理数据
位于 slab 外部，因此将 CFLAGS_OFF_SLAB 标志从 flags 中移除，并将 slab 中剩余
的内存减去了 slab 管理数据的长度.

![](/assets/PDB/RPI/TB000021.png)

函数接着在 2318 行检测 slab 管理数据是否位于 slab 外部，如果此时 slab 管理数据
位于 slab 外部，那么 SLAB 分配器需要独立计算 slab 管理数据占用的内存数量，从
上图可以看出 slab 管理数据包括了 struct slab 以及 kmem_bufctl 数组，kmem_bufctl
数组的长度与 slab 中位于缓存对象数量有关.

![](/assets/PDB/RPI/TB000069.png)

函数 2333 行调用 cache_line_size() 计算出当前 cache line 的长度，以此作为高速
缓存着色的长度，存储在高速缓存的 colour_off 成员里，如果 cache line 的长度
小于之前计算出来的对齐长度，那么函数还是将高速缓存的着色长度设置为 align。
函数在 2337 行将之前获得的 slab 剩余的内存除以着色长度，以此获得高速缓存着色
范围，并存储在高速缓存的 colour 成员里. 接着在 2338 行将 slab 管理数据的长度
存储在高速缓存的 slab_size 成员里，2339 行将高速缓存的创建标志存在在 flags 
成员里，并在 2341 行到 2342 行计算高速缓存从 Buddy 分配器中获得物理页的标志.
2343 行将缓存对象的长度存储在高速缓存的 buffer_size 成员里.

![](/assets/PDB/RPI/TB000070.png)

函数在 2346 行检测 slab 管理数据是否位于 slab 外部，如果位于，那么函数调用
kmem_find_general_cachep() 函数根据 slab 管理数据的长度，从 SLAB 中获得一个
合适的通用高速缓存，并使用高速缓存的 slabp_cache 指向该通用高速缓存. 函数
2357 行设置了高速缓存的构造函数，2358 行设置了高速缓存的名字.

> [kmem_find_general_cachep](#D030025)

![](/assets/PDB/RPI/TB000071.png)

函数在 2360 行调用 setup_cpu_cache() 函数用于设置高速缓存的本地高速缓存、
共享高速缓存以及 slab 链表. 如果设置失败，那么函数调用 \_\_kmem_cache_destroy()
函数摧毁该高速缓存，并将高速缓存指针 cachep 设置为 NULL, 最后跳转到 oops.
如果分配成功，那么将高速缓存插入到系统高速缓存链表 cache_chain 里.

> [setup_cpu_cache](#D030037)

![](/assets/PDB/RPI/TB000072.png)

2368 行即为跳转 oops 处，函数首先判断高速缓存是否已经成功分配了，如果没有
成功分配且 flags 中包含了 SLAB_PANIC 标志，那么调用 panic() 函数. 函数
2372 行调用函数 slab_is_available() 函数检测是否 SLAB 分配器已经正常完整使用，
如果已经可以，那么函数接触 cache_chain_mutex 互斥锁，并调用 put_online_cpus()
函数。函数最后返回指向高速缓存的指针 cachep，并将该函数通过 EXPORT_SYMBOL()
导出给内核其他部分使用.

> [slab_is_available](#D030038)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030038">slab_is_available</span>

![](/assets/PDB/RPI/TB000058.png)

slab_is_available() 函数用于判断 SLAB 分配器是否可以正常使用。由于 SLAB 分配
器的初始化是一个 "鸡和蛋" 的问题，因此 SLAB 分配器将初始化分作几个阶段，通过
g_cpucache_up 进行描述，g_cpucache_up 可以取值:

![](/assets/PDB/RPI/TB000059.png)

NONE 阶段 SLAB 分配器只能分配 struct kmem_cache 对应的高速缓存，且功能不是
很完整; PARTIAL_AC 阶段 SLAB 分配器已经增加了 struct array_cache 对应的高速
缓存，因此高速函数可以为本地高速缓存分配内存了; PARTIAL_L3 阶段，SLAB 分配器
增加了 struct kmem_list3 的缓存，因此该阶段 kmalloc 等函数已经可以使用; EARLY
阶段，SLAB 分配器已经将分配器相关的所有数据都使用 SLAB 分配器分配内存，不再
使用静态数据; FULL 阶段 SLAB 分配器已经可以像一个正常的分配器为内核提供接口
进行内存分配和回收. 因此 SLAB 如果处于 EARLY 及其之后都表示 SLAB 已经准备好，
可以开始使用了.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030037">setup_cpu_cache</span>

![](/assets/PDB/RPI/TB000057.png)

setup_cpu_cache() 函数用于为高速缓存设置本地高速缓存. 参数 cachep 指向高速
缓存，参数 gfp 指向分配内存的标志.

![](/assets/PDB/RPI/TB000001.png)

在每个高速缓存中，SLAB 分配器为加速高速缓存的分配速度，为每个 CPU 创建了一个
本地高速缓存，其使用一个缓存栈维护一定数量的可用缓存对象. 由于 SLAB 分配器的
初始化分作多个阶段，每个阶段都涉及到为高速缓存创建本地高速缓存。SLAB 分配器
使用 g_cpucache_up 变量来标示 SLAB 分配器的初始化进度。函数 2028 行到 2045 行，
SLAB 分配器处于初始化的第一阶段，这个阶段 struct kmem_list3 和
struct cache_array 对应的高速缓存还没有创建，因此为该阶段的高速缓存分配本地
缓存只能使用静态变量 initarray_generic 对应的本地高速缓存，由于这个阶段只有
一个 CPU 在运行，因此函数在 2034 行设置了本地高速缓存. 并调用 set_up_list3s()
函数设置这个阶段的 kmem_list3 链表. 如果此时 INDEX_AC 的值与 INDEX_L3 的值
相等，那么函数将 SLAB 初始化进度设置为 PARTIAL_L3; 反之设置为 PARTIAL_AC。通过
源码分析可以知道，此时 INDEX_AC 与 INDEX_L3 不相等，因此将 SLAB 分配器初始化
进度设置为 PARTIAL_AC. 函数从 2048 行到 2063 行，SLAB 分配器初始化进度达到
第二阶段之后，函数使用这段代码为高速缓存分配本地高速缓存, 此时 kmalloc() 函数
已经可以使用，2047 行到 2048 行，函数调用 kmalloc() 函数为高速缓存分配内存，
此时系统还是只有一个 CPU，由于此时 SLAB 分配器初始化进度是 PARTIAL_AC，因此
函数在 2051 行将 SLAB 分配器初始化进度设置为 PARTIAL_L3; 当 SLAB 分配器初始化
进入第三阶段，此时系统中多个 CPU 已经可以使用，因此函数在 2055 行遍历所有的
在线 CPU，并调用 kmalloc_node() 函数为每个 CPU 分配本地高速缓存，并调用 
kmem_list3_init() 函数初始化高速缓存的 kmem_list3 链表. 对于 SLAB 分配器
初始化进度前三个阶段，SLAB 分配器将这些阶段的本地缓存维护的缓存对象控制在
BOOT_CPUCACHE_ENTRIES 个. 函数 2068 行到 2073 行初始化了基础的成员. SLAB
分配器初始化进度到第四阶段，即 g_cpucache_up 等于 FULL, 那么函数在 2026 行
调用 enable_cpucache() 函数初始化高速缓存的本地高速缓存、共享高速缓存，以及
kmem_list3 slab 链表.

> [enable_cpucache](#D030036)
>
> [set_up_list3s](#D030001)
>
> [kmem_list3_init](#D030000)
>
> [kmalloc](#D030028)
>
> [kmalloc_node](#D030030)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030036">enable_cpucache</span>

![](/assets/PDB/RPI/TB000056.png)

enable_cpucache() 函数用于计算一个高速缓存的本地缓存维护缓存对象的数量，以及
高速缓存对应的共享高速缓存维护缓存对象的数量，并为高速缓存分配本地缓存和 
kmem_list3 链表, 最后更新本地缓存.

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器为加快高速缓存的分配速度，为每个 CPU 创建了一个本地高速缓存，其使用
一个缓存栈维护一定数量的可用缓存对象，每个 CPU 都可以快速从缓存栈上分配可用
的缓存对象. SLAB 分配器还为高速缓存创建了一个共享高速缓存，所有 CPU 都可以从
共享高速缓存中获得可用的缓存对象。SLAB 分配器使用 kmem_list3 链表为高速缓存
在指定 NODE 上维护了 3 类链表，以此管理指定的 slab。enable_cpucache() 函数就是
用于为一个高速缓存规划本地高速缓存和共享高速缓存中维护缓存对象的数量，以及
创建并更新本地缓存，最后为高速缓存创建 kmem_list3 链表。

函数 3971 行到 3980 行用于计算一个高速缓存对应的本地高速缓存维护可用缓存对象
的数量，从代码逻辑可以推断高速缓存对象越大，本地高速缓存能够维护可用缓存对象
数量越少。3991 行到 3993 行，如果高速缓存对象的大小不大于 PAGE_SIZE, 且
num_possible_cpus() 数量大于 1，即 CPU 数量大于 1，那么将共享高速缓存维护的
缓存对象设置为 8. 最后函数在 4003 行处调用 do_tune_cpucache() 函数为高速缓存
创建了本地高速缓存和共享高速缓存，以及 kmem_list3 链表.

> [do_tune_cpucache](#D030033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030035">kzalloc</span>

![](/assets/PDB/RPI/TB000055.png)

kzalloc() 函数用于从 SLAB 分配器中获得指定大小的内存，并且内存内容已经被清空.
参数 size 指向分配内存的大小，flags 指明分配标志. 函数 321 行通过调用
kmalloc() 函数并传入 \_\_GFP_ZERO 标志实现.

> [kmalloc](#D030028)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030034">do_ccupdate_local</span>

![](/assets/PDB/RPI/TB000054.png)

do_ccupdate_local() 函数用于更新高速缓存对应的本地缓存。info 参数指向高速缓存
相关的信息. 在支持 CPU 热插拔的系统中，如果 CPU 离线但没有释放本高速地缓存，
那么函数会使用 struct ccupdate_struct 数据结构进行高速缓存的本地高速缓存释放.

3907 行获得 CPU 热插拔之前的本地高速缓存，3909 行到 3910 行，函数将新的本地
高速缓存与 CPU 热插拔之前的本地高速缓存进行交换. 交换之后，高速缓存使用了新的
本地高速缓存，而原始本地高速缓存已经存储在 struct ccupdate_struct 数据结构里.

> [struct ccupdate_struct](#D0108)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030033">do_tune_cpucache</span>

![](/assets/PDB/RPI/TB000052.png)

do_tune_cpucache() 函数用于为缓存分配本地缓存、共享缓存和 l3 链表. 参数
cachep 指向缓存，limit 用于指明本地缓存最大缓存对象的数量。shared 参数用于
指明共享缓存中缓存对象的数量; gfp 用于分配内存时候使用的标志.

函数 3920 行调用 kzalloc() 函数为 struct ccupdate_struct 分配新的内存。3924 
行到 3933 行，函数调用 for_each_online_cpu() 函数遍历所有的 CPU，并基于
struct ccupdate_struct 结构为 CPU 分配本地缓存结构 struct cache_array, 通过
alloc_arraycache() 函数进行分配. 函数 3936 行调用 on_each_cpu() 函数与
do_ccupdate_local() 函数更新当前高速缓存的本地缓存. 接着函数设置了高速缓存
的 batchcount, limit 以及 shared 成员. 函数 3943 行到 3951 行遍历所有在线的
CPU，如果发生了 CPU 热插拔事件之后，离线的 CPU 没有释放本地缓存，那么函数并
检测高速缓存对应的本地缓存是否存在，如果存在，那么释放本地缓存. 函数最后释放
new 变量，并调用 alloc_kmemlist() 函数为高速缓存分配 kmem_list3 链表数据.

> [kzalloc](#D030035)
>
> [alloc_arraycache](#D030031)
>
> [alloc_kmemlist](#D030032)
>
> [struct ccupdate_struct](#D0108)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030032">alloc_kmemlist</span>

![](/assets/PDB/RPI/TB000051.png)

alloc_kmemlist() 函数用于为缓存分配每个 NODE 的 kmem_list3 数据结构。参数
cachep 指向缓存，参数 gfp 指向分配内存使用的标志.

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 管理一个缓存，NUMA 架构中，每个缓存在每个 
NODE 上都维护着一个 kmem_list3 数据结构，因此该函数用于为缓存的每个 NODE 
的 struct kmem_list3 分配内存。函数 3818 行调用 for_each_online_node() 函数
遍历当前系统的所有 NODE，每次遍历过程中，函数 3820 行判断 use_alien_caches
是否为真. 函数 3827 行检测缓存是否支持共享缓存，如果支持，那么函数调用 
alloc_arraycache() 函数分配一个缓存栈，new_shared 指向该缓存栈; 如果缓存不
支持共享缓存，那么跳过 3828 到 3835 行的代码. 函数 3837 行处检测遍历到的
NODE struct kmem_list3 是否已经存在，如果存在，那么函数执行 3839 行开始的
代码逻辑，在这段代码逻辑中，函数首先获得原始的共享缓存，如果共享缓存存在，
那么调用 free_block() 函数将共享缓存内的缓存对象释放会 slab 中。然后将 shared
成员指向新的共享缓存，更新 struct kmem_list3 的 free_limit 成员; 如果缓存
不支持共享缓存，那么跳过这段代码. 函数 3859 行调用 kmalloc_node() 函数为
struct kmem_list3 分配内存，并在 3866 行调用 kmem_list3_init() 函数初始化
struct kmem_list3 数据结构，函数继续将 l3 的 shared 成员设置为 new_shared,
让其指向新的共享缓存. 并将缓存在该 NODE 下的 nodelist3 成员指向新分配的 l3.
函数继续循环遍历所有的 NODE，循环完毕之后，函数直接返回 0.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030031">alloc_arraycache</span>

![](/assets/PDB/RPI/TB000050.png)

alloc_arraycache() 函数用于为缓存分配缓存栈，并初始化缓存栈。参数 node 指明
NODE ID 信息，entries 指明缓存栈最多维护缓存对象的数量. 参数 batchcount 指明
缓存栈与 slab 交互缓存对象数量. 参数 gfp 用于指明分配内存使用的标志.

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器为了加入缓存对象的分配，为每个 CPU 设置了一个缓存栈，缓存栈里面
维护一定数量的缓存对象. SLAB 分配器使用 struct kmem_cache 维护一个缓存，其
成员 array 指向一个缓存栈，缓存栈使用 struct array_cache 进行维护，其 entry
成员是一个零数组，因此函数在 896 行为 entry 数组结合 entries 的数量规划了内存，
因此 memsize 参数就表示真实 struct array_cache 的长度。函数在 899 行调用 
kmalloc_node() 函数为 struct array_cache 分配内存，如果分配成功，那么将 avail
成员设置为 0，表示缓存栈没有可用缓存对象. limit 成员设置为 entries 表示缓存
栈最大容纳缓存数量; batchcount 成员表示当缓存栈没有可用缓存对象的时候从 slab
中获得缓存对象的数量. touched 成员设置为 0，表示缓存栈没有被使用过. 最后返回
缓存栈.

> [kmalloc_node](#D030030)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030030">kmalloc_node</span>

![](/assets/PDB/RPI/TB000049.png)

kmalloc_node() 函数用于从指定的 NODE 上分配指定长度的内存。参数 size 用于指明
分配内存的长度. node 参数指明 NODE 信息. 在 UMA 架构中，函数在 246 行直接调用
kmalloc() 函数分配所需的内存.

> [kmalloc](#D030028)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030029">kmem_cache_alloc_notrace</span>

![](/assets/PDB/RPI/TB000048.png)

kmem_cache_alloc_notrace() 函数用于从指定的高速缓存中分配一个可用的缓存对象，且
不带调用者信息。参数 cachep 指向高速缓存，flags 参数指明分配的标志. 函数在 120
行通过调用 kmem_cache_alloc() 函数分配可用的缓存对象.

> [kmem_cache_alloc](#D030021)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030028">kmalloc</span>

![](/assets/PDB/RPI/TB000047.png)

kmalloc() 函数用于分配指定长度的内存。参数 size 指明分配内存的长度，参数 flags
指明分配的标志. 函数在 133 行通过 \_\_builtin_constant_p() 函数判断 size
参数是否是一个常量，如果是，那么函数执行 134 行到 161 行代码逻辑, 在这段代码
逻辑中，函数首先判断 size 参数的合法性，然后遍历 SLAB 提供的系统常用缓存的
长度，以此找到第一个满足条件的长度，并将 cachep 指向该高速缓存。函数在 155 行
调用 kmem_cache_alloc_notrace() 函数从高速缓存中分配一个可用的缓存对象. 最后在
160 行返回缓存对象; 如果 size 参数不是常量，那么函数在 162 行出调用 
\_\_kmalloc() 函数分配指定长度的内存.

> [kmem_cache_alloc_notrace](#D030029)
>
> [\_\_kmalloc](#D030027)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030027">\_\_kmalloc</span>

![](/assets/PDB/RPI/TB000046.png)

\_\_kmalloc() 函数用于分配指定长度的内存. 参数 size 指明分配内存的长度，参数
flags 指明分配参数. 函数在 3719 行调用 \_\_do_kmalloc() 函数进行实际的内存
分配.

> [\_\_do_kmalloc](#D030026)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030026">\_\_do_kmalloc</span>

![](/assets/PDB/RPI/TB000045.png)

\_\_do_kmalloc() 函数用于分配指定长度的内存。size 指明内存的长度，flags 指明
分配标志，caller 为申请者. 当使用 \_\_do_kmalloc() 函数分配指定长度的内存，
SLAB 分配器会从 malloc_sizes 缓存数组中找到一个合适的缓存对象，并从缓存对象
中分配可用缓存对象.

函数 3704 行到 3706 行调用 \_\_find_general_cachep() 函数从 malloc_sizes 缓存
数组中找到 size 参数适合的缓存对象，然后 3707 行调用 \_\_cache_alloc() 函数
从缓存对象中分配可用的缓存对象，最后返回缓存对象对应的虚拟地址.

> [\_\_find_general_cachep](#D030024)
>
> [\_\_cache_alloc](#D030020)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030025">kmem_find_general_cachep</span>

![](/assets/PDB/RPI/TB000044.png)

kmem_find_general_cachep() 函数用于通过指定长度找到内核频繁使用的缓存对象。
参数 size 指明长度，gfpflags 指明与物理内存分配相关的标志. 739 行函数调用
\_\_find_general_cachep() 函数获得内核频繁使用的缓存对象.

> [\_\_find_general_cachep](#D030024)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030024">\_\_find_general_cachep</span>

![](/assets/PDB/RPI/TB000043.png)

\_\_find_general_cachep() 函数用于通过 size 参数找到 SLAB 分配器提供的频繁
使用的缓存对象。size 参数指向需求的长度，gfpflags 参数用于指明 Buddy 分配器
相关的分配标志. SLAB 分配器向内核提供了经常使用的缓存对象，这些缓存对象如下:

{% highlight c %}
# .name  .name_dma
size-32                 size-32(DMA)
size-64                 size-64(DMA)
size-96                 size-96(DMA)
size-128                size-128(DMA)
size-192                size-192(DMA)
size-256                size-256(DMA)
size-512                size-512(DMA)
size-1024               size-1024(DMA)
size-2048               size-2048(DMA)
size-4096               size-4096(DMA)
size-8192               size-8192(DMA)
size-16384              size-16384(DMA)
size-32768              size-32768(DMA)
size-65536              size-65536(DMA)
size-131072             size-131072(DMA)
size-262144             size-262144(DMA)
size-524288             size-524288(DMA)
size-1048576            size-1048576(DMA)
size-2097152            size-2097152(DMA)
size-4194304            size-4194304(DMA)
{% endhighlight %}

SLAB 分配器可以通过该函数为指定长度找到一个符号要求的缓存对象。这些缓存对象
使用 malloc_sizes 数组进行维护，因此函数 710 行首先将 csizep 指向了 
malloc_sizes 数组，然后在 722 行到 723 行循环中找到第一个满足长度是缓存对象，
最后返回缓存对象.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030023">calculate_slab_order</span>

![](/assets/PDB/RPI/TB000042.png)

calculate_slab_order() 函数用于计算缓存对象的每个 slab 占用的物理页数量，
并返回缓存对象的着色范围. 参数 cachep 指向缓存对象, 参数 size 指向每个缓存
对象的长度; 参数 align 指明缓存对象的对齐方式; 参数 flags 指向分配物理内存
的标志.

SLAB 分配器为缓存对象从 Buddy 分配器中分配一定数量的物理页之后，将这些内存组织
成 slab，slab 基本又两部分组成，第一部分就是 slab 的管理数据部分，第二部部分
又缓存对象组成. SLAB 分配器支持 slab 管理数据位于 slab 内部，也支持 slab 管理
数据位于 slab 外部.

![](/assets/PDB/RPI/TB000008.png)

上图中，slab 管理数据位于 slab 的内部，因此从 Buddy 分配器分配内存之后，将这些
内存的起始部分规划为着色预留区，紧接着是 slab 管理数据去，包括 struct slab 和
kmem_bufctl 数组，最后是又多个缓存对象组成的区域。calculate_slab_order() 函数
的作用就是确保分配的物理内存能够合理的布局三个区域，因此函数计算了各个区域的
数据。函数 1965 行使用 for 循环从一个物理页开始进行布局，在每次布局中，函数在
1969 行处调用 cache_estimate() 函数计算当前数量的物理页是否能合理布局三个区域，
并将可用缓存对象数量存储在 num 变量里，remiander 变量则存储布局完 slab 管理
区域和缓存对象区域之后，剩余的内存数量，这些内存将作为缓存对象着色使用. 1973
行到 1984 行用于处理 slab 管理数据位于 slab 外部的情况，如下图:

![](/assets/PDB/RPI/TB000021.png)

如果 flags 支持 CFLGS_OFF_SLAB 标志，那么 slab 管理数据位于 slab 外部，此时
函数计算 kmem_bufctl 数组的成员数量是否超过 slab 中缓存对象数量. 1987 行设置
了一个 slab 中缓存对象的数量. 1988 行设置了 slab 占用物理页的数量，left_over
变量用于存储 slab 中减去 slab 管理数据和缓存对象区域之后剩余的内存长度，最后
返回该长度.

> [cache_estimate](#D030002)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030022">kmem_cache_zalloc</span>

![](/assets/PDB/RPI/TB000041.png)

kmem_cache_zalloc() 函数用于从缓存对象中分配一个可用并且清零的缓存对象。参数
k 指向缓存对象，参数 flags 指向分配物理内存使用的标志. 311 行函数通过调用
kmem_cache_alloc() 函数并传入 \_\_GFP_ZERO 标志，以此分配一个可用缓存对象，
并将缓存对象对应的内存清零.

> [kmem_cache_alloc](#D030021)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030021">kmem_cache_alloc</span>

![](/assets/PDB/RPI/TB000040.png)

kmem_cache_alloc() 函数用于从缓存对象中分配一个可用的缓存对象. 参数 cachep
指向缓存对象; 参数 flags 用于指明分配物理内存标志. 3575 行函数通过调用
\_\_cache_alloc() 函数分配一个可用缓存对象. 最后返回缓存对象.

> [\_\_cache_alloc](#D030020)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030020">\_\_cache_alloc</span>

![](/assets/PDB/RPI/TB000039.png)

\_\_cache_alloc() 函数用于从缓存对象中分配可用的缓存对象. 参数 cachep 指向
缓存对象; 参数 flags 指向物理内存分配标志; 参数 caller 用于指向申请者.

3407 行函数检测了 flags 参数，以此符合 Buddy 分配器参数要求. 3416 行函数调用
\_\_do_cache_alloc() 函数分配可用的缓存对象，并调用 prefetchw() 函数将可用
缓存对象预加载到 CACHE。3426 行函数如果检测到 flags 参数中包含了 \_\_GFP_ZERO
标志，那么函数调用 memset() 函数将缓存对象指向的内存清零. 最后返回缓存对象.

> [\_\_do_cache_alloc](#D030019)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030019">\_\_do_cache_alloc</span>

![](/assets/PDB/RPI/TB000038.png)

\_\_do_cache_alloc() 函数用于从缓存对象中分配可用的缓存对象. cachep 参数指向
缓存对象, 参数 flags 用于分配物理内存的标志. 函数通过调用 \_\_\_\_cache_alloc()
函数进行分配.

> [\_\_\_\_cache_alloc](#D030018)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030018">\_\_\_\_cache_alloc</span>

![](/assets/PDB/RPI/TB000037.png)

\_\_\_\_cache_alloc() 函数用于从缓存对象中分配可用的缓存对象. 参数 cachep
指向缓存，参数 flags 指向缓存分配内存使用的标志.

![](/assets/PDB/RPI/TB000001.png)

在 SLAB 分配器里，为了加速缓存对象的分配，SLAB 分配器为缓存对象针对每个 CPU
构建了一个缓存栈，CPU 可以快速从缓存栈上获得可用的缓存对象。其实现原理如上图，
缓存对象使用 struct kmem_cache 进行维护，其成员 array 是一个 struct cache_array
数组，数组中的每个成员对应一个缓存栈，因此 struct cache_array 维护着一个缓存栈.

3114 行函数调用 cpu_cache_get() 函数获得缓存对应的缓存栈，3115 行函数检测缓存
栈的栈顶，以此判断缓存栈里是否有可用的缓存对象，如果栈顶不为零，即 avail 成员
不为 0，那么函数执行 3116 到 2118 行代码，直接从缓存栈上获得可用的缓存对象，
并更新缓存栈的栈顶; 如果栈顶为 0，那么表示缓存栈上没有可用的缓存对象，函数
执行 3120 到 3126 行代码，调用 cache_alloc_refill() 函数将一定数量的可用缓存
对象添加到缓存栈上，并再次使用 ac 变量指向缓存栈. 3136 行函数直接返回获得的
可用缓存对象.

> [cpu_cache_get](#D030004)
>
> [cache_alloc_refill](#D030017)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030017">cache_alloc_refill</span>

![](/assets/PDB/RPI/TB000036.png)

cache_alloc_refill() 函数用于为缓存对象的缓存栈填充可用的缓存对象。参数 cachep
指向缓存对象，参数 flags 用于分配物理内存的标志.

![](/assets/PDB/RPI/TB000001.png)

在 SLAB 分配器里，为了加速缓存对象的分配，SLAB 分配器为缓存对象针对每个 CPU
构建了一个缓存栈，CPU 可以快速从缓存栈上获得可用的缓存对象。其实现原理如上图，
缓存对象使用 struct kmem_cache 进行维护，其成员 array 是一个 struct cache_array
数组，数组中的每个成员对应一个缓存栈，因此 struct cache_array 维护着一个缓存栈，
在 struct cache_array 数据结构中，avail 成员指向缓存栈的栈顶, limit 成员指定
了缓存栈的缓存对象最大数量, 超过 limit 之后，缓存栈会将一定数量的缓存对象归还
给 slabs_partial 中的 slab; batchcount 成员用于指明从 slabs_partial 一次性分配
或向 slabs_partial 一次性释放缓存对象的数量; touched 成员用于表面该缓存栈是否
被访问过; entry 数组用于存储一定数量的可用缓存对象. 

每当 SLAB 分配缓存对象的时候，如果发现缓存栈上没有可用的缓存对象时候，那么 SLAB
分配器就会检测缓存对象的 slabs_partial 上是否有可用的 slab，如果有，那么 SLAB
从 slabs_partial 的 slab 上获取 batchcount 数量的缓存对象到缓存栈里; 如果
slabs_partial 上没有可用的 slab，那么 SLAB 分配器检测 slabs_free 上时候有可用
的 slab，如果有，那么就从对应的 slab 上获得 batchcount 数量的缓存对象到缓存栈
上; 如果此时 slabs_free 上也找不到 slab，那么 SLAB 分配器需要为缓存对象扩充新
的 slab，那么 SLAB 分配器从 Buddy 分配指定数量的物理内存，并将这些内存组织成
新的 slab，并插入到 slabs_free 链表上，并重新执行之前的查找过程. 重新查找的过程
中，slabs_free 链表上有可用的 slab，那么缓存栈获得 batchcount 数量的缓存对象
之后，将 slab 从 slabs_free 上移除并插入到 slabs_partial 链表上。

2945 行到 2961 行函数获得缓存对象对应的缓存栈，通过 ac 进行指定，函数还获得
缓存对象的 kmem_list3 链表，通过 l3 进行指定。2969 行函数检测缓存栈的 batchount
是否大于 0，以此确认缓存栈是否在使用，如果小于 1，那么缓存栈不再使用; 2973 行
到 2979 行，函数检测缓存的 slabs_partial 和 slabs_free 上是否有可用的 slab，
如果最终 slabs_free 上都没有找到可用的 slab，那么函数跳转到 must_grow 出分配
新的 slab; 如果找到可用 slab，那么函数继续执行代码。2981 行到 2990 行函数
找打从 slabs_partial/slabs_free 里找到一个可用的 slab. 2992 行到 2999 行，函数
使用一个 while 循环，只要确认缓存对象正在使用的缓存对象数量小于缓存对象总数，
并且 batchcount 不为 0，那么进入循环，在循环中，函数不断从 slab 中将可用缓存
对象加入到缓存栈里，并更新缓存栈的栈顶. 3003 行到 3007 行，函数将 slab 从原先
的链表中移除，如果此时 slab 中没有可用的缓存对象，那么将 slab 插到 slabs_full
链表中，如果此时 slab 中还有部分可用缓存对象，那么将 slab 插入到 slabs_partial
链表中.

3010 行到 3017 行，如果函数直接跳转到 must_grow, 那么 SLAB 会为缓存对象分配新
的 slab，由于 3010 行到 3015 行代码是公用代码，那么函数在 3015 行处继续检测
SLAB 分配器是否在为缓存对象分配新的 slab，如果缓存栈的栈顶为 0，那么 SLAB
分配器就为缓存对象分配新的 slab，此时在 3017 行调用 cache_grow() 函数进行分配
新的 slab，分配完毕之后，函数再次检测缓存栈是否等于 0，如果为真，那么函数重新
从缓存栈上分配新的可用缓存栈. 3027 行到 3028 行，当缓存栈被访问，那么缓存栈
的 touch 标记为 1，并且从缓存栈中返回一个可用缓存对象.

> [cpu_cache_get](#D030004)
>
> [slab_get_obj](#D030005)
>
> [cache_grow](#D030016)
>
> [struct kmem_cache](#D0103)
>
> [struct kmem_list3](#D0100)
>
> [struct slab](#D0102)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030016">cache_grow</span>

![](/assets/PDB/RPI/TB000035.png)

cache_grow() 函数用于扩充缓存对象的 slab。cachep 参数指向缓存对象; flags 参数
用于从 Buddy 分配器中分配内存的标志; nodeid 参数用于指明 NODE 信息; objp 用于
指向 slab.

![](/assets/PDB/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 管理缓存对象，每个缓存对象在指定的 NODE
上维护着 3 个链表，通过 nodelists 成员进行指定。缓存对象的 3 个链表通过
struct kmem_list3 数据结构进行维护，该结构维护 3 种链表，第一种链表维护着
包含部分可用缓存对象的 slab, 称之为 slabs_partial; 第二种链表维护缓存对象
全部可用的 slab, 称之为 slabs_free; 第三种链表维护的 slab 的所有缓存对象都
已经使用, 称之为 slabs_full. 每当 SLAB 分配缓存对象的时候，首先检查 
slabs_partial 上是否有可用的 slab，如果不存在，那么 SLAB 分配器继续查找
slabs_free 上是否有可用的 slab，如果此时还是没有可用的 slab，那么 SLAB 分配
器就会调用 cache_grow() 函数为缓存对象分配新的 slab.

SLAB 分配器在为缓存对象分配新的 slab 时，首先从 Buddy 分配器中分配指定数量的
物理页，然后将这些物理内存进行组织，将其转换成 slab，转换之后在将 slab 对应的
物理页与缓存对象和 slab 进行绑定，最后初始化 slab 的 kmem_bufctl 数组。新的 
slab 创建完毕之后，将其插入缓存对象的 slabs_free 链表。函数 2754 到 2755 行
对分配物理内存的标志进行检查，以此确认分配的物理页是否可以回收或者不可以回收.
2758 到 2767 行，函数获得缓存对象在指定 NODE 的 kmem_list3 数据结构，从中
获得了当前着色信息，并存储在 offset 变量里。函数更新了缓存对象的下一个着色
信息，如果下一个着色信息超过了缓存支持的着色范围，那么将下一个着色信息设置为 0.
2769 行通过获得的着色信息之后计算着色预留的内存长度。2786 到 2789 行，通过
调用函数 kmem_getpages() 函数，SLAB 分配器从 Buddy 分配器中分配指定数量的物
理内存，并获得物理内存对应的虚拟地址，存储中 objp 变量里。2792 行到 2795 行，
函数通过调用 alloc_slabmgt() 函数对新分配的内存组织成一个 slab，无论 slab 采用
外部管理数据还是内部管理数据，函数都返回了 slab 管理数据对应的 struct slab
指针。2797 行函数调用 slab_map_pages() 函数，将从 Buddy 分配器分配的物理页
与缓存对象和 slab 进行绑定。2799 行函数调用 cache_init_objs() 函数初始化 slab
的 kmem_bufctl 数组。2807 到 2810 行函数将 slab 加入到缓存对象的 slabs_free
链表里，并且添加缓存对象的 kmem_list3 的 free_objects 可用对象数量.

> [alloc_slabmgmt](#D030009)
>
> [kmem_getpages](#D030008)
>
> [slab_map_pages](#D030014)
>
> [cache_init_objs](#D030015)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030015">cache_init_objs</span>

![](/assets/PDB/RPI/TB000034.png)

cache_init_objs() 函数用于 slab 创建过程中，初始化 slab 管理数据的 kmem_bufctl
数组. 参数 cachep 指向缓存对象; 参数 slabp 指向 cache。

![](/assets/PDB/RPI/TB000008.png)

SLAB 分配器为缓存对象从 Buddy 分配器分配物理内存之后，将分配的内存通过一定
数据进行组织，将组织之后的数据称为 slab，slab 主要由两部分组成，一部分就是上
图中的管理数据，其包含了 struct slab 和 kmem_bufctl 数组; 另外一部分就是对象
缓存区，区间里被分成缓存对象大小的区块。slab 使用 kmem_bufctl 数组与缓存区
关联在一起，每个缓存对象区的对象都对应 kmem_bufctl 数组里的一个成员。kmem_bufctl
的成员用于指定对应缓存对象的下一个可用缓存对象。

因此 cache_init_objs() 函数就是用于 slab 初始化的时候，初始化 kmem_bufctl 数
组。2625 行开始到 2663 行，函数使用一个 for 循环，从 slab 的第一个缓存对象开始，
遍历 slab 包含的所有缓存对象. 2626 行，函数在每次遍历中，通过 index_to_obj() 
函数获得 kmem_bufctl 数组对应的缓存对象，存储在 objp 变量里。2662 行代码，首先
通过调用 slab_bufctl() 函数获得 slab 对应的 kmem_bufctl 数组，再通过 i 获得
kmem_bufctl 对应的成员，并将其设置为 "i+1"，以此表示当前缓存对象的下一个可用
缓存对象是其后一个缓存对象. 循环结束后，2664 行代码将 slab 的最后一个缓存对象
在 kmem_bufctl 中的成员设置为 BUFCTL_END, 以此表示缓存对象的末尾。通过上面的
处理，所有的缓存初始化为一条单链表，并以 BUFCTL_END 标记结尾.

> [index_to_obj](#D030006)
>
> [slab_bufctl](#D030007)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030014">slab_map_pages</span>

![](/assets/PDB/RPI/TB000033.png)

slab_map_pages() 函数用于将缓存对象使用的物理页与缓存对象和 slab 进行双向
绑定。参数 cache 指向缓存; 参数 slab 指向 slab; 参数 addr 指向物理页对应的
虚拟地址. SLAB 分配器为缓存对象从 Buddy 分配器获得可用的物理页之后，将这些
物理页标记为 PG_slab, 被标记的物理页就不会像非 PG_slab 的物理页一样，其中
物理页对应的 struct 的 lru 成员不再使用，lru 成员包含了两个指针 prev 和 next，
因此 SLAB 将 next 指向了缓存对象，而将 prev 指向了 slab，这样物理页、缓存对象
和 slab 就进行了双向绑定。缓存对象可以通过 list3 的三个链表找到指定的 slab，
slab 也可以直接通过 list3 的三个链表反向找到对应的缓存对象。slab 可以通过其管
理数据 struct slab 的 s_mem 找到缓存对象对应的虚拟地址，由于 slab 分配的内存
都是线性区的内存，因此可以通过 virt_to_page() 获得对应的物理页，物理页对应的
struct page 的 lru 成员又可以找到对应的缓存对象和 slab。

函数的 2723 行代码调用 virt_to_page() 获得 slab 使用的物理页，然后在 2731 到
2735 行代码用于将所有的物理页的 lru prev 和 next 指向缓存对象和 slab.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030013">page_set_slab</span>

![](/assets/PDB/RPI/TB000032.png)

page_set_slab() 函数用于将物理页与 slab 进行绑定。参数 page 指向物理页; 参数
slab 指向 slab。SLAB 分配器为缓存对象从 Buddy 分配器中分配物理内存之后，将这些
物理页标记为 PG_slab, 标记之后这些物理页就不能像普通物理页一样，物理页对应的
struct page 的 lru 成员不再使用，因此将 lru 的 prev 指向了 slab, 这样 slab 和
物理页就能双向绑定，因此代码 527 将 page lru 的 prev 指向了 slab 参数.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030012">page_get_slab</span>

![](/assets/PDB/RPI/TB000031.png)

page_get_slab() 函数用于获得 SLAB 物理页对应的 slab。参数 page 指向物理页。
SLAB 分配器为缓存对象从 Buddy 分配指定的物理页之后，将这些物理页标记为 PG_slab,
被标记之后就不能像普通物理页一样使用，那么物理页对应的 struct page 的 lru 
成员将不再使用，因此 SLAB 将这个 lru 的 prev 指向了物理页对应的 slab，这样
slab 和物理页就双向绑定. 因此函数的 533 行就是从 struct page lru 的 prev 指针
中获得了物理页对应的 slab。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030011">page_get_cache</span>

![](/assets/PDB/RPI/TB000030.png)

page_get_cache() 函数用于获得 SLAB 物理页对应的缓存对象。参数 page 指向物理
页。SLAB 分配为缓存对象从 Buddy 分配器获得物理内存之后，将这些物理页标记为
PG_slab, 标记完毕之后，物理页对应的 struct page 的 lru 成员将不再使用，那么
SLAB 将物理页 struct page lru 的 next 指向了缓存对象，这样缓存和物理页就双向
绑定，因此函数在 522 行通过 lru 的 next 指针获得了缓存对象.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030010">page_set_cache</span>

![](/assets/PDB/RPI/TB000029.png)

page_set_cache() 函数用于将 SLAB 物理页与缓存对象进行关联. 参数 page 指向
物理页; 参数 cache 指向缓存对象. 当 SLAB 分配器从 Buddy 分配器中分配了物理
页之后，将这些物理页标记为 PG_slab 之后，物理对应的 struct page 的 lru 成员
不在通过的物理页逻辑使用，于是将 struct page lru 成员的 next 指向缓存对象，
这样缓存对象可以找到 slab 使用的物理页，物理页也知道自己属于哪个缓存对象.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030009">alloc_slabmgmt</span>

![](/assets/PDB/RPI/TB000028.png)

alloc_slabmgmt() 函数用于将 Buddy 分配器中获得内存构建成一个 slab。参数 cachep
指向缓存对象; 参数 objp 指向从 Buddy 分配器中获得的内存; 参数 colour_off 参数
用于给新的 slab 进行着色; local_flags 用于存储 slab 构建标志; nodeid 参数用于
指定 NODE ID 信息.

![](/assets/PDB/RPI/TB000008.png)

SLAB 分配器从 Buddy 分配器中获得可用内存之后，将这些内存根据一定的规则组建成
slab，正如上图所示，slab 包含两部分，第一部分是 slab 管理数据，由 struct slab
数据结构和 kmem_bufctl 数组构成. 第二部分是缓存对象区域，多个缓存依次排列，
并且 kmem_bufctl 数组中的每个成员对应一个缓存对象，用于指明对应缓存对象的下一
个缓存对象在 kmem_bufctl 数组中的偏移.

![](/assets/PDB/RPI/TB000021.png)

slab 的管理数据也可以位于 slab 外部，如上图。但不论 slab 管理数据位于 slab
内部还是外部，其管理逻辑都是一致的. alloc_slabmgmt() 函数就是用于将从 Buddy
分配器分配的可用内存构建成以上其中一种 slab。2589 行，如果该缓存对象支持外部
slab 管理数据，那么 2591 行就调用 kmem_cache_alloc_node() 函数从 
struct kmem_cache 的 slabp_cache 缓存中分配 slab 管理数据所使用的缓存对象，
这里值得注意的是，如果缓存对象支持 slab 外部管理数据，那么缓存对象 struct
kmem_cache 的 slabp_cache 成员指向一个缓存，用于分配 slab 管理数据内存. 如果
缓存对象支持 slab 管理数据位于 slab 内部，那么 2604 和 2605 行从 Buddy 分配
器获得的可用内存中，将起始地址处预留作为着色使用，然后将 colour_off 指向缓存
对象区的起始偏移处. 2607 行将 slab 中已经分配的缓存对象设置为 0，即 inuse 设置
为 0，然后将缓存的着色信息设置为 colour_off, 接着将管理数据 struct slab 的 
s_mem 指向了缓存对象区的起始地址，最后将 free 设置为 0，以此表示第一个可用
缓存对象的偏移值为 0. 最后返回 slabp 的地址.

> [struct slab](#D0102)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030008">kmem_getpages</span>

![](/assets/PDB/RPI/TB000027.png)

kmem_getpages() 函数用于从 Buddy 分配器指定数量的物理页并返回物理页对应的
虚拟地址. 参数 cachep 指向缓存对象; 参数 flags 用于从 Buddy 分配器分配物理
页使用的分配标志; nodeid 用于指明从哪个 NODE 上分配.

函数 1624 行到 1626 行代码用于处理分配物理页时候使用的分配标志，如果缓存对象
支持 SLAB_RECLAIM_ACCOUNT, 即支持页回收，那么 flags 添加 \_\_GFP_RECLAIMABLE.
接着调用 alloc_pages_exact_node() 函数从指定的 NODE 分配 "1 << cachep->gfporder"
个物理页，如果分配失败，则直接返回 NULL. 1632 行计算出分配的物理页数量，存储
在 nr_pages 变量里. 如果缓存对象支持 SLAB_RECLAIM_ACCOUNT 标志，那么调用函数
add_zone_page_state() 统计物理页对应的 ZONE 分区可回收页的数量; 反之如果不支持
SLAB_RECLAIM_ACCOUNT 标志，那么调用 add_zone_page_state() 函数统计物理页对应的
ZONE 分区不可回收页的数量. 代码 1639 到 1640 行，将从 Buddy 分配器中获得的物理
页标记为 PG_slab, 以此告诉系统这些物理页属于 SLAB. 函数最后滴啊用 page_address()
函数返回物理页的起始虚拟地址.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030007">slab_bufctl</span>

![](/assets/PDB/RPI/TB000026.png)

slab_bufctl() 函数用于获得 slab 管理数据之一的 kmem_bufctl 数组. slab 参数
用于指向一个 slab。

![](/assets/PDB/RPI/TB000008.png)

![](/assets/PDB/RPI/TB000021.png)

无论 slab 将 slab 管理数据放置在 slab 内部还是 slab 外部，管理数据 struct slab
和 kmem_bufctl 数组都是相连在一起的. 因此通过 slab 可以获得 kmem_bufctl 数组。
函数 2617 行就是通过获得 struct slab 的下一个 struct slab 地址就是 kmem_bufctl
数组.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030006">index_to_obj</span>

![](/assets/PDB/RPI/TB000025.png)

index_to_obj() 函数用于通过 slab 的 kmem_bufctl 数组的索引获得 slab 中缓存对象。
参数 cache 指向缓存对象; 参数 slab 指向指定的 slab; 参数 idx 指向了缓存对象
在 slab kmem_bufctl 数组中的索引.

![](/assets/PDB/RPI/TB000007.png)

SLAB 分配器使用 struct slab 和 kmem_bufctl 数组管理 slab 中缓存对象，kmem_bufctl
数组中的每个成员对应一个缓存对象. slab 的管理数据 struct slab 的 s_mem 成员
用于指定缓存对象在 slab 中的起始地址，cache 参数的 buffer_size 成员指明了每个
缓存对象的长度，因此通过 s_mem 和 buffer_size 成员，以及 idx 是可以唯一确定
一个缓存对象，因此使用 index_to_obj() 函数可以通过一个 idx 索引获得一个缓存
对象.

> [struct slab](#D0102)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030005">slab_get_obj</span>

![](/assets/PDB/RPI/TB000024.png)

slab_get_obj() 函数用于从 slab 中获得可用的缓存对象. cachep 参数指向缓存;
参数 slabp 指向 slab; 参数 nodeid 指向 NODE ID. SLAB 分配器从 Buddy 获得可用
内存之后，将这些内存构建成 slab，并使用一定的数据管理 slab，slab 里面存在一定
量的可用缓存对象。slab 通过 struct slab 数据结构和 kmem_bufctl 数组进行管理，
如下图:

![](/assets/PDB/RPI/TB000008.png)

struct slab 和 kmem_bufctl 数组可以位于 slab 内部，也可以位于 slab 外部，上图
所示的 slab 管理数据位于 slab 内部. kmem_bufctl 数组的每个成员对应 slab 中的
一个缓存对象，用于标记对应缓存对象的下一个可用缓存对象在 kmem_bufctl 数组中的
偏移.

![](/assets/PDB/RPI/TB000007.png)

管理数据 struct slab 中，free 成员用于指明 slab 第一个可用缓存对象在 kmem_bufctl
数组中的偏移. 函数 2680 行调用 index_to_obj() 函数，通过 free 指向的索引获得
slab 中第一个可用的缓存对象, 存储在 objp 变量里，2683 行更新 struct slab 的 
inuse 成员，以此表示该 slab 中缓存对象又有一个被使用. 2684 行调用 slab_bufctl()
函数获得 free 对应缓存对象的下一个可用缓存对象，并将下一个可用缓存对象在 
kmem_bufctl 数组中的索引存储在 next 变量里. 2689 行将管理数据 struct slab 的
free 成员设置为 next 指向的值，以此更新该 slab 第一个可用缓存对象在 kmem_bufctl
数组中的偏移. 2691 行返回从 slab 中获得的缓存对象.

> [slab_bufctl](#D030007)
>
> [index_to_obj](#D030006)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030004">cpu_cache_get</span>

![](/assets/PDB/RPI/TB000023.png)

cpu_cache_get() 用于获得缓存对象的缓存栈。SLAB 分配器为每个 CPU 分配了缓存对象
的缓存栈，用于从缓存栈上快速获得可用的对象. 如下图:

![](/assets/PDB/RPI/TB000001.png)

每个 CPU 都有一个缓存对象的缓存栈，缓存通过 struct kmem_cache 数据结构进行
管理，array 成员是一个数组结构，每个 CPU 对应数组中的一个成员，通过 array
数组就可以找到 CPU 对应的缓存栈，因此 704 行代码，函数通过 smp_processor_id()
函数获得当前 CPU 的 ID，然后从 struct kmem_cache 的 array 数组中获得指定的 
struct array_cache 数据结构，从而获得缓存栈.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030003">slab_mgmt_size</span>

![](/assets/PDB/RPI/TB000022.png)

slab_mgmt_size() 函数用于计数一个缓存对象的 slab 管理数据长度。nr_objs 参数
指明了缓存对象的数量; 参数 align 指明缓存对象的对齐方式.

SLAB 分配器从 Buddy 分配器分配可用内存之后，将其规划为 slab，slab 中包含了多个
缓存对象。slab 是一定的管理数据管理这些缓存对象的分配和释放，slab 使用一个 
struct slab 数据结构和一个 kmem_bufctl 数组进行管理，其中 kmem_bufctl 中的每个
成员正好对应 slab 中一个缓存对象，因此 kmem_bufctl 数组的长度由 slab 中缓存对象
的数量而定. 因此 slab 管理数据的长度由 struct slab 的长度和 kmem_bufctl 数组
长度决定，最后如 744 行定义的算法一致. 

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030002">cache_estimate</span>

![](/assets/PDB/RPI/TB000020.png)

cache_estimate() 函数用于计算从 Buddy 分配器中获得指定长度内存之后，这些
内存一共可以分配多少个可用对象，以及可以支持多少中着色. 参数 gfporder 指向了
从 Buddy 分配器中获得物理页的个数; buffer_size 指明了缓存对象的长度; align
指明了缓存对象的对齐方式; flags 存储缓存对象相关的 SLAB 标志; left_over 用于
指明分配的内存减去可用对象占用的内存之后，剩余的内存长度; num 用于存储指定
长度的内存可分配对象的数量.

SLAB 分配器在分配每个缓存对象的时候，首先从 Buddy 分配器中获得可用的内存，
然后通过一定的组织方式将这些内存组织成为 slab，slab 是 SLAB 管理的单元，slab
里面包含了多个缓存对象。SLAB 分配器采用 slab 管理数据管理每个 slab，但由于
slab 管理数据支持 slab 内部管理，也支持 slab 外部管理，如下图:

![](/assets/PDB/RPI/TB000008.png)

![](/assets/PDB/RPI/TB000021.png)

无论采用那种方式管理 slab，slab 最前端都是为着色预留的内存空间，其后是多个
缓存对象，但是由于从 Buddy 分配器分配的内存都是按 PAGE_SIZE 分配的，那么以上
的结构可能会在 slab 的结尾处剩余一部分的内存区域，这里称为 Left_Over, 因此
系统会调用 cache_estimate() 函数计算出两种模式下，指定长度的内存可以划分多少
个缓存对象，以及剩余的内存长度.

756 行首先计算了从 Buddy 中获得内存的长度，存储在 slab_size 变量里。当该缓存
对象分配时支持 CFLGS_OFF_SLAB 标志，那么 slab 的管理数据将位于 slab 之外，
那么函数执行 773 行到 778 行逻辑，因此 slab 管理数据位于 slab 之外，那么
mgmt_size 的值为 0，接着计算这段内存最大容纳对象的数量，如果计算的结果超出
SLAB 分配器规定的最大缓存数量，那么将 nr_objs 设置为 SLAB_LIMIT; 如果分配
该缓存对象时不支持 CFLGS_OFF_SLAB 标志，那么 slab 管理数据位于 slab 内部，
那么在计算最大缓存对象数量的时候，也要从可用内存中减去 slab 管理数据占用的
长度。slab 管理数据包含了 struct slab 以及 kmem_bufctl 数组，因此 788 行到
789 行处计算了可分配的缓存对象数量。795 行到 797 行检测了在计算出的可分配
对象占用的内存和管理数据占用的内存是否大于从 Buddy 分配器分配的内存，如果
大于，那么将可分配缓存对象数减一. 799 到 800 行同样检测可分配缓存对象数量是否
查过 SLAB 分配器规定的最大数量, 如果超过，那么将可分配缓存对象数量设置为
SLAB_LIMIT。最后在这种模式下，将 slab 管理数据的长度存储在 mgmt_size 变量.
函数最后将可分配缓存对象数量存储在 num 变量里，并计数出剩余内存的长度，存储
在 left_over 变量里.

> [slab_mgmt_size](#D030003)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0108">struct ccupdate_struct</span>

![](/assets/PDB/RPI/TB000053.png)

struct ccupdate_struct 数据结构用于更新高速缓存对应的本地缓存.

![](/assets/PDB/RPI/TB000001.png)

每个高速缓存使用 struct kmem_cache 数据结构进行维护，为了加速每个 CPU 从高速
缓存中分配可用的缓存对象，SLAB 分配器为每个 CPU 创建了本地缓存，使用
struct cache_array 数据结构进行维护. 本地缓存中维护了一个缓存栈，CPU 可以从
这个缓存栈上快速获得可用的缓存对象.

在支持 CPU 热插拔系统里，如果 CPU 离线了但没有释放本地缓存，那么 SLAB 使用
struct ccupdate_struct 数据结构协助高速缓存更新 CPU 本地缓存. 因此 cachep 
成员用于指向需要更新的高速缓存，new 成员用于指向高速缓存新的本地缓存.

> [do_tune_cpucache](#D030033)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0107">struct arraycache_init</span>

![](/assets/PDB/RPI/TB000017.png)

SLAB 分配器初始化阶段，出现了 "鸡生蛋" 问题，为了解决这个问题，SLAB 分配器
使用 struct arraycache_init 结构静态定义了 cache_array 缓存栈提供了 SLAB 分配
器初始化使用.

![](/assets/PDB/RPI/TB000018.png)

SLAB 分配器定义了 initarray_cache 数组用于给 struct kmem_list3 和 
struct cache_array 缓存对象提供了初始化所需的 cache_array 缓存栈.

> [struct array_cache](#D0101)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0106">kmem_bufctl_t</span>

![](/assets/PDB/RPI/TB000016.png)

SLAB 分配器从 Buddy 分配器获得指定数量的物理页之后，着色完毕之后并使用 
struct slab 管理这些内存，称为 slab. 其结构如下:

![](/assets/PDB/RPI/TB000008.png)

在每个 slab 中，起始处是为着色预留的区间，接下来是 struct slab 结构，该结构
就是用于管理 slab 的数据，紧接着就是一个 kmem_bufctl 数组，该数组是由 
kmem_bufctl_t 类型的成员组成的数组。接下来是对象区，这就是用于分配的对象。
在 kmem_bufctl 数组中，每个成员按顺序与每个对象进行绑定，用于表示该对象的下
一个对象的索引，并且最后一个对象的值为 BUFCTL_END, 这样将 slab 中的所有对象
通过一个单链表进行维护。通过上面的分析，kmem_bufctl_t 数据结构构造的数据用于
指向当前对象指向的下一个对象的索引.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0105">struct cache_names</span>

![](/assets/PDB/RPI/TB000014.png)

SLAB 分配器为内核中频繁使用的特定长度分配缓存对象，由于缓存对象是有名的，
因此使用 struct cache_names 用于指明缓存对象的名字. name 成员用于指明缓存
对象的名字，name_dma 成员用于指明缓存对象 DMA 部分的名字.

![](/assets/PDB/RPI/TB000015.png)

SLAB 分配器通过上面的代码定义了多种长度的名字，包含了 DMA 部分的名字，通过
上面的定义，SLAB 为了特定长度定义的名字如下:

{% highlight c %}
# .name  .name_dma
size-32		        size-32(DMA)
size-64		        size-64(DMA)
size-96		        size-96(DMA)
size-128		size-128(DMA)
size-192		size-192(DMA)
size-256		size-256(DMA)
size-512		size-512(DMA)
size-1024		size-1024(DMA)
size-2048		size-2048(DMA)
size-4096		size-4096(DMA)
size-8192		size-8192(DMA)
size-16384		size-16384(DMA)
size-32768		size-32768(DMA)
size-65536		size-65536(DMA)
size-131072		size-131072(DMA)
size-262144		size-262144(DMA)
size-524288		size-524288(DMA)
size-1048576		size-1048576(DMA)
size-2097152		size-2097152(DMA)
size-4194304		size-4194304(DMA)
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0104">struct cache_sizes</span>

![](/assets/PDB/RPI/TB000011.png)

SLAB 分配为系统中经常使用的特定长度分配缓存对象，使用 struct cache_sizes 进行
定义。在结构中 cs_size 成员用于指定缓存对象的长度，成员 cs_cachep 用于指向指定
长度缓存的 struct kmem_cache 指针. 如果系统支持 CONFIG_ZONE_DMA 宏，即支持
ZONE_DMA 分区，那么 DMA 分区提供了对应的缓存对象.

![](/assets/PDB/RPI/TB000012.png)

内核使用上面的代码，定义了内核经常使用的特定长度分配内存对象，在上面定义中，
通过 kmalloc_sizes.h 头文件进行转换:

![](/assets/PDB/RPI/TB000013.png)

上图即 kmalloc_sizes.h 头文件定义，通过上面的转换，内核支持经常使用指定长度
缓存对象如下:

{% highlight c %}
# SIZE
size-32
size-64
size-96
size-128
size-192
size-256
size-512
size-1024
size-2048
size-4096
size-8192
size-16384
size-32768
size-65536
size-131072
size-262144
size-524288
size-1048576
size-2097152
size-4194304
{% endhighlight %}

> [struct kmem_cache](#D0103)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0103">struct kmem_cache</span>

![](/assets/PDB/RPI/TB000009.png)

SLAB 分配器使用 struct kmem_cache 结构管理一个高速缓存. SLAB 分配器从 Buddy
分配器中分配器内存之后，使用 struct slab 管理这些内存，并称为 slab，SLAB 分配器
将 slab 维护在对应的 slabs_partial、slabs_free 或 slabs_full 链表中。slab
中存在可用的对象，但 SLAB 分配器为了加速每个 CPU 获得对象的速率，为对象针对
每个 CPU 创建了一个缓存栈，缓存栈使用 struct cache_array 进行管理，SLAB 分配器
平时从 slab 中将一定数量的可用对象加入到每个 CPU 对应的缓存栈上，那么当对应
CPU 分配对象的时候，那么 SLAB 直接从缓存栈上分配对象，反之当 CPU 使用完对象
之后，将对象直接归还给缓存栈，并不直接返回给 slab，这样有利于局部性原理，使
经常使用的对象不被换出 CACHE。为了增加对象在 CACHE 中的命中率，SLAB 为每个
slab 进行着色，这回让每个对象缓存到不同的 CACHE LINE 上，其架构如下:

![](/assets/PDB/RPI/TB000001.png)

array 成员用于为每个 CPU 指向各自的本地高速缓存 struct cache_array, CPU 分配对象
的时候通过该成员找到指定的缓存栈进行对象的分配和释放动作. buffer_size 用于
指明对象对齐之后的长度, 由于是对齐之后的结果，因此和对象原始长度可能存在差异;
flags 用于指明对象在 SLAB 分配器中的标志; num 成员用于指明每个 slab 上对象
的数量; gfporder 用于指明该缓存对象从 Buddy 分配器分配物理页的数量; gfpflags
用于在从 Buddy 分配器中分配物理页的分配标志; colour 用于指明该缓存对象最大
可着色范围; colour_off 成员用于指定每次着色偏移; slab_size 成员用于指定管理
数据在 slab 的长度; ctor 用于分配对象时候调用的构造函数; name 成员用于指明
缓存对象的名字; next 用于将缓存对象添加到系统的缓存链表 cache_chain 上.

![](/assets/PDB/RPI/TB000010.png)

nodelists 成员是 SLAB 分配器用于为每个 NODE 创建的 kmem_list3, 在 kmem_list3
中一共维护三种链表，分别是 slabs_partial 链表用于维护包含部分可用对象的 slab，
slabs_free 链表用于维护全部对象可用的 slab，slabs_full 链表用于维护全部对象
已经分配的 slab. SLAB 分配器为了让缓存对象从 Buddy 分配器分配的物理内存来自
同一 NODE，因此为每个 NODE 都创建了 kmem_list3 进行维护.

> [struct array_cache](#D0101)
>
> [struct kmem_list3](#D0100)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0102">struct slab</span>

![](/assets/PDB/RPI/TB000006.png)

在 SLAB 分配器中，SLAB 分配器使用 struct kmem_cache 管理一个高速缓存，SLAB
分配器从 Buddy 分配器中为对象分配内存之后，使用 struct slab 管理这些内存，
将内存分作两部分，第一部分是管理部分，第二部分就是可用缓存对象. 如下图:

![](/assets/PDB/RPI/TB000008.png)

SLAB 分配器从 Buddy 分配器中以 PAGE_SIZE 为单位分配一定数量的内存之后，SLAB
分配器对该内存进行着色，也就是在这段内存的起始地址预留一段内存，在这段内存
之后是用于管理这段内存的 struct slab 结构, struct slab 结构包含了管理这段
内存的数据，其之后是一个 kmem_bufctl 数组，数组之后的内存，SLAB 分配器将其
分成对象大小的内存块，这些内存块就是该对象缓存。kmem_bufctl 数组中的每个成员
对应着一个缓存，用于指定该缓存的下一个缓存索引，这样就可以将这些缓存链接成
一个单链表，kmem_bufctl 数组的最后一个成员标记为 BUFCTL_END, 用于指示这段
内存的最后一个对象. 通过这样管理的内存在 SLAB 分配器中称为 slab, 具体如下图:

![](/assets/PDB/RPI/TB000007.png)

在 SLAB 分配器中，struct slab 可以位于 slab 内部，也可以位于 slab 外部，但
不论位于 slab 内部或外部，其成员的含义都是一致的。colouroff 成员用于指明该
slab 着色总长度，其包含了着色、struct slab 以及 kmem_bufctl array 的总长度，
这样可用对象加载到 CACHE 之后就会被插入到不同的 CACHE 行; s_mem 用于指向
slab 中对象的起始地址; inuse 成员用于指定 slab 中可用对象的数量; free 成员
用于指向 slab 当前可用对象在 kmem_bufctl 的索引; nodeid 成员用于指向该 slab
位于哪个 NODE; list 成员用于将 slab 加入到 struct kmem_cache 的 kmem_list3
中三个链表中的其中一个链表，如下图:

![](/assets/PDB/RPI/TB000001.png)

如果 slab 中的所有对象都是可用的，那么 slab 会被加入到 slabs_free 链表中;
如果 slab 中的所有对象都是不可用的，那么 slab 会被加入到 slabs_full 链表中;
如果 slab 中的对象部分可用，那么 slab 会被加入到 slab_partial 链表中.
slab 管理数据也可以位于 slab 外部，如下图:

![](/assets/PDB/RPI/TB000021.png)

在 SLAB 分配器中，分配对象时如果添加了 CFLGS_OFF_SLAB 标志，那么 SLAB 分配器
会将从 Buddy 分配器获得的内存制作成 slab，并将 slab 的管理数据独立与 slab。
将从 Buddy 分配器中获得的内存进行着色，着色值存储在 slab 管理数据的 colouroff
里，然后使用 slab 管理数据的 s_mem 指向 slab 对象的起始地址。kmem_bufctl 数组
对应着 slab 中的每个对象。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0101">struct array_cache</span>

![](/assets/PDB/RPI/TB000004.png)

SLAB 分配器为了加速从高速缓存中分配缓存对象，高速缓存为每个 CPU 创建了本地高
速缓存，通过使用 struct array_cache 结构进行维护。本地高速缓存通过一个缓存栈
维护一定数量的可用缓存对象。每个 CPU 都可以从各自的缓存栈上快速获得可用的缓存
对象, 如下图:

![](/assets/PDB/RPI/TB000001.png)

在上图中，CPU 要从 SLAB 分配器中获得可用的对象，那么 SLAB 分配器会从对应的
struct kmem_cache array 成员中找到指定的 struct array_cache 结构，在该结构
中，avail 成员用于指定该缓存栈中存在多少个可用的对象; limit 成员用于指定该
缓存栈最多可以容纳的对象数量; batchcount 成员用于指定该缓存栈从 slabs_partial
的 slab 获得或释放对象的数量; toched 成员用于指定该缓存栈是否被访问过; lock
成员用于操作该缓存栈添加锁; entry 成员是一个零数组，数组构成了一个 LIFO 的
缓存栈，栈上的每个成员就是一个可用的对象，并且 avail 用于维护栈顶. 对于 SMP
结构，存在多个 CPU，那么 struct kmem_cache 维护多个 struct array_cache，如下图:

![](/assets/PDB/RPI/TB000005.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0100">struct kmem_list3</span>

![](/assets/PDB/RPI/TB000002.png)

SLAB 分配器使用 struct kmem_cache 维护一个高速缓存, 其中包含了成员
nodelist_list3, 其就是一个 struct kmem_list3 结构。SLAB 分配器为高速缓存
在每个 NODE 上都分配一个 struct kmem_list3 结构，struct kmem_list3 结构
在每个 NODE 上维护三个链表，每个链表上都维护一定数量的 slab. 如下图:

![](/assets/PDB/RPI/TB000001.png)

slabs_partial 成员用于维护一个链表，链表上的成员是包含部分可用对象的 slab，
SLAB 分配器可以从链表的成员中获得可用的对象; slabs_full 成员同样用于维护一
个链表，链表上的成员的 slab 上的所有对象全部被分配，SLAB 分配器不会从这些
slab 中查找可用的对象; slabs_free 成员也用于维护一个链表，链表上的 slab 成员
的对象全部可以使用，SLAB 在 slabs_partial 链表上找不到可用的对象之后，就从
slabs_free 链表上查找一个 slab，并从该 slab 中查找一个可用的对象返回给对象，
最后将该 slab 加入到 slabs_partial 链表上.

free_objects 成员用于指定该 NODE 上所有可用对象的数量. free_limit 成员
用于指定该 NODE 上，三个链表维护最大可用对象的数量，当可用对象数量超过
free_limit 之后，SLAB 分配器将 slabs_free 上的 slab 释放给 Buddy 分配器.
colour_next 成员用于为每个 slab 进行着色. shared 成员维护一个共享高速
缓存，用于与多个本地高速缓存交换可用的缓存对象。如下图:

![](/assets/PDB/RPI/TB000003.png)

每当 SLAB 分配器从 Buddy 分配器中获得可用的内存之后，SLAB 分配器会在这段内存
的起始地址处预留 colour_next 的空间作为 SLAB 着色。每当完成一次着色之后，
colour_next 将会增加 struct kmem_cache colour_off 成员对应的长度，这样就
能将不同的 slab 着上不同的色. list_lock 成员是一个 spinlock 锁，由于对 
struct kmem_list3 操作时进行上锁操作.

> [struct array_cache](#D0101)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030000">kmem_list3_init</span>

![](/assets/PDB/RPI/TB000000.png)

kmem_list3_init() 函数用于初始化 struct kmem_list3 结构实例。在 SLAB 中，
struct kmem_cache 结构中包含了 nodelists 成员，该成员为每个 NODE 指向一个
struct kmem_list3 结构。

![](/assets/PDB/RPI/TB000001.png)

如上图所示，struct kmem_cache 为每个 NODE 准备了一个 struct kmem_list3 结构，
在 struct kmem_list3 结构中，包含了 3 个链表，每个链表维护一定数量的 SLAB。
slab_partial 链表上维护的 SLAB 中包含了一定可用数量的 object; slab_full 链表
上维护的 SLAB 中没有可用的 object; slab_free 链表上维护的 SLAB 中所有的
object 都是可用的. 

> [struct kmem_list3](#D0100)

函数中，343 行到 345 行分别初始化了 struct kmem_list3 结构体中 3 个链表.
346 行和 347 行初始化了 shared 成员和 alien 成员; 348 行初始化了 colour_next
成员，因此着色从 0 开始; 349 行初始化了 list_lock spinlock 锁; 350 行和 351 行
将 free_objects 成员设置为 0，以此表示当前没有可用的对象，free_touch 成员
设置为 0 表示 struct kmem_list3 未被访问过.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030001">set_up_list3s</span>

![](/assets/PDB/RPI/TB000019.png)

SLAB 分配器通过 struct kmem_cache 管理缓存对象，其中包含了成员 nodelists 
成员，该成员用于为系统每个 NODE 分配一个 struct kmem_list3 结构，在 struct
kmem_list3 结构中维护来自当前 NODE 的物理页构成的 slab，因此 set_up_list3s()
函数用于设置缓存对象每个 NODE 的 kmem_list3 结构.

参数 cachep 指向缓存对象的 struct kmem_cache; 参数 index 指向 NODE 偏移.
1363 行到 1368 行用于遍历当前系统所支持的 NODE 节点，在每次遍历中，将
nodelists 成员设置为 initkmem_list3 数组中的特定 struct kmem_list3 成员.
由于该函数只用于 SLAB 分配器初始化阶段，因此标记上 \_\_init.

> [struct kmem_cache](#D0103)
>
> [struct kmem_list3](#D0100)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
