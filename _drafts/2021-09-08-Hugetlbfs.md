---
layout: post
title:  "Hugetlbfs and Transparent Hugepages Mechanism"
date:   2021-09-08 06:00:00 +0800
categories: [HW]
excerpt: HugePage.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [Hugepage 原理](#A)
>
> - [Hugepage 使用](#B)
>
> - [Hugepage 实践](#C)
>
> - [Hugepage 源码分析](#D)
>
> - [Hugepage 调试](#E)
>
> - Hugepage 进阶研究
>
>   - 通过 CMDLINE 配置大页长度和大页数量
> 
>     - [hugepagesz= hugepages= default_hugepagesz=]()
>
>   - CMDLINE 中大页字段的先后顺序对初始化的影响
>
>   - 大页所需的内核宏支持
>
>   - CMDLINE 传递错误的大页数量
>
>   - IA-32 支持大页功能
>
>   - 多 NUMA NODE 模式下的大页
>
>   - 如果判断一个复合页是大页，如果判断 HugeHeadPage.
>
>   - 大页状态转变 Active/in-use/Temporary
>
>   - 挂载 Hugetlbfs 文件系统参数研究
>
>   - pgoff 没有对齐对大页映射的影响
>
>   - 大页在 fork() 时的影响
>
>   - 大页不足引起的 OOM
>
>   - 大页不足导致的 PANIC
>
>   - 内存分配策略 mempolicy 对大页的影响
>
>   - madvise 对大页分配的影响研究
>
>   - 大页的预留机制研究
>
>   - hugetlbfs 文件系统有名挂载和匿名挂载研究
>
>   - 大页大内存池(主内存池)和小内存池(子内存池) 研究
>
>   - 大页文件的大小研究
>
>   - 大页为啥成为大页，与普通页如何区别
>
>   - 大页对齐研究
>
>   - 大页内存超发机制研究
>
>   - 大页水位线研究
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

#### Hugetlbfs 源码分析

> - [Hugetlbfs 架构](#D00)
>
> - [Hugetlbfs 数据结构](#D01)
>
>   - [enum hugetlbfs_size_type]()
>
>   - [struct hugepage_subpool]()
>
>   - [struct huge_bootmem_page]()
>
>   - [struct hugetlbfs_config]()
>
>   - [struct hugetlbfs_inode_info]()
>
>   - [struct hugetlbfs_sb_info]()
>
>   - [struct node_hstate]()
>
> - [Hugetlbfs 重要数据](#D0305)
>
>   - [default_hstate]()
>
>   - [default_hstate_size](#D0300)
>
>   - [hstate_attr_group]()
>
>   - [hstate_attrs]()
>
>   - [hstate_kobjs]()
>
>   - [HUGE_MAX_HSTATE](#D0303)
>
>   - [huge_boot_pages]()
>
>   - [HUGETLBFS_MAGIC]()
>
>   - [hugetlb_max_hstate](#D0302)
>
>   - [hugetlbfs_size_type]()
>
>   - [hugetlbfs_vfsmount]()
>
>   - [minimum_order](#D0304)
>
>   - [node_hstates]()
>
>   - [parsed_valid_hugepagesz](#D0301)
>
> - [Hugetlbfs API](#D02036)
>
>   - [alloc_bootmem_huge_page](#D02011)
>
>   - [alloc_buddy_huge_page](#D02023)
>
>   - [alloc_pool_huge_page](#D02025)
>
>   - [clear_page_huge_active](#D02018)
>
>   - [enqueue_huge_page](#D02019)
>
>   - [for_each_hstate](#D02004)
>
>   - [HUGETLBFS_SB](#D02032)
>
>   - [hstate_file](#D02035)
>
>   - [hstate_index](#D02012)
>
>   - [hstate_inode](#D02034)
>
>   - [hstate_is_gigantic](#D2008)
>
>   - [hstate_sizelog](#D02031)
>
>   - [htlb_alloc_mask](#D02022)
>
>   - [huge_page_order](D02009)
>
>   - [huge_page_shift](#D02020)
>
>   - [huge_page_size](#D02005)
>
>   - [hugepage_migration_supported](#D02021)
>
>   - [hugepages_supported](#D02000)
>
>   - [hugetlb_add_hstate](#D02007)
>
>   - [hugetlb_bad_size](#D02003)
> 
>   - [hugetlb_default_setup](#D02001)
>
>   - [hugetlb_hstate_alloc_pages](#D02026)
>
>   - [hugetlb_init_hstates](#D02027)
>
>   - [hugetlb_nrpages_setup](#D02010)
>
>   - [hugetlb_sysfs_add_hstate](#D02029)
>
>   - [hugetlb_sysfs_init](#D02030)
>
>   - [init_hugetlbfs_fs](#D02033)
>
>   - [PageHeadHuge](#D02016)
>
>   - [PageHuge](#D02013)
>
>   - [page_huge_active](#D02017)
>
>   - [page_hstate](#D02014)
>
>   - [prep_new_huge_page](#D02024)
>
>   - [report_hugepages](#D02028)
>
>   - [set_page_huge_active](#D02015)
>
>   - [setup_hugepagesz](#D02002)
>
>   - [size_to_hstate](#D02006)

------------------------------------

###### <span id="D0200X"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000334.png)

{% highlight c %}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02000">hugepages_supported</span>

{% highlight c %}
#define hugepages_supported() boot_cpu_has(X86_FEATURE_PSE)
{% endhighlight %}

hugepages_supported() 函数用于判断系统是否支持 Hugetlbfs 大页功能。所谓大页功能就是支持 2MiB 或者 1Gig 的物理页。要支持大页，那么硬件页表就必须支持 X86_FEATURE_PSE, 该 FEATURE 来自 CPUID 的 PSE 标志。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000998.png)

"32-Bit Paging" 模式映射物理页的大小与 CPUID.01.PSE 标志位、CR4.PSE 标志位以及 PDE.PS 标志位有直接联系，CPUID.01.PSE 标志位用于指明 "32-Bit Paging" 模式是否具有映射 4MiB 物理页的能力, 如果该标志位清零，那么不具有该能力，此时页表只能映射 4KiB 物理页; 反之 CPUID.01.PSE 标志位置位，那么页表具有能力映射 4MiB 物理页，但此时还与 CR4.PSE 标志位和 PDE.PS 标志位有关。CR4.PSE 标志位用于指明是否启用映射 4MiB 物理页的功能，即在 CPUID.01.PSE 置位的情况下，CR4.PSE 标志位置位，那么 "32-Bit Paging" 模式可以启用映射 4MiB 物理页的能力，但此时还与 PDE.PS 标志位有关. PDE.PS 标志位用于指明 "Page Directory Table" 的 PDE 是否映射 4MiB 物理页，如果该标志位置位，那么当前页表正在映射一个 4MiB 的物理页; 反之 PDE.PS 标志位清零，那么当前页表在映射一个 4KiB 的物理页。如果 CPUID.01H.PSE 标志位清零, 那么 "32-Bit Paging" 模式只能映射 4KiB 物理页. 在 "4-level paging" 或者 "5-level paging" 模式下，CPUID.PSE 启用，那么系统就可以映射 2MiB 或者 1Gig 的物理页.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D0300">default_hstate_size</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000940.png)

default_hstate_size 用于设置默认 Hugetlbfs 大页的长度，其在系统启动时通过 CMDLINE "default_hugepagesz=" 字段进行设置。其值可以是 0x200000 或者为 0x40000000. 如果 CMDLINE 设置了该值，那么该值合法，那么系统将使用该值作为 Hugetlbfs 大页的长度。如果 CMDLINE 中没有 "default_hugepagesz=" 字段，那么系统使用 HPAGE_SIZE 作为大页的默认长度.

> [hugetlb_default_setup](#D02001)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02001">hugetlb_default_setup</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000941.png)

hugetlb_default_setup() 函数用于截取 CMDLINE 中的 "default_hugepagesz=" 字段，并将字段的内容转储到 default_hstate_size 变量中，default_hstate_size 变量将决定系统 Hugetlbfs 大页的长度.

> [default_hstate_size](#D0300)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02002">setup_hugepagesz</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000942.png)

setup_hugepagesz() 函数用于截取 CMDLINE 中的 "hugepagesz=" 字段，并检查该字段的值是否合法，如果合法为该字段的长度创建大页相关的 struct hstate 数据结构. 每种类型的大页都需要 struct hstate 数据结构进行管理和描述，系统默认支持的大页长度包括 PMD_SIZE 和 PUD_SIZE，当 "hugepagesz=" 字段值为 PMD_SIZE 时，函数调用 hugetlb_add_hstate() 函数将 PMD_SIZE 长度的大页加入到系统，那么系统就支持 2M 的大页。如果 "hugepagesz=" 字段值为 PUD_SIZE, 且系统支持 X86_FEATURE_GBPAGES 特性，那么系统将z支持 PUD_SIZE 长度的大页，即 1Gig 的大页。如果 "hugepagesz=" 字段值为其他值，那么系统将会报错，并通过调用 hugetlb_bad_size() 函数将 parsed_valid_hugepagesz 设置为 false，这将会告诉后续 Hugetlbfs 正在使用一个错误的尺寸，需要纠正为默认的大页长度.

> [hugetlb_add_hstate](#D02007)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D0301">parsed_valid_hugepagesz</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000940.png)

parsed_valid_hugepagesz 用于判断从 CMDLINE 中解析大页长度是否可用。CMDLINE 提供了 "hugepagesz=" 字段去设置大页的长度，目前支持 PMD_SIZE 和 PUD_SIZE 两种大页，如果将该字段设置为其他值，那么系统会将 parsed_valid_hugepagesz 设置为 false，这样内核在初始化 Hugetlbfs 大页长度的时候就不会使用 CMDLINE 中的数据，而是使用系统默大页长度 HPAGE_SIZE.

> [hugetlb_default_setup](#D02001)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02003">hugetlb_bad_size</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000943.png)

hugetlb_bad_size() 函数用于将 parsed_valid_hugepagesz 标记为 false. 在系统初始化阶段会从 CMDLINE 中获得 "hugepagesz=" 字段来设置大页的长度，如果该值不在大页支持的范围内，那么系统调用 hugetlb_bad_size() 函数将 parsed_valid_hugepagesz 标记为 false，以此告诉后续的 hugetlbfs 初始化过程中不要使用 CMDLINE 中的大页长度作为最终的大页长度.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02004">for_each_hstate</span>

{% highlight c %}
#define for_each_hstate(h) \
        for ((h) = hstates; (h) < &hstates[hugetlb_max_hstate]; (h)++)
{% endhighlight %}

在内核中，每种长度的大页都使用 struct hstate 数据结构进行描述，并统一维护在 hstates[] 数组中，for_each_hsate() 函数就使用与遍历 hsates[] 数组中可用长度大页的 struct hstate 数据结构。hugetlb_max_hstates 表示当前系统最大可用大页种类。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D0302">hugetlb_max_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000944.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstates 数据结构描述某种长度的大页，并使用 hstates[] 数组将所有种类的大页统一维护在 hstates[] 数组中，hstates[] 数组的最大长度为 HUGE_MAX_HSTATE。在实际的内核中不一定都存在全部种类的大页，有的只可能是 2MiB 或者 1Gig 的大页，那么此时系统使用 hugetlb_max_hstate 描述当前系统最大可用大页种类数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02005">huge_page_size</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000945.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其成员 order 表示构成该大页所需 4KiB 页的阶数，例如 2MiB 的大页需要 4KiB 页的阶数为 9，即 2^9 个 4KiB 物理页构成一个 2MiB 的大页. huge_page_size() 函数就是通过 struct hstate 数据结构获得大页的长度，其逻辑是通过获得 struct hstate 的 order 成员，然后将 PAGE_SIZE 向左移动 order 位，即 (2^order * 4KiB).

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02006">size_to_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000946.png)

在 Hugetlbfs 大页机制中，每种指定长度的大页都是用 struct hstate 数据结构进行描述，并且将所有长度大页对应的 struct hstate 数据结构统一维护在 hstates[] 数组中。size_to_hstate() 函数调用 for_each_hstate() 函数遍历 hstates[] 数组中可用大页的 struct hstate，在每次遍历过程中，结合 struct hstate 数据结构，huge_page_size() 函数可以获得其所维护大页的长度，如果此时获得大页的长度正好等于参数 size，那么 size_to_hstate() 函数就找到大页长度与 size 参数一致的 struct hstate 数据结构. 如果 hstates[] 数组中维护的大页长度与 size 参数不匹配，那么 size_to_hstate() 函数将返回 NULL, 依次告诉调用者 size 参数对应长度的大页不存在.

> [for_each_hstate](#D02004)
>
> [huge_page_size](#D02005)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D0303">HUGE_MAX_HSTATE</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000944.png)

{% highlight c %}
#define HUGE_MAX_HSTATE 2 
{% endhighlight %}

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并将描述不同长度大页的 struct hstate 数据结构统一维护在 hstates[] 数组中，数组的默认长度就是 HUGE_MAX_HSTATE. 可以看到 HUGE_MAX_HSTATE 宏定义为 2。在 X86_64 架构中，内核支持的大页长度只有两种 2MiB 和 1Gig 的，因此该宏定义为 2.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02007">hugetlb_add_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000947.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并将描述不同长度大页的 struct hstate 数据结构统一维护在 hstates[] 数组中。当系统要新支持一种大页，那么内核就会调用 hugetlb_add_hstate() 函数进行添加，其逻辑如上图。order 参数表示新加入大页的阶数，即大页的长度为 "2^order * 4KiB", 接着函数首先在 18 行调用 size_to_hstate() 函数检查当前系统中是否已经存在同样长度的大页，如果存在函数就打印相应信息之后直接退出; 反之表明当前系统还没有指定长度的大页。hstates[] 数组的最大长度为 HUGE_MAX_HSTATE, 而当前系统中已经注册大页的数量使用 hugetlb_max_hstate 进行描述，如果此时函数在 22 行检查到 hugetlb_max_hstate 大于 HUGE_MAX_HSTATE, 那么 hstates[] 数组即将越界，那么函数通过 BUG_ON() 进行报错，另外如果检查到 order 参数为 0，那么 order 参数是非法参数，那么函数同样也通过 BUG_ON() 报警. 通过上面检查之后，函数从 hstates[] 数组中取出 hugetlb_max_hstate 对应最新可用的 struct hstate，之后将 hugetlb_max_hstate 加一操作指向 hstates[] 数组中下一个成员。函数在 25 行将 struct hstate 的 order 成员设置为 order 参数，以此表示大页的阶数，另外通过阶数算出大页的掩码，该掩码用于掩盖大页内部偏移。函数接着在 27 行将 nr_huge_pages 成员初始化为 0，以此表示当前指定长度的可用大页数量为 0，同理 free_huge_pages 成员设置为 0，也表示当前指定长度大页的空闲大页数量为 0. 在 struct hstate 数据结构中存在 hugepage_freelists[] 数组成员，数组由双链表构成，其用于维护不同 NUMA NODE 上的大页，那么函数在 29-30 行将 hugepage_freelists[] 数组内的各链表初始化。struct hstate 数据结构中存在 hugepage_activelist 成员，其由一个双链表构成，用于维护正在使用的大页，那么函数在 31 行将其 hugepage_activelist 链表进行初始化。struct hstate 数据结构的 next_nid_to_alloc 成员表示下一个可分配内存 NUMA NODE 节点，而 next_nid_to_free 成员表示下一个可以回收内存的 NUMA NODE 节点，函数在 32-33 行将两个成员都指向了 first_memory_node, first_memory_node 表示第一个存在内存的 NUMA NODE 节点。函数接着在 34 行调用 snprintf() 函数为大页设置一个名字，其名字规则是 "hugepages-XkB", 其中 X 为大页长度包含 KiB 的数量. 函数最后将 parsed_hstate 指向了新插入的 struct hstate 数据结构，以此表示内核正在解析的 struct hstate 大页.

> [first_memory_node](https://biscuitos.github.io/blog/NODEMASK/#D102)
>
> [size_to_hstate](#D02006)
>
> [hugetlb_max_hstate](#D0302)
>
> [HUGE_MAX_HSTATE](#D0303)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02008">hstate_is_gigantic</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000948.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其中 order 成员描述大页长度需要 4KiB 物理页的阶数，因此可以通过 order 成员知道大页的实际长度。在 x86_64 架构中，内核支持 2MiB 和 1Gig 长度的大页，那么 hstate_is_gigantic() 函数的目的就是确认 struct hstate 数据结构描述的大页是 2MiB 大页还是 1Gig 大页。函数首先调用 huge_page_order() 函数获得 struct hstate 数据结构的 order 成员，然后判断 order 成员是否大于获得等于 MAX_ORDER, 由于 2MiB 的 order 为 9，其值小于 MAX_ORDER, 因此当 order 大于 MAX_ORDER 是就可以认为 struct hstate 描述 1Gig 的大页; 反之 struct hstate 描述 2MiB 的大页.

> [huge_page_order](D02009)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02009">huge_page_order</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000949.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其中 order 成员描述大页长度为 4KiB 物理页的阶数，即大页的长度为 "2^order * 4KiB". huge_page_order() 函数用于获得 struct hstate 的 order 成员.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02010">hugetlb_nrpages_setup</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000950.png)

hugetlb_nrpages_setup() 函数用于截取 CMDLINE 中的 "hugepages=" 字段，如果存在该字段，并且字段值为 0x40000000, 那么函数会为 1Gig 的大页提前分配内存。内核可以在 CMDLINE 的 "default_hugepagesz=" 字段中设置系统默认大页的长度，如果该字段的值无效，那么会将 parsed_valid_hugepagesz 设置为 false. 回到函数 hugetlb_nrpages_setup() 中，函数首先检查 parsed_valid_hugepagesz 是否为真，如果为 false，那么函数将打印相应的信息，然后将 parsed_valid_hugepagesz 设置为 true 之后就直接返回了，因此预期的长度非法; 反之检查通过之后，函数继续检查 hugetlb_max_hstate 是否为空，如果系统已经注册了可用的大页，那么该值不为 0，如果该值为 0，那么函数认为系统还没有注册过任何长度的大页，函数将 mhp 指向了 default_hstate_max_huge_pages; 反之如果系统已经注册了某种长度的大页，那么 parsed_hstate 正好指向该种大页，那么将 mhp 指向 parsed_hstate 大页的 max_huge_pages 成员。函数此时在 60 行进行检查，如果 mhp 与 last_mhp 相同，那么表示 CMDLINE 中的 "hugepage=" 字段重复，是一个非法的配置，那么函数打印相应的信息之后直接退出; 反之检测通过，那么函数利用 sscanf() 函数将 s 参数的值转储成 unsigned long 变量存储到 mhp 里。函数接着在 73 行进行检测，如果 hugetlb_max_hstate 不为空，那么系统此时已经有某种长度的大页已经注册，另外该种大页的长度大于 2MiB，那么函数调用 hugetlb_hstate_alloc_pages() 进行大页内存分配。通过上面的逻辑可以知道，如果此时分配大页的长度为 2MiB，那么函数此时不会进行内存分配，而是等到 hugetlb_init() 函数里进行实际的内存分配; 反之如果大页的长度为 1Gig，那么函数会直接进行内存分配。最后函数将 last_mhp 设置为 mhp, 以防止 CMDLINE 中重复出现 "hugepages=" 字段.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02011">alloc_bootmem_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000951.png)

alloc_bootmem_huge_page() 函数用于从 MEMBLOCK 分配器为大页分配内存，参数 h 对应描述大页的 struct hstate 数据结构。从上图的定义可以看出，alloc_bootmem_huge_page() 函数使用了 \_\_attribute\_\_ 的 alias 属性，那么真实的函数应该是 \_\_alloc_bootmem_huge_page() 函数。在函数 101-114 行，函数在 MEMBLOCK 分配器中查找指定长度的物理内存，如果没有找到，那么函数返回 0; 反之如果找到，函数将 m 指向对应物理内存的虚拟地址，然后将跳转到 found 处。如果找到的物理内存没有按大页进行对齐，那么函数将报错。函数此时初始化了 struct huge_bootmem_page 的 list 成员，并将 m 加入到了 huge_boot_pages 链表里。最后将 struct huge_bootmem_page 的 hstate 成员指向了 h.

> [huge_page_size](#D02005)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02012">hstate_index</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000952.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并将所有的 struct hstate 数据结构维护在 hstates[] 数组中。hstate_index() 函数用于获得参数 h 在 hstates[] 数组中的索引，其通过 h 与 hstates[] 数组相减即可.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D0305">minimum_order</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000953.png)

minimum_order 变量用于描述内核所支持不同长度的大页中，长度最小的大页对应的 order. 大页的长度通过 "2^order * 4KiB" 获得，因此 minimum_order 可以描述最小大页的 order 阶数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02013">PageHuge</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000954.png)

在 Hugetlbfs 大页机制中，内核使用复合页 (Compound Page) 来构建大页。在 Compound Page 构成的大页中，Head Page 标记为 PG_head, 其余的 Tail Page 标记为 PG_tail. 但在 32bit 系统中没有那么多标记，因此使用 PG_compound 标记所有的页，而使用 PG_reclaim 标记所有的 Tail Page。因此可以使用 PageCompound() 函数判断一个页是否为复合页。另外在构成大页的复合页中的第一个尾页 Tail Page，将其 compound_dtor 指向了 HUGETLB_PAGE_DTOR. 因此 PageHuge() 函数判断一个页是否为大页的逻辑是: 首先调用 PageCompound() 函数判断物理页是否为复合页，接着判断复合页的第一个尾页的 compound_dtor 是否为 HUGETLB_PAGE_DTOR, 如果两个条件满足，那么函数认为这个物理页是一个大页.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02014">page_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000955.png)

在 Hugetlbfs 大页机制中，使用 buddy 提供的复合页构成物理大页，大页所使用的复合页的第一个尾页与其他复合页有所区别，第一个尾页的 compound_dtor 指向了 HUGETLB_PAGE_DTOR, 因此可以通过此特点判断物理页是否为大页。page_hstate() 函数的作用是找到大页对应的 struct hstate 数据结构。函数首先调用 PageHuge() 函数判断物理页是否为大页，如果不是则通过 VM_BUG_ON_PAGE() 函数报错; 反之如果是则通过 compound_order() 函数获得物理页的阶数，然后将 PAGE_SIZE 左移物理页的阶数，以此获得物理页的长度，最后 size_to_hstate() 函数基于物理页的长度找到大页对应的 struct hstate 数据结构。

> [PageHuge](#D02013)
>
> [size_to_hstate](#D02006)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02015">set_page_huge_active</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000956.png)

在 Hugetlbfs 大页机制中，使用 buddy 提供的复合页构成物理大页，大页使用复合页中第一个尾页表示大页是否处于激活状态。如果一个大页处于激活状态，那么复合页中第一个尾页的 PG_Private 标志置位，激活的大页会被维护在其对应 struct hstate 的 hugepage_activelist 链表上，激活页主要用于区分正在使用 in-use 的大页. set_page_huge_active() 函数用于将一个大页标记为激活态的大页，其逻辑: 首先调用 PageHeadHuge() 函数判断物理页是否为大页的头页，如果不是则通过 VM_BUG_ON_PAGE() 进行报错; 反之如果物理页是大页的头页，那么函数调用 SetPagePrivate() 函数设置第一个尾页的 PG_Private 标志。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02016">PageHeadHuge</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000957.png)

在 Hugetlbfs 大页机制中，内核从 buddy 分配器中分配的复合页构成了物理大页。构成物理大页的复合页为了和其他复合页区分开来，内核会将构成物理大页的复合页第一个尾页的 compound_dtor 指向 HUGETLB_PAGE_DTOR. 另外在复合页中，HugeHeadPage 就是复合页的 Head Page, 而剩余的页都不是 HugeHeadPage, 那么要判断一个物理页是否为 HugeHeadPage 的逻辑为: 首先判断物理页是否为复合页的 Head Page，由之前的讨论可知 Head Page 包含了 PG_head 或者包含 PG_compound 但不包含 PG_reclaim 标志，可以直接使用 PageHead() 函数进行判断，但物理页不是 Head Page, 那么该页也不是物理大页的 HugeHeadPage; 反之物理页是 Head Page, 那么接下来其后一个物理页的 compound_dtor 是否为 HUGETLB_PAGE_DTOR，此时调用 get_compund_page_dtor() 函数直接获得第一个尾页的 compound_dtor 对应的函数，如果此函数是 free_huge_page()，那么函数认为这个物理页是 HugeHeadPage.

> [PageHuge](#D02013)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02017">page_huge_active</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000958.png)

在 Hugetlbfs 大页机制中，内核从 buddy 分配器中分配的复合页构成了物理大页，构成物理大页的复合页与一般的复合页通过第一个尾页的 compound_dtor 进行区分，如果复合页是大页，那么 compound_dtor 指向 HUGETLB_PAGE_DTOR. 内核可以使用 PageHuge() 函数判断一个物理页是否为大页。大页可以分为激活态和正在使用态，内核使用复合页的第一个尾页是否包含 PG_Private 来判断大页是否处于激活态，如果包含，那么物理大页处于激活态; 反之不处于激活态. page_huge_active() 函数就根据上面的逻辑来判断大页是否处于激活态，其具体逻辑如: 函数首先调用 PageHuge() 函数判断物理是否属于大页，如果不是则调用 VM_BUG_ON_PAGE() 函数进行报错; 反之物理页是大页，接着函数调用 PageHead() 函数判断物理页是否为复合页的 Head Page，且复合页的第一个尾页是否包含了 PG_private 标志. 两个条件都满足，那么大页处于激活态.

> [PageHuge](#D02013)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02018">clear_page_huge_active</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000959.png)

在 Hugetlbfs 大页机制中，内核使用 buddy 分配器中分配的复合页作为物理大页，作为大页的复合页与一般复合页的不同点在于: 大页的复合页的第一个尾页的 compound_dtor 指向了 HUGETLB_PAGE_DTOR. 另外大页可以能处于激活态和 in-use 态，那么大页的复合页使用第一个尾页如果包含 PG_private, 那么该大页处于激活态, 反之处于非激活态. clear_page_huge_active() 函数用于将大页设置为非激活态，其逻辑为: 首先函数调用 PageHeadHuge() 判断物理页是否为复合页的 HugeHeadPage 页，如果不是则调用 VM_BUG_ON_PAGE() 直接报错; 反之函数调用 ClearPagePrivate() 函数清除复合页第一个尾页的 PG_private 标志, 至此大页的激活态被清除.

> [PageHeadHuge](#D02016)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02019">enqueue_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000960.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 管理指定长度的大页，其成员 free_huge_pages 用于描述指定长度大页的数量，成员 free_huge_pages_node[] 数组用于描述指定 NUMA NODE 上大页的数量，成员 hugepage_freelists[] 双链表数组用于维护各 NUMA NODE 上可用的物理大页. enqueue_huge_page() 函数用于将一个空闲的大页加入到指定长度的 struct hstate 数据结构中，其插入逻辑为: 首先函数通过 page_to_nid() 函数获得物理页所在的 NUMA NODE, 接着函数通过物理页 page->lru 插入到 struct hstate hugepage_freelists[] 指定 NODE 的链表上，并将 struct hstate 的 free_huge_pages 和指定 NUMA NODE 的 free_huge_page_node[] 数组进行加一操作，至此一个物理大页被加入到指定长度的 strut hstate 数据结构中进行维护.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02020">huge_page_shift</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000961.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其中 order 成员表示大页长度是 4KiB 的倍数阶数，即大页的长度等于 "2^order * 4KiB", 通过这个表达式就可以知道一个大页的长度以及占用的地址位宽，即 "PAGE_SHIFT + order". huge_page_shift() 函数用于获得指定大页占用的地址位宽，其逻辑就是通过大页对于的 struct hstate 的 order 成员加上 PAGE_SHIFT 即可。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02021">hugepage_migration_supported</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000962.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，其 order 成员表示大页长度占 4KiB 的阶数，及大页的长度为 "2^order * 4KiB", 那么可以知道大页占用的地址位宽为 "order + PAGE_SHIFT"，物理大页要支持迁移，那么物理大页占用的地址宽度必须是 PMD_SHIFT 或者 PGD_SHIFT. hugepage_migration_supported() 函数用于判断当前系统是否支持大页迁移功能，其判断逻辑为: 系统要支持大页迁移功能首先 CONFIG_ARCH_ENABLE_HUGEPAGE_MIGRATION 宏要打开，其次函数调用 huge_page_shift() 函数获得大页占用的地址位宽，如果位宽为 PMD_SHIFT 或者 PGD_SHIFT, 那么大页可以支持迁移功能。

> [huge_page_shift](#D02020)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02022">htlb_alloc_mask</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000963.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，大页的物理内存来自 buddy 内存分配器，其通过复合页构成物理大页。由于大页要从 buddy 分配器进行分配，那么分配大页内存时也要传递一定的分配参数给 buddy 分配器。htlb_alloc_mask() 函数就是用于获得大页从 buddy 分配器分配物理内存的分配标志，其逻辑为: 函数首先通过 hugepage_migration_support() 函数判断物理大页是否支持迁移，如果支持，那么内核提供 GFP_HIGHUSER_MOVABLE 标志; 反之物理大页不支持迁移，那么内核提供 GFP_HIGHUSER 标志。

> [hugepage_migration_supported](#D02021)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02023">alloc_buddy_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000964.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，大页通过从 buddy 分配器中分配内存组成复合页，该复合页作为大页的物理内存。在 struct hstate 数据结构中，其 order 成员用于描述大页是 4KiB 倍数的阶数。内核使用 alloc_buddy_huge_page() 函数为大页分配物理内存，其参数 h 指向指定长度大页对应的 struct hstate 数据结构，参数 gfp_mask 描述分配大页物理内存时的分配标志，参数 nid 指明分配内存的 NUMA NODE, 参数 nmask 用于描述 NUMA NODE 对于的 NODEMASK。alloc_buddy_huge_page() 函数分配逻辑为: 函数首先通过 huge_page_order() 函数获得大页的阶数，及大页是 4KiB 倍数的阶数，然后在 gfp_mask 的基础上添加 \_\_GFP_COMP、\_\_GFP_RETRY_MAYFAIL 和 \_\_GFP_NOWARN 标志，其中 \_\_GFP_COMP 表示物理页是复合页。函数接着通过 nid 参数判断当前的 NUMA NODE 数量，如果 nid 为 NUMA_NO_NODE， 即内核没有使用 NUMA, 那么函数调用 numa_mem_id() 函数获得存在内存的节点号并存储在 nid 中。函数接着向 \_\_alloc_pages_nodemask() 函数传入 gfp_mask、order、nid 和 nmask 参数，以此从 buddy 中分配出符合要求的物理内存，如果函数分配成功，那么函数将返回复合页的 Head Page，并调用 \_\_count_vm_event() 函数增加 HTLB_BUDDY_PGALLOC 的引用计数，以此告诉内核成功分配一个大页一次; 反之如果系统没有足够的内存分配大页，那么函数调用 \_\_count_vm_event() 函数增加 HTLB_BUDDY_PGALLOC_FAIL 的引用计数，以此告诉内存失败分配一个大页一次。函数最后返回 page 变量。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02024">prep_new_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000965.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，内核从 buddy 分配器中分配物理内存，并使用复合页的方式组成物理大页。prep_new_huge_page() 函数用于将从 buddy 分配器中分配的复合页进行相关的初始化，使复合页构成最终的物理大页，其逻辑为: 函数首先将复合页 Head Page 的 lru 链表进行初始化，然后调用 set_compound_page_dtor() 函数将复合页第一个尾页的 compound_dtor 设置为 HUGETLB_PAGE_DTOR, 通过这样设置大页的复合页就和普通的复合页区分开来。接着参数 h 指向指定长度大页的 struct hstate 数据结构，此时将 h 的 nr_huge_pages 加一，以此表示指定长度大页的数量增加 1 个，另外将物理页所在的 NUMA NODE 的 nr_huge_page_node 数量加一，以此表示在该 NUMA NODE 上指定长度的大页数量增加 1 个.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02025">alloc_pool_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000979.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并将空闲大页维护在 hstate 的双链表成员中。内核从 buddy 分配指定长度的复合页，并将复合页的析构函数设置为 HUGETLB_PAGE_DTOR, 以此构成物理大页，并与一般的复合页区分开来。alloc_pool_huge_page() 函数的作用就是从 buddy 中分配一个指定长度的复合页，然后将复合页的第一个尾页的 compound_dtor 设置为 HUGETLB_PAGE_DTOR，以此告诉内核该复合页是一个物理大页，最后将其加入到对应 struct hstate 的 hugepage_freelists 链表上，其具体逻辑为: 函数调用 htlb_alloc_mask() 函数获得从 buddy 分配器中分配大页的标志，并通过 \_\_GFP_THISNODE 标志告诉 buddy 分配器从当前 NUMA NODE 分配内存。接着函数调用 for_each_node_mask_to_alloc() 函数遍历 nodes_allowed 参数指定的 NUMA NODE, 在每个 NUMA NODE 上函数接着调用 alloc_fresh_huge_page() 函数从 buddy 中分配一个复合页，并将复合页的第一个尾页的 compound_dtor 设置为 HUGETLB_PAGE_DTOR，如果复合页分配成功，那么函数跳出循环，并在最后调用 put_page() 函数，此时由于新分配的物理大页引用计数为 0，那么将会触发复合页的析构函数，即 free_huge_page() 函数，该函数会将一个空闲的物理大页加入到其对应的 struct hstate 数据结构的 hugepage_freelists 链表上，最后函数返回 1; 反之如果没有从 alloc_fresh_huge_page() 函数中分配到物理大页，那么系统内存不足，此时函数返回 0.

> [htlb_alloc_mask](#D02022)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02026">hugetlb_hstate_alloc_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000980.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，内核从 buddy 分配器中分配指定长度的复合页，并将复合页的析构函数设置为 HUGETLB_PAGE_DTOR, 以此将大页使用的复合页与一般的复合页区分开来。空闲的大页会维护在 struct hstate 数据结构的 hugepage_freelists 链表上。hugetlb_hstate_alloc_pages() 函数的作用就是从 buddy 中分配复合页作为物理大页，并将其维护在指定的 struct hstate 数据结构里，其具体逻辑为: 函数首先从 struct hstate 数据结构的 max_huge_pages 成员获得当前指定长度的最大大页数量，然后使用 for 循环遍历 max_huge_pages 次，在每次遍历中，函数首先调用 hstate_is_gigantic() 函数判断大页是否为 1Gig 的大页，如果是则调用 alloc_bootmem_huge_page() 函数分配 1Gig 的大页; 反之调用 alloc_pool_huge_page() 分配指定长度的大页，分配过程中可能睡眠, 以上的代码逻辑确保可以分配 max_huge_pages 个物理大页. 当循环结束之后，发现分配的物理大页数量少于预期的 max_huge_pages 个时，函数会打印相应的提示信息，并将 struct hstate 数据结构的 max_huge_pages 设置为 i，以此表示还有 i 个大页待分配.

> [hstate_is_gigantic](#D2008)
>
> [alloc_bootmem_huge_page](#D02011)
>
> [alloc_pool_huge_page](#D02025)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02027">hugetlb_init_hstates</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000981.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并将所有的 struct hstate 数据结构维护在 hstates[] 数组中。函数 hugetlb_init_hstates() 的作用就是初始化内核所有的 struct hstate 数据结构，其底层逻辑是: 由于所有的 struct hstate 数据结构都维护在 hstates[] 数组中，那么函数首先调用 for_each_hstate() 函数遍历 hstates[] 数组中所有的 struct hstate 数据结构。在遍历每一个 struct hstate 数据结构时，函数调用 huge_page_order() 函数获得 struct hstate 数据结构维护指定长度的大页占 4KiB 的阶数 order (大页的长度等于 "2^order * 4KiB"), 另外 minimum_order 遍历用于维护系统支持大页长度最小的阶数，此时如果 minmum_order 大于当前大页的阶数，那么函数将 minimum_order 设置为当前大页的阶数。接着函数调用 hstate_is_gigantic() 函数判断大页是否为 1Gig 大页，如果不是，那么函数调用 hugetlb_hstate_alloc_pages() 函数为 struct hstate 数据结构分配大页; 反之则跳过不为 1Gig 大页分配实际的内存. 最后函数检查 minimum_order 是否为 UINT_MAX, 以此验证系统已经存在可用的大页了.

> [huge_page_order](D02009)
>
> [hugetlb_hstate_alloc_pages](#D02026)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02028">report_hugepages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000982.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护不同长度的大页，并将所有的 struct hstate 维护在 hstates[] 数组中。report_hugepages() 函数的作用就是打印所有可用大页的信息，其底层逻辑为: 函数首先调用 for_each_hstate() 函数遍历系统中可用的 struct hstate, 在每次遍历过程中，函数将 struct hstate 维护的大页长度和可用大页数量打印出来，在内核启动过程中，可以看到如下信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000983.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02029">hugetlb_sysfs_add_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000984.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并在 "/sys/kernel/mm" 目录下为大页创建了 "hugepages" 目录，该目录下包含了指定长度的大页的属性。hugetlb_sysfs_add_hstate() 函数的作用是为所有长度的大页在 "/sys/kernel/mm/hugepages" 目录下创建信息节点，其底层逻辑是: 参数 h 指向指定长度大页的 struct hstate 数据结构，参数 parent 指向了 "/sys/kernel/mm/hugepages" 目录的 kobject，参数 hstate_kobj 指向了指定长度大页 struct hstate 对于的 kobject，最后参数 hstate_attr_group 指向了 struct hstate 对应节点包含的属性。由于所有大页的 struct hstate 都维护在 hstates[] 数组里，因此函数首先调用 hstate_index() 函数获得指定大页的 struct hstate 在 hstates[] 数组中的偏移 hi，接着函数调用 kobject_create_and_add() 函数在 "/sys/kernel/mm/hugepages" 目录下创建大页 struct hstate 的 name 成员名字的 kobject，并将 kobject 存储在 hstate_kobjs[] 数组中，数组的偏移与 hstates[] 数组中的偏移一致。紧接着函数调用 sysfs_create_group() 函数为指定长度大页的 kobject 添加了 hstate_attr_group 属性组，该属性组包含了多个属性，例如下图，属性包含了 free_hugepages/nr_hugepages 等:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000985.png)

> [hstate_index](#D02012)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02030">hugetlb_sysfs_init</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000986.png)

在 Hugetlbfs 大页机制中, 内核使用 struct hstate 数据结构维护指定长度的大页，并在 "/sys/kernel/mm" 目录下为大页创建了 "hugepages" 目录，并为不同长度的大页创建节点并添加相应的属性。hugetlb_sysfs_init() 函数的作用就是为所有长度的大页在 "/sys/kernel/mm" 目录下创建 "hugepages" 目录，并为所有长度的大页创建节点和添加属性，其底层逻辑为: 函数首先调用 kobject_create_and_add() 函数在 mm_kobj 下创建 "hugepages" kobject, mm_kobj 对应 "/sys/kernel/mm" 目录，函数调用完毕之后会创建 "/sys/kernel/mm/hugepages" 目录，并使用 hugepages_kobj 维护 "hugepages" 目录的 kobject. 函数接着调用 for_each_hstate() 函数遍历所有大页的 struct hstate 数据结构，在每次遍历过程中，函数调用 hugetlb_sysfs_add_hstate() 函数为指定长度大页在 "/sys/kernel/mm/hugepages" 创建大页名字相同的目录，并在目录下添加 hstate_attr_group 属性组，例如:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000987.png)

> [hugetlb_sysfs_add_hstate](#D02029)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02031">hstate_sizelog</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000988.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护并管理指定长度的大页，并将所有的 struct hstate 维护在 hstates[] 数组中，内核可以通过大页的长度在 hstates[] 数组中反向查找到对应的 struct hstate 数据结构。hstate_sizelog() 函数的作用类似，其通过提供的 page_size_log 作为大页的长度反向查找对应的 struct hstate 数据结构，其底层逻辑是: 函数首先判断 page_size_log 参数是否为 0，如果为零，那么函数将返回默认长度的 struct hstate; 反之函数通过 size_to_hstate() 函数在 hstates[] 数组中进行查找。

> [size_to_hstate](#D02006)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02032">HUGETLBFS_SB</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000989.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述并管理指定长度的大页，并且为每种长度的大页挂载了一个虚拟文件系统 "hugetlbfs", 内核使用 struct hugetlbfs_sb_info 数据结构表示 "hugetlbfs" 文件系统相关的信息，其中 hstate 成员用于指向指定长度的大页。HUGETLBFS_SB() 函数的作用就是通过 VFS 的 super_block 数据结构找到 "hugetlbfs" 文件系统的描述信息，其底层逻辑是: 在 VFS 的 super_block 数据结构中，s_fs_info 成员指向了描述文件系统的数据结构，因此在 "hugetlbfs" 文件系统中正好指向了 struct hugetlbfs_sb_info 数据结构。在大页机制中，整个 hugetlbfs 文件系统存在如下逻辑, 因此可以通过 struct file, struct inode, struct super_block, struct hugetlb_sb_info 最终找到 struct hstate 数据结构:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02033">init_hugetlbfs_fs</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000990.png)

在 Hugetlbfs 大页机制中，内核使用 hugetlfs 文件系统作为支点，通过 page cache 的方式向用户空间提供大页物理内存。init_hugetlbfs_fs() 函数的作用向系统注册 hugetlbfs 文件系统，并为不同长度的大页创建 VFS 文件系统挂载点，以便用户空间通过指定的 hugetlbfs 文件系统挂载点进行大页内存的分配，函数的底层逻辑是: 函数通过 fs_initcall() 函数进行调用，该阶段是内存多种文件系统初始化阶段。函数首先在 394 行调用 hugepages_supported() 函数判断系统是否支持大页内存，如果不支持系统打印相关信息之后返回 ENOTSUPP; 反之系统支持大页，那么函数在 400 行调用 kmem_cache_create() 函数为 struct hugetlbfs_inode_info 数据结构创建换成，并使用 hugetlbfs_inode_cachep 遍历维护该换成。在 hugetlbfs 文件系统中，大页的 inode 使用 struct hugetlbfs_inode_info 进行描述。缓存创建成功之后，函数在 406 行向 register_filesystem() 函数传入 hugetlbfs_fs_type 以此向 VFS 注册 hugetlbfs 文件系统，hugetlbfs_fs_type 变量描述了 hugetlbfs 文件系统的信息. 在 hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并将所有的 struct hstate 数据结构维护在 hstates[] 数组中，此时函数使用 for_each_hstate() 函数遍历所有的 struct hstate 数据结构，在每次遍历过程中，函数首先在 413 行计算出大页长度占用 KiB 的数量，然后使用 snprintf() 函数将该长度构成 "pagesize=XK" 格式的字符串，以此表示大页按 KiB 的长度。函数接着在 416 行调用 kern_mount_data() 函数按 "pagesize=XK" 为参数挂载一个 hugetlbfs 类型的文件系统，并将挂载点存储在 hugetlbfs_vfsmount[] 数组中，这里可能涉及一些 VFS 的概念，一个文件系统注册到 VFS 之后可以有多个挂载点，每个挂载点是 hugetlbfs 类型的文件系统。如果此时挂载失败，那么函数打印报错信息之后，将 hugetlbfs_vfsmount[] 数组对应成员设置为 NULL. 函数最后在 428 行判断系统默认大页对应的 VFS 挂载点是否成功，如果挂载失败，那么函数将 hugetlbfs_inode_cache 缓存释放并返回错误; 反之直接返回 0.

> [hugepages_supported](#D02000)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02034">hstate_inode</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000992.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并为大页注册了 "hugetlbfs" 文件系统，内核还为每种长度的大页都建立的 hugetlbfs 文件系统的挂载点，虽然 "hugetlbfs" 文件系统属于虚文件系统，但其使大页机制具有了文件系统的一些属性。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugeltfs 文件系统中，内核使用 struct hugetlbfs_sb_info 描述了 hugetlbfs 文件系统与 super_block 相关的信息，其成员 hstate 指向了指定长度大页的描述信息 struct hstate. 另外根据文件系统的特性，可以通过上图的关系以此找到最终的 struct hstate 数据结构。hstate_inode() 函数的作用是通过 inode 节点找到对应大页的 struct hstate 数据结构，其底层逻辑是: 在大页机制中，当通过匿名或者文件方式在使用大页内存的时候，内核都会为这段大页内存分配一个 struct inode，以此在 hugetlbfs 文件系统中唯一标识这段大页内存，另外基于文件系统的特性，可以通过 struct inode 数据结构的 i_sb 成员找到 hugetlbfs 文件的 SB，进而找到了挂载指定长度大页的 struct hstate 数据结构.

> [HUGETLBFS_SB](#D02032)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02035">hstate_file</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000993.png)

在 Hugetlbfs 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并为大页注册了 hugetlbfs 文件系统，另外内核在初始化过程中，为每种长度的大页建立了 hugetlbfs 文件系统的挂载点，虽然 hugetlbfs 文件系统属于虚文件系统，但其使大页机制具有了一定的文件系统特性.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugetlbfs 文件系统中，内核使用 struct hugetlbfs_sb_info 描述 hugetlbfs 文件系统与 super_block 相关信息，其成员 hstate 指向了指定长度大页的 struct hstate 数据结构。另外根据文件系统的特性，可以通过上图的关系从不同的起点找到 struct hstate 数据结构。hstate_file() 函数的作用是通过打开的文件描述符 struct file 找到对应大页的 struct hstate 数据结构，其底层逻辑是: 在大页机制中，当通过匿名或者文件方式使用大页时，内核都会为这段大页内存分配一个 struct file, 以此在 hugetlbfs 文件中打开这段大页内存，另外基于文件系统的特性，可以通过 struct file 数据结构找到对应的 struct inode 唯一标识，struct inode 数据结构进而通过 i_sb 成员找到 hugetlbfs 文件系统的 super_block, 最终找到 struct hugetlbfs_sb_info 的 hstate 成员.

> [hstate_inode](#D02034)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)









> [https://blog.csdn.net/yk_wing4/article/details/88080442](https://blog.csdn.net/yk_wing4/article/details/88080442)
