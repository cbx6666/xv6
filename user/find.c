#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 从完整路径中提取文件名并进行格式化
char*
fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // 从路径末尾开始向前查找，直到找到'/'或到达路径开头
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;  // 跳过'/'，指向文件名开头

    // 如果文件名太长，直接返回原始指针
    if (strlen(p) >= DIRSIZ)
        return p;

    // 否则复制到缓冲区并用空格填充到固定长度
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;
    return buf;
}

void 
find(char *path, char *target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    // 打开文件
    if ((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // 获取文件状态
    if (fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 根据文件类型进行搜索
    switch (st.type) {
    case T_FILE:
        if (strcmp(fmtname(path), target) == 0) {
            printf("%s\n", path);
        }
        break;
    case T_DIR:
        // 检查路径长度，防止缓冲区溢出
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("find: path too long\n");
            break;
        }

        // 指针指向路径末尾
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de)){
            if (de.inum == 0)
                continue;
            
            // 将 de.name 中的数据复制到 p 指向的位置
            memmove(p, de.name, DIRSIZ);  // de.name 是一个固定长度的字符数组
            p[DIRSIZ] = 0;  // 在复制的数据末尾添加字符串结束符 \0

            // 跳过'.'和'..'
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            if (strcmp(de.name, target) == 0) {
                printf("%s\n", buf);
            }

            // 检查文件状态获取是否成功
            if (stat(buf, &st) < 0) {
                printf("find: cannot stat %s\n", buf);
                continue;
            }

            // 如果目录项类型为目录，则递归搜索
            if (st.type == T_DIR) {
                find(buf, target);
            }
        }
        break;
    }

    close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(2, "Usage: find <path> <name>\n");
        exit(1);
    }
    
    find(argv[1], argv[2]);
    exit(0);
}