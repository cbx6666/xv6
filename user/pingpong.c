#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char *argv[])
{
    int p1[2], p2[2];  // 两个管道：p1用于父->子，p2用于子->父
    int pid;
    char buf[64];  // 用于读取数据的缓冲区
    
    // 创建两个管道
    if (pipe(p1) < 0) {
        printf("pipe p1 failed\n");
        exit(1);
    }
    if (pipe(p2) < 0) {
        printf("pipe p2 failed\n");
        exit(1);
    }

    // 创建子进程
    pid = fork();
    if (pid < 0) {
         printf("fork failed\n");
         exit(1);
    }

    // 子进程
    if (pid == 0) {
        close(p1[1]);  // 子进程不需要对管道1进行写操作
        close(p2[0]);  // 子进程不需要对管道2进行读操作

        // 从p1读取父进程发送的数据
        if (read(p1[0], buf, sizeof(buf) > 0))
            printf("%d: received ping\n", getpid());

        // 向p2写入数据给父进程
        write(p2[1], "pong", 4);
        
        // 关闭剩余的文件描述符
        close(p1[0]);
        close(p2[1]);

        exit(0);
    }
    // 父进程
    else {
        close(p1[0]);  // 父进程不需要对管道1进行读操作
        close(p2[1]);  // 父进程不需要对管道2进行写操作

        // 向p1写入数据给子进程
        write(p1[1], "ping", 4);

        // 从p2读取子进程发送的数据
        if (read(p2[0], buf, sizeof(buf) > 0))
            printf("%d: received pong\n", getpid());

        // 等待子进程结束
        wait(0);

        // 关闭剩余的文件描述符
        close(p1[1]);
        close(p2[0]);

        exit(0);
    }

    return 0;
}