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
>
> - [从第一行代码编写/调试 ARM Linux 5.0 内核](https://biscuitos.github.io/blog/ARM-BOOT/)

## ARM Linux 5.0 源码实践

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
> - [kernel boot]
>
>   - [head.S](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/)
>
>     - [vmlinux 入口：第一行运行的代码](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#vmlinux first code)
>
>     - [__hyp_stub_install](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__hyp_stub_install)
>
>     - [__lookup_processor_type](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__lookup_processor_type)
>
>
