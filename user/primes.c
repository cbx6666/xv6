#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
sieve(int input_fd)
{
    int prime,n;

    // 读取第一个数字，它必定是素数
    if (read(input_fd, &prime, sizeof(prime)) == 0) {
        close(input_fd);  // 确保关闭文件描述符
        exit(0);  // 没有更多数字，退出
    }

    printf("prime %d\n", prime);

    // 创建管道给下一个筛选进程
    int p[2];
    if (pipe(p) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    if (fork() == 0) {
        // 子进程：下一个筛选层
        close(p[1]);  // 关闭写端
        close(input_fd);  // 关闭上一层的管道
        sieve(p[0]);  // 用新管道的读端作为input_fd
        exit(0);
    } else {
        // 父进程
        close(p[0]);  // 关闭新管道的读端

        // 筛选数据
        while (read(input_fd, &n, sizeof(n)) > 0) {
            if (n % prime != 0) {
                write(p[1], &n, sizeof(n));
            }
        }

        close(input_fd);  // 关闭上一层的管道
        close(p[1]);  // 关闭新管道的写端
        wait(0);  // 等待子进程完成
        exit(0);
    }
}

int 
main(int argc, char *argv[])
{
    int p[2];

    // 创建第一个管道
    if (pipe(p) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    if (fork() == 0) {
        // 子进程：第一个筛选进程
        close(p[1]);  // 关闭写端
        sieve(p[0]);  // 开始筛选
        exit(0);
    } else {
        // 父进程：生成2到35的所有数字
        close(p[0]);  // 关闭读端
        
        // 写入数字（相当于初始化数组）
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(i));
        }
        
        close(p[1]);  // 关闭写端
        wait(0);  // 等待子进程完成
    }
    
    exit(0);
}