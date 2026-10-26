# U-Boot 编译、烧写与启动笔记


## 1. U-Boot 是什么

Bootloader，本质是**引导程序**，不是 Linux。

```text
上电/复位 → Boot ROM → U-Boot → Linux Kernel → rootfs → 应用程序
```

核心作用：初始化硬件、加载内核与设备树、把 Linux 启动起来。

核心任务：
- 初始化硬件：DDR、串口、网卡、存储
- 加载内核（Kernel）与设备树（DTB）
- 启动内核：`bootz` / `bootm`

## 2. 两种启动方式

| 阶段 | 流程 | 特点 |
|---|---|---|
| 正常启动（产品） | Boot ROM → eMMC 读 U-Boot → eMMC 读 Kernel+DTB → 挂载 eMMC 中 rootfs | 全部在 eMMC，可独立启动 |
| 开发阶段 | Boot ROM → SD 卡读 U-Boot → **TFTP** 下载 Kernel+DTB → **NFS** 挂载 rootfs | 文件分离，便于频繁修改调试 |

## 3. 移植前准备

- 软件：Ubuntu 虚拟机、FileZilla（传文件）、MobaXterm（串口）、交叉编译工具链、U-Boot 源码
- 硬件：开发板、TF 卡 + 读卡器、USB 转串口线

## 4. 操作全流程

```text
解压源码 → 编译 → 插入 SD 卡 → 烧写 → 拨码设置 → 串口启动 → 进入命令行
```

### 4.1 解压源码

```bash
tar -xjf u-boot-imx-2016.03-2.1.0-g0ae7e33-v1.7.tar.bz2
```

### 4.2 编译

```bash
gedit mx6ull_emmc_uboot.sh      # 确认工具链/板级配置
chmod a+x mx6ull_emmc_uboot.sh
./mx6ull_emmc_uboot.sh
```

编译成功信息：

```text
Image Type:   Freescale IMX Boot Image
Image Ver:    2 (i.MX53/6/7 compatible)
Mode:         DCD
Load Address: 877ff420
Entry Point:  87800000
```

产物：`u-boot.imx`（可启动，带 IVT/DCD）、`u-boot.bin`（裸二进制）、`u-boot`（ELF）。

### 4.3 识别 SD 卡

插入读卡器，在 VMware/VirtualBox 弹窗中选择**连接到虚拟机**，然后：

```bash
ls /dev/sd*        # 例如新增 /dev/sdb
```

### 4.4 烧写到 SD 卡

```bash
./imxdownload u-boot.bin /dev/sdb
```

看到 `361804 bytes ... copied` 等输出即成功（会生成 `load.imx` 并写入）。

### 4.5 启动开发板

1. 将 SD 卡插到开发板
2. BOOT 拨码：**第 1 个与第 7 个拨到上面**（从 SD 卡启动）
3. USB_TTL 连接电脑（开发板串口 1）
4. 打开 MobaXterm，串口参数 **115200 8N1**
5. 复位开发板

出现 `Hit any key to stop autoboot:` 时：
- 按回车 → 进入 U-Boot 命令行（`=>`）
- 不按 → 3 秒后按默认参数启动 Linux 内核

## 5. 启动日志解读

```text
U-Boot 2016.03 (Aug 18 2026 - 03:55:59 -0700)
CPU:   Freescale i.MX6ULL rev1.1 792 MHz (running at 396 MHz)
CPU:   Industrial temperature grade (-40C to 105C) at 35C
Reset cause: POR
Board: I.MX6U ALPHA|MINI
I2C:   ready
DRAM:  512 MiB
MMC:   FSL_SDHC: 0, FSL_SDHC: 1
In:    serial
Out:   serial
Err:   serial
switch to partitions #0, OK
mmc0 is current device
Net:   FEC1
Normal Boot
Hit any key to stop autoboot: 0
```

| 信息 | 含义 |
|---|---|
| U-Boot 2016.03 | 版本与编译时间 |
| CPU 792MHz / 396MHz | 芯片型号与实际运行频率 |
| Industrial grade / 35C | 工业级温度范围与当前温度 |
| Reset cause: POR | 上电复位 |
| Board: I.MX6U ALPHA\|MINI | 板级配置 |
| I2C: ready | I2C 初始化完成 |
| DRAM: 512 MiB | DDR 初始化成功，容量 512MB |
| MMC: FSL_SDHC: 0, 1 | 检测到 SD/eMMC 两个控制器 |
| mmc0 is current device | 当前使用 mmc0 |
| Net: FEC1 | 网卡初始化成功 |
| Normal Boot | 正常启动流程 |

## 6. i.MX6ULL 启动流程

```text
上电/复位 (POR)
  → Boot ROM：读取 u-boot.imx，加载 DCD
  → DCD（DDR 初始化，imximage-ddr512.cfg）
  → 将 U-Boot 加载到 DDR
  → 跳转执行 U-Boot（Entry Point = 0x87800000）
  → U-Boot 初始化硬件（DDR/MMC/串口/网卡）
  → 自动启动倒计时 3 秒
       ├─ 按键 → 进入 U-Boot 命令行（=>）
       └─ 不按键 → 执行 bootcmd 启动内核
```

> 进入命令行后可用的常用命令：`printenv`、`mmc list`、`ping`、`tftp` 等。
