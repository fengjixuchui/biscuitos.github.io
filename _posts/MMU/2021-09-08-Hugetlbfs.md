---
layout: post
title:  "Hugetlb and Hugetlbfs Mechanism"
date:   2021-09-08 06:00:00 +0800
categories: [HW]
excerpt: HugePage.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [Hugetlb/Hugetlbfs 原理](#A)
>
> - [Hugetlb/Hugetlbfs 使用](#B)
>
>   - hugetlbfs 文件系统使用攻略
>
>   - 共享匿名大页使用攻略
>
>   - 共享文件大页使用攻略
>
>   - 私有匿名大页使用攻略
>
>   - 私有文件大页使用攻略
>
>   - hugetlbfs subpool 池子使用攻略
>
>   - hugetlb 大页内存池子使用攻略
>
>   - libhugetlbfs 使用攻略
>
>   - hugetlb 大页超发机制使用攻略
>
> - [Hugetlb/Hugetlbfs 实践](#C)
>
>   - hugeltb/hugetlbfs on X86_64
>
>   - hugetlb/hugetlbfs on i386
>
>   - hugetlb/hugetlbfs on ARM
>
>   - hugetlb/hugetlbfs on ARM64
>
>   - hugetlb/hugetlbfs on RISCV-32
>
>   - hugetlb/hugetlbfs on RISCV-64
>
> - [Hugetlb/Hugetlbfs 源码分析](#D)
>
>   - hugetlb/hugetlbfs 数据结构
>
>   - hugetlb/hugetlbfs 核心数据
>
>   - hugetlb/hugetlbfs API
>
> - [Hugetlb/Hugetlbfs 调试](#E)
>
>   - Memory Mapping on hugetlb 调试攻略
>
>   - Page fault on hugetlb 调试攻略
>
> - [Hugeltb/Hugetlbfs 历史](#F)
>
>   - Hugetlb 历史时间轴
>
>   - hugetlb/hugetlbfs on Linux v2.2.26
>
> - Hugetlb/Hugetlbfs 进阶研究
>
>   - 硬件如何支持大页
>
>   - 通过 CMDLINE 配置大页长度和大页数量
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
>   - HugeTemporary 大页研究
>
>   - Hugetlb 与 Hugetlbfs 的关系
>
>   - Transparent-hugepage/Persistent-hugepage/Surplus-hugepage 的区别
>
>   - Hugetlb resv_map 研究
>
>   - 文件大页的体积无节制增加问题
>
>   - Hugetlbfs Spool 池子研究
>
>   - Hugetlbfs VFS inode operations 接口研究
>
>   - 文件大页和匿名大页的区别
>
>   - 大页标志集合研究
>
>   - 大页使用量统计
>
>   - 大页注册 MMU notifier
>
>   - 大页 SWAP 研究
>
>   - 大页 COW 深度研究
>
>   - 通过 hugetlb 大页为用户空间大规格连续物理内存
>
>   - 用户空间进程查看大页的物理地址
>
>   - 大页页表分析
>
>   - 大页相关的 MMU Notifier
>
>   - 大页与 interval tree/left-most RBtree/Argumented Cached RBtree
>
>   - 私有匿名大页
>
>   - 私有文件大页
>
> - [Hugetlb/Hugetlbfs BUG and Bugfix](#G)
>
> - [Hugetlb/Hugetlbfs Patch](#H)
>
> - [Hugeltb/Hugetlbfs 论坛讨论](#F)
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
>   - [struct file_region]()
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
>   - [struct resv_map]()
>
>   - [vma_resv_mode]()
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
>   - [hugetlbfs_fs_type]()
>
>   - [hugetlbfs_inode_cachep]()
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
> - [Hugetlbfs API](#D02131)
>
>   - [alloc_bootmem_huge_page](#D02011)
>
>   - [alloc_buddy_huge_page](#D02023)
>
>   - [alloc_buddy_huge_page_with_mpol](#D02113)
>
>   - [alloc_fresh_huge_page](#D02038)
>
>   - [alloc_huge_page](#D02114)
>
>   - [alloc_pool_huge_page](#D02025)
>
>   - [alloc_surplus_huge_page](#D02039)
>
>   - [blocks_per_huge_page](#D02119)
>
>   - [clear_huge_page](#D02116)
>
>   - [clear_page_huge_active](#D02018)
>
>   - [clear_subpage](#D02117)
>
>   - [copy_user_huge_page](#D02125)
>
>   - [dequeue_huge_page_node_exact](#D02111)
>
>   - [dequeue_huge_page_nodemask](#D02110)
>
>   - [dequeue_huge_page_vma](#D02112)
>
>   - [enqueue_huge_page](#D02019)
>
>   - [for_each_hstate](#D02004)
>
>   - [free_pool_huge_page](#D02042)
>
>   - [gather_surplus_pages](#D02040)
>
>   - [get_hstate_idx](#D02071)
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
>   - [hstate_vma](#D02080)
>
>   - [htlb_alloc_mask](#D02022)
>
>   - [HUGETLBFS_SB](#D02032)
>
>   - [huge_add_to_page_cache](#D02118)
>
>   - [huge_page_mask](#D02092)
>
>   - [huge_page_order](#D02009)
>
>   - [huge_page_shift](#D02020)
>
>   - [huge_page_size](#D02005)
>
>   - [huge_pte_clear](#D02100)
>
>   - [huge_pte_clear_flush](#D02128)
>
>   - [huge_pte_dirty](#D02097)
>
>   - [huge_pte_mkdirty](#D02099)
>
>   - [huge_pte_mkwrite](#D02098)
>
>   - [huge_pte_modify](#D02101)
>
>   - [huge_pte_none](#D02104)
>
>   - [huge_pte_offset](#D02086)
>
>   - [huge_pte_write](#D02096)
>
>   - [huge_pte_wrprotect](#D02094)
>
>   - [huge_ptep_get](#D02089)
>
>   - [huge_ptep_get_and_clear](#D02103)
>
>   - [huge_ptep_set_access_flags](#D02090)
>
>   - [huge_ptep_set_wrprotect](#D02091)
>
>   - [hugepage_add_new_anon_rmap](#D02120)
>
>   - [hugepage_migration_supported](#D02021)
>
>   - [hugepage_new_subpool](#D02045)
>
>   - [hugepage_subpool_get_pages](#D02072)
>
>   - [hugepage_subpool_put_pages](#D02073)
>
>   - [hugepages_supported](#D02000)
>
>   - [hugetlb_acct_memory](#D02044)
>
>   - [hugetlb_add_hstate](#D02007)
>
>   - [hugetlb_bad_size](#D02003)
>
>   - [hugetlb_count_add](#D02123)
>
>   - [hugetlb_count_sub](#D02124)
>
>   - [hugetlb_cow](#D02126)
> 
>   - [hugetlb_default_setup](#D02001)
>
>   - [hugetlb_fault](#D02129)
>
>   - [hugetlb_file_setup](#D02075)
>
>   - [hugetlb_hstate_alloc_pages](#D02026)
>
>   - [hugetlb_init_hstates](#D02027)
>
>   - [hugetlb_no_page](#D02130)
>
>   - [hugetlb_nrpages_setup](#D02010)
>
>   - [hugetlb_reserve_pages](#D02074)
>
>   - [hugetlb_sysfs_add_hstate](#D02029)
>
>   - [hugetlb_sysfs_init](#D02030)
>
>   - [hugetlb_vm_op_close](#D02081)
>
>   - [hugetlb_vm_op_fault](#D02076)
>
>   - [hugetlb_vm_op_open](#D02077)
>
>   - [hugetlb_vm_op_pagesize](#D02084)
>
>   - [hugetlbfs_alloc_inode](#D02048)
>
>   - [hugetlbfs_create](#D02056)
>
>   - [hugetlbfs_dec_free_inodes](#D02047)
>
>   - [hugetlbfs_destroy_inode](#D02049)
>
>   - [hugetlbfs_file_mmap](#D02085)
>
>   - [hugetlbfs_fill_super](#D02051)
>
>   - [hugetlbfs_get_inode](#D02054)
>
>   - [hugetlbfs_get_root](#D02052)
>
>   - [hugetlbfs_i_callback](#D02050)
>
>   - [hugetlbfs_inc_free_inodes](#D02046)
>
>   - [hugetlbfs_mknod](#D02055)
>
>   - [hugetlbfs_parse_options](#D02037)
>
>   - [hugetlbfs_size_to_hpages](#D02036)
>
>   - [inode_resv_map](#D02059)
>
>   - [init_hugetlbfs_fs](#D02033)
>
>   - [is_file_hugepages](#D02057)
>
>   - [is_vm_hugetlb_page](#D02064)
>
>   - [is_vma_resv_set](#D02068)
>
>   - [linear_hugepage_index](#D02079)
>
>   - [make_huge_pte](#D02121)
>
>   - [mk_huge_pte](#D02095)
>
>   - [PageHeadHuge](#D02016)
>
>   - [PageHuge](#D02013)
>
>   - [page_huge_active](#D02017)
>
>   - [page_hstate](#D02014)
>
>   - [pages_per_huge_page](#D02115)
>
>   - [pmd_huge](#D02088)
>
>   - [prep_new_huge_page](#D02024)
>
>   - [prepare_hugepage_range](#D02093)
>
>   - [pud_huge](#D02087)
>
>   - [region_abort](#D02063)
>
>   - [region_add](#D02061)
>
>   - [region_chg](#D02060)
>
>   - [region_count](#D02083)
>
>   - [region_del](#D02062)
>
>   - [report_hugepages](#D02028)
>
>   - [reset_vma_resv_huge_page](#D02069)
>
>   - [restore_reserve_on_error](#D02127)
>
>   - [resv_map_alloc](#D02053)
>
>   - [return_unsed_surplus_pages](#D02043)
>
>   - [set_huge_pte_at](#D02102)
>
>   - [set_huge_ptep_writable](#D02122)
>
>   - [set_page_huge_active](#D02015)
>
>   - [set_vma_resv_map](#D02065)
>
>   - [set_vma_resv_flags](#D02067)
>
>   - [setup_hugepagesz](#D02002)
>
>   - [size_to_hstate](#D02006)
>
>   - [subpool_inode](#D02058)
>
>   - [subpool_vma](#D02082)
>
>   - [update_and_free_page](#D02041)
>
>   - [vma_has_reserves](#D02070)
>
>   - [vma_hugecache_offset](#D02078)
>
>   - [vma_add_reservation](#D02109)
>
>   - [vma_commit_reservation](#D02107)
>
>   - [vma_end_reservation](#D02108)
>
>   - [vma_needs_reservation](#D02106)
>
>   - [\_\_vma_resevation_common](#D02105)
>
>   - [vma_resv_map](#D02066)

------------------------------------

###### <span id="D021XX"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH0011.png)

{% highlight c %}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02000">hugepages_supp5674657567orted</span>

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

在 Hugetlb 大页机制中，内核使用 struct hstates 数据结构描述某种长度的大页，并使用 hstates[] 数组将所有种类的大页统一维护在 hstates[] 数组中，hstates[] 数组的最大长度为 HUGE_MAX_HSTATE。在实际的内核中不一定都存在全部种类的大页，有的只可能是 2MiB 或者 1Gig 的大页，那么此时系统使用 hugetlb_max_hstate 描述当前系统最大可用大页种类数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02005">huge_page_size</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000945.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其成员 order 表示构成该大页所需 4KiB 页的阶数，例如 2MiB 的大页需要 4KiB 页的阶数为 9，即 2^9 个 4KiB 物理页构成一个 2MiB 的大页. huge_page_size() 函数就是通过 struct hstate 数据结构获得大页的长度，其逻辑是通过获得 struct hstate 的 order 成员，然后将 PAGE_SIZE 向左移动 order 位，即 (2^order * 4KiB).

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02006">size_to_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000946.png)

在 Hugetlb 大页机制中，每种指定长度的大页都是用 struct hstate 数据结构进行描述，并且将所有长度大页对应的 struct hstate 数据结构统一维护在 hstates[] 数组中。size_to_hstate() 函数调用 for_each_hstate() 函数遍历 hstates[] 数组中可用大页的 struct hstate，在每次遍历过程中，结合 struct hstate 数据结构，huge_page_size() 函数可以获得其所维护大页的长度，如果此时获得大页的长度正好等于参数 size，那么 size_to_hstate() 函数就找到大页长度与 size 参数一致的 struct hstate 数据结构. 如果 hstates[] 数组中维护的大页长度与 size 参数不匹配，那么 size_to_hstate() 函数将返回 NULL, 依次告诉调用者 size 参数对应长度的大页不存在.

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

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并将描述不同长度大页的 struct hstate 数据结构统一维护在 hstates[] 数组中，数组的默认长度就是 HUGE_MAX_HSTATE. 可以看到 HUGE_MAX_HSTATE 宏定义为 2。在 X86_64 架构中，内核支持的大页长度只有两种 2MiB 和 1Gig 的，因此该宏定义为 2.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02007">hugetlb_add_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000947.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并将描述不同长度大页的 struct hstate 数据结构统一维护在 hstates[] 数组中。当系统要新支持一种大页，那么内核就会调用 hugetlb_add_hstate() 函数进行添加，其逻辑如上图。order 参数表示新加入大页的阶数，即大页的长度为 "2^order * 4KiB", 接着函数首先在 18 行调用 size_to_hstate() 函数检查当前系统中是否已经存在同样长度的大页，如果存在函数就打印相应信息之后直接退出; 反之表明当前系统还没有指定长度的大页。hstates[] 数组的最大长度为 HUGE_MAX_HSTATE, 而当前系统中已经注册大页的数量使用 hugetlb_max_hstate 进行描述，如果此时函数在 22 行检查到 hugetlb_max_hstate 大于 HUGE_MAX_HSTATE, 那么 hstates[] 数组即将越界，那么函数通过 BUG_ON() 进行报错，另外如果检查到 order 参数为 0，那么 order 参数是非法参数，那么函数同样也通过 BUG_ON() 报警. 通过上面检查之后，函数从 hstates[] 数组中取出 hugetlb_max_hstate 对应最新可用的 struct hstate，之后将 hugetlb_max_hstate 加一操作指向 hstates[] 数组中下一个成员。函数在 25 行将 struct hstate 的 order 成员设置为 order 参数，以此表示大页的阶数，另外通过阶数算出大页的掩码，该掩码用于掩盖大页内部偏移。函数接着在 27 行将 nr_huge_pages 成员初始化为 0，以此表示当前指定长度的可用大页数量为 0，同理 free_huge_pages 成员设置为 0，也表示当前指定长度大页的空闲大页数量为 0. 在 struct hstate 数据结构中存在 hugepage_freelists[] 数组成员，数组由双链表构成，其用于维护不同 NUMA NODE 上的大页，那么函数在 29-30 行将 hugepage_freelists[] 数组内的各链表初始化。struct hstate 数据结构中存在 hugepage_activelist 成员，其由一个双链表构成，用于维护正在使用的大页，那么函数在 31 行将其 hugepage_activelist 链表进行初始化。struct hstate 数据结构的 next_nid_to_alloc 成员表示下一个可分配内存 NUMA NODE 节点，而 next_nid_to_free 成员表示下一个可以回收内存的 NUMA NODE 节点，函数在 32-33 行将两个成员都指向了 first_memory_node, first_memory_node 表示第一个存在内存的 NUMA NODE 节点。函数接着在 34 行调用 snprintf() 函数为大页设置一个名字，其名字规则是 "hugepages-XkB", 其中 X 为大页长度包含 KiB 的数量. 函数最后将 parsed_hstate 指向了新插入的 struct hstate 数据结构，以此表示内核正在解析的 struct hstate 大页.

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

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其中 order 成员描述大页长度需要 4KiB 物理页的阶数，因此可以通过 order 成员知道大页的实际长度。在 x86_64 架构中，内核支持 2MiB 和 1Gig 长度的大页，那么 hstate_is_gigantic() 函数的目的就是确认 struct hstate 数据结构描述的大页是 2MiB 大页还是 1Gig 大页。函数首先调用 huge_page_order() 函数获得 struct hstate 数据结构的 order 成员，然后判断 order 成员是否大于获得等于 MAX_ORDER, 由于 2MiB 的 order 为 9，其值小于 MAX_ORDER, 因此当 order 大于 MAX_ORDER 是就可以认为 struct hstate 描述 1Gig 的大页; 反之 struct hstate 描述 2MiB 的大页.

> [huge_page_order](D02009)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02009">huge_page_order</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000949.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其中 order 成员描述大页长度为 4KiB 物理页的阶数，即大页的长度为 "2^order * 4KiB". huge_page_order() 函数用于获得 struct hstate 的 order 成员.

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

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并将所有的 struct hstate 数据结构维护在 hstates[] 数组中。hstate_index() 函数用于获得参数 h 在 hstates[] 数组中的索引，其通过 h 与 hstates[] 数组相减即可.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D0305">minimum_order</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000953.png)

minimum_order 变量用于描述内核所支持不同长度的大页中，长度最小的大页对应的 order. 大页的长度通过 "2^order * 4KiB" 获得，因此 minimum_order 可以描述最小大页的 order 阶数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02013">PageHuge</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000954.png)

在 Hugetlb 大页机制中，内核使用复合页 (Compound Page) 来构建大页。在 Compound Page 构成的大页中，Head Page 标记为 PG_head, 其余的 Tail Page 标记为 PG_tail. 但在 32bit 系统中没有那么多标记，因此使用 PG_compound 标记所有的页，而使用 PG_reclaim 标记所有的 Tail Page。因此可以使用 PageCompound() 函数判断一个页是否为复合页。另外在构成大页的复合页中的第一个尾页 Tail Page，将其 compound_dtor 指向了 HUGETLB_PAGE_DTOR. 因此 PageHuge() 函数判断一个页是否为大页的逻辑是: 首先调用 PageCompound() 函数判断物理页是否为复合页，接着判断复合页的第一个尾页的 compound_dtor 是否为 HUGETLB_PAGE_DTOR, 如果两个条件满足，那么函数认为这个物理页是一个大页.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02014">page_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000955.png)

在 Hugetlb 大页机制中，使用 buddy 提供的复合页构成物理大页，大页所使用的复合页的第一个尾页与其他复合页有所区别，第一个尾页的 compound_dtor 指向了 HUGETLB_PAGE_DTOR, 因此可以通过此特点判断物理页是否为大页。page_hstate() 函数的作用是找到大页对应的 struct hstate 数据结构。函数首先调用 PageHuge() 函数判断物理页是否为大页，如果不是则通过 VM_BUG_ON_PAGE() 函数报错; 反之如果是则通过 compound_order() 函数获得物理页的阶数，然后将 PAGE_SIZE 左移物理页的阶数，以此获得物理页的长度，最后 size_to_hstate() 函数基于物理页的长度找到大页对应的 struct hstate 数据结构。

> [PageHuge](#D02013)
>
> [size_to_hstate](#D02006)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02015">set_page_huge_active</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000956.png)

在 Hugetlb 大页机制中，使用 buddy 提供的复合页构成物理大页，大页使用复合页中第一个尾页表示大页是否处于激活状态。如果一个大页处于激活状态，那么复合页中第一个尾页的 PG_Private 标志置位，激活的大页会被维护在其对应 struct hstate 的 hugepage_activelist 链表上，激活页主要用于区分正在使用 in-use 的大页. set_page_huge_active() 函数用于将一个大页标记为激活态的大页，其逻辑: 首先调用 PageHeadHuge() 函数判断物理页是否为大页的头页，如果不是则通过 VM_BUG_ON_PAGE() 进行报错; 反之如果物理页是大页的头页，那么函数调用 SetPagePrivate() 函数设置第一个尾页的 PG_Private 标志。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02016">PageHeadHuge</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000957.png)

在 Hugetlb 大页机制中，内核从 buddy 分配器中分配的复合页构成了物理大页。构成物理大页的复合页为了和其他复合页区分开来，内核会将构成物理大页的复合页第一个尾页的 compound_dtor 指向 HUGETLB_PAGE_DTOR. 另外在复合页中，HugeHeadPage 就是复合页的 Head Page, 而剩余的页都不是 HugeHeadPage, 那么要判断一个物理页是否为 HugeHeadPage 的逻辑为: 首先判断物理页是否为复合页的 Head Page，由之前的讨论可知 Head Page 包含了 PG_head 或者包含 PG_compound 但不包含 PG_reclaim 标志，可以直接使用 PageHead() 函数进行判断，但物理页不是 Head Page, 那么该页也不是物理大页的 HugeHeadPage; 反之物理页是 Head Page, 那么接下来其后一个物理页的 compound_dtor 是否为 HUGETLB_PAGE_DTOR，此时调用 get_compund_page_dtor() 函数直接获得第一个尾页的 compound_dtor 对应的函数，如果此函数是 free_huge_page()，那么函数认为这个物理页是 HugeHeadPage.

> [PageHuge](#D02013)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02017">page_huge_active</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000958.png)

在 Hugetlb 大页机制中，内核从 buddy 分配器中分配的复合页构成了物理大页，构成物理大页的复合页与一般的复合页通过第一个尾页的 compound_dtor 进行区分，如果复合页是大页，那么 compound_dtor 指向 HUGETLB_PAGE_DTOR. 内核可以使用 PageHuge() 函数判断一个物理页是否为大页。大页可以分为激活态和正在使用态，内核使用复合页的第一个尾页是否包含 PG_Private 来判断大页是否处于激活态，如果包含，那么物理大页处于激活态; 反之不处于激活态. page_huge_active() 函数就根据上面的逻辑来判断大页是否处于激活态，其具体逻辑如: 函数首先调用 PageHuge() 函数判断物理是否属于大页，如果不是则调用 VM_BUG_ON_PAGE() 函数进行报错; 反之物理页是大页，接着函数调用 PageHead() 函数判断物理页是否为复合页的 Head Page，且复合页的第一个尾页是否包含了 PG_private 标志. 两个条件都满足，那么大页处于激活态.

> [PageHuge](#D02013)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02018">clear_page_huge_active</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000959.png)

在 Hugetlb 大页机制中，内核使用 buddy 分配器中分配的复合页作为物理大页，作为大页的复合页与一般复合页的不同点在于: 大页的复合页的第一个尾页的 compound_dtor 指向了 HUGETLB_PAGE_DTOR. 另外大页可以能处于激活态和 in-use 态，那么大页的复合页使用第一个尾页如果包含 PG_private, 那么该大页处于激活态, 反之处于非激活态. clear_page_huge_active() 函数用于将大页设置为非激活态，其逻辑为: 首先函数调用 PageHeadHuge() 判断物理页是否为复合页的 HugeHeadPage 页，如果不是则调用 VM_BUG_ON_PAGE() 直接报错; 反之函数调用 ClearPagePrivate() 函数清除复合页第一个尾页的 PG_private 标志, 至此大页的激活态被清除.

> [PageHeadHuge](#D02016)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02019">enqueue_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000960.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 管理指定长度的大页，其成员 free_huge_pages 用于描述指定长度大页的数量，成员 free_huge_pages_node[] 数组用于描述指定 NUMA NODE 上大页的数量，成员 hugepage_freelists[] 双链表数组用于维护各 NUMA NODE 上可用的物理大页. enqueue_huge_page() 函数用于将一个空闲的大页加入到指定长度的 struct hstate 数据结构中，其插入逻辑为: 首先函数通过 page_to_nid() 函数获得物理页所在的 NUMA NODE, 接着函数通过物理页 page->lru 插入到 struct hstate hugepage_freelists[] 指定 NODE 的链表上，并将 struct hstate 的 free_huge_pages 和指定 NUMA NODE 的 free_huge_page_node[] 数组进行加一操作，至此一个物理大页被加入到指定长度的 strut hstate 数据结构中进行维护.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02020">huge_page_shift</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000961.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，其中 order 成员表示大页长度是 4KiB 的倍数阶数，即大页的长度等于 "2^order * 4KiB", 通过这个表达式就可以知道一个大页的长度以及占用的地址位宽，即 "PAGE_SHIFT + order". huge_page_shift() 函数用于获得指定大页占用的地址位宽，其逻辑就是通过大页对于的 struct hstate 的 order 成员加上 PAGE_SHIFT 即可。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02021">hugepage_migration_supported</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000962.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，其 order 成员表示大页长度占 4KiB 的阶数，及大页的长度为 "2^order * 4KiB", 那么可以知道大页占用的地址位宽为 "order + PAGE_SHIFT"，物理大页要支持迁移，那么物理大页占用的地址宽度必须是 PMD_SHIFT 或者 PGD_SHIFT. hugepage_migration_supported() 函数用于判断当前系统是否支持大页迁移功能，其判断逻辑为: 系统要支持大页迁移功能首先 CONFIG_ARCH_ENABLE_HUGEPAGE_MIGRATION 宏要打开，其次函数调用 huge_page_shift() 函数获得大页占用的地址位宽，如果位宽为 PMD_SHIFT 或者 PGD_SHIFT, 那么大页可以支持迁移功能。

> [huge_page_shift](#D02020)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02022">htlb_alloc_mask</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000963.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，大页的物理内存来自 buddy 内存分配器，其通过复合页构成物理大页。由于大页要从 buddy 分配器进行分配，那么分配大页内存时也要传递一定的分配参数给 buddy 分配器。htlb_alloc_mask() 函数就是用于获得大页从 buddy 分配器分配物理内存的分配标志，其逻辑为: 函数首先通过 hugepage_migration_support() 函数判断物理大页是否支持迁移，如果支持，那么内核提供 GFP_HIGHUSER_MOVABLE 标志; 反之物理大页不支持迁移，那么内核提供 GFP_HIGHUSER 标志。

> [hugepage_migration_supported](#D02021)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02023">alloc_buddy_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000964.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，大页通过从 buddy 分配器中分配内存组成复合页，该复合页作为大页的物理内存。在 struct hstate 数据结构中，其 order 成员用于描述大页是 4KiB 倍数的阶数。内核使用 alloc_buddy_huge_page() 函数为大页分配物理内存，其参数 h 指向指定长度大页对应的 struct hstate 数据结构，参数 gfp_mask 描述分配大页物理内存时的分配标志，参数 nid 指明分配内存的 NUMA NODE, 参数 nmask 用于描述 NUMA NODE 对于的 NODEMASK。alloc_buddy_huge_page() 函数分配逻辑为: 函数首先通过 huge_page_order() 函数获得大页的阶数，及大页是 4KiB 倍数的阶数，然后在 gfp_mask 的基础上添加 \_\_GFP_COMP、\_\_GFP_RETRY_MAYFAIL 和 \_\_GFP_NOWARN 标志，其中 \_\_GFP_COMP 表示物理页是复合页。函数接着通过 nid 参数判断当前的 NUMA NODE 数量，如果 nid 为 NUMA_NO_NODE， 即内核没有使用 NUMA, 那么函数调用 numa_mem_id() 函数获得存在内存的节点号并存储在 nid 中。函数接着向 \_\_alloc_pages_nodemask() 函数传入 gfp_mask、order、nid 和 nmask 参数，以此从 buddy 中分配出符合要求的物理内存，如果函数分配成功，那么函数将返回复合页的 Head Page，并调用 \_\_count_vm_event() 函数增加 HTLB_BUDDY_PGALLOC 的引用计数，以此告诉内核成功分配一个大页一次; 反之如果系统没有足够的内存分配大页，那么函数调用 \_\_count_vm_event() 函数增加 HTLB_BUDDY_PGALLOC_FAIL 的引用计数，以此告诉内存失败分配一个大页一次。函数最后返回 page 变量。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02024">prep_new_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000965.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，内核从 buddy 分配器中分配物理内存，并使用复合页的方式组成物理大页。prep_new_huge_page() 函数用于将从 buddy 分配器中分配的复合页进行相关的初始化，使复合页构成最终的物理大页，其逻辑为: 函数首先将复合页 Head Page 的 lru 链表进行初始化，然后调用 set_compound_page_dtor() 函数将复合页第一个尾页的 compound_dtor 设置为 HUGETLB_PAGE_DTOR, 通过这样设置大页的复合页就和普通的复合页区分开来。接着参数 h 指向指定长度大页的 struct hstate 数据结构，此时将 h 的 nr_huge_pages 加一，以此表示指定长度大页的数量增加 1 个，另外将物理页所在的 NUMA NODE 的 nr_huge_page_node 数量加一，以此表示在该 NUMA NODE 上指定长度的大页数量增加 1 个.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02025">alloc_pool_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000979.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并将空闲大页维护在 hstate 的双链表成员中。内核从 buddy 分配指定长度的复合页，并将复合页的析构函数设置为 HUGETLB_PAGE_DTOR, 以此构成物理大页，并与一般的复合页区分开来。alloc_pool_huge_page() 函数的作用就是从 buddy 中分配一个指定长度的复合页，然后将复合页的第一个尾页的 compound_dtor 设置为 HUGETLB_PAGE_DTOR，以此告诉内核该复合页是一个物理大页，最后将其加入到对应 struct hstate 的 hugepage_freelists 链表上，其具体逻辑为: 函数调用 htlb_alloc_mask() 函数获得从 buddy 分配器中分配大页的标志，并通过 \_\_GFP_THISNODE 标志告诉 buddy 分配器从当前 NUMA NODE 分配内存。接着函数调用 for_each_node_mask_to_alloc() 函数遍历 nodes_allowed 参数指定的 NUMA NODE, 在每个 NUMA NODE 上函数接着调用 alloc_fresh_huge_page() 函数从 buddy 中分配一个复合页，并将复合页的第一个尾页的 compound_dtor 设置为 HUGETLB_PAGE_DTOR，如果复合页分配成功，那么函数跳出循环，并在最后调用 put_page() 函数，此时由于新分配的物理大页引用计数为 0，那么将会触发复合页的析构函数，即 free_huge_page() 函数，该函数会将一个空闲的物理大页加入到其对应的 struct hstate 数据结构的 hugepage_freelists 链表上，最后函数返回 1; 反之如果没有从 alloc_fresh_huge_page() 函数中分配到物理大页，那么系统内存不足，此时函数返回 0.

> [htlb_alloc_mask](#D02022)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02026">hugetlb_hstate_alloc_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000980.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，内核从 buddy 分配器中分配指定长度的复合页，并将复合页的析构函数设置为 HUGETLB_PAGE_DTOR, 以此将大页使用的复合页与一般的复合页区分开来。空闲的大页会维护在 struct hstate 数据结构的 hugepage_freelists 链表上。hugetlb_hstate_alloc_pages() 函数的作用就是从 buddy 中分配复合页作为物理大页，并将其维护在指定的 struct hstate 数据结构里，其具体逻辑为: 函数首先从 struct hstate 数据结构的 max_huge_pages 成员获得当前指定长度的最大大页数量，然后使用 for 循环遍历 max_huge_pages 次，在每次遍历中，函数首先调用 hstate_is_gigantic() 函数判断大页是否为 1Gig 的大页，如果是则调用 alloc_bootmem_huge_page() 函数分配 1Gig 的大页; 反之调用 alloc_pool_huge_page() 分配指定长度的大页，分配过程中可能睡眠, 以上的代码逻辑确保可以分配 max_huge_pages 个物理大页. 当循环结束之后，发现分配的物理大页数量少于预期的 max_huge_pages 个时，函数会打印相应的提示信息，并将 struct hstate 数据结构的 max_huge_pages 设置为 i，以此表示还有 i 个大页待分配.

> [hstate_is_gigantic](#D2008)
>
> [alloc_bootmem_huge_page](#D02011)
>
> [alloc_pool_huge_page](#D02025)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02027">hugetlb_init_hstates</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000981.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并将所有的 struct hstate 数据结构维护在 hstates[] 数组中。函数 hugetlb_init_hstates() 的作用就是初始化内核所有的 struct hstate 数据结构，其底层逻辑是: 由于所有的 struct hstate 数据结构都维护在 hstates[] 数组中，那么函数首先调用 for_each_hstate() 函数遍历 hstates[] 数组中所有的 struct hstate 数据结构。在遍历每一个 struct hstate 数据结构时，函数调用 huge_page_order() 函数获得 struct hstate 数据结构维护指定长度的大页占 4KiB 的阶数 order (大页的长度等于 "2^order * 4KiB"), 另外 minimum_order 遍历用于维护系统支持大页长度最小的阶数，此时如果 minmum_order 大于当前大页的阶数，那么函数将 minimum_order 设置为当前大页的阶数。接着函数调用 hstate_is_gigantic() 函数判断大页是否为 1Gig 大页，如果不是，那么函数调用 hugetlb_hstate_alloc_pages() 函数为 struct hstate 数据结构分配大页; 反之则跳过不为 1Gig 大页分配实际的内存. 最后函数检查 minimum_order 是否为 UINT_MAX, 以此验证系统已经存在可用的大页了.

> [huge_page_order](D02009)
>
> [hugetlb_hstate_alloc_pages](#D02026)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02028">report_hugepages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000982.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护不同长度的大页，并将所有的 struct hstate 维护在 hstates[] 数组中。report_hugepages() 函数的作用就是打印所有可用大页的信息，其底层逻辑为: 函数首先调用 for_each_hstate() 函数遍历系统中可用的 struct hstate, 在每次遍历过程中，函数将 struct hstate 维护的大页长度和可用大页数量打印出来，在内核启动过程中，可以看到如下信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000983.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02029">hugetlb_sysfs_add_hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000984.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并在 "/sys/kernel/mm" 目录下为大页创建了 "hugepages" 目录，该目录下包含了指定长度的大页的属性。hugetlb_sysfs_add_hstate() 函数的作用是为所有长度的大页在 "/sys/kernel/mm/hugepages" 目录下创建信息节点，其底层逻辑是: 参数 h 指向指定长度大页的 struct hstate 数据结构，参数 parent 指向了 "/sys/kernel/mm/hugepages" 目录的 kobject，参数 hstate_kobj 指向了指定长度大页 struct hstate 对于的 kobject，最后参数 hstate_attr_group 指向了 struct hstate 对应节点包含的属性。由于所有大页的 struct hstate 都维护在 hstates[] 数组里，因此函数首先调用 hstate_index() 函数获得指定大页的 struct hstate 在 hstates[] 数组中的偏移 hi，接着函数调用 kobject_create_and_add() 函数在 "/sys/kernel/mm/hugepages" 目录下创建大页 struct hstate 的 name 成员名字的 kobject，并将 kobject 存储在 hstate_kobjs[] 数组中，数组的偏移与 hstates[] 数组中的偏移一致。紧接着函数调用 sysfs_create_group() 函数为指定长度大页的 kobject 添加了 hstate_attr_group 属性组，该属性组包含了多个属性，例如下图，属性包含了 free_hugepages/nr_hugepages 等:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000985.png)

> [hstate_index](#D02012)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02030">hugetlb_sysfs_init</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000986.png)

在 Hugetlb 大页机制中, 内核使用 struct hstate 数据结构维护指定长度的大页，并在 "/sys/kernel/mm" 目录下为大页创建了 "hugepages" 目录，并为不同长度的大页创建节点并添加相应的属性。hugetlb_sysfs_init() 函数的作用就是为所有长度的大页在 "/sys/kernel/mm" 目录下创建 "hugepages" 目录，并为所有长度的大页创建节点和添加属性，其底层逻辑为: 函数首先调用 kobject_create_and_add() 函数在 mm_kobj 下创建 "hugepages" kobject, mm_kobj 对应 "/sys/kernel/mm" 目录，函数调用完毕之后会创建 "/sys/kernel/mm/hugepages" 目录，并使用 hugepages_kobj 维护 "hugepages" 目录的 kobject. 函数接着调用 for_each_hstate() 函数遍历所有大页的 struct hstate 数据结构，在每次遍历过程中，函数调用 hugetlb_sysfs_add_hstate() 函数为指定长度大页在 "/sys/kernel/mm/hugepages" 创建大页名字相同的目录，并在目录下添加 hstate_attr_group 属性组，例如:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000987.png)

> [hugetlb_sysfs_add_hstate](#D02029)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02031">hstate_sizelog</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000988.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护并管理指定长度的大页，并将所有的 struct hstate 维护在 hstates[] 数组中，内核可以通过大页的长度在 hstates[] 数组中反向查找到对应的 struct hstate 数据结构。hstate_sizelog() 函数的作用类似，其通过提供的 page_size_log 作为大页的长度反向查找对应的 struct hstate 数据结构，其底层逻辑是: 函数首先判断 page_size_log 参数是否为 0，如果为零，那么函数将返回默认长度的 struct hstate; 反之函数通过 size_to_hstate() 函数在 hstates[] 数组中进行查找。

> [size_to_hstate](#D02006)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02032">HUGETLBFS_SB</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000989.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述并管理指定长度的大页，并且为每种长度的大页挂载了一个虚拟文件系统 "hugetlbfs", 内核使用 struct hugetlbfs_sb_info 数据结构表示 "hugetlbfs" 文件系统相关的信息，其中 hstate 成员用于指向指定长度的大页。HUGETLBFS_SB() 函数的作用就是通过 VFS 的 super_block 数据结构找到 "hugetlbfs" 文件系统的描述信息，其底层逻辑是: 在 VFS 的 super_block 数据结构中，s_fs_info 成员指向了描述文件系统的数据结构，因此在 "hugetlbfs" 文件系统中正好指向了 struct hugetlbfs_sb_info 数据结构。在大页机制中，整个 hugetlbfs 文件系统存在如下逻辑, 因此可以通过 struct file, struct inode, struct super_block, struct hugetlb_sb_info 最终找到 struct hstate 数据结构:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02033">init_hugetlbfs_fs</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000990.png)

在 Hugetlb 大页机制中，内核使用 hugetlfs 文件系统作为支点，通过 page cache 的方式向用户空间提供大页物理内存。init_hugetlbfs_fs() 函数的作用向系统注册 hugetlbfs 文件系统，并为不同长度的大页创建 VFS 文件系统挂载点，以便用户空间通过指定的 hugetlbfs 文件系统挂载点进行大页内存的分配，函数的底层逻辑是: 函数通过 fs_initcall() 函数进行调用，该阶段是内存多种文件系统初始化阶段。函数首先在 394 行调用 hugepages_supported() 函数判断系统是否支持大页内存，如果不支持系统打印相关信息之后返回 ENOTSUPP; 反之系统支持大页，那么函数在 400 行调用 kmem_cache_create() 函数为 struct hugetlbfs_inode_info 数据结构创建换成，并使用 hugetlbfs_inode_cachep 遍历维护该换成。在 hugetlbfs 文件系统中，大页的 inode 使用 struct hugetlbfs_inode_info 进行描述。缓存创建成功之后，函数在 406 行向 register_filesystem() 函数传入 hugetlbfs_fs_type 以此向 VFS 注册 hugetlbfs 文件系统，hugetlbfs_fs_type 变量描述了 hugetlbfs 文件系统的信息. 在 hugetlbfs 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，并将所有的 struct hstate 数据结构维护在 hstates[] 数组中，此时函数使用 for_each_hstate() 函数遍历所有的 struct hstate 数据结构，在每次遍历过程中，函数首先在 413 行计算出大页长度占用 KiB 的数量，然后使用 snprintf() 函数将该长度构成 "pagesize=XK" 格式的字符串，以此表示大页按 KiB 的长度。函数接着在 416 行调用 kern_mount_data() 函数按 "pagesize=XK" 为参数挂载一个 hugetlbfs 类型的文件系统，并将挂载点存储在 hugetlbfs_vfsmount[] 数组中，这里可能涉及一些 VFS 的概念，一个文件系统注册到 VFS 之后可以有多个挂载点，每个挂载点是 hugetlbfs 类型的文件系统。如果此时挂载失败，那么函数打印报错信息之后，将 hugetlbfs_vfsmount[] 数组对应成员设置为 NULL. 函数最后在 428 行判断系统默认大页对应的 VFS 挂载点是否成功，如果挂载失败，那么函数将 hugetlbfs_inode_cache 缓存释放并返回错误; 反之直接返回 0.

> [hugepages_supported](#D02000)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02034">hstate_inode</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000992.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并为大页注册了 "hugetlbfs" 文件系统，内核还为每种长度的大页都建立的 hugetlbfs 文件系统的挂载点，虽然 "hugetlbfs" 文件系统属于虚文件系统，但其使大页机制具有了文件系统的一些属性。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugeltfs 文件系统中，内核使用 struct hugetlbfs_sb_info 描述了 hugetlbfs 文件系统与 super_block 相关的信息，其成员 hstate 指向了指定长度大页的描述信息 struct hstate. 另外根据文件系统的特性，可以通过上图的关系以此找到最终的 struct hstate 数据结构。hstate_inode() 函数的作用是通过 inode 节点找到对应大页的 struct hstate 数据结构，其底层逻辑是: 在大页机制中，当通过匿名或者文件方式在使用大页内存的时候，内核都会为这段大页内存分配一个 struct inode，以此在 hugetlbfs 文件系统中唯一标识这段大页内存，另外基于文件系统的特性，可以通过 struct inode 数据结构的 i_sb 成员找到 hugetlbfs 文件的 SB，进而找到了挂载指定长度大页的 struct hstate 数据结构.

> [HUGETLBFS_SB](#D02032)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02035">hstate_file</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000993.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，并为大页注册了 hugetlbfs 文件系统，另外内核在初始化过程中，为每种长度的大页建立了 hugetlbfs 文件系统的挂载点，虽然 hugetlbfs 文件系统属于虚文件系统，但其使大页机制具有了一定的文件系统特性.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugetlbfs 文件系统中，内核使用 struct hugetlbfs_sb_info 描述 hugetlbfs 文件系统与 super_block 相关信息，其成员 hstate 指向了指定长度大页的 struct hstate 数据结构。另外根据文件系统的特性，可以通过上图的关系从不同的起点找到 struct hstate 数据结构。hstate_file() 函数的作用是通过打开的文件描述符 struct file 找到对应大页的 struct hstate 数据结构，其底层逻辑是: 在大页机制中，当通过匿名或者文件方式使用大页时，内核都会为这段大页内存分配一个 struct file, 以此在 hugetlbfs 文件中打开这段大页内存，另外基于文件系统的特性，可以通过 struct file 数据结构找到对应的 struct inode 唯一标识，struct inode 数据结构进而通过 i_sb 成员找到 hugetlbfs 文件系统的 super_block, 最终找到 struct hugetlbfs_sb_info 的 hstate 成员.

> [hstate_inode](#D02034)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02036">hugetlbfs_size_to_hpage</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000994.png)

在 Hugetlb 大页机制中，内核基于 Hugetlbfs 文件系统为用户空间提供物理大页，其可通过文件的方式，也可以通过匿名的方式获得大页。当使用文件方式获得大页时，需要额外挂载一个 Hugetlbfs 文件系统，并通过在挂载目录下创建文件的方式获得大页内存。Hugetlbfs 文件系统挂载时提供了多个配置参数，用于提供灵活的大页分配需求。hugetblfs 文件系统挂载时提供了 "size=" 字段用于指明挂载的文件系统包含大页的数量信息，其值可以是带 "K/M/G" 的值，也可以是一个百分比，hugetlbfs_size_to_hpages() 函数的作用就是将 "size=" 字段的值统一转换成具体的长度值，其底层逻辑为: 参数 size_opt "size=" 字段的值，val_type 则表明 "size=" 字段的类型，目前支持三种类型，分别是: NO_SIZE 表示 "size=" 字段没有值、SIZE_STD 表示 "size=" 字段是一个正常值、SIZE_PERCENT 表示 "size=" 字段是一个百分比。当函数知道 val_type 的值为 NO_SIZE, 那么 "size=" 字段没有值，函数直接返回 -1; 反之函数继续检查 val_type 的值是否为 SIZE_PERCENT, 如果是，那么函数将按如下规则进行计算:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000995.png)

这个规则也好理解，就是先计算出指定 hugetlb 最大大页数量，并乘与大页的长度，因此可以算出指定 hugetlb 大页的总长度，接着乘与百分比就可以获得 size_opt 对应的大页长度。获得所需大页长度之后，函数将该大页长度右移 huge_page_shift(h)，以此获得大页的数量，最后返回大页的数量.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02037">hugetlbfs_parse_options</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000996.png)

在 Hugetlb 大页机制中，内核提供了 hugeltbfs 文件系统为用户空间提供物理大页，其可通过文件方式或匿名方式获得大页。当使用文件方式获得大页时，需要显示挂载一个 hugetlbfs 的文件系统，并在该挂载点下创建文件来获得所需的大页。hugetlbfs 文件系统在挂载时提供了多个配置参数，用于灵活的配置所需的大页，其提供的字段如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000997.png)

"size=" 字段用于配置 hugetlbfs 文件系统包含大页的长度、"nr_inodes=" 字段用于限制 hugetlbfs 文件系统中最大 inode 数量、"mode=" 字段用于配置文件系统的访问模式、"uid=" 和 "gid=" 字段用于配置访问组信息、"pagesize=" 字段用于配置大页的长度、"min_size=" 字段用于配置文件系统中最少包含大页的数量。hugetlbfs_parse_options() 函数的作用就是解析这些字段，其底层逻辑为: options 参数包含了所有的字段信息，函数使用 while() 循环和 strsep() 函数将每个字段解析出来，在解析每个字段的时候，其逻辑分别为:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000998.png)

Opt_uid 用于解析 "uid=" 字段，而 Opt_gid 用于解析 "gid=" 字段，两个字段的解析比较相似，都是通过 make_kuid() 函数和 make_kgid() 函数将其值存储到 struct hugetlbfs_config 的 uid 和 gid 成员里。Opt_mode 用于解析 "mode=" 字段，函数将解析出来的值存储在 struct hugetlbfs_config 的 mode 成员里.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001000.png)

Opt_size 用于解析 "size=" 字段，该字段的解析比较复杂，由于 "size=" 可支持直接的数值，也可以支持百分比，那么函数首先使用 memparse() 函数将 "K/M/G" 的字符串转换成具体的数值，并将 max_val_type 设置为 SIZE_STD 表示正常的数值，如果此时解析到 rest 中包含了 "%", 那么表示 "size=" 字段是包含百分比的信息，那么此时函数将 max_val_type 设置为 SIZE_PERCENT. Opt_nr_inodes 用于解析 "nr_inodes" 字段，以此获得 hugetlbfs 支持的最大 inode 数量，并将该值存储到 struct hugetlbfs_config 的 nr_inodes 成员里。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001001.png)

Opt_pagesize 用于解析 "pagesize=" 字段，以此获得 hugetlbfs 大页的长度，函数首先通过 memparse() 函数获得字段的值，然后通过 size_to_hstate() 函数获得对应大页的 struct hstate 数据结构，并存储在 struct hugetlbfs_config 的 hstate 成员里。如果没有找到指定 hstate, 那么函数将报错。Opt_min_size 用于解析 "min_size=" 字段，函数通过 memparse() 函数获得字段的值，并将 min_val_type 设置 SIZE_STD, 如果此时发现字段中包含 "%", 说明该字段包含了一个百分比，那么将 min_val_type 设置为 SIZE_PERCENT.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001002.png)

在解析完参数之后，由于 "size="、"min_size=" 字段同时支持具体长度和百分比，因此函数调用 hugetlbfs_size_to_hpages() 函数将两个字段转换大页数量，并将 "size=" 字段转换的值存储在 struct hugetlbfs_config 的 max_hpages 成员里，同理将 "min_size=" 字段转换的值存储在 struct hugetlbfs_config 的 min_hpages 成员里。函数最后检查如果 "size=" 字段中存在值，但 "size=" 的值小于 "min_size=" 的值，那么函数认为这是一种错误的配置，函数将报错并返回 "-EINVAL". 反之函数将返回 0.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02038">alloc_fresh_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001003.png)

在 Hugetlb 大页支持中，同时支持 2MiB 和 1Gig 粒度的大页，大页的物理内存来自 Buddy 内存分配器分配的复合页，从 buddy 中分配的大页是特殊复合页构成的大页，其复合页的析构函数为 HUGETLB_PAGE_DTOR。alloc_fresh_huge_page() 函数的作用就是从 Buddy 分配器中分配物理大页，其底层逻辑是: 函数首先调用 hstate_is_gigantic() 函数判断大页是 1Gig 的大页还是 2MiB 的大页，如果是 1Gig 的大页，函数调用 alloc_gigantic_page() 函数分配 1Gig 的物理内存; 反之如果 2MiB 的大页，函数调用 alloc_buddy_huge_page() 函数分配 2MiB 的物理内存。如果物理内存分配失败，函数直接返回 NULL; 反之分配成功，此时大页是 1Gig 的大页，那么函数调用 prep_compound_gigantic_page() 函数初始化 1Gig 的大页。函数最后调用 prep_new_huge_page() 函数初始化大页。

> [hstate_is_gigantic](#D2008)
>
> [alloc_buddy_huge_page](#D02023)
>
> [prep_new_huge_page](#D02024)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02039">alloc_surplus_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001004.png)

在 Hugetlb 大页机制中，系统可以提前分配指定数量的大页用于 hugetlbfs 或者匿名大页使用，系统同时也支持超发大页，所谓超发大页就是系统设置一个上限，当系统空闲大页没有的情况下，系统可以动态分配一定数量的大页，只要动态分配的大页数量不超过这个上限就没有问题。alloc_surplus_huge_page() 函数的作用就是动态的分配超发的大页，其底层逻辑是: 函数首先调用 hstate_is_gigantic() 函数判断大页是否 1Gig 大页，如果是那么函数不支持动态分配 1Gig 大页，函数直接返回 NULL; 反之函数继续动态分配 2MiB 的大页。参数 h 指向维护 2MiB 大页的 struct hstate 数据结构，其 surplus_huge_pages 成员用于表示已经超发分配的大页数量，而 nr_overcommit_huge_pages 成员则表示可以超发大页的数量。函数如果此时发现 surplus_huge_pages 的数量大于 nr_overcommit_huge_pages, 即系统当前超发大页的数量已经大于当前系统支持的最大超发大页数量，那么函数跳转到 out_unlock; 反之当前系统可以继续超发大页，那么函数调用 alloc_fresh_huge_page() 函数分配大页内存，如果分配失败，函数直接返回 NULL. 当大页物理内存分配成功之后，函数此时检查当前已经超发的大页是否已经超过系统运行最大超发数，如果系统动态的缩减超发页的数量，就会导致超发大页数量小于系统支持的最大超发数量，那么函数将大页设置为 HugeTemporary (临时大页)，并将大页释放; 反之函数增加 surplus_huge_pages 超发大页的数量, 以及指定 NUMA NODE 上 surplus_huge_pages 的数量.

> [alloc_fresh_huge_page](#D02038)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02040">gather_surplus_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001005.png)

在 Hugetlb 大页机制中，系统可以提前分配指定数量的大页用于 hugetlbfs 或者匿名大页使用，系统同时也支持超发大页，所谓超发大页就是系统设置一个超发的上限，当系统可用大页没有的时候，可以动态的从 Buddy 中分配内存充当大页，只要分配大页的数量不超过系统允许超发的上限就行，这种方式称为大页的超发。gather_surplus_pages() 函数的作用就是检查是否需要超发大页，如果需要超发，那么函数会实际的分配超发的大页，其底层逻辑为: 参数 h 表示维护指定长度大页的 struct hstate 数据结构，其成员 resv_huge_pages 表示系统预留大页的数量，free_huge_pages 成员则表示系统可用大页的数量，函数首先检查预留的大页数量加上需要分配的大页数量不小于可用大页数量，那么函数认为系统由足够的大页供系统分配，那么函数将 resv_huge_pages 的数量加上需要分配的数量，然后返回 0; 反之表示当前系统没有足够的内存，此时函数将通过超发大页的方式分配大页.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001006.png)

函数首先将 allocated 设置为 0，然后初始化链表 surplus_list，needed 此时表示需要分配的大页数量，函数使用 for 循环，循环 needed 次，在每次循环中，函数调用 alloc_surplus_huge_page() 函数动态分配大页，如果分配成功则将大页插入到 surplus_list 链表上，此时可能会遇到调度。函数将分配的大页数量加到 allocated 变量中。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001007.png)

由于之前可能被调度，那么函数继续检查此时预留的大页加上需要分配的大页数量是否小于可用大页加上已经分配大页的数量，如果此时还不满足，那么函数跳转到 retry 处继续分配大页; 反之已经由足够的大页，那么函数继续往下走.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001008.png)

函数将刚刚分配的大页数量增加到 needed 变量里，并将 resv_huge_pages 数量增加 delta, 以此为新分配的大页做预留。函数接着遍历 surplus_list 链表上刚刚分配的大页，调用 enqueue_huge_page() 函数将大页添加到 hstate 维护的 freelist 链表上。最后函数再是遍历 surplus_list 链表，将多余的物理大页释放会 Buddy 分配器。

> [alloc_surplus_huge_page](#D02039)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02041">update_and_free_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001009.png)

在 Hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，内核从 Buddy 分配器中分配复合页作为大页，该复合页与一般的复合页不同，其析构函数为 HUGETLB_PAGE_DTOR。大页都维护在 struct hstate 的 hugepage_freelists 等链表上。update_and_free_page() 函数的作用是将一个大页释放回 Buddy 分配器中，其底层逻辑为: 参数 h 维护指定长度的大页，参数 page 则为需要释放的大页。函数首先检查在大页为 1Gig 大页的情况下系统是否支持 1Gig 大页，如果此时不支持，那么逻辑上存在错误，函数直接返回。函数首先将 h 的 nr_huge_pages 大页总数减一，函数接着将大页所有的 NUMA NODE 上的 nr_huge_pages_node 的值也减一。接着函数调用 for 循环将遍历大页复合页上所有的小页，将 PG_locked、PG_error、PG_referenced、PG_dirty、PG_active、PG_private、PG_writeback 标志从页的标志中移除。函数接着将复合页的析构函数由 HUGETLB_PAGE_DTOR 变更为 NULL_COMPOUND_DTOR，这样页按正常的复合页进行回收，接着函数将 page 的引用计数设置为 1. 函数接着调用 hstate_is_gigantic() 函数判断大页是否为 1Gig 的大页，如果大页是 1Gig 的大页，那么函数调用 destroy_compound_gigantic_page() 函数和 free_gigantic_page() 函数回收大页; 反之大页为 2MiB 的大页，那么函数直接调用 \_\_free_pages() 函数进行回收。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02042">free_pool_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001010.png)

在 Hugetlb 大页机制中，大页可以预先分配，这样的大页称为 Persistent Huge page, 也可以通过动态超发的方式进行分配，这样的大页可以称为 Surplus Huge page. 当系统没有可用大页的时候，可以动态分配 Surplus Huge page 大页进行使用。当系统使用完大页进行回收时，内核优先回收超发的大页。free_pool_huge_page() 函数用于释放大页，当 acct_surplus 参数为真的时候，该函数优先回收 Surplus Huge page 大页，反之释放普通大页，其底层逻辑为: 函数首先调用 for_each_node_mask_to_free() 函数遍历所有的 NUMA NODE, 在遍历每一个 NODE 时，当系统由空闲的大页，如果此时 acct_suplus 为真且该 NODE 上存在 Surplus Huge page，那么函数优先释放 Surplus 的大页; 反之如果 acct_surplus 为假或者不存在 Surplus Huge Page，那么系统释放普通的大页。具体的释放过程如下: 函数首先从指定长度大页的 hugepage_freelists 链表上获得一个空闲的大页，然后将该大页从链表上脱离出来，此时将指定长度大页的 free_huge_pages 和 free_huge_pages_node 减一。如果此时 acct_surplus 为真，那么此时一定存在 Surplus Huge Page, 那么函数将指定长度的 surplus_huge_page 和 surplus_huge_pages_node 减一。函数最后调用 update_and_free_page() 将大页释放。最后函数返回 ret.

> [update_and_free_page](#D02041)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02043">return_unused_surplus_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001011.png)

在 Hugetlb 大页机制中，大页可以通过预先分配的方式获得，这样的大页可以称为 Persistent Huge Page, 也可以通过超发方式动态获得，这样的大页称为 Surplus Huge Page. 当系统没有可用大页时，可以动态分配 Surplus Huge page 进行使用。当系统回收大页时，系统优先释放 Surplus Huge Page 大页。return_unused_surplus_pages() 函数的作用是尽可能的释放 Surplus Huge page，并缩减预留大页的数量，其底层逻辑是: 函数首先调用 hstate_is_gigantic() 函数判断大页是否为 1Gig 大页，如果是直接跳转到 out, 这是因为 1Gig 大页不支持 Surplus Huge page; 反之如果大页是 2MiB 大页，那么函数将从指定大页的 surplus_huge_pages 和 unused_resv_pages 参数选择一个最小的，确保本次有 Surplus Huge Page 可以进行释放。当 nr_pages 不为 0，那么表示系统有可以回收的 Surplus Huge page，那么函数使用 while 循环 nr_pages 次，在每次循环中，函数首先对指定长度大页的 resv_huge_pages 减一，接着将 unused_resv_pages 也减一，接着函数调用 free_pool_huge_page() 函数从不同的 NODE 上释放 Surplus Huge Page, 如果释放失败，那么表示所有 NODE 上都不存在 Surplus Huge page，那么函数跳转到 out 处; 反之函数将一个 Surplus Huge Page 释放。函数最后在 out 处将指定长度的 resv_huge_pages 值减去 unused_resv_pages, 这样实现了释放 Surplus Huge Page 的同时缩减了预留大页的数量.

> [free_pool_huge_page](#D02042)


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02044">hugetlb_acct_memory</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001012.png)

在 Hugetlb 大页机制中，大页可以通过预先分配的方式获得，这样的大页称为 Persistent Huge Page, 也可以通过超发的方式动态获得，这样的大页称为 Surplus Huge Page。由于 Surplus Huge Page 的存在，那么大页的数量会动态变化。hugetlb_acct_memory() 函数的作用就是用于计算调整大页内存，这里的计算调整包括判断大页是否满足预留，如果不满足则需要动态增加大页，如果满足则直接预留大页。另外还包括缩减超发的大页，其底层逻辑为: 参数 h 指向指定长度的大页，而参数 delta 表示大页改变的部分。函数如果检查到 delta 大于 0，那么函数首先调用 gather_surplus_pages() 函数确认当前可用大页是否满足预留大页，如果满足则进行大页预留; 反之当前系统没有足够的大页，那么函数进行大页超发并进行预留。当预留完毕之后函数检查系统当前可用大页数量还是小于 delta，那么函数调用 return_unused_surplus_pages() 函数将 Surplus Huge Page 进行是否并调整到 out 处. 函数如果检测到 delta 的值小于 0，表示系统需要缩减预留的大页，并是否 Surplus Huge Page. 此时函数通过调用 return_unused_surplus_pages() 函数实现。

> [return_unused_surplus_pages](#D02043)
>
> [gather_surplus_pages](#D02040)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02045">hugepage_new_subpool</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001013.png)

在 Hugetlb 大页机制中，大页可以通过 hugetlbfs 文件系统方式获得大页，在 hugetlbfs 文件系统中可以使用 subpool 方式维护一定数量的大页，subpool 并不真正维护大页，而是从系统 hstate 大页中预留一定数量的大页，然后通过 struct hugepage_subpool 中多个成员维护指定数量大页的使用。hugepage_new_subpool() 函数的作用就是创建一个 hugetlbfs 的 subpool，其底层逻辑为: 函数首先调用 kzalloc() 为 struct hugepage_subpool 类型的 spool 变量分配内存，接着对 spool 数据结构进行初始化，函数将 spool 的 count 设置为 1，表示一个使用者。函数将 max_hpages 成员设置为 max_hpages 参数，以此表示 spool 最多维护 max_hpages 个大页，函数接着将 hstate 成员指向 h 参数，以此表示该 hugetlbfs 使用的大页来自 h 描述的大页，函数接着将 min_pages 成员设置为 min_hpages 成员，以此表示 hugetlbfs 文件系统最少应该维护 min_hpages 个大页。函数如果发现 min_hpages 不为 -1，那么函数调用 hugetlb_acct_memory() 函数从 h 中预留 min_hpages 个大页，最后函数将 rsv_hpages 成员设置为 min_hpages 参数，以此表示 hugetlbfs 文件系统已经预留 min_hpages 个大页。另外可以通过挂载一个 hugetlbfs 文件系统时指定以下参数来触发系统创建 hugepage_subpool:

{% highlight c %}
# Persistent Huge Page
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
# Surplus Huge Page
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_overcommit_hugepages
# Mount Hugetlbfs
mkdir -p /mnt/BiscuitOS-hugetlbfs/
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K -o size=40M -o min_size=32M
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001014.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02046">hugetlbfs_inc_free_inodes</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001015.png)

在 Hugetlb 大页机制中，内核可以使用 hugetlbfs 文件系统为用户提供接口进行大页分配，由于使用文件系统，大页将以文件形式进行提供，因此每次提供大页分配的时候需要使用 struct inode 进行描述。hugetlbfs_inc_free_inodes() 函数的作用就是当销毁一个共享文件大页的时候增加 hugetlbfs 文件系统中可用 inode 的个数，其底层逻辑是: hugetlbfs 文件系统相关的信息维护在 struct hugetlbfs_sb_info 数据结构里，其中 free_inodes 成员用于统计文件系统中可用的 struct inode 数量，因此当调用该函数时，函数就增加 struct hugetlbfs_sb_info 的 free_inodes 成员.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02047">hugetlbfs_dec_free_inodes</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001016.png)

在 Hugetlb 大页机制中，内核可以使用 hugetlbfs 文件系统为用户提供接口进行大页分配，由于使用文件系统，大页将以文件形式提供，因此每次提供大页分配的时候需要使用 struct inode 进行描述。hugetlbfs_dec_free_inodes() 函数的作用就是当创建一个共享文件大页的时候减少 hugetlbfs 文件系统中可用 inode 的个数，其底层逻辑是: hugetlbfs 文件系统相关的信息通过 struct hugetlbfs_sb_info 数据进行描述，其中 free_inodes 成员用于统计 hugetlbfs 文件系统中空闲 struct inode 的数量，因此当调用该函数时，函数检测到 free_inodes 的数量大于 0，那么，函数试图去减少 free_inodes 的值，如果此时检查到 free_inodes 的值为 0，那么直接退出，反之将 free_inodes 的值减一.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02048">hugetlbfs_alloc_inode</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001017.png)

在 Hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统为用户空间提供大页内存，hugetlbfs 将大页内存以文件形式提供给用户，因此每次大页内存分配需要基于文件，那么 hugetlbfs 文件系统会为其创建对应的 struct inode。hugetlbfs_alloc_inode() 函数的作用是为 hugetlbfs 文件系统分配新的 struct inode，其底层逻辑为: 内核使用 struct hugetlbfs_sb_info 数据结构描述一个挂载的 hugetlbfs 文件系统，另外内核使用 struct hugetlbfs_inode_info 数据结构描述 hugetlbfs 文件系统中的 inode，其数据结构中包含 VFS 所需的 struct inode 数据结构。函数首先通过 HUGETLBFS_SB() 函数将 struct super_block 数据结构转换成 struct hugetlbfs_sb_info 数据结构，接着函数调用 hugetlbfs_dec_free_inodes() 函数对 hugetlbfs 文件系统的空闲 inode 数量减一，然后调用 kmem_cache_alloc() 函数从 hugetlbfs_inode_cachep 缓存中为 struct hugetlbfs_inode_info 分配内存，如果此时分配失败，那么函数调用 hugetlbfs_inc_free_inodes() 函数重新将空闲的 inode 数量加一。接着调用 mpol_shared_policy_init() 函数初始化 inode 的内存策略，最后函数返回 struct hugetlbfs_inode 中的 vfs_inode 成员，即真实的 struct inode 数据结构.

> [HUGETLBFS_SB](#D02032)
>
> [hugetlbfs_dec_free_inodes](#D02047)
>
> [hugetlbfs_inc_free_inodes](#D02046)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02049">hugetlbfs_destroy_inode</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001018.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页内存，由于以文件形式提供，那么每次大页分配时需要为其建立相应的 struct inode 数据结构。在 hugetlbfs 文件中使用 struct hugetlbfs_inode_info 描述 inode，其包含具体的 struct inode 数据结构。hugetlbfs_destroy_inode() 函数的作用是摧毁 inode，其底层逻辑是: 函数首先调用 hugetlbfs_inc_free_inodes() 函数增加 hugetlbfs 文件系统的可用 inode 数量，接着调用 mpol_free_shared_policy() 函数修改 inode 的内存策略，最后函数通过 call_rcu() 的方式调用 hugetlbfs_i_callback() 函数进行实际的 inode 摧毁操作.

> [hugetlbfs_i_callback](#D02050)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02050">hugetlbfs_i_callback</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001019.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页内存，由于以文件形式提供，那么每次大页分配时需要为其建立相应的 struct inode数据结构。在 hugetlbfs 文件中使用 struct hugetlbfs_inode_info 描述 inode，其包含具体的 struct inode 数据结构。hugetlbfs_i_callback() 函数的作用通过 RCU 机制对 struct hugetlbfs_inode_info 数据结构进行释放，其底层逻辑是: 当其他函数通过 RCU 机制调用到该函数时，函数通过 container_of() 函数将 head 参数转换成 struct inode 数据结构，接着函数调用 kmem_cache_free() 函数将 inode 释放回 hugetlbfs_inode_cachep 缓存中.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02051">hugetlbfs_fill_super</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001020.png)

在 Hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统为用户提供大页内存，hugetlbfs 以文件的形式为用户提供大页内存。内核使用 struct hugetlbfs_sb_info 描述 hugetlbfs 文件系统相关的信息，struct hugetlbfs_config 则用于描述 hugetlbfs 文件系统挂载时的配置信息。当内核使用 hugetlbfs 文件系统时需要将其进行挂载之后才能使用，内核可以在内核启动的时候进行 hugetlbfs 文件系统的挂载，也可以在用户空间进行挂载。hugetlbfs_fill_super() 函数的作用是当挂载一个 hugetlbfs 文件系统时，系统会调用该函数进行 hugetlbfs 文件系统挂载相关的设置，其底层逻辑是: 函数首先初始化 struct hugetlbfs_config 数据结构的 config 变量，通过 current_fsuid() 函数和 current_fsgid() 函数设置 hugetlbfs 文件系统的 kuid 和 kgid，并将文件系统的访问权限设置为 0755, 接着将 struct hugetlbfs_config 的 hstate 成员设置为 default_hstate, 以此表示该 hugetlbfs 文件系统分配 default_hstate 对应的大页。函数接着调用 hugetlbfs_parse_options() 函数解析 hugetlbfs 文件系统挂载的参数。接着函数函数调用 kmalloc() 函数为 sbinfo 变量分配内存，函数将 struct super_block 的 s_fs_info 指向了 sbinfo，以此表示该 hugetlbfs 文件系统的配置信息通过 sbinfo 遍历进行描述，接着函数将 sbinfo 的 hstate 设置为 config 的 hstate，这里完成了 hugetlbfs 文件系统与 hugetlb 的绑定，sbinfo 的 max_inodes 表示 hugetlbfs 支持最大文件数，但这里不代表最大打开文件数，free_inodes 则表示 hugetlbfs 文件系统中空闲的 inode 数量，两者的值均来自挂载 hugetlbfs 文件系统的参数，接下来将 struct hugetlbfs_config 中的数据转移到 struct hugetlbfs_sb_info 中。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001021.png)

函数接着判断 config 中的 max_hpages 和 min_hpages 是否不为 -1，两个值均从 hugetlbfs 文件系统挂载时进行配置，如果挂载时带上了 "size=" 或 "min_size=" 字段，那么 config 的 max_hpages 或者 min_hpages 将不为 -1，那么表示 hugetlbfs 文件系统在挂载时就要为其预留指定数量的大页，那么函数此时调用 hugepage_new_subpool() 函数从 hugetlb 中预留 min_hpages 数量的大页，并将 subpool 的信息维护在 sbinfo 的 spool 里。接下来是对 hugetlbfs 文件系统的基础设置: s_maxbytes 设置为 MAX_LFS_FIFSIZE 表示文件最大长度，s_blocksize 设置为 huge_page_size() 为大页的长度，s_magic 为文件系统的 MAGIC HUGETLBFS_MAGIC. 函数接着将 s_op 指向了 hugetlbfs_ops，该 ops 中包含了 inode 的创建和摧毁等操作。函数接下来调用 hugetlbfs_get_root() 函数为 hugetlbfs 文件系统分配 root 节点对应的 inode，并调用 d_make_root() 函数将该 inode 设置为 root 目录. 用户空间挂载一个 hugetlbfs 可以参考如下:

{% highlight bash %}
# Hugetlb 分配大页或超发大页
echo 10 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_overcommit_hugepages
# 创建或指定挂载目录
mkdir -p /mnt/BiscuitOS-hugetlbfs/
# 挂载 Hugetlbfs
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o size=4M,min_size=2M,pagesize=2M,nr_inodes=100
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001022.png)

> [hugetlbfs_parse_options](#D02037)
>
> [hugepage_new_subpool](#D02045)
>
> [huge_page_size](#D02005)
>
> [huge_page_shift](#D02020)
>
> [hugetlbfs_get_root](#D02052)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02052">hugetlbfs_get_root</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001023.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户提供大页内存，由于是以文件的方式提供，那么每次大页分配 hugetlbfs 都会为该文件创建 struct inode 数据结构，另外由于是以 hugetlbfs 文件系统的方式提供，那么 hugetlbfs 文件系统在挂载点下也存在 root 节点，hugetlbfs_get_root() 函数的作用就是创建 root 节点对应的 struct inode，其底层逻辑是: 函数首先通过 new_inode() 函数分配了一个新的 struct inode 数据结构，如果分配成功，那么函数使用 struct hugetlbfs_config 维护的配置信息对该 inode 进行初始化，其中将 inode 的 i_op 指向了 hugetlbfs_dir_inode_operations，而 i_fop 指向了 simple_dir_operations. 初始化完毕之后，函数返回该 inode.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02053">resv_map_alloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001024.png)

在 Hugetlb 大页机制中，内核可以通过 hugetlbfs 文件系统以文件的形式为用户提供大页内存. 当分配大页时，内核需要在 mmap 阶段将需要分配的大页内存进行预留，以便发生缺页时内核可以提供足够数量的大页。内核使用 resv_map 机制维护大页的预留，在共享文件分配内存时，A 进程预留的共享大页内存被 B 进程打开时，内核不必在预留大页内存，因为 A 和 B 进程都使用共同的大页，因此不许额外使用其他大页内存。Hugetlb 大页机制使用 resv_map 来实现共享文件间的大页预留。resv_map_alloc() 函数的作用就是为共享文件分配一套 resv_map，其底层逻辑为: struct resv_map 描述共享文件大页内存的预留信息，struct file_region 描述某个进程对共享文件大页内存的使用情况。函数首先通过 kmalloc() 为 resv_map 和 rg 变量分配内存，接着调用 kref_init() 函数初始化 struct resv_map 的 refs 成员，以此表示该贡献文件大页内存没有其他进程使用。函数接着将维护各个进程的 file_region 链表 resv_map->regions 进程初始化，此时函数将 adds_in_progress 设置为 0，表示此时共享文件的大页内存不存在预留变更。函数接着将 rg 插入到 resv_map->regions_cache 链表中，供特殊情况使用，并将 region_cache_count 设置为 1，表示该 resv_map 维护了 1 个可用的 struct file_region. 函数最后返回 resv_map。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02054">hugetlbfs_get_inode</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001025.png)

在 Hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户提供大页内存，每次通过该方法分配大页内存时，内核都会在 hugetlbfs 文件系统挂载点下创建一个 struct inode, 并打开或创建文件的方式使用大页。hugetlbfs_get_inode() 函数的作用就是为 hugetlbfs 文件系统创建一个新的 VFS inode, 其底层逻辑是: 在 hugetlb 大页机制中，大页在 mmap 阶段需要将分配的大页内存进行预留，内核使用 resv_map 机制维护共享文件内存的预留信息，函数在创建文件阶段，在 46 行通过调用 resv_map_alloc() 函数分配 resv_map 数据结构，然后在 50 行通过 new_inode() 函数分配一个 VFS inode. 如果 inode 分配成功，那么文件创建成功。在 hugetlbfs 文件系统中使用 struct hugetlbfs_inode_info 描述 hugetlbfs inode 信息。函数在 54 行设置了 VFS inode ID，并在 58 行设置了 VFS inode address_space operations 为 hugetlbfs_aops. 接着 59 行通过调用 current_time() 函数设置 VFS inode 时间相关的信息。函数接下来在 60 行将 resv_map 塞到 VFS inode 的 i_mapping->private_data 里，以便共享时候使用。接下来函数将判断 VFS inode 的类型，如果是 S_IFREG, 即 VFS inode 描述一个文件，那么函数将 VFS inode 的 inode operations 设置为 hugetlbfs_inode_operations, 并将 VFS inode 的 file operations 设置为 hugetlbfs_file_operations; 反之 inode 的类型是 S_IFDIR, 那么 VFS inode 是一个目录，那么函数将 VFS inode 的 inode operations 设置为 hugetlbfs_dir_inode_operations, 同时将 VFS inode 的 file operations 设置为 simple_dir_operations, 最后调用 inc_nlink() 函数将 inode 的 i_nlink 加一，以此让新目录下 的 "." 的引用计数为 2; 反之 inode 类型是 S_IFLNK, 那么 VFS inode 是一个链接，那么将 VFS inode 的 inode operations 设置为 page_symlink_inode_operations, 并调用 inode_nohighmem() 函数。函数最后返回 VFS inode.

> [resv_map_alloc](#D02053)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02055">hugetlbfs_mknod</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001026.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户提供大页内存，因此通过次方式分配大页内存之前都需要在 hugetlbfs 文件系统挂载点下创建一个文件或者打开一个已经存在的文件，hugetlbfs_mknod() 函数的作用是在 hugetlbfs 文件系统中创建一个文件或目录，其底层逻辑是: 函数首先调用 hugetlbfs_get_inode() 函数在参数 dir 的目录下创建并初始化一个 VFS inode, VFS inode 可以是一个文件或者是一个目录，当 VFS inode 创建成功，那么函数调整 dir inode 的时间为 current_time() 对应的时间，接着函数调用 d_instantiate() 将新分配的 VFS inode 加入到 dentry 目录下作为新的节点，最后调用 dget() 增加目录的引用计数. 至此 dentry 目录下新增一个文件或者目录.

> [hugetlbfs_get_inode](#D02054)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02056">hugetlbfs_mkdir</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001027.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页内存，那么当使用该形式分配大页内存时，内核需要在 hugetlbfs 文件系统的挂载点下创建一个文件或打开一个已经存在的文件，hugetlbfs_create() 函数的作用是在 hugetlbfs 文件系统挂载点下创建一个文件，其底层逻辑是: 参数 dentry 指明 hugetlbfs 文件系统挂载点信息，参数 dir 则是挂载点目录对应的 VFS inode，函数直接通过调用 hugetlbfs_mknod() 函数创建文件. 函数调用完毕之后可以在 dentry 目录下看到一个文件.

> [hugetlbfs_mknod](#D02055)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02057">is_file_hugepages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001028.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页，基于该形式，在每次分配大页的时候，内核需要在 hugetlbfs 文件系统挂载点下创建一个文件或打开一个已经存在的大页。is_file_hugepages() 函数的作用是判断一个文件是否为 hugetlbfs 的大页文件，其底层逻辑为: hugetlbfs 的文件大页都是维护 hugetlbfs 文件系统挂载点下，并且其 file operations 设置为 hugetlbfs_file_operations, 因此可以通过这个条件判断一个文件是否为大页文件。另外由于历史原因还保留 SHM 的大页接口，is_file_shm_hugepages() 函数则判断 file operations 为 shm_file_operations_huge 时也认为文件为用于映射大页的文件.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02058">subpool_inode</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 Hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式向用户空间提供大页内存，如果使用该方式分配内存，那么首先需要挂载 hugetlbfs 文件系统，hugetlbfs 使用 struct hugetlbfs_sb_info 描述 hugetlbfs super block 信息，其成员 spool 指向了该 hugetlbfs 挂载点上大页数量信息。函数可以根据上面的信息通过 VFS 的关系链从 struct file/struct inode/struct super_block 最终找到 subpool。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001029.png)

函数 subpool_inode() 的作用就是通过 VFS inode 找到对应 hugetlbfs 文件挂载点的 subspool. 其底层逻辑为: 首先通过 i_sb 找到 VFS super_block, 然后通过宏 HUGETLBFS_SB() 函数找到对应的 hugetlbfs super block, 其 spool 成员指向 struct hugepage_subpool。

> [HUGETLBFS_SB](#D02032)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02059">inode_resv_map</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001030.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件页的形式为用户空间提供大页内存。在进程分配虚拟内存用于映射大页物理内存时，内核需要为虚拟内存预留足够的大页，此时内核使用 resv_map 机制进行预留大页的管理。inode_resv_map() 用于获得 struct resv_map 数据，其底层逻辑为: 在 hugetlbfs 文件系统挂载点上，当内核分配大页内存时，需要进程在其挂载点下创建一个文件或打开一个文件，因此每次大页分配都涉及一个 VFS inode，因此内核将 resv_map 塞到了 inode->i_mapping 的 private_data 成员里，这样在共享方式打开文件时，可以获得唯一的 struct resv_map 数据。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02060">region_chg</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001031.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，当进程需要通过文件方式分配大页，那么首先需要在 hugetlbfs 文件系统的挂载点上创建一个文件或打开一个文件，然后将文件通过 mmap 映射到进程的地址空间中。在进程映射过程中，内核需要计算虚拟内存消耗的大页数量，并将这部分大页进行预留。另外如果文件是以共享的方式创建，那么表示多个进程可以共同使用同一个大页，那么内核在进程 mmap 时只需预留一份大页内存，而不需要每个进程都预留一份。内核为了更好的管理多个进程共享一个文件使用的大页预留，引入了 resv_map 机制进行预留内存的管理。resv_map 机制以打开文件的 pgoff 和 len 作为区间注册到 resv_map 维护的链表上，如果其他进程打开文件的区间已经在 resv_map 链表上了，那么内核不需要为其预留大页; 反之区间不完全与 resv_map 链表上存在的区间重合，那么内核需要为多出的区域预留大页内存.

region_chg() 函数的作用就是判断新增加的区域是否需要预留大页, 其底层逻辑为: 函数首先在 357 行获得 resv_map 的区间链表，每个区间使用 struct file_region 进行描述，函数接着在 370 行检查 resv_map 的 adds_in_progress 是否大于 region_cache_count, 这里 adds_in_progress 表示正在修改 resv_map 进程的数量，而 region_cache_count 则表示 resv_map 维护的 struct file_region 缓存数量，如果 adds_in_progress 大于 region_cache_count, 那么表示多个进程正在进行预留，但 struct file_region 缓存不够，此时需要动态申请一个 struct file_region, 并将其添加到 resv_map 的 region_cache 链表上，以便供其他进程分配时使用。分配完毕之后增加 resv_map region_cache_count 成员的数量，最后跳转到 retry_locked 处重新执行该函数. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001032.png)

函数接着在 391 行遍历所有已经预留的区域，如果检查到预留的区域的结束地址不小于此次预留的起始值，那么结束循环，发生这种情况的一般是以前的进程打开了文件的后半段，文件的前面部分还没有打开，那么这时系统可能向前或者预留区间的范围。另外如果文件没有被任何进程预留过，那么 rg 变量不指向任何 file_region. 函数接着在 398 行判断，如果 resv_map 没有任何预留区间，或者新预留的结束地址小于已经存在预留区间的起始值，那么函数进入 399 的分支执行，如果此时 nrg 变量为空，那么函数进入 400 分支，函数首先将 adds_in_progress 减一，并调用 kmalloc() 函数为 nrg 分配内存，并将 nrg 的 from 和 to 成员都设置为 f，并将 nrg 链表初始化，最后在 409 行跳转到 retry 处执行，这个分支一般是需要新增预留区间的情况会执行，并且第一次进程 400 行分支，跳转到 retry 之后还是会进入 399 分支，第二次进入函数直接执行 412 行代码，函数将 nrg 加入到 resv_map 的区间链表上，并计算此时预留区域的长度，t 减去 f 的值即为区间改变的值，函数接着跳转到 out_nrg 处执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001033.png)

首先看 444 行 out_nrg 处，函数将 spin 锁解除之后直接返回新增区间的大小。另外一种情况是 resv_map 的预留区间链表上已经存在预留区间，此时可能出现三种情况: 第一种是需要预留区间被已经存在的预留区间包含、第二种情况则是新增区域可能与已存在的区域重叠，但新增区间的左边存在一部分并不与存在的预留区间重叠、同理最后一种是新增区域可能和已经存在的区间重叠，但新增区域的最右边一定存在一份区域与已经存在的预留区间不重合。对于上面情况，函数的处理在 418 行，函数发现新增区域的起始值大于已经存在区域的起始值，那么函数将新增区域的起始值设置为已经存在预留区间的起始值，函数接着在 420 行重新计算了新增区间的长度。函数继续在 423 行使用 list_for_each_entry() 函数遍历 resv_map 的预留区间，在遍历每个预留区域时，函数首先在 424 行判断是否已经遍历一遍了，如果是则跳出遍历; 反之如果没有遍历完，那么函数在 426 行如果检查到预留区域的起始地址大于新增区域的结束地址，由于这种情况上一张图已经处理完了，那么此时函数认为这是一种错误的情况，那么函数直接跳转到 out 处; 反之那么新增区间可能与已存在的预留区间可能相交. 函数继续在 432 行判断预留区间的结束地址是否大于新增区域的结束地址，如果大于，那么函数在 433 行重新计算新区域的长度，并将新增区域的结束地址指向了已经存在区域的结束地址，最后函数在 436 行再次计算新区域的长度。函数最后返回新增预留区域的长度 chg.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02061">region_add</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001034.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，当进程需要通过文件方式分配大页，那么首先需要在 hugetlbfs 文件系统的挂载点上创建一个文件或打开一个文件，然后将文件通过 mmap 映射到进程的地址空间中。在进程映射过程中，内核需要计算虚拟内存消耗的大页数量，并将这部分大页进行预留。另外如果文件是以共享的方式创建，那么表示多个进程可以共同使用同一个大页，那么内核在进程 mmap 时只需预留一份大页内存，而不需要每个进程都预留一份。内核为了更好的管理多个进程共享一个文件使用的大页预留，引入了 resv_map 机制进行预留内存的管理。resv_map 机制以打开文件的 pgoff 和 len 作为区间注册到 resv_map 维护的链表上，如果其他进程打开文件的区间已经在 resv_map 链表上了，那么内核不需要为其预留大页; 反之区间不完全与 resv_map 链表上存在的区间重合，那么内核需要为多出的区域预留大页内存.

region_add() 函数的作用是向 resv_map 中新增一个预留区间，其底层逻辑为: 函数首先在 261 行获得 resv_map 的预留区间链表，然后在 267 行使用 list_for_each_entry() 函数遍历所有预留区域，如果发现新增预留区间起始值不大于现有预留区间的结束地址，那么函数停止循环, 如果遍历里所有现有的预留区间都没有匹配该条件，那么新增区域的起始值比现存的预留区间还大。无论两种情况如何，函数都获得一个现有的预留区间。函数接着在 277 行判断 rg 是否为空，或者新增预留区的结束地址小于 rg 对应预留区的起始地址，那么函数将进入 278 分支继续执行。函数从 resv_map 的 region_cache 中获得一个 struct file_region, 然后将该预留区的起始值 from 和结束值 to 设置为新增预留区范围，接着将预留区加入到 resv_map 的预留区链表上，最后计算处新增预留区的长度之后跳转到 out_locked 处运行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001035.png)

函数在 294 行判断新增预留区间的起始值是否大于现有预留区间的起始值，如果大于，那么表示新增预留区在后面，那么函数将新增预留区间的起始值设置为现有预留区间的起始地址。函数接着在 299 行调用 list_for_each_entry_safe() 遍历 resv_map 的所有预留区间，在每次遍历过程中，函数在 300 行判断是否已经遍历一遍了，如果是直接退出; 反之如果不是，那么函数检测到预留区的起始地址大于新增预留区的结束地址，那么函数直接结束循环，新增的预留区将添加在现有预留区; 反之如果预留区的结束地址大于新增预留区，那么函数将新增预留区的结束值设置为现有预留区的结束值; 如果此时检查到遍历的预留区不等于已经找到的预留区，那么函数认为原先的预留区已经被移除，那么此时函数将预留区从 resv_map 的预留区链表中移除，是否预留区资源并更新新增预留区的大小。经过遍历之后，函数找到了一个合适的区间将新增预留区插入进去，那么函数在 321 行再次更新新增预留区的长度，并将预留区的范围扩大成新的范围.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02062">region_del</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001036.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，当进程需要通过文件方式分配大页，那么首先需要在 hugetlbfs 文件系统的挂载点上创建一个文件或打开一个文件，然后将文件通过 mmap 映射到进程的地址空间中。在进程映射过程中，内核需要计算虚拟内存消耗的大页数量，并将这部分大页进行预留。另外如果文件是以共享的方式创建，那么表示多个进程可以共同使用同一个大页，那么内核在进程 mmap 时只需预留一份大页内存，而不需要每个进程都预留一份。内核为了更好的管理多个进程共享一个文件使用的大页预留，引入了 resv_map 机制进行预留内存的管理。resv_map 机制以打开文件的 pgoff 和 len 作为区间注册到 resv_map 维护的链表上，如果其他进程打开文件的区间已经在 resv_map 链表上了，那么内核不需要为其预留大页; 反之区间不完全与 resv_map 链表上存在的区间重合，那么内核需要为多出的区域预留大页内存.

region_del() 函数的作用从 resv_map 中移除一个预留区，其底层逻辑是: 参数 f 和 t 指明了需要移除预留区的范围，函数在 484 行获得 resv_map 预留区链表，然后函数在 491 行使用 list_for_each_entry_safe() 函数遍历 resv_map 的所有预留区间，在遍历每个预留区时，函数首先在 499 行进行判断，如果需要移除的预留区的起始值不小于当前预留区间的结束值，那么函数继续判断，如果此时预留区不是空的，或者当前预留区间与移除区间不想交，那么函数继续遍历下一个预留区间; 反之函数继续在 502 行进行判断，如果当前区间的起始值不小于移除区间的结束地址，那么说明移除区间和当前区间可能相离，但不相交，那么说明移除区间并不在 resv_map 维护的预留区间内，此时函数直接结束循环; 反之函数继续在 505 行进行判断，如果移除区域的起始值大于当前预留区间的起始值，并且当前预留区的结束值大于移除预留区域的结束地址，那么移除区域位于当前预留区的内部，那么当前预留区间将会被拆分成两个预留区，函数进入 506 分支，函数在 510 行检查到 nrg 为 NULL，并且 resv_map 的 region_cache_count 大于 adds_in_progress, 那么函数认为 resv_map region_cache 中维护的 struct file_region 过多，于是使用 list_first_entry() 函数从 resv_map 的 region_cache 链表中移除一个 struct file_region 成员，接着将 resv_map 的 region_cache_count 减一; 反之如果此时只检测到 nrg 为空，那么此时调用 kmalloc() 函数为 nrg 分配内存, 内存分配成功之后，函数跳转到 retry 处继续执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001037.png)

函数继续在 527 行计算移除区域的长度，由于此时移除的区域位于当前预留区的前段，那么函数在 530 行将 nrg 的起始值设置为移除预留区的结束地址，将 nrg 的结束值设置为当前区域的结束值。函数最后将 nrg 插入到 resv_map 的预留区链表上; 另外处理上面的情况，如果移除区间的范围正好包含了当前预留区，那么函数进入 543 分支，函数统计当前预留区的长度并存储到 del 遍历里，最后将当前预留区从 resv_map 的预留区间链表中移除，并释放预留区占用的内存. 最后移除的区域可以部分与当前预留区相交，函数 549 行检测到移除区间与当前预留区的前半部分相交，那么函数直接将当前预留区间的起始值设置为移除预留区的结束值，del 为当前预留区起始值到移除预留区结束值的长度; 函数 552 行则检测到移除预留区间与当前预留区间的后半部相交，那么函数只需将当前预留区的结束值设置为移除预留区的起始值即可，del 的值则为移除预留区的起始值与当前预留区结束值的长度。遍历完所有的预留区之后，移除区域已经从 resv_map 预留区间中移除，因此函数在 559 行将 nrg 占用的内存释放.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02063">region_abort</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001038.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，当进程需要通过文件方式分配大页，那么首先需要在 hugetlbfs 文件系统的挂载点上创建一个文件或打开一个文件，然后将文件通过 mmap 映射到进程的地址空间中。在进程映射过程中，内核需要计算虚拟内存消耗的大页数量，并将这部分大页进行预留。另外如果文件是以共享的方式创建，那么表示多个进程可以共同使用同一个大页，那么内核在进程 mmap 时只需预留一份大页内存，而不需要每个进程都预留一份。内核为了更好的管理多个进程共享一个文件使用的大页预留，引入了 resv_map 机制进行预留内存的管理。resv_map 机制以打开文件的 pgoff 和 len 作为区间注册到 resv_map 维护的链表上，如果其他进程打开文件的区间已经在 resv_map 链表上了，那么内核不需要为其预留大页; 反之区间不完全与 resv_map 链表上存在的区间重合，那么内核需要为多出的区域预留大页内存.

region_abort() 函数的作用是终止对 resv_map 预留区的修改，其底层逻辑是: 当函数检测到 resv_map 的 region_cache_count 为 0 时，函数就调用 VM_BUG_ON() 进行报错，此时将 resv_map 的 adds_in_progress 减一，以此终止系统继续向 resv_map 添加新的宇路区间.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02064">is_vm_hugetlb_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001039.png)

在 Hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件页形式为用户空间提供大页内存。进程通过在 hugetlbfs 文件系统挂载点上创建文件或打开已经存在的文件方式，将文件 mmap 到进程的地址空间，通过这种方式映射的虚拟内存可以与大页进行映射。is_vm_hugetlb_page() 函数用于判断进行的指定虚拟内存是否映射 hugetlbfs 提供的大页，其底层逻辑是: 当进程打开文件之后，使用 mmap 分配虚拟内存用于映射大页内存时，内核会将 VMA 添加 VM_HUGETLB 表示，这样将映射大页的 VMA 与一般的 VMA 区分开来。因此该函数通过判断 VMA 中是否包含 VM_HUGETLB 表示进行实际的判断.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02065">set_vma_resv_map</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001040.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页内存。当进程在为大页内存分配用于映射的虚拟内存时，内核需要为进程预留指定数量的大页内存，内核使用 resv_map 机制管理大页内存的预留。当通过 hugetlbfs 文件系统挂载点创建文件或打开文件的时，hugetlbfs 文件系统都会为文件准备唯一的 VFS inode, 对于共享文件而言，VFS inode 可以被多次打开，因此对于这种情况，多个进程可以实现大页内存的共享。由于存在多个进程共享同一个大页，那么此时系统只需预留一个大页，为了更好的维护大页预留，linux 使用 resv_map 机制，为了确保多个文件共享大页时能够正确的进行大页预留，那么内核将 resv_map 维护在 inode 里。但对于私有文件页，那么进程不会与其他进程共享一个大页，那么此时可以将 resv_map 放置在 VMA 的 private_data 里。set_vma_resv_map() 函数的作用就是将 resv_map 放置到指定的 VMA 的 private_data 里进行维护，其底层逻辑是: 函数首先调用 is_vm_hugetlb_page() 函数判断对应的 VMA 是否映射 hugetlb 大页，如果不是则报错. 接着函数如果检测到 VMA 是共享的，而不是私有的，那么函数会报错. 检测都通过之后，函数调用 set_vma_private_data() 函数将 resv_map 存储到 VMA 的 private_data 里，并带上了 HPAGE_RESV_MASK 标志.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02066">vma_resv_map</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001041.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户提供大页内存，由于以文件形式给出，因此需要在 hugetlbfs 文件系统挂载点下创建文件或打开文件方式进行大页内存分配。进程在打开文件之后需要通过 mmap 为大页内存分配虚拟内存，进程可以通过共享的方式进行映射，可以通过私有的方式进行映射，无论使用那种映射方式，进程 mmap 分配虚拟内存时，内核同时计算需要分配大页的数量，并将这些大页进行预留。内核使用 resv_map 机制管理预留内存，由于采用不同的映射方式，resv_map 相关的管理数据可能维护在 VFS inode 中，也可能维护在 VMA 的 private_data 里. vma_resv_map() 函数的作用是获得进程指定 VMA 映射大页时使用的 resv_map. 其底层逻辑为: 函数首先调用 is_vm_hugetlb_page() 函数判断 VMA 是否通过 hugetlbfs 文件方式分配的大页，如果不是则报错. 函数接着判断 VMA 是共享映射还是私有映射，如果是共享映射，那么 resv_map 存在于 VFS inode 中，那么函数调用 inode_resv_map() 函数从 VFS inode 中取出 resv_map; 反之是私有映射，那么 resv_map 存在与 VMA 的 private_data，因此函数直接通过调用 get_vma_private_data() 函数并去除预留标志之后，即是 resv_map。

> [inode_resv_map](#D02059)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02067">set_vma_resv_flags</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001042.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件页的形式为用户空间提供大页内存。其文件页方式支持共享和私有的方式，当通过该方式分配大页内存时，根据需要在使用大页之前需要对大页内存提前预留，私用和共享在预留上存在不同的策略，对于共享，预留的内存可以和其他进程共享; 对于私有方式，那么预留的内存只能自己使用。由于两种方式在预留大页存在差异，那么也就导致 fork 场景下预留大页的策略不同。对于私有映射大页预留在 VMA 的 private_date 里，并且需要标记该大页是预留给父进程的，因此内核需要对父进程预留的大页进行打标。set_vma_resv_flags() 函数的作用就是用于设置预留页的标志，对于父进程，其预留标志可以设置为 HPAGE_RESV_OWNER，表示大页预留给自己，不预留给子进程。函数的底层逻辑是: 函数首先调用 is_vm_hugetlb_page() 函数判断 VMA 是否通过 hugetlbfs 映射的大页，如果不是就报错. 接着判断 VMA 是否为私有映射，如果不是，那么函数同样报错。最后函数调用 set_vma_private_data() 函数向 VMA 的 resv_map 添加上 flags 的标志.

> [is_vm_hugetlb_page](#D02064)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02068">is_vma_resv_set</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001043.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件页的形式为用户空间提供大页内存。其文件页方式支持共享和私有的方式，当通过该方式分配大页内存时，根据需要在使用大页之前需要对大页内存提前预留，私用和共享在预留上存在不同的策略，对于共享，预留的内存可以和其他进程共享; 对于私有方式，那么预留的内存只能自己使用。内核使用 resv_map 机制管理预留大页内存，由于私有和映射存在差异，内核将 resv_map 存储在不同位置。对于私有映射方式，内核将 resv_map 维护在 VMA 的 private_data 成员里，另外对于私有映射，内核还会打上不同标志，因此确认大页是给父进程预留的还是给子进程预留的。函数 is_vma_resv_set() 函数的作用就是判断 VMA 的 private_data 是否已经打上相应的标签。函数的底层逻辑为: 函数首先调用 is_vm_hugetlb_page() 函数判断 VMA 是否映射了 hugetlb 大页，如果没有映射则报错. 函数接着调用 get_vma_private_data() 函数获得 VMA 的 private_data 成员，并判断其是否已经打上 flag 对应的标.

> [is_vm_hugetlb_page](#D02064)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02069">reset_vma_resv_huge_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001044.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件页的形式为用户空间提供大页内存。其文件页方式支持共享和私有的方式，当通过该方式分配大页内存时，根据需要在使用大页之前需要对大页内存提前预留，私用和共享在预留上存在不同的策略，对于共享，预留的内存可以和其他进程共享; 对于私有方式，那么预留的内存只能自己使用。内核使用 resv_map 机制管理预留大页内存，由于私有和映射存在差异，内核将 resv_map 存储在不同位置。对于私有映射方式，内核将 resv_map 维护在 VMA 的 private_data 成员里; 而对于共享映射 resv_map 则存储在文件对于 inode 里，这样方便共享的进程使用。reset_vma_resv_huge_pages() 函数的作用是当内核释放并解除一段大页映射的时候，内核需要将私有映射的 VMA private_data 成员清零，其底层逻辑是: 函数首先调用 is_vm_hugetlb_page() 判断 VMA 是否映射了 hugetlb 大页，如果不是则报错。接着函数判断 VMA 是否为私有映射，如果是则匹配上了私有 hugetlb 映射，那么函数将 VMA 的 private_data 成员清零.

> [is_vm_hugetlb_page](#D02064)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02070">vma_has_reserves</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001045.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页内存。进程在使用大页内存之前，需要在进程地址空间分配虚拟内存，以此在缺页时将虚拟内存映射到大页的物理内存上。进程在为大页分配虚拟内存时需要提前预留大页内存，然后等待进程访问大页引起缺页时，内核从预留的大页中为其分配大页内存，并建立虚拟内存到大页物理内存的映射。内核使用 resv_map 机制来管理 hugetlb 大页的预留，另外由于文件存在私有映射和共享映射，那么 resv_map 预留策略存在差异。对于共享映射，多个进程可以与其他进程共享预留大页，而私有映射只能使用自己的预留大页。vma_has_reserves() 函数的作用是判断 chg 个大页是否需要预留。函数的底层逻辑是: 如果 VMA 带有 VM_NORESERVE 标志，那么表示该进程的 VMA 使用的大页内存在映射阶段不需要预留，或者表示其他进程已经预留的大页。函数接着检查到如果 chg 为 0 即不需要预留任何大页，且 VMA 是共享映射，因此这种情况下内核认为大页已经预留好了; 反之返回 false。函数如果没有检查到 VMA 中包含 VM_NORESERVE, 那么函数接着判断是否为共享映射，如果是且 chg 不为 0，那么代表内核还没有为 chg 对应的虚拟内存预留大页，因此返回 false; 如果此时 chg 为 0，那么表示内核已经为虚拟内存预留了大页，因此返回 true.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001046.png)

如果函数发现 VMA 是私有映射，那么函数调用 is_vma_resv_set() 判断 VMA 的 private_data 是否已经包含了 HPAGE_RESV_OWNER 标志，如果包含，那么该进程是父进程并使用自己预留的大页，此时如果 chg 不为 0 表示需要新预留大页，因此返回 false; 反之表示私有映射无需预留新的大页。如果以上情况都不匹配，那么函数直接返回 false.

> [is_vma_resv_set](D02068)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02071"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001047.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页内存。内核在初始化阶段自动 mount 里不同长度大页的 hugetlbfs 文件系统挂载点，其挂载点维护在 hugetlbfs_vfsmount[] 数组中，数组中的成员按大页的长度从小进行排序，2M 大页的 mount 点为 hugetlbfs_vfsmount[] 数组的第一个成员。get_hstate_idx() 函数的作用是通过长度找到其在 hugetlbfs_vfsmount[] 数组中的索引，其底层逻辑为: 参数 page_size_log 长度的 log 指数，函数首先调用 hstae_sizelog() 函数获得指定长度大页的 struct hstate 数据结构，如果 h 存在，那么将 h 减去 hstates 数组，因此可以获得偏移，因此可以知道 hstates[] 与 hugetlbfs_vfsmount[] 数组存在一一对应关系.

> [hstate_sizelog](#D02031)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02072">hugepage_subpool_get_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001048.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式为用户空间提供大页内存。在每个 hugetlbfs 文件系统的 mount 点下都维护一个 subpool 小池子，该池子可以预留一定数量的大页进行管理。内核如果在该 mount 点下分享大页内存，那么内核将从 subpool 池子中分配大页内存。hugepage_subpool_get_pages() 函数用于从指定的 subpool 中分配大页，其底层逻辑为: 参数 spool 为指定 mount 点下的 subpool 池子，参数 delta 表示需要分配大页的数量。函数首先判断 spool 是否为空，有的 hugetlbfs 文件系统的 mount 点下就不带 subpool，因此 spool 参数可能为空，如果为空直接返回不从 subpool 小池子分配大页; 反之 spool 不为空，那么函数检查 spool 的 max_hpages 是否不为 -1， 那么表示 subpool 池子设置了最大大页数量，那么函数将池子已经分配的大页数 used_hpages 加上要分配的大页数量的和，如果和没有超过 subpool 的最大大页数，那么函数将 subpool 的 used_hpages 加上 delta; 反之表示 subpool 不能在增加大页了，那么函数直接返回 ENOMEM; 如果 subpool 没有设置维护大页的上限，那么函数此时检查 subpool 维护大页数量的下限，如果此时 min_hpages 不为 -1，并且 subpool 的 rev_hpages 不为 0，那么 subpool 设置了下限，并且预留了一定数量的大页，那么函数此时检查 delta 是否大于预留的大页，如果不大于，那么 subpool 预留的大页可以满足分配，那么函数将 ret 设置为 0，然后将 subpool 的 rsv_hpages 减去 delta; 反之如果此时预留的大页不够 delta 分配，那么函数计算抛开预留还剩没有分配的大页数量，并将 subpool 的 rev_hpages 设置为 0. 函数最后返回 ret，此时 ret 代表了还需要额外分配大页的数量.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02073">hugepage_subpool_put_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001049.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式向用户空间提供大页内存。在 hugetlbfs 文件系统 mount 的挂载点下都维护一个 subpool 小池子，小池子里从系统大页中预留一定数量的大页进行维护。如果进程在该 mount 挂载点下分配大页内存，那么内核会从该 subpool 中分配大页内存，如果进程不再使用大页内存，那么进程将大页再次归还 subpool。hugepage_subpool_put_pages() 函数的作用是将大页归还给 subpool，其底层逻辑为: 参数 spool 指向需要归还的池子，参数 delta 则表明需要归还大页的数量。函数首先判断 subpool 是否存在，由于 hugetlbfs 文件系统的挂载点不一定都存在 subpool。如果 subpool 不存在，系统直接返回，那么需要归还的大页就直接返回给系统大页池子里; 反之 subpool 存在，那么 subpool 中的 max_hpages 表示该池子能够维护最大的大页数，min_hpages 表明 subpool 至少维护的大页数量，used_hpages 则表示 subpool 池子已经分配的大页数量，rev_hpages 则表示 subpool 已经从系统大页池子中预留的大页数量. 函数如果检测到 max_hpages 不为 -1，那么 subpool 池子已经设置了最大维护大页数量，那么函数直接将 used_hpages 减去 delta 个大页。函数如果接着检测到 min_hpages 不等于 -1，并且 used_hpages 小于 min_hpages, 那么说明 subpool 需要预留一定数量的大页。如果 rsv_hpages 加上 delta 之后不超过 min_hpages, 那么说明 subpool 不需要从系统大页池子中预留大页; 反之 subpool 不要从大页池子中预留大页。函数接着将 rsv_hpages 增加 delta 进行预留，如果此时 rsv_hpages 大于 min_hpages, 那么 subpool 需要减少预留大页的数量，那么函数将 rsv_hpages 设置为 min_hpages. 函数最后返回 ret 表示需要从系统大页池子中再预留大页的数量.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02074">hugetlb_reserve_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001050.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式向用户空间提供大页内存。进程通过在 hugetlbfs 文件系统的挂载点下创建或打开文件的方式，将大页物理内存映射到进程的地址空间。进程在使用 mmap 从地址空间分配虚拟内存时，内核需要为虚拟内存预留指定数量的大页，由于进程可以通过共享或私有的方式映射大页内存，内核使用 resv_map 管理预留大页内存。对于共享映射，resv_map 用于管理多个进程之间预留大页内存，而对于私有映射，resv_map 只需维护父进程大页预留。hugetlb_reserve_pages() 函数的作用是进程在分配虚拟内存时预留指定数量的大页内存，其底层逻辑是: 参数 inode 指向打开的文件，参数 from 和 to 指明了文件 page cache 的范围，参数 vma 指明进程的 VMA，vm_flags 则表明 VMA 映射内存的标志. 函数首先在 439 行通过 hstate_inode() 函数将 inode 转换成指定长度的 struct hstate 数据结构，接着在 440 行通过 subpool_inode() 函数获得 hugetlbfs 文件系统挂载点的 subpool 小池子。from 和 to 参数是文件的按大页粒度的文件偏移，如果 from 大于 to，那么函数认为这是不可能发生的情况，因此函数在 446 行报错之后直接返回 -EINVAL. 函数接着在 455 行判断 VMA 在映射时是否设置了 MAP_NORESERVE 这个标志，这个描述被内核转换为 VM_NORESERVE, 以此告诉内核在为该 VMA 映射时不要预留大页内存，那么如果 VMA 包含了该标志，函数无需为进程预留大页内存，那么函数直接返回 0. 接下来函数在 464 行判断 VMA 不存在或者 VMA 是共享映射的情况，那么函数进入 465 行分支，这里对于 VMA 不存在的情况，其一般发生在 IPC 进程之间使用 hugetlb 大页通信的情况, 因此这两种情况都可以统一归纳为共享映射模式下，resv_map 维护在文件对于的 struct inode 中，因此函数调用 inode_resv_map() 函数获得 resv_map, 将 resv_map 存在 inode 中便于多个进程打开文件时对应的 inode 都是同一个，那么这样便于 resv_map 数据的唯一和同步。函数接着在 467 行调用 region_chg() 函数检查 from 到 to 的区域中最终需要新增几个大页进行预留，结果存储在 chg 中，这里可以知道如果文件的某个区域被其他进程预留过大页，那么其他进程在使用文件这段区域时将不会被预留大页; 反之 VMA 是私有映射，所谓私有映射就是内存只能该进程写不能被其他进程共享，因此对应私有映射，函数进程 470 分支。对于私有映射，其 resv_map 可以存储在进程 VMA 的 private_data 成员里，因为 VMA 的 private_data 的数据不会继承到子进程或被其他进程共享，因此私有映射的 resv_map 维护在 VMA 的 private_data 中。函数首先在 470 行调用 resv_map_alloc() 函数新分配一个 resv_map 专门给当前进程使用，此时函数在 474 行将 to 减去 from 计算预留的大页数量，接着在 476 行调用 set_vma_resv_map() 函数将新分配的 resv_map 塞到 VMA 的 private_data 成员里，并调用 set_vma_resv_flags() 函数将 HPAGE_RESV_OWNER 标志加入到 resv_map 里，以此表示预留的大页只供给该进程使用。函数接着在 480 行检测到 chg 小于 0，那么预留的大页数不正确，那么函数跳转到 out_err 处; 反之 chg 可能是不小于 0 的值.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001051.png)

函数在 490 行调用 hugepage_subpool_get_pages() 函数从 hugetlbfs 文件系统挂载点的 subpool 中预留 chg 数量的大页，如果 subpool 有足够数量的大页，那么函数将从 subpool 进行预留; 反之 subpool 中没有足够的大页，那么函数将不从 subpool 中进行预留。另外有的 hugetlbfs 文件系统挂载点并没有 subpool 小池子。函数接下来在 500 行调用 hugetlb_acct_memory() 函数检查系统大页池子中是否有充足的大页进行预留，如果有则从大页池子中预留指定数量的大页，如果没有充足的大页的话，那么函数通过大页超发机制动态分配指定数量的大页进行预留。如果以上两种方法都无法找到充足的大页进行预留，那么函数在 503 行调用 hugepage_subpool_put_pages() 函数是否从 subpool 中分配的大页，并跳转到 out_err 处。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001052.png)

函数已经从大页池子中预留足够数量的大页之后，函数需要将共享映射预留大页同步到 resv_map 中，因此函数在 518 行检测到 VMA 不存在或者 VMA 是共享映射时，函数进入 529 分支继续支持，这里 VMA 不存在的情况是 IPC 进程使用 hugetlb 大页进行共享通信的时候。函数首先在 519 行调用 region_add() 函数将 from 到 to 的新预留区域更新到 resv_map 中。如果此时发现 chg 大于 add，那么表示预留失败，可能此时某个进程将已经预留的区域取消了预留，那么函数将在 531 行调用 hugepage_subpool_put_pages() 将 subpool 已经预留的大页释放，另外调用 hugetlb_acct_memory() 函数重新更新系统大页池子中预留大页的情况。函数最后返回 0. 如果函数执行过程中跳转到 out_err 处执行，那么函数首先判断是否为非私有映射，如果是函数发现 chg 大于 0，那么说明已经预留新的大页，那么函数直接调用 region_abort() 函数停止预留，另外如果函数在 542 行判断了 VMA 映射是私有映射，那么函数调用 kref_put() 函数减少 resv_map refs 的引用计数。

> [is_vma_resv_set](#D02068)
>
> [hugetlb_acct_memory](#D02044)
>
> [hugepage_subpool_put_pages](#D02073)
>
> [hugepage_subpool_get_pages](#D02072)
>
> [region_add](#D02061)
>
> [region_chg](#D02060)
>
> [region_abort](#D02063)
>
> [set_vma_resv_map](#D02065)
>
> [set_vma_resv_flags](#D02067)
>
> [resv_map_alloc](#D02053)
>
> [inode_resv_map](#D02059)
>
> [subpool_inode](#D02058)
>
> [hstate_inode](#D02034)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02075">hugetlb_file_setup</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001053.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存。进程想分配大页内存就需要在 hugetlbfs 文件系统的挂载点下创建或打开文件，然后从进程地址空间分配一段虚拟内存用于映射文件的 page cache，这里的 page cache 即大页内存。进程在分配这段虚拟内存之后，内核会为这段虚拟内存预留指定数量的大页内存，这里的预留并不是真正的建立页表。当进程访问这段虚拟内存时触发缺页中断，缺页中断处理函数将虚拟内存映射到之前预留的大页内存上，待缺页中断返回之后，进程就可以真正使用大页物理内存。hugetlb_file_setup() 函数的作用是为进程从 hugetlbfs 文件系统的挂载点下创建一个文件用于映射大页物理内存，其底层逻辑是: name 参数指明文件的名字，size 指明文件的长度，accflag 创建文件的标志。内核使用 hstates[] 数组维护不同长度的大页，每种长度的大页使用 strut hstate 数据结构进行维护，内核在启动过程中会为每种长度的大页创建一个 hugetlbfs 文件系统挂载点，用于匿名映射时使用，这些 mount 点的顺序与 hstaes[] 数组中的顺序一致，并存储在 hugetlbfs_vfsmount[] 数组中。函数在 336 行通过调用 get_hstate_idx() 函数获得 page_size_log 对应长度的 mount 点的索引，并存储在 hstate_idx 里，函数接着在 341 行结合 hstate_idx 从 hugetlbfs_vfsmount[] 数组中获得指定长度的 hugetlbfs 文件系统挂载点，并使用 mnt 指向该挂载点。345-356 行历史遗留的代码不做过多分析. 函数在 358 行将 file 变量初始化为 -ENOSPC, 接着调用 hugetlbfs_get_inode() 函数从 mnt 对应的 mount 点下分配一个 struct inode。函数在 362 行如果检查到 create_flags 中包含 HUGETLB_SHMFS_INODE 标志，那么函数将 S_PRIVATE 添加到 inode 的 i_flags 中，不过这种情况已经不存在。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001054.png)

函数在 365 行将 inode 的 i_size 设置为 size，那么打开文件的长度就是 size。函数接着调用 clear_nlink() 移除 inode 的链接相关的信息。函数接着调用 hugetlb_reserve_page() 函数从系统大页池子中预留指定数量的大页。如果预留成功，那么函数接着调用 alloc_file_pseudo() 函数创建一个虚拟文件，并将文件的操作函数设置为 hugetlbfs_file_operations. 最后如果文件创建成功之后，文件节点页预留了足够数量的大页，那么函数将返回打开的文件 file.

> [hugetlb_reserve_pages](D02074)
>
> [hugetlbfs_get_inode](#D02054)
>
> [get_hstate_idx](#D02071)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02076">hugetlb_vm_op_fault</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001055.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，进程可以通过在 hugetlbfs 文件系统的挂载点下创建或打开文件，然后在进程的地址空间分配一段虚拟内存用于映射大页内存，内核会为这段虚拟内存预留指定数量的大页。当进程真正访问这段内存时会发生缺页，缺页中断会将这段虚拟内存映射到之前预留的大页内存上，这样进程就可以真正使用大页内存。进程的这段虚拟内存使用 VMA 进行维护，内核为这段虚拟内存提供了 vm_operations_struct 操作接口, 默认情况下 VMA 都会绑定一个 hugetlb_vm_ops, hugetlb_vm_op_fault() 函数的作用是为 VMA 提供私有的 fault 接口，其底层逻辑为: 由于使用大页进程在缺页时都使用公共的缺页路径，如果该函数被调用，那么函数会调用 BUG() 函数进行报错.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02077">hugetlb_vm_op_open</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001056.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，进程可以通过在 hugetlbfs 文件系统的挂载点下创建或打开文件，然后在进程的地址空间分配一段虚拟内存用于映射大页内存，内核会为这段虚拟内存预留指定数量的大页。当进程真正访问这段内存时会发生缺页，缺页中断会将这段虚拟内存映射到之前预留的大页内存上，这样进程就可以真正使用大页内存。进程的这段虚拟内存使用 VMA 进行维护，内核为这段虚拟内存提供了 vm_operations_struct 操作接口, 默认情况下 VMA 都会绑定一个 hugetlb_vm_ops, hugetlb_vm_op_open() 函数的作用是当 VMA 在 fork() 时创建的子进程 VMA 会调用该接口，用于进行初始化设置，其底层逻辑是: 函数调用 vma_resv_map() 函数获得 VMA 对应的 resv_map，函数接着判断如果 resv_map 存在，并且调用 is_vma_resv_set() 函数判断该 VMA 是私有映射的 VMA，并且是父进程，那么函数将 resv_map 的引用计数加一. 

> [vma_resv_map](#D02066)
>
> [is_vma_resv_set](#D02068)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02078">vma_hugecache_offset</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001057.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式向用户空间提供大页内存，大页以文件 page cache 的方式与进程的虚拟内存进行映射。vma_hugecache_offset() 函数的作用是将虚拟地址转换成 page cache 的偏移，其底层逻辑是: 参数 h 指向指定长度的大页，参数 vma 指向进程的 VMA，参数 address 指向需要转换的虚拟地址。函数首先将 address 减去 VMA 的 vm_start, 以此计算出 address 在 VMA 中的偏移，然后将该偏移向右移动 huge_page_shift() 位，这里与具体的大页长度有关。另外 VMA 的 vm_pgoff 表示该 VMA 在文件 page cache 中的偏移，那么将该偏移右移动 huge_page_order() 位之后，再加上 address 的偏移就可以获得 address 在文件的 page cache 偏移了。

> [huge_page_shift](#D02020)
>
> [huge_page_order](#D02009)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02079">linear_hugepage_index</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001058.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件的形式向用户空间提供大页内存。由于使用文件的缘故，大页在文件中以 huge page cache 的方式向用户空间提供内存。linear_hugepage_index() 函数用于将虚拟地址装换成映射大页内存文件的偏移，其底层逻辑是: 参数 vma 指向进程的 VMA，参数 address 指向需要转换的虚拟地址。进程在使用大页内存之前需要在 hugetlbfs 文件系统挂载点下创建或打开一个文件，然后从进程的地址空间分配一段虚拟内存，并建立这段虚拟内存到大页的映射关系，这里的映射关系并不一定包括真正的页表建立。大页以文件 huge page cache 的方式提供，那么每个大页在文件中都一个按大页粒度的偏移，linear_hugepage_index() 函数的作用就是获得虚拟地址映射大页的文件偏移，由于进程可以映射文件某一段，因此 VMA 的 pgoff 指明了其映射了文件的起始偏移，然后函数通过调用 hstate_vma() 获得 VMA 对应指定长度的 hstate, 函数接着调用 vma_hugecache_offset() 计算 address 相对于 VMA 的 vm_start 的偏移，然后再加上 VMA 的 pgoff 就可以获得 address 在文件中的偏移.

> [hstate_vma](#D02080)
>
> [vma_hugecache_offset](#D02078)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02080">hstate_vma</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，那么内核首先挂载一个 hugetlbfs 的文件系统，每个挂载的 hugetlbfs 文件都对应一个 struct super_block 数据结构，该结构中包含了用于特定描述 hugetlbfs 的 struct hugetlbfs_sb_info, 该结构中包含了描述指定长度大页 struct hstate. 另外进程为了能够从 hugetlbfs 文件系统中分配到大页内存，那么需要在 hugetlbfs 文件系统的挂载点下创建或打开文件，然后从进程的地址空间分配一段虚拟内存用于映射大页内存，那么就会存在一条链路，每个打开的文件 struct file 都会对应唯一的 struct inode, struct inode 中的 i_sb 指向了 hugetlbfs 文件系统挂载点的 struct super_block. 由了上面的链路都可以通过其中一个环节最终找到描述指定长度的 struct hstate.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001059.png)

hstate_vma() 函数的作用就是通过 VMA 获得 struct hstate，其底层逻辑是: 由于进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个文件，然后从地址空间分配一段虚拟内存映射文件，大页以文件 huge page cache 的方式映射到这段虚拟内存，那么进程这段虚拟内存对应一个 VMA，其 vm_file 指向打开的大页文件，那么就找到对应的 struct file，接着顺着上上图的逻辑找到最终的 struct hstate.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02081">hugetlb_vm_op_close</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001060.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的形式为用户空间提供大页内存。进程为了使用大页，需要在 hugetlbfs 文件系统挂载点下创建或打开一个文件，然后从进程地址空间分配一段虚拟内存用于映射该文件，当进程真正访问这段虚拟内存的时候，内核通过缺页中断从大页池子分配大页，然后将虚拟内存映射到大页物理内存上，那么进程就可以真正使用大页内存。进程的这段虚拟内存通过 VMA 进行描述，内核会为这段虚拟内存提供 hugetlb_vm_ops 作为操作 VMA 时的特定接口函数。hugetlb_vm_op_close() 函数作为 hugetlb_vm_ops 接口之一，用于销毁 VMA 时用于处理大页内存的接口，其底层逻辑是: 函数首先在 13 行调用 hstate_vma() 获得 VMA 对应的 struct hstate，该结构维护了指定长度的大页池子. 函数接着在 14 行通过 vma_resv_map() 函数获得 VMA 对应的 resv_map 预留结构，15 行通过 subpool_vma() 获得挂载点的 subpool。函数在 19 行进行检测，如果 resv 不存在或者 is_vma_resv_set() 返回 false，即 VMA 不是私有映射，那么对于共享映射函数不过过多操作，直接返回; 反之 resv_map 存在且 VMA 为私有映射，那么函数在 22-23 行通过 vma_hugecache_offset() 函数获得 VMA 描述的区间在 file 中的偏移区间。函数在 25 行首先调用 region_count() 函数计算 start 到 end 中已经预留的长度，然后再计算 VMA 的 start 到 end 之间的长度，两者相减既可以知道 VMA 中真实有多少预留大页，如果 reserve 不为 0，那么其可以大于 0 或者小于 0，如果大于 0，那么表示 VMA 中只有部分区域预留，那么函数需要将预留的大页内存进行释放，函数进入 30 行分支进行执行，函数在 34 行调用 hugepage_subpool_put_pages() 函数向 hugetlbfs mount 点的 subpool 中释 reserve 个大页，然后调用 hugetlb_acct_memory() 函数将 h 对应的大页池子减少 gbl_reserve 个大页。

> [hstate_vma](#D02080)
>
> [vma_resv_map](#D02066)
>
> [subpool_vma](#D02082)
>
> [is_vma_resv_set](#D02068)
>
> [vma_hugecache_offset](#D02078)
>
> [region_count](#D02083)
>
> [hugepage_subpool_put_pages](#D02073)
>
> [hugetlb_acct_memory](#D02044)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02082">subpool_vma</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件形式为用户空间提供大页内存，那么内核首先挂载一个 hugetlbfs 的文件系统，每个挂载的 hugetlbfs 文件都对应一个 struct super_block 数据结构，该结构中包含了用于特定描述 hugetlbfs 的 struct hugetlbfs_sb_info, 该结构中包含了描述指定长度大页 struct hstate. 另外进程为了能够从 hugetlbfs 文件系统中分配到大页内存，那么需要在 hugetlbfs 文件系统的挂载点下创建或打开文件，然后从进程的地址空间分配一段虚拟内存用于映射大页内存，那么就会存在一条链路，每个打开的文件 struct file 都会对应唯一的 struct inode, struct inode 中的 i_sb 指向了 hugetlbfs 文件系统挂载点的 struct super_block. 由了上面的链路都可以通过其中一个环节最终找到 subpool.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001061.png)

subpool_vma() 函数的作用就是通过 VMA 找到其对应的 hugetlbfs 文件系统挂载点 subpool 小池子，其底层逻辑是: 函数首先通过 VMA 的 vm_file 找打对应的 struct file, 然后通过 file_inode() 函数将 struct file 转换成对应的 struct inode, 接着函数调用 subpool_inode() 函数在上上图的链路中通过 inode 找到最终的 subpool。

> [subpool_inode](#D02058)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02083">region_count</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001062.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 方式向用户空间提供大页内存。进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个文件，然后将文件映射到进程的地址空间，在映射的过程中内核会预留指定数量的大页，以便进程访问这段虚拟内存时，系统将发生缺页中断，然后缺页中断将虚拟内存映射到预留的大页内存上，这样进程就行正常使用大页内存。内核使用 resv_map 机制来管理一个大页文件的预留情况，由于文件可以通过私有映射或者共享映射的方式映射到进程的地址空间，因此 resv_map 的预留机制将不同。region_count() 函数用于统计 resv_map 的某段区域预留大页的数量，其底层逻辑为: 函数首先找到 resv_map 的预留区域链表，然后使用 list_for_each_entry() 函数遍历该链表上的区域，如果参数 f 到 t 的区域有相交的情况，那么函数将从遍历区间的起始值和 f 中找到最大的，然后从遍历区间的结束值和 t 中找到最小的，以此找到一个最小重叠区域，该区域的长度通过 seg_to 减去 seg_from 获得，并存储在 chg 变量里，函数遍历完毕之后的 chg 正好代表了 from 到 to 中已经预留的大页数量.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02084">hugetlb_vm_op_pagesize</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001063.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存，进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个文件，然后进程从地址空间分配一段虚拟内存映射大页文件，这样进程的虚拟内存就与大页内存建立了联系，此时系统会为这段虚拟内存预留指定数量的大页，当进程访问这段虚拟内存时，由于没有建立虚拟内存到大页的页表，因此会触发缺页中断，缺页中断程序会将虚拟内存映射到之前预留的大页物理内存上，待缺页中断返回之后，进程就可以使用大页内存。进程映射的虚拟内存通过 VMA 进行维护，内核会为这段 VMA 提供相应的操作接口 hugetlb_vm_ops，hugetlb_vm_op_pagesize() 函数作为 huegtlb 为 VMA 的 pagesize 接口，用于获得当前 VMA 映射大页的长度，系统不仅可以支持 2 MiB 大页，也可以支持 1Gig 大页，因此 VMA 可以通过 pagesize 接口获得其映射大页的长度。hugetlb_vm_op_pagesize() 函数的底层逻辑为: 函数首先通过 hstate_vma() 函数获得 VMA 对应的 struct hstate 数据结构，然后通过 huge_page_shift() 函数从 hstate 数据结构中大页占用的 order 数，最后将其转换成实际大页长度.

> [hstate_vma](#D02080)
>
> [huge_page_shift](#D02020)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02085">hugetlbfs_file_mmap</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001064.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程首先需要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间分配一段内存映射大页文件，此时系统可能会为这段虚拟内存预留指定数量的大页，当进程开始访问这段虚拟内存时，由于这段虚拟内存没有与大页建立页表，那么会触发缺页中断，缺页处理程序会将虚拟内存映射到预留的大页物理内存上，待缺页中断返回之后进程就可以正常通过这段虚拟内存访问大页物理内存。hugetlbfs_file_mmap() 函数的作用是当进程分配一段虚拟内存映射大页文件时提供 hugetlbfs 的 mmap 相关的操作，包括在映射阶段为虚拟内存预留大页以及定制化设置，其底层逻辑是: 函数首先在 123 行通过 file_inode() 函数获得 file 参数对应的 struct inode，函数接着在 126 行调用 hstate_file() 函数获得参数 file 对应的 struct hstate. 在映射 hugetlb 大页时，函数在 136 行将 VM_HUGETLB 和 VM_DONTEXPAND 标志添加到 VMA 的标记集合，其中 VM_HUGETLB 标志指明 VMA 映射了 hugetlb 的大页，另外 VM_DONTEXPAND 标志指明该 VMA 区域不能像堆栈一样动态增长。函数接着在 137 行为 VMA 设置了 VM 接口 hugetlb_vm_ops，该接口集合包括了 fault、pagesize、close 等 VMA 操作接口。函数接着在 145 行对 VMA 的 vm_pgoff 进行检查，以便其不要越界。函数同时在 151 行对 vm_pgoff 进行对齐检查，如果没有按 huge page 对齐的话，那么函数直接返回 -EINVAL. 函数在 154 行计算了 VMA 的长度，然后将 VMA 的 pgoff 向左移动 PAGE_SHIFT, 然后将其加上 VMA 的长度，可以获得其在 file 对应的结尾处。如果此时发现 len 小于 vma_len, 那么代表 VMA 越界，是一种错误的情况，那么函数直接返回 -EINVAL.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001065.png)

函数接着在 160 行调用 inode_lock() 对 inode 进行上锁，这里上锁是为了放置多个进程同时打开一个大页文件产生竞态，接着函数调用 file_accessed() 函数增加文件的引用计数。接下来函数在 164 行调用 hugetlb_reserve_pages() 函数为这段虚拟内存预留指定数量的大页，这部分大页一旦预留就不再是空闲可用的大页，只能供预留它的进程使用。当大页预留完之后，函数判断 VMA 是否可写，并且发现文件的长度小于 len，那么函数调用 i_size_write() 函数修改文件的长度，因此可以说明大页文件可以动态增加长度。函数最后调用 inode_unlock() 函数解除 inode 的锁。至此进程完成了映射大页文件的任务.

> [hstate_file](#D02035)
>
> [hugetlb_reserve_pages](#D02074)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02086">huge_pte_offset</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001066.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存，用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个文件，然后从进程的地址空间分配一段虚拟内存用于映射大页文件，内核此时会为这段虚拟内存预留指定数量的大页内存，当进程真正的访问这段虚拟内存时，由于虚拟内存还没有与大页物理内存建立页表，那么触发系统缺页中断，缺页中断处理函数接着建立虚拟内存到之前预留大页物理内存的页表，待缺页中断返回之后，进程可以真正使用大页物理内存。huge_pte_offset() 函数的作用是遍历映射大页虚拟内存的页表，以此找到最后一级页表的 Entry，其底层逻辑是: 参数 mm 指向进程的地址空间，参数 addr 指向需要遍历页表的虚拟地址，参数 sz 则指明虚拟内存的长度。Linux 系统默认支持 5 级页表，分别是 PGD、P4D、PUD、PMD 和 PTE，对于大页来说只需要 4 级页表(2MiB 大页)或 3 级页表(1Gig 大页)。函数从 PGD 页表开始遍历，函数首先在 801 行调用 pgd_offset() 函数获得 PGD Entry，如果此时 pgd_present() 返回空，那么表示 PGD entry 不存在，不符合预期，那么函数直接返回 NULL; 反之函数继续在 804 行调用 p4d_offset() 函数基于 PGD Entry 查找 P4D Entry, 同理如果调用 p4d_offset() 函数发现 P4D Entry 不存在，那么不符合预期直接返回 NULL; 反之函数继续在 808 行调用 pud_offset() 函数基于 P4D 查找 PUD Entry，函数此时调用 pud_none() 检查 PUD Entry, 如果 sz 参数不等于 PUD_SIZE，即大页的长度不是 1Gig 的情况下，PUD Entry 是空的，那么这是一种不符合预期的，因为对应映射 2MiB 的情况 PUD Entry 一定不为空，那么函数直接返回 NULL; 反之函数继续调用 pud_huge() 函数检查 PUD Entry, 如果此时 PUD Entry 的 PSE 标志位置位，那么大页是映射 1Gig 的页表，另外如果 PUD Entry 不存在，那么函数认为这两种情况只要满足其中一种就是映射 1Gig 的大页，那么函数直接返回 PUD Entry 即可; 反之映射 2MiB 的大页，那么函数在 815 行调用 pmd_offset() 函数获得 PMD Entry，如果此时 sz 不等于 PMD_SIZE 且 PMD Entry 为空，那么这是一种错误的配置，那么函数直接返回 NULL。最后函数调用 pmd_huge() 检查到 PMD Entry 的 PSE 置位，或者调用 pmd_present() 函数发现 PMD Entry 不存在，那么函数找到 2MiB 大页的最后一级页表，函数返回 PMD Entry. 如果以上都不符合，那么没有找到大页的最后一级页表，函数直接返回 NULL.

> [pud_huge](#D02087)
>
> [pmd_huge](D02088)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02087">pud_huge</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001067.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程从地址空间分配一段虚拟内存建立页表映射到大页内存上，以此可以直接使用大页内存。pud_huge() 函数的作用是判断大页是否为 1Gig 大页，其底层逻辑为: 在 Linux 内核中使用 5 级页表，包括 PGD、P4D、PUD、PMD 以及 PTE，对于映射 1Gig 的大页只需要使用 PGD、P4D 和 PUD 三级页表，如果映射 1Gig 的大页，那么 PUD Entry 的 PSE 标志会置位，那么 pud_huge() 函数通过判断 PUD Entry 的 PSE 标志位是否置位来判断页表是否映射 1Gig 大页。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02088">pmd_huge</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001068.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程通过分配一段虚拟内存并建立页表映射到大页物理内存上，以此来访问大页内存。pmd_huge() 函数的作用是判断大页视为 2MiB 的大页，其底层逻辑是: Linux 支持 5 级页表，分别是 PUD、P4D、PUD、PMD 以及 PTE，对于 2MiB 页表其最后一级页表是 PMD, 并且 PMD Entry 的 PSE 标志位置位，那么函数通过判断 PMD Entry 存在且 PSE 标志位是否置位来判断是否映射 2MiB 大页.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02089">huge_ptep_get</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001069.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程通过从地址空间分配一段虚拟内存并建立页表映射到大页物理内存上，以此访问大页物理内存。huge_ptep_get() 函数的作用是获得大页最后以及页表的内容，其底层逻辑是: 参数 ptep 指向最后一级页表的 Entry，对于映射 1Gig 大页，Entry 指向 PUD Entry。对于映射 2MiB 大页，Entry 指向 PMD Entry. 函数直接返回 Entry 的值。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02090">huge_ptep_set_access_flags</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001070.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页物理内存。进程通过从地址空间分配一段虚拟内存并建立页表映射到大页物理内存上，以此访问大页物理内存。huge_ptep_set_access_flags() 函数的作用是将大页页表最后一级页表的 Access 标志位置位，其底层逻辑是: 对于 1Gig 的大页，最后一级页表是 PUD, 而对于映射 2MiB 的大页，最后一级页表是 PMD. 无论是那种页表，函数都会调用 ptep_set_access_flags() 函数将最后一级页表的 Access 标志位置位，以此表示该大页已经被访问过。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02091">huge_ptep_set_wrprotect</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001071.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。进程通过从地址空间分配一段虚拟内存并建立页表映射到大页的物理内存上，以此访问大页物理内存。huge_ptep_set_wrprotect() 函数的作用是将映射大页物理内存的最后一级页表设置为写保护，其底层逻辑为: 所谓页表的写保护就是将页表的 Write 权限清除，在清除之后如果进程对该内存进行读操作没有影响，但是进行写操作将触发写缺页，这个功能用户辅助 COW 机制。参数 ptep 为指向映射大页物理内存的最后一级页表，函数通过调用 ptep_set_wrprotect() 函数将页表的 Write 权限清除.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02092">huge_page_mask</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001072.png)

在 hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页池子，其成员 mask 作为大页的掩码，用于掩住大页的低位，即将大页地址与掩码相与之后，大页内的偏移全部为 0。如果一个大页是 2Mib，那么 mask 将掩住大页的页内偏移，那么其值为 0xffffffffffe00000, 即将 [0:20] 区域全部掩住; 如果一个大页是 1Gig，那么 mask 将 [0:29] 区域全部掩住，即 0xffffffffC000000.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02093">prepare_hugepage_range</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001073.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程在 hugetlbfs 文件系统挂载点下创建或打开一个文件，然后通过从进程地址空间分配一段虚拟空间，并将虚拟内存建立页表映射到文件的 page cache 上，以此达到使用大页的目的。prepare_hugepage_range() 函数用于检测一段虚拟内存的合法性，其底层逻辑是: 参数对应进程在 hugetlbfs 文件系统挂载点下打开的文件，参数 addr 和 len 指明了虚拟内存的范围。函数首先通过 hstate_file() 函数将 file 转换成维护指定长度大页池子的 struct hstate 数据结构，函数调用 huge_page_mask() 函数获得大页的掩码，这里将其取反并与 len 相与，以此检查 len 是否按大页的长度进行对齐，如果没有那么函数认为 len 是一个不合法的值，直接返回 -EINVAL。同理函数检查这段虚拟内存的起始地址 addr 是否合法，如果都合法则返回 0.

> [hstate_file](#D02035)
>
> [huge_page_mask](#D02092)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02094">huge_pte_wrprotect</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001074.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_wrprotect() 函数用于检查大页的最后一级页表是否设置了写保护，所谓写保护就是将页表的 Write 权限清零，如果进程继续读页表对应的内存，那么不会有变化，但是如果进程向页表对应的内存进行写操作，那么会触发系统缺页中断。函数通过 pte_wrprotect() 函数进行实际的检测.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02095">mk_huge_pte</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001075.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。mk_huge_pte() 函数的作用是制作大页最后一级页表，其底层逻辑是: 参数 page 指向大页物理内存对应的 struct page，参数 pgprot 为页表的内容。由于大页支持 2Mib 或者 1Gig 的粒度，那么大页最后一级页表可能是 PUD 或者 PMD，那么函数调用 mk_pte() 函数进行实际的页表内容制作。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02096">huge_pte_write</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001076.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_write() 函数的作用是检测大页是否具有写权限，大页的写权限通过页表的 Write 标志位描述，如果一个页表的 Write 标志位清零的情况下对大页进行写操作，将触系统报错。因此函数可用通过读取页表的 Write 标志位判断页表是否具有写权限。其底层通过 pte_write() 函数进行判断。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02097">huge_pte_dirty</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001077.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_dirty() 函数的作用是判断大页是否为脏页，所谓脏页就是被写过内容的大页，硬件会自动检测当大页被写入后会将页表的 Dirty 位自动置位，那么函数 huge_pte_dirty() 判断大页是否为脏页的逻辑就是通过判断对应页表的 Dirty 位是否置位，如果置位那么大页是脏页，否则大页不是脏页.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02098">huge_pte_mkwrite</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001078.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_mkwrite() 函数的作用是让大页的页表具有写权限，页表中 Write 标志为用于记录大页是否具有写权限，当该标志位清零的情况下不具有写权限，这个时候如果对页表进行写操作，那么会触发系统错误。因此为了让大页具有写权限，可以通过 huge_pte_write() 函数将页表的 Write 标志位置位即可。函数通过 pte_mkwrite() 函数实现。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02099">huge_pte_mkdirty</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001079.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_mkdirty() 的作用是将大页标记为脏页。软件层面的脏页就是将页表 Dirty 标志位置位的大页，硬件上的脏页则是被写入内容的页，两者其实是一致的，因为当一个干净的页其页表的 Dirty 位是清零的，当向干净的页写入内容之后，硬件会自动将 Dirty 标志位置位，那么可以统一认为这个页是脏页。软件上可以调用 huge_pte_mkdirty() 函数将一个大页标记为脏页，其通过将大页对应页表的 Dirty 标志位置位.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02100">huge_pte_clear</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001080.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_clear() 函数用于将页表内容清零，参数 mm 对应进程的地址空间，参数 ptep 对应具体的页表。函数通过调用 pte_clear() 函数将 ptep 对应页表内容都清零。该函数经常用在进程释放大页的情况。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02101">huge_pte_modify</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001081.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_modify() 函数用于将大页页表添加新的属性集合，其底层逻辑是: pte 参数为原始页表，参数 newprot 则为新的页表属性集合，函数通过调用 pte_modify() 函数将新属性添加到页表中，并返回新的页表.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02102">set_huge_pte_at</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001082.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。set_huge_pte_at() 函数用于设置大页最后一级页表，由于 Linux 支持五级页表，另外大页支持 2MiB 和 1Gig 大页, 因此大页的最后一级页表可能是 PUD, 也可能是 PMD. 无论是 PMD 或者是 PUD, set_huge_pte_at() 都可以将页表设置为指定的值，其底层逻辑通过调用 set_pte_at() 函数实现。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02103">huge_ptep_get_and_clear</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001083.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_ptep_get_and_clear() 函数的作用是获得当前大页最后一级页表的内容，并将大页页表清零，函数的底层逻辑通过调用 ptep_get_and_clear() 函数实现。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02104">huge_pte_none</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001084.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程需要要在 hugetlbfs 文件系统的挂载点下创建或打开一个文件，然后从进程的地址空间中分配一段虚拟内存用映射文件，并在发生缺页的时候建立虚拟内存到文件 page cache 的页表，以此访问大页。huge_pte_none() 函数的作用是判断大页的页表是否为全新的，即页表没有被访问过，其底层逻辑通过函数 pte_none() 函数实现.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02105">\_\_vma_reservation_common</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001085.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。当进程在 hugetlbfs 文件系统挂载点下打开或创建大页文件之后，并从进程地址空间分配一段虚拟内存用于映射大页文件，此时还没有建立虚拟内存到大页物理内存的页表映射，但内核会为这段虚拟内存预留足够的大页。内核使用 resv_map 机制管理大页的预留，通过该机制可以有效处理共享大页和私有大页预留的问题。\_\_vma_reservation_common() 函数的作用是根据需求处理进程对 resv_map 的请求，其底层逻辑为: resv_map 支持的请求包括: VMA_NEEDS_RESV 进程的虚拟区域需要预留大页、VMA_COMMIT_RESV 确定之前的大页预留数量并对 resv_map 进行实际的调整、VMA_END_RESV 结束对 resv_map 的调整，一般用于突然停止大页的预留、VMA_ADD_RESV 新预留一个大页。函数对应的参数 h 用于描述指定长度大页的 struct hstate, 参数 vma 用于描述进程的 VMA 虚拟区域，参数 addr 表示进程映射大页的虚拟地址，参数 mode 则表示对 resv_map 的请求。函数首先在 870 行通过调用 vma_resv_map() 函数获得进程的 resv_map, 如果此时没有找 resv_map, 那么函数直接返回 1. 函数接着在 874 行调用 vma_hugecache_offset() 函数获得 addr 虚拟地址在文件中的偏移 idx。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001086.png)

函数开始在 875 行对 resv_map 的需求进行分流，如果 mode 为 VMA_NEEDS_RESV, 那么函数进入 877 行分支执行，此时函数将调用 region_chg() 函数告诉 resv_map() 需要在 idx 处预留一个大页，region_chg() 函数将预留的情况返回存储到 ret 之后 break，这里值得注意的是，如果 VMA 对应是共享映射，那么 idx 处的大页可能已经预留，那么 resv_map 不会申请新的预留; 如果 mode 为 VMA_COMMIT_RESV, 那么函数进入 879 行分支执行，此时函数调用 region_add() 函数将 idx 处预留一个大页，并更新到 resv_map 里，这里确保了系统新预留一个大页, 并将预留大页的数量存储在 ret 里并 break; 如果 mode 为 VMA_END_RESV, 那么函数进入 883 行分支进行处理，进入该分支意味着内核将暂停当前的大页内存分配，region_abort() 函数暂停对 resv_map 的变更，此时函数将 ret 设置为 0 并直接 break; 如果 mode 为 VMA_ADD_RESV, 那么函数进入 887 分支，该分支表示直接向 resv_map 中新增一个大页预留，函数此时在 887 行检查 VMA 是否为共享映射，如果是，那么函数调用 region_add() 函数在 idx 处预留一个大页，如果 idx 处没有预留过大页，那么 resv_map 新预留一个大页，反之则不用做预留。另外如果 VMA 是私有映射，那么函数会停止对 resv_map 的变更，然后调用 region_del() 函数在 idx 处移除一个预留的大页; 如果 mode 都不是上面的情况，那么 mode 就是一个非法值，函数直接调用 BUG() 进行报错. 函数如果在 898 行检查到 VMA 是共享映射，那么函数直接返回 ret; 反之如果函数检测到 VMA 是私有映射，且 resv_map 中包含 HPAGE_RESV_OWNER, 其 ret 不小于 0，那么如果此时 ret 为真，那么函数返回 0，否则返回 1; 反之返回将根据 ret 的值进行返回.

> [vma_resv_map](#D02066)
>
> [vma_hugecache_offset](#D02078)
>
> [region_chg](#D02060)
>
> [region_add](#D02061)
>
> [region_abort](#D02063)
>
> [is_vma_resv_set](#D02068)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02106">vma_needs_reservation</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001087.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程在 hugetlbfs 挂载点下创建或打开大页文件，然后从进程的地址空间分配一段虚拟内存映射大页文件，此时内核会为这段虚拟内存提前预留指定数量的大页。为了更好的管理预留的大页，内核使用 resv_map 机制管理进程大页的预留。vma_needs_reservation() 函数的作用告诉 resv_map 需要预留在指定区域预留一个大页，其底层逻辑是: 当进程需要预留大页时，进程首先找到其对应的 resv_map 数据结构，然后向其申请需要预留的区域，该区域可能已经存在 resv_map 中，或者该区域还没有在 resv_map 中注册，那么进程会调用 \_\_vma_reservation_common() 函数通知 resv_map VMA_NEEDS_RESV 信号，这样 resv_map 会检查需要添加的区域是否预留，如果预留那么直接返回预留的数量; 反之不需要预留那么直接返回 0.

> [\_\_vma_reservation_common](#D02105)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02107">vma_commit_reservation</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001088.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程在 hugetlbfs 挂载点下创建或打开大页文件，然后从进程的地址空间分配一段虚拟内存映射大页文件，此时内核会为这段虚拟内存提前预留指定数量的大页。为了更好的管理预留的大页，内核使用 resv_map 机制管理进程大页的预留。vma_commit_reservation() 函数的作用是 resv_map 提交并确认在指定区域预留一个大页，其底层逻辑是: resv_map 中新增一个大页之前需要提前确认指定区域是否已经预留大页，其通过向 resv_map 传递 VMA_NEEDS_RESV 信号，如果需要新增预留大页，那么进程接着向 resv_map 传递 VMA_COMMINT_RESV 确认新增一个预留大页，函数通过 \_\_vma_reservation_common() 函数实现.

> [\_\_vma_reservation_common](#D02105)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02108">vma_end_reservation</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001089.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程在 hugetlbfs 挂载点下创建或打开大页文件，然后从进程的地址空间分配一段虚拟内存映射大页文件，此时内核会为这段虚拟内存提前预留指定数量的大页。为了更好的管理预留的大页，内核使用 resv_map 机制管理进程大页的预留。vma_end_reservation() 函数的作用是终止向 resv_map 中预留大页，其底层逻辑是: 当一个进程映射大页的时候需要向 resv_map 中预留大页，但由于某些原因导致无法最终添加预留大页，那么函数向 resv_map 发送 VMA_END_RESV 信息终止预留大页的提交。

> [\_\_vma_reservation_common](#D02105)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02109">vma_add_reservation</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001090.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户进程提供大页内存。进程在 hugetlbfs 挂载点下创建或打开大页文件，然后从进程的地址空间分配一段虚拟内存映射大页文件，此时内核会为这段虚拟内存提前预留指定数量的大页。为了更好的管理预留的大页，内核使用 resv_map 机制管理进程大页的预留。vma_add_reservation() 函数的作用是向 resv_map 指定区域添加新的大页预留，其底层逻辑是: 函数通过调用 \_\_vma_reservation_common() 函数向 resv_map 发送 VMA_ADD_RESV 信号，以此向 resv_map 的指定区域添加预留大页。

> [\_\_vma_reservation_common](#D02105)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02110">dequeue_huge_page_nodemask</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001091.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 方式向用户进程提供大页内存。内核支持 2MiB 和 1Gig 粒度的大页，这些大页来自于 buddy 的复合页，因此当系统需要新分配大页时，其从 buddy 中找到指定的长度的复合页之后包装成大页，然后维护在指定的 struct hstate 数据结构里，待内核其他子系统进行使用。dequeue_huge_page_nodemask() 函数的作用是从维护指定大页的 struct hstate 数据结构中分配一个大页，其底层逻辑是: 由于在支持多 NUMA NODE 的架构中，CPU 需要从亲和力最大的 NUMA NODE 上分配内存，因此维护大页的 struct hstate 数据结构也维护了多个链表，每个链表对应 NODE 上的大页信息。函数的整体逻辑是确认一个可用的 NUMA NODE，然后从指定 struct hstate 的 NODE 链表上分配大页。函数首先在 892 行调用 node_zonelist() 函数获得 nid 对应的 NODE 上的 Zone list 信息，然后在 895 行调用 read_mems_allowed_begin() 开始进行大页的分配，这里函数调用了 for_each_zone_zonelist_nodemask() 遍历 zonelist 上的 ZONE，直到分配到大页为止。函数在遍历 NODE 上的每个 ZONE 时，函数首先调用 cpuset_zone_allowed() 函数检查当前 ZONE 是否支持 gfp_mask 对应的分配策略，如果不支持那么函数直接遍历下一个 ZONE; 反之函数在 905 行调用 zone_to_nid() 函数检查当前 NODE 是否与 node 一致，如果一致那么代表当前 NODE 是没有满足需求的内存，那么函数直接遍历下一个 Zone, 下一个 Zone 可能位于下一个 NODE; 反之函数在 907 行调用 zone_to_nid() 函数将当前 NODE 信息存储到 node 变量里，接着函数调用 dequeue_huge_page_node_exact() 函数从指定的 NODE 上分配大页，如果分配成功那么函数直接返回大页对应的 struct page; 反之继续遍历 ZONE。当循环结束之后，函数调用 read_mems_allowed_retry() 与之前的 read_mems_allowed_begin() 对应来结束此次失败的分配，如果 read_mems_allowed_retry() 函数没有返回 0，那么函数跳转到 retry_cpuset 处重新分配大页; 反之返回 NULL。

> [dequeue_huge_page_node_exact](#D02111)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02111">dequeue_huge_page_node_exact</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001092.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。内核支持 2MiB 和 1Gig 粒度的大页，这些大页分别使用 struct hstate 数据结构维护，当内核需要分配大页时就从对应 struct hstate 数据结构的空闲大页链表上获得一个可用的大页，反之当系统释放一个大页使，对应的 struct hstate 数据结构将该大页重新添加到可用大页链表中。dequeue_huge_page_node_exact() 函数的作用是从指定的 struct hstate 数据结构中获得一个可用的大页，其底层逻辑是: 可用大页维护在 struct hstate 数据结构的 hugepage_freelists[] 数组中，每个 NODE 对应数组中一个成员，因此每个 NODE 都维护了一条私有的 hugepage_freelists 可用大页链表。因此函数在 867 行调用 list_for_each_entry() 函数遍历指定 NODE 的 hugepage_freelists[] 链表，然后通过 PageHWPoison() 判断大页不是坏页，那么函数结束循环以此找到一个可用的大页，接着函数在 74 行判断 hugepage_freelists[nid] 的地址与 page->lru 相等，那么 hugepage_freelists[nid] 上没有空闲的大页，那么函数直接返回 NULL; 反之函数找到可用的大页之后，函数在 76 行调用 list_move() 函数将大页由原先的 hugepage_freelists[nid] 链表移动到 hugepage_activelist 链表上，以此表示该大页是一个正在使用的大页。函数接着调用 set_page_refcounted() 函数将大页的引用统计设置为 1，以此表示大页有一个使用者。接着函数将 struct hstate 的 free_huge_pages 和 free_huge_pages_node[nid] 的统计数减一，以此表示可用的大页数量减少一个。最后函数返回大页对应的 struct page 数据结构. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02112">dequeue_huge_page_vma</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001093.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。进程通过在 hugetlbfs 文件系统挂载点下创建或打开文件，然后从进程的地址空间分配一段虚拟内存映射大页文件，当进程访问这段虚拟内存的时候，由于映射大页的虚拟内存还没有建立页表映射 page cache 大页物理内存，那么触发缺页中断，缺页中断处理程序会为这段虚拟内存分配大页并建立相应的页表。dequeue_huge_page_vma() 函数的作用是为 VMA 分配大页，其底层逻辑是: 函数首先在 44 行调用 vma_has_reserves() 函数判断 VMA 是否已经预留了 chg 个大页，如果没有预留，且指定长度大页的可用大页数量等于 0，那么函数认为这是一种错误的场景，那么函数直接跳转到 err 处，这里可用大页的数量等于空闲大页减去预留大页。函数如果检查到可用大页为 0 且 avoid_reserve 为真，那么这是一种错误的情况，因为没有预留所以要检查 pool 里面是否有足够的大页，如果没有那么跳转到 err 处. 通过上面的检查之后，函数调用 htlb_alloc_mask() 函数获得大页的分配标志，接着调用 huge_node() 获得分配大页的 NUMA NODE, 最后函数调用 dequeue_huge_page_nodemask() 函数从指定的 NUMA NODE 获得可用的大页。分配完毕之后函数检查大页是否分配成功，如果分配成功且 avoid_reserve 为 0，另外 vma_has_reserves() 为真，那么认为分配是成功的，那么函数将 struct page 标记为私有页，然后将指定长度大页 struct hstate 的 resv_huge_pages 即预留大页数量减一。函数最后调用 mpol_cond_put() 函数之后直接返回 struct page。

> [vma_has_reserves](#D02070)
>
> [dequeue_huge_page_nodemask](#D02110)
>
> [htlb_alloc_mask](#D02022)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02113">alloc_buddy_huge_page_with_mpol</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001094.png)

在 hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，内核可以通过从 buddy 中分配指定数量的复合页包装成大页，并维护在大页池子里，大页池子有的可以动态增加大页的数量，有的只能固定数量的大页。alloc_buddy_huge_page_with_mpol() 函数的作用是从大页池子中分配大页，其底层逻辑是: 函数首先通过 htlb_alloc_mask() 函数获得大页分配标志，然后调用 huge_node() 从 mpol 和 nodemask 解析出需要从哪个 NUMA NODE 中分配大页内存，接着函数调用 alloc_surplus_huge_page() 函数通过超发方式动态分配一个大页，并返回大页对应的 struct page.

> [htlb_alloc_mask](#D02022)
>
> [alloc_surplus_huge_page](#D02039)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02114">alloc_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001095.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户空间提供大页内存。用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开文件的方式，从进程的地址空间分配一段虚拟内存用于映射大页文件，此时内核可以为虚拟内存预留指定数量的大页，当进程访问这段虚拟内存时，由于没有与大页建立实际的页表，因此触发缺页中断，缺页中断会从预留的大页中拿出一个大页，并将虚拟地址映射到该大页上，待缺页中断返回后，进程就可用正常访问大页内存。内核使用 struct hstate 数据结构维护指定长度的大页，其成员 hugepage_freelists[] 数组维护不同 NUMA NODE 上空闲的大页，以此构成大页池子。当系统启动的时候设置 CMDLINE 或者手动增加大页池子的大小，系统会为大页池子填充大页，大页来自 buddy 分配器的复合页，包装之后称为大页。内核另外可以通过超发的方式动态增加大页池子的数量。alloc_huge_page() 函数的作用就是为 VMA 分配一个可用的大页，并通过 avoid_reserve 控制是否从预留池子中分配大页，其底层逻辑是: 函数首先调用 subpool_vma() 函数获得 VMA 所打开文件对应的 hugetlbfs 文件挂载点的 subpool，并通过 hstate_vma() 获得 VMA 对应的 struct hstate，通过以上操作可以获得指定长度的大页池子，以及 hugetlbfs 挂载点的 subpool 小池子。函数接着在 2002 行调用 hstate_index() 函数获得 struct hstate 在系统默认 hugetlbfs 挂载点数组的索引。函数在 2008 行调用 vma_needs_reservation() 函数确认 VMA 的 addr 是否新预留大页，并将预留的大页数量存储在 map_chg 和 gbl_chg 变量里，如果此时 map_chg 小于 0，那么函数直接返回 -ENOMEM 以此表示系统没有可用的内存。函数接着在 2019 行判断发现 map_chg 不为 0，那么表示需要新增预留大页, 或者 avoid_reserve 为真表示不从预留池子中分配大页，以上两个任一条件都是需要新预留大页，那么函数进入 2020 分支执行。函数首先在 2020 行调用 hugepage_subpool_get_pages() 从 VMA 在 hugetlbfs 文件系统挂载点的 subpool 中分配大页，函数将可分配大页数量存储在 gbl_chg 中，如果此时 gbl_chg 小于 0，那么表示 subpool 没有空闲的大页，因此大页调用 vma_end_reservation() 函数结束大页预留，直接返回 -ENOSPC; 反之 subpool 中有可用的大页，此时函数继续检查 avoid_reserve 是否为 1，如果为 1 那么不使用预留的大页，那么函数需要分配新的大页，那么将 gbl_chg 设置为 1.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001096.png)

函数接着在 2048 行调用 dequeue_huge_page_vma() 函数从指定长度的大页 struct hstate 池子中分配大页，如果分配失败，那么大页池子里没有足够的大页了，那么函数进入 2050 行分支，函数通过在 2051 行调用 alloc_buddy_huge_page_with_mpol() 函数通过超发的方式从 buddy 中动态分配一个复合页作为大页，如果此时大页还是没有分配成功，那么函数跳转到 out_uncharge_cgroup 处执行，反之如果此时 avoid_reserve 为 0 且 vma_has_reserves() 为真，那么代表大页分配需要时需要更新预留大页的状态，于是函数将大页设置为私有页之后将对应 struct hstate 的 resv_huge_pages 数量减一. 函数接着将大页放到了 struct hstate 的 hugepage_activelist 链表上，以此表示大页处于使用中。如果执行完 2050 分支或大页在 2048 行已经成功分配，那么函数继续执行，此时函数在 2065 行将 struct page 的 private 成员指向了 spool, 即 hugetlbfs 文件系统挂载点的 subpool 小池子. 函数接着在 2067 行调用 vma_commit_reservation() 函数将分配的大页文件区间进行最终的预留，并将实际预留的长度存储在 map_commit 里，如果此时 map_commit 小于 map_chg, 那么表示实际预留的大页数小于需要预留大页的数量，那么函数进入 2069 分支执行，在 2069 行分支，函数可能遇到竞态的风险，那么函数在 2080 行调用 hugepage_subpool_put_pages() 函数增加 subpool 的预留大页数量，使其不低于 subpool 最小预留数，接着函数调用 hugetlb_acct_memory() 函数调整大页的内存。最后函数返回大页对应的 struct page。对于 out_subpool_put 分支，函数如果发现 map_chg 或者 avoid_reserve 为真，那么代表已经从 subpool 中分配了大页，那么调用函数 hugepage_subpool_put_pages() 函数将大页释放会 subpool 小池子中，最后函数调用 vma_end_reservation() 函数停止 resv_map 的预留操作，最后返回 ENOSPC.

> [subpool_vma](#D02082)
>
> [hstate_index](#D02012)
>
> [vma_needs_reservation](#D02106)
>
> [hugepage_subpool_get_pages](#D02072)
>
> [vma_end_reservation](#D02108)
>
> [dequeue_huge_page_vma](#D02112)
>
> [alloc_buddy_huge_page_with_mpol](#D02113)
>
> [vma_has_reserves](#D02070)
>
> [vma_commit_reservation](#D02107)
>
> [hugepage_subpool_put_pages](#D02073)
>
> [hugetlb_acct_memory](#D02044)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02115">pages_per_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001097.png)

在 hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，其成员 order 用于指定大页占用 4KiB 页的数量，并使用 order 表示 2 的幂。pages_per_huge_page() 函数用于表示指定长度的大页占用 4KiB 页的数量，其底层逻辑是: 从指定长度大页的 struct hstate 数据结构的 order 成员转换成 2^order，即 "1 << order" 可以获得该大页占用 4KiB 页的数量.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02116">clear_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001098.png)

在 hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，内核从 buddy 分配器中分配的复合页包装成大页，因此一个大页包含了多个 4KiB 页。函数 clear_huge_page() 的作用就是将大页占用的内存都清零，其底层逻辑是: 参数 page 指向需要清零的大页，参数 addr_hint 表示大页的虚拟地址，参数 pages_per_huge_page 表示大页包含 4KiB 页的数量，函数首先对 addr_hist 按 pages_per_huge_page 进行掩码，以此获得 addr_hint 内没有对齐的虚拟地址。接着函数判断大页是 1Gig 大页还是 2MiB 大页，如果是 1Gig 大页，那么函数调用 clear_gigantic_page() 函数对 1Gig 大页内存清零; 反之大页是 2MiB 大页，那么函数调用 process_huge_page() 函数进行清零，其中关键的清零函数是 clear_subpage() 函数.

> [clear_subpage,](#D02117)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02117">clear_subpage</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001099.png)

在 hugetlb 大页机制中，内核使用 struct hstate 数据结构描述指定长度的大页，每个大页从 buddy 中分配的复合页包装而来，因此大页由多个 4KiB 页构成。clear_subpage() 函数的作用是对构成大页的一个 4KiB 页进行清零操作，其底层逻辑是: 参数 arg 指向 4KiB 页，参数 addr 指向 4KiB 物理内存映射的虚拟地址，idx 则表示在复合页中的偏移。函数首先将 arg 转换成 struct page，然后调用 clear_user_highpage() 函数将 4KiB 物理页映射到 addr 对应的虚拟地址，然后进行清零，清除之后函数将 addr 到物理页的虚拟地址进行解耦.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02118">huge_add_to_page_cache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001100.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户空间提供大页内存。进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程地址空间分配一段虚拟内存映射大页文件，当进程访问这段虚拟内存时，由于没有与具体的大页物理内存建立页表，因此触发缺页中断，在缺页中断处理函数中，内核会为进程分配大页，并将大页作为 page cache 建立虚拟内存到大页物理内存的页表，当缺页中断返回之后，进程可以继续正常使用大页内存。huge_add_to_page_cache() 函数的作用是将一个大页作为某个大页文件的 page cache，其底层逻辑是: 参数 page 为大页，mapping 大页文件 struct file 的 f_mapping, 用于管理页面偏移与 page cache 的映射关系。函数首先在 700 行通过 mapping 找到大页文件对应的 struct inode, 接着通过 struct inode 找打了对应的 struct hstate. 接着调用 add_to_page_cache() 函数基于 mapping 建立了 idx 到 page 的映射关系，也就是说可以通过文件偏移 idx 找到对应的 struct page, 且 idx 按大页粒度进行偏移。struct page 如果正确添加到 page cache 里，那么函调用 ClearPagePrivate() 函数将 page 的 PG_Priate 标志清零，接着调用 set_page_dirty() 函数将 struct page 标记为脏页。接着函数在 715 行调用 blocks_per_huge_page() 函数更新了 inode 里 i_blocks 的数量。至此一个大页就称为了 page cache.

> [hstate_inode](#D02034)
>
> [blocks_per_huge_page](#D02119)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02119">blocks_per_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001101.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 方式向用户进程提供大页内存，进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程地址空间分配一段虚拟内存用于映射大页，但进程访问这段虚拟内存的时候，由于虚拟内存还没有建立到大页物理内存的页表，因此会触发缺页中断，在缺页中断中内核为虚拟内存分配物理大页并建立页表，待缺页中断返回之后进程可以正常访问这段虚拟内存，以此进程可以正常使用大页内存。blocks_per_huge_page() 函数的作用是计算一个大页占用的磁盘逻辑块的数量，对应文件方式分配的大页，其基于一个文件节点 struct inode, 因此需要对应后端存储设备，由于 hugetlbfs 没有实际的后端存储设备，因此 hugetlbfs 使用虚假文件作为后端，因此同样需要为其提供接口计算大页占用的 block 数，在磁盘存储中，block 大小为 512 个字节，因此其计算逻辑使用 huge_page_size() 函数获得大页的长度，然后直接将长度除以 512 便可以知道大页占用了多少个磁盘 block.

> [huge_page_size](#D02005)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02120">hugepage_add_new_anon_rmap</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001102.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从地址空间分配一段虚拟内存用于映射文件，当进程访问这段虚拟内存的时候，由于虚拟内存没有与大页物理内存建立页表，那么将触发缺页中断。在缺页中断处理函数中，内核会为虚拟内存分配大页物理内存，并建立虚拟内存到物理内存的页表，另外对于匿名映射，缺页中断还会为其建立逆向映射。当缺页中断处理完成返回之后，进程可以正确访问这段虚拟内存。hugepage_add_new_anon_rmap() 函数的作用就是为匿名映射建立逆向映射，其底层逻辑是: 参数 page 指向大页，参数 vma 指向虚拟内存，address 参数指向逆向的虚拟地址，逆向映射映射主要建立 page 到 address 的映射关系。函数首先判断 address 是否位于 VMA 虚拟内存内，如果不再就调用 BUG_ON() 报错，如果在那么函数调用 atomic_set() 函数将大页 struct page 的映射计数设置为 0，大页默认的映射计数为 -2, 那么这里设置为 0 表示这里的逆向映射已经被统计。函数接着调用 \_\_page_set_anon_rmap() 函数建立 page、vma 和 address 之间的逆向映射.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02121">make_huge_pte</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001103.png)

在 hugetlb 大页机制中，内核支持 2MiB 或者 1Gig 的大页，进程的虚拟地址可以建立页表映射大页物理内存上，这样进程就可以使用这段大页物理内存了。Linux 内核支持 5 级页表，分别是 PGD、P4D、PUD、PMD 和 PTE，内核通过 5 级页表可以将一个 4KiB 的虚拟区域映射到 4KiB 的物理页上。由于大页的粒度为 2MiB 和 1Gig，那么在映射大页时最后一级页表是 PMD 或者是 PUD。make_huge_pte() 函数的作用是构建一个 hugetlb 大页最后一级页表项, 其底层逻辑是: 参数 vma 指向虚拟内存，page 则指向大页，参数 writable 用于控制页表的可写权限。函数首先在 85 行判断大页是否可写，如果可以那么函数进入 86 分支继续执行，此时函数函数调用 mk_huge_pte() 函数基于 VMA 的 vm_page_prot 标志和 page 制作最基础的页表内容，接着再调用 huge_pte_mkdirt() 将页表的 Dirty 置位，以此表示该页是脏页，函数最后调用 huge_pte_mkwrite() 函数将页表的 Write 标示位置位，最后获得页表项内存存储到 entry 变量里; 反之如果页表不可以写，那么函数进入 89 行分支，此时函数首先调用 mk_huge_pte() 函数基于 VMA 的 vm_page_prot 和 page 制作最基础的页表，然后调用 huge_pte_wrprotect() 函数将页表的 Write 标志位清零，并将页表项存储到 entry 变量里。制作完基础页表之后，函数在 92 行调用 pte_mkyong() 函数将页表的 Access 标志位清零，以此表示页表没有进程访问过。函数在 93 行调用 pte_mkhuge() 函数将页表 PSE 标志位置位，以此告诉 MMU 这是页表的最后一级页表。最后函数调用 arch_make_huge_pte() 函数进行体系结构相关页表标记. 经过上面的处理之后，函数已经完全制作一个页表，并返回该页表项的内容.

> [mk_huge_pte](#D02095)
>
> [huge_pte_mkdirty](#D02099)
>
> [huge_pte_mkwrite](#D02098)
>
> [huge_pte_wrprotect](#D02094)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02122">set_huge_ptep_writable</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001105.png)

在 hugetlb 大页机制中，内核支持 2MiB 或者 1Gig 的大页，进程的虚拟地址可以建立页表映射大页物理内存上，这样进程就可以使用这段大页物理内存了。Linux 内核支持 5 级页表，分别是 PGD、P4D、PUD、PMD 和 PTE，内核通过 5 级页表可以将一个 4KiB 的虚拟区域映射到 4KiB 的物理页上。由于大页的粒度为 2MiB 和 1Gig，那么在映射大页时最后一级页表是 PMD 或者是 PUD。set_huge_ptep_writable() 函数的作用是将页表的 Writable 标志位置位，其底层逻辑是: 函数首先通过 huge_ptep_get() 函数获得 ptep 指针对应的页表，然后调用 huge_pte_mkdirty() 函数将页表的 Dirty 标志位置位，最后调用 huge_pte_mkwrite() 函数将页表的 Writable 标志位置位，这样页表的 Dirty 和 Writable 标志位置位，页表对应的大页就是可写的脏页。接下来函数调用 huge_ptep_set_access_flags() 函数判断此时页表的 Access 标志位是否置位，即大页此时有没有被访问过，如果有那么函数调用 update_mmu_cache() 更新页表.

> [huge_ptep_get](#D02089)
>
> [huge_pte_mkdirty](#D02099)
>
> [huge_pte_mkwrite](#D02098)
>
> [huge_ptep_set_access_flags](#D02090)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02123">hugetlb_count_add</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001106.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从地址空间分配一段虚拟内存用于映射文件，当进程访问这段虚拟内存的时候，由于虚拟内存没有与大页物理内存建立页表，那么将触发缺页中断。在缺页中断处理函数中，内核会为虚拟内存分配大页物理内存，并建立虚拟内存到物理内存的页表，另外对于匿名映射，缺页中断还会为其建立逆向映射。当缺页中断处理完成返回之后，进程可以正确访问这段虚拟内存。hugetlb_count_add() 函数的作用是进程地址空间新映射一个大页，其底层逻辑是: 进程地址空间 struct mm_struct 的 hugetlb_usage 成员用于记录进程映射 hugetlb 大页的数量，当进程新映射一个大页，那么函数调用 atomic_long_add() 函数增加 hugetlb_usage 的引用计数。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02124">hugetlb_count_sub</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001107.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从地址空间分配一段虚拟内存用于映射文件，当进程访问这段虚拟内存的时候，由于虚拟内存没有与大页物理内存建立页表，那么将触发缺页中断。在缺页中断处理函数中，内核会为虚拟内存分配大页物理内存，并建立虚拟内存到物理内存的页表，另外对于匿名映射，缺页中断还会为其建立逆向映射。当缺页中断处理完成返回之后，进程可以正确访问这段虚拟内存。hugetlb_count_sub() 函数的作用是进程地址空间解除映射一个大页，其底层逻辑是: 进程地址空间 struct mm_struct 的 hugetlb_usage 成员用于记录进程 hugetlb 大页的数量，当进程解除映射一个大页，那么函数调用 atomic_long_sub() 函数减少 hugetlb_usage 的引用计数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02125">copy_user_huge_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001108.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从地址空间分配一段虚拟内存用于映射文件，当进程访问这段虚拟内存的时候，由于虚拟内存没有与大页物理内存建立页表，那么将触发缺页中断。在缺页中断处理函数中，内核会为虚拟内存分配大页物理内存，并建立虚拟内存到物理内存的页表，另外对于匿名映射，缺页中断还会为其建立逆向映射。当缺页中断处理完成返回之后，进程可以正确访问这段虚拟内存。当进程进行 fork 操作产生子进程的时候，由于映射大页文件时可以采用共享映射和私有映射，因此在 fork 时候的行为不一样，其中对于私有映射会走 COW 流程，在 COW 流程中如果需要拷贝父进程大页的内容到新大页上。copy_user_huge_page() 函数的作用是 COW 时负责拷贝大页内容到新大页上，其底层逻辑是: dst 指向新大页，src 则指向源端大页，addr_hint 则表示大页对于的虚拟地址，VMA 则是虚拟内存，最后 pages_per_huge_page 指明一个大页中包含 4KiB 页的数量。函数首先对 addr_hint 进行掩码操作，以此找到 addr_hint 非对齐的起点，接着函数基于 dst、src 和 vma 构造一个 struct copy_subpage 数据结构，然后在 22 行检查大页中包含 4KiB 页的数量是否超过 MAX_ORDER_NR_PAGES, 如果超过那么大页就是 1Gig 的大页，此时函数调用 copy_user_gigantic__page() 函数进行大页内容的拷贝; 反之大页是 2MiB 的大页，那么函数调用 process_huge_page() 函数对大页包含的 4KiB 页进行内容的拷贝.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02126">hugetlb_cow</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001109.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。进程通过从 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程的地址空间分配一段虚拟内存映射大页文件。当进程访问这段虚拟内存的时候，由于虚拟内存还没有与实际的大页建立页表，那么系统触发缺页中断，在缺页中断处理程序中，系统会从大页池子中分配大页，并建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程就可以通过访问这段虚拟内存使用大页物理内存。进程可能会 fork 新的子进程，那么大页对应的 VMA 也会 fork 到子进程中，但由于 VMA 的映射方式不同的缘故，fork 的行为也存在差异。对应私有映射的大页对应的 VMA，当 fork 完毕之后，无论是子进程还是父进程，只要第一个访问这段虚拟内存，都会触发 COW。hugetlb_cow() 函数的作用就是但子进程或父进程 fork 之后对映射大页的虚拟内存进行访问时触发 COW 的处理逻辑，其底层逻辑为: 在发生 fork 的时候，私有映射的 VMA 对应的页表都会设置为写保护，当 fork 完毕之后，无论是父进程还是子进程，只要第一个对 VMA 进行写操作的时候就会触发 COW，所谓 COW 就是新分配一个大页，接着将原始大页内存的内容拷贝到新大页上，然后将进程的虚拟地址重新映射到新大页上，并解除写保护，最后进程就对新大页进行写操作; 对于第二个写的进程来说，由于此时只有一个进程的 VMA 映射到原始大页上，那么此时进程对大页写操作的时候并不会触发 COW，而是简单的将写保护清除，然后往大页上继续写内容。参数 mm 对应进程的地址空间，参数 vma 则对应进程的虚拟区域，addr 参数对应发生写操作的虚拟地址，参数 ptep 为发生写操作虚拟地址对应的页表。

函数首先在 546 行调用 hstate_vma() 函数获得 VMA 对应的 struct hstate, 该数据结构用于维护指定长度的大页，接着在 550 行调用 huge_page_mask() 函数获得大页地址掩码，并将 address 虚拟地址进行对齐操作。函数在 553 行调用 huge_ptep_get() 函数获得 ptep 对应的页表内容，并存储在 pte 变量里，然后通过 pte_page() 函数获得页表中指向大页的 struct page. 函数接着在 559 行首先判断当前的物理大页被几个进程映射，如果大于 1 个，那么表示函数需要执行 COW; 相反如果只有一个进程映射了大页，那么此时无需发生 COW，只需将写保护清除即可，此时函数检查大页对应的 struct page 是否为匿名大页，如果是那么函数进入 560 分支继续执行。在 560 分支，函数首先调用 page_move_anon_rmap() 函数将大页标记为匿名反向映射，接着调用 set_huge_ptep_writable() 函数将页表的写保护清除，这样进程就可以对大页进程正常的读写操作，最后直接返回 0; 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001110.png)

对于 fork 之后第一次对大页写操作的进程而言，函数首先在 3574 行检查父进程是否为其独自预留的大页，此时通过调用 is_vma_resv_set() 函数检查 VMA 的 private_data 中是否包含 HPAGE_RESV_OWNER 标志来判断进程是否在大页池子中预留了大页，如果已经预留了，且 old_page 不是 pagecache_page, 那么函数将 outside_reserve 设置为 1，以此表示进程已经在大页池子中预留了大页。另外对于子进程而言，由于其没有在大页池子中预留大页，那么需要新分配大页，那么 outsize_reserve 不能设置为 1. 函数接着在 578 行将 old_page 的引用计数加 1. 函数接着在 585 行调用 alloc_huge_page() 函数分配大页，此时如果 outside_reserve 为 true, 那么函数不使用大页池子中预留的大页，而是使用空闲的大页，反之如果 outside_reserve 为 false，那么函数将使用大页池子中预留的大页。分配完毕之后，如果在 588 行检查到大页分配失败，那么函数进入 589 分支继续执行，此时进入缺页异常错误处理分支，如果此时 outside_reserve 为真，那么表示需要分配新的大页，那么函数将 old_page 的引用减一，然后检测到 pte 页表内容为空，那么函数调用 BUG_ON() 函数进行报错，函数此时调用 huge_pte_offset() 函数再次获得页表，如果此时页表与 pte 相同，那么函数跳转到 retry_avoidcopy 处再次执行，反之面临竞态，函数直接返回 0. 另外如果 outside_reserve 为 false，那么函数认为大页池中应该预留足够的大页的，但现在没有，那么函数直接跳转到 out_relase_old 处。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001111.png)

如果新的大页分配成功，那么函数在 621 行调用 anon_vma_prepare() 函数检查 VMA 是否包含了 anon_vma, 如果没有那么新分配 anon_vma 用于逆向映射，如果此时检查失败，函数将 ret 设置为 VM_FAULT_OOM, 并跳转到 out_release_all 处执行; 反之函数的逆向映射没有问题，那么函数在 626 行调用 copy_user_huge_page() 函数将旧 page 的内容拷贝到新的大页上，拷贝完毕之后调用 \_\_SetPageUptodate() 函数将新的大页标记为更新过的页。函数接下来调用 mmu_notifier_range_init() 函数和 mmu_notifier_invalidate_range_start() 函数通知注册该通知链上的监听者 haddr 已经改变了。函数接下来在 637 行调用 spin_lock() 为页表上锁，并调用 huge_get_offset() 获得指向页表的指针，如果此时调用 pte_same() 发现期间页表没有被修改过，那么函数进入 640 分支进行执行，此时函数首先调用 ClearPagePrivate() 将页的 PG_Private 标志清零，然后调用 huge_ptep_clear_flush() 函数将页表的内容清零，并把对应的 TLB 也刷新，然后调用 mmu_notifier_invalidate_range() 函数通知 mm 监听链。接着函数调用 make_huge_pate() 构建可读写的页表，然后调用 set_huge_pte_at() 函数设置页表。设置完页表之后，函数调用 page_remove_rmap() 将旧大页的逆向映射移除，并调用 hugepage_add_new_anon_rmap() 函数添加新的逆向映射。设置完逆向映射之后，函数调用 set_page_huge_active() 函数将大页标记为正在使用的大页，最后函数将 new_page 指向了 old_page. 处理完 640 分支之后，函数调用 spin_unlock() 函数解除页表的锁，然后调用 mmu_notifier_invalidate_range_end() 函数结束监听链的通知。函数接着进入 out_release_all 处，函数调用 restore_reserve_on_error() 函数重新更新大页的 resv_map 预留信息，并结束预留。最后函数将 new_page 和 old_page 的引用计数减一操作.

> [restore_reserve_on_error](#D02127)
>
> [set_page_huge_active](#D02015)
>
> [hugepage_add_new_anon_rmap](#D02120)
>
> [set_huge_pte_at](#D02102)
>
> [make_huge_pte](#D02121)
>
> [huge_ptep_clear_flush](#D02128)
>
> [huge_pte_offset](#D02086)
>
> [huge_page_size](#D02005)
>
> [copy_user_huge_page](#D02125)
>
> [alloc_huge_page](#D02114)
>
> [huge_page_mask](#D02092)
>
> [hstate_vma](#D02080)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02127">restore_reserve_on_error</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001112.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。进程通过在 hugetlbfs 文件系统挂载点下创建或打开大页文件，然后从进程地址空间分配一段虚拟内存映射大页文件，此时系统会为这段虚拟内存预留指定数量的大页，当进程访问这段虚拟内存时，由于没有与大页物理内存建立页表，因此会触发系统的缺页中断，在缺页处理程序中，内核从预留的大页找指定数量的大页与虚拟内存建立页表，待到缺页中断返回之后，进程可以通过访问虚拟内存使用大页。内核为了更好的管理预留大页，使用了 resv_map 机制。restore_reserve_on_error() 函数的作用当内核进入错误的预留路径时，将 resv_map 相关的预留重置，其底层逻辑是: 当一个大页通过 alloc_huge_page() 函数分配之后，其 PG_Private 标志是置位的，也表示该大页已经被其他进程使用，如果函数在 62 行调用 PagePrivate() 函数发现该标志位已经置位，但是此时函数期望的是 page 的 PG_Private 是没有置位的，也就是 page 是一个新的大页; 如果此时 PG_Private 置位，那么函数进入 63 行分支执行，函数首先调用 vma_needs_reservation() 函数计算出 VMA 需要预留大页的数量，如果此时 rc 小于 0，那么表示此时无需新预留大页，那么函数直接调用 ClearPagePrivate() 函数，page 因此变成了新的大页; 反之如果 rc 不小于 0， 那么需要预留新的大页，那么函数进入 79 行分支，调用 vma_add_reservation() 函数对 VMA 进行预留大页，如果此时 rc 小于 0，那么没有足够的大页可以预留，那么函数直接调用 ClearPagePrivate() 函数将大页的 PG_Private 标志清零; 反之不需要进行的预留，那么函数调用 vma_end_reservation() 函数结束预留流程.

> [vma_needs_reservation](#D02106)
>
> [vma_add_reservation](#D02109)
>
> [vma_end_reservation](#D02108)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02128">huge_ptep_clear_flush</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001113.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以 page cache 的方式为用户进程提供大页。进程通过在 hugetlbfs 文件系统挂载点创建或打开一个大页文件，然后从地址空间分配一段虚拟内存映射文件，当进程访问这段虚拟内存时，由于没有建立虚拟内存到大页的页表，那么触发内核缺页中断。在缺页中断处理函数中，内核会建立虚拟内存到大页的页表，待缺页中断返回之后，进程就可以正常访问这段虚拟内存。huge_ptep_clear_flush() 函数的作用是清零大页最后一级页表，并将虚拟内存从 TLB 中刷出，其底层逻辑是: Linux 支持五级页表，分别是 PGD、P4D、PUD、PMD 和 PTE，但由于大页粒度的不同，2MiB 大页只使用 4 级页表，因此最后一级页表是 PMD, 另外 1Gig 大页只使用 3 级页表，因此最后一级页表是 PUD. 函数通过调用 ptep_get_and_clear() 函数首先获得最后一级页表的内容，然后将页表的内容清零，接着将虚拟地址从 TLB 中刷出. 这样做了之后可以干净的清除大页的最后一级页表。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02129">hugetlb_fault</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001114.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供物理大页。进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程地址空间中分配一段虚拟内存映射大页文件，当进程访问这段虚拟内存时，由于虚拟内存还没有与大页物理内存建立页表，因此会触发内核缺页中断。在缺页中断处理函数中，内核会从大页内存池子中分配一个大页，然后建立虚拟内存到大页物理内存的页表。待缺页中断返回之后，进程通过访问虚拟内存间接使用大页内存。hugetlb_fault() 函数是内核缺页中断中关于 hugetlb 的中断处理函数，其底层逻辑为: 参数 mm 指向进程地址空间，参数 VMA 指向发生缺页的 VMA，参数 address 则指向发生缺页的虚拟地址。函数首先在 3933 行通过调用 hstate_vma() 函数获得 VMA 虚拟内存所使用大页的 struct hstate, 该结构维护指定长度的大页，接着在 3936 行调用 huge_page_mask() 函数将发生缺页的虚拟地址按大页粒度进行对齐。准备好基础数据之后，函数在 3938 行调用 huge_page_size() 获得大页的长度，然后调用 huge_pte_offset() 函数获得虚拟地址对应的最后一级页表，由于大页粒度不同，最后一级页表页不同。对于 2MiB 粒度的大页，最后一级页表是 PMD，而对于 1Gig 粒度的大页，最后一级页表是 PUD。函数在获得最后一级页表的地址之后，如果此时最后一级页表是空的，那么函数进入 3948 行调用 huge_pte_alloc() 函数分配一个新的页表; 反之如果最后一级页表已经存在，那么函数进入 3940 分支执行，此时函数调用 huge_ptep_get() 函数获得最后一级页表的内容，此时函数调用 is_hugetlb_entry_migration() 函数判断大页是否已经被迁移走，如果是函数调用 migration_entry_wait_huge() 函数等待迁移完毕，迁移完毕之后直接返回 0; 反之函数调用 is_hugetlb_entry_hwpoisoned() 函数判断大页最后一级页表的内容是否为有污染的，即不可用的，如果是那么函数直接返回错误码. 函数接下来在 3961 行调用 hugetlb_fault_mutex_hash() 函数从 hugetlb mutex hash 链表上获得一个 mutex 锁，然后调用 mutex_lock() 函数将 mutex 锁加上。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001115.png)

函数接下来在 3964 行调用 huge_ptep_get() 函数获得最后一级页表的内容，接着调用 huge_pte_none() 函数判断页表的内容是否为空。如果为空，那么这段虚拟内存是第一次缺页，那么函数进入 3966 行调用 hugetlb_no_page() 函数分配一个大页，大页分配完毕之后，函数跳转到 out_mutex 处执行; 反之页表不为空，那么可能是 fork 之后的子进程或父进程的写操作引起的缺页，因为在私有映射中，父进程在 fork 子进程的时候会将父子进程的页表都设置为只读模式，如果父子进程任一个发起写操作都会触发写缺页，这种情况下函数在 3965 行会把这种情况给隔离出来，以此不会在 3966 行新分配一个大页。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001116.png)

如果进程不是因为其虚拟地址没有与大页物理内存建立页表引发的缺页，而是因为向写保护的虚拟地址进行写操作引发的缺页，那么这种情况大多发生在私有映射的时候，此时父进程通过 fork 系统调用之后生成子进程，另外父进程映射物理大页的 VMA 一共负责到子进程，那么父子进程的虚拟内存同时映射到同一个大页物理内存上，接着 fork 系统调用将父子进程映射大页的最后一级页表设置为写保护。无论是父进程还是子进程，只要第一个对映射大页物理内存的虚拟内存进行写操作时，都会触发系统的缺页中断，但不同的时，父子进程中第一个对大页进行写操作的时候，由于大页同时被两个进程的虚拟内存映射，会触发 COW，所谓的 COW 就是第一个发起写操作的进程会新分配一个大页，然后将原始大页的内容拷贝到新大页上，接着对应的虚拟内存的页表建立到新的大页上，这样进程就对新的大页执行写操作。那么对于第二个对大页进行写操作的进程，由于此时只有该进程的虚拟内存映射映射大页，因此缺页中断只会将写保护移除掉。函数接着在 3979 行调用 pte_present() 函数判断大页是否存于迁移中或者 HWPOISONED 中，如果置位那么函数直接跳转到 out_mutex 处继续执行。函数接下来在 3990 行通过 flags 判断缺页是否由写操作触发的，如果是函数继续检查页表此时是否没有写权限，如果是那么函数进入 3991 分支执行。在 3991 分支，函数首先调用 vma_needs_reservation() 函数判断发生缺页的进程是否需要预留新的大页，如果此时函数返回值小于 0，那么代表系统此时无法预留新的大页，那么函数认为缺页中断中触发了 OOM，于是将 ret 设置为 VM_FAULT_OOM, 接着跳转到 out_mutex 处执行; 反之系统可以预留新的大页，那么函数在 3996 行调用 vma_end_reservation() 函数结束预留逻辑。接着函数在 3998 行检查到 VMA 虚拟内存是通过私有映射的方式映射到大页物理内存，那么函数调用 hugetlbfs_pagecache_page() 函数获得进程虚拟地址对应的 pagecache_page, 对于私有匿名映射无法获得 pagecache_page. 结束 3991 分支之后，函数在 4003 行调用 huge_get_lock() 函数将大页最后一级页表 spinlock 锁上锁。函数接着扎起 4006 行调用 pte_same() 函数判断缺页中断开始获得的页表内容和现在页表的内容是否有改变，如果发现两者的内容不同，那么很可能是页表正在发生迁移等操作，那么函数将不会继续发生缺页，而是终止缺页，那么函数跳转到 out_ptl 处执行; 反之两个页表的内容都没变，说明虚拟内存没有被修改过。该检查很大程度上控制大页缺页的有效性.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001117.png)

由于大页 COW 时需要锁住大页和 pagecace_page, 因此函数首先在 4014 行调用 pte_page() 获得页表中指向的大页，然后判断 page 是否与 pagecache_page 是同一个页，如果不是，那么需要将大页上锁，那么函数调用 trylock_page() 函数进行上锁，如果上锁失败，那么函数将 need_wait_lock 设置为 1，并跳转到 out_ptl 处执行; 反之 pagecache_page 与 page 不是同一个页，或者 pagecache_page 为 NULL 时，都需要将大页上锁。接着函数在 4021 处调用 get_page() 函数将大页的引用计数加一，以此后续操作正常执行下去。函数通过 4023 行和 4024 行的两个判断确定是 COW 的条件，也就是对写保护的虚拟地址发起写操作，这样触发的缺页中断需要处理 COW，那么函数进入 4025 分支执行，此时函数调用 hugetlb_cow() 函数执行大页的 COW 流程，执行完毕之后函数跳转到 out_put_page 处继续执行; 反之如果因为写操作触发的缺页中断，但此时页表有写权限，那么函数不会执行 COW，而是在 4029 行调用 huge_pte_mkdirty() 函数将页表的 Dirty 置位，以此将大页标记为脏页。执行完 4024 分支之后，函数在 4031 行调用 pte_mkyoung() 函数将 entry 页表的 Access 位清, 接着函数调用 huge_ptep_set_access_flags() 函数判断 entry 内容是否和页表的内容相同，如果此时页表 Access 标志置位，那么表示大页被访问过，此时函数调用 update_mmu_cache() 函数将页表对应的 cache 都刷会内存中，以此确保数据的一致.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001118.png)

接下来是不同的处理分支，首先是 out_put_page 处，函数会判断大页与 pagecache_page 是否为同一个大页，如果不是那么函数需要将大页解锁，因为之前这种情况下将大页上锁了。由于之前在 COW 之前对大页的引用计数加一，当 COW 执行完毕之后需要将大页的引用计数减一。接下来是 out_ptl 分支，函数调用 spin_unlock() 函数将大页页表的 spinlock 锁解锁。如果此时函数发现 pagecache_page 存在，那么函数将 pagecache_page 解锁并对引用计数减一。最后是 out_mutex 分支，函数对之前上锁的 hugetlb_fault_mutex_table[] 的 mutex 锁进行解锁，接着函数检测到 need_wait_lock 为真，那么函数调用 wait_on_page_locked() 函数等待大页解锁，大页可能在其他流程中被上锁了。最后的最后函数返回 ret 的值，至此大页的缺页中断处理程序执行完毕。

> [huge_pte_mkdirty](#D02099)
>
> [hugetlb_cow](#D02126)
>
> [huge_ptep_get](#D02089)
>
> [hugetlbfs_pagecache_page]()
>
> [vma_end_reservation](#D02108)
>
> [vma_needs_reservation](#D02106)
>
> [huge_pte_write](#D02096)
>
> [hugetlb_no_page]()
>
> [vma_hugecache_offset](#D02078)
>
> [huge_pte_alloc]()
>
> [huge_pte_offset](#D02086)
>
> [huge_page_size](#D02005)
>
> [huge_page_mask](#D02092)
>
> [hstate_vma](#D02080)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02130">hugetlb_no_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001119.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个文件大页，然后从进程地址空间分配一段虚拟内存映射文件大页。当进程访问这段虚拟内存时，由于虚拟内存还没有与大页物理内存建立页表，那么会触发系统缺页中断。在缺页中断处理程序中，内核会从大页内存池子中分配一个大页人，然后建立虚拟内存到物理大页的页表。待缺页中断返回之后，进程就可以通过访问虚拟内存的途径使用大页物理内存了。在缺页处理函数里，需要从大页内存池子中分配大页，如果大页池子没有足够的大页，那么会通过大页超发机制，动态从 buddy 分配器中分配大页。hugetlb_no_page() 函数的作用是在 hugetlb 缺页中断中分配大页，其底层逻辑是: 参数 mm 指向进程的地址空间

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)


> [https://blog.csdn.net/yk_wing4/article/details/88080442](https://blog.csdn.net/yk_wing4/article/details/88080442)
