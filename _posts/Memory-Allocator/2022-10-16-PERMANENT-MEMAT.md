---
layout: post
title:  "Permanent Mapping Memory Allocator"
date:   2022-10-16 12:00:00 +0800
categories: [MMU]
excerpt: Permanent Mapping.
tags:
  - Boot-Time Allocator
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
> - [永久映射分配器使用场景](#C)
>
> - 永久映射分配器 BUG 合集

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### 永久映射分配器原理

**Permanent Mapping Memory Allocator** 称为**永久映射内存分配器**, 其属于**固定映射内存分配器**的一个分支，其特点就是在编译阶段就可以确定某段虚拟地址分配给特定的功能使用。从实现角度来看，永久映射内存分配器通过在编译阶段从内核的虚地址地址空间占用一段虚拟内存，再等到系统运行之后，通过将这个区域的某段虚拟地址映射到特定的物理内存或者外设寄存器上，只要不释放那么这段映射会永久有效.

![](/assets/PDB/HK/TH001990.png)

"永久" 到底有多久? 为什么是 "永久"? 可以从两个维度来回答这个问题: 维度一内核从启动到系统真正运行会经历不同的阶段，每个阶段会临时建立一些临时映射用于完成该阶段的任务，当任务完成之后映射也会清除，例如启动阶段的恒等映射阶段。当对于永久映射分配器分配的虚拟内存，一旦页表建立，只要映射关系不解除，那么映射关系可以一直存在。

维度二对比常见的内存分配器，内核或者应用程序需要分配虚拟内存，分配器会动态从虚拟空间分配一块可用的虚拟内存区域，然后通过直接或缺页的方式与物理内存或外设寄存器建立页表，最后才能使用这块虚拟内存。但永久映射分配器分配的虚拟内存就不同，其可以在编译阶段就可以知道虚拟内存值. 结合两个维度的分析，永久映射内存分配器称为 **Compile-Time** 分配器.

![](/assets/PDB/HK/TH001991.png)

永久内存分配器的实现原理如上图，分配器维护了一个索引，每个索引与特定的功能绑定在一起，因此可以从索引对应的宏知道这段虚拟地址的用途。分配器从系统的虚拟地址空间分配了 FIXADDR_START 到 FIXADDR_TOP 的区域，其长度为 FIXADDR_SIZE. 分配器从 FIXADD-R_TOP 向地址将虚拟地址分作 4KiB 大小的页，并将索引表依次反向对应 FIXADDR_TOP 向下的 4KiB 页，也就是索引 0 对应 FIXADDR_TOP 向下的第一个 4KiB 页，索引 1 对应 FIXADDR_TOP 向下的第二个 4KiB 页，依次类推。因此可以通过索引表就可以获得一个唯一的虚拟地址，其关系如下:

{% highlight c %}
## FIXADDR Range

#define FIXADDR_TOP       (round_up(VSYSCALL_ADDR + PAGE_SIZE, 1<<PMD_SHIFT) - PAGE_SIZE)
#define FIXADDR_SIZE      (__end_of_permanent_fixed_addresses << PAGE_SHIFT)
#define FIXADDR_START     (FIXADDR_TOP - FIXADDR_SIZE)

## Translation
#define __fix_to_virt(x)  (FIXADDR_TOP - ((x) << PAGE_SHIFT))
#define __virt_to_fix(x)  ((FIXADDR_TOP - ((x)&PAGE_MASK)) >> PAGE_SHIFT)
{% endhighlight %}

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

通过 FIXMAP_PAGE_IO 的定义 PAGE_KERNEL_IO 映射 IO 的内核页表，该页表属性专门用于映射 MMIO; FIXMAP_PAGE_NOCACHE  定义为不带 CACHE 的内核页表 PAGE_KERNEL-\_NOCACHE, 可以用于 MMIO 的映射; FIXMAP_PAGE_RO 定义为 PAGE_KERNEL_RO 只读的内核页表, 因此一些只读物理区域. FIXMAP_PAGE_CLEAR 则定义为 \_\_pgprot(0) 空的页表内容，那么可以用于清空最后一级页表 Entry.

-------------------------------

##### 永久映射内存释放

![](/assets/PDB/HK/TH002003.png)

由于永久映射的特点，当不再使用分配器分配内存时，仅仅需要将最后一级页表 Entry 内容清零即可，其余虚拟内存一直存在并保持原有的含义，这样从语义上保持了虚拟内存的永久性.

![](/assets/PDB/HK/TH002004.png)

分配器提供了 clear_fixmap() 函数释放永久映射分配分配的虚拟内存，其实现与建立永久映射页表的 set_fixmap_offset() 函数采用同样的代码流程，只是其在 \_\_set_fixmap_offset() 函数传入了 FIXMAP_PAGE_CLEAR 参数，那么代码流程最后的 \_\_set_pte_vaddr() 函数会调用 set_pte() 函数传入 0，以此清除最后一级页表 Entry 里的内容，清除完毕之后刷新一下 TLB 对应的项.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

#### 永久映射分配器使用

> - [永久映射外设 MMIO/物理内存](#B1)
>
> - [永久映射为不带 Cache](#B2)
>
> - [永久映射 IO](#B3)
>
> - [新增永久映射](#B4)

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

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

##### 永久映射分配器实践

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




















