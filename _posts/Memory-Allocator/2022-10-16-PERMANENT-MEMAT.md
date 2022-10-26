---
layout: post
title:  "Permanent Mapping Memory Allocator"
date:   2022-10-16 12:00:00 +0800
categories: [MMU]
excerpt: Permanent Mapping.
tags:
  - Compile-Time Allocator
  - Linux 6.0+
  - Fixmap Allocator Sub
  - BiscuitOS DOC 3.0+
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [永久映射分配器原理](#A)
>
> - [永久映射分配器实践](#C)
>
> - [永久映射分配器使用](#B)
>
>   - [永久映射外设 MMIO/物理内存](#B1)
>
>   - [永久映射为不带 Cache](#B2)
>
>   - [永久映射 IO](#B3)
>
>   - [新增永久映射](#B4)
>
> - [永久映射分配器 API 合集](#B5)
>
> - [永久映射分配器使用场景](#X)
>
>   - [VSYSCALL 应用场景](#C1)
>
>   - [EarlyCon/EHCI Debug Port 应用场景](#C2)
>
>   - [LAPIC 中断控制器应用场景](#C3)
>
>   - [IOAPIC 中断控制器应用场景](#C4)
>
>   - [KMAP 临时映射分配器应用场景](#C5)
>
>   - [PCIe Configuration Space MMIO-Way 应用场景](#C6)
>
>   - [General Hardware Error: GHES 应用场景](#C7)
>
> - 永久映射分配器 BUG 合集
>
>   - [永久映射内存分配器占用的内存超限 BUG](/blog/Memory-ERROR/#A00A05)
>
> - 永久映射分配器进阶研究
>
>   - [BiscuitOS Doc1.0 &2.0 永久映射内存分配器](/blog/HISTORY-FIXMAP/)
>
>   - [永久映射内存分配器历史补丁](/blog/HISTORY-FIXMAP/#H)
>
>   - [用户空间实现: 永久映射内存分配器](/blog/Memory-Userspace/#P)

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### 永久映射分配器原理

![](/assets/PDB/HK/HK000226.png)

**Permanent Mapping Memory Allocator** 称为**永久映射内存分配器**, 其属于**固定映射内存分配器**的一个分支，其特点就是在编译阶段就可以确定某段虚拟地址分配给特定的功能使用。从实现角度来看，永久映射内存分配器通过在编译阶段从内核的虚地址地址空间占用一段虚拟内存，再等到系统运行之后，通过将这个区域的某段虚拟地址映射到特定的物理内存或者外设寄存器上，只要不释放那么这段映射会永久有效.

![](/assets/PDB/HK/TH001990.png)

**"永久"** 到底有多久? 为什么是 "永久"? 可以从三个维度来回答这个问题: 维度一内核从启动到系统真正运行会经历不同的阶段，每个阶段会临时建立一些临时映射用于完成该阶段的任务，当任务完成之后映射也会清除，例如启动阶段的恒等映射阶段。但对于永久映射分配器分配的虚拟内存，一旦页表建立，只要映射关系不解除，那么映射关系可以一直存在，并且其页表从编译阶段就存在.

![](/assets/PDB/HK/TH002040.png)

维度二对比常见的内存分配器，内核或者应用程序需要分配虚拟内存，分配器会动态从虚拟空间分配一块可用的虚拟内存区域，然后通过直接或缺页的方式与物理内存或外设寄存器建立页表，最后才能使用这块虚拟内存。但永久映射分配器分配的虚拟内存就不同，其可以在源码编写阶段就可以知道虚拟内存值，且到系统运行这个虚拟地址值的含义都不会变. 因此永久映射内存分配器称为 **Compile-Time** 分配器.

![](/assets/PDB/HK/TH002039.png)

维度三进程的地址空间包括了堆、栈、MMAP 区域等，这些区域都不是固定位置，即这些区域的长度和起始位置可能不相同，但使用永久映射分配器分配的虚拟内存都是保持一致的，且位于内核空间的末尾 0xffffffffff7ff000 向下的区域，例如 VSYSCALL 使用的虚拟地址对所有的进程都是一样的，因此永久还体现在所有进程看到的都是一致的.

![](/assets/PDB/HK/TH001991.png)

永久内存分配器的实现原理如上图，分配器维护了一个索引，每个索引与特定的功能绑定在一起，因此可以从索引对应的宏知道这段虚拟地址的用途。分配器从系统的虚拟地址空间分配了 FIXADDR_START 到 FIXADDR_TOP 的区域，其长度为 FIXADDR_SIZE. 分配器从 FIXADD-R_TOP 向地址将虚拟地址分作 4KiB 大小的页，并将索引表依次反向对应 FIXADDR_TOP 向下的 4KiB 页，也就是索引 0 对应 FIXADDR_TOP 向下的第一个 4KiB 页，索引 1 对应 FIXADDR_TOP 向下的第二个 4KiB 页，依次类推。因此可以通过索引表就可以获得一个唯一的虚拟地址，其关系如下:

![](/assets/PDB/HK/TH002060.png)

------------------------------

##### 分配器初始化

![](/assets/PDB/HK/TH001999.png)

永久映射内存分配器的初始化分做两个阶段，在系统初始化早期，系统采用恒等映射将内核空间虚拟地址 1:1 映射到物理地址，即内核虚拟地址空间起始地址映射到物理内存起始地址，以此形成大块连续映射的线性空间，内核只需通过一个线性公式就可以过得内核虚拟地址与物理内存的映射关系，恒等映射只建立好页目录的页表，并未建立最后一级的页表，因此需要通过缺页异常建立最后一级虚拟内存到物理内存的页表映射.

![](/assets/PDB/HK/TH001998.png)

对于永久映射内存分配器占用的虚拟内存为内核虚拟空间末尾，因此在 early_top_pgt 页表中占用最后一个 Entry。在恒等映射阶段，内核将 CR3 指向 early_to_pgt 页表，然后在 \_\_start-up_64() 函数中将 level3_kernel_pgt 页表映射到 early_top_pgt 中，其余的 level2_fixmap_pgt 和 level1_fixmap_pgt 在编译阶段已经与 level3_kernel_pgt 页表建立了联系，具体定义在 head_64.S:

![](/assets/PDB/HK/TH002000.png)

从汇编代码可以看出 level1_fixmap_pgt 页表一共包含了 FIXMAP_PMD_NUM * 512 个 PTE，因此需要占用 FIXMAP_PMD_NUM 个 PMD. 在 level2_fixmap_pgt 页表中，FIXMAP 只占用了 PMD Table 507th 之后的 PMD(507 = 512 - 4 - FIXMAP_PMD_NUM), 接着可以看到 level2-_fixmap_pgt 507th PMD 映射了 level1_fixmap_pgt 页表的物理地址，因此建立了 PMD 页表到 PTE 页表的映射; 在看看 level3_kernel_pgt PUD 页表的映射逻辑，其前半部分映射了 level2-_kernel_pgt 页表，接着映射了 level2_fixmap_pgt 页表，这样就建立了 FIXMAP 的 PUD 页表到 PMD 页表的映射关系.

![](/assets/PDB/HK/TH002001.png)

系统初始化早期内核之后，需要将恒等映射建立的关系清除，以便建立内核运行时使用的页表。内核在 x86_64_start_kernel() 函数中调用 reset_early_page_tables() 将恒等映射建立页表清除，然后将 early_top_pgt 页表的 511th Entry 拷贝到 init_top_pgt 页表的 511th Entry，内核仅仅保留 FIXMAP 映射关系，那么 init_top_pgt 页表可以通过 level3_kernel_pgt/level2_fixmap_pgt 页表最终找到 level1_fixmap_pgt PTE 页表. 分配器初始化完毕之后，分配器可以直接使用页表建立永久映射，无需考虑为页表页分配内存的问题.

--------------------------------

##### 分配器分配内存

![](/assets/PDB/HK/TH001994.png)
![](/assets/PDB/HK/TH001997.png)

永久映射内存分配器分配内存很简单，由于虚拟内存都是在编译阶段已经分配好的，因此系统运行时不能将分配器维护的虚拟内存分配用于其他目的。分配器提供了 fix_to_virt() 函数，请求者只需提供索引即可获得对应的虚拟内存，分配器也提供了便捷的接口 set_fixmap_offset() 函数，既可以分配虚拟内存，也可以将虚拟内存映射到预留物理内存或者外设寄存器(MMIO). fix_t-o_virt()/virt_to_fix() 函数可以实现索引和虚拟地址的转换

--------------------------

##### 永久映射页表建立

![](/assets/PDB/HK/TH002002.png)

当通过永久内存分配器分配到虚拟内存之后，接下来就是将虚拟内存映射到物理内存或者外设寄存器上(MMIO)，页表的建立过程和普通页表一样，由于虚拟地址是内核空间的，并且永久映射使用的 P4D、PUD、PMD、PTE 页表页都在编译阶段已经分配，因此只需设置最后一级页表 Entry 的值即可..

![](/assets/PDB/HK/TH001993.png)

可以通过永久映射分配器提供的函数接口查看整个页表建立过程，set_fixmap_offset() 函数通过调用 \_\_set_fixmap_offset() 函数建立一个基础的内核页表，其核心通过 native_set_fixmap() 函数实现，可以看到内核页表的 PGD、PUD、PMD 和 PTE 页表都存在，分配器只需设置 PTE Entry，将其指向物理内存或者外设寄存器(MMIO) 即可. 当页表创建完毕之后，可以通过 fix_to-\_virt() 函数获得虚拟地址之后直接使用虚拟内存.

--------------------------------

##### 永久映射内存使用

![](/assets/PDB/HK/TH001995.png)

由于永久内存映射的特点，可以在内核不同的阶段使用分配器分配的内存。上图提供了一个简单的案例介绍如何使用永久映射分配器分配的虚拟内存，首先需要提供永久映射的 IDX，例如 15 行提供的 BROILER_FIXMAP_IDX, 接下来提供一个物理内存地址或者 MMIO 地址，例如 14 行提供了一个 MMIO 地址 BROILER_MMIO_BASE; 然后在 23 行调用 set_fixmap_offset() 函数建立 BROILER_FIXMAP_IDX 对应的虚拟地址映射到 BROILER_MMIO_BASE MMIO 的页表，最后在 24 行调用 virt_to_fix() 函数直接获得 BROILER_FIXMAP_IDX 对应的虚拟地址。接下来就是在 26 行对虚拟地址进行读操作，当使用完毕之后可以 clear_fixmap() 函数清除之前建立的页表.

![](/assets/PDB/HK/TH002005.png)

永久内存映射分配器为了满足映射物理内存和 MMIO 的需求，需要提供不同的页表，以此满足有的映射是 Cache 的，但有的映射是 No Cache 的，分配器提供了 set_fixmap_io() 映射 MMIO 地址, 其提通过供 FIXMAP_PAGE_IO 页表实现; 分配器同样也提供了 set_fixmap_nocache() 函数映射是 No Cache，其通过提供 FIXMAP_PAGE_NOCACHE 页表实现.

![](/assets/PDB/HK/TH002006.png)

通过 FIXMAP_PAGE_IO 的定义 PAGE_KERNEL_IO 映射 IO 的内核页表，该页表属性专门用于映射 MMIO; FIXMAP_PAGE_NOCACHE  定义为不带 CACHE 的内核页表 PAGE_KERNEL-\_NOCACHE, 可以用于 MMIO 的映射; FIXMAP_PAGE_RO 定义为 PAGE_KERNEL_RO 只读的内核页表, 因此一些只读物理区域. FIXMAP_PAGE_CLEAR 则定义为 \_\_pgprot(0) 空的页表内容，那么可以用于清空最后一级页表 Entry. 更多使用实践案例，可以参考:

> [永久映射分配器使用](#B)

-------------------------------

##### 永久映射内存释放

![](/assets/PDB/HK/TH002003.png)

由于永久映射的特点，当不再使用分配器分配内存时，仅仅需要将最后一级页表 Entry 内容清零即可，其余虚拟内存一直存在并保持原有的含义，这样从语义上保持了虚拟内存的永久性.

![](/assets/PDB/HK/TH002004.png)

分配器提供了 clear_fixmap() 函数释放永久映射分配分配的虚拟内存，其实现与建立永久映射页表的 set_fixmap_offset() 函数采用同样的代码流程，只是其在 \_\_set_fixmap_offset() 函数传入了 FIXMAP_PAGE_CLEAR 参数，那么代码流程最后的 \_\_set_pte_vaddr() 函数会调用 set_pte() 函数传入 0，以此清除最后一级页表 Entry 里的内容，清除完毕之后刷新一下 TLB 对应的项.

--------------------------------

##### 永久映射分配器大小

![](/assets/PDB/HK/HK000226.png)

永久映射内存分配器维护了一段虚拟内存区域，这段区域位于内核空间的末尾，因此其在 Upper 内核页表都属于末尾项。永久映射内存分配器属于固定映射分配器管理的虚拟内存区域中的一部分，其与 Early IO/MEM 分配器管理的区域相邻，在 i386 架构中，永久映射内存分配器维护的虚拟区域还包括了 KMAP 临时映射分配器维护的虚拟内存区域(X86-64 架构不包含 KMAP 维护的区域).

![](/assets/PDB/HK/TH002055.png)

永久映射内存分配器维护的虚拟内存通过 FIXADDR_TOP、FIXADDR_START 和 FIXADDR_SIZE 三个宏进行描述，其中 FIXADDR_TOP 为 VSYSCALL_ADDR 的下一个 PAGE_SIZE 处，并按 PMD_SIZE 向上对其，由于 VSYSCALL_ADDR 是一个固定值，那么 FIXADDR_TOP 也是一个固定值 0xffffffffff7ff000. FIXADDR_SIZE 指明了永久映射内存分配器维护虚拟区域的大小，其与 \_\_end_of_permanent_fixed_addresses 的数量有关，那么 \_\_end_of_permanent_fixed_addresses 的取值范围如何计算呢?

![](/assets/PDB/HK/TH002056.png)

可以通过查看 enum fixed_address 枚举体中包含 Item 的数量，有的 Item 占用一个整数，有的则占用很多个整数，例如 FIX_IO_APIC_BASE_END 的值与 MAX_IO_APICS 的数量有关，\_\_end_of_permanent_fixed_addresses 的值则是 FIX_BISCUITOS 加一, 那么 \_\_end_of_permanent_fixed_addresses 最大能是多少，这也就说明了永久映射内存分配器最大能维护多少的虚拟内存?

![](/assets/PDB/HK/TH002000.png)

在永久映射分配器初始化介绍处分析过，永久映射分配器分配的虚拟内存之所以永久是因为其页表页在编译阶段就可以确定，因此可以查看其页表的定义过程，可以看到其最后一级页表 level1_fixmap_pgt 有 FIXMAP_PMD_NUM 个 512 Item，512 Item 可以理解为一个 PTE 页表，一个 PTE 页表可以映射 512 个 4KiB 物理区域，那么 level1_fixmap_pgt 可以映射 FIXMAP_PMD_NUM * 512 个 4KiB 物理区域。

![](/assets/PDB/HK/TH002057.png)

FIXMAP_PMD_NUM 的值与 CONFIG_DEBUG_KMAP_LOCAL_FORCE_MAP 宏有关，在 X86-64 架构中该宏不能启用，那么在 X86-64 架构中永久映射内存分配器维护的最大内存区域为 2 * 512 * 4KiB = 4MiB. 那么 FIXADDR_SIZE 的值为 4MiB，FIXADDR_START 的值为 0xFFFFFFFFFF3FF000. 因此 X86-64 架构中永久映射内存分配器维护的虚拟地址范围是: \[0xFFFFFFFFFF3FF000, 0xffffffffff7ff000). 由于 X86 架构上限制，永久映射分配器维护的内存只能是 4MiB.

![](/assets/PDB/HK/TH002058.png)

在 i386 架构中，如果开启了 CONFIG_DEBUG_KMAP_LOCAL_FORCE_MAP，那么永久映射分配器维护的虚拟地址大小与 CPU 数量之间的关系如上图. 当不开启 CONFIG_DEBUG_KMAP_LOCAL_FORCE_MAP 时，永久映射内存分配器维护的虚拟地址范围是: \[0xFFFFFFFFFF3FF000, 0xffffffffff7ff000).

![](/assets/PDB/HK/TH002059.png)

通过上面的分析，在 i386 架构如果想增大永久映射分配器维护的虚拟内存区域，可以通过修改 FIXMAP_PMD_NUM 宏实现. FIXMAP_PMD_NUM 每增加 1，永久映射分配器维护的虚拟内存就增加 2MiB. 如果永久映射分配器分配的虚拟内存超出其维护的范围会引起什么问题呢？具体请看 BUG 分析:

> [永久映射内存分配器占用的内存超限 BUG](/blog/Memory-ERROR/#A00A05)

-----------------------------

##### 永久映射分配器使用场景

![](/assets/PDB/HK/TH002056.png)

X86 架构下正在使用永久映射内存的场景如上图，这些场景都有一些特点，总结如下:

* 提供一个从编译到系统启动，最后到系统运行都有效且含义不变的虚拟地址
* 为明确的公共功能提供一个统一的访问入口
* 通过统一的访问入口屏蔽不同硬件产商带来的差异
* 可以服务于公共信息源，包括硬件错误信息源等
* 使用较少的虚拟内存访问巨量的物理资源

> - [VSYSCALL 应用场景](#C1)
>
> - [FIX_DBGP_BASE: EHCI Debug Port 应用场景](#C2)
>
> - [FIX_EARLYCON_MEM_BASE: EarlyCon 应用场景](#C2)
>
> - [FIX_APIC_BASE: LAPIC 中断控制器应用场景](#C3)
>
> - [FIX_IO_APIC_BASE: IOAPIC 中断控制器应用场景](#C4)
>
> - [FIX_KMAP_BEGIN: KMAP 临时映射分配器应用场景](#C5)
>
> - [FIX_PCIE_MCFG: PCIe Configuration Space MMIO-Way 应用场景](#C6)
>
> - [FIX_APEI_GHES_IRQ/NMI: General Hardware Error: GHES 应用场景](#C7)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

#### 永久映射分配器实践

开发者可以在 BiscuitOS 或者 Broiler 上实践永久映射分配器，提供了多个实践案例，具体案例可以参考[永久映射分配器使用章节](#B). 本节用于介绍如何进行永久映射分配的实践，本文以 Linux 6.0 X86_64 架构进行介绍。开发者首先需要搭建 BiscuitOS 或 Broiler 环境，具体参考如下:

> [BiscuitOS Linux 6.0 X86-64 开发环境部署](/blog/Kernel_Build/#Linux_5Y)
>
> [Broiler 开发环境部署](/blog/Broiler/#B)

环境部署完毕之后，接下来参考如下命令在 BiscuitOS 中部署永久映射相关的实践案例:

{% highlight bash %}
cd BiscuitOS
make linux-6.0-x86_64_defconfig
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_offset_io()  --->

BiscuitOS/output/linux-6.0-x86_64/package/BiscuitOS-FIXMAP-set_fixmap_offset_io-default
{% endhighlight %}

> [BiscuitOS-FIXMAP-set_fixmap_offset_io_default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_offset_io)

![](/assets/PDB/HK/TH001747.png)
![](/assets/PDB/HK/TH002012.png)
![](/assets/PDB/HK/TH002013.png)
![](/assets/PDB/HK/TH002009.png)

程序源码很精简，程序在 22 行调用 set_fixmap_offset_io() 函数将 BROILER_FIXMAP_IDX 对应的虚拟内存映射到外设 MMIO 地址 BROILER_MMIO_BASE 上，并将映射的虚拟地址存储在 addr 变量里。接下来程序直接在 24 行访问 addr 进而访问外设 MMIO, 但使用完
毕之后，可以使用 clear_fixmap() 函数清除页表即可. 最后 set_fixmap_offset() 函数建立的页表是不带 CACHE 的，因此建议使用
这个函数映射外设 MMIO. 接下来是编译源码，可以选择在 BiscuitOS 上实践，也可以在 Broiler 上实践，考虑到使用到 Broiler 模拟的硬件，那么这里以 Broiler 实践为准:

{% highlight bash %}
cd BiscuitOS/output/linux-6.0-x86_64/package/BiscuitOS-FIXMAP-set_fixmap_offset_io-default
# 1.1 Only Running on BiscuitOS
  make build
# 1.2 Need Running on Broiler
  make broiler
{% endhighlight %}

由于要在 Broiler 上实践，那么运行 1.2 的命令 `make broiler`, 接下来在 Broiler 中实践

![](/assets/PDB/HK/TH002014.png)

可以看到 Broiler 启动过程中，程序从 0xD0000000 中读到了 0x20 的值，这是符合预期的，因此永久映射实践成功.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

#### 永久映射分配器使用

永久映射内存映射提供了很多接口函数用于不同的场景，整理如下:

> - [永久映射外设 MMIO/物理内存](#B1)
>
> - [永久映射为不带 Cache](#B2)
>
> - [永久映射 IO](#B3)
>
> - [新增永久映射](#B4)
> 
> - <span id="B5">永久映射分配器 API 合集</span>
>
>   - [clear_fixmap](#B5B)
>
>   - [fix_to_fix](#B59)
>
>   - [\_\_set_fixmap](#B52)
>
>   - [set_fixmap](#B51)
>
>   - [set_fixmap_io](#B57)
>
>   - [set_fixmap_nocache](#B55)
>
>   - [\_\_set_fixmap_offset](#B54)
>
>   - [set_fixmap_offset](#B53)
>
>   - [set_fixmap_offset_io](#B58)
>
>   - [set_fixmap_offset_nocache](#B56)
>
>   - [virt_to_fix](#B5A)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

##### 永久映射外设 MMIO/物理内存

可以使用永久映射分配器分配一段虚拟内存，然后映射到外设的 MMIO 或者物理内存上，其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_offset()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_offset-default
{% endhighlight %}

> [BiscuitOS-FIXMAP-set_fixmap_offset-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_offset)

![](/assets/PDB/HK/TH002007.png)

程序源码很精简，程序在 22 行调用 set_fixmap_offset() 函数将 BROILER_FIXMAP_IDX 对应的虚拟内存映射到外设 MMIO 地址 BROILER_MMIO_BASE 上，并将映射的虚拟地址存储在 addr 变量里。接下来程序直接在 24 行访问 addr进而访问外设 MMIO, 但使用完毕之后，可以使用 clear_fixmap() 函数清除页表即可. 如果将程序中的 BROILER_MMIO_BASE 指向物理内存，那么就是永久映射一段物理内存. 最后 set_fixmap_offset() 函数建立的页表是带 CACHE 的，因此不建议使用这个函数映射外设 MMIO，可以用这个函数映射预留内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

##### 永久映射为不带 Cache

可以使用永久映射分配器分配一段虚拟内存，然后映射到外设的 MMIO 或者物理内存上，并且这段映射是不带 cache 的，其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_offset_nocache()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_offset_nocache-default
{% endhighlight %}

> [BiscuitOS-FIXMAP-set_fixmap_offset_nocache_default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_offset_nocache)

![](/assets/PDB/HK/TH002008.png)

程序源码很精简，程序在 22 行调用 set_fixmap_offset_nocache() 函数将 BROILER_FIXMAP_IDX 对应的虚拟内存映射到外设 MMIO 地址 BROILER_MMIO_BASE 上，并将映射的虚拟地址存储在 addr 变量里。接下来程序直接在 24 行访问 addr进而访问外设 MMIO, 但使用完毕之后，可以使用 clear_fixmap() 函数清除页表即可. 如果将程序中的 BROILER_MMIO_BASE 指向物理内存，那么就是永久映射一段物理内存. 最后 set_fixmap_offset() 函数建立的页表是不带 CACHE 的，因此建议使用这个函数映射外设 MMIO.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

##### 永久映射 IO

可以使用永久映射分配器分配一段虚拟内存，然后映射到外设的 MMIO，并且这段映射是不带 cache 的，其在 BiscuitOS 上的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_offset_io()  --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_offset_io-default
{% endhighlight %}

> [BiscuitOS-FIXMAP-set_fixmap_offset_io_default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_offset_io)

![](/assets/PDB/HK/TH002009.png)

程序源码很精简，程序在 22 行调用 set_fixmap_offset_io() 函数将 BROILER_FIXMAP_IDX 对应的虚拟内存映射到外设 MMIO 地址 BROILER_MMIO_BASE 上，并将映射的虚拟地址存储在 addr 变量里。接下来程序直接在 24 行访问 addr 进而访问外设 MMIO, 但使用完毕之后，可以使用 clear_fixmap() 函数清除页表即可. 最后 set_fixmap_offset() 函数建立的页表是不带 CACHE 的，因此建议使用这个函数映射外设 MMIO.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

##### 新增永久映射

![](/assets/PDB/HK/TH002010.png)

当某些场景中需要新增一个段永久映射的虚拟内存，可以参考上图 patch 在 enum fixed_addresses 中新增一个 Entry，这就就实现在永久映射中新增一段虚拟内存，那么接下来在 BiscuitOS 中部署: 

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] Permanent Mapping: Add New Entry --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-Permanent-default
{% endhighlight %}

> [BiscuitOS-Permanent_default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-Permanent)

![](/assets/PDB/HK/TH002011.png)

程序源码很精简，程序在 21 行调用 set_fixmap_io() 函数将 BROILER_FIXMAP_IDX 对应的虚拟内存映射到外设 MMIO 地址 BROILER_MMIO_BASE 上，然后调用 virt_to_fix() 函数将映射的虚拟地址存储在 addr 变量里。接下来程序直接在 24 行访问 addr 进而访问外设 MMIO, 但使用完毕之后，可以使用 clear_fixmap() 函数清除页表即可.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B51">set_fixmap</span>

![](/assets/PDB/HK/TH002036.png)

set_fixmap() 函数的作用是将 idx 对应的虚拟内存映射到物理地址 phys 上，phys 可以是物理内存，也可以是 MMIO. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap-default
{% endhighlight %}

> [The Practice Code for 'set_fixmap' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap)

![](/assets/PDB/HK/TH002015.png)

在程序中，函数 set_fixmap() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，然后通过 fix_to_virt() 函数获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，并通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B52">\_\_set_fixmap</span>

![](/assets/PDB/HK/TH002016.png)

\_\_set_fixmap() 函数的作用是将 idx 对应的虚拟内存根据不同的映射类型，映射到物理地址 phys 上，phys 可以是物理内存，也可以是 MMIO. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. flags 指明了映射的类型，目前支持的映射类型有 FIXMAP_PAGE_NORMAL、FIXMAP_PAGE_RO、FIXMAP_PAGE_NOCACHE、FIXMAP_PAGE_IO 和 FIXMAP_PAGE_CLEAR. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: __set_fixmap() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-__set_fixmap-default
{% endhighlight %}

> [The Practice Code for '\_\_set_fixmap' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-__set_fixmap)

![](/assets/PDB/HK/TH002017.png)

在程序中，函数 \_\_set_fixmap() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表, 并且页表属性为 FIXMAP_PAGE_IO，然后通过 fix_to_virt() 函数获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，并通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B53">set_fixmap_offset</span>

![](/assets/PDB/HK/TH002018.png)

set_fixmap_offset() 函数的作用是将 idx 对应的虚拟内存映射到物理地址 phys 上并返回虚拟地址，phys 可以是物理内存，也可以是 MMIO. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_offset() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_offset-default
{% endhighlight %}

> [The Practice Code for 'set_fixmap_offset' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_offset)

![](/assets/PDB/HK/TH002019.png)

在程序中，函数 set_fixmap_offset() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，并获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，然后通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B54">\_\_set_fixmap_offset</span>

![](/assets/PDB/HK/TH002020.png)

\_\_set_fixmap_offset() 函数的作用是将 idx 对应的虚拟内存根据不同的映射类型，映射到物理地址 phys 上，phys 可以是物理内存，也可以是 MMIO. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. flags 指明了映射的类型，目前支持的映射类型有 FIXMAP_PAGE_NORMAL、FIXMAP_PAGE_RO、FIXMAP_PAGE_NOCACHE、FIXMAP_PAGE_IO 和 FIXMAP_PAGE_CLEAR. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: __set_fixmap_offset() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-__set_fixmap_offset-default
{% endhighlight %}

> [The Practice Code for '\_\_set_fixmap_offset' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-__set_fixmap_offset)

![](/assets/PDB/HK/TH002021.png)

在程序中，函数 \_\_set_fixmap_offset() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表, 并且页表属性为 FIXMAP_PAGE_NORMAL, 并将返回的虚拟地址保存在 addr 变量里，并通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B55">set_fixmap_nocache</span>

![](/assets/PDB/HK/TH002022.png)

set_fixmap_nocache() 函数的作用是将 idx 对应的虚拟内存映射到物理地址 phys 上，且页表属性是 NoCache 的，phys 可以是物理内存，也可以是 MMIO. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_nocache() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_nocache-default
{% endhighlight %}

> [The Practice Code for 'set_fixmap_nocache' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_nocache)

![](/assets/PDB/HK/TH002023.png)

在程序中，函数 set_fixmap_nocache() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，然后调用 fix_to_virt() 函数获得 BROILER_FIXMAP_IDX 对应的虚拟内存，并获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，然后通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B56">set_fixmap_offset_nocache</span>

![](/assets/PDB/HK/TH002024.png)

set_fixmap_offset_nocache() 函数的作用是将 idx 对应的虚拟内存映射到物理地址 phys 上并返回虚拟地址，且页表属性是 NoCache 的，phys 可以是物理内存，也可以是 MMIO. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_offset_nocache() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_offset_nocache-default
{% endhighlight %}

> [The Practice Code for 'set_fixmap_offset_nocache' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_offset_nocache)

![](/assets/PDB/HK/TH002025.png)

在程序中，函数 set_fixmap_offset_nocache() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，并获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，然后通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B57">set_fixmap_io</span>

![](/assets/PDB/HK/TH002026.png)

set_fixmap_io() 函数的作用是将 idx 对应的虚拟内存映射到物理地址 phys 上，且页表属性是给 MMIO 使用的，phys 是 MMIO 地址. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_io() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_io-default
{% endhighlight %}

> [The Practice Code for 'set_fixmap_io' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_io)

![](/assets/PDB/HK/TH002027.png)

在程序中，函数 set_fixmap_io() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，然后调用 fix_to_virt() 函数获得 BROILER_FIXMAP_IDX 对应的虚拟内存，并获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，然后通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B58">set_fixmap_offset_io</span>

![](/assets/PDB/HK/TH002028.png)

set_fixmap_offset_io() 函数的作用是将 idx 对应的虚拟内存映射到物理地址 phys 上并返回虚拟地址，且页表属性是针对 IO 的，phys 是 MMIO 地址. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: set_fixmap_offset_io() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-set_fixmap_offset_io-default
{% endhighlight %}

> [The Practice Code for 'set_fixmap_offset_io' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-set_fixmap_offset_io)

![](/assets/PDB/HK/TH002029.png)

在程序中，函数 set_fixmap_offset_io() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，并获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，然后通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B59">fix_to_fix</span>

![](/assets/PDB/HK/TH002030.png)

fix_to_virt() 函数的作用是获得 idx 对应的虚拟内存. idx 的取值不要超过 \_\_end_of_permanent_fixed_addresses. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: fix_to_virt() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-fix_to_virt-default
{% endhighlight %}

> [The Practice Code for 'fix_to_virt' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-fix_to_virt)

![](/assets/PDB/HK/TH002031.png)

在程序中，函数 set_fixmap() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，然后通过 fix_to_virt() 函数获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，并通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B5A">virt_to_fix</span>

![](/assets/PDB/HK/TH002032.png)

virt_to_fix() 函数的作用是将永久映射分配器管理的虚拟地址转换成 IDX. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: virt_to_fix() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-virt_to_fix-default
{% endhighlight %}

> [The Practice Code for 'virt_to_fix' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-virt_to_fix)

![](/assets/PDB/HK/TH002033.png)

在程序中，函数 set_fixmap_offset() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，并获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，然后使用 virt_to_fix() 函数获得虚拟地址对应的 IDX，最后通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="B5B">clear_fixmap</span>

![](/assets/PDB/HK/TH002034.png)

clear_fixmap() 函数的作用是清除永久映射分配器建立的页表，也就是释放永久映射分配分配的内存. 具体函数在 BiscuitOS 实践部署请看:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] FIXMAP API: clear_fixmap() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-FIXMAP-clear_fixmap-default
{% endhighlight %}

> [The Practice Code for 'clear_fixmap' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-FIXMAP-clear_fixmap)

![](/assets/PDB/HK/TH002035.png)

在程序中，函数 set_fixmap_offset() 建立了 BROILER_FIXMAP_IDX 对应虚拟内存到 MMIO BROILER_MMIO_BASE 之间的页表，并获得 BROILER_FIXMAP_IDX 对应的虚拟地址并保存在 addr 变量里，然后通过 addr 访问 MMIO，最后使用完毕之后通过 clear_fixmap() 函数将之前的页表清除，那么不能在访问 BROILER_FIXMAP_IDX 对应的虚拟内存.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### <span id="X">永久映射分配器使用场景</span>

-------------------------------------------

<span id="C1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000V.jpg)

##### VSYSCALL 应用场景

Linux 系统调用是内核一种特殊的运行机制，该机制可以使用户空间的应用程序可以请求某些特权级下的任务，例如文件的写入和 Socket 打开等。在 Linux 内核中发起一个系统调用是特别昂贵的操作，因为处理器需要中断当前正在执行的任务，切换内核模式的上下文，然后执行特定的操作，待系统调用处理完毕之后跳转到用户空间。因此看来传统的系统调用是一个费时的操作，但对于某些需要快速相应的系统调用，显然传统的系统调用机制已经无法满足需求，因此 Linux 提供了 VSYSCALL 机制.

在 Linux 提出 VSYSCALL 机制之前，Intel 和 AMD 也提相同目的的指令 SYSENTER/SYSEXIT 和 SYSCALL/SYSRET, 即所谓的快速系统调用，虽然使用这些指令系统调用速度更快，但是存在兼容性问题，于是 Linux 实现了 VSYSCALL 机制. VSYSCALL 机制很简单，Linux 用户空间映射一个包含一些变量以及一些系统调用实现的内存页。

![](/assets/PDB/HK/TH002038.png)

在 VSYSCALL 机制中，内核使用一个物理页 **vsyscall page** 实现了三个系统调用: \_\_NR_gettimeofday、\_\_NR_time 和 \_\_NR_getcpu. gettimeofday 系统调用用于获取当前时间和时区信息，time 系统调用用于获取系统时间(秒数), 最后 getcpu 系统调用用于确认线程正在运行的 CPU 和 NUMA 节点概要.

![](/assets/PDB/HK/TH002037.png)

内核在初始化时调用 map_vsyscall() 函数在 88 行调用 \_\_set_fixmap() 函数，将 VSYSCALL_PAGE 虚拟地址映射到 vsyscall page 上，并调用 set_vsyscall_pgtable_user_bits() 函数将页表的属性添加了 \_PAGE_USER, 那么系统在创建或 fork 进程时，由于每个进程的内核空间部分都是相同的，那么每个进程都会包含 VSYSCALL_PAGE 这段虚拟地址，且虚拟地址都映射到了 vsyscall page 里。

![](/assets/PDB/HK/TH002039.png)

可以在系统上查看不同进程的地址空间，都会发现 vsyscall 占用的区域都是 \[ffffffffff600000, ffffffffff601000), 因此 \_\_NR_gettimeofday、\_\_NR_time 和 \_\_NR_getcpu 上个系统调用的虚拟地址对所有的进程都是固定的，因此 VSYSCALL 机制可以保持对所有进程的一致性，最后通过一个实例实践一下 VSYSCALL 调用:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] FIXMAP Memory Allocator  --->
          [*] Permanent Mapping: VSYSCALL() --->

BiscuitOS/output/linux-X.Y.Z-ARCH/package/BiscuitOS-Permanent-VSYSCALL-default
{% endhighlight %}

> [The Practice Code for 'Permanent VSYSCALL' on @Gitee ](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-Permanent-VSYSCALL)

![](/assets/PDB/HK/TH002040.png)

实践案例代码很简单，首先直接计算出 gettimeofdata 系统调用的虚拟地址，然后在 28 行处将一个符合 gettimeofday() 函数指针指向这个虚拟地址，然后在 29 行通过引用这个函数指针调用 gettimeofday 系统调用. 最后在 BiscuitOS 上进行实践:

![](/assets/PDB/HK/TH002041.png)

BiscuitOS 运行之后，运行 BiscuitOS-Permanent-VSYSCALL-default 应用程序，可以看到程序打印了当前系统时间. 通过上面的理论和实践分析，如果 VSYSCALL 要实现将指定的系统调用放在指定的虚拟地址上，然后让所有进程都可以看到这个固定的地址，那么 VSYSCALL 就需要永久映射分配器将 VSYSCALL_PAGE 虚拟地址映射到 vsyscall page 对应的物理页上，因此 VSYSCALL 很好的展示了永久映射内存分配器的使用场景.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000D.jpg)

##### EarlyCon/EHCI Debug Port 应用场景

![](/assets/PDB/HK/TH002042.png)

在系统早期需要将内核启动信息通过某些设备进行打印，其中 UART 为常见的设备，那么 UART 为什么需要永久映射的内存呢? 原因是在不同的产商生产的开发板上会采用不同的 UART 芯片，UART 芯片映射到存储域的地址 MMIO 各有不同，为了兼容所有 UART 寄存器映射，那么统一使用一个一致有效的虚拟地址，这个虚拟地址正好是来自永久映射内存分配器的 FIX_EARLYCON_MEM_BASE, 各家厂商将其 UART 芯片对应的 MMIO 地址与 FIX_EARLYCON_MEM_BASE 对应的虚拟地址建立页表，因此内核可以统一访问 FIX_EARLYCON_MEM_BASE 虚拟地址就可以无差别访问到不同 UART 芯片的寄存器.

![](/assets/PDB/HK/TH002043.png)

同理各家厂商也提供了 EHCI USB2.0 控制器，可以将内核打印的信息通过 USB2.0 输出到调试端。此 USB 线不是标准的 USB2.0 电缆，因为它具有额外的硬件组件，使用与 USB2.0 调试设备功能规范兼容。内核需要通过访问 EHCI USB2.0 控制器内部寄存器来控制数据传输，但是不同产商因为主板设计差异导致 EHCI 主控映射到存储域不同的位置，即没有固定的 MMIO 地址，因此内核借助永久映射内存分配器，将 FIX_DBGP_BASE 对应的虚拟地址映射到 EHCI 主控内部寄存器的 MMIO 上，这样无论 EHCI 主控在不同硬件主板上映射 MMIO 差异，都可以使用统一的虚拟地址进行访问，很好的兼容了不同厂家的 EHCI USB2.0 Debug Port.

> [EHCI Debug Port Describe @Link](https://www.coreboot.org/EHCI_Debug_Port)

**结论**: 通过上面两个应用场景的分析，当某个功能的硬件由不同的厂商提供，由于该硬件在主板上布局的差异，导致其内部寄存器映射到存储域不同的地址上，也就是具有了不同的 MMIO 地址，内核可以使用永久映射分配一个永久有效的虚拟地址，将其映射到不同产商芯片的 MMIO 地址上，这样就可以屏蔽 MMIO 地址带来的差异，统一了对芯片的访问逻辑.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="C3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

##### LAPIC 中断控制器应用场景

![](/assets/PDB/HK/TH001758.png)

在 X86 架构中每个 CPU 都会有一个 Local APIC 高级中断控制器，外设中断引脚可以连接到 PIC 中断控制器或者 IOAPIC 控制器上，然后经过分发之后中断信号会发送到指定 CPU 对应的 LAPIC 中断控制器上，然后 LAPIC 中断控制器决定如果将中断发送给 CPU，CPU 收到 LAPIC 发送的中断就停止当前运行的程序，进而处理中断.

![](/assets/PDB/HK/TH002044.png)

LAPIC 本地高级中断控制器内部包含多种寄存器，用于仲裁分发应答中断等功能，每个 LAPIC 都有 4KiB 大小的寄存器域，但是系统只在存储域的 0xFEE00000 开始的处映射了 4KiB 空间，从上图展示的系统存储域总线的布局可以看到 Local APIC，无论有多少个 LAPIC，从存储域上只看得到 4KiB 区域，这是因为 LAPIC 寄存器是 PERCPU 粒度的，虽然系统只看到 4KiB 区域，当 CPU 访问 4KiB 区域时都会访问到各自的 4KiB 区域.

![](/assets/PDB/HK/TH002045.png)

由于每个 CPU 都用各自的 LAPIC，并且 LAPIC 相关的寄存器都映射到 0xFEE00000 处，为了让每个 CPU 都可以访问到 LAPIC 寄存器，那么使用永久映射分配器分配一个固定的虚拟内存地址 FIX_APIC_BASE, 将其映射到 LAPIC 寄存器的 MMIO 地址上，这样所有 CPU 都可以采用统一虚拟地址对 LAPIC 进行访问, 硬件自动转换为各自的 4KiB 区域进行访问.

**结论**: 通过对上面应用场景的分析，当多个 CPU 要访问 PERCPU 硬件设备，所有 CPU 都要访问同一个物理地址，那么可以使用永久映射分配器分配一段虚拟内存，然后所有 CPU 都可以访问这段虚拟内存，最后由硬件决定访问哪个 CPU 的 PERCPU 区域.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

<span id="C4"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

##### IOAPIC 中断控制器应用场景

![](/assets/PDB/HK/TH001852.png)

在 X86 架构中，采用 IOAPIC 中断控制器代替传统的 8259A 中断控制器，外设将其中断连接到 IOAPIC 中断引脚，IOAPIC 中断内部的处理逻辑，然后将中断分发到指定的 LAPIC 中断控制器上。系统中可以包含多个 IOAPIC 中断控制器，每个 IOAPIC 中断控制器内部包含多个寄存器，CPU 通过访问这些寄存器获得中断相关的信息.

![](/assets/PDB/HK/TH002044.png)

从上面的系统存储域总线布局可以看到 IOAPIC 映射到 0xFEC00000 起始的位置，由于系统可以支持多个 IOAPIC，因此 IOAPIC 映射 MMIO 地址可能会不同，因此内核向通过一个统一地址知道第一个 IOAPIC、第二个 IOAPIC 等，以此对 IOAPIC 进行统一的访问.

![](/assets/PDB/HK/TH002046.png)

内核借助永久映射分配器为每个 IOAPIC 分配了一段虚拟地址，然后以此映射到 IOAPIC 上，因此可以通过 IDX 就可以访问指定的 IOAPIC 内部寄存器。

**结论**: 通过对上面场景的分析，当系统具有相关功能的外设，其外设的数量不定，因此可以使用永久映射内存分配器将起始虚拟地址映射到第一个外设 MMIO，紧接着第二段虚拟地址映射到第二个外设 MMIO 上，以此类推，这些外设的 MMIO 地址可能不连续，但虚拟地址是固定且连续的，因此系统可以通过固定的虚拟内存就可以访问所有的这类外设.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

<span id="C6"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

##### PCIe Configuration Space MMIO 场景

![](/assets/PDB/HK/TH001475.png)

在 X86 架构中，系统存储域由两部分组成: 内存和 MMIO，其中 PCI 外设可以将其内部寄存器映射到存储域形成 MMIO。另外系统也维护一颗或多颗 PCI 树，每科 PCI 树形成一条 PCI 总线域，PCI 外设也将其内部寄存器映射到 PCI 总线域上。每个 PCI 设备都有一个配置空间，用于配置 PCI 设备的行为。X86 架构可以通过两种方式访问 PCI 设备的配置空间，首先通过 IO Port 0xCF8/0xCFC 进行访问，这种方式的缺点是只能访问 256-Byte 的配置空间，但大部分 PCI  设备的配置空间为 4K，因此需要使用第二种方式进行访问，系统将所有 PCI 设备的 4K 配置空间按 BDF 的顺序映射到 MMIO，然后系统可以结合 PCI 设备的 BDF 号在这段 MMIO 中找到指定的配置空间。

![](/assets/PDB/HK/TH002047.png)

可以查看 "/proc/iomem" 来获得所有 PCI 配置空间的 MMIO 地址，但存在一个问题，系统一般都是需要访问某个 PCI 设备的配置寄存器时才会访问这段 MMIO 地址的，如果将所有外设都映射到内核空间或进程的地址空间，那么将白白占用 256MiB 的空间。因此内核借助永久映射分配器，专门分配一个 4K 大的虚拟空间，当要访问某个 PCI 设备的配置空间时，就将 4K 虚拟空间映射到对应的 MMIO 区域，这样就可以确保 FIX_PCIE_MCFG 对应的虚拟地址就是指定 PCI 设备的配置空间.

![](/assets/PDB/HK/TH002048.png)

从内核实现来看，pci_exp_set_dev_base() 函数首先获得指定 PCI 设备配置空间映射的 MMIO 地址，然后通过 set_fix_nocache() 函数将 FIX_PCIE_MCFG 对应的虚拟空间映射到这段 MMIO 区域，这样就可以访问指定 PCI 设备的配置空间.

**结论**: 当某个功能占用了很大的 MMIO 区域，且这些区域可以分作很多功能相同的区域时，可以使用永久映射分配器分配一段虚拟内存，可以采用临时映射的方式，是虚拟地址代表特定的功能并访问了不同的 MMIO 区域.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

<span id="C7"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

##### General Hardware Error: GHES 应用场景

在服务器 RAS 机制中，GHES: General Hardware Error Source 称为硬件错误源，当系统启动时 BIOS 会建立 GHEST 表，该表用于指明系统发生硬件故障时，错误信息存储的物理地址，以及决定固件通过何种方式通知操作系统，可以是 NMI 也可以是轮询等。操作系统也有加载特定的 GHES 驱动，驱动会在收到固件通知之后，通过特定的方式从 GHES 内读出错误信息并进行处理。

![](/assets/PDB/HK/TH002049.png)

当固件收到一个硬件故障之后，通过 NMI 中断通知 CPU，那么 Linux GHES 驱动的 NMI 中断处理函数 ghes_notify_nmi() 会被调用，在中断处理函数中，通过调用 ghes_in_nmi_spool_from_list() 函数将 FIX_APEI_GHES_NMI 对应的虚拟地址映射到 GHES 对应的物理地址上，然后通过 FIX_APEI_GHES_NMI 对应的虚拟地址读出 GHES 的错误信息进行分析处理.

![](/assets/PDB/HK/TH002050.png)

当固件收到一个硬件故障之后，Linux GHES 驱动也可以通过轮询通知 CPU，驱动会通过将 FIX_APEI_GHES_IRQ 对应的虚拟地址映射到 GHES 对应的物理地址上，然后通过访问 FIX_APEI_GHES_IRQ 对应的虚拟地址获得硬件故障的信息，并通过 Polling 方式上报.

**结论**: 故障源、信息源之类可以提供给很多子模块使用，那么可以永久映射分配器分配的虚拟内存进行统一的访问，这样各模块都可以很便捷的获得所需的信息，而不需要各自独占的进行映射.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------------

<span id="C5"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

##### KMAP 临时映射分配器应用场景

KMAP 临时映射分配器是在 i386 架构中，由于 i386 架构采用 32 个地址线进行寻址，那么系统线性地址空间为 0 到 4Gig，而内核空间只有 3Gig 到 4Gig 的访问，在 1Gig 的虚拟内存里并不是都映射了物理内存，也映射了很多外设寄存器的 MMIO，因此在 i386 架构中，内核的虚拟内存空间是相当紧张的。内核为了更高效的使用内核空间，对有的内核请求只需要临时使用一下的场景，内核提出了临时映射分配器。例如当需要迁移一个物理页时，可以将原始页和目的页临时进行映射，然后进行数据拷贝，待拷贝完毕解除临时映射，这样就可以做到不浪费内核空间的虚拟内存。

![](/assets/PDB/RPI/RPI000969.png)

KMAP 分配器从永久映射分配器中分配一段虚拟内存区域: FIXMAP_KMAP_BEGIN 到 FIXMAP_KMAP_END，然后将虚拟区域按 idx 分作很多 4KiB 的虚拟内存区域，当需要临时映射时 KMAP 分配器就从中找到一个空闲的 idx 并标记为 used，然后将 idx 对应的虚拟内存映射到物理地址上，然后就可以使用这段虚拟内存。当使用完虚拟内存时，KMAP 分配器解除虚拟地址到物理地址之间的页表，然后释放物理内存，最后将 idx 标记为 free。

**结论**: 永久映射分配器分配的虚拟内存可以做特定功能使用，即虚拟地址的含义不变但映射的物理地址可以随便改变. 

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------------

