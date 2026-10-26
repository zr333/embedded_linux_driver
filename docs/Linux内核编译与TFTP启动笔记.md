# Linux 内核编译与 TFTP 启动笔记

## 1. 内核源码重点目录

| 目录/文件 | 作用 |
|---|---|
| `arch/arm/` | ARM 架构与平台相关代码、启动与配置 |
| `arch/arm/boot/dts/` | 设备树源码 `.dts` / `.dtsi` → 编译为 `.dtb` |
| `drivers/` | 各类硬件驱动（GPIO/I2C/SPI/USB/网卡等） |
| `.config` | 内核裁剪与功能选择，决定哪些代码参与编译 |

编译后最关心的产物：**`zImage`（内核镜像）** 与 **`imx6ull-xxx.dtb`（设备树）**。

## 2. 开发阶段启动方式

```text
Boot ROM → SD 卡读取 U-Boot
          → U-Boot 通过 TFTP 下载 Kernel + DTB
          → Kernel 通过 NFS 挂载 rootfs
```

特点：U-Boot、Kernel、rootfs 分离，方便反复修改调试。

## 3. 编译前置：安装 lzop

生成 zImage 需 LZO 压缩，未安装会导致 Image 生成成功但 zImage 压缩失败。

```bash
sudo apt update
sudo apt install lzop
```

## 4. 编译内核

```bash
tar -xjf linux-imx-rel_imx_4.1.15_2.1.1_ga_alientek_v2.2.tar.bz2
cd linux-imx-rel_imx_4.1.15_2.1.1_ga_alientek_v2.2
./mx6ull_alientek_emmc.sh
```

编译结束提示：

```text
Kernel: arch/arm/boot/zImage is ready
```

产物：

```text
arch/arm/boot/zImage                          # 内核镜像
arch/arm/boot/dts/imx6ull-alientek-emmc.dtb   # 设备树
```

## 5. 发送到 TFTP 目录

```bash
cd arch/arm/boot/dts
cp imx6ull-alientek-emmc.dtb(这里用imx6ull-14x14-emmc-7-1024x600-c.dtb，原厂内核源码编译出的镜像) ~/linux/tftp/
cd ..
cp zImage ~/linux/tftp/
```

## 6. U-Boot 网络设置

```text
setenv ipaddr    192.168.6.101
setenv ethaddr   b8:ae:1d:01:00:00
setenv gatewayip 192.168.5.1
setenv netmask   255.255.255.0
setenv serverip  192.168.5.100
saveenv
```

验证：

```text
192.168.5.100
# host 1192.168.5.100 is alive
```

## 7. 设置 bootargs

从 eMMC 第 2 分区启动：

```text
setenv bootargs 'console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw'
```


| 参数 | 含义 |
|---|---|
| `console=ttymxc0,115200` | 串口控制台，波特率 115200 |
| `root=/dev/mmcblk1p2` | 根文件系统位置：eMMC/SD 第 2 分区 |
| `rootwait` | 等待存储设备就绪 |
| `rw` | 以读写方式挂载根文件系统 |

本质：`bootargs` 是 U-Boot 传给 Linux 内核的启动参数。

## 8. 三条启动命令

```text
tftp 80800000 zImage                       # 下载内核到 0x80800000
tftp 83000000 imx6ull-alientek-emmc.dtb    # 下载设备树到 0x83000000
bootz 80800000 - 83000000                  # 启动内核
```

`bootz` 参数：

1. `0x80800000`：zImage 地址
2. `-`：不使用 initrd
3. `0x83000000`：DTB 地址

## 9. 加载地址说明

- DDR 起始地址：`0x80000000`
- `0x80800000`：zImage 加载地址
- `0x83000000`：DTB 加载地址
- 选择原则：在有效 DDR 范围内、避开 U-Boot 自身占用、互不覆盖、给内核留足空间

> 这两个地址是**人为选择的 DDR 加载地址**，不是文件固定规定的。

## 10. 启动成功日志

```text
Kernel image @ 0x80800000 [ 0x000000 - 0x5949d0 ]
## Flattened Device Tree blob at 83000000
   Booting using the fdt blob at 0x83000000
Starting kernel ...
Booting Linux on physical CPU 0x0
Linux version 4.1.15 ...
```

## 11. 全流程速查

```text
装 lzop → 解压内核 → ./mx6ull_alientek_emmc.sh → 拷贝 zImage/dtb 到 tftp
→ U-Boot 设置网络与 bootargs → tftp 下载内核 → tftp 下载 dtb → bootz 启动
```
