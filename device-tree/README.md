# 1. 设备树修改后用应用程序验证

修改设备树后用应用程序测试，步骤：

## 1. 编译设备树并拷到 TFTP

```sh
cd ~/linux/linux-imx6ull
make imx6ull-14x14-emmc-7-1024x600-c.dtb
cp arch/arm/boot/dts/imx6ull-14x14-emmc-7-1024x600-c.dtb ~/linux/tftp/
```

## 2. 交叉编译测试程序

```sh
cd ~/iM6ULL_DriverLearning/device-tree
arm-linux-gnueabihf-gcc led_app_test.c -o led_app_test
```

## 3. 拷可执行文件到 NFS rootfs

```sh
cp ./led_app_test /home/zr-arm/linux/nfs/rootfs/home/root
```

## 4. 开发板上运行验证

```sh
cd /home/root
chmod +x led_app_test
./led_app_test
```

现象：LED 亮 1 秒、灭 2 秒循环，说明设备树修改生效。

> 程序路径 `/sys/class/leds/sys-led-zr/brightness` 中的 `sys-led-zr` 是 dts 里的 `label`，改了 label 需同步修改；可用 `ls /sys/class/leds/` 确认。
