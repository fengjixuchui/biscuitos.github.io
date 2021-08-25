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
> - [QEMU Memory Manager 实践]()
>
> - [QEMU Memory Manager 使用]()
>
> - [QEMU Memory Manager 调试工具]()
>
> - [QEMU Memory Manager 进阶研究]()
>
>   - [内存总线研究](#C108)
>
>   - [内存控制器与内存条研究](#C109)
>
>   - [内存条物理地址分配研究]()
>
>   - [新增一块物理内存研究]()
>
>   - [移除一块物理内存研究]()
>
>   - [GPA/HVA/HPA 地址转换研究]()
>
>   - [GPA AddressSpaceDispatch 页表研究]()
>
>   - [内存热插拔之热插研究]()
>
>   - [内存热插拔之热拔研究]()
>
> - [QEMU Memory Manager 核心数据结构]()
>
>   - [AddressSpace]()
>  
>   - [AddressSpaceDispatch]()
>
>   - [FlatRange]()
>
>   - [FlatView]()
>
>   - [KVMSlot]()
>
>   - [kvm_userspace_memory_region]()
>
>   - [MemoryListener]()
>   
>   - [MemoryRegion](#C1)
>
>   - [MemoryRegionSection]()
>   
>   - [Node]()
>
>   - [PhysPageEntry]()
>
>   - [PhysPageMap]()
>
>   - [subpage_t]()


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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### Overview

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000928.png)

在 QEMU QOM 中，QOM 使用 MemoryRegion 对象描述不同类型的内存组件，其中 MemoryRegion 可以用来描述内存总线。何为内存总线? 广义上的内存总线是用于连接内存系统和芯片组的北桥区域，从技术上讲内存总线由两部分组成: 数据总线在内存和芯片组之间传输信息, 地址总线用于定位存储介质的位置。QEMU 使用 MemoryRegion 对象模拟的内存总线也要具备这些功能。

在 QEMU 中使用 AddressSpace 描述虚拟机的地址空间，是一种高层的内存空间抽象，其下属的内存总线，QEMU 也使用 MemoryRegion 对象来模拟内存总线，这里称这种 MemoryRegion 为 "内存总线" MemoryRegion. QEMU 还使用 MemoryRegion 对象来模拟内存控制器，所谓内存控制器就是控制内存条与 CPU 之间的访问，因此从技术角度来讲，内存总线为了让 CPU 能否访问内存条里面的内存，那么内存总线需要使虚拟机的地址总线能否访问到内存条，另外内存总线需要使虚拟机的数据总线能否和内存条交互数据。QEMU 使用 "物理区域" MemoryRegion 对象从 "内存控制器" MemoryRegion 维护的 "内存条" RAMBlock 中划分一段内存出来，这段内存将暴露给虚拟机使用，也就是虚拟机能看到 "物理区域" MemoryRegion 描述的物理内存，而 "内存控制器" MemoryRegion 的 "内存条" RAMBlock 没有被描述的部分虚拟机是看不到的。"内存总线" MemoryRegion 以 subregions 作为链表头，以 "物理区域" MemoryRegion 的 subregions_link 为节点，将所以 "物理区域" 连接在一起形成模拟的 "内存总线"，每个 "物理区域" MemoryRegion 的 addr 成员指明了其在虚拟机中的物理地址，这样虚拟机的地址总线可以访问到该物理内存区域.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000886.png)

通过上面的描述可以获得上图的关系，"内存控制器" MemoryRegion (R3) 维护的内存有 "内存条" RAMBlock (RB0) 进行描述。在 "内存条" RAMBlock 中，offset 成员表示 QEMU 在虚拟机启动之前将 "内存条" RAMBlock (RB0) 插入到虚拟机地址总线之后获得物理地址，也就是 "内存条" RAMBlock (RB0) 在虚拟机地址总线地址为 GPA:addr3，其在虚拟机内存成为 Memory Bank。另外 QEMU 为 "内存条" RAMBlock (RB0) 分配虚拟内存，通过 "内存条" RAMBlock (RB0) 的 host 成员进行维护，因此 "内存条" RAMBlock 的起始 HVA:addr0 地址就是 host 成员的值. 同理 "物理区域" MemoryRegion (R0) 和 (R1) 从 "内存条" RAMBlock (RB0) 中各自划分了一段物理内存进行描述。"物理区域" MemoryRegion (R0) 在 "内存条" RAMBlock (RB0) 中的偏移是 R0->alias_offset, 那么 "物理区域" MemoryRegion (R0) 的物理地址就是 RB0->offset + R0->alias_offset. 同理 "物理区域" MemoryRegion (R1) 维护的物理区域的起始地址就是 RB0->offset + R1->alias_offset. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000929.png)

在 QEMU 中多个 "内存控制器" MemoryRegion 维护的 "内存条" RAMBlock 在上电之前插入到虚拟机的地址总线上，将会形成多个 Memory Bank 区域，这些区域不存在重合的部分，它们之间可以相临或者相离. 虚拟机并不能直接看到这些 "内存条"，而是需要使用 "物理区域" MemoryRegion 描述虚拟机能看到的部分，因此 "内存条" RAMBlock 内存就会出现暴露给虚拟机看到的物理内存区域，如 MR0、MR1 直到 MRn 这些区域。相反如果 "内存条" RAMBlock 中没有被 "物理区域" MemoryRegion 描述的区域虚拟机将不可见，那么这部分对于虚拟机来说就是内存空洞 Hole, 例如上图的 Hole0. "内存总线" MemoryRegion 将所有的 "物理区域" MemoryRegion 连接在一起，起初来看这个链表上的区域就是虚拟机能看到的物理内存，但事情远没有我们想象的简单，"物理区域" MemoryRegion 在描述区域可能会与其他 "物理区域" MemoryRegion 重叠，例如上图的 MR2 于 MR0 和 MR1 都有重叠区域。那么 "内存总线" MemoryRegion 就不能将一个线性的地址空间传递给虚拟机。那么此时 QEMU 使用了 FlatView 来将重叠的 "物理区域" MemoryRegion 中重叠部分去重之后组成一个平坦的线性物理内存空间。
 
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000930.png)

QEMU 使用 FlatView 数据结构将虚拟机的内存平坦化，其由 AddressSpace 的 current_map 进行指定，FlatView 数据结构维护了一个 FlatRange 的数组, 每个 FlatRange 可以理解为 "内存条" RAMBlock 暴露给虚拟机的区域，因此 FlatRange 只是多个 "物理区域" MemoryRegion 去重之后的物理内存区域描述，所以 FlatRange 主要维护内存来自哪个 "内存控制器" MemoryRegion, 以及描述其在 "内存条" RAMBlock 上的范围。FlatView 最终将暴露给虚拟机内存都按地址从低到高的维护在 FlatRange 数组里，这样从 QEMU 角度来看虚拟机要使用的内存已经平坦化了。那么现在问题来了，Host 端已经使用 FlatView 的 FlatRange 数组实现了平坦物理内存的管理，已经 "内存控制器" MemoryRegion 可以获得 Host 端内存到虚拟机物理内存的映射关系，那么 QEMU Guest 端的物理内存如何与 Host 端的虚拟内存建立映射关系呢? 这个问题可能会与 EPT 页表混淆，这里说明一下 EPT 页表处理的是虚拟机运行时处理 GPA 到 HPA 的关系，而这里的问题是当虚拟机没有启动或者进入 QEMU 的 monitor 模式之后，如何处理虚拟机物理内存与 Host 端内存的关系，两个问题还是有本质区别的。QEMU 这时引入了新的数据结构 MemoryRegionSection, 该数据结构是不是与 MemoryRegion 很相似，对很相似，但其主要处理虚拟机物理内存与 "内存控制器" MemoryRegion 的联系，为什么是 "内存控制器" MemoryRegion 而不是 "内存总线" MemoryRegion 或者 "物理区域" MemoryRegion 的联系呢? 大家可以思考一下 "内存控制器" 可是管理 "内存条" 虚拟机物理内存和 QEMU 分配的虚拟内存，直接建立 "内存控制器" MemoryRegion 便于地址转换.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000933.png)

当 QEMU 将重叠的 "物理区域" MemoryRegion 对象去重之后使用 FlatView 的 FlatRange 数组进行描述时，其逻辑框图如上. 在一个 "内存控制器" MemoryRegion (MC0) 管理的 "内存条" RAMBlock (RB0) 中，其在虚拟机地址总线上的地址为 RB0->offset, 即 Memory Bank (MB0) 区域的地址地址 GPA:addr0, 那么 RB0->offset 就等于 GPA:addr0. QEMU 为 "内存条" RAMBlock (RB0) 分配的虚拟内存通过 host 成员进行指定，其起始虚拟地址为 HVA:addr1. QEMU 将所有重叠和相邻的 "物理区域" MemoryRegion 合并在一起，形成了 Region (R0) 到 Region (Rn) 多个区域，每个区域可以相邻和相离，但不在重叠。此时 FlatView 的 FlatRange 数组按每个区域的起始地址从低到高对 Region 进行描述。例如 FlatRange 数组的第一个成员用于描述第一个 Region (R0), 那么 FaltRange (FR0) 的 AddrRange:addr 用于描述 Region (R0) 在虚拟机地址总线上的起始地址 GPA:addr0 以及 Region (R0) 的长度. FR0 的 mr 成员指向了 "内存控制器" MC0, offset_in_region 成员指向了 Region (R0) 在 "内存条" RAMBlock 内的偏移，那么此时 FR0->offset_in_region 等于 "HVA:addr1 - RB0->host". 同理使用 FlatRange 数组的第二个成员用于描述 Region (R1) 区域，那么 FaltRange (FR1) 的 AddrRange:addr 用于描述 Region (R1) 在虚拟机地址总线上的起始地址 GPA:addr2 以及 Region (R1) 的长度. FR1 的 mr 成员指向了 "内存控制器" MC0, offset_in_region 成员指向了 Region (R1) 在 "内存条" RAMBlock 内的偏移，那么此时 FR1->offset_in_region 等于 "HVA:addr3 - RB0->host" 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000932.png)

FlatView 和 FlatRange 的存在只是将 "内存总线" MemoryRegion 上的 "物理内存区域" MemoryRegion 去重并平坦化，使 QEMU 看到的 "内存总线" MemoryRegion 上的内存是线性平坦的。然后要在 QEMU 中建立虚拟机物理地址到 QEMU 端虚拟地址的联系，那还需要 MemoryRegionSection 的协助，首先通过上面的代码可以知道 MemoryRegionSection 是从 FlatRange 转换而来，但其在 QEMU 中当担的作用却和 FlatRange 不同，FlatRange 着重描述 Host 侧的内存布局，而 MemoryRegionSection 则是描述虚拟机内物理区域的描述. 从上图的代码可以获得 MemoryRegionSection 与 FlatRange、MemoryRegionSection 与 "内存控制器" MemoryRegion 联系的建立。 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000931.png)

在一个 "内存控制器" MemoryRegion (MC0) 管理的 "内存条" RAMBlock (RB0) 中，其在虚拟机地址总线上的地址为 RB0->offset, 即 Memory Bank (MB0) 区域的地址地址 GPA:addr0, 那么 RB0->offset 就等于 GPA:addr0. QEMU 为 "内存条" RAMBlock (RB0) 分配的虚拟内存通过 host 成员进行指定，其起始虚拟地址为 HVA:addr1. 此时虚拟机内部 "内存条" RAMBlock 对应 "Memory Bank (MB0)", 该 "Memory Bank (MB0)" 被分作两个区域，第一个区域称为 Region (R0), 第二个区域称为 Region (R1). QEMU 此时使用 MemoryRegionSection (MRS0) 来描述虚拟机内部的 Region (R0) 区域，其 offset_within_address_space 成员表示 Region (R0) 区域在虚拟机地址总线上的起始地址，也就是 GPA:addr0, 接着 mr 成员指向 "内存控制器" MemoryRegion (MC0), fv 成员指向 FlatRange (上图并未给出), offset_within_region 成员则表示其在虚拟内存中的偏移，其值等于 "HVA:addr1 - RB0->host"。同理 MemoryRegionSection (MRS1) 描述虚拟机内部的 Region (R1) 区域，其 offset_within_address_space 成员表示 Region (R1) 区域在虚拟机地址总线上的起始地址，也就是 GPA:addr2, 接着 mr 成员指向 "内存控制器" MemoryRegion (MC0), 其与 MemoryRegionSection (MRS0) 指向同一个 "内存控制器" MemoryRegion. fv 成员指向 FlatRange (上图并未给出), offset_within_region 成员则表示其在虚拟内存中的偏移，其值等于 "HVA:addr3 - RB0->host".

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000935.png)

通过上面的分析，MemoryRegionSection 可以通过其 fv 成员找到 FlatRange，然后通过 FlatRange 的 mr 成员可以找到 "内存控制器" MemoryRegion, 或者 MemoryRegionSection 可以直接通过 mr 找到 "内存控制器" MemoryRegion。只要找到 "内存控制器" MemoryRegion 就可以找到 "内存条" RAMBlock, 进而找到 "内存条" RAMBlock 在虚拟机地址总线上的物理地址 GPA，另外还可以获得 "内存条" RAMBlock 在 Host 端的虚拟内存 HVA。虽然 MemoryRegionSection 可以很好的描述虚拟机物理内存的指定区域，但 QEMU 还是没法通过虚拟机的物理地址 GPA 找到最终的 HVA，因此这个时候如果建立虚拟机物理地址 GPA 到 MemoryRegionSection 的映射关系，那么通过上图的关系虚拟机物理地址 GPA 就可以找到对于的 HVA.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000936.png)

QEMU 使用了一种类似页表的方式建立了虚拟机物理地址到 MemoryRegionSection 的映射。例如虚拟机物理页的长度为 4K，那么虚拟机物理页帧号的范围是物理地址的 65:12, QEMU 将这段地址按 9 bit 作为一段由低到高划分，那么就形成了 6 段偏移，其中 5 个段的位宽都是 9 bit，最高一段 65:57 的位宽是 8 bit。QEMU 首先通过一个类似于 CR3 寄存器的数据结构 PhysPageEntry 作为入口，其指向了由 512 个 PhysPageEntry 组成的数据结构 Node. QEMU 根据虚拟机的物理地址从最高偏移段开始，每个段的值就是一个 offset，并在 Node 中找到下一级 Node 的 PhysPageEntry. QEMU 依次遍历查询虚拟机的物理地址，最终遍历到最后一级 Node 的时候，根据最后一级的偏移找到 PhysPageEntry, 该入口指向了最终要找的 MemoryRegionSection. 以上便是虚拟机物理地址 GPA 与 MemoryRegionSection 映射逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000937.png)

QEMU 使用 FlatView 来支持虚拟机物理地址 GPA 到 MemoryRegionSection 的映射逻辑，在 FlatView 数据结构中存在 dispatch 成员，该成员是由 struct AddressSpaceDispatch 数据结构进行描述，在 struct AddressSpaceDispatch 数据结构中存在 PhyPageEntry 数据结构的成员 phys_map, 其构成了 "页表" 的 "CR3 寄存器"，QEMU 依次为入口找到 "页表" 的入口，"页表" 通过数据结构 Node 进行描述，其由 512 个 PhysPageEntry 构成，正好构成了 "页表入口"，QEMU 根据查表找到最后一级 "页表入口项"。另外 AddressSpaceDispatch 还存在另外一个成员 map，其由 PhysPageMap 数据结构进行描述。map 成员主要维护了两个内容，第一个是由 nodes 成员指向的 Node 数组，该数组维护了所有的 "页表"。map 成员维护的另外一个是由 sections 指向的 MemoryRegionSection 数组。当为虚拟机物理地址 GPA 建立映射关系时，QEMU 会将虚拟机物理地址的页帧号按 9 bit 划分成多个字段，每个字段作为一个偏移值正好可以表示 512 种值，最高字段只有 8 bit。QEMU 从 AddressSpaceDispatch 的 phys_map 开始进行查找，其存储了第 6 级页表在 map 维护的 Nodes 数组中的偏移，如果此时 phys_map 没有指向一个页表，那么表明第 6 级页表还没有建立，那么 QEMU 就从 map 维护的 nodes 数组中找到一个空闲的 Node 作为页表，并把该 Node 在 nodes 中的偏移存储在 phys_map 中，这样的操作完成了一次页表页的分配和绑定操作。当找到第 6 级页表之后，QEMU 将虚拟机物理地址页帧的第 6 级字段作作为第 6 级页表中的偏移找到下一级页表的 Entry。当找到下一级页表的 Entry 时发现其没有下一级页表的信息，那么采取和上一级页表的操作从 nodes 数组中查找一个空闲的 Node 作为页表页，并将页表页在 nodes 中的索引作为下一级页表的信息填入到 Entry，依次类推。但 QEMU 遍历到最后一级页表的时候 Entry 需要指向 MemoryRegionSection, 此时 QEMU 在 sections 数组中找一个空闲的槽位，将槽位里的 MemoryRegionSection 指针指向 MemoryRegionSection, 接着将槽位在 sections 数组中的索引写入到 Entry 中，那么这样就建立了 Entry 到 MemoryRegionSection 的映射。通过上面的操作完成了虚拟机物理地址 GPA 到 MemoryRegionSection 的映射。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000938.png)

通过上面的映射建立，那么虚拟机的物理地址 GPA 可以通过 "页表映射" AddressSpaceDispatch 找到对应的 MemoryRegionSection，进而找到对应的 "内存控制器" MemoryRegion 和 "内存条" RAMBlock, 最终找到 QEMU 分配的虚拟内存 HVA。有了上面的关系之后，QEMU 接下来要做的就是将这些内存信息提交给 KVM, 然后 KVM 为虚拟机构建内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000939.png)

总结之前的分析，一个 "物理区域" MemoryRegion 描述了需要暴露给虚拟机的物理内存信息，然后通过 FlatView 的平坦化转换成了 FlatRange，FlatRange 没有做过多的处理直接转换成了 MemoryRegionSection, MemoryRegionSection 中维护了 "内存控制器" MemoryRegion 的映射。接着通过 AddressSpaceDispatch 建立了虚拟机物理地址 GPA 到 MemoryRegionSection 的映射关系。此时 QEMU 通过 MemoryListener 通知链将新的 MemoryRegionSection 传递给 accel/kvm，其调用 kvm_region_add() 函数将 MemoryRegionSection 转换成 KVMSlot, 进行再加工之后转换成了 kvm_userspace_memory_region 数据结构。最后 QEMU 通过调用 IOCTL KVM_SET_USER_MEMORY_REGION 将新添加的虚拟机内存下发到 KVM 模块，待虚拟机启动就可以使用新的内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

#### MemoryRegion

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000885.png)

MemoryRegion 作为内存模拟中的核心数据结构，不仅用来表示虚拟机的一段物理内存区域，也用于搭建 GPA 与 RamBlock(HVA) 之间的桥梁。QEMU 中使用 AddressSpace 数据结构来表示 Guest 中 CPU/设备能够看到的内存，其中 address_space_memory 用来表示 Guest 中的物理内存, 另外 MemoryRegion 与 MemoryRegion 之间通过 subregions/subregions_link 双链表构成了树状的拓扑结构, 那么 AddressSpace 的 root 指针指向的根节点的 MemoryRegion，根节点的 MemoryRegion 相当于虚拟机的 "内存总线", 树状的叶子节点对应的 MemoryRegion 是实际分配给虚拟机的 "物理内存区域"。另外 MemoryRegion 的 alias 指向的 MemoryRegion 称为指向者的 "别名"，"别名" MemoryRegion 相当于虚拟机的 "内存控制器"，其管理实际的内存。每个 "物理内存区域" MemoryRegion 的 container 成员指向其父节点, 这样父子关系就得到明确.

> MemoryRegion 基础知识
>
> - [MemoryRegion 数据结构分析](#C105)
>
> - [MemoryRegion 对象初始化](#C106)
>
> - [MemoryRegion 对象属性](#C107)
>
> - [内存总线 MemoryRegion](#C108)
>
> - [内存控制器 MemoryRegion 与内存条 RAMBlock](#C109)
>
> - [物理内存区域 MemoryRegion](#C10B)
>
> - [MemoryRegion 与 GPA 之间的关系](#C100)
>
> - [MemoryRegion 与 HVA 之间的关系](#C101)
>
> - [MemoryRegion 与 HPA 之间的关系](#C102)
>
> - [MemoryRegion 与 AddressSpace 之间关系](#C103)
>
> - [MemoryRegion 与 MemoryRegionSection 之间关系](#C104)
>  
> MemoryRegion 使用
>
> - [BiscuitOS 实践 MemoryRegion]()
>
> - [虚拟机新添加一个内存控制器](#C1X3)
>
> - [虚拟机新添加一个内存条](#C1X4)
>
> - [虚拟机新添加一段可用物理内存](#C1X5)
>
> - [虚拟机新添加一段预留物理内存](#C1X6)
>
> - [MemoryRegion 对象添加属性](#C1X7)
>
> MemoryRegion 分析工具
>
> - [mtree with MemoryRegion]()
>
> - [E820 with MemoryRegion]()
>
> - [iomem/system memory with MemoryRegion]()

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C106"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

#### MemoryRegion 对象初始化

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000920.png)

在 QEMU QOM 架构中，MemoryRegion 也是一种对象抽象，因此 MemoryRegion 在初始化的时候也会继承、封装和多态。在面向对象思想中，MemoryRegion 也有属于它的类，其类为 TYPE_MEMORY_REGION。TYPE_MEMORY_REGION 类的定义如下: 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000895.png)

TYPE_MEMORY_REGION 在定义时其父类是 TYPE_OBJECT, 其为每个 MemoryRegion 对象提供了构造函数 memory_region_initfn 和析构函数 memory_region_finalize， 也就是说当初始化一个 MemoryRegion 对象的时候，MemoryRegion 对象都会调用 memory_region_initfn() 函数，而在释放 MemoryRegion 对象的时候，MemoryRegion 对象会自动调用 memory_region_finalize() 函数。TYPE_MEMORY_REGION 类通过 type_region_static() 函数注册到系统，并通过 type_init() 函数进行调用。MemoryRegion 对象的初始化一般如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000896.png)

memory_region_init() 函数用于初始化一个 MemoryRegion 对象，其参数 mr 指向需要初始化的 MemoryRegion 对象，name 参数指明了对象的名字，size 则指明的 MemoryRegion 表示的长度. 函数首先调用 object_initialize() 函数找到 MR 对应的类 TYPE_MEMORY_REGION，并调用该类的构造函数，此时会调用到 TYPE_MEMORY_REGION 类提供的 instance_init 接口函数  memory_region_initfn() 函数，该函数初始化 MemoryRegion 对象时做了如下操作:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000897.png)

* MemoryRegion 对象默认的操作函数集合 unassigned_mem_ops
* 将 MemoryRegion 的 enable 设置为 true，即使能 MemoryRegion
* 初始化 MemoryRegion 的 subregions 和 coalesced 双链表
* MemoryRegion 对象添加 link 类型的 container 属性
  * 每个 MemoryRegion 可以是其他 MemroyRegion 的 container
  * link 类型属性是一种链接关系，表示一个设备引用另外一个设备
* MemoryRegion 对象添加 addr、priority 和 size 属性

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000896.png)

接着 memory_region_init() 函数调用 memory_region_do_init() 函数对 MR 对象进一步的初始化，memory_region_do_init() 函数实现逻辑如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000900.png)

函数首先设置了 MemoryRegion 对象的长度，这里调用 int128_make64() 函数将一个 64 位长度转换成 128 位长度，因此 MemoryRegion 对象中维护的长度位 128 位。函数接着调用 g_strdup() 函数为 MemoryRegion 对象的名字分配空间并设置对象的名字。接着将 ram_block 成员设置为 NULL, 因为在 MemoryRegion 对象初始化的时候不维护任何虚拟内存，另外不是所有的 MemoryRegion 都要维护虚拟内存，因此 MemoryRegion对象初始化的时候将 ram_block 对象初始化为 NULL. 如果 name 参数不为空，函数将 name 构造成 "name[\*]" 的模式, 如果 owner 参数为空，那么函数会将名为 "/unattached" 的对象作为 MemoryRegion 对象的 owner。函数调用 object_property_add_child() 函数将 MR 对象作为 owner 的 child 属性. MR 对象的 owner 和 child 关系可以通过 qemu monitor 模式 (info qom-tree) 进行查看，例如:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C107"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### MemoryRegion 对象属性

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000904.png)

在 QEMU QOM 架构中，MemoryRegion 也是一种对象抽象，因此 MemoryRegion 对象也有属性的概念。属性对于 MemoryRegion 来说就是将 MemoryRegion 成员通过 QOM 的属性机制提供了 "GET" 和 "SET" 的接口，以此供其他函数使用。QOM 对象的属性可以通过动态或静态的方式进行添加，对于 MemoryRegion 对象，其属性通过动态方式进行添加，例如在 "MemoryRegion 对象初始化" 章节介绍了 MR 对象在初始化时，TYPE_MEMORY_REGION 类会为其添加 "container"、"addr"、"size" 以及 "priority" 属性. 另外 QOM 也提供了函数获得属性值以及动态设置属性的值，具体函数如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000901.png)

利用 QOM 提供的函数，可以动态为 MemoryRegion 添加属性，以及设置和读取属性值，那么属性对 MemoryRegion 意味着什么? 属性对于 MemoryRegion 对象就是将其成员通过 QOM 提供的接口透露给外部使用。例如 MemoryRegion 对象初始化自带的属性，都是将 MemoryRegion 的成员外部通过 object_property_get() 类似的函数简洁的获得，另外属性也是 MemoryRegion 对象想展示给外部看到的特性. 那么这里对 MemoryRegion 自带的属性进行详细分析:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000899.png)

系统在调用 memory_region_initfn() 函数初始化 MemoryRegion 对象时，首先会调用 object_property_add() 函数为 MemoryRegion 对象添加了 link 类型的 container 属性，该属性实现了每个 MemoryRegion 都可以是其他 MemoryRegion 的 container, 另外 link 属性是一种链接关系，其表示一种设备引用另一种设备，那么初始化时 "container" 属性值设置为 "link<" TYPE_MEMORY_REGION ">". "container" 属性的 GET 接口是 memory_region_get_container() 函数，该函数用于从 MemoryRegion 中读取 container 成员的值，结合 MemoryRegion 的 container 成员，可知 MemoryRegion 对象的 "container" 属性表示其父 MemoryRegion 对象.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000902.png)

系统在调用 memory_region_initfn() 函数初始化 MemoryRegion 对象时，首先会调用 object_property_add() 函数为 MemoryRegion 对象添加 "addr" 属性，属性的 GET 接口时 memory_region_get_addr() 函数，该函数的作用是从 MemoryRegion 对象中读取 addr 成员的值，addr 成员用于描述 MemoryRegion 描述 Guest OS 物理内存的起始地址，即 GPA 地址值。结合 addr 成员的含义，那么 MemoryRegion 对象的 addr 属性用于描述 MemoryRegion 维护的 Guest OS 物理内存起始地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000903.png)

系统在调用 memory_region_initfn() 函数初始化 MemoryRegion 对象时，首先会调用 object_property_add() 函数为 MemoryRegion 对象添加 "priority" 属性，属性的 GET 接口时 memory_region_get_priority() 函数，该函数的作用是从 MemoryRegion 对象中读取 priority 成员的值，priority 成员用于描述 MemoryRegion 优先级，优先级用于处理相交的 MemoryRegion 的情况, QEMU 优先采用优先级高的 MemoryRegion。结合上面的分析，MemoryRegion 对象的 priority 属性用于描述 MemoryRegion 对象的优先级.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000905.png)

系统在调用 memory_region_initfn() 函数初始化 MemoryRegion 对象时，首先会调用 object_property_add() 函数为 MemoryRegion 对象添加 "size" 属性，属性的 GET 接口时 memory_region_get_size() 函数，该函数的作用是从 MemoryRegion 对象中读取 size 成员的值，size 成员用于描述 MemoryRegion 维护虚拟机内存的长度，结合上面的分析，MemoryRegion 对象的 size 属性用于描述 MemoryRegion 对象维护虚拟机内存的长度.

以上便是 MemoryRegion 对象初始化自带的属性，如果想为 MemoryRegion 中某个成员属性化，那么可以参考如下章节:

> [MemoryRegion 对象添加属性](#C1X7)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C108"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000M.jpg)

#### 内存总线 MemoryRegion

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000928.png)

在 QEMU QOM 中，QOM 使用 MemoryRegion 对象描述不同类型的内存组件，其中 MemoryRegion 可以用来描述内存总线。何为内存总线? 广义上的内存总线是用于连接内存系统和芯片组的北桥区域，从技术上讲内存总线由两部分组成: 数据总线在内存和芯片组之间传输信息, 地址总线用于定位存储介质的位置。QEMU 使用 MemoryRegion 对象模拟的内存总线也要具备这些功能。

在 QEMU 中使用 AddressSpace 描述虚拟机的地址空间，是一种高层的内存空间抽象，其下属的内存总线，QEMU 也使用 MemoryRegion 对象来模拟内存总线，这里称这种 MemoryRegion 为 "内存总线" MemoryRegion. QEMU 还使用 MemoryRegion 对象来模拟内存控制器，所谓内存控制器就是控制内存条与 CPU 之间的访问，因此从技术角度来讲，内存总线为了让 CPU 能否访问内存条里面的内存，那么内存总线需要使虚拟机的地址总线能否访问到内存条，另外内存总线需要使虚拟机的数据总线能否和内存条交互数据。QEMU 使用 "物理区域" MemoryRegion 对象从 "内存控制器" MemoryRegion 维护的 "内存条" RAMBlock 中划分一段内存出来，这段内存将暴露给虚拟机使用，也就是虚拟机能看到 "物理区域" MemoryRegion 描述的物理内存，而 "内存控制器" MemoryRegion 的 "内存条" RAMBlock 没有被描述的部分虚拟机是看不到的。"内存总线" MemoryRegion 以 subregions 作为链表头，以 "物理区域" MemoryRegion 的 subregions_link 为节点，将所以 "物理区域" 连接在一起形成模拟的 "内存总线"，每个 "物理区域" MemoryRegion 的 addr 成员指明了其在虚拟机中的物理地址，这样虚拟机的地址总线可以访问到该物理内存区域.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000886.png)

通过上面的描述可以获得上图的关系，"内存控制器" MemoryRegion (R3) 维护的内存有 "内存条" RAMBlock (RB0) 进行描述。在 "内存条" RAMBlock 中，offset 成员表示 QEMU 在虚拟机启动之前将 "内存条" RAMBlock (RB0) 插入到虚拟机地址总线之后获得物理地址，也就是 "内存条" RAMBlock (RB0) 在虚拟机地址总线地址为 GPA:addr3，其在虚拟机内存成为 Memory Bank。另外 QEMU 为 "内存条" RAMBlock (RB0) 分配虚拟内存，通过 "内存条" RAMBlock (RB0) 的 host 成员进行维护，因此 "内存条" RAMBlock 的起始 HVA:addr0 地址就是 host 成员的值. 同理 "物理区域" MemoryRegion (R0) 和 (R1) 从 "内存条" RAMBlock (RB0) 中各自划分了一段物理内存进行描述。"物理区域" MemoryRegion (R0) 在 "内存条" RAMBlock (RB0) 中的偏移是 R0->alias_offset, 那么 "物理区域" MemoryRegion (R0) 的物理地址就是 RB0->offset + R0->alias_offset. 同理 "物理区域" MemoryRegion (R1) 维护的物理区域的起始地址就是 RB0->offset + R1->alias_offset. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000929.png)

在 QEMU 中多个 "内存控制器" MemoryRegion 维护的 "内存条" RAMBlock 在上电之前插入到虚拟机的地址总线上，将会形成多个 Memory Bank 区域，这些区域不存在重合的部分，它们之间可以相临或者相离. 虚拟机并不能直接看到这些 "内存条"，而是需要使用 "物理区域" MemoryRegion 描述虚拟机能看到的部分，因此 "内存条" RAMBlock 内存就会出现暴露给虚拟机看到的物理内存区域，如 MR0、MR1 直到 MRn 这些区域。相反如果 "内存条" RAMBlock 中没有被 "物理区域" MemoryRegion 描述的区域虚拟机将不可见，那么这部分对于虚拟机来说就是内存空洞 Hole, 例如上图的 Hole0. "内存总线" MemoryRegion 将所有的 "物理区域" MemoryRegion 连接在一起，起初来看这个链表上的区域就是虚拟机能看到的物理内存，但事情远没有我们想象的简单，"物理区域" MemoryRegion 在描述区域可能会与其他 "物理区域" MemoryRegion 重叠，例如上图的 MR2 于 MR0 和 MR1 都有重叠区域。那么 "内存总线" MemoryRegion 就不能将一个线性的地址空间传递给虚拟机。那么此时 QEMU 使用了 FlatView 来将重叠的 "物理区域" MemoryRegion 中重叠部分去重之后组成一个平坦的线性物理内存空间。
 
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000930.png)

QEMU 使用 FlatView 数据结构将虚拟机的内存平坦化，其由 AddressSpace 的 current_map 进行指定，FlatView 数据结构维护了一个 FlatRange 的数组, 每个 FlatRange 可以理解为 "内存条" RAMBlock 暴露给虚拟机的区域，因此 FlatRange 只是多个 "物理区域" MemoryRegion 去重之后的物理内存区域描述，所以 FlatRange 主要维护内存来自哪个 "内存控制器" MemoryRegion, 以及描述其在 "内存条" RAMBlock 上的范围。FlatView 最终将暴露给虚拟机内存都按地址从低到高的维护在 FlatRange 数组里，这样从 QEMU 角度来看虚拟机要使用的内存已经平坦化了。那么现在问题来了，Host 端已经使用 FlatView 的 FlatRange 数组实现了平坦物理内存的管理，已经 "内存控制器" MemoryRegion 可以获得 Host 端内存到虚拟机物理内存的映射关系，那么 QEMU Guest 端的物理内存如何与 Host 端的虚拟内存建立映射关系呢? 这个问题可能会与 EPT 页表混淆，这里说明一下 EPT 页表处理的是虚拟机运行时处理 GPA 到 HPA 的关系，而这里的问题是当虚拟机没有启动或者进入 QEMU 的 monitor 模式之后，如何处理虚拟机物理内存与 Host 端内存的关系，两个问题还是有本质区别的。QEMU 这时引入了新的数据结构 MemoryRegionSection, 该数据结构是不是与 MemoryRegion 很相似，对很相似，但其主要处理虚拟机物理内存与 "内存控制器" MemoryRegion 的联系，为什么是 "内存控制器" MemoryRegion 而不是 "内存总线" MemoryRegion 或者 "物理区域" MemoryRegion 的联系呢? 大家可以思考一下 "内存控制器" 可是管理 "内存条" 虚拟机物理内存和 QEMU 分配的虚拟内存，直接建立 "内存控制器" MemoryRegion 便于地址转换.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000933.png)

当 QEMU 将重叠的 "物理区域" MemoryRegion 对象去重之后使用 FlatView 的 FlatRange 数组进行描述时，其逻辑框图如上. 在一个 "内存控制器" MemoryRegion (MC0) 管理的 "内存条" RAMBlock (RB0) 中，其在虚拟机地址总线上的地址为 RB0->offset, 即 Memory Bank (MB0) 区域的地址地址 GPA:addr0, 那么 RB0->offset 就等于 GPA:addr0. QEMU 为 "内存条" RAMBlock (RB0) 分配的虚拟内存通过 host 成员进行指定，其起始虚拟地址为 HVA:addr1. QEMU 将所有重叠和相邻的 "物理区域" MemoryRegion 合并在一起，形成了 Region (R0) 到 Region (Rn) 多个区域，每个区域可以相邻和相离，但不在重叠。此时 FlatView 的 FlatRange 数组按每个区域的起始地址从低到高对 Region 进行描述。例如 FlatRange 数组的第一个成员用于描述第一个 Region (R0), 那么 FaltRange (FR0) 的 AddrRange:addr 用于描述 Region (R0) 在虚拟机地址总线上的起始地址 GPA:addr0 以及 Region (R0) 的长度. FR0 的 mr 成员指向了 "内存控制器" MC0, offset_in_region 成员指向了 Region (R0) 在 "内存条" RAMBlock 内的偏移，那么此时 FR0->offset_in_region 等于 "HVA:addr1 - RB0->host". 同理使用 FlatRange 数组的第二个成员用于描述 Region (R1) 区域，那么 FaltRange (FR1) 的 AddrRange:addr 用于描述 Region (R1) 在虚拟机地址总线上的起始地址 GPA:addr2 以及 Region (R1) 的长度. FR1 的 mr 成员指向了 "内存控制器" MC0, offset_in_region 成员指向了 Region (R1) 在 "内存条" RAMBlock 内的偏移，那么此时 FR1->offset_in_region 等于 "HVA:addr3 - RB0->host" 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000932.png)

FlatView 和 FlatRange 的存在只是将 "内存总线" MemoryRegion 上的 "物理内存区域" MemoryRegion 去重并平坦化，使 QEMU 看到的 "内存总线" MemoryRegion 上的内存是线性平坦的。然后要在 QEMU 中建立虚拟机物理地址到 QEMU 端虚拟地址的联系，那还需要 MemoryRegionSection 的协助，首先通过上面的代码可以知道 MemoryRegionSection 是从 FlatRange 转换而来，但其在 QEMU 中当担的作用却和 FlatRange 不同，FlatRange 着重描述 Host 侧的内存布局，而 MemoryRegionSection 则是描述虚拟机内物理区域的描述. 从上图的代码可以获得 MemoryRegionSection 与 FlatRange、MemoryRegionSection 与 "内存控制器" MemoryRegion 联系的建立。 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000931.png)

在一个 "内存控制器" MemoryRegion (MC0) 管理的 "内存条" RAMBlock (RB0) 中，其在虚拟机地址总线上的地址为 RB0->offset, 即 Memory Bank (MB0) 区域的地址地址 GPA:addr0, 那么 RB0->offset 就等于 GPA:addr0. QEMU 为 "内存条" RAMBlock (RB0) 分配的虚拟内存通过 host 成员进行指定，其起始虚拟地址为 HVA:addr1. 此时虚拟机内部 "内存条" RAMBlock 对应 "Memory Bank (MB0)", 该 "Memory Bank (MB0)" 被分作两个区域，第一个区域称为 Region (R0), 第二个区域称为 Region (R1). QEMU 此时使用 MemoryRegionSection (MRS0) 来描述虚拟机内部的 Region (R0) 区域，其 offset_within_address_space 成员表示 Region (R0) 区域在虚拟机地址总线上的起始地址，也就是 GPA:addr0, 接着 mr 成员指向 "内存控制器" MemoryRegion (MC0), fv 成员指向 FlatRange (上图并未给出), offset_within_region 成员则表示其在虚拟内存中的偏移，其值等于 "HVA:addr1 - RB0->host"。同理 MemoryRegionSection (MRS1) 描述虚拟机内部的 Region (R1) 区域，其 offset_within_address_space 成员表示 Region (R1) 区域在虚拟机地址总线上的起始地址，也就是 GPA:addr2, 接着 mr 成员指向 "内存控制器" MemoryRegion (MC0), 其与 MemoryRegionSection (MRS0) 指向同一个 "内存控制器" MemoryRegion. fv 成员指向 FlatRange (上图并未给出), offset_within_region 成员则表示其在虚拟内存中的偏移，其值等于 "HVA:addr3 - RB0->host".

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000935.png)

通过上面的分析，MemoryRegionSection 可以通过其 fv 成员找到 FlatRange，然后通过 FlatRange 的 mr 成员可以找到 "内存控制器" MemoryRegion, 或者 MemoryRegionSection 可以直接通过 mr 找到 "内存控制器" MemoryRegion。只要找到 "内存控制器" MemoryRegion 就可以找到 "内存条" RAMBlock, 进而找到 "内存条" RAMBlock 在虚拟机地址总线上的物理地址 GPA，另外还可以获得 "内存条" RAMBlock 在 Host 端的虚拟内存 HVA。虽然 MemoryRegionSection 可以很好的描述虚拟机物理内存的指定区域，但 QEMU 还是没法通过虚拟机的物理地址 GPA 找到最终的 HVA，因此这个时候如果建立虚拟机物理地址 GPA 到 MemoryRegionSection 的映射关系，那么通过上图的关系虚拟机物理地址 GPA 就可以找到对于的 HVA.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000936.png)

QEMU 使用了一种类似页表的方式建立了虚拟机物理地址到 MemoryRegionSection 的映射。例如虚拟机物理页的长度为 4K，那么虚拟机物理页帧号的范围是物理地址的 65:12, QEMU 将这段地址按 9 bit 作为一段由低到高划分，那么就形成了 6 段偏移，其中 5 个段的位宽都是 9 bit，最高一段 65:57 的位宽是 8 bit。QEMU 首先通过一个类似于 CR3 寄存器的数据结构 PhysPageEntry 作为入口，其指向了由 512 个 PhysPageEntry 组成的数据结构 Node. QEMU 根据虚拟机的物理地址从最高偏移段开始，每个段的值就是一个 offset，并在 Node 中找到下一级 Node 的 PhysPageEntry. QEMU 依次遍历查询虚拟机的物理地址，最终遍历到最后一级 Node 的时候，根据最后一级的偏移找到 PhysPageEntry, 该入口指向了最终要找的 MemoryRegionSection. 以上便是虚拟机物理地址 GPA 与 MemoryRegionSection 映射逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000937.png)

QEMU 使用 FlatView 来支持虚拟机物理地址 GPA 到 MemoryRegionSection 的映射逻辑，在 FlatView 数据结构中存在 dispatch 成员，该成员是由 struct AddressSpaceDispatch 数据结构进行描述，在 struct AddressSpaceDispatch 数据结构中存在 PhyPageEntry 数据结构的成员 phys_map, 其构成了 "页表" 的 "CR3 寄存器"，QEMU 依次为入口找到 "页表" 的入口，"页表" 通过数据结构 Node 进行描述，其由 512 个 PhysPageEntry 构成，正好构成了 "页表入口"，QEMU 根据查表找到最后一级 "页表入口项"。另外 AddressSpaceDispatch 还存在另外一个成员 map，其由 PhysPageMap 数据结构进行描述。map 成员主要维护了两个内容，第一个是由 nodes 成员指向的 Node 数组，该数组维护了所有的 "页表"。map 成员维护的另外一个是由 sections 指向的 MemoryRegionSection 数组。当为虚拟机物理地址 GPA 建立映射关系时，QEMU 会将虚拟机物理地址的页帧号按 9 bit 划分成多个字段，每个字段作为一个偏移值正好可以表示 512 种值，最高字段只有 8 bit。QEMU 从 AddressSpaceDispatch 的 phys_map 开始进行查找，其存储了第 6 级页表在 map 维护的 Nodes 数组中的偏移，如果此时 phys_map 没有指向一个页表，那么表明第 6 级页表还没有建立，那么 QEMU 就从 map 维护的 nodes 数组中找到一个空闲的 Node 作为页表，并把该 Node 在 nodes 中的偏移存储在 phys_map 中，这样的操作完成了一次页表页的分配和绑定操作。当找到第 6 级页表之后，QEMU 将虚拟机物理地址页帧的第 6 级字段作作为第 6 级页表中的偏移找到下一级页表的 Entry。当找到下一级页表的 Entry 时发现其没有下一级页表的信息，那么采取和上一级页表的操作从 nodes 数组中查找一个空闲的 Node 作为页表页，并将页表页在 nodes 中的索引作为下一级页表的信息填入到 Entry，依次类推。但 QEMU 遍历到最后一级页表的时候 Entry 需要指向 MemoryRegionSection, 此时 QEMU 在 sections 数组中找一个空闲的槽位，将槽位里的 MemoryRegionSection 指针指向 MemoryRegionSection, 接着将槽位在 sections 数组中的索引写入到 Entry 中，那么这样就建立了 Entry 到 MemoryRegionSection 的映射。通过上面的操作完成了虚拟机物理地址 GPA 到 MemoryRegionSection 的映射。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000938.png)

通过上面的映射建立，那么虚拟机的物理地址 GPA 可以通过 "页表映射" AddressSpaceDispatch 找到对应的 MemoryRegionSection，进而找到对应的 "内存控制器" MemoryRegion 和 "内存条" RAMBlock, 最终找到 QEMU 分配的虚拟内存 HVA。有了上面的关系之后，QEMU 接下来要做的就是将这些内存信息提交给 KVM, 然后 KVM 为虚拟机构建内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000939.png)

总结之前的分析，一个 "物理区域" MemoryRegion 描述了需要暴露给虚拟机的物理内存信息，然后通过 FlatView 的平坦化转换成了 FlatRange，FlatRange 没有做过多的处理直接转换成了 MemoryRegionSection, MemoryRegionSection 中维护了 "内存控制器" MemoryRegion 的映射。接着通过 AddressSpaceDispatch 建立了虚拟机物理地址 GPA 到 MemoryRegionSection 的映射关系。此时 QEMU 通过 MemoryListener 通知链将新的 MemoryRegionSection 传递给 accel/kvm，其调用 kvm_region_add() 函数将 MemoryRegionSection 转换成 KVMSlot, 进行再加工之后转换成了 kvm_userspace_memory_region 数据结构。最后 QEMU 通过调用 IOCTL KVM_SET_USER_MEMORY_REGION 将新添加的虚拟机内存下发到 KVM 模块，待虚拟机启动就可以使用新的内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C109"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000C.jpg)

#### 内存控制器 MemoryRegion 与内存条 RAMBlock

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000928.png)

在 QEMU QOM 中，QOM 使用 MemoryRegion 对象用于描述不同类型的内存组件，其中 MemoryRegion 可以用于描述内存控制器。何为内存控制器呢? 内存控制器是计算机系统内部控制内存并且负责内存与 CPU 之间数据交换的重要组成部分。内存控制器决定了计算机系统所能使用的最大内容容量、内存条 BANK 数量、内存类型和速度、内存颗粒深度和数据宽度等重要参数。QOM 使用 MemoryRegion 来模拟内存控制器实现了部分的功能，其中包括了虚拟机能使用的最大内存容量、内存条 BANK 数、以及内存类型等。 

在 QEMU 中使用 AddressSpace 描述虚拟机的内存地址空间，是一种高层的内存空间抽象，其下属内存总线，QEMU 也使用 MemoryRegion 对象来模拟内存总线，这里称这种 MemoryRegion 为 "内存总线" MemoryRegion. 内存总线上维护多个暴露给虚拟机使用的物理内存区域，这些物理内存区域也是使用 MemoryRegion 对象进行描述，"内存总线" MemoryRegion 将其 subregions 作为链表头，将所有可用的 "物理区域" MemoryRegion 通过其 subregions_link 关联在一起。那么问题来了， "物理区域" 并不维护真正的内存，而是维护暴露给虚拟机物理内存信息，那么虚拟机真正的内存来自哪里呢? 这里就需要内存控制器登场了，QEMU 使用 MemoryRegion 描述一个内存控制器，这里称这种 MemoryRegion 对象为 "内存控制器" MemoryRegion. "内存控制器" MemoryRegion 维护真实内存，其 ram_block 成员指向一个 RAMBlock, 这里称该 RAMBlock 为 "内存条" RAMBlock. 在计算机体系中，内存条需要插入到地址总线上，这样系统硬件才可以访问该内存条，QEMU 也模拟该过程，QEMU 在虚拟机启动之前，将 "内存条" RAMBlock 插入到虚拟机的地址总线上，那么就可以获得一个虚拟机的物理地址，该物理地址就是 "内存条" RAMBlock 在虚拟机地址总线上的物理地址，因此 "内存控制器" MemoryRegion 就可以知道其维护的内存在虚拟机内的物理地址。另外内存条就是真实用来存储内容的介质，那么 QEMU 也要为 "内存条" RAMBlock 分配虚拟内存来存储虚拟机的内容。QEMU 为 "内存条" RAMBlock 分配内存之后，"内存条" RAMBlcok 就可以知道其在 Host 端的虚拟地址，进一步 "内存控制器" MemoryRegion 也就知道其维护的内存在 Host 端的虚拟地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000886.png) 

通过上面的描述可以获得上图的关系，"内存控制器" MemoryRegion (R3) 维护的内存有 "内存条" RAMBlock (RB0) 进行描述。在 "内存条" RAMBlock 中，offset 成员表示 QEMU 在虚拟机启动之前将 "内存条" RAMBlock (RB0) 插入到虚拟机地址总线之后获得物理地址，也就是 "内存条" RAMBlock (RB0) 在虚拟机地址总线地址为 GPA:addr3，其在虚拟机内存成为 Memory Bank。另外 QEMU 为 "内存条" RAMBlock (RB0) 分配虚拟内存，通过 "内存条" RAMBlock (RB0) 的 host 成员进行维护，因此 "内存条" RAMBlock 的起始 HVA:addr0 地址就是 host 成员的值. 那么如何在 QEMU 中创建一个新的 "内存控制器" MemoryRegion 和 "内存条" RAMBlock 呢? 可以参考如下代码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000906.png)

"内存控制器" 的创建很简单，首先是定义一个 MemoryRegion 实例，这里需要为实例分配好内存，接着并调用 memory_region_allocate_system_memory() 函数, 传入的第一个参数是 "内存控制器" 的 MemoryRegion, 第二个参数则是 "内存控制器" MemoryRegion 对象的 Owner，这里可以默认为 NULL. 函数的第三个参数是 "内存控制器" 的名字，这里取名为 "BiscuitOS-memory-control". 函数的最后一个参数是 "内存控制器" 管理的内存大小，这里设置为 16MiB. 执行完上面的命令之后，系统会为 "内存控制器" MemoryRegion 分配一块 "内存条" RAMBlock，该 RAMBlock 上就是 QEMU 为虚拟机分配的虚拟内存。通过内存条可以获得内存条在虚拟机地址总线上的物理地址，即 MemoryBank_base 的值，也可以获得 QEMU 在 host 端为内存条分配的虚拟内存，即 BiscuitOS_HVA 的值.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000907.png)

在 BiscuitOS 上运行上面代码，通过命令 "cat /proc/iomem" 查看虚拟机内部的物理内存拓扑, 并未发现任何新 "内存控制器" 的内存信息 (可以在上面的代码中将 MemoryBank_base 的值打印出来，并在打印之后添加 10s 左右的延时查看该值，这里根据实践平台假设物理地址是 0x20000000)。此时通过 "QEMU monitor" 查看 qom-tree 信息，可以看到新添加的 "内存控制器" 的 MemoryRegion:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000908.png)

那么 "内存控制器" MemoryRegion 通过 "内存条" RAMblock 维护了虚拟机的物理内存，为什么虚拟机还是看不到这部分内存呢? 这里就得了解一下 "物理区域" MemoryRegion 的作用了，"物理区域" MemoryRegion 是从 "内存控制器" MemoryRegion 中管理的内存中划分出一块区域，这部区域会暴露给虚拟机。"物理区域" MemoryRegion 的 alias_offset 成员指明了需要使用 "内存控制器" MemoryRegion 维护内存的偏移，通过 alias_offset 成员，再结合 "内存条" RAMBlock 在虚拟机内的物理地址，就可以计算出 "物理区域" MemoryRegion 描述的内存在虚拟机内的物理地址，并使用 addr 成员维护该值，另外 "物理区域" MemoryRegion 的 size 成员指明了其描述内存的长度。那么结合上面的逻辑框图可以知道 "物理区域" MemoryRegion (R0) 描述区域的起始物理地址等于 "R0->alias_offset + GPA:addr3". 最后代码逻辑参考如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000909.png)

基于 "内存控制器" 创建代码，这里新建里一个 "物理区域" MemoryRegion 实例，并调用 memory_region_init_alias() 函数, 该函数的第一个参数是 "物理区域" MemoryRegion 实例，第二个参数则是 "物理区域" MemoryRegion 对象的 Owner，这里使用默认值 NULL. 函数的第三个参数是 "物理区域" MemoryRegion 的名字，第四个参数则是新建 "内存控制器" MemoryRegion, 第五个参数则是 "物理区域" 描述的区域在 "内存控制器" 管理的内存内的偏移，最后一个参数则用于说明 "物理区域" 的长度. 通过该函数 "物理区域" MemoryRegion 已经从 "内存控制器" MemoryRegion 中接管过了一部分内存，接下来 "物理区域" MemoryRegion 将其描述的内存注册到系统内存中使虚拟机可以看到这段内存，此时调用函数 memory_region_add_subregion(), 函数的第一个参数是 "系统总线" MemoryRegion，其为 system_memory. 第二个参数是 "物理区域" MemoryRegion 描述的物理内存在虚拟机物理内存的起始地址，这里使用变量 BiscuitOS_GPA 进行描述，此时该变量的值为内存条在虚拟机上物理地址. 函数的第三个参数是 "物理区域" MemoryRegion 的长度，此处使用 BiscuitOS_memory_rsize 进行描述，该值为 1MiB. 通过调用该参数系统可以看到 "内存控制器" 维护的内存，并告诉系统总线 "物理区域" MemoryRegion 描述的内存是一块物理内存区域。最后还需要通过 E820 表告诉 "物理区域" MemoryRegion 描述的物理内存是作为系统可用内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000910.png)

在 BiscuitOS 上运行并通过命令 "cat /proc/iomem" 查看虚拟机的地址总线上的拓扑结构，发现 "物理区域" MemoryRegion 描述的物理内存虚拟机已经可以看到，其范围是 "0x20000000-0x20100000", 并且是可用的物理内存, 这里也可以看到 "内存控制器" MemoryRegion 没有被 "物理区域" MemoryRegion 暴露给虚拟机的区域，系统是不感知的。另外使用 “QEMU monitor” 查看虚拟的 mtree 信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000911.png)

可以通过 QEMU QMH 查看虚拟机的 mtree 信息，看到 “物理区域” MemoryRegion 信息已经注册到系统内存总线. MemoryRegion 对象可以描述不同功能的组件，那么如何区分 MemoryRegion 对象描述的组件是 "内存控制器" 呢? 由于 "内存控制器" MemoryRegion 包含真正的内存，因此其 ram 成员为 True，因此可以通过这个进行判断.

###### 源码逻辑

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000923.png)

QEMU 中创建一个 "内存控制器" MemoryRegion 和 "内存条" RAMBlock 的代码逻辑基本如上，但考虑到实际与 NUMA、大页内存等因素有关会存在差异，这里就以通用的 4K 页大小以及非 NUMA 的情况进行讲解，其余情况参考分析.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000924.png)

memory_region_init_ram_shared_nomigrate() 函数是流程中第一个比较重要的调用，该函数首先调用 memory_region_init() 函数初始化了 "内存控制器" MemoryRegion 对象，将该对象的 ram 成员设置为 true，以此和其他 MemoryRegion 对象区分开来，terminates 成员设置为 true 则表明 "内存控制器" MemoryRegion 对象是叶子节点不包含任何子 MemoryRegion 对象。最后调用 qemu_ram_alloc() 函数为 "内存控制器" MemoryRegion 分配 "内存条" RAMBlock。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000925.png)

qemu_ram_alloc_internal() 函数是创建 "内存条" RAMBlock 的核心函数。函数首先通过 g_malloc() 实例化一个 "内存条" RAMBlock，然后将 "内存条" RAMBlock的 mr 成员指向 "内存控制器"，并且设置了了 "内存条" RAMBlock 物理页的大小，如果此时 "内存条" RAMBlock 已经分配内存，那么 "内存条" RAMBlock 的标志集合中添加 RAM_PREALLOC 标志，如果此时 resizeable 参数使能，那么 "内存条" RAMBlock 的标志集合中也添加 RAM_RESIZEABLE. 最后调用 ram_block_ad() 函数将 "内存条" RAMBlock 加入到 QEMU 全局的 ram_list 链表中。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000926.png)

ram_block_add() 函数比较长，但这里比较关心几个细节，首先是函数调用 find_ram_offset() 函数获得上一个 "内存条" RAMblock 插入到虚拟机的结束物理地址，该地址将成为新 "内存条" RAMBlock 在虚拟机地址总线上的起始物理地址，find_ram_offset() 内部细节这里不展开讲，大概的逻辑是 QEMU 使用 ram_list 链表将所有的 "内存条" RAMBlock 按 offset 成员从小到大按循序排列，并在这些 "内存条" RAMBlock 空隙中找到一个容得下新 "内存条" RAMBlock 的位置, 该位置就是 "内存条" RAMBlock 在虚拟机内的物理地址. 接下来函数将调用 phys_mem_alloc() 函数为 "内存条" RAMBlock 分配 Host 端的虚拟内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000927.png)

qemu_ram_mmap() 函数是 QEMU 为 "内存条" RAMBlock 分配虚拟内存的核心函数. 函数第一次按 "size + align" 的长度调用 mmap() 函数按私有匿名映射的方式映射了一段 PROT_NONE 虚拟内存. 接着如果 shared 参数为真，那么使用共享映射方式，反之使用私有映射方式，如果参数 fd 不为空，那么使用文件映射方式，反之使用匿名映射方式，函数接着使用 mmap() 函数进行映射，如果此时映射的长度小于第一次，那么第一次多出来的虚拟内存会被当做 GUARD 功能使用，即用于隔离作用。

> [虚拟机新添加一个内存控制器](#C1X3)
>
> [虚拟机新添加一个内存条](#C1X4)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C100"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000G.jpg)

#### MemoryRegion 与 GPA 之间的关系

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000884.png)

AddressSpace 与 MemoryRegion 的树状拓扑结构构成了虚拟机的 "内存总线"、"内存控制器" 以及 "Guest 物理区域". 三者的关系如上图，虚拟机使用 address_space_memory 表示虚拟机可以使用的物理内存空间，其 root 成员指向的 MemoryRegion (Root) 为虚拟机的 "内存总线"。"内存控制器" MemoryRegion (RC0) 用于管理一块 "内存条" RAMBlock (RB), 该 "内存条" 被插入到虚拟机上，可以获得 "内存条" 在虚拟机物理地址总线上的地址，该地址就是 RAMBlock (RB) 的 offset 成员，"Memory Bank" 区域为虚拟机内地址总线上内存条区域 (虚拟机不一定全部能感知到整块内存条). "物理区域" MemoryRegion 用于从 "内存控制器" MemoryRegion (RC0) 管理的 "内存条" RAMBlock (RB) 上划分一段内存进行维护，"物理区域" MemoryRegion 通过 alias 成员指向唯一的 "内存管理器" MemoryRegion (RC0). "物理区域" MemoryRegion 的 addr 成员即通过其在从 "内存条" 内的偏移加上 "内存条" 在虚拟机地址总线上的物理地址所得。最后 "内存总线" MemoryRegion (Root) 以 subregions 成员为链表头，将所有 "物理区域" MemoryRegion 的 subregions_link 成员链接在一起，这样就构成了虚拟机 "内存总线" 拓扑架构, 因此虚拟机能看到的物理内存就是 "内存总线" 上 "物理区域" 构成的物理内存. 通过上面的分析可以获得如下的地址关系: 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000921.png)

从上面的关系可以知道 "内存控制器" MemoryRegion 管理的 "内存条" RAMBlock 插入到虚拟机地址总线上物理地址成为了 MemoryRegion 与 GPA 之间的关键桥梁，"物理区域" MemoryRegion 的 GPA 地址都是通过其在 "内存条" 中的偏移加上 "内存条" 在虚拟机地址总线上的物理地址。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C101"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### MemoryRegion 与 HVA 之间的关系

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000886.png)

AddressSpace 与 MemoryRegion 的树状拓扑结构构成了虚拟机的 "内存总线"、"内存控制器" 以及 "Guest 物理区域". 三者的关系如上图，虚拟机使用 address_space_memory 表示虚拟机可以使用的物理内存空间，其 root 成员指向的 MemoryRegion (Root) 为虚拟机的 "内存总线"。"内存控制器" MemoryRegion (R3) 用于管理一块 "内存条" RAMBlock (RB0), 该 "内存条" 被插入到虚拟机上，可以获得 "内存条" 在虚拟机物理地址总线上的地址，该地址就是 RAMBlock (RB0) 的 offset 成员，"Memory Bank" 区域为虚拟机内地址总线上内存条区域 (虚拟机不一定全部能感知到整块内存条). 另外 QEMU 在 Host 端会为 "内存条" RAMBlock (RB0) 分配一段虚拟内存，并使用 host 成员指向这段虚拟内存，这段虚拟内存就是虚拟机运行时使用的内存。"物理区域" MemoryRegion 用于从 "内存控制器" MemoryRegion (R3) 管理的 "内存条" RAMBlock (RB) 上划分一段内存进行维护, 具体划分 "内存条" RAMBlock (RB0) 的哪一段通过 alias_offset 成员指定。"物理区域" MemoryRegion 通过 alias 成员指向唯一的 "内存管理器" MemoryRegion (RC0). "物理区域" MemoryRegion 的 addr 成员即通过其在从 "内存条" 内的偏移 alias_offset 加上 "内存条" 在虚拟机地址总线上的物理地址所得。最后 "内存总线" MemoryRegion (Root) 以 subregions 成员为链表头，将所有 "物理区域" MemoryRegion 的 subregions_link 成员链接在一起，这样就构成了虚拟机 "内存总线" 拓扑架构, 因此虚拟机能看到的物理内存就是 "内存总线" 上 "物理区域" 构成的物理内存. 通过上面的分析可以获得如下的地址关系: 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000922.png)

从上面的关系可以知道 "内存控制器" MemoryRegion 管理的 "内存条" RAMBlock 在 host 成员，其作为 QEMU 为其分配的虚拟内存是沟通 "物理区域" MemoryRegion 与 HVA 挂钩的关键桥梁. 另外 "物理区域" MemoryRegion 的 alias_offset 也决定了其维护 "内存条" RAMBlock 的偏移，也是关键因素.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C102"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

####  MemoryRegion 与 HPA 之间的关系

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000739.png)

当 QEMU 为虚拟机分配了虚拟内存并与虚拟机的物理内存进行绑定，但此时的 QEMU 分配的虚拟内存并没有与真实的物理内存进行映射，因此当虚拟机访问虚拟机的物理内存时，系统会遇到 EPT 页表没有建立，那么虚拟机就会 VM_EXIT 退出到 Host，此时 kvm 模块就会为没有建立 EPT 页表的 GPA 建立页表，此时 kvm 模块首先找到 HVA, 如果 HVA 没有建立 HPA 的映射，那么此时会主动触发缺页建立该映射，建立完毕之后 kvm 模块再建立 GPA 到 HPA 的映射，因此在 QEMU 并不感知 HPA 的存在。在有的虚拟化方案中，当 QEMU 为虚拟机分配虚拟内存时就为该虚拟内存分配了物理内存，并建立好页表，那么虚拟机运行时对于没有建立 EPT 页表的 GPA，只需建立 EPT 页表即可。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000888.png)

MemoryRegion 与 HPA 的关系可以从图中可以看出 AddressSpace 通过 "内存总线" MemoryRegion、"内存控制器" MemoryRegion 和 "物理内存区域" MemoryRegion 只维护了 HVA 和 GPA 的关系，至于 GPA 与 HPA 的关系通过 kvm 模块进行维护，QEMU 不进行维护. 在上图中可以看出 R0 对应的虚拟内存 HAV:addr4, 该地址只有建立页表才能与物理内存 HPA:addr5 建立映射关系。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------------------

<span id="C1X3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000F.jpg)

#### 虚拟机新添加一个内存控制器

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000886.png)

在 QEMU 中使用 MemoryRegion 作为虚拟机内存管理的基础数据，其可以用于描述一个 "内存控制器"，所谓内存控制器便是用于管理一段真实的内存，这里的内存分作两部分，一部分是虚拟机的物理内存; 另外一部分这是 Host 端 QEMU 为虚拟机分配的虚拟内存。"内存控制器" 用来管理这两部分内存，"物理区域" MemoryRegion 可以描述 "内存控制器" MemoryRegion 中部分内存，如上图 "物理区域" MemoryRegion 将其 alias 指向 "内存控制机"，并通过一定的操作可以获得描述部分内存的权利。QEMU 要创建一个内存控制器的代码逻辑如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000906.png)

"内存控制器" 的创建很简单，首先是定义一个 MemoryRegion 实例，这里需要为实例分配好内存，接着并调用 memory_region_allocate_system_memory() 函数, 传入的第一个参数是 "内存控制器" 的 MemoryRegion, 第二个参数则是 "内存控制器" MemoryRegion 对象的 Owner，这里可以默认为 NULL. 函数的第三个参数是 "内存控制器" 的名字，这里取名为 "BiscuitOS-memory-control". 函数的最后一个参数是 "内存控制器" 管理的内存大小，这里设置为 16MiB. 执行完上面的命令之后，系统会为 "内存控制器" MemoryRegion 分配一块 "内存条" RAMBlock，该 RAMBlock 上就是 QEMU 为虚拟机分配的虚拟内存。通过内存条可以获得内存条在虚拟机地址总线上的物理地址，即 MemoryBank_base 的值，也可以获得 QEMU 在 host 端为内存条分配的虚拟内存，即 BiscuitOS_HVA 的值.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000907.png)

在 BiscuitOS 上运行上面代码，通过命令 "cat /proc/iomem" 查看虚拟机内部的物理内存拓扑, 并未发现任何新 "内存控制器" 的内存信息 (可以在上面的代码中将 MemoryBank_base 的值打印出来，并在打印之后添加 10s 左右的延时查看该值，这里根据实践平台假设物理地址是 0x20000000)。此时通过 "QEMU monitor" 查看 qom-tree 信息，可以看到新添加的 "内存控制器" 的 MemoryRegion:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000908.png)

有了上面的代码只是创建了一个 "内存控制器"，而要让虚拟机看到并使用这些内存，还需要使用 "物理区域" MemoryRegion 使用该 "内存控制器" 上的内存，那么完整代码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000909.png)

基于 "内存控制器" 创建代码，这里新建里一个 "物理区域" MemoryRegion 实例，并调用 memory_region_init_alias() 函数, 该函数的第一个参数是 "物理区域" MemoryRegion 实例，第二个参数则是 "物理区域" MemoryRegion 对象的 Owner，这里使用默认值 NULL. 函数的第三个参数是 "物理区域" MemoryRegion 的名字，第四个参数则是新建 "内存控制器" MemoryRegion, 第五个参数则是 "物理区域" 描述的区域在 "内存控制器" 管理的内存内的偏移，最后一个参数则用于说明 "物理区域" 的长度. 通过该函数 "物理区域" MemoryRegion 已经从 "内存控制器" MemoryRegion 中接管过了一部分内存，接下来 "物理区域" MemoryRegion 将其描述的内存注册到系统内存中使虚拟机可以看到这段内存，此时调用函数 memory_region_add_subregion(), 函数的第一个参数是 "系统总线" MemoryRegion，其为 system_memory. 第二个参数是 "物理区域" MemoryRegion 描述的物理内存在虚拟机物理内存的起始地址，这里使用变量 BiscuitOS_GPA 进行描述，此时该变量的值为内存条在虚拟机上物理地址. 函数的第三个参数是 "物理区域" MemoryRegion 的长度，此处使用 BiscuitOS_memory_rsize 进行描述，该值为 1MiB. 通过调用该参数系统可以看到 "内存控制器" 维护的内存，并告诉系统总线 "物理区域" MemoryRegion 描述的内存是一块物理内存区域。最后还需要通过 E820 表告诉 "物理区域" MemoryRegion 描述的物理内存是预留还是作为可用内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000910.png)

在 BiscuitOS 上运行并通过命令 "cat /proc/iomem" 查看虚拟机的地址总线上的拓扑结构，发现 "物理区域" MemoryRegion 描述的物理内存虚拟机已经可以看到，其范围是 "0x20000000-0x20100000", 并且是可用的物理内存. 另外使用 "QEMU monitor" 查看虚拟的 mtree 信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000911.png)

可以通过 QEMU QMH 查看虚拟机的 mtree 信息，看到 "物理区域" MemoryRegion 信息已经注册到系统内存总线。至此添加一个新的 "内存控制器" 并使用其使用物理内存的分析到此为止.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C1X4"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### 虚拟机新添加一个内存条

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000886.png)

在 QEMU 中使用 MemoryRegion 作为虚拟机内存管理的基础数据，其可以用于描述一个 "内存控制器"，所谓内存控制器便是用于管理一段真实的内存，这里的内存分作两部分，一部分是虚拟机的物理内存; 另外一部分这是 Host 端 QEMU 为虚拟机分配的虚拟内存。"内存控制器" 通过内存条管理内存，那么内存条就需要能维护两部分内存。在 QEMU 中使用 RAMBlock 描述一个内存条，它用于构建虚拟机物理内存 GPA 到 Host 端 HVA 之间的映射关系。QEMU 要创建一个内存条的代码逻辑如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000906.png)

"内存条" 的创建很简单，由于其与 "内存控制器" 绑定在一起，因此与 "内存控制器" 的创建一致，首先是定义一个 MemoryRegion 实例，这里需要为实例分配好内存，接着并调用 memory_region_allocate_system_memory() 函数, 传入的第一个参数是 "内存控制器" 的 MemoryRegion, 第二个参数则是 "内存控制器" MemoryRegion 对象的 Owner，这里可以默认为 NULL. 函数的第三个参数是 "内存控制器" 的名字，这里取名为 "BiscuitOS-memory-control". 函数的最后一个参数是 "内存控制器" 管理的内存大小，这里设置为 16MiB. 执行完上面的命令之后，系统会为 "内存控制器" MemoryRegion 分配一块 "内存条" RAMBlock，通过内存条可以获得内存条在虚拟机地址总线上的物理地址，即 MemoryBank_base 的值，也可以获得 QEMU 在 host 端为内存条分配的虚拟内存，即 BiscuitOS_HVA 的值. 在 BiscuitOS 上运行上面代码，此时通过 "QEMU monitor" 查看 qom-tree 信息，可以看到新添加的 "内存控制器" 的 MemoryRegion:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000908.png)

有了上面的代码只是创建了一个 "内存控制器" 和一个 "内存条"，而要让虚拟机看到并使用这些内存，还需要使用 "物理区域" MemoryRegion 使用该 "内存控制器" 上的内存，那么完整代码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000912.png)

基于 "内存控制器" 创建代码，这里新建里一个 "物理区域" MemoryRegion 实例，并调用 memory_region_init_alias() 函数, 该函数的第一个参数是 "物理区域" MemoryRegion 实例，第二个参数则是 "物理区域" MemoryRegion 对象的 Owner，这里使用默认值 NULL. 函数的第三个参数是 "物理区域" MemoryRegion 的名字>，第四个参数则是新建 "内存控制器" MemoryRegion, 第五个参数则是 "物理区域" 描述的区域在 "内存控制
器" 管理的内存内的偏移，最后一个参数则用于说明 "物理区域" 的长度. 通过该函数 "物理区域" MemoryRegion 已经从 "内存控制器" MemoryRegion 中接管过了一部分内存，接下来 "物理区域" MemoryRegion 将其>描述的内存注册到系统内存中使虚拟机可以看到这段内存，此时调用函数 memory_region_add_subregion(), 函数的第一个参数是 "系统总线" MemoryRegion，其为 system_memory. 第二个参数是 "物理区域" MemoryRegion 描述的物理内存在虚拟机物理内存的起始地址，这里使用变量 BiscuitOS_GPA 进行描述，此时该变量的
值为 0x200000000. 函数的第三个参数是 "物理区域" MemoryRegion 的长度，此处使用 BiscuitOS_memory_rsize 进行描述，该值为 1MiB. 通过调用该参数系统可以看到 "内存控制器" 维护的内存，并告诉系统总线 "物理区域" MemoryRegion 描述的内存是一块物理内存区域。最后还需要通过 E820 表告诉 "物理区域" MemoryRegion 描述的物理内存是预留还是作为可用内存。如果此时想获得 "物理区域" MemoryRegion 对应的 HVA 地址，可以使用内存块的虚拟地址加上 "物理区域" MemoryRegion 在 "内存控制器" 内的偏移即可:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000910.png)

在 BiscuitOS 上运行并通过命令 "cat /proc/iomem" 查看虚拟机的地址总线上的拓扑结构，发现 "物理区域" MemoryRegion 描述的物理内存虚拟机已经可以看到，其范围是 "0x20000000-0x20100000", 并且是可用的物理内存。开发者可能会疑问为什么是 0x20000000, 其实可以在 QEMU 代码案例中将内存条在虚拟机中的物理地址 MemoryBank_base 打印出来即可，由于 QEMU 启动特别快，可以在打印之后添加 10 秒的延时以此观察内存条的物理地址. 另外使用 "QEMU monitor" 查看虚拟的 mtree 信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000911.png)

可以通过 QEMU QMH 查看虚拟机的 mtree 信息，看到 "物理区域" MemoryRegion 信息已经注册到系统内存总线。同理可以使用该方法添加多条内存条。至此添加一个新的 "内存控制器" 并使用其使用物理内存的分析到此为止.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C1X5"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000R.jpg)

#### 虚拟机新添加一段可用物理内存

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000884.png)

在 QEMU QOM 架构中，使用 MemoryRegion 作为虚拟机内存管理的基础数据结构，将 MemoryRegion 划分成 "内存总线" MemoryRegion、"内存控制器" MemoryRegion 与 "内存条" 、"物理区域" MemoryRegion. 为了向虚拟机添加一块可用的物理内存，那么需要准备好上面三种 MemoryRegion. QEMU 中 "内存总线" MemoryRegion 为 system_memory, 对于 "内存控制器" MemoryRegion 与 "内存条" 需要独立建立，其管理着真正的内存. 最后 "物理区域" MemoryRegion 只是用于控制 "内存控制器" 维护的内存暴露给虚拟机能看到的内存信息.这里给出一个同样案例来描述如何为虚拟机添加一块可用物理内存:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000912.png)

在上面的案例代码中，BiscuitOS_memory_control 为 "内存控制器" MemoryRegion, 首先为其分配内存，并调用 memory_region_allocate_system_memory() 函数，函数的第一个参数是 "内存控制器" MemoryRegion, 第二个参数则是 "内存控制器" MemoryRegion 对象的 Owner，这里使用默认值 NULL. 第三个参数则为 "内存控制器" MemoryRegion 的名字，第四个参数则为 "内存控制器" MemoryRegion 的 "内存条" 长度，这里设置为 16MiB。当函数调用成功之后，可以通过 "内存条" 获得其在虚拟机地址总线上的物理地址。接下来创建一个 "物理区域" MemoryRegion, 用于管理 "内存控制器" 的 "内存条" 暴露给虚拟机物理内存。函数使用 BiscuitOS_memory_size 描述暴露给虚拟机物理内存的长度为 1MiB，另外 BiscuitOS_GPA 变量表示 "物理区域" MemoryRegion 在虚拟机内的物理地址。通过调用 memory_region_init_alias() 函数初始化了 "物理区域" MemoryRegion, 接着调用 memory_region_add_subregion() 函数将 "内存控制器" 添加到 "内存总线"，并将 "物理区域" MemoryRegion 管理的内存暴露给虚拟机，该函数的第一个参数是 "内存总线" Memoryregion syste_memory, 第二个参数是 "物理区域" MemoryRegion 管理的内存在虚拟机的物理地址，最后一个参数是 "物理区域" MemoryRegion. 通过上面的操作之后，"物理区域" MemoryRegion 维护的物理区域已经暴露给虚拟机，但是这些内存只有虚拟机的 BIOS 能看到，为了让虚拟机的操作系统可以看到，这里需要向 E820 表中填入内存信息，因此调用 e820_add_entry() 函数将 "物理区域" MemoryRegion 的物理区域传递给虚拟机的操作系统，该函数地第一个参数是这段物理内存的起始地址，第二个参数是这段物理内存的长度，最后一个参数表示这段物理内存区域对操作系统是可用还是预留，这里设置为 E820_RAM, 即对虚拟机操作系统可见.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000910.png)

在 BiscuitOS 上运行并通过命令 "cat /proc/iomem" 查看虚拟机的地址总线上的拓扑结构，发现 "物理区域" MemoryRegion 描述的物理内存虚拟机已经可以看到，其范围是 "0x20000000-0x20100000", 并且是可用的物理内存。开发者可能会疑问为什么是 0x20000000, 其实可以在 QEMU 代码案例中将内存条在虚拟机中的物理地址 MemoryBank_base 打印出来即可，由于 QEMU 启动特别快，可以在打印之后添加 10 秒的延时以此观察内存条的物理地址. 另外使用 "QEMU monitor" 查看虚拟的 mtree 信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000911.png)

可以通过 QEMU QMH 查看虚拟机的 mtree 信息，看到 "物理区域" MemoryRegion 信息已经注册到系统内存总线。同理可以使用该方法向虚拟机添加多个可用物理区域。至此添加一个段可用物理内存分析到此为止.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C1X6"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000E.jpg)

#### 虚拟机新添加一段预留物理内存

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000884.png)

在 QEMU QOM 架构中，使用 MemoryRegion 作为虚拟机内存管理的基础数据结构，将 MemoryRegion 划分成 "内存总线" MemoryRegion、"内存控制器" MemoryRegion 与 "内存条" 、"物理区域" MemoryRegion. 为了向虚拟机添加一块可用的物理内存，那么需要准备好上面三种 MemoryRegion. QEMU 中 "内存总线" MemoryRegion 为 system_memory, 对于 "内存控制器" MemoryRegion 与 "内存条" 需要独立建立，其管理着真正的内存. 最后 "物理区域" MemoryRegion 只是用于控制 "内存控制器" 维护的内存暴露给虚拟机能看到的内存信息.这里给出一个同样案例来描述如何为虚拟机添加一块预留物理内存:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000913.png)

在上面的案例代码中，BiscuitOS_memory_control 为 "内存控制器" MemoryRegion, 首先为其分配内存，并调用 memory_region_allocate_system_memory() 函数，函数的第一个参数是 "内存控制器" MemoryRegion, 第二个参数则是 "内存控制器" MemoryRegion 对象的 Owner，这里使用默认值 NULL. 第三个参数则为 "内存控制器" MemoryRegion 的名字，第四个参数则为 "内存控制器" MemoryRegion 的 "内存条" 长度，这里设置为 16MiB。当函数调用成功之后，可以通过 "内存条" 获得其在虚拟机地址总线上的物理地址。接下来创建一个 "物理区域" MemoryRegion, 用于管理 "内存控制器" 的 "内存条" 暴露给虚拟机物理内存。函数使用 BiscuitOS_memory_size 描述暴露给虚拟机物理内存的长度为 1MiB，另外 BiscuitOS_GPA 变量表示 "物理区域" MemoryRegion 在虚拟机内的物理地址。通过调用 memory_region_init_alias() 函数初始化了 "物理区域" MemoryRegion, 接着调用 memory_region_add_subregion() 函数将 "内存控制器" 添加到 "内存总线"，并将 "物理区域" MemoryRegion 管理的内存暴露给虚拟机，该函数的第一个参数是 "内存总线" Memoryregion syste_memory, 第二个参数是 "物理区域" MemoryRegion 管理的内存在虚拟机的物理地址，最后一个参数是 "物理区域" MemoryRegion. 通过上面的操作之后，"物理区域" MemoryRegion 维护的物理区域已经暴露给虚拟机，但是这些内存只有虚拟机的 BIOS 能看到，为了让虚拟机的操作系统可以看到，这里需要向 E820 表中填入内存信息，因此调用 e820_add_entry() 函数将 "物理区域" MemoryRegion 的物理区域传递给虚拟机的操作系统，该函数地第一个参数是这段物理内存的起始地址，第二个参数是这段物理内存的长度，最后一个参数表示这段物理内存区域对操作系统是可用还是预留，这里设置为 E820_RESERVED, 即对虚拟机操作系统预留这段物理内存.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000914.png)

在 BiscuitOS 上运行并通过命令 "cat /proc/iomem" 查看虚拟机的地址总线上的拓扑结构，发现 "物理区域" MemoryRegion 描述的物理内存虚拟机已经可以看到，其范围是 "0x20000000-0x20100000", 并且是预留的物理内存，但由于 0x1ffe0000-0xfebfffff 都是预留内存，因此 0x20000000-0x20100000 被包含在其中。开发者可能会疑问为什么是 0x20000000, 其实可以在 QEMU 代码案例中将内存条在虚拟机中的物理地址 MemoryBank_base 打印出来即可，由于 QEMU 启动特别快，可以在打印之后添加 10 秒的延时以此观察内存条的物理地址. 另外开发者也可以查看 BIOS 传递给内核的 E820 表，可以从 dmesg 中获得，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000915.png)

从 dmesg 中可以看到 BIOS 传递给内核的原始 E820 表，表中表明 1ffe0000-0x0200fffff 这段是系统预留内存，符合预期。另外使用 "QEMU monitor" 查看虚拟的 mtree 信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000911.png)

可以通过 QEMU QMH 查看虚拟机的 mtree 信息，看到 "物理区域" MemoryRegion 信息已经注册到系统内存总线。同理可以使用该方法向虚拟机添加多个预留内存区域。至此添加一个段预留物理内存分析到此为止.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C1X7"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### MemoryRegion 对象添加属性

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000904.png)

在 QEMU 的 QOM 中, MemoryRegion 作为 TYPE_MEMORY_REGION 类的对象，其在初始化实例时就自带了多个属性，具体细节可以参考 [MemoryRegion 对象属性](#C107) 章节。本节重点介绍如何为 MemoryRegion 对象添加新的属性，以及如何使用这些属性. 正如上图所示 QEMU 使用 struct MemoryRegion 数据结构描述 MemoryRegion 对象，MemoryRegion 对象可以通过 QOM 机制，将 struct MemoryRegion 的成员通过属性暴露给其他模块使用。对于对象的属性，可以提供 SET 和 GET 接口，另外属性有不同的类型，例如 int 整数型、str 字符串型、以及 bool 型。这么多不同类型的属性，QOM 提供了一套接口用于处理不同类型的属性，那么接下来介绍如何为 MemoryRegion 对象添加属性。首先在 struct MemoryRegion 数据结构中找到一个需要属性化的成员:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000916.png)

上图是 struct MemoryRegion 数据结构的定义，这里选取 alias_offset 为例子进行讲解，其数据类型是 hwaddr (uint64_t), 然后使用如下函数动态添加属性:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000917.png)

QOM 提供了 object_property_add() 函数动态添加一个属性，该函数的第一个参数是 MemoryRegion 对象，第二个参数是属性的名字，第三个参数是属性的类型，第四个参数是属性的 GET 函数，也就是读取属性值的接口，第五个函数是属性的 SET 函数，也就是设置属性值的接口，接下来两个参数均使用默认值 NULL, 最后一个参数是错误码。属性不一定强制具有 SET 或者 GET 接口，如果需要该接口可以参考上图的案例去写。BiscuitOS_property_alias_offset_get() 函数是属性的 GET 接口，其首先通过 MEMORY_REGION() 宏将 MemoryRegion 对象转换成 struct MemoryRegion 数据结构体，然后从 struct MemoryRegion 读取 alias_offset 成员的值，接着通过 visit_type_uint64() 函数传递给 QOM。BiscuitOS_property_alias_offset_set() 函数则是属性的 SET 接口，该函数首先通过 MEMORY_REGION() 宏获得 struct MemoryRegion 数据结构，然后调用 visit_type_uint64() 函数从 QOM 中读取新的属性值，最后将新值赋值到 alias_offset 成员. 以上便是一个完整的添加属性. 接下来就是读取或者设置属性，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000918.png)

QOM 提供了 object_property_get_uint() 函数用于读取 MemoryRegion 对象的指定 uint64 类型的属性，该函数的第一个参数指向 MemoryRegion 对象，第二个参数为需要读取的属性名，第三个参数为错误码。另外 QOM 提供了 object_property_set_uint() 函数设置一个 uint64 类型的属性，该函数的第一个参数指向 MemoryRegion 对象，第二个参数为新设置的属性值，第三个参数则为属性的名字，第四个参数为错误码。结合上面的代码在 BiscuitOS 上实践，由于 QEMU 启动快，因此在打印属性值的时候添加延时以便查看打印的结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000919.png)

从 BiscuitOS 启动的时候，QEMU 打印的日志可以看出 alias_offset 属性设置成功，并成功读取新属性的值。以上便是添加一个属性的完整步骤。另外其他类型的属性 QOM 也提供了一套属性接口函数来操作属性。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)








