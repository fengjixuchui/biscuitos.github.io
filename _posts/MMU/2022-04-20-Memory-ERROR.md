---
layout: post
title:  "Memory ERROR Collection"
date:   2022-04-20 12:00:00 +0800
categories: [HW]
excerpt: Memory Error.
tags:
  - Memory
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [GFP Flags 标志混淆使用导致 Kernel BUG](#A00A00)
>
>   - [PCP 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A01)
>
>   - [Buddy 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A02)
>
>   - [SLAB/SLUB/SLOB 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A03)
>
>   - [VMALLOC 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A04)
>
> - 永久映射内存分配器(Permanent Mapping Memory Allocator) BUG
>
>   - [永久映射内存分配器占用的内存超限 BUG](#A00A05)

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A00A00"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### GFP Flags 标志混淆使用导致 Kernel BUG

![](/assets/PDB/HK/TH001557.png)

GFP Flags 标志是在分配系统内存时指定内存的特性，常见的标识为 GFP_KERNEL、GFP_HIGHUSER、GFP_USE 等多个标志，这些标志可以单独使用，也可以多个标志组合使用。不同的标志导致分配的内存具有不同的属性，例如 GFP_KERNEL 分配的内存来自普通内存，又例如 \_\_GFP_DMA32 分配的内存优先来自 ZONE_DMA32 区域，还比如 GFP_HIGHUSER 分配的内存可能来自 ZONE_HIGHMEM 或者 ZONE_NORMAL. 单独使用 GFP 标志不太可能直接导致错误，但也会因为指定属性的内存不足导致间接错误。GFP 标志组合使用往往更容易引发 BUG，那么本案例就讲解一个由 GFP 标志错误组合使用导致的内核 BUG.

![](/assets/PDB/HK/TH001558.png)

如上图所示，这个 BUG 的特点是提示 **kernel BUG at ./include/linux/gfp.h:425!**. 不同的内核可以不是 425 这个值。那么接下来在 BiscuitOS 分析一下该案例，其在 BiscuitOS 上部署逻辑如下(以 linux 5.0 X64_64 架构为例):

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] Memory Error Collect (Kernel/Userspace) --->
          [*] Confuse GFP Flags trigger Kernel BUG --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-default/
# 下载案例源码
make download
{% endhighlight %}

> [BiscuitOS-MM-ERROR-Confuse-GFP-Flags Gitee Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MM-ERROR/BiscuitOS-MM-ERROR-Confuse-GFP-Flags)
>
> [BiscuitOS 独立模块部署手册](https://biscuitos.github.io/blog/Human-Knowledge-Common/#B1)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001559.png)
![](/assets/PDB/HK/TH001560.png)
![](/assets/PDB/HK/TH001561.png)
![](/assets/PDB/HK/TH001562.png)

案例源码通过一个驱动进行展示，在驱动的初始化函数 BiscuitOS_init() 中，27 行调用 page_alloc() 函数分配一个物理页，此时分配使用的 GFP 标志是: \_\_GFP_DMA32 和 \_\_GFP_DMA, 当物理页分配完毕之后，函数在 34 行调用 \_\_free_page() 函数释放了物理页. 案例代码很精简，核心代码落在 27 行处，那么接下来使用如下命令在 BiscuitOS 上实践案例代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-default/
# 编译源码
make
# 安装驱动
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run

# BiscuitOS 运行之后安装驱动模块
insmod /lib/modules/5.0.0/extra/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-default.ko
{% endhighlight %}

![](/assets/PDB/HK/TH001563.png)

现在可以看到只要内核模块一安装问题就 100% 复现，那么接下来就是问题排查过程. 从出错的 LOG 可以看到 CPU 的调用栈、寄存器信息以及关键的字段 **kernel BUG at ./include/linux/gfp.h:425!**, 可以看出 BUG 出现在 gfp.h 文件的 425 行，那么接下里就是打开 gfp.h 文件进行查看:

![](/assets/PDB/HK/TH001564.png)

可以看出 BUG 是因为 "(GFP_ZONE_BAD >> bit) & 1" 条件满足了，那么来分析一下造成该问题的原因，首先来分析一下 gfp_zone() 函数，该函数主要目的是通过参数 flags，也就是 GFP 标识来确认所分配的物理内存来自哪个 ZONE。

![](/assets/PDB/HK/TH001565.png)

在 Linux 中不同的架构中，物理内存按功能划分成不同的区域，以 x86_64 为例来说明，物理内存最开始的 1MiB 区域，也就是 \[0x00000000, 0x00100000) 区域，该区域称为 ZONE_DMA, 其内存用来给老式设备做 DMA 使用; 物理内存 1MiB 到前 4GiB 的区域，也就是 \[0x00100000, 0x100000000) 区域，该区域称为 ZONE_DMA32, 其物理内存优先为设备做 DMA 使用，其次可做普通内存使用; 物理内存超过 4GiB 的区域，其内存供给内核和用户空间进程使用. 另外例如 i386 和 ARM 架构，由于其总线只有 32 位，因此存在内核空间虚拟地址不足以映射所有物理内存的区域，这部分物理内存区域称为 ZONE_HIGHMEM, 该区域的内存供内核非线性映射区和用户空间进行使用.

![](/assets/PDB/HK/TH001566.png)

GFP 标志中存在用于指明 ZONE 属性的 Flags，如 \_\_GFP_DMA 标志表示期望物理内存来自 ZONE_DMA 区域, \_\_ZONE_DMA32 标志表示期望物理内存来自 ZONE_DMA32 区域, \_\_GFP_HIGHMEM 标志表示期望物理内存来自 ZONE_HIGHMEM 区域，最后 \_\_GFP_MOVABLE 标志表示期望内存来自 ZONE_MOVABLE 区域. 另外如果分配内存的标志都不包含这些标志，那么物理内存来自 ZONE_NORMAL 区域. Linux 使用 GFP_ZONEMASK 宏从 GFP 标志集合中隔离出 ZONE 字段.

![](/assets/PDB/HK/TH001567.png)

另外 ZONE_DMA32 和 ZONE_HIGHMEM 等区域不是在所有的架构中都存在，那么 GFP 标志为了兼容不同的架构，内核定义了 GFP_ZONE_TABLE 分配表，该表简单理解为通过 GFP 标志找到可用的 ZONE 区域，例如不包含任何 GFP ZONE 字段标志的情况下，内存来自 ZONE_NORMAL, 以及 \_\_GFP_MOVABLE 标志的内存来自 ZONE_NORMAL. 另外对于 \_\_GFP_HIGHMEM 标志，其物理内存来自 OPT_ZONE_HIGHMEM, 对于存在 ZONE_HGIHMEM 的架构，OPT_ZONE_HIGHMEM 指向 ZONE_HIGHMEM, 而对于 ZONE_NORMAL. 另外 GFP_ZONE_TABLE 也定义了多个 GFP ZONE 标志组合使用的情况，例如 \_\_GFP_MOVABLE 标志可以和 \_\_GFP_DMA/\_\_GFP_DMA32/\_\_GFP_HIGHMEM 组合使用。因此 GFP_ZONE_TABLE 表即定义了 GFP ZONE 标志分配物理内存的来源，由规定了 GFP ZONE 标志可以组合的情况.

![](/assets/PDB/HK/TH001568.png)

内核同样也定义了 GFP_ZONE_BAD 表，该表由于判断错误的 GFP ZONE 标志组合，例如 \_\_GFP_DMA 不能和 \_\_GFP_HIGHMEM 标志组合，如果能组合那么可以理解为本来老式设备需要分配 ZONE_DMA 内存做 DMA 操作，结果内核分配 ZONE_HIGHMEM 内存，这样显然是错误的. GFP_ZONE_BAD 表把所有错误的 GFP ZONE 标志组合都定义出来，用于隔离出分配请求中错误的 GFP 标志.

![](/assets/PDB/HK/TH001564.png) 

那么再次回到 gfp_zone() 函数，其逻辑就很好理解，首先在 421 行从 flags 参数中隔离出 GFP ZONE 字段，然后 423 行在 GFP_ZONE_TABLE 中查找对应的 ZONE 区域，接着函数在  425 行检查 GFP ZONE 组合标志是否满足 GFP_ZONE_BAD 的情况，如果满足那么触发内核 BUG。通过上面的分析可以一下组合的 GFP Flags 会触发这个 BUG:

{% highlight bash %}
# e.g. Allocate memory from Buddy/PCP
alloc_page(__GFP_DMA | __GFP_HIGHMEM)
alloc_page(__GFP_DMA | __GFP_DMA32)
alloc_page(__GFP_DMA32 | __GFP_HIGHMEM)
alloc_page(__GFP_DMA | __GFP_DMA32 | __GFP_HIGHMEM)
alloc_page(__GFP_MOVABLE | __GFP_DMA | __GFP_HIGHMEM)
alloc_page(__GFP_MOVABLE | __GFP_DMA | __GFP_DMA32)
alloc_page(__GFP_MOVABLE | __GFP_DMA32 | __GFP_HIGHMEM)
alloc_page(__GFP_MOVABLE | __GFP_DMA | __GFP_DMA32 | __GFP_HIGHMEM)
{% endhighlight %}

相反以下 GFP Flags 标志组合使用不会触发该 BUG:

{% highlight bash %}
# e.g. Allocate memory from Buddy/PCP
alloc_page(GFP_KERNEL)
alloc_page(__GFP_DMA)
alloc_page(__GFP_HIGHMEM)
alloc_page(__GFP_DMA32)
alloc_page(__GFP_MOVABLE)
alloc_page(__GFP_MOVABLE | __GFP_DMA)
alloc_page(__GFP_MOVABLE | __GFP_HIGHMEM)
alloc_page(__GFP_MOVABLE | __GFP_DMA32)
alloc_page(GFP_KERNEL | __GFP_DMA)
alloc_page(GFP_KERNEL | __GFP_HIGHMEM)
alloc_page(GFP_KERNEL | __GFP_DMA32)
alloc_page(GFP_KERNEL | __GFP_MOVABLE)
alloc_page(GFP_KERNEL | __GFP_MOVABLE | __GFP_DMA)
alloc_page(GFP_KERNEL | __GFP_MOVABLE | __GFP_HIGHMEM)
alloc_page(GFP_KERNEL | __GFP_MOVABLE | __GFP_DMA32)
{% endhighlight %}

经过上面的分析，只要混淆组合使用 GFP Flags 就会触发该 BUG，那么该 BUG 也会处在多个分配器，例如 PCP、SLAB/SLUB/SLOB 以及 VMALLOC 等多个分配器里，那么接下就在各个分配器里进行实践验证。

> - [PCP 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A01)
>
> - [Buddy 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A02)
>
> - [SLAB/SLUB/SLOB 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A03)
>
> - [VMALLOC 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG](#A00A04)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

<span id="A00A01"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

###### PCP 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG

基于 <[GFP Flags 标志混淆使用导致 Kernel BUG](#A00A00)> 章节对原理的分析，本节直接通过 PCP 分配器混淆使用 GFP Flags 标志的代码进行分析，BiscuitOS 已经支持案例源码的部署，开发者参考使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] Memory Error Collect (Kernel/Userspace) --->
          [*] .... [PCP] Confuse GFP Flags trigger Kernel BUG --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-PCP-default/
# 下载案例源码
make download
{% endhighlight %}

> [BiscuitOS-MM-ERROR-Confuse-GFP-Flags-PCP Gitee Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MM-ERROR/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-PCP)
>
> [BiscuitOS 独立模块部署手册](https://biscuitos.github.io/blog/Human-Knowledge-Common/#B1)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001559.png)
![](/assets/PDB/HK/TH001569.png)
![](/assets/PDB/HK/TH001570.png)
![](/assets/PDB/HK/TH001571.png)

案例源码通过一个驱动进行展示，在驱动的初始化函数 BiscuitOS_init() 中，27 行调用 page_alloc() 函数从 PCP 分配器分配一个物理页，此时分配使用的 GFP 标志是: \_\_GFP_DMA32 和 \_\_GFP_DMA, 当物理页分配完毕之后，函数在 34 行调用 \_\_free_page() 函数将物理页释放回 PCP 分配器. 案例代码很精简，核心代码落在 27 行处，此时会触发 "Confuse GFP Flags BUG", 那么接下来使用如下命令在 BiscuitOS 上实践案例代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-PCP-default/
# 编译源码
make
# 安装驱动
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run

# BiscuitOS 运行之后安装驱动模块
insmod /lib/modules/5.0.0/extra/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-PCP-default.ko
{% endhighlight %}

![](/assets/PDB/HK/TH001572.png)

可以看到只要模块一安装就触发 "Confuse GFP Flags BUG", 从错误的 Log 可以看到 **kernel BUG at ./include/linux/gfp.h:425!** 字符串，那么验证了 PCP 分配器同样存在该 BUG.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

<span id="A00A02"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

###### Buddy 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG

基于 <[GFP Flags 标志混淆使用导致 Kernel BUG](#A00A00)> 章节对原理的分析，本节直接通过 Buddy 分配器混淆使用 GFP Flags 标志的代码进行分析，BiscuitOS 已经支持案例源码的部署，开发者参考使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] Memory Error Collect (Kernel/Userspace) --->
          [*] .... [Buddy] Confuse GFP Flags trigger Kernel BUG --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-Buddy-default/
# 下载案例源码
make download
{% endhighlight %}

> [BiscuitOS-MM-ERROR-Confuse-GFP-Flags-Buddy Gitee Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MM-ERROR/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-Buddy)
>
> [BiscuitOS 独立模块部署手册](https://biscuitos.github.io/blog/Human-Knowledge-Common/#B1)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001559.png)
![](/assets/PDB/HK/TH001573.png)
![](/assets/PDB/HK/TH001574.png)
![](/assets/PDB/HK/TH001575.png)

案例源码通过一个驱动进行展示，在驱动的初始化函数 BiscuitOS_init() 中，27 行调用 page_allocs() 函数从 Buddy 分配器分配四个物理页，此时分配使用的 GFP 标志是: \_\_GFP_DMA32 和 \_\_GFP_DMA, 当物理页分配完毕之后，函数在 34 行调用 \_\_free_pages() 函数将四个物理页释放回 Buddy 分配器. 案例代码很精简，核心代码落在 27 行处，此时会触发 "Confuse GFP Flags BUG", 那么接下来使用如下命令在 BiscuitOS 上实践案例代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-Buddy-default/
# 编译源码
make
# 安装驱动
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run

# BiscuitOS 运行之后安装驱动模块
insmod /lib/modules/5.0.0/extra/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-Buddy-default.ko
{% endhighlight %}

![](/assets/PDB/HK/TH001576.png)

可以看到只要模块一安装就触发 "Confuse GFP Flags BUG", 从错误的 Log 可以看到 **kernel BUG at ./include/linux/gfp.h:425!** 字符串，那么验证了 Buddy 分配器同样存在该 BUG.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

<span id="A00A03"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

###### SLAB/SLUB/SLOB 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG

基于 <[GFP Flags 标志混淆使用导致 Kernel BUG](#A00A00)> 章节对原理的分析，本节直接通过 Buddy 分配器混淆使用 GFP Flags 标志的代码进行分析，BiscuitOS 已经支持案例源码的部署，开发者参考使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] Memory Error Collect (Kernel/Userspace) --->
          [*] .... [SLAB/SLUB/SLOB] Confuse GFP Flags trigger Kernel BUG --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-SLAB-default/
# 下载案例源码
make download
{% endhighlight %}

> [BiscuitOS-MM-ERROR-Confuse-GFP-Flags-SLAB Gitee Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MM-ERROR/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-SLAB)
>
> [BiscuitOS 独立模块部署手册](https://biscuitos.github.io/blog/Human-Knowledge-Common/#B1)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001559.png)
![](/assets/PDB/HK/TH001577.png)
![](/assets/PDB/HK/TH001578.png)
![](/assets/PDB/HK/TH001579.png)

案例源码通过一个驱动进行展示，在驱动的初始化函数 BiscuitOS_init() 中，26 行调用 kmalloc() 函数从 SLAB/SLUB/SLOB 分配器分配 256 个字节，此时分配使用的 GFP 标志是: \_\_GFP_DMA32 和 \_\_GFP_DMA, 当 256 字节分配完毕之后，函数在 32 行调用 kfree() 函数将 256 字节释放回 SLAB/SLUB/SLOB 分配器. 案例代码很精简，核心代码落在 26 行处，此时会触发 "Confuse GFP Flags BUG", 那么接下来使用如下命令在 BiscuitOS 上实践案例代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-SLAB-default/
# 编译源码
make
# 安装驱动
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run

# BiscuitOS 运行之后安装驱动模块
insmod /lib/modules/5.0.0/extra/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-SLAB-default.ko
{% endhighlight %}

![](/assets/PDB/HK/TH001580.png)

可以看到只要模块一安装就触发 "Confuse GFP Flags BUG", 从错误的 Log 可以看到 **kernel BUG at ./include/linux/gfp.h:425!** 字符串，那么验证了 SLAB/SLUB/SLOB 分配器同样存在该 BUG.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

<span id="A00A04"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000V.jpg)

######VMALLOC 内存分配器 GFP Flags 标志混淆使用导致 Kernel BUG

基于 <[GFP Flags 标志混淆使用导致 Kernel BUG](#A00A00)> 章节对原理的分析，本节直接通过 VMALLOC 分配器混淆使用 GFP Flags 标志的代码进行分析，BiscuitOS 已经支持案例源码的部署，开发者参考使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package --->
      [*] Memory Error Collect (Kernel/Userspace) --->
          [*] .... [VMALLOC] Confuse GFP Flags trigger Kernel BUG --->

make
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-VMALLOC-default/
# 下载案例源码
make download
{% endhighlight %}

> [BiscuitOS-MM-ERROR-Confuse-GFP-Flags-VMALLOC Gitee Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MM-ERROR/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-VMALLOC)
>
> [BiscuitOS 独立模块部署手册](https://biscuitos.github.io/blog/Human-Knowledge-Common/#B1)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001559.png)
![](/assets/PDB/HK/TH001581.png)
![](/assets/PDB/HK/TH001582.png)
![](/assets/PDB/HK/TH001583.png)

案例源码通过一个驱动进行展示，在驱动的初始化函数 BiscuitOS_init() 中，26 行调用 \_\_vmalloc() 函数从 VMALLOC 分配器分配 2MiB 的内存，此时分配使用的 GFP 标志是: \_\_GFP_DMA32 和 \_\_GFP_DMA, 当 2MiB 内存分配完毕之后，函数在 32 行调用 vfree() 函数将 2MiB 内存释放回 VMALLOC 分配器. 案例代码很精简，核心代码落在 26 行处，此时会触发 "Confuse GFP Flags BUG", 那么接下来使用如下命令在 BiscuitOS 上实践案例代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-VMALLOC-default/
# 编译源码
make
# 安装驱动
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run

# BiscuitOS 运行之后安装驱动模块
insmod /lib/modules/5.0.0/extra/BiscuitOS-MM-ERROR-Confuse-GFP-Flags-VMALLOC-default.ko
{% endhighlight %}

![](/assets/PDB/HK/TH001584.png)

可以看到只要模块一安装就触发 "Confuse GFP Flags BUG", 从错误的 Log 可以看到 **kernel BUG at ./include/linux/gfp.h:425!** 字符串，那么验证了 VMALLOC 分配器同样存在该 BUG.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

#### 永久映射内存分配器 BUG

-------------------------------------------

<span id="A00A05"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

##### 永久映射内存分配器占用的内存超限

![](/assets/PDB/HK/TH002052.png)

当向永久内存映射分配器中新增一块虚拟内存，采用了上图的代码逻辑，新增的 IDX 为 (2 * PTRS_PER_PTE), 也就是 IDX 等于 1024 接下来使用测试用例:

{% highlight bash %}
cd BiscuitOS/
make linux-X.Y.Z-${ARCH}\_defconfig
make menuconfig

  [*] Package --->
      [*] Memory Error Collect (Kernel/Userspace) --->
          [*] Permanent Mapping BUG: Overflow Range --->

make
cd BiscuitOS/output/linux-X.Y.Z-${ARCH}/package/BiscuitOS-MM-ERROR-Permanent-Overflow-default/
# 下载案例源码
make download
{% endhighlight %}

> [BiscuitOS-MM-ERROR-Permanent-Overflow Gitee Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/FIXMAP/BiscuitOS-Permanent)
>
> [BiscuitOS 独立模块部署手册](https://biscuitos.github.io/blog/Human-Knowledge-Common/#B1)

![](/assets/PDB/HK/TH001502.png)
![](/assets/PDB/HK/TH001559.png)
![](/assets/PDB/HK/TH002053.png)
![](/assets/PDB/HK/TH002011.png)

程序源码很精简，程序在 21 行调用 set_fixmap_io() 函数将 BROILER_FIXMAP_IDX 对应的虚拟内存映射到外设 MMIO 地址 BROILER_MMIO_BASE 上，然后调用 virt_to_fix() 函数将映射的虚拟地址存储在 addr 变量里。接下来程序直接在 24 行访问 addr 进而访问外设 MMIO, 但使用完毕之后，可以使用 clear_fixmap() 函数清除页表即可. 源码没有问题，但一旦编译就报错，那么接下来使用如下命令在 BiscuitOS 上实践案例代码:

{% highlight bash %}
cd BiscuitOS/output/linux-X.Y.Z-${ARCH}/package/BiscuitOS-MM-ERROR-Permanent-Overflow-default/
# 编译源码
make
# 安装驱动
make install
# Rootfs 打包
make pack
# 运行 BiscuitOS
make run
{% endhighlight %}

![](/assets/PDB/HK/TH002051.png)

一旦编译编译器就报错，通过提示的信息可以看到是因为 BUILD_BUG_ON() 在编译阶段检查到了错误，这个错误是 BUILD_BUG_ON(\_\_end_of_permanent_fix_address) 越界了。内核源码只修改了 enum fixed_addresses，仅仅是新增加了一个 IDX FIX_BISCUITOS，其值为 (2 * PTRS_PER_PTE), 那么为什么新增加一个 IDX 就触发编译错误呢? 首先从案例分析，案例中调用了 set_fixmap_io() 函数:

![](/assets/PDB/HK/TH002054.png)
 
set_fixmap_io() 函数最终会调用 \_\_native_set_fixmap() 函数，其实现逻辑如上，在 51 行处函数调用 BUILD_BUG_ON() 函数在编译阶段对 \_\_end_of_permanent_fixed_addresses 的值进行检查，该值表示永久映射分配器支持的最大 IDX，通过该 IDX 可以知道永久映射支持最大的虚拟内存范围. 从 51 行可以看到如果 \_\_end_of_permanent_fixed_addresses 大于 (FIXMAP_PMD_NUM * PTRS_PER_PTE) 时编译器就会报错，那么内核为什么要加入这个限制呢?

![](/assets/PDB/HK/TH002001.png)
![](/assets/PDB/HK/TH002002.png)

这个问题还要从永久映射分配器的起源说起，既然是永久，那么其虚拟地址从源码编译解决到系统运行时，其含义一直没变，如果做到不变，内核通过保持永久映射维护的虚拟地址对应的页表都是固定不变即可，那么内核从编译时就为永久映射分配了所有页表页占用的内存，这样无论在内核启动早期还是内核运行中，其虚拟内存对应的页表页维持不变。如上图中 level3_kernel_pgt、level2_fixmap_pgt 和 level1_fixmap_pgt 之间的关系一直保持不变. 由于这个关系的存在这些页表的大小决定了永久映射分配器维护虚拟内存的大小。内核为永久映射分配了 FIXMAP_PMD_NUM 个 PTE 页表，那么其维护的范围为 FIXMAP_PMD_NUM * PTRS_PER_PTE 个 4KiB 虚拟内存。通过上面的分析，如果在永久映射分配器中新增一个 IDX，那么这个 IDX 不能超过 FIXMAP_PMD_NUM * PTRS_PER_PTE，否则编译器就直接报错.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------



