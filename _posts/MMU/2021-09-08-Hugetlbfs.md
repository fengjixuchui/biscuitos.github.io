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
>   - [共享匿名大页使用攻略](#BA)
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
> - Hugetlb/Hugetlbfs 实践
>
>   - Hugeltb/Hugetlbfs on X86_64
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
>   - [hugetlb/hugetlbfs 数据结构](#D0)
>
>   - [hugetlb/hugetlbfs 核心数据](#D3)
>
>   - [hugetlb/hugetlbfs API](#D2)
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
> - [Hugetlb/Hugetlbfs 进阶研究](#KC)
>
>   - 硬件如何支持大页
>
>   - 2MiB Hugetlb 大页研究
>
>   - 1Gig Hugetlb 大页研究
>
>   - [Hugetlb/Hugetlbfs 与 CMDLINE 关系研究](#KA)
> 
>   - [Hugetlb 大页机制与内核宏关系研究](#KB)
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

<span id="D0"></span>
> - [Hugetlbfs 数据结构](#D015)
>
>   - [enum hugetlbfs_size_type](#D010)
>
>   - [struct file_region]()
>
>   - [struct hstate](#D014)
>
>   - [struct hugepage_subpool](#D011)
>
>   - [struct huge_bootmem_page]()
>
>   - [struct hugetlbfs_config](#D012)
>
>   - [struct hugetlbfs_inode_info]()
>
>   - [struct hugetlbfs_sb_info](#D013)
>
>   - [struct node_hstate]()
>
>   - [struct resv_map]()
>
>   - [vma_resv_mode]()

<span id="D3"></span>
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

<span id="D2"></span>
> - [Hugetlbfs API](#D02133)
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
>   - [huge_pte_alloc](#D02132)
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
>   - [hugetlbfs_pagecache_page](#D02131)
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
> [hugetlbfs_pagecache_page](#D02131)
>
> [vma_end_reservation](#D02108)
>
> [vma_needs_reservation](#D02106)
>
> [huge_pte_write](#D02096)
>
> [hugetlb_no_page](#D02130)
>
> [vma_hugecache_offset](#D02078)
>
> [huge_pte_offset](#D02086)
>
> [huge_page_size](#D02005)
>
> [huge_page_mask](#D02092)
>
> [hstate_vma](#D02080)
>
> [huge_pte_alloc](#D02132)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02130">hugetlb_no_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001119.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个文件大页，然后从进程地址空间分配一段虚拟内存映射文件大页。当进程访问这段虚拟内存时，由于虚拟内存还没有与大页物理内存建立页表，那么会触发系统缺页中断。在缺页中断处理程序中，内核会从大页内存池子中分配一个大页人，然后建立虚拟内存到物理大页的页表。待缺页中断返回之后，进程就可以通过访问虚拟内存的途径使用大页物理内存了。在缺页处理函数里，需要从大页内存池子中分配大页，如果大页池子没有足够的大页，那么会通过大页超发机制，动态从 buddy 分配器中分配大页。hugetlb_no_page() 函数的作用是在 hugetlb 缺页中断中分配大页，其底层逻辑是: 参数 mm 指向进程的地址空间, 参数 vma 对应进程的虚拟内存 VMA，参数 mapping 对应大页文件的 struct address_space 数据结构，参数 idx 指明缺页地址在大页文件中的偏移，参数 address 指向发生缺页的地址，参数 ptep 指向最后一级页表，参数 flags 则指向缺页的标志集合. 函数首先在 3725 行调用 hstate_vma() 函数获得 VMA 虚拟内存映射大页的 struct hstate, 该数据结构用于维护指定长度的大页，函数接着在 3732 行调用 huge_page_mask() 函数将缺页地址按大页 mask 进行对齐。函数首先在 3740 行调用 is_vma_resv_set() 函数检查 VMA 是否设置了 HPAGE_RESV_UNMAPPED 标志，如果该标志置位，表示父进程 fork 子进程之后，系统没有足够的大页为子进程分配大页，因此这种情况下需要直接 kill 掉子进程，那么函数进入 3740 分支打印相关的信息之后，直接返回 VM_FAULT_SIGBUG, 此时系统会因为 BUS Err 夯住.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001120.png)

函数接下来调用 find_lock_page() 函数在文件的 mapping 中查找映射的 page cache，如没有找到进入 3753 分支执行，缺页情况下大概率找不到 page cache，需要新分配大页。函数 3760 到 3786 行是大页 userfaultfd 的处理流程，这里先不做讨论.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001121.png)

函数接下来在 3788 行调用 alloc_huge_page() 函数分配一个大页，如果分配失败，那么函数使用 ret 记录下出错的原因，并跳转到 out 处执行; 反之成功分配一个新的大页，函数接着调用 clear_huge_page() 函数将大页内容清零，接着调用 \_\_SetPageUptodate() 将大页标记一个刚更新的新大页，函数接着将 new_page 设置为 true。函数接下来在 3797 行检查虚拟内存是否通过共享方式映射到大页的，如果是那么进入 3797 分支执行，此时由于采用共享映射的方式进行映射，那么多个进程的虚拟内存可能映射到同一个大页上，那么函数调用 huge_add_to_page_cache() 函数将大页添加到大页文件的 mapping。如果添加失败，那么函数将大页的引用计数减一，如果此时失败的原因是 EEXIST, 即 page cahce 已经存在，那么函数直接跳转到 retry 处执行 page cache 存在的逻辑，否则直接跳转到 out 处执行. 如果 page cache 添加成功，那么调用 anon_vma_prepare() 函数添加匿名映射的反向映射，如果虚拟内存不是匿名映射，那么直接跳过, 否则反向映射如果失败，那么函数将 ret 设置为 VM_FAULT_OOM, 然后跳转到 backout_unlocked 处执行; 反之函数将 anon_rmap 设置为 1, 这里值得注意的是 3806 分支的虚拟内存按私有方式进行映射.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001122.png)

接下处理的是 page cache 本来就存在的情况，此时函数进入 3814 分支，此时函数会检测大页是否为 HWPOISON, 如果是那么函数将 ret 设置为 VM_FAULT_HWPOISON, 最后跳转到 backout_unlocked 处执行. 函数接下来在 3832 行通过两个判断确认缺页是否为私有映射对写保护引起的，如果是那么函数进入 3833 分支执行，此时函数调用 vma_needs_reservation() 函数检查预留的大页是否满足缺页的需求，如果大页池子没有足够的大页，那么函数认为内存不足，然后将 ret 设置为 VM_FAULT_OOM, 然后跳转到 backout_unlocked 处执行，否则函数调用 vma_end_reservation() 结束预留检测。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001123.png)

函数接下来要对新分配的大页建立相应的页表，那么函数首先需要在 3841 行调用 huge_pte_lock() 函数对页表上锁，接着调用 i_size_read() 函数获得大页文件最大长度，然后向右移动 huge_page_shift() 获得大页文件最大的偏移数，函数接着判断发生缺页的 idx 是否已经超出文件最大的索引，如果超出那么跳转到 backout 处执行; 反之发生缺页的位置是安全的。函数继续在 3847 行调用 huge_pte_none() 函数检查页表是否为空，如果不空那么不符合预期并跳转到 backout 处，此时页表应该空. 函数接着检查 anon_rmap 是否为真，如果为真那么为私有映射，那么函数调用 ClearPagePrivate() 将新大页的 PG_Private 标志清零，然后调用 hugepage_add_new_anon_rmap() 函数建立大页 page 到 VMA 的方向映射; 反之映射为共享映射，那么函数在 3854 行调用 page_dump_rmap() 函数设置反向映射。函数接下来调用 make_huge_pte() 函数构建页表，构建时根据虚拟区域是否可写，已经 VMA 是否为共享映射两个条件进行判断，如果 VMA 是共享映射且可写，那么页表是可写的。函数接着在 3857 行调用 set_huge_pte_at() 函数设置对应的页表。设置完页表之后，函数调用 hugetlb_count_add() 函数添加进程对 hugetlb 大页的引用计数。函数在 3860 行判断到映射是一个私有可写时，函数会调用 hugetlb_cow() 函数确保页表可写。至此缺页分配大页的逻辑基本处理完毕，接下来就是解锁和异常处理。在 3865 行对页表解锁，然后检查到 new_page 为真，那么需要将大页设置为正在使用态。最后解锁大页并返回 ret。对于 backout_unlocked 分支函数解锁大页，并调用 restore_reserve_on_error() 函数重置大页的 resv_map 统计，最后将大页的引用计数减一，触发大页的回收。backout 分支则多了页表解锁操作. 最后都跳转到 out 处返回 ret.

> [restore_reserve_on_error](#D02127)
>
> [hugetlb_cow](#D02126)
>
> [hugetlb_count_add](#D02123)
>
> [set_huge_pte_at](#D02102)
>
> [make_huge_pte](#D02121)
>
> [hugepage_add_new_anon_rmap](#D02120)
>
> [huge_pte_none](#D02104)
>
> [huge_ptep_get](#D02089)
>
> [huge_page_shift](#D02020)
>
> [vma_end_reservation](#D02108)
>
> [vma_needs_reservation](#D02106)
>
> [huge_add_to_page_cache](#D02118)
>
> [clear_huge_page](#D02116)
>
> [alloc_huge_page](#D02114)
>
> [is_vma_resv_set](#D02068)
>
> [huge_page_mask](#D02092)
>
> [hstate_vma](#D02080)

------------------------------------

###### <span id="D02131">hugetlbfs_pagecache_page</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001124.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为进程提供大页。进程通过在 hugetlbfs 文件系统的挂载点下创建或打开一个大页文件，然后将进程地址空间的一段虚拟内存映射到文件，当进程访问这段虚拟内存时，由于虚拟内存没有与大页物理内存建立页表，那么此时会触发系统缺页。在缺页处理程序中，内核会为虚拟内存分配大页，并建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程可以通过访问虚拟内存间接使用大页内存。hugetlbfs_pagecache_page() 函数的作用就是获得虚拟内存对应的大页 page cache，该函数不是通过页表获得，而是通过虚拟空间映射文件的逻辑实现，其底层逻辑为: 进程将虚拟内存映射大页内存时，其通过大页文件 struct file 的 f_mapping 成员维护虚拟内存到大页文件 page cache 的关系，f_mapping 为 struct address_space 数据结构，该数据结构维护一颗基数树或者 Xarray，用于建立虚拟地址在文件中的偏移与大页 page cache 的映射，因此函数首先通过 VMA 找到对应的大页文件 vma->vm_file, 然后获得对应的 f_mapping 成员，接着调用 vma_hugecache_offset() 获得虚拟地址 address 在文件中的偏移，偏移的粒度与大页粒度一致。最后函数调用 find_lock_page() 从 f_mapping 的基数树或者 Xarray 中获得对应的 page cache.

> [vma_hugecache_offset](#D02078)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D02132">huge_pte_alloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001125.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为进程提供大页。进程通过在 hugetlbfs 文件系统的挂载点下创建或打开一个大页文件，然后将进程地址空间的一段虚拟内存映射到文件，当进程访问这段虚拟内存时，由于虚拟内存没有与大页物理内存建立页表，那么此时会触发系统缺页。在缺页处理程序中，内核会为虚拟内存分配大页，并建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程可以通过访问虚拟内存间接使用大页内存。huge_pte_alloc() 函数的作用是为映射大页的页表分配最后一级页表，其底层逻辑是: 参数 mm 指向进程的地址空间，参数 addr 表示页表的虚拟地址，sz 则表示虚拟内存的长度。由于 Linux 支持 5 级页表，分别是 PGD、P4D、PUD、PMD 和 PTE，由于大页粒度的不同，最后一级页表也有所不同，对于 1Gig 的大页，其最后一级页表是 PMD, 而对于 2MiB 的大页，其最后一级页表是 PUD. 函数首先在 63 行通过 pgd_offset() 从进程地址空间 mm 获得 PGD 页表，接着直接调用 p4d_alloc() 函数分配 P4D 页表，如果存在则直接返回 P4D 页表，接着函数调用 pud_alloc() 函数分配 PUD，如果此时 PUD 存在，那么函数继续检测虚拟内存的长度 sz 是否为 PUD_SIZE, 即检测大页的长度是否为 1Gig，如果是那么最后一级页表就是 PUD; 反之大页粒度是 2MiB，那么此时如果检测到 sz 不等于 PMD_SIZE 系统就报错，否则函数继续检测是否需要共享 PMD 且 PUD 是空的，那么函数就调用 huge_pmd_share() 函数分配最后一级页表; 反之函数调用 pmd_alloc() 函数分配最后一级页表. 当最后页表分配完毕之后，函数如果检测到最后一级页表存在当最后一级页表不是映射大页，那么系统就会报错。最后函数返回最后一级页表的 Entry 地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D010">enum hugetlbfs_size_type</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001126.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，并从进程的地址空间分配一段虚拟内存映射大页文件，当进程访问这段虚拟内存时，由于虚拟内存还没有与大页物理内存建立页表，因此会触发系统的缺页中断。在去也中断处理函数中，内核会从大页池子中找到一个可用的大页，并建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程可以通过访问这段虚拟内存间接的使用大页。内核为了让用户进程可以使用上大页，那么内核会自动挂载 hugetlbfs 文件系统以供用户进程分配匿名 hugetlb 大页，另外用户进程也可以挂载 hugetlbfs 文件系统，以此分配文件 hugetlb 大页。无论是分配匿名大页还是文件大页，其都需要挂载 hugetlbfs 文件系统。enum hugetlbfs_size_type 枚举变量用于指明 hugetlbfs 文件系统挂载时的长度类型，这里的长度类型具体指 hugetlbfs 文件系统中包含大页数量的计算方式。hugetlbfs 文件系统支持三种长度类型。

###### NO_SIZE

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K
{% endhighlight %}

NO_SIZE 类型用于指明在挂载 hugetlbfs 文件系统时，没有指明该挂载点包含的大页的数量，那么挂载点可以直接使用大页池子中所有大页。以上便是挂载 NO_SIZE 类型的 hugetlbfs 文件系统，首先创建一个目录，然后向大页池子中添加 20 个大页，接着在使用 mount 挂载 hugetlbfs 文件系统时，不使用任何参数指明挂载点的大页，那么挂载点默认的长度就等于大页池子的长度.

###### SIZE_STD

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,size=8M,min_size=4M
{% endhighlight %}

SIZE_STD 类型用于在挂载 hugetlbfs 文件系统时，显示的指明了挂载点的长度，该长度可以使用标准长度单位: K/M/G. 当显示的指明长度之后，挂载点就包含了指定数量的大页，可以从上面的例子中看 SIZE_STD 的使用方式: 创建一个目录，然后向大页池子中添加 20 个大页，接着挂载 hugetlbfs 文件系统时，使用 "size=" 字段指明挂载点的最大长度是 8MiB, 那么挂载点最大包含 4 个大页，另外使用 "min_size=" 字段指明挂载点至少预留的大页数量，例子中设置为 4M，那么 hugetlbfs 文件系统挂载是必须保证其最少有 2 个大页是专门给该挂载点使用的.

###### SIZE_PERCENT

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,size=50%,min_size=10%
{% endhighlight %}

SIZE_PERCENT 类型用于在挂载 hugetlbfs 文件系统时，显示的指明挂载点包含大页数量占大页内存池子的百分比。当显示指明长度之后，挂载点就包含了百分比的大页，可以从上面的例子中看 SIZE_PERCENT 的使用方式: 创建一个目录，然后向大页内存池子中添加 20 个大页，接着挂载 hugetlbfs 文件系统时，使用 "size=" 字段指明挂载点最大长度时使用了百分比，也就是该挂载点最大长度占大页池子的百分比，在例子中是 50%，那么挂载点最多包含 10 个大页，另外可以使用 "min_size=" 字段指明挂载点至少预留大页时也可以使用百分比，以此表示挂载点至少从大页池子中预留大页的百分比，例如例子中是 10%，那么挂载点最少预留 2 个大页.

最后当 hugetlbfs 文件系统挂载完毕之后，可以使用 mount 命令或者通过 "/proc/mounts" 节点查看 hugetlbfs 文件系统挂载参数信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001128.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D011">struct hugepage_subpool</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001127.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，并从进程的地址空间分配一段虚拟内存映射大页文件，当进程访问这段虚拟内存时，由于虚拟内存还没有与大页物理内存建立页表，因此会触发系统的缺页中断。在去也中断处理函数中，内核会从大页池子中找到一个可用的大页，并建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程可以通过访问这段虚拟内存间接的使用大页。内核为了让用户进程可以使用上大页，那么内核会自动挂载 hugetlbfs 文件系统以供用户进程分配匿名 hugetlb 大页，另外用户进程也可以挂载 hugetlbfs 文件系统，以此分配文件 hugetlb 大页。无论是分配匿名大页还是文件大页，其都需要挂载 hugetlbfs 文件系统。内核使用 struct hstate 数据结构维护系统的大页内存池子，hugetlbfs 文件系统也可以维护一个私有的小池子，该小池子用于描述 hugetlbfs 文件系统挂载点包含最大可使用大页数量和最小预留大页数量，小池子使用 struct hugepage_subpool 数据结构进行维护。hugetlbfs 文件系统挂载点不一定要使用小池子，但一旦使用 hugetlbfs 文件系统挂载点就会从内核大页池子中预留一部分大页，然后进程优先从挂载点的小池子中分配大页。hugetlbfs 文件系统挂载点并不实际维护大页，而是维护大页的使用，包括挂载点包含的大页数量、正在使用的大页数量和预留的大页数量等，那么接下来具体分析每个成员的含义:

###### count

count 成员用于指明 hugetlbfs 文件系统挂载点是否正在使用 subpool。当挂载点确认使用 subpool 的时候，会将 count 成员设置为 1，以此表示 count 正在使用，可以通过 hugepage_new_subpool() 函数查看其逻辑。另外当系统 umount hugetlbfs 文件系统的挂载点是，系统会调用 hugepage_put_subpool() 函数将 count 成员减一。最终内核调用 unlock_or_release_subpool() 函数检判断 subpool 是否释放，其释放条件是 count 为 0 且没有进程使用 subpool 中的大页时，添加满足内核将 subpool 释放. 因此 count 成员用于指明 subpool 是否正在使用。count 成员和 used_hpages 成员组合可以有已下集中情况:

* count = 1 && used_hpages = 0: Subpool 正在使用但池子中的大页没有进程在使用
* count = 1 && used_hpages > 0: Subpool 正在使用且池子中的大页有进程在使用
* count = 0 && used_hpages = 0: Subpool 没有在使用并且池子中的大页没有进程在使用
* count = 0 && used_hpages > 0: Subpool 没有使用但有进程还在使用池子中的大页

###### max_hpages

max_hpages 成员用于说明 hugetlbfs 文件系统挂载点最多可以使用大页数量，其值有两种，当 max_hpages 等于 -1 表示没有对 hugetlbfs 文件系统挂载点的最大可使用大页数量进行限制，即内核大页内存池子中有多少可用大页，subpool 就能使用多少大页; max_hpages 的另外一种值是一个大于 0 的整数值，用于描述 hugetlbfs 文件系统挂载点最多可以使用大页的数量。当 max_hpages 大于 0 时，即 hugetlbfs 文件系统挂载点设置了最多可使用的大页数量时，如果进程所使用的大页数量超过 max_hpages, 那么映射将失败。如果 struct hugepage_subpool 的 min_hpages 不为零，那么 hugetlbfs 文件系统挂载点的 subpool 预留一定数量的大页，那么此时系统优先从 subpool 的 resv_hpages 中分配大页。hugepage_subpool_get_pages() 函数中可以看出 subpool 的分配策略:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001048.png)

在 hugepage_subpool_get_pages() 函数中，函数首先判断 max_hpages 是否不为 -1，如果不为 -1， 那么此时 used_pages 表示 subpool 已经从系统大页池子中分配使用的大页，如果此时 used_pages 加上即将分配的大页数量小于 max_hpages, 那么可以继续从 subpool 中分配大页，此时只需将 used_pages 引用计数增加即可; 反之如果 used_hpages 加上即将分配的大页数量已经超过 max_hpages, 那么系统将报 ENOMEM 错误导致 hugetlbfs 文件系统挂载点分配大页失败. 另外当 max_hpages 等于 -1 的时候，也就是不对 subpool 的最大大页数进行限制，只要系统大页池子有多少大页，subpool 就能分配多少大页。函数接下来样检测 min_hpages 和 resv_hpages 的值，如果 min_hpage 不等于 -1 且 resv_hpages 不为零，那么这种情况首先说明 hugetlbfs 文件系统挂载点提前预留了一定数量的大页，这些大页作为 subpool 私有的大页，那么内核优先从 subpool 的预留页中进行分配，如果预留页足够分配，那么 subpool 从 resv_hpages 中减去需要分配的页; 反之如果 subpool 的 resv_hpages 预留页少于需要分配的大页，那么 subpool 将全部的预留大页分配出去，然后不够的大页从系统大页池子中进行分配. max_hpages 的值需要在 hugetlbfs 文件系统挂载时指定，例如:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,size=8M,min_size=4M
{% endhighlight %}

如上面的例子，在挂载 hugetlbfs 文件系统时，使用参数 "size=" 字段可以指明挂载点最大可使用大页数量，"size=" 可以支使用 K/M/G 为单位的标准的长度模式，例如在上面的例子中 "size=8M", 那么 max_hpages 成员的值就是 4 个大页 (8M/2M); 也可以使用百分比的方式指明挂载点最大可用大页数量. 例如 "size=50%", 那么 max_hpaegs 成员的值等于 10 个大页 (nr_hugepages/2M).

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,min_size=4M
{% endhighlight %}

如上面的例子，在挂载 hugetlbfs 文件系统时，由于没有使用 "size=" 字段而只使用了 "min_size" 字段，那么 max_hpages 成员的值为 -1. 那么系统优先从 subpool 的预留大页中分配，如果预留大页中没有可用的大页，那么系统从大页池子中分配大页. hugetlbfs 文件系统如果要使用 subpool, 那么挂载的时候 "size=" 和 "min_size=" 字段必须存在其一.

###### used_hpages

used_hpages 成员用于在 hugetlbfs 文件系统挂载点限定最大使用大页数量之后，该值用于记录 subpool 大页使用数量，以防止 subpool 超额使用大页。当 subpool 分配一定数量的大页时，subpool 首先检测 used_hpages 的数量加上需要分配的大页数量是否已经超过 subpool 的上限 max_hpages, 如果是直接返回 ENOMEM; 反之如果 used_hpages 的数量加上需要分配的大页数量没有超过上限 max_hpages, 那么 subpool 可以提供大页。如果 hugetlbfs 文件系统挂载点没有设置最大使用大页量而只设置最小预留大页数量时，即 hugetlbfs 文件系统启用 subpool 大页小池子，那么这个时候由于 max_hpages 没有启用，那么 used_hpages 也不启用。hugetlbfs 文件系统如果要启用 used_hpages, 那么挂载参数如下:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,size=8M,min_size=4M
{% endhighlight %}

###### hstate

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

hstate 成员用于指向指定的大页，在 hugetlb 大页机制中，内核使用 struct hstate 维护指定长度的大页，那么不同的粒度都对应一个 struct hstate. subpool 可以通过 hstate 成员快速知道其使用的大页基础信息.

###### min_hpages

min_hpages 成员用于指明 hugetlbfs 文件系统挂载点至少预分配的大页数量。在 hugetlbfs 文件系统挂载时，如果指定了 "min_size=" 字段，那么系统大页内存池子必须为 hugetlbfs 文件系统挂载点的 subpool 小池子预留指定数量的大页。min_hpages 成员与 resv_hpages 成员搭配使用，hugetlbfs 文件系统在挂载时，系统根据 min_hpages 的值从系统大页池子中分配指定数量的大页，然后 hugetlbfs 文件系统挂载点的 subpool 将这些大页作为自己预留使用，同时将 resv_hpages 初始化预留的大页数。min_hpages 的值有两种情况，一种情况为 -1，即代表 hugetlbfs 文件系统挂载点的 subpool 没有是指预留水位线，即 subpool 需要包含大页的最小值，那么 subpool 只能从系统大页池子中分配; min_hpages 的另外一类值是大于 0 的值，即 hugetlbfs 文件系统在挂载时，系统就为 subpool 预留了指定数量的大页，当需要从 subpool 中分配内存，那么先从 resv_hpages 预留的大页中进行分配。hugepage_subpool_get_pages() 函数可以看出 subpool 的 min_hpages 分配策略:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001048.png)

在 hugepage_subpool_get_pages() 函数中，函数首先判断 max_hpages 是否不为 -1，如果不为 -1， 那么此时 used_pages 表示 subpool 已经从系统大页池子中分配使用的大页，如果此时 used_pages 加上即将分配的大页数量小于 max_hpages, 那么可以继续从 subpool 中分配大页，此时只需将 used_pages 引用计数增加即可; 反之如果 used_hpages 加上即将分配的大页数量已经超过 max_hpages, 那么系统将报 ENOMEM 错误导致 hugetlbfs 文件系统挂载点分配大页失败. 另外当 max_hpages 等于 -1 的时候，也就是不对 subpool 的最大大页数进行限制，只要系统大页池子有多少大页，subpool 就能分配多少大页。函数接下来样检测 min_hpages 和 resv_hpages 的值，如果 min_hpage 不等于 -1 且 resv_hpages 不为零，那么这种情况首先说明 hugetlbfs 文件系统挂载点提前预留了一定数量的大页，这些大页作为 subpool 私有的大页，那么内核优先从 subpool 的预留页中进行分配，如果预留页足够分配，那么 subpool 从 resv_hpages 中减去需要分配的页; 反之如果 subpool 的 resv_hpages 预留页少于需要分配的大页，那么 subpool 将全部的预留大页分配出去，然后不够的大页从系统大页池子中进行分配. min_hpages 的值需要在 hugetlbfs 文件系统挂载时指定，例如:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,size=8M,min_size=4M
{% endhighlight %}

如上面的例子，在挂载 hugetlbfs 文件系统时，使用参数 "min_size=" 字段可以指明挂载点最大可使用大页数量，"min_size=" 可以支使用 K/M/G 为单位的标准的长度模式，例如在上面的例子中 "size=4M", 那么 min_hpages 成员的值就是 2 个大页 (8M/2M); 也可以使用百分比的方式指明挂载点最大可用大页数量. 例如 "min_size=50%", 那么 min_hpaegs 成员的值等于 10 个大页 (nr_hugepages/2M).

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001129.png)

在 BiscuitOS 上直接挂载一个 hugetlbfs 文件系统，并为挂载参数 "min_size=" 字段的值设置为 4M，那么系统大页池子中将为其预留 2 个大页，此时查看大页池子的情况，可以看到 HugePages_Rsvd 字段为 2，那么系统为 hugetlbfs 文件系统挂载点的 subpool 预留了 2 个大页.

###### resv_hpages

当 hugetlbfs 文件系统挂载参数中指定了 "min_size=" 时，系统大页池子需要为其预留指定数量的大页，resv_hpages 用于维护 hugetlbfs 文件系统挂载点 subpool 的预留大页使用情况。当系统从 hugetlbfs 文件系统挂载点上分配大页时，如果 subpool 中 min_hpages 不为 -1，那么 subpool 优先检查其 resv_hpages 是否满足分配需求，如果满足那么从 subpool 的预留的大页中进行分配; 反之如果预留的大页无法满足分配需求，那么 subpool 优先从预留的大页中进行分配，剩余的从系统大页池子中进行分配. 分配过程中 resv_hpages 记录了 subpool 预留大页的剩余情况.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D012">struct hugetlbfs_config</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001130.png)

在 hugetlb 大页机制中，内核可以基于  hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存，因此进程在使用大页之前需要挂载一个 hugetlbfs 文件系统。内核支持在启动的时候为不同粒度的大页挂载 hugetlbfs 文件系统，以此为匿名映射提供大页，另外内核也支持用户空间挂载 hugetlbfs 文件系统，以此为文件映射提供大页。hugetlbfs 文件系统在挂载时支持一定的挂载参数，用于控制 hugetlbfs 文件系统挂载点的特征。struct hugetlbfs_config 数据结构用于存储 hugetlbfs 文件系统挂载时的挂载参数，以此共给 hugetlbfs 文件系统内部使用，其各成员的含义如下:

###### hstate

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugetlb 大页机制，内核使用 struct hstate 数据结构委维护指定长度的大页，hstate 成员用于指定 hugetlbfs 文件系统挂载点所使用的大页池子。通过该成员建立了 struct hugetlbfs_config 到 struct hstate 的映射关系.

###### max_hpages

max_hpages 成员用于指明 hugetlbfs 文件系统挂载点支持最大大页数量，hugetlbfs 文件系统在挂载时，可以使用 "size=" 字段指明该挂载点具有最大大页内存的长度。如果 hugetlbfs 挂载点限制了最大大页数量，那么其 subpool 将被启用。hugetlbfs 文件系统挂载时的 "size=" 字段的最大长度转换成 struct hugetlbfs_config 的 max_hpages, 最终 struct hugetlbfs_config 将 max_hpages 转换成 hugetlbfs 文件系统挂载点的 subpool 的 max_hpages, 以此表示 subpool 最大大页数量上限。如果 max_hpages 成员要生效，可以参考如下挂载方式:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,size=8M
{% endhighlight %}

如上面的例子，在挂载 hugetlbfs 文件系统时，使用参数 "size=" 字段可以指明挂载点最大可使用大页数量，"size=" 可以支使用 K/M/G 为单位的标准的长度模式，例如在上面的例子中 "size=8M", 那么 max_hpages 成员的值就是 4 个大页 (8M/2M); 也可以使用百分比的方式指明挂载点最大可用大页数量. 例如 "size=50%", 那么 max_hpaegs 成员的值等于 10 个大页 (nr_hugepages/2M). struct hugetlbfs_config 的转换主要在 hugetlbfs_fill_super() 函数中，可以参考其实现:

> [hugetlbfs_fill_super](#D02051)

###### nr_inodes

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页，用户进程通过在 hugetlbfs 文件系统的挂载点下创建或打开一个大页文件，然后从进程的地址空间中分配一段虚拟内存用于映射大页文件。当进程向通过访问这段虚拟内存的方式间接使用大页内存时，由于系统没有建立虚拟内存到大页物理内存的物理内存的页表，那么会触发系统缺页中断。在缺页中断处理函数中，系统会从为其分配大页，然后建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程可以真正的通过访问虚拟内存来使用大页物理内存。由于上面的逻辑存在，那么 hugetlbfs 文件系统需要通过大页文件在中间进行中转，进程每次打开大页文件时 hugetlbfs 文件系统都会为其分配 struct file 来维护打开的大页，另外 hugetlbfs 文件系统在挂载点下创建的大页文件都唯一对应一个 struct inode, 而该 struct inode 可以被多个进程打开，每次被打开 hugetlbfs 都会为进程分配一个 struct file. 因此就形成了多个 struct file 对应一个 struct inode。hugetlbfs 文件系统的 struct inode 描述了其包含大页文件的数量，因此在 hugetlbfs 文件系统挂载时可以通过 "nr_inodes=" 字段设置最大文件数量，这里文件数量不是指问文件的打开数量，当 hugetlbfs 文件系统挂载时包含了 "nr_inodes=" 字段时，该字段将会在 hugetlbfs_parse_options() 函数传递给 struct hugetlbfs_config 的 nr_inodes 成员，系统最终将 struct hugetlbfs_config 的 nr_inodes 传递给 struct hugetlbfs_sb_info 的 max_inodes 成员，以此控制 hugetlbfs 文件系统挂载点最大文件数. 如果 nr_inodes 成员要生效，可以参考如下挂载方式:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,nr_inodes=20
{% endhighlight %}

如上面的例子，在挂载 hugetlbfs 文件系统时，使用参数 "nr_inodes=" 字段可以指明挂载点最大大页文件数量，以此表示该挂载点下最大可以创建 nr_inodes 个大页文件。另外指定注意的是 hugetlbfs 文件系统挂载是，挂载点本身作为 root 节点需要消耗一个 struct inode, 那么为了 hugetlbfs 文件系统创建出一个文件，那么 "nr_inodes=" 字段如果存在，那么其值必须大于 1. 当 hugetlbfs 文件系统挂载点的文件数量已经等于 "nr_inodes" 限定的值，如果继续尝试在 hugetlbfs 文件系统挂载点创建文件，那么将会导致文件创建失败。

###### min_hpages

min_hpages 成员用于指明 hugetlbfs 文件系统挂载点预先预留大页的数量，hugetlbfs 文件系统在挂载时，可以使用 "min_size=" 字段指明 hugetlbfs 文件系统在挂载时需要预留大页的数量。如果 "min_size=" 字段存在，那么 hugetlbfs 文件系统挂载点的的 subpool
 将被启用，那么 "min_size=" 字段将被存储在 struct hugetlbfs_config 的 min_hpages 成员里，以此在 hugetlbfs 文件系统挂载是系统为其预留 min_hpages 个大页，并且启用 subpool 小池子，另外 subpool 小池子的 min_hpages 被设置为 struct hugetlbfs_config 的 min_hpages. 如果 min_hpages 成员要生效，可以参考如下的挂载方式:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,min_size=2M
{% endhighlight %}

如上面的例子，在挂载 hugetlbfs 文件系统时，使用参数 "min_size=" 字段可以指明挂载点最大可使用大页数量，"min_size=" 可以支使用 K/M/G 为单位的标准的长度模式，例如在上面的例子中 "size=2M", 那么 min_hpages 成员的值就是 1 个大页 (8M/2M); 也可以使用百分比的方式指明挂载点最大可用大页数量. 例如 "min_size=50%", 那么 min_hpaegs 成员的值等于 10 个大页 (nr_hugepages/2M). 值得注意的是，hugetlbfs 文件系统在挂载时指定了预留大页数量，如果此时系统大页池子没有足够大页内存时会导致 hugetlbfs 文件系统挂载失败。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D013">struct hugetlbfs_sb_info</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001131.png)

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存，用户进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程地址空间分配一段虚拟内存用于映射大页文件。当进程访问这段虚拟内存时，由于虚拟内存还没有与大页的物理内存建立页表，因此会触发系统缺页中断，在缺页中断处理函数中，系统会为其分配大页并建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程可以通过访问虚拟内存间接的使用大页内存。从上面的分析可知如果系统向用户空间提供大页内存，那么必须基于 hugetlbfs 文件系统，hugetlbfs 文件系统是一个没有后端存储的虚文件系统，通过文件 page cache 的方式可以为用户进程提供大页，因此在使用 hugetlb 大页之前，系统需要挂载 hugetlbfs 文件系统。hugetlbfs 文件系统的挂载分两类，第一类是内核启动过程中内核挂载的 hugetlbfs 文件系统，这类文件系统主要用于匿名映射的大页; 另外一类是用户空间挂载的 hugetlbfs 文件系统，这类文件系统主要用于文件映射的大页。hugetlbfs 文件系统也是基于 VFS 的文件系统，那么其也通过一个 struct super_block 进行描述，并提供相应的 VFS 接口, 另外 struct super_block 数据结构也包含了每种文件系统的私有描述，struct hugetlbfs_sb_info 就是用于描述 hugetlbfs 文件系统的私有描述，用于指定 hugetlbfs 文件系统的指定特定，其成员描述为:

###### max_inodes/free_inodes

用户进程如果向使用大页的方法是在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，每个大页文件对应唯一的 struct inode，进程每当创建或打开该文件大页时，hugetlbfs 文件系统会为其分配一个 struct file. 另外每个 hugetlbfs 文件系统挂载点可以限制最大文件创建数量，而对于文件打开数没有限制，那么内核使用 struct hugetlbfs_sb_info 的 max_inodes 成员用来指明该 hugetlbfs 文件系统挂载点最多可以创建文件大页的数量, 另外使用 free_inodes 成员来统计当前 hugetlbfs 文件系统挂载点还可以创建大页文件的数量，那么在 hugetlbfs 文件系统挂载点下存在如下关系:

{% highlight bash %}
# Hugetlbfs mount
最大创建大页文件数 = max_inodes
可以创建大页文件数 = free_inodes
已经创建大页文件数 = max_indoes - free_indoes
{% endhighlight %}

hugetlbfs 文件系统挂载点的最大可以创建文件数在挂载时通过 "nr_inodes=" 字段进行指定，那么 struct hugetlbfs_sb_info 的 max_inodes 就有两类值: 第一类值是 hugetlbfs 文件系统挂载时通过挂载参数 "nr_inodes=" 进行指定，需要注意的是 hugetlbfs 文件系统挂载点的根节点本身也占用一个 struct inode, 因此使用 "nr_inodes=" 设置最大文件打开数时需要比预期值多一个，那么挂载可以参考如下命令:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K,nr_inodes=6
{% endhighlight %}

如上面的例子，在挂载 hugetlbfs 文件系统时，使用参数 "nr_inodes=" 字段指明挂载点最大可创建 5 个文件，如果创建文件超过 5 个会失败。max_inodes 的另外一类值就是在 hugetlbfs 文件系统挂载时不指定 "nr_inodes=" 参数，那么 hugetlbfs 文件系统的挂载点下没有创建大页文件的限制，进程可以创建 VFS 支持的最大文件数. 那么挂载可以参考如下命令:

{% highlight c %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K
{% endhighlight %}

对于 free_inodes 成员的统计逻辑，其初始值是在 hugetlbfs 文件系统挂载时初始化为与 max_inodes 成员一样的值。如果 max_inodes 成员没有使用那么 free_inodes 也没有启用。每当进程在 hugetlbfs 文件系统挂载点下新建立一个大页文件时，free_inodes 的计数就减一，相反当在 hugetlbfs 文件系统挂载点下移除一个大页文件时，free_inodes 的计数将加一.

###### hstate

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000991.png)

在 hugetlb 大页机制，内核使用 struct hstate 数据结构委维护指定长度的大页，hstate 成员用于指定 hugetlbfs 文件系统挂载点所使用的大页池子。通过该成员建立了 struct hugetlbfs_sb_info 到 struct hstate 的映射关系.

###### spool

在 hugetlb 大页机制中，内核使用 struct hstate 描述指定长度的大页内存池子，内核可以从大页内存池子中分配大页，不同粒度的大页都对应一个 struct hstate 数据结构，因此系统会为不同粒度的大页维护不同的内存池子。hugetlbfs 文件系统也支持私有大页内存池子，其使用 struct hugepage_subpool 数据结构维护该私有池子，其工作原理是在 hugetlbfs 文件系统挂载时从指定长度的大页内存池子中预留一部分大页，这部分大页只能该 hugetlbfs 文件系统挂载点使用，其他 hugetlbfs 文件系统挂载点或系统的大页池子都无法使用这部分大页。当进程在该 hugetlbfs 文件系统挂载点下分配大页内存，挂载点优先从其 subpool 私有池子中分配大页，如果 subpool 池子中没有足够的大页时才从系统的大页内存池子中分配大页; 当挂载点下的文件摧毁时，其对应的大页将被 subpool 私有池子回收，subpool 回收时只有满足需要预留的大页之后才会将剩余的大页释放回系统大页内存池子里. 因此 struct hugetlbfs_sb_info 的 spool 成员指向其私有的内存池子，并且一个 hugetlbfs 文件系统挂载点最多只有一个 subpool; hugetlbfs 文件系统挂载点也可以不使用任何 subpool, 那么 subpool 为 NULL. 具体的 spool 描述可以参考:

> [struct hugepage_subpool 完全解析](#D011)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="D014">struct hstate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001132.png)

在 hugetlb 大页机制中，内核对大小为 4KiB 物理内存块称为小页，另外把大于 4KiB 的物理内存块称为大页。由于不同的架构支持的大页粒度有所不同，在 X86 架构中，大页的粒度可以为 2MiB 和 1Gig。内核使用 struct hstate 数据结构维护指定长度的大页，因此每一种粒度的大页都对应一个 struct hstate 数据结构。struct hstate 抽象为大页内存池子，内核将所有的大页都维护在内存池子了，进程需要分配大页时，内核就从这个内存池子中分配大页，当进程不需要大页时，可以将大页归还大页内存池子。另外内核可以基于 hugetlbfs 文件系统以 page cache 的方式为用户空间分配大页，进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程的地址空间中分配一段虚拟内存用于映射大页文件，当进程访问虚拟内存时，由于进程的虚拟内存还没有与大页的物理内存建立页表，那么触发系统的缺页中断。在缺页中断处理函数中，系统从指定长度的大页内存池子中分配一个大页，然后建立虚拟内存到大页物理内存的页表，待缺页中断返回之后，进程就可以通过访问虚拟内存间接使用大页。当进程使用完大页之后将大页文件摧毁时，系统回收大页并重新放回指定长度的大页内存池子中.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001133.png)

struct hstate 数据结构有三部分组成，第一部分是基础信息部分，这部分用于记录大页内存池子的名字 name[], 大页的长度相关的信息 order，大页对应的掩码 mask; 第二部分则是计数部分，用于管理大页内存池子的使用情况，大页存在多种状态，分别是: 空闲状态、预留状态、超发状态、以及激活状态等. 计数部分就是用来统计大页内存池子中不同状态的大页数量，另外结合 NUMA 架构，统计时也对不同的 NUMA NODE 进行统计; 第三部分为链表区域，该区域维护的链表上就是一个一个大页，大页处于不同的状态会被移动到不同的链表上，以便 hstate 统一管理，另外结合 NUMA 架构，每个 NUMA NODE 上也会维护一个链表，用于将同 NUMA NODE 且同状态的大页维护在链表上，另外一个大页同一时刻只能在一个链表上。那么接下来对 struct hstate 的成员进行讲解:

###### order

在 hugetlb 大页机制中，内核使用 struct hstate 数据结构维护指定长度的大页，大页的粒度可以是 2MiB 或 1Gig，那么 order 成员用来指明 struct hstate 数据结构维护大页的长度的信息，不过这个长度信息不是直接的长度，而是首先计算 1 个大页是由多少个 4KiB 的小页组成，然后使用以 2 为底 order 为指数的幂来表示使用小页的个数，因此 order 的计算方式如下:

{% highlight bash %}
# 2MiB 大页
nr_4KiB   = 2MiB / 4KiB = 512
nr_4KiB   = 2^order
2^order   = 512
order     = 9
Size_2MiB = (1 << 9) * 4KiB
Size_2MiB = 0x200000

# 1Gig 大页
nr_4KiB   = 1Gig / 4KiB = 262144 = 0x40000
nr_4KiB   = 2^order
2^order   = 0x40000
order     = 18
Size_1Gig = (1 << 18) * 4KiB
Size_1Gig = 0x40000000
{% endhighlight %}

对于 2MiB 大页，其需要 512 个 4KiB 小页组成, 那么用于描述 2MiB 的 struct hstate 数据结构的 order 为 **9**; 同理对于 1Gig 大页，其需要 262144 个 4KiB 小页组成，那么用于描述 1Gig 的 struct hstate 数据结构的 order 为 **18**.

###### mask

mask 成员是大页的对齐掩码，对于 2MiB 粒度的大页，mask 的值为 0xffffffffffe00000，对于 1Gig 粒度的大页，mask 的值为 0xC0000000. mask 的计算方式如下:

{% highlight bash %}
# Type of mask
mask = ~((1ULL << (order + PAGE_SHIFT)) - 1)

# 2MiB 大页
mask = ~((1ULL << (order + PAGE_SHIFT)) - 1)
mask = ~((1ULL << (9 + 12)) - 1)
mask = ~((0x200000) - 1)
mask = 0xffffffffffe00000

# 1Gig 大页
mask = ~((1ULL << (order + PAGE_SHIFT)) - 1)
mask = ~((1ULL << (18 + 12)) - 1)
mask = ~((0x40000000) - 1)
mask = 0xffffffffc0000000
{% endhighlight %}

mask 经常被用于对虚拟地址按大页粒度向下对齐，或者用于获得大页地址的内部偏移，或者对 vm_pgoff 进行对齐。

###### name

name 成员用于表示大页内存池子的名字，内核在调用 hugetlb_add_hstate() 函数初始化一个指定长度大页的 struct hstate 时对 name 进行赋值。对于 2MiB 粒度的大页内存池子，其名字为 **hugepages-2048kB**, 对于 1Gig 粒度的大页内存池子，其名字为 **hugepages-1048576kB**.

###### nr_huge_pages

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001141.png)

nr_huge_pages 成员用于指明在指定长度的大页内存池子中总共包含大页的数量。当 struct hstate 数据结构在 hugetlb_add_hstate() 函数初始化时，nr_huge_pages 初始化为 0.系统可以通过三种方法增加大页内存池子中大页的数量: 第一种是通过 CMDLINE 在启动时为大页内存池子分配指定数量的大页; 第二种是通过 /sys/kernel/mm/hugepages/hugepages-X/nr_hugepages 提供的接口主动扩容; 第三种是通过大页的超发机制动态的增加大页内存池子中大页数量。无论采用那种方式都会增加指定长度大页的 struct hstate 数据结构 nr_huge_pages 成员的值。另外系统可以通过两种方法减少内存池子中大页的数量: 第一种是通过 /sys/kernel/mm/hugepages/hugepages-X/nr_hugepages 提供的接口主动缩容; 第二种是在存在通过超发机制添加的大页情况下，系统释放大页会触发大页内存池子的缩容。两种方式的缩容都会减少指定长度大页的 struct hstate 数据结构 nr_huge_pages 成员的值. 系统提供了 /proc/meminfo 接口可以动态查看 nr_huge_pages 的值:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001134.png)

/prc/meminfo 的 HugePages_Total 字段即 struct hstate 数据结构的 nr_huge_pages 成员，通过上面的大页内存池子的扩容和缩容，可以看到 HugePages_Total 字段都在跟随变化，因此可以通过 HugePages_Total 成员获得指定长度大页池子的大页总数. 最后可以通过 /sys/kernel/mm/hugepages/hugepages-X/nr_hugepages 查看当前指定长度大页内存池子中大页总数.

###### max_huge_pages

max_huge_pages 成员用于指明指定长度大页内存池子中固定大页的数量, 所谓固定大页就是一次性分配的多个大页，并一直维护在大页内存池子中的大页。相对固定大页，对可动态分配和动态回收的大页称为超发大页。大页内存池子中的大页根据来源可以分作两类，第一类是通过 CMDLINE 内存启动时预先分配的大页，或者通过 /sys/kernel/mm/hugepages/hugepages-X/nr_hugepages 扩容的大页称为固定大页 (Persistent HugePage); 第二类通过大页超发机制动态分配的大页，这里大页称为超发大页 (Surplus HugePage). struct hstate 数据结构使用 nr_huge_pages 表示内存池中大页的总数，而 surplus_huge_pages 成员表示超发的大页数量，那么三者之间的关系是:

{% highlight bash %}
# 大页内存池子大页总数
nr_huge_pages = max_huge_pages + surplus_huge_pages
# 大页内存池子中固定大页的总数
nr_Persistent = nr_huge_pages - surplus_huge_pages
# 大页内存池子中超发大页的总数
nr_Surplus    = nr_huge_pages - max_huge_pages
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001135.png)

接下来用一个实例进行讲解，在实例中通过文件映射和匿名映射分别分配 2 个大页，并使用 4 个大页，停留 10s 之后释放虚拟内存，并关闭文件。如果上面的程序执行成功，那么在 "/mnt/BiscuitOS-hugetlbfs/" 目录下还存在 hugepage 文件，那么接下来使用如下命令进行测试:

{% highlight bash %}
mkdir -p /mnt/BiscuitOS-hugetlbfs/
echo 2 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
echo 2 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_overcommit_hugepages
mount -t hugetlbfs none /mnt/BiscuitOS-hugetlbfs/ -o pagesize=2048K
{% endhighlight %}

命令首先创建目录 "/mnt/BiscuitOS-hugetlbfs/", 然后通过 "nr_hugepages" 接口分配两个固定大页，然后通过 "nr_overcommit_hugepages" 接口设置系统可以超发 2 个大页，最后挂载一个 hugetlbfs 文件系统在 "/mnt/BiscuitOS-hugetlbfs/" 目录下，此时系统 2MiB 的大页内存池子中大页总数为 2. 那么接下来运行程序:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001136.png)

当系统启动完毕之后，向 nr_hugepages 写入 2 以此分配两个固定大页，接着向 nr_overcommit_hugepages 写入 2 以此支持动态分配 2 个大页，但此时没有分配超发的大页，因此查看 /proc/memcinfo 时 HugePages_Surp 为 0，且 HugePages_Total 为 2; 当程序运行的前 10s，此时程序通过文件映射和匿名映射总共分配了 4 个大页，由于此时固定大页只有 2 个大页，那么剩余的两个通过动态分配两个超发大页，那么此时查看 /proc/meminfo 可以看到 HugePages_Surp 为 2，且 HugePages_Total 为 4; 应用程序运行 10s 之后，应用程序将文件映射和匿名映射的虚拟内存解除映射，然后关闭文件并退出程序，此时超发大页将动态回收，但不是所有的超发大页一次行回收，其会预留一部分超发大页作为空闲大页待下一次分配使用，另外由于大页文件还存在没有被摧毁，那么被大页文件占用的大页还在使用中，因此此时查看 /proc/meminfo 可以看到 HugePages_Surp 的数量为 1，并且这个超发大页处在空闲状态，那么 HugePages_Free 为 1，最后此时 HugePages_Total 的值为 3，那么大页内存池子中包含 3 个大页.

###### free_huge_pages

free_huge_pages 成员用于表示指定长度大页内存池子中空闲大页的数量，空闲但不代表可用。在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页，用户进程可以通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程的地址空间分配一段虚拟内存用于映射大页文件，此时系统会为这段映射大页的虚拟内存预留一定数量的大页，这些大页就是预留大页，但这些大页因为没有被真正使用，所以属于空闲大页，但这些大页只能用于这段映射大页文件的虚拟内存，不能用于其他进程，因此虽然这些大页是空闲的但属于不可再分配的大页。当进程访问这段虚拟内存时，由于没有建立虚拟内存到大页物理内存的页表，那么会触发系统缺页中断。在中断处理函数中，内核会从预留大页中找到一个大页，这个大页此时是空闲状态，然后建立虚拟内存到该大页物理内存的页表，并将这个大页的状态修改为激活态度，然后系统会减少大页内存池子中空闲大页数量和预留大页数量各自减一，待缺页中断返回之后，进程可以通过访问这段虚拟内存间接使用大页。通过上面的分析大页内存池子中存在如下关系:

{% highlight bash %}
# 可分配大页数量
nr_Alloc_HugePage  = free_huge_pages - resv_huge_pages
# 正在使用的大页数量
nr_Active_HugePage = nr_huge_pages - free_huge_pages
{% endhighlight %}

通过上面的讨论可以知道，由于进程分配虚拟内存映射大页文件时，默认情况下系统需要在真正使用前为这段虚拟内存预留足够的大页，否则映射会失败，在大页未真正使用之前预留的大页不能被用于其他用途，但此时大页的状态是空闲的，因此对于大页内存池子可分配的大页数量为空闲大页数减去预留大页数，因此 free_huge_pages 的大页集合不一定都可分配。在大页内存池子中抛除空闲的大页就是正在使用的大页，或称为激活态的大页，那么正在使用的大页等于大页内存池子大页总数 nr_huge_pages 减去空闲大页 free_huge_pages.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001137.png)

接下来通过一个实例验证上面的说法，实例首先分配一段虚拟内存映射大页，然后等待 10s，接着正在使用大页，然后停留 10s，最后释放映射的虚拟内存。使用如下命令进行测试:

{% highlight bash %}
echo 2 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
cat /proc/meminfo | grep Huge
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001138.png)

系统启动完毕之后，向 nr_hugepages 接口写如 2 以便分配两个固定大页，此时查看 /proc/meminfo, 可以看到 HugePages_Total 为 2，以此表示大页内存池子中总共两个大页，另外 HugePages_Free 为 2，而 HugePages_Rsvd 为 0，那么大页内存池子中空闲 2 个大页，并且可分配 2 个大页。当程序运行是，进程只是分配一段虚拟内存映射大页文件，并未建立页表，那么此时系统会为这段虚拟内存预留 2 个大页，因此此时 /proc/meminfo 下的 HugePages_Rsvd 为 2，另外这两个大页的状态是空闲的，因此此时 HugePages_Free 为 2，那么此时可分配的大页为 0. 接下来进程开始缺页然后正在使用大页，该大页从预留的池子中分离出来，并从原先的空闲态转变为激活态，那么此时 /proc/meminfo 下 HugePages_Free 的值为 1 个大页，并且 HugePages_Rsvd 也变成为 1，此时可分配的大页依旧为 0; 进程最后将使用完的大页归还给大页内存池子，此时大页由激活态变成了空闲态，但此时这些大页已经没有进程预留了，因此此时查看 /proc/meminfo 下 HugePages_Free 为 2，而 HugePages_Rsvd 为 0，那么可分配的大页数变成 2.

###### resv_huge_pages

resv_huge_page 成员用于表示指定长度大页内存池子中预留池子的大页数量。在 hugetlb 大页机制中，内核可以基于 hugelbfs 文件系统以文件 page cache 的方式为用户进程提供大页，进程通过在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，然后从进程地址空间分配一段虚拟内存用于映射大页文件，系统默认情况下会为映射大页文件的虚拟内存预留足够数量的大页，这些大页来自大页内存池子，当被预留之后，这些大页将进入预留池子中，只有预留大页的进程才能使用这部分大页。当进程访问这段虚拟内存时，由于虚拟内存到大页物理内存的页表不存在，那么触发系统的缺页中断。在缺页中断处理函数中，系统从大页预留池子中找一个大页，接着将大页移出大页预留池子，并将 resv_huge_pages 减一，然后建立虚拟内存到大页物理内存的页表，最后将大页的状态修改为激活态，并将大页放入大页激活链表里。待缺页中断返回之后，进程可以正常访问这段虚拟内存，并间接使用大页。内核使用 struct hstate 数据结构维护指定长度的大页内存池子，因此 resv_huge_pages 用于统计大页内存池子中预留池子的大小。rsve_huge_pages 成员与其他几个成员的关系:

{% highlight bash %}
# 可分配大页
nr_Alloc_HugePage = free_huge_pages - resv_huge_pages
{% endhighlight %}

当大页的状态处于空闲状态时，由于 hugetlb 预留机制的存在，那么空闲大页不一定是直接可以分配的大页，系统需要将空闲大页数量减去预留大页数量才是真正可以分配的大页数量. 另外进程分配虚拟内存映射大页文件时也可以不提前预留大页，可以在 mmap() 时使用标志 MAP_NORESERVE, 那么只有进程真正缺页使用大页内存时，系统才真正为其分配大页. 可以通过一下例子验证上面的说法:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001140.png)

验证的例子如上，程序被分作两部分，第一部分是进程通过 mmap 分配一段虚拟内存用于映射大页，这个时候并为真正使用，停留 10s 之后访问内存导致缺页，最终真正使用大页，之后再次停留 10s，接着将虚拟内存解除映射，至此第一部分完成。第二部分的逻辑与第一部分一致，唯一的不同点是在 mmap 分配虚拟内存是新增了 MAP_NORESERVE 标志，那么接下来使用命令进行实例测试:

{% highlight bash %}
echo 2 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
cat /proc/meminfo | grep Huge
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001139.png)

从实际的运行可以看出，当向 nr_hugepages 接口分配 2 个固定大页时，/proc/meminfo 的 HugePages_Total 和 HugePages_Free 为 2，HugePages_Rsvd 为 0，那么此时大页内存池子可以提供两个大页用于分配。当第一部分执行是，首先调用 mmap 分配虚拟内存之后，由于预留机制的存在，那么系统会为这段虚拟内存预留大页，此时虚拟内存长度为 2 个大页，那么预留大页的数量为 2，此时查看 /proc/meminfo，HugePages_Free 和 HugePages_Rsvd 的值都是 2，那么此时大页内存池子可提供分配的大页数量为 0，HugePages_Total 的 2 个大页都被进程给预留了，虽然 2 个大页此时还是空闲状态，但还是大页内存池子无法提供任何大页。接着进程访问虚拟内存触发缺页中断，缺页中断处理程序从预留的大页池子中获得一个大页，并建立虚拟内存到大页物理内存的页表，以此同时将大页预留池子大页数减一，然后将大页的状态有空闲态转换成激活态，此时查看 /proc/meminfo 的信息，HugePages_Free 和 HugePages_Rsvd 的数都变成了 1，因此 HugePages_Active 为 1， 此时大页内存池子可提供分配的大页数量任然为 0. 第一部分的最后操作是将映射大页的虚拟内存释放，那么此时这段虚拟内存预留的大页也解除了预留，进程正在使用的大页也被系统回收放入系统大页内存池子中，此时查看 /proc/meminfo 的信息，HugePages_Free 变成了 2，但 HugePages_Rsvd 变成 0，此时系统大页内存池子可提供的分配的大页数量为 2，正在使用的大页数也变成了 0.

对于第二部分操作，逻辑大体与第一部分一致，在进程开始第二部分之前，/proc/meminfo 信息显示 HugePages_Total 和 HugePages_Free 为 2，其余为 0，那么系统大页内存池子可向外提供分配大页的数量为 2. 进程开始调用 mmap 分配虚拟内存映射大页文件，由于 mmap 时带有 MAP_NORESERVE 标志，那么系统在该阶段不会为映射大页文件的虚拟内存预留大页，那么此时 /proc/meminfo 的信息显示 HugePages_Free 为 2，而 HugePages_Rsvd 为 0，那么系统确实没有为这段虚拟内存预留大页，那么此时系统大页内存池子确实可以向外提供分配的大页数量为 2. 进程接着访问这段虚拟内存并触发缺页中断，在缺页处理函数中，函数直接从系统大页内存池子中获得一个可分配的大页，由于之前有两个可分配的大页，那么缺页中断不会因为没有大页而失败，此时缺页处理程序建立虚拟内存到大页物理内存的页表，并将大页的状态由空闲转换为激活状态，待缺页中断返回之后，进程就可以使用该大页了，此时查看 /proc/meminfo 信息，HugePages_Free 变为 1，当 HugePages_Rsvd 仍然为 0，那么此时系统大页池子可以向外提供分配大页数量为 1. 第二部分的最后一个操作就是释放这段虚拟内存，此时正在使用大页会被系统回收，并将大页的状态由激活态转换为空闲态，并加入系统大页内存空闲池子中，此时查看 /proc/meminfo 的信息可以知 HugePages_Free 变成了 2，但 HugePages_Rsvd 仍为 0，那么此时系统大页内存池子可以向外提供分配大页的数量为 2. 通过上面实践例子的分析结果可以验证之前的说法.

###### nr_overcommit_huge_pages

nr_overcommit_hugepages 成员用于指明指定长度大页能否超发大页的数量，大页超发机制是相对固定分配机制而言的，在默认的情况下，系统大页内存池子中的大页需要通过 CMDLINE 或 /sys/kernel/mm/hugepages/hugepages-X/nr_hugepages 接口指定并一次性分配，如果没有显示的缩容，那么这些大页空闲的时候都一直待在大页内存池子了，这样的策略存在有一定的好处，那么就是确保系统大页内存池子可以为系统提供固定数量的大页，不好的地方就是在系统内存紧张的时候，系统大页内存池子却有很多空闲的大页，这些大页占用了很大部分的内存系统却不能使用，另外一个缺点就是当大页内存池子中没有空闲的大页，当系统还有很多空闲大页，那么大页内存池子还是无法向外提供大页。为了解决这个问题大页引入了超发机制，所谓超发机制就是设置一个上限值，当系统大页内存池子没有足够的大页时，系统可以从 Buddy 分配器中动态分配大页，然后将其添加到系统大页内存池子中，当进程不再使用大页将其归还给大页内存池子时，系统会检查其是否为超发的大页，如果是且现在大页内存池子空闲大页相对充裕或者系统内存相对紧张，那么系统将超发的大页重新归还给 Buddy 分配器。hugetlb 大页机制向外提供了接口，以便用户配置大页超发数量，其使用如下:

{% highlight bash %}
# HugePage Overcommint Interface (hugepages-X 为 2MiB 或 1Gig 的目录)
/sys/kernel/mm/hugepages/hugepages-X/nr_overcommit_hugepages
# Current Overcommit
cat /sys/kernel/mm/hugepages/hugepages-X/nr_overcommit_hugepages
# Setup Overcommit
echo NR_OverCommit > /sys/kernel/mm/hugepages/hugepages-X/nr_overcommit_hugepages
# Expand Overcommit Space
# NEW_OC > Default_OC
echo NEW_OC > /sys/kernel/mm/hugepages/hugepages-X/nr_overcommit_hugepages
# Shrink Overcommit Space
# NEW_OC < Default_OC
echo NEW_OC > /sys/kernel/mm/hugepages/hugepages-X/nr_overcommit_hugepages
{% endhighlight %}

系统提供了 nr_overcommit_hugepages 接口，该接口可读可写。当读取该接口时可以获得当前指定长度大页内存池子中允许超发的大页数量; 当向接口写入值时用于设置指定长度大页内存池子新的超发大页数量。另外值得注意的是，如果写入的值大于当前值，那么相当于大页内存池子的扩容，反之如果写入的值小于当前值，那么相对于大页内存池子的缩容，但释放的大页均来自超发的大页。另外如果缩容的时候没有达到预期，即没有其他超发大页可以释放，并且需要缩容超发的大页还在被使用，那么系统会等到该超发大页释放并回收成功之后 nr_overcommit_hugepages 才更新为预期的值，否则为实际缩容的值.

###### suplus_huge_pages

suplus_huge_pages 成员用于表示指定长度大页内存池子中通过超发机制动态分配的大页数量。所谓超发大页就是当系统大页内存没有可用的大页时，系统会从 Buddy 分配器动态分配内存构成大页，并填充到大页内存池子内，struct hstate 数据结构使用 surplus_huge_pages 成员来统计超发大页的数量，并使用 nr_overcommit_huge_pages 成员限制了系统最大可以分配超发大页的数量，那么当大页内存池子使用了 nr_overcommit_huge_pages 个大页之后，那么大页内存池子将没有能力向外提供可分配的大页。超发大页与固定大页在大页池子中没有特殊的区分，都是作为一个大页来看待。相对于固定大页，超发大页由于通过从 Buddy 分配器中动态分配，那么超发大页分配的时候耗时会比较多一些，另外由于从 Buddy 分配器动态分配，可能受内存碎片的影响，Buddy 分配器可能找不到连续物理内存组成大页。当系统大页内存池子中存在较多空闲大页时，系统通过将新释放的大页归还给 Buddy，以此减少超发大页的数量。另外系统也可以强制大页内存池子释放超发的大页，但无法强制释放固定大页。surplus_huge_pages 与其他几个成员之间的关系:

{% highlight bash %}
# Total HugePages: nr_huge_pages
nr_huge_pages      = max_huge_pages + surplus_huge_pages
# Persistent HugePages
max_huge_pages     = nr_huge_pages - surplus_huge_pages
# Surplus HugePages
surplus_huge_pages = nr_huge_pages - max_huge_pages
# Limit
suplus_huge_pages <= nr_overcommit_huge_pages
max_huge_pages
{% endhighlight %}

系统大页内存池子中只有在统计上区分固定大页和超发大页，而实际中固定大页和超发大页没有任何区别。统计上大页内存池子中大页总数等于固定大页数量加上超发大页数量。/proc/meminfo 接口提供的信息可以知道大页内存池子的大页总数 HugePages_Total, 以此超发大页的数量 HugePages_Surp, 因此可以通过上面的公式获得大页内存池子中固定大页的数量。另外可以通过 /sys/kernel/mm/hugepages/ 目录下的 nr_overcommit_hugepages 获得可以超发大页的上限，以及 surplus_hugepages 接口可以获得当前超发大页的数量。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001142.png)

系统可以通过超发大页机制动态改变系统大页内存池子的大页的数量，以此达到大页内存池子的扩容和缩容。对于扩容只能在大页内存池子中固定大页消耗殆尽的时候才能进行扩容，而进程不再使用超发大页时，超发大页会立即归还 Buddy 分配器，以此达到大页内存池子缩容效果.

###### hugepage_freelists



![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------

<span id="KA"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Hugetlb/Hugetlbfs 与 CMDLINE 关系研究

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001169.png)

在 hugetlb 大页机制中，内核会提前分配一定数量的大页到系统大页内存池子里，并作为固定大页供给用户进程重复使用，只有主动释放这些大页，不然这些大页会一直存在大页内存池子里。系统获取大页的方式有两种，第一种通过 CMDLINE 中的特定字段进行设置，系统在启动初期从 CMDLINE 中捕获了 Hugetlb 相关的字段之后，接着申请一定数量的大页放到大页内存池子里; 另外一种获得固定大页的方式是系统提供了的两个接口来扩容或缩容大页内存池子，两个接口为:

{% highlight bash %}
# 不同粒度的大页
# size 为大页粒度按 KiB 的值
/sys/kernel/mm/hugepages/hugepages-{size}kB/nr_hugepages
# 默认粒度的大页
/proc/sys/vm/nr_hugepages
{% endhighlight %}

无论使用以上哪种方式，都会向指定粒度的大页内存池子中添加或缩减一定数量的固定大页。另外两种方式虽然都是添加了固定大页，但固定大页的来源却存在差异。对于 1Gig 粒度的大页，如果固定大页来自 CMDLINE，那么大页来自 MEMBLOCK 内存分配器; 而对于 /proc 或 /sys 接口分配大页，其来自 CMA 内存分配器。对于 2MiB 粒度的大页，无论那种方式，其来自 Buddy 分配的复合页转变而来。本节不对大页的来源进行讨论，而对 CMDLINE 方式提供的大页进行分配，并在多个架构中进行分析研究。

###### CMDLINE default_hugepagesz 字段

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001143.png)

default_hugepagesz 字段用于设置系统默认使用的大页粒度。当系统同时支持多种粒度的大页场景下，如果没有明确的指定使用哪种粒度的情况下，系统默认使用该粒度的大页，例如分配匿名大页时，如果没有在 mmap 中包含 MAP_HUGE_2MB 或者 MAP_HUGE_1GB 等字段时，匿名大页使用 default_hugepaegs 指定的大页。如果 CMDLINE 中没有使用 default_hugepagesz 字段，那么系统缺省使用某种粒度的大页作为默认大页，不同的架构缺省默认大页粒度不一致，例如 i386 架构缺省默认粒度大页为 4MiB，而 X86_64 和 ARM64 架构缺省默认粒度大页为 2MiB。系统为默认大页提供了 /proc/sys/vm/ 目录下的 nr_hugepages、nr_hugepages_mempolicy、nr_overcommit_hugepages 三个节点，分别管理默认大页内存池子 , default_hugepagesz 字段则可以指定默认大页池子。另外 /proc/meminfo 中展示了默认大页内存池子的信息，其中 Hugepagesize 表示默认大页的长度，HugePages_Total 表示默认大页池子中大页数量，HugePages_Free 表示默认大页池子中空闲没有使用的大页数量, HugePages_Rsvd 表示默认大页池子中预留大页的数量, HugePages_Surp 表示默认大页池子中超发大页的数量。由于 default_hugepagesz 可以指定默认粒度的大页，那么 hugetlbfs 文件系统挂载时没有显示指明使用哪种大页，那么可以使用 default_hugepagesz 字段控制这里 hugetlbfs 文件系统使用的大页粒度. 那么接下来通过实践例子验证上面的说法, 首先在 CMDLINE 添加 default_hugepagesz 字段，并指定大小，实践基于 BiscuitOS X86_64 架构进行讲解，首先在 BiscuitOS 的 RunBiscuitOS.sh 中 CMDLINE 中添加该字段:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001144.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001145.png)

当内核的 CMDLINE 中指定了 "default_hugepagesz=2M", 那么系统默认的 hugetlb 大页粒度为 2MiB，那么 /proc/sys/vm/ 目录下关于大页的配置节点都是指向 2MiB 粒度的大页，例如上图中在 /proc/sys/vm/nr_hugepages 节点写入 2，也就是使 2MiB 大页内存池子中固定大页的数量为 2，此时查看 /proc/meminfo 就可以看到 HugePages_Total 和 HugePages_Free 的值都是 2. 并且 Hugepagesize 的值为 2MiB。最后挂载一个 hugetlbfs 文件系统，此时没有指定其使用大页的粒度，因此挂载点使用默认的大页粒度，也就是 2MiB，通过 mount 命令可以看到 hugetlbfs 文件系统挂载点的 pagesize 大小为 2MiB。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001146.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001147.png)

当内核的 CMDLINE 中指定了 "default_hugepagesz=1G", 那么系统默认的 hugetlb 大页粒度为 1Gig，那么 /proc/sys/vm 目录下关于大页的配置节点都值针对 1Gig 粒度的大页内存池子，例如上图中在 /proc/sys/vm/nr_hugepages 节点写入 1，那么系统将 1Gig 粒度大页池子中固定大页的数量控制在 1 个大页，此时查看 /proc/meminfo 就可以看到 HugePages_Total 和 HugePages_Free 的值都是 1，并且 Hugepagesize 为 1Gig。最后挂载一个 hugetlbfs 文件系统，此时没有指定其使用的大页粒度，因此挂载点使用默认的大页粒度，也就是 1Gig，并通过 mount 命令可以看到 hugetlbfs 文件系统挂载点 pagesize 大页为 1Gig.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001148.png)

如果内核的 CMDLINE 中没有指定 default_hugepagesz 字段，那么不同的架构默认大页粒度可能不同，那么接下来对几个常见的架构进行实践分析:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001149.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001150.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001151.png)

通过 BiscuitOS 上运行不同的架构的 /proc/sys/vm/nr_hugepages 节点写入 10，以此向系统默认的大页内存池子中调整固定大页的数量为 10，接着查看各个架构此时 /proc/meminfo 中关于大页统计信息的描述，可以通过 Hugepagesize 字段看出默认大页的长度，那么总计几个架构特点如下:

* i386 架构默认大页粒度是 4MiB
* X86_64 架构默认大页粒度是 2MiB
* ARM64 架构默认大页粒度是 2MiB

最后还需要研究的就是 default_hugepagesz 字段在不同架构下能够设置哪些值，根据不同架构的硬件页表的描述，支持大页的粒度有所不同，总结为:

* i386 架构支持: 4MiB
* X86_64 架构支持: 1Gig/2MiB
* ARM64 架构支持: 64KiB/2MiB/32MiB/1Gig

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001158.png)

接下来是实践 default_hugepagesz 字段干预匿名大页选择不同大页粒度的场景，实践使用一个简单的程序，程序首先通过 mmap 分配一段虚拟内存映射匿名大页，虚拟内存的长度为 4MiB，接着使用这段虚拟内存，但最后使用 sleep(-1) 不释放虚拟内存，此时观察系统默认大页内存使用情况，另外为了更好的说明问题，在 CMDLINE 中同时支持 2MiB 和 1Gig 粒度的大页，然后通过 default_hugepagesz 控制匿名大页使用不同粒度的大页:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001159.png)

从实践结果可以看出，当内核 CMDLINE 中 default_hugepagesz 字段值为 2M 时，并且同时存在 1Gig 和 2MiB 粒度的大页内存池子，此时通过 /proc/meminfo 查看默认粒度大页的信息，Hugepagesize 为 2MiB 说明默认大页为 2MiB 粒度，HugePages_Total 和 HugePages_Free 为 10，说明 2MiB 粒度的大页池子 10 个大页空闲。接着运行程序分配匿名大页，此时匿名大页使用完毕之后并未释放，那么查看 /proc/meminfo 的值，可以看到默认粒度大页池子的 HugePages_Free 变成 9，且 HugePages_rsvd 变为 1，说明程序运行时的进程分配了 4MiB 虚拟内存预留了 2 个 2MiB 大页，并且其中一个大页被使用，而另外一个没有被使用保持预留状态，因此说明匿名大页使用了 default_hugepagesz 字段指定粒度的大页。那么接下来将进程杀死之后释放占用的大页，此时查看 /proc/meminfo 默认粒度大页可用大页又变回 10 个大页。那么接下来将 default_hugepagesz 字段修改为 1G:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001160.png)

当内核 CMDLINE 的 default_hugepagesz 字段修改为 1G 之后，并且同时存在 1Gig 和 2MiB 粒度的大页内存池子，此时通过 /proc/meminfo 查看默认粒度大页的信息，Hugepaegsize 为 1Gig 说明默认大页已经是 1Gig 粒度，HugePages_Total 和 HugePages_Free 为 1，说明 1Gig 粒度的大页池子 1 个大页空闲。接下来运行程序分配匿名大页，此时匿名大页使用完毕之后并未释放，那么查看 /proc/meminfo 的值，可以看到默认粒度大页内存池子的 HugePages_Free 变成 0，那么此时默认粒度大页内存池子中没有可供分配的大页。程序运行时的进程只是分配了 4MiB 的虚拟内存映射匿名大页，那么默认粒度大页池子就会预留一个大页，当进程使用这个大页时，默认粒度大页池子预留数量减一，可用大页数量也减一，那么符合当前 /proc/meminfo 展示的默认大页内存池子状态。最后就是杀掉进程，进程将占用的大页释放会默认粒度大页池子，此时可以从 /proc/meminfo 中获得默认粒度大页池子的可用大页已经为 1。从上面两个验证实验可以得出，在不改变分配匿名大页代码逻辑的前提下，如果映射大页采用默认粒度的大页，那么内核 CMDLINE 的 default_hugepagesz 字段确实可以控制匿名大页的粒度。那么放过来问，匿名大页如果控制使用指定粒度的大页呢? 这里做简单的介绍，在通过匿名映射大页时，可以在 mmap() 函数中使用 MAP_HUGE\_\{粒度\} 的标志, 如下图，标志定义在头文件 "linux/mmap.h" 中，当匿名映射大页时，可以使用下图标志映射指定粒度的大页: 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001161.png)


---------------------------------------------

###### CMDLINE hugepagesz/hugepages 字段

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001152.png)

hugepagesz 字段和 hugepages 字段搭配使用，用于告诉系统启动时创建一个 hugepagesz 粒度的大页池子，并且池子中包含 hugepages 个大页。hugepagesz 字段用于指明大页的粒度，hugepages 用于指明大页的数量，两者通常搭配使用，另外 CMDLINE 中可以同时存在多对 hugepagesz/hugepages。当 hugepagesz/hugepages 字段生效后，系统会在 /sys/kernel/mm/hugepage/ 目录下为其创建名为 hugepages-{size}KB 的目录，其中 size 为大页粒度按 KB 为单位的十进制值。每个目录下是关于该粒度大页池子中的管理数据，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001153.png)

free_hugepages 节点用于指明该粒度大页池子中没有被使用的大页数量; nr_hugepages 节点用于指明该粒度大页池子中大页的数量; resv_hugepages 节点用于指明指定粒度大页池子中被预留大页的数量; surplus_hugepages 节点用于指明指定粒度大页池子中已经超发大页的数量; nr_overcommit_hugepages 节点用于指明指定粒度大页池子中能够超发大页的数量. 通过 hugepagesz/hugepages 字段创建一个大页内存池子之后，系统挂载 hugetlbfs 文件系统时就可以与指向该大页池子进行绑定，这样进程可以基于该 hugetlbfs 文件系统挂载点使用该粒度的大页。接下来在 BiscuitOS X86 架构中验证上面的说法, 开发者首先在 BiscuitOS 的 RunBiscuitOS.sh CMDLINE 中添加字段:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001154.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001155.png)

在 kernel 的 CMDLINE 中添加了 "hugepagesz=2M hugepages=10" 字段之后，系统启动之后会创建一个粒度为 2MiB 的大页池子，池子里面包含了 10 个固定大页，此时查看 /proc/meminfo 节点的信息，可以看到 HugePages_Total 和 HugePages_Free 为 10，并且此时系统默认大页长度 Hugepagesize 为 2MiB，如果这里不是 2MiB，需要在 kernel CMDLINE 中添加 "default_hugepagesz=2M" 的字段，以此 /proc/meminfo 节点显示 2MiB 大页池子信息. 接着 /sys/kernel/mm/hugepages 目录下存在 hugepages-2048KB 目录，该目录下的节点信息用于管理系统 2MiB 粒度大页池子。最后在挂载 hugetlbfs 文件系统时，可以设置挂载参数 "pagesize=2M", 以及 "min_size=16M", 这样这个挂载点就与 2MiB 大页池子挂钩，并且挂载点的 spool 小池子从 2MiB 大页池子中预留了 8 个大页。此时查看 /proc/meminfo 可以看到 2MiB 大页池子 HugePages_Rsvd 为 8。以上实践数据正好符合预期, 因此上面所述属实. 接下来讨论 CMDLINE 中同时存在多对 hugepagesz/hugepages 的情况:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001157.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001156.png)

在 kernel 的 CMDLINE 中添加 "hugepagesz=2M hugepages=10 hugepagesz=1G hugepages=1" 字段之后，系统启动之后会创建两个大页池子，其中一个大页池子的粒度是 2MiB 并且包含 10 个大页，另外一个大页池子的粒度是 1Gig 并且包含 1 个大页。此时可以看到 /sys/kernel/mm/hugepages/ 目录下存在 hugepages-2048KB 和 hugepages-1048576KB 两个目录，通过查看两个目录下的 nr_hugepages 和 free_hugepages 节点，都可以获得与 hugepagesz/hugepages 字段一样的值，那么接下来分别挂载两个 hugetlbfs 文件系统，第一个 hugetlbfs 文件系统与 2MiB 粒度大页池子绑定，然后挂载点预留 8 个大页; 另外一个 hugetlbfs 文件系统与 1Gig 粒度大页池子绑定，然后挂载点预留 1 个大页。此时从 /sys/kernel/mm/hugepages 目录下各粒度对应目录下的 resv_hugepages 节点均为挂载点预留的值，最后使用 mount 命令查看两个 hugetlbfs 文件系统挂载点的信息，均符合预期值，因此实践验证上述说法正确无误.

最后需要研究的就是 hugepagesz 和 hugepages 的取值范围，hugepagesz 的取值范围与具体的体系架构有关，有的架构可以支持多种粒度的大页，但有的架构就支持一种粒度的大页，总结来说如下:

* i386 架构支持: 4MiB
* X86_64 架构支持: 1Gig/2MiB
* ARM64 架构支持: 64KiB/2MiB/32MiB/1Gig

对于 hugepages 的取值，原则上只要 hugepages 的取值不要超过系统内存大小即可，但为了保证 hugepages 有效，最好不要超过系统内存的一半.

------------------------------------

###### Hugetlb CMDLINE Usage

通过上面的分析，可以知道 Hugetlb 机制提供的 CMDLINE 字段主要完成两个任务，第一个任务是设置系统使用的默认粒度的大页; 另外一个任务是创建指定粒度的大页内存池子并向池子中添加指定数量的大页。在使用这些字段时需要注意:

* default_hugepagesz 和 hugepagesz 的值是要复合要求的
* hugepagesz 和 hugepages 要搭配使用
* hugepagesz/hugepages 可以同时存在多对，但 hugepagesz 的值不能相同
* hugepages 的值不能超过系统内存大页，最后是系统内存的一半

既然有这些条件限制，那么就会出现合规的情况和非法的情况，接下来对每种情况进行分析。首先是合规的情况，如下:

{% highlight bash %}
# 假设系统硬件支持 1Gig 和 2MiB 的大页
# 并且系统内存为 8Gig

# 设置默认粒度大页
CMDLINE="... default_hugepagesz=1G ..."
CMDLINE="... default_hugepagesz=2M ..."

# 创建指定粒度的大页内存池子
CMDLINE= "... hugepagesz=1G hugepages=2 ..."
CMDLINE= "... hugepagesz=2M hugepages=1024 ..."
CMDLINE= "... hugepagesz=1G hugepages=1 hugepagesz=2M hugepages=20 ..."
{% endhighlight %}

对于合规的使用场景，系统能够创建出符合要求的大页池子，也可以正确设置系统使用的默认粒度大页。接下来看看非法使用情况:

{% highlight bash %}
# 假设系统硬件支持 1Gig 和 2MiB 的大页
# 并且系统内存为 8Gig

# 设置默认粒度大页
CMDLINE="... default_hugepagesz=32M ..."
CMDLINE="... default_hugepagesz=64M ..."

# 创建指定粒度的大页内存池子
CMDLINE= "... hugepagesz=32M hugepages=2 ..."
CMDLINE= "... hugepagesz=2M hugepages=8192 ..."
CMDLINE= "... hugepagesz=2M hugepages=1024 hugepagesz=2M hugepages=20 ..."
{% endhighlight %}

当 default_hugepagesz 和 hugepagesz 设置了不支持的粒度时，系统启动时会打印提示信息, 对于 default_hugepagesz 设置为不支持的大页粒度时，系统会提示设置错误，并将默认大页粒度修改为某种正确的粒度，该粒度与体系架构有关. 当 hugepagesz 设置为不支持的粒度时，系统会提出错误信息，然后将 hugepages 字段也一同忽略.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001162.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001163.png)

当 hugepagesz 字段正确，但 hugepages 字段设置错误，例如 hugepages 设置的值转换成大页内存之后，其长度超过系统物理内存. 由于 hugepages 字段是在系统启动的时候从 Buddy 分配器动态分配所需的内存，系统启动阶段会一直从 Buddy 分配器中分配复合页来构成大页内存池中的大页，直到分配够才停止分配，这样很容易导致 Buddy 分配器触发系统 OOM.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001164.png)

当 hugepagesz 和 hugepages 字段都正确，只是 CMDLINE 中出现重复的 hugepagesz 值相同的情况，对于这种情况，系统会打印提示信息，然后只采用第一次 "hugepagesz/hugepages" 的配置，其他都忽略丢弃.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001165.png)

{% highlight bash %}
# 假设系统硬件支持 1Gig 和 2MiB 的大页
# 并且系统内存为 8Gig

# 创建指定粒度的大页内存池子
CMDLINE= "... hugepagesz=1G hugepages=7 ..."
CMDLINE= "... hugepagesz=2M hugepages=3700 ..."
CMDLINE= "... hugepagesz=1G hugepages=4 hugepagesz=2M hugepages=20 ..."
{% endhighlight %}

hugetlb CMDLINE 字段除了合规和非法使用的场景外还存在第三种场景，这种场景下每个字段都是符合要求的，但是系统无法提供足够的内存共给大页内存池子，例如上面提到的三种情况，第一种情况就是对于 1Gig 大页，其来自 MEMBLOCK 内存分配器，此时系统没法提供这么多的连续物理内存, 但此时系统并不会因为分配不出内存而 OOM，而是尽可能的分配足够数量的大页，例如下图的场景，需要分配 7 个 1Gig 的大页，但系统只能分配 5 个 1Gig 的大页，那么系统就只分配 5 个 1Gig 的大页.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001166.png)

另外一种场景是分配粒度为 2MiB 的大页，由于 2MiB 的大页通过 Buddy 分配器分配的复合页构成，因此当系统需要分配大量的大页时，但 Buddy 无法提供这么多连续复合页，那么系统可以在分配过程中触发 OOM，例如下面在分配 3999 个 2MiB 大页，分配到 3800 个左右的时候触发系统 OOM.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001167.png)

最后一种场景就是内核 CMDLINE 中同时存在分配两种粒度的大页，其中先分配 1G 粒度的大页，接着分配 2MiB 的粒度，这种情况下 1G 粒度大页分配成功，但这个时候 Buddy 分配器已经分配不出更多的复合页，那么这个时候就会触发 Buddy 分配器的 OOM，如上图。就算不触发 OOM，那么也会使系统可用内存超级少，这回导致系统运行卡顿等问题. 因此通过上面的讨论，在创建大页内存池子之前一定要做好规划.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001168.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------

<span id="KB"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### Hugetlb 大页机制与内核宏关系研究

在 hugetlb 大页机制中，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式向用户空间提供大页。无论进程通过文件映射的方式在指定 hugetlbfs 文件系统挂载点，还是通过匿名映射的方式在系统默认的 hugetlbfs 文件系统挂载点，进程都需要创建或打开一个大页文件，只是对于文件映射方式创建或打开的是一个大页文件，而匿名映射则创建一个大页虚拟文件，两种文件文件没有本质区别。进程接着从进程空间分配一段虚拟内存并用于映射大页文件(大页虚拟文件)，此时系统会为进程的这段虚拟内存预留指定数量的大页(在特殊情况下也可以不预留)。待进程访问这段虚拟内存时，由于虚拟内存还没有与任何大页的物理内存建立页表，因此会触发系统缺页。在缺页中断处理函数中，系统会从预留的大页池子中获得一个大页，然后建立虚拟内存到大页物理内存的页表。待缺页中断返回之后，进程可以正常访问这段虚拟内存，并间接使用大页。当进程不再使用大页时候，对于匿名映射进程直接解除这段虚拟内存与大页的映射，并将大页归还大页内存池子; 对于文件映射进程同样解除虚拟内存到大页的映射并关闭文件，但是由于大页文件的存在，大页并不会归还大页池子，只有到文件被摧毁的时候才会被释放会大页池子。以上便是一次完整的大页分配过程，其关键点是基于 hugetlbfs 文件系统，并且系统支持大页。那么本节用来研究在不同的架构中为了支持 hugetlb 大页机制，内核需要启用哪些内核宏。

###### CONFIG_HUGETLBFS

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001170.png)

CONFIG_HUGETLBFS 宏用于支持 Hugetlbfs 文件系统，如果系统启用该宏，那么系统在启动过程中会为默认粒度和内核 CMDLINE 中指定粒度的大页内存池子创建 hugetlbfs 文件系统挂载点，该 hugetlbfs 文件系统有内核挂载，以便为进程通过匿名映射分配大页。该宏启用之后可以在用户空间挂载一个 hugetlbfs 文件系统并绑定指定粒度的大页。该宏如果要启用，首先需要满足任一条件:

* X86 架构支持
* IA64 架构支持
* SPARC64 架构支持
* S390 架构且 64 位模式支持
* SYS_SUPPORTS_HUGETLBFS 宏启用
* BROKEN 宏启用

如果 CONFIG_HUGETLBFS 宏启用，CONFIG_HUGETLB_PAGE 宏一同被启用。另外系统会在 /proc/meminfo 中显示默认粒度大页内存池子的使用情况，并会在 /proc/sys/vm 目录下提供默认粒度大页内存池子的配置节点 (nr_hugepages nr_hugepages_mempolicy nr_overcommit_hugepages), 最后会在 /sys/kernel/mm/hugepages 目录下为不同粒度的大页池子创建目录，并在每种粒度大页目录下存在其内存池子的配置节点 (nr_hugepages free_hugepages nr_hugepages_mempolicy nr_overcommit_hugepages resv_hugepages surplus_hugepages). 另外可以在 /proc/filesystems 中看到系统挂载的 hugeltbfs 文件系统，以及用户可以自行挂载 hugetlbfs 文件系统.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001171.png)

###### Hugetlb CONFIG on X86_64

为了在 X86_64 架构中支持 Hugetlb 大页机制，内核必须打开指定的宏。由于 X86_64 架构支持的大页粒度为 2MiB 和 1Gig。对于 2MiB 粒度的大页，其一直由 Buddy 分配器分配的复合页构成，而对于 1Gig 粒度的大页，如果是通过 CMDLINE 分配，那么其由 MEMBLOCK 分配，如果是系统运行中分配，那么其由 CMA 分配其分配，因此如果要支持 1Gig 粒度，系统需要支持 CMA. 总结 X86_64 架构需要启用的宏如下:

{% highlight bash %}
# Hugetlbfs
CONFIG_HUGETLBFS
CONFIG_HUGETLB_PAGE
CONFIG_ARCH_ENABLE_HUGEPAGE_MIGRATION

# CMA
CONFIG_MEMORY_ISOLATION
CONFIG_CMA
CONFIG_CMA_AREAS=7
{% endhighlight %}

另外如果要在 X86_64 架构中关闭 Hugetlb 大页机制，只需关闭 CONFIG_HUGETLBFS 宏即可，CMA 功能只是影响是否能不能分配到 1Gig 的连续内存，但不与 hugetlb 大页机制强耦合，因此不必关闭 CMA 功能.

###### Hugetlb CONFIG on i386

为了在 i386 架构中支持 Hugetlb 大页机制，内核必须打开指定的宏。由于 i386 架构只支持 4MiB 粒度的大页。因此只需支持 Hugetlbfs 文件系统即可使用大页，因此 i386 架构所需的宏如下:

{% highlight bash %}
# Hugetlbfs
CONFIG_HUGETLBFS
CONFIG_HUGETLB_PAGE
{% endhighlight %}

另外如果要在 i386 架构中关闭 Hugetlb 大页机制，只需关闭 CONFIG_HUGETLBFS 宏即可，这将彻底关闭 Hugetlbfs 文件系统和 hugetlb 大页机制.

###### Hugetlb CONFIG on ARM64

为了在 ARM64 架构中支持 Hugetlb 大页机制，内核必须打开指定的宏。由于 ARM64 架构同时支持 64KiB、2MiB、32MiB、1Gig 粒度的大页, 因此 ARM64 架构所需的宏如下:

{% highlight bash %}
# Hugetlbfs
CONFIG_HUGETLBFS
CONFIG_HUGETLB_PAGE
CONFIG_SYS_SUPPORTS_HUGETLBFS
{% endhighlight %}

另外如何要在 ARM64 架构中关闭 Hugetlb 大页机制，只需关闭 CONFIG_HUGETLBFS 宏即可.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------

<span id="BA"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000N.jpg)

#### 共享匿名映射 Hugetlb 大页使用攻略

在 hugetlb 大页机制，内核可以基于 hugetlbfs 文件系统以文件 page cache 的方式为用户进程提供大页内存。进程可以通过匿名共享的方式从系统获得 hugetlb 大页，其底层逻辑是: 系统在启动过程中内核会自动挂载多种粒度大页的 hugetlbfs 文件系统，进程从其地址空间分配一段虚拟内存，然后使用 MAP_HUGETLB 和 MAP_SHARED 标志进行匿名映射，如果没有指定映射 hugetlb 大页的粒度，那么系统会在绑定默认大页粒度的 hugetlbfs 文件系统挂载点下创建一个虚拟文件; 如果进程匿名映射时指定了 hugetlb 大页的粒度，那么系统会在绑定指定大页粒度的 hugetlbfs 文件系统挂载点下创建一个虚拟文件。然后将进程的虚拟内存映射到该虚拟文件上, 此时并没有建立虚拟内存到 hugetlb 大页物理内存的页表，而是系统会为这段虚拟内存从指定粒度的大页内存池子中预留指定数量的大页，这些被预留的大页除了可以被进程和其子进程使用外，其他进程都不能再使用这些大页，直到进程释放这些大页。当进程第一次访问这段虚拟内存，由于虚拟内存没有建立到大页物理内存的页表，那么会触发系统缺页异常，进程暂停运行系统进入缺页中断，在缺页中断处理函数中，系统会从指定粒度大页预留池子中取出一个大页，然后建立虚拟内存到大页物理内存的页表，待缺页中断返回之后进程恢复运行，此时进程可以正常访问虚拟内存，并间接使用大页。待进程不再使用这段虚拟内存时，进程解除虚拟内存到虚拟文件的映射，那么此时系统会回收虚拟内存已经使用的大页，以及虚拟内存预留的大页，这些大页最终都会回到指定粒度大页内存池子中。

匿名映射大页是与文件映射相对了，与普通的映射不同点是 hugetlb 大页机制的匿名映射也会创建一个虚拟文件，然后与文件映射一样基于文件的 page cache 为进程提供大页。但与文件映射大页不同的地方是进程无需显示的在 hugetlbfs 文件系统挂载点下创建或打开一个大页文件，而指向直接映射然后系统会在默认的 hugetlbfs 文件系统挂载点为其创建一个文件。另外进程不再使用文件映射的大页时，大页并不能直接被回收，而是要等到文件大页被摧毁时才能被回收; 而进程在使用完匿名大页时，只要解除进程虚拟内存到虚拟大页文件映射时，系统就会回收大页。对于共享匿名大页，那么是与私有匿名大页对比来说的，两者的区别在与当进程分配一段虚拟内存映射虚拟大页文件时，系统会为这段虚拟内存映射预留指定数量的大页，对于共享匿名映射的大页来说，这些预留的大页可以被进程和其子进程读写，但对于私有匿名映射的大页来说，这些预留的大页只能被进程使用，其子进程只能读不能写。另外匿名映射的大页粒度也不同，不同的架构可以支持多种粒度的大页，并且统一架构统一进程可以同时使用不同粒度的匿名大页，那么接下来给出了多种场景用来说明匿名大页的使用攻略.

> [默认粒度共享匿名映射 Hugetlb 大页使用攻略](#BA0)
>
> [64KiB 粒度共享匿名映射 Hugetlb 大页使用攻略](#BA1)
>
> [2MiB 粒度共享匿名映射 Hugetlb 大页使用攻略](#BA2)
>
> [32MiB 粒度共享匿名映射 Hugetlb 大页使用攻略](#BA2)
>
> [1Gig 粒度共享匿名映射 Hugetlb 大页使用攻略](#BA3)
>
> [指定粒度共享匿名映射 Hugetlb 大页使用攻略](#BA4)
>
> [进程与子进程共同使用匿名共享大页](#BA4)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)


--------------------------------------

<span id="BA0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000D.jpg)

#### 默认粒度共享匿名映射 Hugetlb 大页使用攻略

在支持 hugetlb 大页机制的系统中，如果没有在内核 CMDLINE 中通过 default_hugepagesz 字段指明默认大页粒度的情况下，不同架构会采用某种粒度的大页作为默认粒度的大页，并且内核会在 /proc/sys/vm 目录下创建 nr_hugepages、nr_overcommit_hugepages、nr_hugepages_mempolicy 用于控制默认粒度的大页内存池子。进程在使用共享匿名映射方式使用 hugetlb 大页时，如果没有显示的指明大页的粒度，那么系统将会分配默认粒度的大页给进程使用。另外默认粒度大页内存池子的大页可来自三个地方: 第一个是通过内核 CMDLINE 的 hugepagesz 和 hugepages 字段在系统启动阶段分配的固定大页; 第二个是来自 /proc/sys/vm 提供的 nr_hugepagesz 接口分配的固定大页; 第三种通过 /proc/sys/vm 提供的 nr_overcommit_hugepages 接口动态从系统内存分配器分配的超发大页. 那么接下来重点分析如何在系统中使用默认粒度的共享匿名映射 hugetlb 大页

###### 准备 Hugetlb 大页

通过上面的分析可以知道，在使用默认粒度大页之前，默认粒度的大页内存池子中需要要能分配可用的大页，那么分别介绍三种方式的使用攻略. 如果默认粒度的大页来自系统启动时的固定大页，那么需要在内核 CMDLINE 中进行指定，如下:

{% highlight bash %}
# size 为默认粒度的大页长度，单位可以是 K/M/G
# num 为需要分配默认粒度大页的数量
#   e.g. size 可以的值为: 64K、2M、32M、1G
CMDLINE= "... default_hugepagesz={size} hugepagesz={size} hugepages={num} ..."
{% endhighlight %}

内核 CMDLINE 的 default_hugepagesz 字段可以指定系统默认大页的粒度，如果不使用该字段，那么不同的架构默认大页粒度不同，例如 X86_64 和 ARM64 架构默认粒度为 2MiB，而 i386 架构默认粒度为 4MiB。无论 default_hugepagesz 字段是否存在，hugepagesz/hugepages 字段必须存在，并且 hugepagesz 的值必须是默认粒度大页的长度，hugepages 字段则指明默认粒度大页的固定大页数量. 通过上面的设置系统在启动完毕之后，就可以在 /proc/meminfo 节点下看到默认粒度大页池子中已经存在 hugepages 个空闲大页. 如果不在系统启动是准备默认粒度的固定大页，而是在系统启动完毕之后再分配固定大页，那么可以参考如下命令进行分配:

{% highlight bash %}
# Num 为默认粒度固定大页数量
echo {Num} > /proc/sys/vm/nr_hugepages
{% endhighlight %} 

通过上面命令，如果系统有充足的连续内存，那么系统会分配指定数量的固定大页到默认粒度大页内存池子中，分配完毕之后可以在 /proc/meminfo 节点下查看默认粒度大页内存池子的使用情况. 如果也不想通过这种方式来填充默认粒度大页内存池子，而是想考虑系统内存的灵活性，想让进程使用多少大页就分配多少大页，不使用时就归还给系统，那么可以采用超发大页机制来满足默认粒度大页内存池子的分配需求，可以参考如下命令进行分配:

{% highlight bash %}
# Num 为允许默认粒度大页内存池子最多可获得超发大页数量
echo {Num} > /proc/sys/vm/nr_overcommit_hugepages
{% endhighlight %}

通过上面这种方法，只有进程运行时默认粒度大页池子才会从系统分配超发大页，这样能很好平衡默认大页池子与系统内存池子的可用内存占用比例。无论采用上述那种方法分配大页，从系统角度来看都是默认粒度大页内存池子中的一个可用大页而已。那么接下来就是实践例子，其在 BiscuitOS 部署逻辑如下:

###### BiscuitOS 实践运行

{% highlight bash %}
cd BiscuitOS
make menuconfig

[*] Package  --->
    [*]  Hugetlb and Hugetlbfs Mechanism  --->
        [*] hugetlb: Anonymous Shared-mapping for Default Size Hugepage  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/BiscuitOS-hugetlb-anonymous-share-mapping-default
{% endhighlight %}

> [BiscuitOS 独立应用程序实践攻略](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C2)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001172.png)

实践例子是一个用户空间的程序，代码逻辑很简单，首先调用 mmap() 映射一段虚拟内存，此时 MAP_ANONYMOUS 标志用于指明这次映射是匿名映射，MAP_SHARED 标志则指明这次是共享映射，MAP_HUGETLB 标志则指明用于映射 Hugetlb 大页，通过这三个标志就可以实现共享匿名映射一个 Hugetlb 大页。另外映射的权限设置为 PROT_READ 和 PROT_WRITE，即进程对这段虚拟内存具有读写权限。这段虚拟内存的长度为 BISCUITOS_MAP_SIZE 即 4MiB。接着如果映射成功，那么系统在会在系统挂载默认粒度大页的 hugetlbfs 文件系统下创建一个大页，然后为这段虚拟内存预留指定数量的大页，如果默认粒度是 2MiB，那么系统会预留 2 个大页; 如果默认粒度是 4MiB，那么系统会预留一个大页; 如果默认粒度大于 4MiB， 那么系统预留 1 个大页。接下来进程向这段虚拟区域的第一个字节写入 "B" 字符，此时由于虚拟内存还没有与某个大页的物理内存建立页表，此时触发缺页异常，进程停止运行并进入系统缺页中断处理程序，系统会从预留的大页中任意挑选一个大页，并将大页的状态调整为激活态，然后建立虚拟内存到大页物理内存的页表。待缺页中断处理程序执行完毕之后，进程恢复执行，进程再次访问虚拟内存，此时虚拟内存已经和大页的物理内存建立页表，那么进程可以访问这段虚拟内存，从而间接使用大页。进程接着将虚拟内存的地址和值打印出来，最后进程使用完虚拟内存之后使用 munmap() 函数解除映射，此时系统将回收进程预留的大页和被进程激活的大页，将其归还给默认粒度大页内存池子，如果此时大页是超发大页，那么默认粒度大页内存池子将超发大页就继续归还给系统, 至此实践例子进程周期完结. 接下来在 BiscuitOS 上实践该案例:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001173.png)

在 BiscuitOS 系统启动完毕之后，首先读取 /proc/meminfo 节点，以此获得默认粒度大页内存池子，此时可以看到大页默认粒度为 2MiB，此时默认粒度大页池子中没有可用的大页，那么此时运行程序，结果程序在映射阶段就失败了，这是应为共享匿名映射时会从默认粒度大页池子中预留 2 个大页，但是此时默认粒度大页池子没有可用的大页，因此预留大页失败导致了映射失败。那么接下来通过 /proc/sys/vm/nr_hugepages 接口写入 10 让系统向默认粒度大页内存池子中新增 10 个可用大页，此时查看 /proc/meminfo 节点，已经看到默认粒度大页内存池子中已经有 10 个可用大页了，此时成功运行程序。待程序运行完毕之后，系统回收了所有进程占用的大页并归还给默认粒度大页，由于大页都是固定大页，因此默认粒度大页内存池子不会将固定大页继续归还给系统。至此就是一次完整的共享匿名映射 hugetlb 大页的使用过程。

-----------------------------------

###### 自定义默认粒度大页

通过上述的分析研究可以知道应用程序如何使用一个共享匿名大页，那么接下来进一步将如何控制系统默认粒度大页的大页。通过上面的分析，如果向控制默认粒度的大页，那么需要使用内核 CMDLINE 的 default_hugepagesz 字段进行控制，由于不同架构支持的大页粒度不同，但在可支持大页粒度前提下，可以将需要设定的大页粒度写入到 default_hugepagesz, 具体写法如下:

{% highlight bash %}
# size 为需要指定默认粒度大页长度
# num 为需要分配默认粒度大页的数量
CMDLINE= "... default_hugepagesz={size} ..."
CMDLINE= "... default_hugepagesz={size} hugepagesz={size} hugepages={num} ..."

# 64KiB 默认粒度大页
CMDLINE= "... default_hugepagesz=64K ..."
CMDLINE= "... default_hugepagesz=64K hugepagesz=64K hugepages=10 ..."

# 2MiB 默认粒度大页
CMDLINE= "... default_hugepagesz=2M ..."
CMDLINE= "... default_hugepagesz=2M hugepagesz=2M hugepages=10 ..."

# 32M 默认粒度大页
CMDLINE= "... default_hugepagesz=32M ..."
CMDLINE= "... default_hugepagesz=32M hugepagesz=32M hugepages=10 ..."

# 1Gig 默认粒度大页
CMDLINE= "... default_hugepagesz=1G ..."
CMDLINE= "... default_hugepagesz=1G hugepagesz=1G hugepages=10 ..."
{% endhighlight %}

在内核 CMDLINE 中通过 default_hugepagesz 字段指定默认粒度大页时，可以连带使用 "hugepagesz/hugepages" 为默认粒度大页内存池子预先分配指定数量的固定大页, 也可以只指定默认粒度大页的大小，而等到系统启动完毕之后再分配固定大页或动态分配超发大页。系统启动完毕之后，可以在 /proc/meminfo 节点下查看默认粒度大页池子的信息，此时在不改变实践例子的情况下，运行实践例子就可以通过共享匿名映射使用指定粒度的大页。(例如 i386 架构上使用默认 4MiB 粒度的大页)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001174.png)

在 BiscuitOS 上运行实践例子，此时在内核 CMDLINE 中设置了 default_hugepagesz 字段为 4MiB，接着使用 hugepagesz/hugepages 字段向 4MiB 粒度大页池子中预分配了 10 个大页，待系统启动完毕之后，查看 /proc/meminfo 节点，可以看到默认粒度大页为 4MiB，并且有 10 个大页都可用。那么运行程序，进程通过共享匿名映射的方式映射了 2 个 4MiB 大页，并使用了其中的一个 4MiB 大页，进程运行完毕之后，系统回收进程占用的 2 个大页，此时查看 /proc/meminfo 节点，可以看到默认粒度大页已经恢复 10 个可用大页.

-----------------------------------------

###### 绑定 NUMA NODE

在支持多 NUMA NODE 的系统中，默认粒度大页内存池子的大页可以来自不同 NUMA NODE，另外提供了 numactl 工具，可以将应用程序绑定在指定的 CPU 和 NUMA NODE 上分配资源，这是可以基于这些特性来自定义进程使用的默认粒度大页来自指定的 NUMA NODE. 如果进程要使用指定 NUMA NODE 上的大页，那么首先确保默认粒度大页内存池子中有来自该 NUMA NODE 的大页，可以使用如下命令从指定 NUMA NODE 上分配大页:

{% highlight bash %}
# 从 <node-list> 的 NUMA NODE 上交错分配固定大页
# 固定大页会交错散布在 <node-list> 的 NUMA NODE 上
numactl --interleave <node-list> echo 20 > /proc/sys/vm/nr_hugepages_mempolicy
 
# 从可以分配内存的 NUMA NODE 列表 <node-list> 上分配固定大页
# 固定大页会均匀的散布在 <node-list> 的 NUMA NODE 上
# 可以将 <node-list> 指定为某个 NUMA NODE, 那么只从该 NUMA NODE 上分配固定大页
numactl -m <node-list> echo 20 > /proc/sys/vm/nr_hugepages_mempolicy

# 从本地 NUMA NODE 上分配固定大页
numactl --localalloc echo 20 > /proc/sys/vm/nr_hugepages_mempolicy

# 优先从指定的 NUMA NODE 上分配固定大页
numactl --preferred={node} echo 20 > /proc/sys/vm/nr_hugepages_mempolicy
{% endhighlight %}

以上提供了多种从 NUMA NODE 上分配固定大页的方法，如果上述代码的 <node-list> 只是一个 NUMA NODE 的话，那么系统只从该 NUMA NODE 上分配固定大页，这会导致进程如果非要从某个 NUMA NODE 上分配内存，但该 NUMA NODE 上没有可用的固定大页，其他 NUMA NODE 上却有固定大页，但进程还是会分配失败. 当在指定 NUMA NODE 上分配完毕固定大页之后，可以通过如下命令查看指定 NUMA NODE 分配大页情况:

{% highlight bash %}
# NODE_INFO 为 NUMA NODE 的节点
cat /sys/devices/system/node/${NODE_INFO}/meminfo | fgrep Huge

~ # Node {NODE_INFO} HugePages_Total:    10
~ # Node {NODE_INFO} HugePages_Free:     10
~ # Node {NODE_INFO} HugePages_Surp:      0
{% endhighlight %}

每个 NUMA NODE 的 meminfo 节点下记录大页总数 HugePages_Total，空闲大页总数 HugePages_Free, 以及超发大页总数 HugePages_Surp. 那么接下来在不改动实践程序的基础上，控制进程通过共享匿名映射指定 NUMA NODE 上的固定大页: (在 BiscuitOS 上实践之前需要准备 NUMA 环境和 numactl 工具，可以餐卡如下)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001179.png)

为了支持 NUMA 环境，需要在 BiscuitOS 的启动脚本 RunBiscuitOS.sh 中添加上面字段用于部署两个 NUMA NODE, 并且 2 个 CPU 各自亲和在一个 NUMA NODE 上，每个 NUMA NODE 的内存大小为总内存的一半，接下来就是部署 numactl 工具，其部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

[*] Package  --->
    [*]  NUMA Mechanism  --->
        [*] numctl tools and libnuma library  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/numactl-libnuma-default
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001175.png)

BiscuitOS 启动之后，首先查看系统默认粒度大页内存池的情况，目前没有任何大页。接着从 NUMA NODE 0 上分配 10 个固定大页填充到默认粒度大页内存池子中，此时查看 NUMA NODE 0 上 meminfo 节点信息，可以看到该节点上总共具有 10 个大页，其中 10 个空闲大页和 0 个超发大页。接下来运行程序，使用 numactl 工具加上 "--membind=0" 参数，让进程从 NUMA NODE 0 上通过共享匿名映射的方式分配 2 个大页，并使用其中一个大页，此时查看 NUMA NODE 0 上 meminfo 节点信息, 可以看到有一个大页在使用，空闲大页变成 9 (由于没有统计预留大页，并且预留大页也是空闲大页，因此这里实际有 1 个大页是预留的). 接着查看 NUMA NODE 1 上大页情况，此时 NUMA NODE 1 上的 meminfo 展示没有任何大页，此时通过 /proc/meminfo 节点继续查看系统默认粒度大页内存池子信息，发现还有 8 个可分配的大页，那么此时让进程在 NUMA NODE 1 上通过共享匿名映射分配大页，当程序运行之后发现进程只要一访问虚拟内存就会触发 Bus error 错误，这是因为进程访问虚拟内存触发缺页中断，当缺页中断处理程序无法在 NUMA NODE 0 上找到可用的大页，最终触发内核的 Bus error 错误。以上便是共享匿名映射的大页绑定 NUMA NODE 的分析.

-----------------------------------

###### 默认粒度大页用途

共享匿名映射大页可让两个或多个进程共同读写一个 hugetlb 大页，也可以让父进程和子进程共同读写一个大页，那么这里重点介绍如何让多个进程共同读写一个 hugetlb 大页。实践案例在 BiscuitOS 中的部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

[*] Package  --->
    [*]  Hugetlb and Hugetlbfs Mechanism  --->
        [*] hugetlb: Anonymous Shared-mapping for SHMEM  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/BiscuitOS-hugetlb-anonymous-share-mapping-shmem-default
{% endhighlight %}

> [BiscuitOS 独立应用程序实践攻略](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C2)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001176.png)

为了更好的展示多个进程共享匿名映射 hugetlb 大页，案例中使用了 Server 端和 Client 端来模拟两个进程共同使用一个 hugetlb 大页，上图是 Server 端。程序首先调用 shmget() 函数创建一个共享匿名 hugetlb 大页对象，然后调用 shmat() 函数把共享内存区域对象映射到进程的地址空间，接下来向共享区域写入字符串.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001177.png)

对于 SHMEM 的 Client 端，程序首先调用 system() 函数运行 "ipcs -m" 命令查看当前共享内存信息，接着调用  shmget() 函数获得一个共享匿名 hugetlb 内存区对象，同理调用 shmat() 函数将共享内存对象映射到进程的地址空间。映射完毕之后读取共享区域的内容，并通过 printf() 函数大页读取的内容。函数读取完毕之后，调用 shmdt() 函数断开共享内存的连接，并调用 shmctl() 函数对共享内存进行操作，这里使用 IPC_RMID 命令，即删除这片共享内存。程序最后再次调用 system() 函数运行 "ipcs -m" 命令查看共享内存信息. 那么接下来在 BiscuitOS 上进行实践:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001178.png)

BiscuitOS 运行之后，首先向默认粒度的大页内存池子中新增 10 个固定大页。然后后台方式运行 Server 端程序，接着查看 /proc/meminfo 节点下 Hugetlb 大页使用情况，此时可以看到两个大页被使用，这与 Server 程序预期一致. 接着运行 Clint 段程序，Client 进程首先使用命令 "ipcs -m" 查看当前系统的共享内存信息，可以看到此时 key 为 2 的共享区域对象存在一块 4MiB 的共享区域，然后 Client 进程映射 key 为 2 的共享区域并读取首地址处的字符串，此时打印的字符串正好是 Server 端向共享区域写入的字符串 "Hello BiscuitOS on Shared Anonymous Hugepage!". 接着 Client 进程与 Server 端断开，然后删除了这块共享内存区域，最后再次查看系统共享内存信息，此时已经没有任何共享内存。以上便是通过匿名共享映射方式实现多个进程使用一个 hugetlb 大页.

-------------------------------------

###### [提高] 迁移一个默认粒度的共享匿名映射 hugetlb 大页

在支持多 NUMA NODE 的架构中，默认粒度大页内存池子的大页可以来自不同的 NUMA NODE, 另外提供的 numactl 工具可以将应用程序绑定在指定的 CPU 上运行以及指定的 NUMA NODE 上分配内存，这是前面有讨论的，那么本节基于之前多 NUMA NODE 情况下共享匿名映射 hugetlb 大页的讨论，进一步研究如何在 NUMA NODE 之间迁移共享匿名映射的 hugetlb 大页。迁移的本质是在应用程序不感知的情况下将虚拟内核映射的物理页替换成其他物理页，同理共享匿名映射 hugetlb 大页的迁移也是让应用程序不感知的情况下替换成其他 NUMA NODE 的 hugetlb 大页，这里使用一个实践案例进行讲解，在讲解之前同样也需要在 BiscuitOS 上准备多 NUMA 的环境以及带有 numactl 工具的系统，那么可以参考如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001179.png)

为了支持 NUMA 环境，需要在 BiscuitOS 的启动脚本 RunBiscuitOS.sh 中添加上面字段用于部署两个
 NUMA NODE, 并且 2 个 CPU 各自亲和在一个 NUMA NODE 上，每个 NUMA NODE 的内存大小为总内存的
一半，接下来就是部署 numactl 工具，其部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

[*] Package  --->
    [*]  NUMA Mechanism  --->
        [*] numctl tools and libnuma library  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/numactl-libnuma-default
{% endhighlight %}

准备好工具和环境之后，接下来是在 BiscuitOS 上部署实践代码，其部署逻辑如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

[*] Package  --->
    [*]  Hugetlb and Hugetlbfs Mechanism  --->
        [*] hugetlb: Anonymous Shared-mapping for Migration  --->

OUTPUT:
BiscuitOS/output/linux-XXX-YYY/package/BiscuitOS-hugetlb-anonymous-share-mapping-migration-default
{% endhighlight %}

> [BiscuitOS 独立应用程序实践攻略](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C2)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001180.png)

实践程序主体分作三大部分，第一个部分就是通过共享匿名映射的方式分配一个 hugetlb 大页，然后使用这个 hugetlb 大页，相关代码为 56 到 73 行。第二部分就是通过 numa_move_pages() 函数获得当前进程所在的 NUMA NODE 信息，之后再次调用 numa_move_pages() 函数将进程的 NUMA NODE 信息设置为预期的节点, 相关代码为 84 到 90 行。第三部分则是调用 numa_migrate_pages() 函数执行实际的迁移操作，迁移完毕之后再次查看进程所在的 NUMA NODE 信息，相关代码为 93 到 100 行。程序 81 行和 103 行添加了两个 sleep() 函数的目的是为了便于观察系统在不同 NUMA NODE 上默认粒度大页内存池子的使用情况。那么接下来在 BiscuitOS 实际运行验证:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH001181.png)

BiscuitOS 运行之后，用户首先向 /proc/sys/vm/nr_hugepages 节点写入 10，以此向系统默认粒度大页内存池子中添加 10 个固定大页，接着查看各 NUMA NODE 上固定大页池子的分布，可以看到起始状态时 NUMA NODE 0 和 NUMA NODE 1 上都有 5 个可用的大页。那么接下来运行测试程序，此时测试进程显示其位于 NUMA NODE 1 上，并且程序会消耗一个大页，接着再次查看 NUMA NODE 0 和 NUMA NODE 1 上大页消耗情况，此时注意到 NUMA NODE 1 上确实被消耗了一个大页，可用大页变成了 4，而 NUMA NODE 0 上没有消耗任何大页. 继续等待进程迁移，可以看到进程打印消息显示已经将大页由 NUMA NODE 1 迁移到 NUMA NODE 0 上，此时查看 NUMA NODE 0 和 NUMA NODE 1 上大页消耗情况，此时 NUMA NODE 0 上消耗 1 个大页，而 NUMA NODE 1 上没有消耗任何大页。通过上面实践符合预期结果，成功迁移一个共享匿名映射的 Hugetlb 大页.

--------------------------------------

###### 共享匿名映射 Hugetlb 大页失败合集























> [https://blog.csdn.net/yk_wing4/article/details/88080442](https://blog.csdn.net/yk_wing4/article/details/88080442)

