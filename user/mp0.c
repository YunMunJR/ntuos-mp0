#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define MAX_DEPTH 20
#define MAX_LENGTH 256

void print(char *path, int level, int is_last[], int keynum) {

    printf("%s %d\n",path, keynum);
}

void traverse(char *path, char *basename, int level, int is_last[], int *file_num,
              int *dir_num, char key, int num) {
    char buf[MAX_LENGTH], *p;
    int fd;
    struct dirent de;
    struct stat st;
    int len = 0;
    int keynum = num;

    for(int i=0;i<strlen(basename);i++){
        if(*(basename + i) == key){
            keynum++;
        }
    }

    if ((fd = open(path, 0)) < 0) {
        printf("%s [error opening dir]\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "tree: cannot stat (new recursion) %s\n", path);
        return;
    } else {
        close(fd);
    }

    if (st.type == T_FILE) {
        if (level == 0) {
            printf("%s [error opening dir]\n", path);
            return;
        }
        
        (*file_num)++;
        print(path, level, is_last, keynum);
        return;
    } else if (st.type == T_DIR) {
        if (level > 0) {
            (*dir_num)++;
        }
        print(path, level, is_last, keynum);
    } else {
        printf("tree: path %s is a device\n", path);
        return;
    }

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        printf("tree: path too long\n");
        close(fd);
        return;
    }

    strcpy(buf, path);
    len = strlen(buf);
    p = buf + len;
    *p++ = '/';

    int count = 0;
    if ((fd = open(path, 0)) < 0) {
        printf("%s [error opening dir]\n", path);
        return;
    }

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (strcmp(de.name, ".") && strcmp(de.name, "..")) {
            count++;
        }
    }

    close(fd);
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "tree: cannot open %s after counting files\n", path);
        return;
    }
    // read files under current path
    int cnt = 0;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;
        
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (strcmp(de.name, ".") && strcmp(de.name, "..")) {
            if (cnt == count - 1) {
                is_last[level] = 1;
            }
            cnt++;
            traverse(buf, de.name, level + 1, is_last, file_num, dir_num, key, keynum);
            is_last[level] = 0;
        }
    }

    close(fd);
    return;
}

void mp0(char *root, char key)
{
    // TODO: implement mp0
    // Hint: Use pipe, fork, and walk
    int pid, ret = 0;
    int fds[2];

    if (pipe(fds) < 0) {
        printf("tree: pipe failed\n");
        exit(-1);
    }

    pid = fork();
    if (pid == 0) { // Child
        int file_num = 0, dir_num = 0;
        int is_last[MAX_DEPTH] = {};

        traverse(root, root, 0, is_last, &file_num, &dir_num, key, 0);

        write(fds[1], &file_num, sizeof(int));
        write(fds[1], &dir_num, sizeof(int));
        printf("\n");

        exit(0);
    } else if (pid > 0) { // Parent

        int file_num, dir_num;
        if (read(fds[0], &file_num, sizeof(int)) != sizeof(int)) {
            printf("tree: pipe read failed (file_num)\n");
        }
        if (read(fds[0], &dir_num, sizeof(int)) != sizeof(int)) {
            printf("tree: pipe read failed (dir_num)\n");
        }
        printf("%d directories, %d files\n", dir_num, file_num);
    } else {
        printf("tree: fork failed\n");
        ret = -1;
    }

    close(fds[0]);
    close(fds[1]);
    exit(ret);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: mp0 <root_directory> <key>\n");
        exit(1);
    }
    mp0(argv[1], *argv[2]);
    exit(0);
}
