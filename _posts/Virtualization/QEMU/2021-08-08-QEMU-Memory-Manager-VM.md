---
layout: post
title:  "QEMU Memory Manager for Virtual Machine"
date:   2021-08-08 12:00:00 +0800
categories: [HW]
excerpt: Memory Manager on QEMU.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [Start to Why](#A)
>
> - [Overview](#B)
>
> - [QEMU Memory Manager 核心数据结构](#C)
>
>   - [MemorySection](#C1)


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Start to Why (十万个不为什么)

> QEMU 的内存管理是管理自己还是虚拟机?
>
> QEMU 分配的内存到底是 HVA 还是 GPA?
>
> MemoryRegion 到底是用来管理 HVA 还是 GPA 的?
>
> 为啥有了 AddressSpace 树状结构的内存拓扑，还非得弄一个 FlatView 干啥?
>
> MemoryRegion 和 MemoryRegionSection 有啥区别?
>
> FlatView 用来干啥的? 为什么内存需要平坦?
>
> QEMU 有好多数据管理结构，它们是如何相互联系在一起的?

----------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### Overview

----------------------------------

<span id="C1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

#### MemoryRegion

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000885.png)

MemoryRegion 作为内存模拟中的核心数据结构，不仅用来表示虚拟机的一段物理内存区域，也用于搭建 GPA 与 RamBlock(HVA) 之间的桥梁。QEMU 中使用 AddressSpace 数据结构来表示 Guest 中 CPU/设备能够看到的内存，其中 address_space_memory 用来表示 Guest 中的物理内存, 另外 MemoryRegion 与 MemoryRegion 之间通过 subregions/subregions_link 双链表构成了树状的拓扑结构, 那么 AddressSpace 的 root 指针指向的根节点的 MemoryRegion，根节点的 MemoryRegion 相当于虚拟机的 "内存总线", 树状的叶子节点对应的 MemoryRegion 是实际分配给虚拟机的 "物理内存区域"。另外 MemoryRegion 的 alias 指向的 MemoryRegion 称为指向者的 "别名"，"别名" MemoryRegion 相当于虚拟机的 "内存控制器"，其管理实际的内存。每个 "物理内存区域" MemoryRegion 的 container 成员指向其父节点, 这样父子关系就得到明确.

> MemoryRegion 基础知识
>
> - [MemoryRegion 与 GPA 之间的关系](#C100)
>
> - [MemoryRegion 与 HVA 之间的关系](#C101)
>
> - [MemoryRegion 与 HPA 之间的关系](#C102)
>
> - [MemoryRegion 与 AddressSpace 之间关系]()
>
> - [MemoryRegion 与 MemoryRegionSection 之间关系]()
>  
> - [MemoryRegion 数据结构分析]()
>
> MemoryRegion 使用
>
> - [BiscuitOS 实践 MemoryRegion]()
>
> - [虚拟机新添加一条内存总线]()
>
> - [虚拟机新添加一个内存控制器]()
>
> - [虚拟机新添加一个内存条]()
>
> - [虚拟机新添加一段物理内存]()
>
> MemoryRegion 分析工具
>
> - [mtree with MemoryRegion]()
>
> - [E820 with MemoryRegion]()
>
> - [iomem/system memory with MemoryRegion]()

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

###### <span id="C100">MemoryRegion 与 GPA 之间的关系</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000884.png)

AddressSpace 与 MemoryRegion 的树状拓扑结构构成了虚拟机的 "内存总线"、"内存控制器" 以及 "Guest 物理内存区域". 三者的关系如上图，虚拟机使用 address_space_memory 表示虚拟机可以使用的物理内存空间，其 root 成员指向的 MemoryRegion (Root) 为虚拟机的 "内存总线"。"内存总线" 通过 subregions 作为起点，通过 subregions_link 挂载了多个 "物理内存区域"，每个 "物理内存区域" 没有子节点，"物理内存区域" 通过 MemoryRegion 进行描述。"物理内存区域" MemoryRegion 的 addr 成员指向其在虚拟器中维护的物理区域的起始值 (即 GPA 地址); "物理内存区域" MemoryRegion 的 size 成员表示 "物理内存区域" 的长度，例如上图 R0.size 就是 size0, 同理 R1.size 就是 size1，以及 R2.sze 就是 size2. 每个 "物理内存区域" MemoryRegion 的 container 成员指向其父节点，这样父子关系就得到确认. 通过上图可以获得如下关系:

{% highlight bash %}
# 映射关系
GPA:addr0 = R0->addr
GPA:addr1 = R1->addr
GPA:addr2 = R2->addr
{% endhighlight %}

------------------------------------------------------

###### <span id="C101">MemoryRegion 与 HVA 之间的关系</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000886.png)

虚拟机的物理内存需要在虚拟机运行之前与 QEMU 为其分配的虚拟内存进行绑定, 在 AdressSpace 与 MemoryRegion 构成的树形拓扑结构中，虽然 "物理内存区域" MemoryRegion 可以管理一段虚拟机物理内存，但是由于需要与实际的 Qemu 内存相互绑定，那么这个时候需要与 "内存控制器" 进行联系，"物理内存区域" MemoryRegion 通过 alias 指向 "内存控制器" MemoryRegion. 多个 "物理内存区域" MemoryRegion 可以指向同一个 "内存控制器" MemoryRegion. "内存控制器" MemoryRegion 通过 ram_block 成员指向 RAMBlock 数据结构，该数据结构可以理解为一块 "内存条", RAMBlock 的 size 成员指明了 "内存条" 的长度，另外 RAMBlock 的 host 指向 QEMU 为虚拟机分配的虚拟内存，而 offset 成员则用于指明该内存条在虚拟机物起始地址，那么通过 RAMBlock 可以获得 GPA 与 HAV 的映射关系。"内存控制器" MemoryRegion 负责管理这块内存条，另外 "物理内存区域" MemoryRegion 可以是这块内存条中一段区域，"物理内存区域" 的 addr 是其在虚拟机上的物理地址，那么 "物理内存区域" 的 alias_offset 就是其在内存条内的偏移，因此可以通过上面的逻辑关系结合上图可以获得:

{% highlight bash %}
# 映射关系
GPA:addr3 = R1->addr = R1->alias->ram_block->offset + R1->alias_offset
HVA:addr1 = R1->alias_offset + R1->alias->ram_block->host
# 推到关系
GPA:addr3 = R1->addr
          = R1->alias->ram_block->offset + R1->alias_offset
          = HVA:addr1 + (R1->alias->ram_block->offset - R1->alias->ram_block->host)
          = HVA:addr1 + (R3->ram_block->offset - R3->ram_block->host)
          = HVA:addr1 + (RB0->offset - RB0->host)
HVA:addr1 = R1->alias_offset + R1->alias->ram_block->host
          = GPA:addr3 - (R1->alias->ram_block->offset - R1->alias->ram_block->host)
          = GPA:addr3 - (R3->ram_block->offset - R3->ram_block->host)
          = GPA:addr3 - (RB0->offset - RB0->host)
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000887.png)

"内存总线" MemoryRegion 上存在多个 "物理内存区域" MemoryRegion, 多个 "物理内存区域" MemoryRegion 可以指向同一个 "内存控制器" MemoryRegion, 并且一个 "物理内存区域" MemoryRegion 只能指向一个 "内存控制器" MemoryRegion, 另外一个 "内存总线" MemoryRegion 上可以同时存在多个 "内存控制器"，那么就可以存在多个 "内存条" RAMBlock, 但一个 "内存控制器" MemoryRegion 只能指向一个 "内存条" RAMBlock, 并且 "内存条" RAMBlock 只能属于一个 "内存控制器" MemoryRegion. 上图给出了一个例子: "物理内存区域" R0 和 R1 指向 "内存控制器" R3, 那么其对应的 "内存条" 是 RB0. 同理 "物理内存区域" R2 指向 "内存控制器" R4, 那么其对应的 "内存条" 就是 RB1.

------------------------------------------------------

###### <span id="C102">MemoryRegion 与 HPA 之间的关系</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000739.png)

当 QEMU 为虚拟机分配了虚拟内存并与虚拟机的物理内存进行绑定，但此时的 QEMU 分配的虚拟内存并没有与真实的物理内存进行映射，因此当虚拟机访问虚拟机的物理内存时，系统会遇到 EPT 页表没有建立，那么虚拟机就会 VM_EXIT 退出到 Host，此时 kvm 模块就会为没有建立 EPT 页表的 GPA 建立页表，此时 kvm 模块首先找到 HVA, 如果 HVA 没有建立 HPA 的映射，那么此时会主动触发缺页建立该映射，建立完毕之后 kvm 模块再建立 GPA 到 HPA 的映射，因此在 QEMU 并不感知 HPA 的存在。在有的虚拟化方案中，当 QEMU 为虚拟机分配虚拟内存时就为该虚拟内存分配了物理内存，并建立好页表，那么虚拟机运行时对于没有建立 EPT 页表的 GPA，只需建立 EPT 页表即可。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000888.png)

MemoryRegion 与 HPA 的关系可以从图中可以看出 AddressSpace 通过 "内存总线" MemoryRegion、"内存控制器" MemoryRegion 和 "物理内存区域" MemoryRegion 只维护了 HVA 和 GPA 的关系，至于 GPA 与 HPA 的关系通过 kvm 模块进行维护，QEMU 不进行维护. 在上图中可以看出 R0 对应的虚拟内存 HAV:addr4, 改地址只有建立页表才能与物理内存 HPA 建立映射关系。

