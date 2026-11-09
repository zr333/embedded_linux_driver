#include <stdio.h>                            
#include <stdlib.h>                                                */
#include <fcntl.h>                                    
#include <unistd.h>                                      
#include <linux/input.h> /* struct input_event 及 EV_* / KEY_* 宏定义     */

/*
 * ============================================================================
 * 文件: key_app.c
 * 功能: Linux input 子系统(evdev 接口)用户态测试程序
 *
 * 作用:
 *   打开一个输入设备节点(如 /dev/input/event1),以阻塞方式不断读取
 *   struct input_event 结构体,并把事件的 类型 / 代码 / 值 打印到终端。
 *   通过本程序可以直观地看到按键按下、松开、长按重复等原始事件,
 *   从而验证底层 input 驱动是否正确上报事件。
 *
 *  先确认按键对应的设备节点：
 *  cat /proc/bus/input/devices
 *  执行此程序：
 *  ./key /dev/input/event0
 * ============================================================================
 */

int main(int argc, char *argv[])
{
    /* --------------------------------------------------------------------
     * 1. 参数检查
     *    argc: 命令行参数个数,程序名本身算 1 个,所以至少为 2
     *          (argv[0] = "./key_app", argv[1] = "/dev/input/event2")
     *    argv: 参数字符串数组
     * ------------------------------------------------------------------ */
    if (argc < 2)
    {
        /* 参数不够,打印用法提示后退出。
         * 用 fprintf(stderr, ...) 而不是 printf,是 POSIX 惯例:
         * 错误信息走"标准错误",即使用户把标准输出重定向到文件,
         * 提示信息依然会显示在终端上。
         * argv[0] 就是程序自身的名字,这样提示语更通用。 */
        fprintf(stderr, "Usage: %s <input_event_device>\n", argv[0]);
        exit(-1); /* 退出码 -1,进程实际返回给 shell 的是 255(只取低 8 位) */
    }

    /* --------------------------------------------------------------------
     * 2. 打开输入设备节点
     *    argv[1]   : 设备路径,如 /dev/input/event0 ~ eventN
     *    O_RDONLY  : 只读打开。读事件只需读权限;
     *                若要回写(如点亮 LED、发送力反馈)才需要 O_RDWR
     *    返回值 fd : 成功 >= 0;失败返回 -1,错误码放在全局变量 errno 中
     * ------------------------------------------------------------------ */
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        /* perror 会自动在提示语后面追加 ": " + errno 对应的错误描述,
         * 例如 "Failed to open input event device: No such file or directory"
         * 或 "...: Permission denied",便于快速定位问题。 */
        perror("Failed to open input event device");
        exit(-1);
    }

    /* --------------------------------------------------------------------
     * 3. 定义事件结构体变量
     *
     * struct input_event (见 linux/input.h) 主要成员:
     *   struct timeval time;  // 事件产生的时间戳(秒 + 微秒)
     *                         // 新内核中拆成 input_event_sec / input_event_usec
     *   __u16 type;           // 事件类型:EV_KEY / EV_REL / EV_ABS / EV_SYN ...
     *   __u16 code;           // 事件代码:type=EV_KEY 时即按键码 KEY_xxx
     *   __s32 value;          // 事件值  :按键 1=按下 0=松开 2=长按重复
     * ------------------------------------------------------------------ */
    struct input_event in_ev;

    /* --------------------------------------------------------------------
     * 4. 无限循环读取事件
     *    for (;;) 等价于 while (1),是"死循环"的惯用写法。
     * ------------------------------------------------------------------ */
    // 无限循环读取按键事件
    for (;;) // 相当于while(1) 无限循环
    {
        if (sizeof(struct input_event) !=
            read(fd, &in_ev, sizeof(struct input_event))) // 阻塞读取
        {
            /* 读失败:打印错误原因,关闭设备后退出 */
            perror("Failed to read input event");
            close(fd);
            exit(-1);
        }

    
        printf("Event Type: %d, Code: %d, Value: %d\n", in_ev.type, in_ev.code, in_ev.value);
    }

    printf("Event Type: %d, Code: %d, Value: %d\n", in_ev.type, in_ev.code, in_ev.value);
    close(fd);
    return 0;
}