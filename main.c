#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MMAP_FILE "/var/tmp/test_mmap"
#define NAME_LEN 64
#define SEGSIZE 4096

typedef struct node
{
    int t[5];
    int a;
    int b;
    int _exit;
} Node;
Node *smap;
void func1()
{
    if (smap->a == -1)
    {

        smap->a = 0;
        smap->t[smap->b++] = 1;
        smap->b %= 5;
    }
    else if (smap->b == smap->a - 1)
    {
        printf("订单已满\n");
        return;
    }
    else
    {
        printf("charge the list\n");
        smap->t[smap->b++] = 1;
        smap->b %= 5;
    }
}
void func2()
{
    if (smap->b == smap->a)
    {
        printf("订单已满\n");
        return;
    }
    else if (smap->a == -1)
    {
        smap->a = 0;
        smap->t[smap->b++] = 2;
    }
    else
    {
        smap->t[smap->b++] = 2;
        smap->b %= 5;
    }
}
void funcexit()
{
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    printf("现在开始不接受订单\n");
}
int main()
{
    // int shmid;
    pid_t p;
    int fd = -1;
    if ((fd = open(MMAP_FILE, O_RDWR | O_CREAT | O_TRUNC)) < 0)
    {
        perror("open file error\n");
        return -1;
    }

    if (lseek(fd, sizeof(Node) * 2 - 1, SEEK_SET) < 0)
    {
        perror("lseek error\n");
        close(fd);
        return -1;
    }

    write(fd, "", 1);

    if ((smap = (Node *)mmap(NULL, sizeof(Node) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        perror("mmap error\n");
        return -1;
    }

    close(fd);
    // key_t key = ftok(".", 0);
    // if (key == -1)
    // {
    //     perror("ftok error");
    //     return -1;
    // }
    // shmid = shmget(key, SEGSIZE, IPC_CREAT | IPC_EXCL | 0664);
    // if (shmid == -1)
    // {
    //     perror("shmget error");
    //     return -1;
    // }
    // smap = (Node *)shmat(shmid, NULL, 0);
    smap->a = -1;
    smap->b = 0;
    smap->_exit = 0; // 共享内存初始化完成

    signal(SIGUSR1, func1);
    signal(SIGUSR2, func2);
    signal(SIGINT, funcexit);
    int i = 0;
    for (i = 0; i < 2; i++)
    {
        if ((p = fork()) == 0)
        {
            break;
        }
        printf("child %d's pid = %d\n", i, p);
    }
    if (i == 0)
    {
        execl("./chef1", "chef", NULL);
    }
    else if (i == 1)
    {
        execl("./chef2", "chef2", NULL);
    }
    else if (i == 2)
    {
        while (wait(NULL) != -1)
        {
        }
    }
    // if (shmdt(smap) == -1)
    // {
    //     perror("解除绑定错误");
    //     return -1;
    // }
    // shmctl(shmid, IPC_RMID, 0);
}
