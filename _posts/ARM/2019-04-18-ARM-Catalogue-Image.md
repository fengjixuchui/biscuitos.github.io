---
layout: post
title:  "ARM"
date:   2019-04-18 15:06:33 +0800
categories: [MMU]
excerpt: ARM32.
tags:
---

## <span id="ARM Boot">ARM Feature</span>

> - [ARM Linux: vmlinux, Image, zImage, uImage](/blog/ARM-Kernel-Image/)
>
> - [ARM Linux deugging manual](/blog/BOOTASM-debuggingTools/)

## <span id="SC">ARM Linux 5.0 源码实践</span>

> - [实践准备](/blog/ARM-BOOT/#%E7%AE%80%E4%BB%8B)
>
>   - [实践简介](/blog/ARM-BOOT/#文章简介)
>
>   - [实践原理](/blog/ARM-BOOT/#实践原理)
>
>   - [实践准备](/blog/ARM-BOOT/#实践准备)
>
>   - [芯片手册及文档使用](/blog/ARM-BOOT/#芯片手册及文档使用)
>
>   - [调试准备](/blog/ARM-BOOT/#调试准备)
>
>   - [zImage 实践选择](#zImage 实践选择)
>
> - [zImage Bootstrap kernel](/blog/ARM-BOOT/)
>
>   - [zImage 重定位之前实践](/blog/ARM-BOOT/#zImage 重定位之前实践)
>
>     - [bootstrap 第一行汇编代码](/blog/ARM-BOOT/#v0.0.1)
>
>     - [建立 armv7 cache table](/blog/ARM-BOOT/#v0.0.2)
>
>     - [建立 zImage 临时页表](/blog/ARM-BOOT/#v0.0.3)
>
>     - [MMU/cache on](/blog/ARM-BOOT/#v0.0.4)
>
>     - [cache flush](/blog/ARM-BOOT/#v0.0.5)
>
>     - [cache flush done](/blog/ARM-BOOT/#v0.0.6)
>
>   - [zImage 重定位之后实践](/blog/ARM-BOOT/#zImage 重定位之后实践)
>
>     - [zImage 重定位起点](/blog/ARM-BOOT/#v0.0.7)
>
>     - [开始解压内核](/blog/ARM-BOOT/#v0.0.8)
>
>     - [关闭 MMU, 刷新 cache](/blog/ARM-BOOT/#v0.0.9)
>
>     - [将执行权移交给内核](/blog/ARM-BOOT/#v0.1.0)
>
> - 内核初始化
>
>   - 内核基础初始化，MMU，cache 均关闭
>
>     - [vmlinux 入口：第一行运行的代码](/blog/ARM-SCD-kernel-head.S/#vmlinux first code)
>
>     - [\_\_hyp_stub_install](/blog/ARM-SCD-kernel-head.S/#__hyp_stub_install)
>
>     - [\_\_lookup_processor_type](/blog/ARM-SCD-kernel-head.S/#__lookup_processor_type)
>
>     - [\_\_proc_info](/blog/ARM-SCD-kernel-head.S/#__proc_info)
>
>     - [\_\_vet_atags](/blog/ARM-SCD-kernel-head.S/#__vet_atags)
>
>     - [\_\_fixup_smp](/blog/ARM-SCD-kernel-head.S/#__fixup_smp)
>
>     - [\_\_do_fixup_smp_on_up](/blog/ARM-SCD-kernel-head.S/#__do_fixup_smp_on_up)
>
>     - [\_\_fixup_pv_table](/blog/ARM-SCD-kernel-head.S/#__fixup_pv_table)
>
>     - [\_\_create_page_tables](/blog/ARM-SCD-kernel-head.S/#__create_page_tables)
>
>     - [swapper_pg_dir](/blog/ARM-SCD-kernel-head.S/#swapper_pg_dir)
>
>     - [ARMv7 Cortex-A9 proc_info_list](/blog/ARM-SCD-kernel-head.S/#ARMv7 Cortex-A9 proc_info_list)
>
>     - [\_\_v7_setup](/blog/ARM-SCD-kernel-head.S/#__v7_setup)
>
>     - [v7_invalidate_l1](/blog/ARM-SCD-kernel-head.S/#v7_invalidate_l1)
>
>     - [\_\_v7_setup_cont](/blog/ARM-SCD-kernel-head.S/#__v7_setup_cont)
>
>     - [v7_ttb_setup](/blog/ARM-SCD-kernel-head.S/#v7_ttb_setup)
>
>     - [\_\_enable_mmu](/blog/ARM-SCD-kernel-head.S/#__enable_mmu)
>
>   - [MMU 初始化完毕，为跳转到 start_kernel 做准备]()
>
>     - [\_\_mmap_switched](/blog/ARM-SCD-kernel-head.S/#__mmap_switched)
>
>     - [调用 start_kernel 之前](/blog/ARM-SCD-kernel-head.S/#start_kernel_last)
