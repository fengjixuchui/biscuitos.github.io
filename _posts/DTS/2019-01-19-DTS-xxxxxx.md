---
layout: post
title:  "DTS"
date:   2019-07-22 05:30:30 +0800
categories: [HW]
excerpt: DTS.
tags:
  - DTS
---

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

> [Github: BBBXXX](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/API/BBBXXX)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------


> - [DTS 简介](#A001)
>
> - [ARM 引入 DTS 原因](#A002)
>
> - []


### <span id="A001">DTS 简介</span>

DTS 是为 Linux 提供一种硬件信息的描述方法，以此代替源码中的
硬件编码 (hard code)。DTS 即 Device Tree Source 设备树源码，
Device Tree 是一种描述硬件的数据结构，起源于 OpenFirmware (OF).
在 Linux 2.6 中， ARM 架构的板级硬件细节过多的被硬编码在 arch/arm/plat-xxx
和 arch/arm/mach-xxx (比如板上的 platform 设备，resource，
i2c_board_info, spi_board_info 以及各种硬件的 platform_data）,
这些板级细节代码对内核来讲只不过是垃圾代码。而采用 Device Tree 后，
许多硬件的细节可以直接透过它传递给 Linux，而不再需要在 kernel 中
进行大量的冗余编码。

### <span id="A002">ARM 引入 DTS 的原因</span>

每次正式发布 linux kernel release 之后都会有两周的 merge window，
在这个窗口期间，kernel 各个子系统的维护者都会提交各自的 patch，将测
试稳定的代码请求并入 kernel mainline。每到这个时候，**Linus** 就会比较
繁忙，他需要从各个子系统维护者的分支上获得最新的代码并 merge 到自己的
kernel source tree 中。其中有一个维护者 Tony Lindgren，
OMAP development tree 的维护者，发送了一个邮件给 Linus，请求提交
OMAP 平台代码修改，并给出一些细节描述：

{% highlight bash %}
1. 简单介绍本次修改
2. 关于如何解决 merge conficts
{% endhighlight %}

一切都很正常，也给出足够的信息，然而，这好是这个 pull request
引起了一场针对 ARM Linux 内核代码的争论。也许 Linus 早就对 ARM
相关的代码早就不爽了，ARM 的 merge 工作量不仅较大，而且他认为 ARM
很多的代码都是垃圾，代码里有很多愚蠢的 table，从而导致了冲突。因此，
在处理完 OMAP 的 pull request 之后 （Linus 并非针对 OMAP 平台，
只是 Tony Lindgren 撞在枪口上了），Linus 在邮件中写道：

{% highlight bash %}
Gaah.Guys, this whole ARM thing is a f*ching pain in the ass.
{% endhighlight %}

这件事之后， ARM 社区对 ARM 平台相关的 code 做出了如下规范调整，
这个也正是引入 DTS 的原因，如下：

{% highlight bash %}
1. ARM 的核心代码仍然存储在 arch/arm 目录下

2. ARM Soc core architecture code 存储在 arch/arm 目录下

3. ARM Soc 的周边外设模块驱动存储在 drivers 目录下

4. ARM Soc 的特定代码在 arch/arm/mach-xxx 目录下

5. ARM Soc board specific 代码被移除，由 DeviceTree 机制负责传递硬件拓展和硬件信息
{% endhighlight %}

本质上，DeviceTree 改变原来用 hardcode 方式将 HW 配置信息
嵌入到内核代码的方法，改用 bootloader 传递一个 DB 的形式。
对于嵌入式系统，在系统启动阶段，bootloader 会加载内核并将控
制权转移给内核。

更多 DTS 内核历史邮件请查看如下：

> - [讨论引入 FDT 到嵌入式 linux 平台](#A010)
>
> - [Russell King 反对 DTS 加入到 ARM](#A011)
>
> - [Russell 最终被说服接受 DTS 进入 ARM](#A013)
>
> - [David Gibson 辩护 FDT](#A014)

-----------------------------------------------

## <span id="C00">DTS 语法</span>

> - [node-name@unit-addrss](#C001)
>
> - [Path Names](#C002)
>
> - [Property](#C003)
>
> - [compatible](#C200)
>
> - [model](#C202)
>
> - [phandle](#C203)
>
> - [status](#C204)
>
> - [#address-cell and #size-cells](#C205)
>
> - [reg](#C206)
>
> - [virtual-reg](#C207)
>
> - [ranges](#C208)
>
> - [dma-ranges](#C209)





---------------------------------

#### <span id="C001">node-name@unit-addrss</span>

任何一个节点在 DeviceTree 中都有一个名字相对于，请标准格式如下：

{% highlight bash %}
node-name@unit-address
{% endhighlight %}

`node-name` 部分指明了节点的名字，其可由 1 到 31 个字符构成，
字符可以为下面中的任何字符：

![](/assets/PDB/BiscuitOS/boot/BOOT000215.png)

`node-name` 部分以一个小写或大写字符串开始，用于指明节点对应设备
的类型。`unit-address` 成分用于说明节点与总线的关系，一般为设备
在总线上的位置。`unit-address` 由上表的字符串构成。`unit-address`
成分的值必须与节点 reg 属性的值一致。如果节点没有 reg 属性，那么
`@unit-address` 必须被省略，并且节点的名字必须与同一级别的其他节点
通过名字区分开来。当节点绑定到特定总线，可以使用 reg 属性和 `@unit-address`
进行更具体的指定。

![](/assets/PDB/BiscuitOS/boot/BOOT000216.png)

root 节点没有 `node-name` 和 `unit-address`, 其使用 `/` 代表。
在上图中，1) 节点名字为 cpu 通过 `unit-address` 进行区分，其值
为 0 和 1，即 cpu@0 与 cpu@1. 2) 名字为 ethernet 的节点通过它们
的 `unit-address` 部分进行确认，分别是 `ethernet@fe002000` 与
`ethernet@fe003000`.

###### DTS 推荐节点名字

节点的名字应该通用和反应设备的功能，而不是精确的编程模型，
下表是推荐的节点名字：

{% highlight bash %}
adc
accelerometer
atm
audio-codec
audio-controller
backlight
bluetooth
bus
cache-controller
camera
can
charger
clock
clock-controller
compact-flash
cpu
cpus
crypto
disk
display
dma-controller
dsp
eeprom
efuse
endpoint
ethernet
ethernet-phy
fdc
flash
gnss
gpio
gpu
gyrometer
hdmi
i2c
i2c-mux
ide
interrupt-controller
isa
keyboard
key
keys
lcd-controller
led
leds
led-controller
light-sensor
magnetometer
mailbox
mdio
memory
memory-controller
mmc
mmc-slot
mouse
nand-controller
nvram
oscillator
parallel
pc-card
pci
pcie
phy
pinctrl
pmic
pmu
port
ports
power-monitor
pwm
regulator
reset-controller
rtc
sata
scsi
serial
sound
spi
sram-controller
ssi-controller
syscon
temperature-sensor
timer
touchscreen
usb
usb-hub
usb-phy
video-codec
vme
watchdog
wifi
{% endhighlight %}

---------------------------------

#### <span id="C002">Path Names</span>

一个节点在 DeviceTree 中，可以通过根节点到节点的全路径唯一
指定。全路径的标准定义如下：

{% highlight bash %}
/node-name-1/node-name-2/node-name-N
{% endhighlight %}

例如下图中 cpu#1 节点的全路径为：

{% highlight bash %}
/cpus/cpu@1
{% endhighlight %}

![](/assets/PDB/BiscuitOS/boot/BOOT000216.png)

根节点的全路径是 `/`。1) 如果一个节点的全路径是明确的，那么可以忽略
节点的 `unit-address`。 2) 如果一个节点的全路径是不明确的，那么定义
位未定义的。

---------------------------------

## <span id="C003">Property</span>

每个节点在 DeviceTree 中都包含多个属性，这些属性由于描述
节点的性质等。属性由名字和属性值构成。

#### 属性名字

属性名字有 1 到 31 个字符构成，字符可以从下表中选取：

![](/assets/PDB/BiscuitOS/boot/BOOT000215.png)

非标准的属性名字应制定唯一的字符串前缀，例如一个 stock ticker 符号，
识别公司和组织的名字去定义属性名字，例如：

{% highlight bash %}
fsl,channel-fifo-len
ibm,ppc-interrupt-server#s
linux,network-index
{% endhighlight %}

#### 属性值

属性值包含与属性相关联的信息的零个或多个字节的数组。如果
属性用于传递真假信息，那么属性值可能有空值。在这种情况下，
如果属性存在，那么属性值就是真；如果属性不存在，那么属性值
就是假。下面详细介绍属性值的类型。

> - [empty](#C010)
>
> - [u32](#C011)
>
> - [u64](#C012)
>
> - [string](#C013)
>
> - [phandle](#C014)
>
> - [stringlist](#C015)
>
> - [prop-encoded-array](#C016)

------------------------------------

###### <span id="C010">empty</span>

如果属性值的类型是 `<empty>`，那么属性表达的信息就是真假，当
属性存在与节点中，那么其值就是真；反之，如果属性不在节点中，
那么属性的属性值就是假，例如：

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                device_hotplug;
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

对于上诉例子的属性 `device_hotplug` ，其属性值就是真；
对于上诉例子的属性 `device_moveable`, 由于该属性不在
节点中，那么其属性值就是假。

---------------------------------

###### <span id="C011">u32</span>

如果属性值的类型是 u32，那么其值以大端模式表示，例如:

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                bs_val = <0x11223344>;
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

在上面的例子中，属性 bs_val 的属性类型就是 u32，由于其大端模式
表示，其属性值是 0x11223344，其布局如下：

{% highlight bash %}
address+0        11
address+1        22
address+2        33
address+3        44
{% endhighlight %}

因此开发者在使用给属性设置 u32 属性值时，应该充分考虑属性值
以大端模式存储。

-----------------------------------

###### <span id="C012">u64</span>

如果属性值的类型是 u64，那么其值由两个 u32 整型以大端模式
构成。u64 属性值的第一个 u32 值存储 u64 值的 MSB 部分，第二个
u32 值存储 u64 的 LSB 部分，例如：

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                bs_val = <0x11223344 0x55667788>;
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

bs_val 属性值是一个 u64，在 DeviceTree 中由两个 u32 构成，
bs_val 的属性值是 0x122334455667788. 其内存布局如下：

{% highlight bash %}
address+0        11
address+1        22
address+2        33
address+3        44
address+4        55
address+5        66
address+6        77
address+7        88
{% endhighlight %}

因此开发者在使用给属性设置 u64 属性值时，应该充分考虑属性
值以大端模式存储。

---------------------------------

#### <span id="C013">string</span>

如果属性值是 string 类型，那么其值是可打印，并以 `\0` 结尾
的字符串。例如：

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                bs_val = <0x11223344 0x55667788>;
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

属性 device_type 的属性值就是 string，其值是 "memory",
其值在内存里的布局如下：

{% highlight bash %}
address+0        'm'
address+1        'e'
address+2        'm'
address+3        'o'
address+4        'r'
address+5        'y'
address+6        '\0'
{% endhighlight %}

---------------------------------

#### <span id="C014">phandle</span>

如果属性值类型是 phandle，那么其值也是一个 u32，该 u32 整型用于
引用 DeviceTree 里的其他的节点。任意可引用的节点都可以定义为一个
唯一的 u32 值。例如下面节点：

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                bs_val = <0x11223344 0x55667788>;
                reg = <0x60000000 0x40000000>;
                ref-phandle = <&node0>;

                node0: node@1 {
                      reg = <1>;
                };
        };
{% endhighlight %}

属性 ref-phandle 的属性值就是一个 phandle 类型，其指向 node0
节点。

---------------------------------

#### <span id="C015">stringlist</span>

如果属性值类型是 stringlist，那么其值由一连串的 string 构成，
例如：

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                bs_strings = "uboot", "kernel", "rootfs";
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

bs_strings 属性值的类型就是一个 stringlist，其由三个 string
组成，分别是 "uboot", "kernel", "rootfs"，其内存布局如下：

{% highlight bash %}
address+0        'u'
address+1        'b'
address+2        'o'
address+3        'o'
address+4        't'
address+5        '\0'
address+6        'k'
address+7        'e'
address+8        'r'
address+9        'n'
address+a        'e'
address+b        'l'
....
{% endhighlight %}

---------------------------------

#### <span id="C016">prop-encoded-array</span>

如果属性值的类型是 prop-encoded-array, 那么其值就是
一种混合型，即属性值中含有多种属性，例如：

{% highlight bash %}
 / {
        DTS_demo {
                compatible = "DTS_demo, BiscuitOS";
                status = "okay";
                phy-handle = <&phy0 1 2 3 &phy1 4 5>;
                reset-gpio = <&gpio0 25 0>;
        };

        phy0: phy@0 {
                #phy-cells = <3>;
                compatible = "PHY0, BiscuitOS";
        };

        phy1: phy@1 {
                #phy-cells = <2>;
                compatible = "PHY1, BiscuitOS";
        };

        gpio0: gpio@0 {
                #gpio-cells = <2>;
                compatible = "gpio, BiscuitOS";
        };
 };
{% endhighlight %}

DTS_demo 节点的 phy-handle 属性的属性值就是 prop-encoded-array，
里面包含了 u32 与 phandle 类型属性值。prop-encoded-array 类型
也可以是同类型的数组，例如：


{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                bs_array = <0x12 0x34 0x45 0x87 0x89 0x22>;
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

属性 bs_array 属性值就是一个 prop-encoded-array，
其值就是 u32 的数组。


---------------------------------

#### <span id="C201">compatible</span>

compatible 属性，属性值类型是一个 `<stringlist>`。
compatible 属性值由设备模型相关的一个或多个字符串组成。该属性
由 Client 端设备选取对应驱动，属性值由一连串以终止符结尾的字符串
组成。该属性具有兼容与设备相似的设备驱动，也允许只有一个设备驱动
程序使用该驱动。

compatible 推荐的格式为：

{% highlight bash %}
manufacturer,model
{% endhighlight %}

manufacturer 部分是一个字符串，主要用来描述生产厂商，model
部分用于描述模组信息。例如：

{% highlight bash %}
        Kingnode@60000000 {
                compatile = "fsl,mpc8641","ns16550";
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

在上面的例子中，操作系统首先定位 "fsl,mcpc8641" 设备驱动，
如果驱动没有找到，操作系统才试图定位 "ns16550"。在实际的设备
驱动开发中，如果驱动程序的 of_device_id 结构的 compatible
成员不与 DeviceTree 中节点的 compatible 相同，那么对应的
设备驱动无法运行。

---------------------------------

#### <span id="C202">model</span>

model 属性，属性值是一个 `<string>`。
model 属性用于指定设备厂商的模组信息。推荐的 model
属性值格式如下：

{% highlight bash %}
manufacturer,model
{% endhighlight %}

manufacturer 部分用于指明厂商信息，model 部分则表明模组相关的
信息。例如：

{% highlight bash %}
        Kingnode@60000000 {
                compatile = "fsl,mpc8641","ns16550";
                model = "fsl,MPC8349EMITX";
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

Kingnode 节点的 model 属性值 "fsl,MPC8349EMITX"，fsl 指明了
厂商信息，MPC8349EMITX 指明了模组信息。

---------------------------------

#### <span id="C203">phandle</span>

phandle 属性，属性值类型是 `u32` 类型。
phandle 属性通过一个唯一的 u32 整数指向 DeviceTree 中的其他节点。
例如:

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                bs_val = <0x11223344 0x55667788>;
                reg = <0x60000000 0x40000000>;
                phandle = <&node0>;

                node0: node@1 {
                      reg = <1>;
                };
        };
{% endhighlight %}

memory 节点中，phandle 属性指向了节点 node0。老版本的 DeviceTree
可能会遇到一种不推荐的属性形式 `linux,phandle`。为了兼容，希望客户端
程序也解析 `linux,phandle`, 两种形式都是一致的。大多数 DTS 节点中不会
显示的包含 phandle 属性，只有当使用 DTC 工具将 DTS 转换成 DTB 之后自动
插入 phandle 属性和属性值。例如下面的 DTS 中某节点的描述：

{% highlight bash %}
                bs_cma: cma@61000000 {
                        status = "okay";
                        compatible = "bs-cma";
                        reg = <0x61000000 0x6400000>;
                };
{% endhighlight %}

通过 DTC 工具生成 DTB 之后，再将 DTB 转出 DTS 文件之后，
如下：

{% highlight bash %}
                cma@61000000 {
                        status = "okay";
                        compatible = "bs-cma";
                        reg = <0x61000000 0x6400000>;
                        phandle = <0x12>;
                };
{% endhighlight %}

注意，只有其他节点通过 <phandle> 类型的属性引用了该节点，
该节点生成 DTB 的时候才会自动加入 phandle 属性，否则 DTB
不会自动为节点添加 phandle 属性。另外开发者应该正确区分
phandle 属性和属性值类型是 <pahndle>, 两者不要搞混，

> - [phandle 属性类型](C014)

---------------------------------

#### <span id="C204">status</span>

status 属性，属性值类型是 <string>。
status 属性指明了一个节点的状态，属性值可以为 "okay",
"disabled", "reserved", "fail", "fail-sss". status
使用如下：

{% highlight bash %}
                bs_cma: cma@61000000 {
                        status = "okay";
                        compatible = "bs-cma";
                        reg = <0x61000000 0x6400000>;
                };
{% endhighlight %}

不同的属性值代表不同的含义，具体描述如下：

###### okay

"okay" 属性值指明设备节点对应的设备是可以使用的。当 status
属性设置为 "okay" 之后，操作系统启动后就会运行节点对应的
设备驱动。

###### disabled

"disabled" 属性值表明设备目前不可以运行，但将来可能会运行
(例如有些设备没有插入或者关闭)。

###### reserved

"reserved" 属性值表示可以运行，但不应该被使用。通常情况下，
这是操作系统之外的软件控制设备，例如平台固件。

###### fail

"fail" 属性值表示设备无法运行。在设备上检测到一个严重的错误，
如果不修复，那么设备不可能运行。

###### fail-sss

"fail-sss" 属性值表示设备无法运行，并在设备上检测到一个严重
的错误，如果不修复，设备不太可能运行。"sss" 用于指定设备，
并指示错误的描述。

正常情况下，如果要使用某个设备，那么将 status 设置成 "okay",
不使用则设置为 "disabled".

---------------------------------

#### <span id="C205">#address-cell and #size-cells</span>

"#address-cells" 属性和 "#size-cells" 属性，属性值是
"u32" 类型。"#address-cells" 属性和 "#size-cells" 属性
可以在任意节点中使用，其作用于子节点，并用于描述子节点应该
如何寻址。"#address-cells" 属性定义了用于在子节点的 reg
属性中编码地址域中 u32 使用的个数，"#size-cells" 属性
用于定义了子节点的 reg 属性中编码 size 域中 u32 的个数。
例如：

{% highlight bash %}
        Fil@60000000 {
                compatible = "BS,FIL2356";
                #address-cells = <2>;
                #size-cells = <1>;
                reg = <0x60000000 0x40000000>;
                phandle = <&node0>;

                node0: node@1 {
                      reg = <0 1 0x100>;
                };
        };
{% endhighlight %}

在节点 Fil 中，"#address-cells" 的属性值为 2，代表子节点
reg 属性的地址域由两个 u32 组成，node0 是 Fil 的子节点，
node0 node-name@unit-addrss 的 unit-address 域为 1，所以
node0 节点 reg 属性的地址域也要为 1，结合上述要求，那么
reg 属性的地址域最终为 "0 1"。同理 Fil 节点的 "#size-cells"
属性值为 1，那么 Fil 子节点 reg 属性的 size 域由一个 u32
构成，因此 node0 节点 reg 属性值的 size 域值为 0x100。

"#address-cells" 属性和 "#size-cells" 属性不从节点的祖先
那里继承下来，应该显示的定义这两个属性。在所有包含子节点的
节点中，应该提供 "#address-cells" 属性和 "#size-cells" 属性，
如果没有提供，那么客户端程序假设 "#address-cells" 属性值为 2，
"#size-cells" 属性值为 1.

---------------------------------

#### <span id="C206">reg</span>

"reg" 属性，属性值 <prop-encoded-array>, 其由
(address,length) 两个域构成。reg 属性用于描述节点在
父总线定义的空间内的地址。通常属性值代表 memory-mapped
IO 寄存器块的偏移和长度，但在某些总线类型上可能有不同的
含义。根节点定义的地址空间中的地址就是 CPU 真实地址。
reg 属性值的类型是 <prop-encode-array>, 其值由 address
和 length 成对表示，即 <address length>. address
和 length 所占用 u32 的个数由父节点的 "#address-cells"
和 "#size-cells" 指定。如果父节点的 "#size-cells" 的值
为 0，那么 reg 属性就忽略 length 域。例如：

{% highlight bash %}
        #address-cells = <1>;
        #size-cells = <1>;
        Fil: Fil@60000000 {
                compatible = "BS,FIL2356";
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

节点 Fil 包含了 reg 属性，由于 Fil 父节点的 #address-cells 值
为 1，那么 reg 的 address 域占用一个 u32。同理父节点的
#size-cells 值为 1，那么 reg 的 length 域占用一个 u32.

指的注意的是，节点 node-name@unit-address 的 "unit-address"
如果存在，那么其值一定要与节点 reg 属性的 address 域值一致。

---------------------------------

#### <span id="C207">virtual-reg</span>

"virtual-reg" 属性，属性值是 u32。"virtual-reg" 属性
提供了一个有效的虚拟地址，其映射到 reg 属性对应的第一个物理
地址上。此属性使引导程序能够为客户端程序提供已设置的虚拟到
物理映射。例如

{% highlight bash %}
        Fil: Fil@60000000 {
                compatible = "BS,FIL2356";
                reg = <0x60000000 0x40000000>;
                virtual-reg = <0x80000000>;
        };
{% endhighlight %}

---------------------------------

#### <span id="C208">ranges</span>

"ranges" 属性，属性值类型为 "<empty>" 或者 "<prop-encoded-array>",
当属性值类型为 "<prop-encoded-array>" 由
(child-bus-address, parent-bus-address, length) 三元组构成。
range 属性提供了一种定义子节点地址空间和父地址空间的映射和转换方法。
range 属性值的格式是任意数量的三元数组 (子节点总线地址，
父节点总线地址，长度)。

"child-bus-address" 代表子节点总线上的一个物理地址，其包含 u32
的数量由该节点的 "#address-cells" 属性决定。"parent-bus-address"
代表父总线上的一个物理地址，其包含 u32 的数量由父节点的 "#address-cells"
属性指定。"length" 域指定了子节点地址空间的长度，其包含 u32 的
数量有当前节点的 "#size-cells" 进行指定。

如果 ranges 属性的属性值类型是一个 "<empty>", 那么表示父节点的地址空间
和子节点的地址空间相同，不需要地址转换。如果节点中不存在 ranges 属性，那么
假设子节点和父节点之间不存在地址映射。例如：

{% highlight bash %}
        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0x0 0xe0000000 0x00100000>;

                serial@4600 {
                      device_type = "serial";
                      compatible = "nsl16550";
                      reg = <0x4600 0x100>;
                      clock-frequency = <0>;
                      interrupts = <0xA 0x8>;
                      interrupt-parent = <&ipic>;
                };
        };
{% endhighlight %}

在上面的例子中，soc 节点包含了 ranges 属性，其属性值为
"<0x0 0xe0000000 0x00100000>", 其表示 soc 的子节点的
物理地址 0 映射到 soc 地址空间的物理地址 0xe0000000.serial
节点为 soc 的子节点，其地址是父节点总线偏移 0x4600 处，因此
serial 在父节点总线上的地址就是 0x4600 + 0xe0000000 = 0xe0004600.
在如下面的例子：

{% highlight bash %}
        #address-cells = <1>;
        soc {
                compatible = "simple-bus";
                #address-cells = <2>;
                #size-cells = <2>;
                ranges = <0x0 0x0 0xe0000000 0x0 0x00100000>;

                serial@4600 {
                      device_type = "serial";
                      compatible = "nsl16550";
                      reg = <0x4600 0x100>;
                      clock-frequency = <0>;
                      interrupts = <0xA 0x8>;
                      interrupt-parent = <&ipic>;
                };
        };
{% endhighlight %}

soc 节点包含了 ranges 属性，且 soc 节点的 #address-cells 属性
为 2，那么 ranges 的 child-bus-address 占用两个 u32 整数，
其值为 0x00000000 00000000。由于 soc 节点的 #size-cells 属性值
为 2，那么 ranges 属性的 length 域占用两个 u32 整数。其值为
0x00000000 00100000。soc 节点的父节点的 #address-cells 属性值为 1，
那么 soc 节点的 rangs 属性 parent-bus-address 占用一个 u32，
其值为 0xe0000000。因此 ranges 属性表示 soc 子节点地址空间从物理
地址 0 映射到父节点地址空间物理地址 0xe0000000, 映射长度是 0x00100000.
那么子节点 serial 在子节点地址总线上的偏移是 0x4600, 那么其在父总线
上的地址就是: 0x4600 + 0xe0000000 = 0xe0004600.

---------------------------------

#### <span id="C209">dma-ranges</span>

"dma-range" 属性，属性值类型是 "<empty>" 或 "<prop-encoded-array>,
"<prop-encode-array>" 定义为 (child-bus-address, parent-bus-address,
length) 三元数组。"dma-ranges" 属性子节点 DMA 访问的物理地址域父节点
DMA 访问的物理地址之间的映射。dma-ranges 属性可以任意数量的三原数组
(child-bus-address, parent-bus-address, length) 构成。每个三元
数组描述一段连续的 DMA 地址空间范围。

"child-bus-address" 域是子节点地址空间上的一个物理地址，其占用
u32 整数的数量由该节点的 "#address-cells" 进行指定。
"parent-bus-address" 域是父节点总线上的一个物理地址，其占用
u32 的数量由父节点的 "#address-cells" 进行指定。"length"
域表示子节点地址空间的长度，其占用 u32 的数量由该节点的
"#size-cells" 进行指定。例如：

{% highlight bash %}
                #addrss-cells = <2>;
                #size-cells = <2>;
                pciec: pcie@fe000000 {
                        compatible = "renesas,pcie-r8a7743",
                                     "renesas,pcie-rcar-gen2";
                        reg = <0 0xfe000000 0 0x80000>;
                        #address-cells = <3>;
                        #size-cells = <2>;
                        bus-range = <0x00 0xff>;
                        device_type = "pci";
                        ranges = <0x01000000 0 0x00000000 0 0xfe100000 0 0x00100000
                                  0x02000000 0 0xfe200000 0 0xfe200000 0 0x00200000
                                  0x02000000 0 0x30000000 0 0x30000000 0 0x08000000
                                  0x42000000 0 0x38000000 0 0x38000000 0 0x08000000>;
                        /* Map all possible DDR as inbound ranges */
                        dma-ranges = <0x42000000 0 0x40000000 0 0x40000000 0 0x80000000
                                      0x43000000 2 0x00000000 2 0x00000000 1 0x00000000>;
{% endhighlight %}

pciec 节点中包含了 dma-ranges 属性，该节点的 "#address-cells" 为 3，
那么 dma-ranges 属性的 child-bus-address 使用三个 u32 表示，
因此 dma-ranges 第一个三元数组 child-bus-address 域值为：
0x42000000 00000000 40000000; 第二个三元数组 child-bus-address
域值为: 0x43000000 00000002 00000000。该节点的 "#size-cells" 为 2，
那么 dma-ranges 属性的 parent-bus-address 使用了两个 u32 表示，
因此 dma-ranges 第一个三元数组 length 域值为：0x0000000 80000000;
第二个三原数组 length 域值为: 0x00000001 00000000。pciec 节点
的父节点的 "#address-cells" 属性值为 2，那么 dma-ranges 的一个三元
数组的 parent-bus-address 占用 2 个 u32 整数，其值为：
0x00000000 40000000; 同理第二个三元数组的 parent-bus-address
值为 0x00000002 00000000.

---------------------------------

#### <span id="C210">device_type</span>

"device_type" 属性，属性值类型是 <string>.


{% highlight bash %}

{% endhighlight %}


---------------------------------

#### <span id="C00"></span>

{% highlight bash %}

{% endhighlight %}

---------------------------------

#### <span id="C00"></span>

{% highlight bash %}

{% endhighlight %}

---------------------------------

#### <span id="C00"></span>

{% highlight bash %}

{% endhighlight %}

---------------------------------

#### <span id="C00"></span>

{% highlight bash %}

{% endhighlight %}

---------------------------------

#### <span id="C00"></span>

{% highlight bash %}

{% endhighlight %}

---------------------------------

#### <span id="C00"></span>

{% highlight bash %}

{% endhighlight %}




-------------------------------------------------

## DTB 标准结构设计

DTB 是一个二进制文件，由 dtc 工具将 dts 文件转换而来，用于存储板子硬件
描述信息。板子上电之后，由 u-boot 传递给内核。

##### DTB 文件的作用

{% highlight bash %}
1. 使用减少了内核版本数。比如同一块板子，在外设不同的情况下，使用
   dtb 文件需要编译多个版本的内核。当使用 dtb 文件时同一份 linux
   内核代码可以在多个板子上运行，每个板子可以使用自己的 dtb 文件。

2. 嵌入式中 ，linux 内核启动过程中只要解析 dtb 文件，就能加载对应的模块。

3. 编译 linux 内核时必须选择对应的外设模块，并且 dtb 中包括外设的信息，
   在 linux 内核启动过程中才能自动加载该模块
{% endhighlight %}

### DTB 二进制文件架构

DTB 二进制文件采用一定的标准进行设计规划，开发者只要在程序
中根据 DTB 的标准，就能从 DTB 中获得所需的信息。DTB 的架构
如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000213.png)

DTB 分作如下几个部分：

> - [boot_param_header](#B010)
>
> - [memory reserve map](#B011)
>
> - [device-tree structure](#B012)
>
> - [device-tree strings](#B013)

-------------------------------------------------------

##### <span id="B010">boot_param_header</span>

boot_param_header 部分称为 DTB 的头部，该部分包含了 DTB
二进制的基本信息，其在内核中定义如下：

{% highlight bash %}
/*
 * This is what gets passed to the kernel by prom_init or kexec
 *
 * The dt struct contains the device tree structure, full pathes and
 * property contents. The dt strings contain a separate block with just
 * the strings for the property names, and is fully page aligned and
 * self contained in a page, so that it can be kept around by the kernel,
 * each property name appears only once in this page (cheap compression)
 *
 * the mem_rsvmap contains a map of reserved ranges of physical memory,
 * passing it here instead of in the device-tree itself greatly simplifies
 * the job of everybody. It's just a list of u64 pairs (base/size) that
 * ends when size is 0
 */
struct boot_param_header {
        __be32  magic;                  /* magic word OF_DT_HEADER */
        __be32  totalsize;              /* total size of DT block */
        __be32  off_dt_struct;          /* offset to structure */
        __be32  off_dt_strings;         /* offset to strings */
        __be32  off_mem_rsvmap;         /* offset to memory reserve map */
        __be32  version;                /* format version */
        __be32  last_comp_version;      /* last compatible version */
        /* version 2 fields below */
        __be32  boot_cpuid_phys;        /* Physical CPU id we're booting on */
        /* version 3 fields below */
        __be32  dt_strings_size;        /* size of the DT strings block */
        /* version 17 fields below */
        __be32  dt_struct_size;         /* size of the DT structure block */
};
{% endhighlight %}

开发者可以通过使用二进制工具直接分析一个 DTB 的头部
文件，结合该例子一同分析 DTB 头部的各个成员。例如在 Linux 5.0
上使用一个例子：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/
hexdump vexpress-v2p-ca15_a7.dtb -n 112
{% endhighlight %}

在 Linux 5.0 arm32 体系中，BiscuitOS 使用了 vexpress-v2p-ca15_a7.dtb
作为 DTB，此时使用上面的命令可以获得头文件的二进制内容，如下：

{% highlight bash %}
buddy@BDOS: $ hexdump vexpress-v2p-ca15_a7.dtb -n 112
0000000 0dd0 edfe 0000 4548 0000 3800 0000 6444
0000010 0000 2800 0000 1100 0000 1000 0000 0000
0000020 0000 e103 0000 2c44 0000 0000 0000 0000
0000030 0000 0000 0000 0000 0000 0100 0000 0000
0000040 0000 0300 0000 0d00 0000 0000 3256 2d50
0000050 4143 3531 435f 3741 0000 0000 0000 0300
0000060 0000 0400 0000 0600 0000 4902 0000 0300
{% endhighlight %}

DTB header 包含了 DTB 文件的所有信息，分别代表如下:

--------------------------------------

###### magic

DTB 文件的 MAGIC，用于表明文件属于 DTB，其为 DTB 第一个 4 字节，
由于其定义为 `__be32`, 因此数据采用的大端模式，因此此时 DTB 的
MAGIC 信息为：0xD00DFEED。在 arm/kernel/head-common.S 中有
如下定义：

{% highlight bash %}
#ifdef CONFIG_CPU_BIG_ENDIAN
#define OF_DT_MAGIC 0xd00dfeed
#else
#define OF_DT_MAGIC 0xedfe0dd0 /* 0xd00dfeed in big-endian */
#endif
{% endhighlight %}

因此在采用大端模式的 CPU 中，DTB MAGIC 的宏定义为 0xD00DFEED,
而在小端模式的 CPU 中，DTB MAGIC 的宏定义为 0xEDFE0DD0. 注意！
在 ARM 中 DTB 延续了 POWERPC 的设计，DTB 也采用了大端模式。

---------------------------------------

###### totalsize

标识 DTB 文件的大小, 其为 DTB 第二个 4 字节，由于采用大端模式，
因此 DTB 二进制文件的大小为： 0x00004845. 此时在 Linux 中查看
DTB 文件的大小如下：

{% highlight bash %}
buddy@BDOS:$ ll vexpress-v2p-ca15_a7.dtb
-rw-rw-r-- 1 buddy buddy 18501 7月   8 10:09 vexpress-v2p-ca15_a7.dtb
{% endhighlight %}

-------------------------------------------

###### off_dt_struct

标识 Device-tree structure 在文件中的位置. 其值为 DTB
文件的第三个 4 字节，由于采用大端模式，此时其值为：0x00000038.
解析 DTB 的程序中，只要找到该地址就可以获得 DTS 中节点的起始
地址。

------------------------------------------

###### off_dt_strings

标识 Device-tree strings 在文件中的位置，其值为 DTB
文件的第四个 4 字节，由于采用大端模式，此时其值为：0x00004464.
解析 DTB 的程序中，只要找到该地址就可以获得 DTS 中 string 字符串
存储的起始地址。

---------------------------------

###### off_mem_revmap

标识 reserve memory 在文件中的位置。DTB 二进制文件中也会存在
一段预留的内存区域，其值为 DTB文件的第五个 4 字节，由于采用大
端模式，此时其值为：0x00000028. 解析 DTB 的程序中，只要找到该
地址就可以获得 DTB 二进制文件中的保留内存区。

----------------------------------

###### version

标识 DTB 文件的版本信息。其值为 DTB文件的第六个 4 字节，由于采用大
端模式，此时其值为：0x00000011。

-------------------------------

###### last_comp_version

标识上一个兼容 DTB 版本信息

-------------------------------

###### boot_cpuid_phys

物理 CPU 号

------------------------------

###### dt_strings_size

标识 Device-tree strings 的大小，结合 off_dt_strings 可以
计算出 DTB 文件 device-tree string 的完整信息。

------------------------------

###### dt_struct_size

标识 Device-tree structure 的大小，结合 off_dt_struct 可以
计算出 DTB 文件 device-tree struct 的完整信息。


-------------------------------------------------------

##### <span id="B011">memory reserve map</span>

这个段保存的是一个保留内存映射列表，每个表由一对 64 位的
物理地址和大小组成。其在内核中的定义如下：

{% highlight bash %}
struct fdt_reserve_entry {
    uint64_t address;
    uint64_t size;
};
{% endhighlight %}

-------------------------------------------------------

##### <span id="B012">device-tree structure</span>

这个段保存全部节点的信息，即包括节点的属性又包括节点的子节点，关系如下图：

![](/assets/PDB/BiscuitOS/boot/BOOT000214.png)

###### 节点 Node

从上图可知，每个节点或子节点都是以 FDT_BEGIN_NODE 开始，到
FDT_END_NODE 结束。节点中可以嵌套节点，嵌套的节点称为父节点，
被嵌套的节点称为子节点。

{% highlight bash %}
FDT_BEGIN_NODE         .......... 节点 N
|
|---FDT_BEGIN_NODE     .......... 子节点 0
|   |
|   FDT_END_NODE
|
|---FDT_BEGIN_NODE     .......... 子节点 1
|   |
|   FDT_END_NODE
|
FDT_END_NODE
{% endhighlight %}

节点在内核中的定义为：

{% highlight bash %}
struct fdt_node_header {
    uint32_t tag;
    char name[0];
};
{% endhighlight %}

DTS 文件中一个节点例子如下：

{% highlight bash %}
        cpus {
                #address-cells = <1>;
                #size-cells = <0>;

                A9_0: cpu@0 {
                        device_type = "cpu";
                        compatible = "arm,cortex-a9";
                        reg = <0>;
                        next-level-cache = <&L2>;
                };

                A9_1: cpu@1 {
                        device_type = "cpu";
                        compatible = "arm,cortex-a9";
                        reg = <1>;
                        next-level-cache = <&L2>;
                };

                A9_2: cpu@2 {
                        device_type = "cpu";
                        compatible = "arm,cortex-a9";
                        reg = <2>;
                        next-level-cache = <&L2>;
                };

                A9_3: cpu@3 {
                        device_type = "cpu";
                        compatible = "arm,cortex-a9";
                        reg = <3>;
                        next-level-cache = <&L2>;
                };
        };
{% endhighlight %}

在 DTS 文件中，存在节点 cpus，其包含了 4 个子节点，分别是
A9_0, A9_1, A9_2, 和 A9_3.

###### 属性 property

从上图可知，属性位于节点内部，并且每个属性以 FDT_PROP 开始，到下一个
FDT_PROP 结束。属性内包含了属性字符串大小，属性字符串在 device-tree strings
中的偏移，以及属性数值等信息。

{% highlight bash %}
FDT_BEGIN_NODE
|
|----FDT_PROP
|    |--- Property string size
|    |--- Property string offset
|    |--- Property value
|    
|----FDT_PROP
|    |--- Property string size
|    |--- Property string offset
|    |--- Property value
|
FDT_END_NODE
{% endhighlight %}

属性在内核中的定义为：

{% highlight bash %}
struct fdt_property {
    uint32_t tag;
    uint32_t len;
    uint32_t nameoff;
    char data[0];
};
{% endhighlight %}

DTS 文件中一个属性例子：

{% highlight bash %}
        memory@60000000 {
                device_type = "memory";
                reg = <0x60000000 0x40000000>;
        };
{% endhighlight %}

DTS 文件中 ，节点 memory@60000000 包含了两个属性 device_type
和 reg。其中 device_type 属性值是一个字符串 "memory", 而属性
reg 的值是一个整型。

###### 宏定义

宏定义位于：/scripts/dtc/libfdt/fdt/h

{% highlight bash %}
#define FDT_MAGIC    0xd00dfeed    /* 4: version, 4: total size */
#define FDT_TAGSIZE    sizeof(uint32_t)

#define FDT_BEGIN_NODE    0x1        /* Start node: full name */
#define FDT_END_NODE    0x2        /* End node */
#define FDT_PROP    0x3        /* Property: name off,
                       size, content */
#define FDT_NOP        0x4        /* nop */
#define FDT_END        0x9

#define FDT_V1_SIZE    (7*sizeof(uint32_t))
#define FDT_V2_SIZE    (FDT_V1_SIZE + sizeof(uint32_t))
#define FDT_V3_SIZE    (FDT_V2_SIZE + sizeof(uint32_t))
#define FDT_V16_SIZE    FDT_V3_SIZE
#define FDT_V17_SIZE    (FDT_V16_SIZE + sizeof(uint32_t))
{% endhighlight %}

----------------------------------

#### <span id="B013">Device-tree strings</span>

由于某些属性 (比如 compatible) 在大多数节点下都会存在，
为了减少 dtb 文件大小，就需要把这些属性字符串指定一个存储位置即可，
这样每个节点的属性只需找到属性字符串的位置就可以得到那个属性字符串，
所以 dtb 把 device-tree strings 单独列出来存储。

![](/assets/PDB/BiscuitOS/boot/BOOT000213.png)

device-tree strings 在 dtb 中的位置由 dtb header 即
boot_param_header 结构的 off_dt_strings 进行指定。




















------------------------------------------------------

##### <span id="A010">讨论引入 FDT 到嵌入式 linux 平台</span>

"Recent" (2009) discussion of "Flattened Device Tree"
work on linux-embedded mailing list:

> - [http://www.mail-archive.com/linux-embedded@vger.kernel.org/msg01721.html](http://www.mail-archive.com/linux-embedded@vger.kernel.org/msg01721.html)

{% highlight bash %}
Re: Representing Embedded Architectures at the Kernel Summit

Grant Likely Tue, 02 Jun 2009 10:30:27 -0700

On Tue, Jun 2, 2009 at 9:22 AM, James Bottomley
<james.bottom...@hansenpartnership.com> wrote:
> Hi All,
>
> We've got to the point where there are simply too many embedded
> architectures to invite all the arch maintainers to the kernel summit.
> So, this year, we thought we'd do embedded via topic driven invitations
> instead.  So what we're looking for is a proposal to discuss the issues
> most affecting embedded architectures, or preview any features affecting
> the main kernel which embedded architectures might need ... or any other
> topics from embedded architectures which might need discussion or
> debate.  If you'd like to do this, could you either reply to this email,
> or send proposals to:
>
> ksummit-2009-disc...@lists.linux-foundation.org


Hi James,

One topic that seems to garner debate is the issue of decoupling the
kernel image from the target platform.  ie. On x86, PowerPC and Sparc
a kernel image will boot on any machine (assuming the needed drivers
are enabled), but this is rarely the case in embedded.  Most embedded
kernels require explicit board support selected at compile time with
no way to produce a generic kernel which will boot on a whole family
of devices, let alone the whole architecture.  Part of this is a
firmware issue, where existing firmware passes very little in the way
of hardware description to the kernel, but part is also not making
available any form of common language for describing the machine.

Embedded PowerPC and Microblaze has tackled this problem with the
"Flattened Device Tree" data format which is derived from the
OpenFirmware specifications, and there is some interest and debate (as
discussed recently on the ARM mailing list) about making flattened
device tree usable by ARM also (which I'm currently
proof-of-concepting).  Josh Boyer has already touched on discussing
flattened device tree support at kernel summit in an email to the
ksummit list last week (quoted below), and I'm wondering if a broader
discussing would be warranted.

I think that in the absence of any established standard like the PC
BIOS/EFI or a real Open Firmware interface, then the kernel should at
least offer a recommended interface so that multiplatform kernels are
possible without explicitly having the machine layout described to it
at compile time.  I know that some of the embedded distros are
interested in such a thing since it gets them away from shipping
separate images for each supported board.  ie. It's really hard to do
a generic live-cd without some form of multiplatform.  FDT is a great
approach, but it probably isn't the only option.  It would be worth
debating.

Cheers,
g.

On Thu, May 28, 2009 at 5:41 PM, Josh Boyer <jwbo...@linux.vnet.ibm.com> wrote:
> Not to hijack this entirely, but I'm wondering if we could tie in some of the
> cross-arch device tree discussions that have been taking place among the ppc,
> sparc, and arm developers.
>
> I know the existing OF code for ppc, sparc, and microblaze could probably use
> some consolidation and getting some of the arch maintainers together in a room
> might prove fruitful.  Particularly if we are going to discuss how to make
> drivers work for device tree and standard platforms alike.
>
> josh
>



--
Grant Likely, B.Sc., P.Eng.
Secret Lab Technologies Ltd.
--
To unsubscribe from this list: send the line "unsubscribe linux-embedded" in
the body of a message to majord...@vger.kernel.org
More majordomo info at  http://vger.kernel.org/majordomo-info.html
{% endhighlight %}

----------------------------------------------------

##### <span id="A011">Russell King 反对 DTS 加入到 ARM</span>

Russell King is against adding support for FDT to the ARM
platform (see whole thread for interesting discussion):

> - [http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/01942.html](http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/01942.html)

{% highlight bash %}
Re: [RFC] [PATCH] Device Tree on ARM platform
From: Russell King
Date: Wed May 27 2009 - 13:48:45 EST

    Next message: Oren Laadan: "[RFC v16][PATCH 40/43] c/r: support semaphore sysv-ipc"
    Previous message: Oren Laadan: "[RFC v16][PATCH 36/43] c/r: support share-memory sysv-ipc"
    In reply to: Mark Brown: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Next in thread: Grant Likely: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Messages sorted by: [ date ] [ thread ] [ subject ] [ author ]

(For whatever reason, I don't have the initial email on this.)

On Wed, May 27, 2009 at 08:27:10AM -0600, Grant Likely wrote:
> On Wed, May 27, 2009 at 1:08 AM, Janboe Ye <yuan-bo.ye@xxxxxxxxxxxx> wrote:
> > Hi, All
> >
> > Currently, ARM linux uses mach-type to figure out platform. But mach-type could not handle variants well and it doesn't tell the kernel about info about attached peripherals.
> >
> > The device-tree used by powerpc and sparc could simplifiy board ports, less platform specific code and simplify device driver code.
> >
> > Please reference to Grant Likely and Josh Boyer's paper, A Symphony of Flavours: Using the device tree to describe embedded hardware , for the detail of device tree.
> >
> > www.kernel.org/doc/ols/2008/ols2008v2-pages-27-38.pdf
> >
> > Signed-off-by: janboe <yuan-bo.ye@xxxxxxxxxxxx>
>
> Heeheehe, This is Fantastic. I'm actually working on this too. Would
> you like to join our efforts?

My position is that I don't like this approach. We have _enough_ of a
problem getting boot loaders to do stuff they should be doing on ARM
platforms, that handing them the ability to define a whole device tree
is just insanely stupid.

For example, it's taken _years_ to get boot loader folk to pass one
correct value to the kernel. It's taken years for boot loaders to
start passing ATAGs to the kernel to describe memory layouts. And even
then there's various buggy platforms which don't do this correctly.

I don't see device trees as being any different - in fact, I see it as
yet another possibility for a crappy interface that lots of people will
get wrong, and then we'll have to carry stupid idiotic fixups in the
kernel for lots of platforms.

The end story is that as far as machine developers are concerned, a
boot loader, once programmed into the device, is immutable. They never
_ever_ want to change it, period.

So no, I see this as a recipe for ugly hacks appearing in the kernel
working around boot loader crappyness, and therefore I'm against it.

--
Russell King
Linux kernel 2.6 ARM Linux - http://www.arm.linux.org.uk/
maintainer of:
--
To unsubscribe from this list: send the line "unsubscribe linux-kernel" in
the body of a message to majordomo@xxxxxxxxxxxxxxx
More majordomo info at http://vger.kernel.org/majordomo-info.html
Please read the FAQ at http://www.tux.org/lkml/
{% endhighlight %}

------------------------------------------------

##### <span id="A013">Russell 最终被说服接受 DTS 进入 ARM</span>

But maybe Russell can be convinced:

> - [http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/03618.html](http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/03618.html)

{% highlight bash %}
Re: [RFC] [PATCH] Device Tree on ARM platform
From: Russell King - ARM Linux
Date: Sun May 31 2009 - 06:54:00 EST

    Next message: Alon Ziv: "Preferred kernel API/ABI for accelerator hardware device driver"
    Previous message: Pekka J Enberg: "[PATCH] slab: document kzfree() zeroing behavior"
    In reply to: Benjamin Herrenschmidt: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Next in thread: Grant Likely: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Messages sorted by: [ date ] [ thread ] [ subject ] [ author ]

On Fri, May 29, 2009 at 10:51:14AM +1000, Benjamin Herrenschmidt wrote:
> On Thu, 2009-05-28 at 17:04 +0200, Sascha Hauer wrote:
> >
> > We normally hide these subtle details behind a baseboard= kernel
> > parameter. I agree with you that it's far better to have a
> > standardized
> > way to specify this. For my taste the oftree is too bloated for this
> > purpose.
>
> Define "bloated" ?
>
> We got a lot of push back on powerpc initially with this exact same
> argument "too bloated" but that was never backed up with facts and
> numbers, and I would mostly say that nobody makes it anymore now
> that it's there and people use it.

It really depends how you look at it.

If you look at the amount of supporting code required for one platform
on ARM, it's typically fairly small - mostly just declaration of data
structures (platform devices, platform device data), and possibly a few
small functions to handle the quirkyness of the platform. That's of the
order of a few K of data and code combined - that's on average about 4K.
Add in the SoC platform device declaration code, and maybe add another
10K.

So, about 14K of code and data.

The OF support code in drivers/of is about 3K. The code posted at the
start of this thread I suspect will be about 4K or so, which gives us a
running total of maybe 7K.

What's now missing is conversion of drivers and the like to be DT
compatible, or creation of the platform devices to translate DT into
platform devices and their associated platform device data. Plus
some way to handle the platforms quirks, which as discussed would be
hard to represent in DT.

So I suspect it's actually marginal whether DT turns out to be larger
than our current approach. So I agree with BenH - I don't think there's
an argument to be made about 'bloat' here.

What /does/ concern me is what I percieve as the need to separate the
platform quirks from the description of the platform - so rather than
having a single file describing the entire platform (eg,
arch/arm/mach-pxa/lubbock.c) we would need:

- a file to create the DT information
- a separate .c file in the kernel containing code to handle the
platforms quirks, or a bunch of new DT drivers to do the same
- ensure both DT and quirks are properly in-sync.

But... we need to see how Grant gets on with his PXA trial before we
can properly assess this.
--
To unsubscribe from this list: send the line "unsubscribe linux-kernel" in
the body of a message to majordomo@xxxxxxxxxxxxxxx
More majordomo info at http://vger.kernel.org/majordomo-info.html
Please read the FAQ at http://www.tux.org/lkml/
{% endhighlight %}

----------------------------------------------------------

##### <span id="A014">David Gibson 辩护 FDT</span>

David Gibson definds FDT：

> - [http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/02304.html](http://lkml.indiana.edu/hypermail/linux/kernel/0905.3/02304.html)

{% highlight bash %}
Re: [RFC] [PATCH] Device Tree on ARM platform
From: David Gibson
Date: Wed May 27 2009 - 23:45:33 EST

    Next message: Rusty Russell: "Re: [my_cpu_ptr 1/5] Introduce my_cpu_ptr()"
    Previous message: David Gibson: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    In reply to: Grant Likely: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Next in thread: Pavel Machek: "Re: [RFC] [PATCH] Device Tree on ARM platform"
    Messages sorted by: [ date ] [ thread ] [ subject ] [ author ]

On Wed, May 27, 2009 at 11:52:50AM -0600, Grant Likely wrote:
> On Wed, May 27, 2009 at 11:44 AM, Russell King
> <rmk+lkml@xxxxxxxxxxxxxxxx> wrote:
> > (For whatever reason, I don't have the initial email on this.)
> >
> > On Wed, May 27, 2009 at 08:27:10AM -0600, Grant Likely wrote:
> >> On Wed, May 27, 2009 at 1:08 AM, Janboe Ye <yuan-bo.ye@xxxxxxxxxxxx> wrote:
> >> > Hi, All
> >> >
> >> > Currently, ARM linux uses mach-type to figure out platform. But mach-type could not handle variants well and it doesn't tell the kernel about info about attached peripherals.
> >> >
> >> > The device-tree used by powerpc and sparc could simplifiy board ports, less platform specific code and simplify device driver code.
> >> >
> >> > Please reference to Grant Likely and Josh Boyer's paper, A Symphony of Flavours: Using the device tree to describe embedded hardware , for the detail of device tree.
> >> >
> >> > www.kernel.org/doc/ols/2008/ols2008v2-pages-27-38.pdf
> >> >
> >> > Signed-off-by: janboe <yuan-bo.ye@xxxxxxxxxxxx>
> >>
> >> Heeheehe, This is Fantastic.  I'm actually working on this too.  Would
> >> you like to join our efforts?
> >
> > My position is that I don't like this approach.  We have _enough_ of a
> > problem getting boot loaders to do stuff they should be doing on ARM
> > platforms, that handing them the ability to define a whole device tree
> > is just insanely stupid.
>
> The point of this approach is that the device tree is *not* create by
> firmware. Firmware can pass it in if it is convenient to do so, (ie;
> the device tree blob stored in flash as a separate image) but it
> doesn't have to be and it is not 'owned' by firmware.
>
> It is also true that there is the option for firmware to manipulate
> the .dts, but once again it is not required and it does not replace
> the existing ATAGs.
>
> If a board port does get the device tree wrong; no big deal, we just
> fix it and ship it with the next kernel.

Indeed one of the explicit goals we had in mind in building the
flattened device tree system is that the kernel proper can rely on
having a usable device tree, without requiring that the bootloader /
firmware get all that right.

Firmware can supply a device tre, and if that's sufficiently good to
be usable, that's fine. But alternatively our bootwrapper can use
whatever scraps of information the bootloader does provide to either
pick the right device tree for the platform, tweak it as necessary for
information the bootloader does supply correctly (memory and/or flash
sizes are common examples), or even build a full device tree from
information the firmware supplies in some other form (rare, but
occasionally usefule, e.g. PReP).

We explicitly had the ARM machine number approach in mind as one of
many cases that the devtree mechanism can degenerate to: the
bootwrapper just picks the right canned device tree based on the
machine number. If the bootloader gets the machine number wrong, but
supplies a few other hints that let us work out what the right machine
is, we have logic to pick the device tree based on that. Yes, still a
hack, but at least it's well isolated.

If the firmware does provide a device tree, but it's crap, code to
patch it up to something usable (which could be anything from applying
a couple of tweaks, up to replacing it wholesale with a canned tree
based on one or two properties in the original which let you identify
the machine) is again well isolated.

> > The end story is that as far as machine developers are concerned, a
> > boot loader, once programmed into the device, is immutable.  They never
> > _ever_ want to change it, period.

You're over focusing - as too many people do - on the firmware/kernel
communication aspects of the devtree. Yes, the devtree does open some
interesting possibilities in that area, but as you say, firmware can
never be trusted so the devtree doesn't really bring anything new
(better or worse) here.

What it does bring is a *far* more useful and expressive way of
representing consolidated device information in the kernel than simple
tables. And with the device tree compiler, it also becomes much more
convenient to prepare this information than building C tables. I
encourage you to have a look at the sample device trees in
arch/powerpc/boot/dts and at the device tree compiler code (either in
scripts/dtc where it's just been moved, or from the upstream source at
git://git.jdl.com/software/dtc.git).

I have to agree with DaveM - a lot of people's objections to the
devtree stem from not actually understanding what it does and how it
works.

--
David Gibson | I'll have my music baroque, and my code
david AT gibson.dropbear.id.au | minimalist, thank you. NOT _the_ _other_
| _way_ _around_!
http://www.ozlabs.org/~dgibson
--
To unsubscribe from this list: send the line "unsubscribe linux-kernel" in
the body of a message to majordomo@xxxxxxxxxxxxxxx
More majordomo info at http://vger.kernel.org/majordomo-info.html
Please read the FAQ at http://www.tux.org/lkml/
{% endhighlight %}

--------------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [搭建高效的 Linux 开发环境](/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)
