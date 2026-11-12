/*
 * ============================================================================
 * 文件: keyValue_app_test.c
 * 功能: 读取输入子系统事件，把 USB 键盘按键码翻译成可读键名并打印按键状态
 *
 * 作用:
 *   Linux input 子系统把 USB 键盘的按键上报为 /dev/input/eventX 上的
 *   struct input_event 事件（type = EV_KEY，code = KEY_* 键码，
 *   value = 0/1/2）。本程序打开指定 event 设备，阻塞读取事件，
 *   先把键码翻译成键名（如 KEY_A、KEY_ENTER），再打印按下/释放/重复状态，
 *   用于验证 USB 键盘驱动与输入子系统是否工作正常。
 *
 * 编译:
 *   arm-linux-gnueabihf-gcc keyValue_app_test.c -o keyValue_app_test
 * 运行:
 *   ./keyValue_app_test /dev/input/eventX   # X 为 USB 键盘对应的 event 编号
 *   ./keyValue_app_test                     # 不带参数时自动搜索键盘设备
 *

 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>

/* --------------------------------------------------------------------
 * 键码 -> 键名映射表
 * 键码宏定义来自 <linux/input.h>，这里用 #define 把 "code" 和 "名字"
 * 一起展开，避免手工重复书写字符串导致笔误。
 * ------------------------------------------------------------------ */
struct key_name_entry
{
    unsigned int code; /* input 子系统键码，即 KEY_* 宏的值 */
    const char *name;  /* 可读键名，直接使用宏名字符串 */
};

#define KEY_NAME(code) {(code), #code}

static const struct key_name_entry key_name_table[] = {
    /* --- 主键盘区：字母、数字、符号 --- */
    KEY_NAME(KEY_ESC),
    KEY_NAME(KEY_GRAVE),
    KEY_NAME(KEY_1),
    KEY_NAME(KEY_2),
    KEY_NAME(KEY_3),
    KEY_NAME(KEY_4),
    KEY_NAME(KEY_5),
    KEY_NAME(KEY_6),
    KEY_NAME(KEY_7),
    KEY_NAME(KEY_8),
    KEY_NAME(KEY_9),
    KEY_NAME(KEY_0),
    KEY_NAME(KEY_MINUS),
    KEY_NAME(KEY_EQUAL),
    KEY_NAME(KEY_BACKSPACE),
    KEY_NAME(KEY_TAB),
    KEY_NAME(KEY_Q),
    KEY_NAME(KEY_W),
    KEY_NAME(KEY_E),
    KEY_NAME(KEY_R),
    KEY_NAME(KEY_T),
    KEY_NAME(KEY_Y),
    KEY_NAME(KEY_U),
    KEY_NAME(KEY_I),
    KEY_NAME(KEY_O),
    KEY_NAME(KEY_P),
    KEY_NAME(KEY_LEFTBRACE),
    KEY_NAME(KEY_RIGHTBRACE),
    KEY_NAME(KEY_ENTER),
    KEY_NAME(KEY_LEFTCTRL),
    KEY_NAME(KEY_A),
    KEY_NAME(KEY_S),
    KEY_NAME(KEY_D),
    KEY_NAME(KEY_F),
    KEY_NAME(KEY_G),
    KEY_NAME(KEY_H),
    KEY_NAME(KEY_J),
    KEY_NAME(KEY_K),
    KEY_NAME(KEY_L),
    KEY_NAME(KEY_SEMICOLON),
    KEY_NAME(KEY_APOSTROPHE),
    KEY_NAME(KEY_LEFTSHIFT),
    KEY_NAME(KEY_BACKSLASH),
    KEY_NAME(KEY_Z),
    KEY_NAME(KEY_X),
    KEY_NAME(KEY_C),
    KEY_NAME(KEY_V),
    KEY_NAME(KEY_B),
    KEY_NAME(KEY_N),
    KEY_NAME(KEY_M),
    KEY_NAME(KEY_COMMA),
    KEY_NAME(KEY_DOT),
    KEY_NAME(KEY_SLASH),
    KEY_NAME(KEY_RIGHTSHIFT),
    KEY_NAME(KEY_KPASTERISK),
    KEY_NAME(KEY_LEFTALT),
    KEY_NAME(KEY_SPACE),
    KEY_NAME(KEY_CAPSLOCK),

    /* --- 功能键 F1 ~ F12 --- */
    KEY_NAME(KEY_F1),
    KEY_NAME(KEY_F2),
    KEY_NAME(KEY_F3),
    KEY_NAME(KEY_F4),
    KEY_NAME(KEY_F5),
    KEY_NAME(KEY_F6),
    KEY_NAME(KEY_F7),
    KEY_NAME(KEY_F8),
    KEY_NAME(KEY_F9),
    KEY_NAME(KEY_F10),
    KEY_NAME(KEY_F11),
    KEY_NAME(KEY_F12),

    /* --- 小键盘 --- */
    KEY_NAME(KEY_NUMLOCK),
    KEY_NAME(KEY_KP0),
    KEY_NAME(KEY_KP1),
    KEY_NAME(KEY_KP2),
    KEY_NAME(KEY_KP3),
    KEY_NAME(KEY_KP4),
    KEY_NAME(KEY_KP5),
    KEY_NAME(KEY_KP6),
    KEY_NAME(KEY_KP7),
    KEY_NAME(KEY_KP8),
    KEY_NAME(KEY_KP9),
    KEY_NAME(KEY_KPDOT),
    KEY_NAME(KEY_KPMINUS),
    KEY_NAME(KEY_KPPLUS),
    KEY_NAME(KEY_KPENTER),
    KEY_NAME(KEY_KPSLASH),
    KEY_NAME(KEY_KPCOMMA),

    /* --- 控制键与方向键 --- */
    KEY_NAME(KEY_SCROLLLOCK),
    KEY_NAME(KEY_SYSRQ),
    KEY_NAME(KEY_PAUSE),
    KEY_NAME(KEY_INSERT),
    KEY_NAME(KEY_HOME),
    KEY_NAME(KEY_PAGEUP),
    KEY_NAME(KEY_DELETE),
    KEY_NAME(KEY_END),
    KEY_NAME(KEY_PAGEDOWN),
    KEY_NAME(KEY_UP),
    KEY_NAME(KEY_DOWN),
    KEY_NAME(KEY_LEFT),
    KEY_NAME(KEY_RIGHT),
    KEY_NAME(KEY_RIGHTCTRL),
    KEY_NAME(KEY_RIGHTALT),
    KEY_NAME(KEY_LEFTMETA),
    KEY_NAME(KEY_RIGHTMETA),
    KEY_NAME(KEY_COMPOSE),

    /* --- 多媒体 / 电源 / 指示灯相关键 --- */
    KEY_NAME(KEY_MUTE),
    KEY_NAME(KEY_VOLUMEDOWN),
    KEY_NAME(KEY_VOLUMEUP),
    KEY_NAME(KEY_POWER),
    KEY_NAME(KEY_SLEEP),
    KEY_NAME(KEY_WAKEUP),
    KEY_NAME(KEY_PLAYPAUSE),
    KEY_NAME(KEY_NEXTSONG),
    KEY_NAME(KEY_PREVIOUSSONG),
    KEY_NAME(KEY_STOPCD),
    KEY_NAME(KEY_FASTFORWARD),
    KEY_NAME(KEY_REWIND),
    KEY_NAME(KEY_HOMEPAGE),
    KEY_NAME(KEY_BACK),
    KEY_NAME(KEY_FORWARD),
    KEY_NAME(KEY_REFRESH),
    KEY_NAME(KEY_SEARCH),
    KEY_NAME(KEY_CALC),
    KEY_NAME(KEY_WWW),
    KEY_NAME(KEY_MAIL),
    KEY_NAME(KEY_PRINT),
    KEY_NAME(KEY_BRIGHTNESSDOWN),
    KEY_NAME(KEY_BRIGHTNESSUP),
    KEY_NAME(KEY_KBDILLUMTOGGLE),
    KEY_NAME(KEY_KBDILLUMDOWN),
    KEY_NAME(KEY_KBDILLUMUP),
    KEY_NAME(KEY_SWITCHVIDEOMODE),
};

#undef KEY_NAME

/* 每个 unsigned long 的位数，用于按键位图下标计算（32 位 ARM 上为 32） */
#define KEY_BITS_PER_LONG (8 * sizeof(unsigned long))

/* 测试位图某一位是否为 1：用于判断设备是否支持某个键码 */
#define TEST_BIT(nr, addr)                                       \
    (((const unsigned long *)(addr))[(nr) / KEY_BITS_PER_LONG] & \
     (1UL << ((nr) % KEY_BITS_PER_LONG)))

/* Ctrl+C 退出标志，sig_atomic_t 保证信号处理函数中读写安全 */
static volatile sig_atomic_t g_stop = 0;

/* --------------------------------------------------------------------
 * 函数名: on_sigint
 * 功能  : SIGINT（Ctrl+C）信号处理函数，只置退出标志
 * 参数  : sig - [in] 信号编号，未使用
 * 返回值: 无
 * 注意  : 信号处理上下文中只能做异步信号安全的操作，因此仅设置标志位
 * ------------------------------------------------------------------ */
static void on_sigint(int sig)
{
    (void)sig;
    g_stop = 1;
}

/* --------------------------------------------------------------------
 * 函数名: keycode_to_name
 * 功能  : 把 input 子系统键码翻译成可读键名
 * 参数  : code - [in] EV_KEY 事件的键码（KEY_* 宏的值）
 * 返回值: 成功返回键名字符串（静态常量，调用方不可释放）；
 *         表中不存在时返回 "UNKNOWN"
 * 注意  : 线性查找，表很小，开销可忽略
 * ------------------------------------------------------------------ */
static const char *keycode_to_name(unsigned int code)
{
    size_t i;

    for (i = 0; i < sizeof(key_name_table) / sizeof(key_name_table[0]); i++)
    {
        if (key_name_table[i].code == code)
            return key_name_table[i].name;
    }

    return "UNKNOWN";
}

/* --------------------------------------------------------------------
 * 函数名: keyvalue_to_state
 * 功能  : 把 EV_KEY 事件的 value 翻译成中文状态字符串
 * 参数  : value - [in] 0 = 释放, 1 = 按下, 2 = 自动重复（长按）
 * 返回值: 状态字符串常量指针；其他值返回 "未知"
 * ------------------------------------------------------------------ */
static const char *keyvalue_to_state(int value)
{
    switch (value)
    {
    case 0:
        return "释放";
    case 1:
        return "按下";
    case 2:
        return "重复";
    default:
        return "未知";
    }
}

/* --------------------------------------------------------------------
 * 函数名: find_keyboard_device
 * 功能  : 自动扫描 /dev/input/event0 ~ event31，找到支持字母键的设备
 * 参数  : path    - [out] 找到的设备节点路径，如 /dev/input/event1
 *         pathlen - [in]  path 缓冲区大小，建议不小于 32 字节
 * 返回值: 成功返回 0 并写出 path；未找到键盘返回 -1
 * 注意  : 只做只读探测，不长期占用设备；open 失败（无权限/无设备）则跳过
 * ------------------------------------------------------------------ */
static int find_keyboard_device(char *path, size_t pathlen)
{
    /* 按键能力位图：每一位对应一个 KEY_* 键码，KEY_MAX 为最大键码 */
    unsigned long keybit[(KEY_MAX / KEY_BITS_PER_LONG) + 1];
    int i;

    for (i = 0; i < 32; i++)
    {
        int fd;
        int ret;

        snprintf(path, pathlen, "/dev/input/event%d", i);

        /* 非阻塞打开：设备不存在或无权限时立即返回，不阻塞 */
        fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;

        /* 查询设备支持的 EV_KEY 键码位图 */
        memset(keybit, 0, sizeof(keybit));
        ret = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);
        close(fd);

        if (ret < 0)
            continue;

        /*
         * 键盘一定同时支持字母键 A 和回车键 ENTER；
         * 鼠标/触摸屏虽然也有按键位，但不会有字母键，以此区分。
         */
        if (TEST_BIT(KEY_A, keybit) && TEST_BIT(KEY_ENTER, keybit))
            return 0;
    }

    return -1;
}

/* --------------------------------------------------------------------
 * 函数名: main
 * 功能  : 打开 event 设备，循环读取并解析按键事件后打印
 * 参数  : argc/argv - [in] argv[1] 可选，指定 /dev/input/eventX
 * 返回值: 正常退出（Ctrl+C）返回 0；打开/读取失败返回 -1
 * 注意  : read 为阻塞读取，按键到来前一直等待
 * ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    char dev_path[64];
    char dev_name[256];
    struct sigaction sa;
    int fd;

    /* --- 1. 确定要打开的 event 设备 --- */
    if (argc >= 2)
    {
        /* 命令行指定了设备节点 */
        snprintf(dev_path, sizeof(dev_path), "%s", argv[1]);
        fd = open(dev_path, O_RDONLY);
        if (fd < 0)
        {
            perror("打开输入设备失败");
            return -1;
        }
    }
    else
    {
        /* 未指定设备时自动搜索 USB 键盘 */
        if (find_keyboard_device(dev_path, sizeof(dev_path)) < 0)
        {
            fprintf(stderr, "未找到键盘设备，请手动指定: %s /dev/input/eventX\n",
                    argv[0]);
            return -1;
        }

        fd = open(dev_path, O_RDONLY);
        if (fd < 0)
        {
            perror("打开输入设备失败");
            return -1;
        }
    }

    /* --- 2. 打印设备名，确认打开的是哪个键盘（如 "USB Keyboard"） --- */
    memset(dev_name, 0, sizeof(dev_name));
    if (ioctl(fd, EVIOCGNAME(sizeof(dev_name)), dev_name) >= 0)
        printf("已打开设备 %s: %s\n", dev_path, dev_name);
    else
        printf("已打开设备 %s\n", dev_path);
    printf("等待按键... (Ctrl+C 退出)\n");

    /* --- 3. 注册 SIGINT 处理，让 Ctrl+C 能打断阻塞的 read --- */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    /* 不设置 SA_RESTART，read 才能被信号打断并返回 EINTR */
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    /* 行缓冲：即使输出重定向到文件，也能及时看到按键信息 */
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* --- 4. 循环读取事件并解析 --- */
    while (!g_stop)
    {
        struct input_event in_ev;
        ssize_t n;

        n = read(fd, &in_ev, sizeof(in_ev)); /* 阻塞读取一个事件 */
        if (n < 0)
        {
            if (errno == EINTR) /* 被 Ctrl+C 打断，回到循环检查退出标志 */
                continue;

            perror("读取输入事件失败");
            close(fd);
            return -1;
        }

        if ((size_t)n != sizeof(in_ev))
        {
            /* 读到不完整的输入事件结构，说明设备异常 */
            fprintf(stderr, "读取事件长度异常: %zd\n", n);
            close(fd);
            return -1;
        }

        /* 只关心按键事件；EV_SYN/EV_MSC 等同步、扫描码事件直接忽略 */
        if (EV_KEY != in_ev.type)
            continue;

        printf("[%s] %-16s code=%-3u (0x%02x)\n",
               keyvalue_to_state(in_ev.value),
               keycode_to_name(in_ev.code),
               (unsigned int)in_ev.code,
               (unsigned int)in_ev.code);
    }

    close(fd);
    printf("退出\n");
    return 0;
}