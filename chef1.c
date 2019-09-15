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

static int sem_id;
char *menu[2] = {"pork", "beef"};
pthread_mutex_t m;
sem_t sem1, sem2;
Node *smap;
int cnt = 0;
int num;

void *func1()
{
    pthread_mutex_lock(&m);
    sem_wait(&sem2);
    printf("准备阶段%d：厨师%d正在洗 %s\n", ++cnt, getpid(), menu[num]);
    sleep(3);
    printf("准备阶段%d：厨师%d洗 %s 完成\n", cnt, getpid(), menu[num]);
    pthread_mutex_unlock(&m);
    sem_post(&sem1);
}
void *func2()
{
    pthread_mutex_lock(&m);
    sem_wait(&sem1);
    printf("准备阶段%d：厨师%d正在切 %s\n", ++cnt, getpid(), menu[num]);
    sleep(2);
    printf("准备阶段%d：厨师%d切 %s 完成\n", cnt, getpid(), menu[num]);
    pthread_mutex_unlock(&m);
    sem_post(&sem2);
}
void *func3()
{
    printf("准备阶段%d：厨师%d正在烧水\n", ++cnt, getpid());
    sleep(8);
    printf("准备阶段%d：厨师%d烧水完成\n", cnt, getpid());
}

typedef union _semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} semun;
//initsem.c  对信号量赋初值，初值固定为1
int initsem(key_t semkey)
{
    int status = 0, semid;
    if ((semid = semget(semkey, 1, SEMPERM | IPC_CREAT | IPC_EXCL)) == -1)
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
    key_t semkey = 0x200;
    sem_id = initsem(semkey);
    int fd = -1;
    int i = 0;
    Node *smap = NULL;

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
    int c = 0;//CHEF1的下标
    // int shmid;
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

    printf("smap->a = %d\n", smap->a);
    while (1)
    {
        if(smap->a == -1){
            printf("正在等待订单\n");
            sleep(3);
        }
        else if (smap->t[c] == 0 && smap->_exit == 1)
        {
            munmap(smap, sizeof(Node) * 4);
            exit(1);
        }
        else if(c == smap->b){
            while (c == smap->b)
            {
                printf("chef1正在等待订单\n");
                sleep(3);
            }
            
        }
        else if (smap->t[c] == 1 || smap->t[c] == 2)
        {
            num = smap->t[c]-1;
            pthread_t tid1, tid2, tid3;
            pthread_mutex_init(&m, NULL);
            sem_init(&sem1, 0, 0);
            sem_init(&sem2, 0, 1);
            if (pthread_create(&tid1, NULL, func1, NULL) != 0)
            {
                perror("线程1失败");
            }
            if (pthread_create(&tid2, NULL, func2, NULL) != 0)
            {
                perror("线程2失败");
            }
            if (pthread_create(&tid3, NULL, func3, NULL) != 0)
            {
                perror("线程3失败");
            }
            pthread_join(tid1,NULL);
            pthread_join(tid2,NULL);
            pthread_join(tid3,NULL);
            c = (++c)%5;
            sem_destroy(&sem1);
            sem_destroy(&sem2);
            v(sem_id);
        }
        else
        {
            printf("当前厨房正在等待订单\n");
            sleep(5);
        }
    }
    munmap(smap, sizeof(Node) * 4);
    return 0;
}
