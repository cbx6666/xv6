#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int 
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: xargs command [args...]\n");
        exit(1);
    }

    // 存储从标准输入读取的每一行
    char line[512];

    // 循环读取标准输入的每一行
    while (gets(line, sizeof(line)) != 0){
        // 去掉行末的换行符
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

         // 跳过空行
        if (strlen(line) == 0) {
            continue;
        }

        // 解析当前行的参数
        char *line_args[MAXARG];
        int line_argc = 0;

        char *p = line;
        while(*p && line_argc < MAXARG - 1) {
            // 跳过前导空格
            while (*p == ' ' || *p == '\t') {
                p++;
            }

            // 如果到了字符串末尾，退出
            if (*p == '\0') {
                break;
            }

            // 记录参数开始位置
            line_args[line_argc++] = p;
            
            // 找到参数结束位置
            while (*p && *p != ' ' && *p != '\t') {
                p++;
            }
            
            // 如果不是字符串末尾，用 '\0' 分隔参数
            if (*p) {
                *p++ = '\0';
            }
        }

        // 构造新的参数数组
        char *new_argv[MAXARG];
        int new_argc = 0;

        // 复制原命令和参数
        for (int i = 1; i < argc && new_argc < MAXARG - 1; i++) {
            new_argv[new_argc++] = argv[i];
        }
        
        // 添加从标准输入读取的参数
        for (int i = 0; i < line_argc && new_argc < MAXARG - 1; i++) {
            new_argv[new_argc++] = line_args[i];
        }

        // 参数数组必须以 NULL 结尾
        new_argv[new_argc] = 0;

        // 创建子进程执行命令
        if (fork() == 0) {
            // 子进程执行命令
            exec(argv[1], new_argv);
            printf("exec %s failed\n", argv[1]);
            exit(1);
        } else {
            // 父进程等待子进程完成
            wait(0);
        }
    }

    exit(0);
}