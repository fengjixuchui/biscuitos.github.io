---
layout: post
title:  "KVM HugePage (2M) on BiscuitOS (KOB-2M)"
date:   2020-10-24 10:24:00 +0800
categories: [HW]
excerpt: KVM Running on BiscuitOS.
tags:
  - KVM
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [KOB-2M 项目介绍](#A)
>
> - [KOB-2M 项目实践](#C)
>
> - [KOB-2M 源码解析](#D)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### KOB-2M 项目介绍

KOB-2M 项目全称 "KVM on BiscuitOS", 该项目用于在 BiscuitOS 快速为开发者直接创建一套 2M HugePage KVM 开发调试环境。开发者只需专注与 KVM 代码的开发与调试，其余部署相关的任务 BiscuitOS 会一站式解决。

KVM 全称 "Kernel-Based Virtual Machine", 是基于内核的虚拟机，它由一个 Linux 内核模块组成，该模块可以将 Linux 变成一个 Hypervisor。KVM 由 Quramnet 公司开发，于 2008 年被 Read Hat 收购。KVM 支持 x86 (32bit and 64bit), s390, PowerPC 等 CPU。KVM 从 Linux 2.6.20 起作为一个模块包含到 Linux 内核中。KVM 支持 CPU 和 内存的虚拟化。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000589.png)

本文重点介绍如何在 BiscuitOS 快速部署 KVM 开发环境。BiscuitOS 提供的开发环境包括了一个用户空间的应用程序，该应用程序模通过与 KVM 内核模块的交互，创建并运行了一个单核 16 MiB 内存的虚拟机，使用 2MiB HugePage，并在虚拟机中运行了简单的代码。具体请参见:

> [KOB-2M 项目实践](#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### KOB-2M 项目实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

KOB-2M 项目目前支持 x86_64，由于需要内核 HugePage 的支持，因此需要确保实践平台 HugePage 功能已经打开。首先开发者基于 BiscuitOS 搭建一个 x86_64 架构的开发环境，请开发者参考如下文档:

> - [BiscuitOS Linux 5.0 X86_64 环境部署](https://biscuitos.github.io/blog/Linux-5.0-x86_64-Usermanual/)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0001">实践部署</span>

在部署完毕开发环境之后, 由于需要在内核中支持 KVM 模块和 HugePage，因此开发者需要在内核配置中打开以下宏:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/linux/linux
make ARCH=x86_64 menuconfig

  [*] Virtualization  --->
      <*>   Kernel-based Virtual Machine (KVM) support
      <*>     KVM for Intel processors support
      [ ]     KVM for AMD processors support

make ARCH=x86_64 bzImage -j4
{% endhighlight %}

重新编译内核，内核编译完毕并重新运行 BiscuitOS，可以在 "/dev/" 目录下看到 kvm 节点:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000587.png)

接下来是安装 BiscuitOS KVM Userspace HugePage Application 源码，开发者可以参考如下命令进行部署:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-x86_64_defconfig
make menuconfig 

  [*] Package  --->
      [*] KVM  --->
          [*] KVM Application on BiscuitOS (2MiB HugePage)  --->

make
{% endhighlight %}

配置保存并执行 make，执行完毕之后会在指定目录下部署开发所需的文件，并在该目录下执行如下命令进行源码的部署，请参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-UKVM-2M-default
make download
{% endhighlight %}

执行完上面的命令之后，BiscuitOS 会自动部署所需的源码文件，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000615.png)

"main.c" 函数中是一个用户空间基于 KVM 创建虚拟机的过程，"BiscuitOS-Virtual-Machine.S" 文件是虚拟机操作系统的执行代码，使用汇编编写。"Makefile" 为编译应用程序的脚本. "RunBiscuitOS.sh" 脚本用于在 BiscuitOS 上创建 HugePage 相关的设置。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

部署完毕之后，接下来进行源码的编译和安装，并在 BiscuitOS 中运行. 参考如下代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-UKVM-2M-default
make
make install
make pack
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000616.png)

如上图在 BiscuitOS 运行之后，直接执行 RunBiscuitOS.sh 脚本，该脚本首先为虚拟机创建 2M HugePage 的运行环境，然后时虚拟机启动。虚拟机启动之后向指定 IO 输出 "hello BiscuitOS :)" 字符串. 开发者可以在 /mnt/HugePagefs/ 目录下查看 2MiB HugePage 对应的文件 BiscuitOS_HugePage. 至此 KOB-2M 项目实践完成.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### KOB-2M 源码分析

KOB-2M 项目源码主要分配两部分，一部分是内核空间的 KVM 源码，这部分源码不在本文展开讲解; 另一部分源码是用户空间基于 KVM 创建虚拟机的源码，以及虚拟机操作系统的源码，本文重点讲解这部分的源码.

> [> KVM Userspace Application](#D0)
>
> [> Virtual-Machine OS](#D1)
>
> [> Makefile scripts](#D2)
>
> [> RunBiscuitOS.sh](#D3)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

###### <span id="D0">KVM Userspace Application</span>

{% highlight c %}
/*
 * KVM Userspace 2M HugePage on BiscuitOS
 *
 * (C) 2020.10.24 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
/* KVM Header */
#include <linux/kvm.h>

/* KVM Path */
#define KVM_PATH		"/dev/kvm"
/* KVM-Virtual-Machine ISO */
#define BISCUITOS_OS_PATH	"/usr/bin/firmware/_BiscuitOS-VM.iso"
/* HugePage Path */
#define BISCUITOS_HUGE_PATH	"/mnt/HugePagefs/BiscuitOS_HugePage"
/* ISO Bunck */
#define ISO_BUNCK		4096
/* KVM Version */
#define KVM_VERSION		12
/* Virutal-Machine Memory size */
#define VM_MEMORY_SIZE		0x1000000

/*
 * Load OS into VM memory.
 */
static int BiscuitOS_load_OS(char *memory)
{
	int fd;
	int nbytes;

	fd = open(BISCUITOS_OS_PATH, O_RDONLY);
	if (fd < 0)
		return fd;

	/* Copy ISO */
	while (1) {
		nbytes = read(fd, memory, ISO_BUNCK);
		if (nbytes <= 0)
			break;
		memory += nbytes;
	}

	close(fd);
	return 0;
}

int main()
{
	struct kvm_userspace_memory_region region;
	int kvm_fd, vm_fd, vcpu_fd, huge_fd;
	struct kvm_run *kvm_run;
	int kvm_run_size;
	struct kvm_sregs sregs;
	struct kvm_regs regs;
	char *vm_memory;
	int error;

	/* Open KVM */
	kvm_fd = open(KVM_PATH, O_RDWR | O_CLOEXEC);
	if (kvm_fd < 0) {
		printf("ERROR[%d]: open %s failed.\n", kvm_fd, KVM_PATH);
		error = kvm_fd;
		goto err_open;
	}

	/* Check KVM Version */
	error= ioctl(kvm_fd, KVM_GET_API_VERSION, NULL);
	if (error != KVM_VERSION) {
		printf("ERROR[%d]: Invalid KVM version.\n", error);
		goto err_version;
	}

	/* Create an Virtual-Machine */
	vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, (unsigned long)0);
	if (vm_fd < 0) {
		printf("ERROR[%d]: Create Virtual-Machine failed.\n", vm_fd);
		error = vm_fd;
		goto err_version;
	}

	/* HugePage */
	huge_fd = open(BISCUITOS_HUGE_PATH, O_CREAT | O_RDWR);
	if (huge_fd < 0) {
		error = huge_fd;
		printf("ERROR[%d]: Open %s failed.\n", error,
							BISCUITOS_HUGE_PATH);
		goto err_version;
	}

	/* Allocate memory for Virtual-Machine from mmap */
	vm_memory = mmap(NULL, 
			 VM_MEMORY_SIZE,
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED,
			 huge_fd,
			 0);

	if (vm_memory == MAP_FAILED) {
		perror("ERROR: Allocat VM memory failed.\n");
		error = -1;
		goto err_huge;
	}
	close(huge_fd);

	/* Load Virtual-VM Code into VM memory */
	error = BiscuitOS_load_OS(vm_memory);
	if (error < 0) {
		printf("ERROR[%d]: Load OS failed.\n", error);
		goto err_load;
	}

	/* Setup KVM memory region */
	region.slot = 0;
	region.flags = 0;
	region.guest_phys_addr = 0x1000;
	region.memory_size = VM_MEMORY_SIZE;
	region.userspace_addr = (uint64_t)vm_memory;
	error = ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);
	if (error < 0) {
		printf("ERROR[%d]: KVM_SET_USER_MEMORY_REGION.\n", error);
		goto err_load;
	}

	/* Create Single VCPU */
	vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, (unsigned long)0);
	if (vcpu_fd < 0) {
		printf("ERROR[%d]: KVM_CREATE_VCPU.\n", vcpu_fd);
		error = vcpu_fd;
		goto err_load;
	}

	/* Obtain and check KVM size on Running-time */
	kvm_run_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
	if (kvm_run_size < 0 || kvm_run_size < sizeof(struct kvm_run)) {
		error = kvm_run_size > 0 ? -1 : kvm_run_size;
		printf("ERROR[%d]: KVM_GET_VCPU_MMAP_SIZE.\n", error);
		goto err_load;
	}

	/* Bind kvm_run with vcpu to obtain more running information */
	kvm_run = mmap(NULL,
		       kvm_run_size,
		       PROT_READ | PROT_WRITE,
		       MAP_SHARED,
		       vcpu_fd,
		       0);
	if (!kvm_run) {
		perror("ERROR: Bind kvm and vcpu.");
		error = -1;
		goto err_load;
	}

	/* Obtain Special Registers */
	error = ioctl(vcpu_fd, KVM_GET_SREGS, &sregs);
	if (error < 0) {
		printf("ERROR[%d]: KVM_GET_SREGS.\n", error);
		goto err_sregs;
	}
	/* Set Code/Data segment on address 0, and load OS here */
	sregs.cs.base = 0;
	sregs.cs.selector = 0;
	sregs.ss.base = 0;
	sregs.ss.selector = 0;
	sregs.ds.base = 0;
	sregs.ds.selector = 0;
	sregs.es.base = 0;
	sregs.es.selector = 0;
	sregs.fs.base = 0;
	sregs.fs.selector = 0;
	sregs.gs.base = 0;
	sregs.gs.selector = 0;
	error = ioctl(vcpu_fd, KVM_SET_SREGS, &sregs);
	if (error < 0) {
		printf("ERROR[%d]: KVM_SET_SREGS.\n", error);
		goto err_sregs;
	}

	/* Setup main entry from 0x1000 */
	memset(&regs, 0, sizeof(struct kvm_regs));
	regs.rip	= 0x0000000000001000;
	regs.rflags	= 0x0000000000000002ULL;
	regs.rsp	= 0x00000000ffffffff;
	regs.rbp	= 0x00000000ffffffff;
	error = ioctl(vcpu_fd, KVM_SET_REGS, &regs);
	if (error < 0) {
		printf("ERROR[%d]: KVM_SET_REGS.\n", error);
		goto err_sregs;
	}

	/* Running Virtual-Machine */
	while (1) {
		/* Starting */
		error = ioctl(vcpu_fd, KVM_RUN, NULL);
		if (error < 0) {
			printf("ERROR[%d]: KVM_RUN.\n", error);
			goto err_sregs;
		}

		/* Capture Exit reason */
		switch (kvm_run->exit_reason) {
		case KVM_EXIT_HLT:
			printf("KVM_EXIT_HLT\n");
			return 0;
		case KVM_EXIT_IO: {
			uint8_t data = *(char *)(((char *)(kvm_run) + 
						kvm_run->io.data_offset));
			putchar(data);
			if (data == '\n')
				sleep(6);
			break;
		}
		case KVM_EXIT_FAIL_ENTRY:
			printf("KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_"
			       "reason = %#llx", (unsigned long long)
			kvm_run->fail_entry.hardware_entry_failure_reason);
			break;
		case KVM_EXIT_INTERNAL_ERROR:
			printf("KVM_INTERNAL_ERROR: suberror = %#x\n",
					kvm_run->internal.suberror);
			break;
		default:
			break;
		}
			
	}


	/* Unbind kvm and vcpu */
	munmap(kvm_run, kvm_run_size);
	/* Free Virtual-Machine Memory */
	munmap(vm_memory, VM_MEMORY_SIZE);

	/* Close KVM */
	close(kvm_fd);

	printf("Hello Application Demo on BiscuitOS.\n");
	return 0;

err_sregs:
	munmap(kvm_run, kvm_run_size);
err_load:
	munmap(vm_memory, VM_MEMORY_SIZE);
err_huge:
	close(huge_fd);
err_version:
	close(kvm_fd);
err_open:
	return error;
}
{% endhighlight %}

源代码较长，这里分段进行讲解，这段代码实现了一个最简单的 QEMU-KVM 逻辑，创建了一个虚拟机，虚拟机运行的时候向指定 IO 端口写入数据。这部分代码的实现尽量精简，只保留核心功能代码，源码解析如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000617.png)

函数定义了 KVM_PATH 指向了 "/dev/kvm" 节点，KVM 驱动安装成功之后，会在 "/dev" 目录下创建该节点。接着虚拟机操作系统的目录定义在宏 BISCUITOS_OS_PATH 内，虚拟机启动的时候就会运行这个操作系统里面的代码. BISCUITOS_HUGE_PATH 宏指向了用于支持 2MiB HugePage 的路径。 VM_MEMORY_SIZE 宏指明了虚拟机物理内存的长度，这里定义为 16 MiB. ISO_BUNCK 宏表示虚拟机在加载 OS 镜像的时候，一次性拷贝内容的长度，这里按 PAGE_SIZE 进行拷贝. KVM_VERSION 宏定义了 KVM 应用程序版本为 12，用于与内核 KVM 版本做校验。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000591.png) 

BiscuitOS_load_OS() 函数用于将虚拟机操作系统的内容加载到虚拟机的物理内存上。函数的实现很简单，memory 参数对应虚拟机的物理内存，虚拟机操作系统的代码则存储在 BISCUITOS_OS_PATH 指向的路径里，函数通过 open() 函数以只读的形式打开了虚拟机操作系统对应的文件，并调用 read() 函数循环的将操作系统的内容拷贝到虚拟机物理内存上，直到把全部的文件拷贝到虚拟机物理内存为止。最后函数调用 close() 关闭文件并退出.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000592.png)

在应用程序的 main() 函数里，函数首先调用 open() 以可读可写和 O_CLOEXEC 的方式打开 "/dev/kvm" 节点，并将文件句柄存储在 kvm_fd 里。函数接着调用 ioctl() 函数，将 KVM_GET_API_VERSION 命令通过 kvm_fd 句柄传递给内核的 KVM 模块，以此获得内核 KVM 模块支持的版本信息。如果函数在获得该信息之后，与 KVM_VERSION 做对比，如果不相等，那么虚拟机将无法创建. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000618.png)

函数接下来调用 ioctl() 函数，通过 kvm_fd 句柄向内核 KVM 模块传递 KVM_CREATE_VM 命令，以此告诉内核 KVM 创建虚拟机. 函数成功执行之后，内核 KVM 模块会返回一个文件句柄，这个文件句柄用于表示新创建的虚拟机，函数将其值存储在 vm_fd 变量里。函数在 95 行打开了 HugePage 所在的文件，并基于打开的文件句柄 huge_fd, 函数继续在 104 行调用 mmap() 函数映射一段长度为 VM_MEMORY_SIZE 的虚拟内存，这段内存作为虚拟机的物理内存使用，接下来函数在 119 行调用 BiscuitOS_load_OS() 函数将虚拟机操作系统加载到虚拟机的物理内存上。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000594.png)

应用程序准备好虚拟机的物理内存之后，需要将虚拟机物理内存信息告诉内核 KVM 模块，以此创建 KVM memory slot, 这里不展开介绍。函数定义了一个 struct kvm_userspace_memory_region 的数据结构，这个数据结构主要用于将用户空间创建的一段虚拟地址传递给内存 KVM 模块，以此知道虚拟机物理内存信息。114 行定义了该 region 的 slot ID 为 0，如果了解过内核 KVM kvm_memory_slot 的开发者应该会对这个比较熟悉，slot ID 用于查找多个内存区域。116 行将 guest_phys_addr 设置为 0x1000, 这也就告诉内核 KVM 模块，这块内存区域对应虚拟机物理内存的起始地址是 0x1000. 这里将 guest_phys_addr 设置为 0x1000 之后，虚拟机操作系统的代码就直接加载到了虚拟机物理内存 0x1000 处，并且虚拟机物理内存 0x1000 之后才可以使用，0x1000 之前用于存储虚拟机内存全局页表. 117 行 memory_size 设置为 VM_MEMORY_SIZE 表示这段虚拟机物理内存的长度为 16 MiB. 118 行将虚拟机物理内存对应的虚拟地址存储在 userspace_addr 变量里，如果有虚拟化背景的童鞋可能知道，这里就是所谓的 HVA 地址。函数设置完毕之后，通过 vm_fd 句柄调用 ioctl() 函数，将 KVM_SET_USER_MEMORY_REGION 传递给内核 KVM 模块，以此为虚拟机新增一块物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000595.png)

设置完虚拟机物理内存之后，函数在 126 行通过 vm_fd 句柄，调用 ioctl() 函数向内核 KVM 模块传递 KVM_CREATE_VCPU 命令，以此告诉内核 KVM 模块为其创建一个 VCPU. VCPU 开发者可以理解为虚拟机的 CPU，虚拟机可以支持多个 VCPU. VCPU 创建成功之后，函数在 134 行同样调用 ioctl() 函数传递 KVM_GET_VCPU_MMAP_SIZE 命令，以此获得 VCPU 运行状态区域的大小，这个区域存储了 VCPU 运行时的状态信息。在获得 VCPU 运行状态区域大小之后，函数调用 mmap() 函数将这个区域映射到应用程序空间，并使用 struct kvm_run 数据结构进行存储。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000596.png)

应用程序在设置完 VCPU 相关信息之后，接下来在虚拟机运行之前，需要设置虚拟机运行前寄存器的状态，此时函数在 155 行通过 vcpu_fd 句柄调用 ioctl() 函数向内核 KVM 传递 KVM_GET_SREGS 命令，以此获得 VCPU 对应的寄存器信息，这些信息在用户空间使用 struct kvm_sregs 数据结构进行维护。接着函数设置虚拟机的 CS/SS/DS/ES/FS/GS 段寄存器，均指向 0，这样一个预设好的环境已经准备好，接着函数通过 vcpu_fd 句柄调用 ioctl() 函数向内核 KVM 传递 KVM_SET_SREGS 命令，以便将刚设置的段寄存器写入到虚拟机初始化环境里，然后函数定义了一个 struct kvm_regs 数据结构，用于存储一些通用寄存器，这些寄存器用于设置虚拟机运行时的初始状态。函数 181 行将 RIP 寄存器设置为 0x1000, 那么虚拟机上点运行之后直接在 0x1000 物理内存处运行，接着将 rflags 寄存器设置为 0x2, rsp 和 bsp 都设置为 0xffffffff, 以此定义堆栈的栈顶位置，准备好要设置的寄存器之后，函数通过 vcpu_fd 句柄调用 KVM_SET_SREGS 命令传递给内核 KVM, 以便设置虚拟机运行时的相关寄存器初始值.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000597.png)

一切准备完毕之后，函数准备启动虚拟机，此时使用一个 while() 循环循环运行，由于虚拟机运行过程中会由于多种原因退出，因此在退出之后确定退出原因并提供解决办法之后，虚拟机又通过 while 循环重新运行起来。函数在 194 行通过 vcpu_fd 句柄，调用 ioctl() 函数向内核 KVM 传递 KVM_RUN 命令以此运行虚拟机，执行这个命令之后，虚拟机就在 non-root 模式下运行，虚拟机会一致运行虚拟机操作系统里面的代码，直到某些情况退出执行，退出之后，函数会捕捉虚拟机退出的原因，这里只是简单的列举几种退出的原因。其中与本实例有关的就是 KVM_EXIT_IO 方式退出，也就是虚拟机操作系统内部如果访问 IO，那么虚拟机就会退出，此时函数捕捉到这个退出条件之后，在 206 - 207 行将退出端口的数据打印到标准输出。至此一个虚拟机的运行过程已经完整执行完毕, 以上函数实际运行的效果如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000616.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

###### <span id="D1">Virtual-Machine OS</span>

{% highlight bash %}
#
# Virtual-Machine OS
#
# (C) 2020.10.24 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

.globl _BiscuitOS_start
        .code16
        .text

_BiscuitOS_start:
        mov     $0x1040, %bx

loop:
        mov     (%bx), %ax
        out     %ax, $0x10
        sub     $'\n', %ax
        jz      _BiscuitOS_start
        inc     %bx
        jmp     loop

        .data
        .org 0x1040
        .ascii  "Hello BiscuitOS :)\n"

        .bss
{% endhighlight %}

BiscuitOS-Virtual-Machine.S 文件存储 Vritual-Machine 操作系统的代码，该代码就是虚拟机运行之后，虚拟机操作系统执行的代码，代码很简单，仅用于功能说明。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000598.png)

操作系统使用汇编代码实现，操作系统运行之后，会在 0x1040 处存放一段字符串 "Hello BiscuitOS :)\n", 汇编函数在 15 行处将字符串的地址 0x1040 存储在 BX 寄存器中，然后在 18 行将 BX 寄存器指向的地址的值存储在 AX 寄存器里，然后在 19 行将 AX 寄存器的值写入到 IO 端 0x10 处，然后在 20 行将 '\n' 字符的值与 AX 寄存器内的值相减，如果相减的结果为 0，那么函数执行 21 行的 jz 指令跳转到 14 行处执行; 反之函数在 22 行调用 inc 指令增加 BX 寄存器的值，最后在 23 行跳转到 17 行处执行。该汇编函数的逻辑就是不停的将 "Hello BiscuitOS :)\n" 字符串输出到 0x10 IO 端口上。实际运行的效果如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000589.png)

---------------------------------

###### <span id="D2">Makefile scripts</span>

{% highlight bash %}
#
# KVM Application Project
#
# (C) 2020.10.24 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

## Target
ifeq ("$(origin TARGETA)", "command line")
TARGET			:= $(TARGETA)
else
TARGET			:= BiscuitOS_app
endif

OS_NAME			:= _BiscuitOS-VM.iso
OS_ASM			:= BiscuitOS-Virtual-Machine

## Source Code
SRC			+= main.c

## CFlags
LCFLAGS			+= -DCONFIG_BISCUITOS_APP
## Header
LCFLAGS			+= -I./ -I$(PWD)/include

DOT			:= -
## X86/X64 Architecture
ifeq ($(ARCH), i386)
CROSS_COMPILE	=
LCFLAGS		+= -m32
DOT		:=
else ifeq ($(ARCH), x86_64)
CROSS_COMPILE   :=
DOT		:=
endif

# Compile
B_AS		= $(CROSS_COMPILE)$(DOT)as
B_LD		= $(CROSS_COMPILE)$(DOT)ld
B_CC		= $(CROSS_COMPILE)$(DOT)gcc
B_CPP		= $(CC) -E
B_AR		= $(CROSS_COMPILE)$(DOT)ar
B_NM		= $(CROSS_COMPILE)$(DOT)nm
B_STRIP		= $(CROSS_COMPILE)$(DOT)strip
B_OBJCOPY	= $(CROSS_COMPILE)$(DOT)objcopy
B_OBJDUMP	= $(CROSS_COMPILE)$(DOT)objdump

## Install PATH
ifeq ("$(origin INSPATH)", "command line")
INSTALL_PATH		:= $(INSPATH)
else
INSTALL_PATH		:= ./
endif

all:
	@$(B_CC) $(LCFLAGS) -o $(TARGET) $(SRC)
	@$(B_AS) -32 $(OS_ASM).S -o $(OS_ASM).o
	@$(B_LD) -m elf_i386 --oformat binary -N -e _BiscuitOS_start \
	         -Ttext 0x10000 -o $(OS_NAME) $(OS_ASM).o

install:
	@cp -rfa $(TARGET) $(INSTALL_PATH)
	@mkdir -p $(INSTALL_PATH)/firmware/
	@cp -rfa $(OS_NAME) $(INSTALL_PATH)/firmware/
	@chmod 755 RunBiscuitOS.sh
	@cp -rfa RunBiscuitOS.sh $(INSTALL_PATH)

clean:
	@rm -rf $(TARGET) *.o *.iso
{% endhighlight %}

以上脚本用于编译并生成虚拟机运行的文件。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

###### <span id="D3">RunBiscuitOS.sh</span>

{% highlight bash %}
#!/bin/ash
# 
# Establish HugePage for KVM

mkdir -p /mnt/HugePagefs
mount none /mnt/HugePagefs -t hugetlbfs

echo 20 > /proc/sys/vm/nr_hugepages
BiscuitOS-UKVM-2M-default
{% endhighlight %}

RunBiscuitOS.sh 脚本会被安装到 BiscuitOS 的 /usr/bin 目录下，脚本的用途就是在 "/mnt/HugePagefs" 挂载一个支持 2MiB HugePage，并运行 BiscuitOS-UKVM-2M-default.


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)
