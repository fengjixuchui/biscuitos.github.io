---
layout: post
title:  "ARM"
date:   2019-04-18 15:06:33 +0800
categories: [MMU]
excerpt: ARM32.
tags:
---

## <span id="ARM Boot">ARM Feature</span>

> - [ARM Linux: vmlinux, Image, zImage, uImage](https://biscuitos.github.io/blog/ARM-Kernel-Image/)
>
> - [ARM Linux deugging manual](https://biscuitos.github.io/blog/BOOTASM-debuggingTools/)

## ARM Linux 5.0 源码实践

> - [实践准备](https://biscuitos.github.io/blog/ARM-BOOT/#%E7%AE%80%E4%BB%8B)
>
>   - [实践简介](https://biscuitos.github.io/blog/ARM-BOOT/#文章简介)
>
>   - [实践原理](https://biscuitos.github.io/blog/ARM-BOOT/#实践原理)
>
>   - [实践准备](https://biscuitos.github.io/blog/ARM-BOOT/#实践准备)
>
>   - [芯片手册及文档使用](https://biscuitos.github.io/blog/ARM-BOOT/#芯片手册及文档使用)
>
>   - [调试准备](https://biscuitos.github.io/blog/ARM-BOOT/#调试准备)
>
>   - [zImage 实践选择](#zImage 实践选择)
>
> - [zImage Bootstrap kernel](https://biscuitos.github.io/blog/ARM-BOOT/)
>
>   - [zImage 重定位之前实践](https://biscuitos.github.io/blog/ARM-BOOT/#zImage 重定位之前实践)
>
>     - [bootstrap 第一行汇编代码](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.1)
>
>     - [建立 armv7 cache table](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.2)
>
>     - [建立 zImage 临时页表](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.3)
>
>     - [MMU/cache on](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.4)
>
>     - [cache flush](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.5)
>
>     - [cache flush done](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.6)
>
>   - [zImage 重定位之后实践](https://biscuitos.github.io/blog/ARM-BOOT/#zImage 重定位之后实践)
>
>     - [zImage 重定位起点](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.7)
>
>     - [开始解压内核](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.8)
>
>     - [关闭 MMU, 刷新 cache](https://biscuitos.github.io/blog/ARM-BOOT/#v0.0.9)
>
>     - [将执行权移交给内核](https://biscuitos.github.io/blog/ARM-BOOT/#v0.1.0)
>
> - [内核初始化]
>
>   - [内核基础初始化，MMU，cache 均关闭]()
>
>     - [vmlinux 入口：第一行运行的代码](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#vmlinux first code)
>
>     - [__hyp_stub_install](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__hyp_stub_install)
>
>     - [__lookup_processor_type](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__lookup_processor_type)
>
>     - [__proc_info](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__proc_info)
>
>     - [__vet_atags](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__vet_atags)
>
>     - [__fixup_smp](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__fixup_smp)
>
>     - [__do_fixup_smp_on_up](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__do_fixup_smp_on_up)
>
>     - [__fixup_pv_table](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__fixup_pv_table)
>
>     - [__create_page_tables](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__create_page_tables)
>
>     - [swapper_pg_dir](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#swapper_pg_dir)
>
>     - [ARMv7 Cortex-A9 proc_info_list](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#ARMv7 Cortex-A9 proc_info_list)
>
>     - [__v7_setup](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__v7_setup)
>
>     - [v7_invalidate_l1](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#v7_invalidate_l1)
>
>     - [__v7_setup_cont](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__v7_setup_cont)
>
>     - [v7_ttb_setup](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#v7_ttb_setup)
>
>     - [__enable_mmu](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__enable_mmu)
>
>   - [MMU 初始化完毕，为跳转到 start_kernel 做准备]()
>
>     - [__mmap_switched](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__mmap_switched)
>
>     - [调用 start_kernel 之前](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#start_kernel_last)
>
> - [内核初始化 start_kernel]
>
>   - [set_task_stack_end_magic](https://biscuitos.github.io/blog/ARM-SCD-start_kernel.S/#set_task_stack_end_magic)
>
>   - [smp_setup_processor_id](https://biscuitos.github.io/blog/ARM-SCD-start_kernel.S/#smp_setup_processor_id)
>
>   - [debug_objects_early_init](https://biscuitos.github.io/blog/ARM-SCD-start_kernel.S/#debug_objects_early_init)
>
>   - [cgroup_init_early](https://biscuitos.github.io/blog/ARM-SCD-start_kernel.S/#cgroup_init_early)
>
>
