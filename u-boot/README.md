# U-Boot 实操手册（精简版）

> 平台：正点原子 i.MX6ULL（I.MX6U-ALPHA/MINI）

## 0. 全流程

```text
解压源码 → 交叉编译 → 识别 SD 卡 → 烧写 → 设置拨码 → 串口启动 → 进入命令行
```

## 1. 准备

| 项目 | 内容 |
|---|---|
| 编译环境 | Ubuntu 虚拟机 |
| 交叉工具链 | `arm-linux-gnueabihf-` |
| 源码 | 正点原子 U-Boot 2016.03 BSP |
| 烧写工具 | `imxdownload` |
| 串口终端 | MobaXterm / picocom |
| 硬件 | 开发板、SD 卡、读卡器、USB 转 TTL 线 |

串口参数：**115200，8N1，无流控**，连接开发板 UART1。

## 2. 编译

确认工具链：

```bash
arm-linux-gnueabihf-gcc -v
```

如未安装：

```bash
sudo apt install gcc-arm-linux-gnueabihf
```

解压并进入源码目录：

```bash
tar -xjf u-boot-imx-*.tar.bz2
cd u-boot-imx-*/
```

编译（等价于课件脚本 `mx6ull_emmc_uboot.sh`）：

```bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

make distclean
make mx6ull_14x14_evk_emmc_defconfig
make -j$(nproc)
```

> `defconfig` 名称以实际源码为准：`ls configs | grep mx6ull`。
> `CROSS_COMPILE` 末尾的 `-` 不能少。

编译成功会看到：

```text
Image Type:   Freescale IMX Boot Image
Mode:         DCD
Load Address: 877ff420
Entry Point:  87800000
```

产物：

| 文件 | 说明 |
|---|---|
| `u-boot.bin` | 裸二进制，Boot ROM 不识别 |
| `u-boot.imx` | 带 IVT/DCD 头，可启动 |
| `u-boot` | ELF 文件，调试用 |

## 3. 识别 SD 卡

插入读卡器和 SD 卡，连接到虚拟机后：

```bash
 ls /dev/sd*
```

输出中新增的设备（如 `/dev/sdb`）就是 SD 卡。

> 注意：
> - 不确定时，拔插一次 SD 卡再对比 `lsblk`。

## 4. 烧写

### 方法一：imxdownload（课件方法）

```bash
chmod +x imxdownload
sudo ./imxdownload u-boot.bin /dev/sdb
```

工具会自动加 i.MX 头生成 `load.imx`，再写到 0x400 偏移处。


## 5. 启动开发板

1. SD 卡插入开发板；
2. BOOT CFG：**第 1 个和第 7 个拨到 ON**（从 SD 卡启动）；
3. USB 转 TTL 连接 UART1，注意 TX/RX 交叉、共地；
4. 打开串口终端（115200 8N1）；
5. 复位开发板。

看到倒计时时按回车：

```text
Hit any key to stop autoboot: 3
```

成功进入命令行：

```text
=>
```

不按键则倒计时结束后执行 `bootcmd` 启动内核。

## 6. 看懂启动日志

```text
U-Boot 2016.03 (Aug 18 2026 - 03:55:59 -0700)
CPU:   Freescale i.MX6ULL rev1.1 792 MHz (running at 396 MHz)
Reset cause: POR
Board: I.MX6U ALPHA|MINI
DRAM:  512 MiB
MMC:   FSL_SDHC: 0, FSL_SDHC: 1
Net:   FEC1
Hit any key to stop autoboot: 0
=>
```

| 日志 | 含义 |
|---|---|
| `CPU: ... 792 MHz` | CPU 型号和主频 |
| `Reset cause: POR` | 上电复位 |
| `DRAM: 512 MiB` | DDR 初始化成功 |
| `MMC: FSL_SDHC: 0, 1` | 检测到两个 MMC 控制器 |
| `mmc0 is current device` | 当前设备是 SD 卡 |
| `Net: FEC1` | 网卡初始化成功 |

## 7. 常用命令

| 命令 | 作用 |
|---|---|
| `help` | 查看帮助 |
| `bdinfo` | 查看内存、重定位地址等 |
| `printenv` | 打印环境变量 |
| `setenv 名 值` | 设置环境变量 |
| `saveenv` | 保存环境变量 |
| `mmc list` | 列出 MMC 设备 |
| `mmc dev 0` | 切换 MMC 设备 |
| `mmc info` | 查看容量等信息 |
| `fatls mmc 0:1` | 列出 FAT 分区文件 |
| `ping 192.168.1.10` | 测试网络 |
| `tftp 地址 文件名` | 下载文件 |
| `bootz 内核地址 - dtb地址` | 启动 zImage |
| `boot` | 执行 `bootcmd` |
| `reset` | 复位 |

修改倒计时：

```text
=> setenv bootdelay 5
=> saveenv
```

改错恢复默认：

```text
=> env default -a
=> saveenv
=> reset
```

## 8. 网络调试（TFTP + NFS）

开发阶段常用：U-Boot 从 SD 卡启动，内核用 TFTP 下载，rootfs 用 NFS 挂载。

开发板设置：

```text
=> setenv ipaddr 192.168.1.20
=> setenv serverip 192.168.1.10
=> ping 192.168.1.10
```

下载并启动：

```text
=> tftp 0x80800000 zImage
=> tftp 0x83000000 imx6ull-14x14-evk.dtb
=> bootz 0x80800000 - 0x83000000
```

NFS 启动参数示例：

```text
=> setenv bootargs 'console=ttymxc0,115200 root=/dev/nfs rw nfsroot=192.168.1.10:/home/zr-arm/linux/nfs/rootfs,proto=tcp ip=192.168.1.20:192.168.1.10:192.168.1.1:255.255.255.0::eth0:off'
```

主机端需要提前启动 TFTP 和 NFS 服务，并把 `zImage`、DTB 放进 TFTP 目录。
