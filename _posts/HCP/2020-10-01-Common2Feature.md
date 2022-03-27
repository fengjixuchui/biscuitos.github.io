---
layout: post
title:  "BiscuitOS 一带一路计划"
date:   2020-10-01 06:00:00 +0800
categories: [HW]
excerpt: Common for knowledge.
tags:
  - Common2Feature
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [BiscuitOS 一带一路计划介绍](#A)
>
> - [Lesson 0](#Lesson0)
>
> - [Lesson 1](#Lesson1)
>
> - [Lesson 2](#Lesson2)
>
> - [Lesson 3](#Lesson3)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### BiscuitOS 一带一路计划介绍

欢迎参加 BiscuitOS 社区一带一路活动。首先给大家介绍一下什么是一带一路。内核的学习可谓是困难重重，更不要说一个初学者独自学习内核了的痛处，笔者也是从一个初学者过来，期间遇到的困难至今难忘。结合笔者多年的内核学习经验，总结出内核学习需要做的三件事: 第一件事，内核学习最关键的就是实践，实践是让你靠近问题的本质最有效的方法，也是自己对内核世界的一种认知; 第二件事，内核学习需要靠谱有效的文档资料，在文档的指引上不会在内核里迷路; 第三件事则是，内核学习需要有人指点方向，一个有效的指点能让内核学习更有方向性，并给学习者提供一定信心去面对困难。

为什么笔者要推行一带一路计划呢？ 内核学习的方法多种多样，有的喜欢写书出视频，有的喜欢写博客发文章，在各种各种的方法都会发现一个共同的特点，那就是什么才是最有效的内核学习。笔者从事内核开发多年，也积累了一定的学习经验，想通过这个活动和大家分享笔者的学习方法，也让想学习内核的小伙伴能够顺利的入门 Linux 内核，并作为一个 "l灵魂伴侣" 为小伙伴们指引前进的道路。

如何推行 "一带一路" 计划呢？ 笔者认为内核的学习分为三个阶段，第一个阶段要学会 "用内核"，也就是能够熟练的使用内核各个模块接口。第二个阶段要学会 "思考内核"，这个阶段能够对某个功能或函数顺藤摸瓜去发现其本源. 第三阶段则是 "创"，有了前两个的基础，到达这个阶段就可以用自己的想法创意去内核中实现。为了让小伙伴能依次达到上面的三个阶段，我会以 "指引者" 的角色，为每一个阶段的你通过派发实践课程的方法，指引你完成这个阶段所有课程，并分享一下学习心得，让小伙伴们获得这个阶段所具备的能力和想法，最终达到第三阶段。

小伙伴们该怎么做呢？ 由于每个小伙伴的学习时间无法统一，因此每个小伙伴可以自行安排时间，只要自己准备好时间之后，就可以向笔者申请课程进行学习，每当学习完成一个课程之后，提交相应的作业，符号要求之后，小伙伴可以向笔者申请下一课。如果学习期间遇到问题，可以直接在 "BiscuitOS 一带一路社区" 微信群进行提问，里面都是同道中人，会尽可能帮你解决问题。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="Lesson0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### Lesson0

第一个必做的课，搭建一个你喜欢架构的 BiscuitOS，以后的课程都要基于 BiscuitOS 进行实践，因此小伙伴请选择一个你喜欢的架构进行搭建，文档如下: 

> [BiscuitOS on ARM](/blog/Linux-5.0-arm32-Usermanual/)
>
> [BiscuitOS on ARM64](/blog/Linux-5.0-arm64-Usermanual/)
>
> [BiscuitOS on I386](/blog/Linux-5.0-i386-Usermanual/)    
>
> [BiscuitOS on X64](/blog/Linux-5.0-x86_64-Usermanual/)
>
> [BiscuitOS on RISCV32](/blog/Linux-5.0-riscv32-Usermanual/)
>
> [BiscuitOS on RISCV64](/blog/Linux-5.0-riscv64-Usermanual/)

###### 作业要求

感谢参加 BiscuitOS 社区的一带一路活动，第一次作业的要求是:

* 制作一个你喜欢 arch 的 5.0 的 BiscuitOS 并运行截图
* 按照驱动章节内容，完成一个 platform 驱动
* 按照应用程序章节，完成一个应用程序

截图发给笔者，合格之后发下一个作业。如果遇到问题，请直接在 "一带一路群" 里提问.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="Lesson1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### Lesson1

恭喜小伙伴完成了第一个课程，你的付出会给你的未来带来巨大的收益。在 Linux 的世界里有很多复杂难懂的子系统、函数或者代码含义，你在学习的时候是否也遇到过这个问题。如果遇到，那么不妨试试我提供的这个方法。在 Linux 内核中，任何复杂的机制都是有最简单的零件构成的，因此我们首先学会将一个复杂的机制拆成一个一个简单的零件，然后将一个一个零件研究透彻，最后再将所有的零件全部组合成原先复杂的机制。经过这样一个实践过程大概率就攻克了这个复杂的机制，因此我在这里推荐并指引你使用这种方法，这个阶段要学的基础很多，但很有用，就像建立大厦的地基一样，地基的坚实程度决定了大楼的高度。虽然困难重重，但咬咬牙坚持下去。

第二课我们从 bitmap 中开始，内核中很多概念的核心都是基于 bitmap 构建，例如文件的 fd、cpumask、Bootmem 分配器、以及 CMA 分配器等一些算法中也会经常用到，因此这一课我们学习 bitmap 的原理，实践加使用。小伙伴请参考如下文档进行实践:

> [Linux Bitmap 机制](/blog/BITMAP/)

###### 作业要求

课程的作业就是提交一个基于第一个 platform 驱动之上实现的一个 bitmap 逻辑，具体逻辑自行设定，截图提交代码为过，若有问题请直接在一带一路群里提问。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="Lesson2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000E.jpg)

#### Lesson2

BiscuitOS 一带一路计划第三个作业是内核双链表，内核双链表可以说是构成内核多种子系统的基础，因此掌握好内核双链表及其必要, 小伙伴参考如下文档进行学习:

> [内核双链表学习](/blog/LIST/)

###### 作业要求

第二个作业完成之后，邀请你参加人类知识共同体计划，详细计划参见 

> [BiscuitOS "人类知识共同体" 计划](/blog/Human-Knowledge-Common/)

如果你愿意参加，请根据这个计划的提示，提交一份关于双链表的独立程序到 BiscuitOS。如果不参加，也提交一个基于第一个 platform 驱动之上实现的一个双链表逻辑，具体逻辑自行设定，截图提交代码为过，若有问题请直接在一带一路群里提问。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="Lesson3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

#### Lesson3

BiscuitOS 一带一路计划第四个作业是二叉树，内核中树的使用可谓是最广泛的一类数据，例如红黑树，优先查找树，基数树等，但这些树由于太复杂导致学习难度一度提高，学好树也就掌握内核核心更进一步。小伙伴们不要被各种树给吓到，还记得之前我和大家布道的一个思想吗？任何复杂的事物都是由最简单的事物构成的，我们无法一招击破复杂的事物，那么可以将复杂的事物一个一个拆分成细小的事物，逐一击破，最后将逐一击破的小事物合成一个复杂事物，这个复杂事物不就解决了吗。说道这里小伙伴们是否有一定信心解决这个复杂的问题了吗，因此我作为 "指引者"，我帮大家已经把复杂的事物已经拆分成细小事物了，接下来大家就一个一个击破这些细小的事物就行。

二叉树作为所有树的基础模型，学习二叉树对理解和认知其他复杂树起到一个很好方向，因此本节课的内容就是学习二叉树，具体课程如下:

> [二叉树学习](/blog/Tree_binarytree/)

作业提交也是提交一份关于二叉树独立代码，代码逻辑自行设计，有问题请在 "BiscuitOS 一带一路" 群讨论.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
