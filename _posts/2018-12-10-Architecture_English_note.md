---
layout: post
title:  "Computer Architecture English note"
date:   2018-12-10 08:45:33 +0800
categories: [HW]
excerpt: Computer Architecture English note.
tags:
  - CS
---

![](/assets/PDB/BiscuitOS/kernel/INDA00001.png)

#### List

> - [A](#A)
>
>   - [aware](#aware)
>
> - B
>
> - [C](#C)
>
>   - [consult](#consult)
>
>   - [corresponds](#corresponds)
>
> - D
>
> - E
>
> - F
>
> - G
>
> - H
>
> - I
>
> - J
>
> - K
>
> - L
>
> - M
>
> - N
>
> - [O](#O)
>
>   - [occupy](#occupy)
>
> - [P](#P)
>
>   - [permit](#permit)
>
> - Q
>
> - R
>
>   - [relevant](#relevant)
>
>   - [retain](#retain)
>
> - [S](#S)
>
>   - [subsequently](#subsequently)
>
> - T
>
> - U
>
> - V
>
> - W
>
> - X
>
> - Y
>
> - Z


----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

###### <span id="aware">aware</span>

**察觉**

* There is no way for software to be **aware** that translation for pages have been used for large page.
* 软件没有办法**察觉**这个映射了大页.

----------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

# B

----------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

###### <span id="consult">consult</span>

**查询**

* The processor may not **consult** the paging structures in memory. 
* 处理器也许不会去内存里**查询**页表.

###### <span id="corresponds">corresponds</span>

**相似的**

* The page number of linear address **corresponds** to a TLB entry.
* 与线性地址页号**相似的** TLB entry.

----------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000D.jpg)

# D

----------------------------------

<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000E.jpg)

# E

----------------------------------

<span id="F"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

# F

----------------------------------

<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

# G

----------------------------------

<span id="H"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

# H

----------------------------------

<span id="I"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

# I

----------------------------------

<span id="J"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000J.jpg)

# J

----------------------------------

<span id="K"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

# K

----------------------------------

<span id="L"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

# L

----------------------------------

<span id="M"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

# M

----------------------------------

<span id="N"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000N.jpg)

# N

----------------------------------

<span id="O"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000O.jpg)

# O

###### <span id="occupy">occupy</span>

**占用**

* The user space **occupies** the lower portion of the address space, starting from address 0 and extending up to the TASK_SIZE.
* 用户空间占用了低地址部分，该部分从 0 开始一直拓展到 TASK_SIZE 处。

{% highlight ruby %}
0                                               TASK_SIZE            4G
+-----------------------------------------------+---------------------+
|                                               |                     |
|                                               |                     |
|                                               |                     |
+-----------------------------------------------+---------------------+
| <---------------- User Space ---------------->|
{% endhighlight %}

----------------------------------

<span id="P"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

###### <span id="permit">permit</span>

**允许**

* The processors **permit** software to specify the memory type.
* 处理器**允许**软件指定内存类型.

----------------------------------

<span id="Q"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

# Q

----------------------------------

<span id="R"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

###### <span id="relevant">relevant</span>

**相关的**

* The processor modifies the **relevant** paging-structure entries in memory.
* 处理器修改了内存里**相关的**页表 Entry.

###### <span id="retain">retain</span>

**预留**

* The processor may **retain** a TLB entry.
* 处理器可能会**预留**一个 TLB entry.

----------------------------------

<span id="S"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

###### <span id="subsequently">subsequently</span>

**随后地**

* The processor **subsequently** modifies the page-structures entries.
* 处理器**随后**修改了页表 Entry.

----------------------------------

<span id="T"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

# T

----------------------------------

<span id="U"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000U.jpg)

# U

----------------------------------

<span id="V"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000V.jpg)

# V

----------------------------------

<span id="W"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000W.jpg)

# W

----------------------------------

<span id="X"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

# X

----------------------------------

<span id="Y"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Y.jpg)

# Y

----------------------------------

<span id="Z"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Z.jpg)

# Z
