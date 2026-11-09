/* ---------------------------------------------------------------- 头文件 -- */
#include <fcntl.h>     /* open / O_WRONLY                              */
#include <stdio.h>     /* perror                                       */
#include <sys/stat.h>  /* 文件属性相关(本程序未直接使用, 保留习惯写法)  */
#include <sys/types.h> /* 基本类型定义                                 */
#include <unistd.h>    /* write / sleep / close                        */

/*
 * ============================================================================
 * 文件: led_app_test.c
 * 功能: 通过 sysfs 接口控制 LED 亮灭的用户态测试程序
 *
 * 作用:
 *   循环向 /sys/class/leds/sys-led-zr/brightness 写入 "1" / "0",
 *   让 LED 亮 1 秒、灭 2 秒, 用于验证修改设备树后 LED 节点是否生效。
 *
 * 编译(交叉编译, i.MX6ULL 是 ARM Cortex-A7):
 *   arm-linux-gnueabihf-gcc led_app_test.c -o led_app_test
 * 运行(开发板终端, 需先将可执行文件拷入 rootfs):
 *   ./led_app_test
 *
 * 背景知识 / 注意事项:
 *   1. sys-led-zr 是设备树 leds 子节点里的 label, 改 label 必须同步改本文件
 *      的路径, 否则 open 会因目录不存在而失败; 可用 ls /sys/class/leds/ 确认。
 *   2. brightness 只需写入, 故用 O_WRONLY 打开;
 *      写 "0" 熄灭, 写 "1" 点亮, 部分驱动还支持 0 ~ max_brightness 的数值。
 * ============================================================================
 */

/* LED 亮度节点: /sys/class/leds/<label>/brightness,
 * 其中 <label> 由设备树中 leds 节点的 label 属性决定。 */
#define LED_BRIGHTNESS_PATH "/sys/class/leds/sys-led-zr/brightness"

/* --------------------------------------------------------------------
 * 函数名: main
 * 功能  : 打开 LED 亮度节点, 循环控制 LED 亮 1 秒 / 灭 2 秒
 * 参数  : 无(本程序不接收命令行参数)
 * 返回值: 正常情况永不返回(死循环);
 *         open 失败时返回 1
 * 注意  : 需以 root 运行, 普通用户可能没有 /sys 下节点的写权限;
 *         需在存在 leds 的设备树环境中运行, 否则设备节点不存在
 * ------------------------------------------------------------------ */
int main()
{
    /* 以只写方式打开亮度节点:
     *   - 不需要读取当前亮度, 因此用 O_WRONLY;
     *   - sysfs 属性文件必须已存在, 打开不会自动创建设备,
     *     节点缺失通常说明设备树未生效或 label 名字写错。 */
    int fd = open(LED_BRIGHTNESS_PATH, O_WRONLY);
    if (fd < 0)
    {
        /* 常见错误: No such file or directory(label 不匹配 / 设备树未加载)、
         *           Permission denied(权限不足) */
        perror("Failed to open brightness file");
        return 1;
    }

    /* --------------------------------------------------------------------
     * 闪烁主循环: 亮 1 秒 -> 灭 2 秒, 无限循环, Ctrl+C 结束
     * ------------------------------------------------------------------ */
    while (1)
    {
        /* 写入 1 个字节 "1": 点亮 LED。
         * 注意 sysfs 属性要求写入的字符串长度与内容符合驱动约定,
         * 这里只写 1 个字节, 因此不需要写 '\n' 结尾。 */
        write(fd, "1", 1); // Turn on the LED
        sleep(1);          // 保持点亮 1 秒(sleep 单位: 秒)

        /* 写入 "0": 熄灭 LED */
        write(fd, "0", 1); // Turn off the LED
        sleep(2);          // 保持熄灭 2 秒
    }

    /* 不可达代码: 上面是死循环, 本行仅用于让 return 0 语法完整,
     * 实际执行不到, 也不会走到 close(fd) —— 进程被 Ctrl+C 直接终止。 */
    return 0;
}