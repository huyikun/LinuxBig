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
#define SEMPERM 0664
typedef struct node
{
    int t[5];
    int a;
    int b;
    int _exit;
} Node;
char *menu[2] = {"pork", "beef"};
Node *smap;
pthread_mutex_t m;
void *func1()
{
    pthread_mutex_lock(&m);
    int index = smap->t[smap->a]-1;
    printf("烹饪阶段：厨师%d开始炒 %s\n", getpid(), menu[index]);
    sleep(3);
    printf("烹饪阶段：厨师%d 炒 %s 完成\n", getpid(), menu[index]);
    smap->t[smap->a++] = 0;
    smap->a %= 5;
    pthread_mutex_unlock(&m);
}

typedef union _semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} semun;
static int sem_id;
//initsem.c  对信号量赋初值，初值固定为1
int initsem(key_t semkey)
{
    int status = 0, semid;
    if ((semid = semget(semkey, 1, SEMPERM | IPC_CREAT)) == -1)
    {
        if (errno == EEXIST)
            semid = semget(semkey, 1, 0);
    }
    else
    {
        semun arg;
        arg.val = 0;
        status = semctl(semid, 0, SETVAL, arg);
    }
    if (semid == -1 || status == -1)
    {
        perror("initsem failed");
        return (-1);
    }
    /*all ok*/
    return (semid);
}

//v.c  V操作
int v(int semid)
{
    struct sembuf v_buf;

    v_buf.sem_num = 0;
    v_buf.sem_op = 1;
    v_buf.sem_flg = SEM_UNDO;
    if (semop(semid, &v_buf, 1) == -1)
    {

        perror("v(semid)failed");
        exit(1);
    }
    return (0);
}

//p操作
int p(int semid)
{
    struct sembuf p_buf;
    p_buf.sem_num = 0;
    p_buf.sem_op = -1;
    p_buf.sem_flg = SEM_UNDO;

    //p_buf = {0,-1,SEM_UNDO};
    if (semop(semid, &p_buf, 1) == -1)
    {
        perror("p(semid)failed");
        exit(1);
    }
    return (0);
}
int main()
{
    int shmid;
    int fd = -1;
    int i = 0;
    key_t semkey = 0x200;
    sem_id = initsem(semkey);
    if ((fd = open(MMAP_FILE, O_RDWR | O_CREAT)) < 0)
    {
        perror("open file error\n");
        return -1;
    }
    //consume map data
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
    // shmid = shmget(key, 0, 0);
    // if (shmid == -1)
    // {
    //     perror("shmget error");
    //     return -1;
    // }
    // smap = (Node *)shmat(shmid, NULL, 0);
    while (1)
    {
        p(sem_id);
        pthread_mutex_init(&m, NULL);
        pthread_t tid1;
        if (pthread_create(&tid1, NULL, func1, NULL) != 0)
        {
            perror("chef2 线程失败");
        }
        pthread_join(tid1, NULL);
        if (smap->a == -1)
        {
            printf("chef2 is waitiing\n");
            sleep(5);
        }
        else if (smap->_exit == 1 && smap->t[smap->a] == 0)
        {

            munmap(smap, sizeof(Node) * 4);
            exit(1);
        }
    }

    // if (shmdt(smap) == -1)
    // {
    //     perror("解除绑定错误");
    //     return -1;
    // }
    // shmctl(shmid, IPC_RMID, 0);

    return 0;
}
