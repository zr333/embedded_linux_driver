# U-Boot 理论与 i.MX6ULL 启动机制（笔记）



## 1. U-Boot 是什么

**U-Boot 是 Linux 启动前运行的引导程序（Bootloader）。**

它不负责运行应用程序，只做三件事：

1. 初始化硬件（DDR、串口、MMC、网卡等）；
2. 从存储或网络加载内核与设备树；
3. 把启动参数交给内核，然后跳转过去。

完整启动链：

```text
上电/复位 → Boot ROM（芯片内部） → U-Boot → Linux Kernel → rootfs → 应用程序
```

| 阶段 | 运行位置 | 主要工作 |
|---|---|---|
| Boot ROM | 芯片内部 | 选择启动介质，初始化 DDR，加载 U-Boot |
| U-Boot | DDR | 初始化外设，提供命令行，加载内核 |
| Kernel | DDR | 初始化系统，挂载根文件系统 |
| rootfs | eMMC/SD/NFS | 提供系统文件和应用程序 |

## 2. i.MX6ULL 的启动过程

```text
上电
  → Boot ROM 根据 BOOT 拨码/eFUSE 选择启动介质
  → 从 SD/eMMC 的 0x400 偏移处读取 IVT
  → 执行 DCD，初始化 DDR 和时钟
  → 把 U-Boot 搬到 DDR
  → 跳转到入口地址 0x87800000 执行 U-Boot
```

需要记住的三个关键词：

| 关键词 | 含义 |
|---|---|
| IVT | 镜像入口表，告诉 Boot ROM 入口地址、DCD 在哪里 |
| DCD | 一段寄存器配置，主要用来初始化 DDR |
| `0x400` | i.MX 启动镜像的固定偏移（1 KB），所以烧写要用 `seek=1` |

对应两个文件：

- `u-boot.bin`：裸二进制，**Boot ROM 不认识**；
- `u-boot.imx`：加了 IVT/DCD 头，**Boot ROM 能启动**。

## 3. U-Boot 自己的启动流程

```text
_start（汇编入口）
  → board_init_f    早期初始化，计算内存和重定位地址
  → relocation      把 U-Boot 搬到 RAM 更高地址
  → board_init_r    初始化驱动、环境变量、命令
  → main_loop       倒计时，进入命令行或执行 bootcmd
```

`main_loop` 的行为就是启动日志里的：

```text
Hit any key to stop autoboot: 3
```

- 3 秒内按回车：进入命令行 `=>`；
- 不按键：执行 `bootcmd`，启动内核。

## 4. 源码目录（先记住 6 个）

| 目录 | 作用 |
|---|---|
| `arch/` | 架构相关代码，如 ARM 启动汇编 |
| `board/` | 板级初始化代码 |
| `configs/` | 各板子的 `*_defconfig` |
| `drivers/` | 外设驱动 |
| `include/` | 头文件和板级配置 |
| `cmd/` | U-Boot 命令实现 |

编译流程：

```text
configs/<board>_defconfig  →  .config  →  make  →  u-boot.bin / u-boot.imx
```

## 5. 环境变量与启动方式

环境变量决定“从哪里启动、怎么启动”，常用：

| 变量 | 作用 |
|---|---|
| `bootcmd` | 自动启动时执行的命令 |
| `bootargs` | 传给内核的启动参数 |
| `bootdelay` | 倒计时秒数 |
| `ipaddr` / `serverip` | 开发板 IP / 服务器 IP |

`saveenv` 保存后掉电不丢失；改错可用 `env default -a` 恢复默认。

两种典型启动方式：

| 场景 | 内核 | rootfs | 特点 |
|---|---|---|---|
| 产品阶段 | eMMC | eMMC | 全部本地化，适合量产 |
| 开发阶段 | TFTP 下载 | NFS 挂载 | 改内核/文件系统不用反复烧写 |

启动命令：

```text
bootz 0x80800000 - 0x83000000
```

三个参数：内核地址、initrd 地址（没有用 `-`）、设备树地址。

## 6. 镜像与移植

常见产物：

| 文件 | 说明 |
|---|---|
| `u-boot.bin` | 裸二进制，不能直接启动 |
| `u-boot.imx` | 带 i.MX 头的启动镜像 |
| `u-boot` | ELF 文件，调试用 |

移植的基本思路：

```text
选一块最接近的参考板
  → 复制 board/、dts、defconfig
  → 修改 DDR 参数、引脚、时钟、存储、网卡
  → 烧写，看串口输出，逐项验证
```

其中 DDR 参数最敏感，改错通常表现为“串口完全无输出”，不要随意改动已验证的配置。


## 8. 术语速查

| 术语 | 含义 |
|---|---|
| Boot ROM | 芯片内部固化的启动代码 |
| Bootloader | 引导操作系统的程序 |
| DTB | 设备树二进制，描述硬件 |
| TFTP | 通过网络下载文件的协议 |
| NFS | 网络文件系统，开发时常用来挂 rootfs |
| SPL | 二级加载器，先初始化 DDR 再加载 U-Boot |

