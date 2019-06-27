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
#define SEGSIZE 4096
#define SEMPERM 0600
typedef struct node {
    int* t[5] ;
    int a ;
    int b ;
    int _exit;
}Node;

static int sem_id;
char * menu[2] = {"pork", "beef"};
pthread_mutex_t m;
sem_t sem1, sem2;
Node * smap;
int cnt = 0;
int num;

void func1()
{
    pthread_mutex_lock(&m);
    sem_wait(&sem2);
    printf("准备阶段%d：厨师%d正在洗 %s \n", ++cnt, getpid(), menu[num]);
    sleep(3);
    printf("准备阶段%d：厨师%d洗 %s 完成\n", cnt, getpid(), menu[num]);
    pthread_mutex_unlock(&m);
    sem_post(&sem1);
}
void func2()
{
    pthread_mutex_lock(&m);
    sem_wait(&sem1);
    printf("准备阶段%d：厨师%d正在切 %s", ++cnt, getpid(), menu[num]);
    sleep(2);
    printf("准备阶段%d：厨师%d切 %s 完成", cnt, getpid(), menu[num]);
    pthread_mutex_unlock(&m);
    sem_post(&sem2);
}
void func3()
{
    pthread_mutex_lock(&m);
    printf("准备阶段#d：厨师%d正在烧水", ++cnt, getpid());
    sleep(8);
    printf("准备阶段%d：厨师%d烧水完成", cnt, getpid());
    pthread_mutex_unlock(&m);
}

typedef union _semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
} semun;
//initsem.c  对信号量赋初值，初值固定为1
int initsem(key_t semkey)
{
    int status = 0, semid;
    if ((semid = semget(semkey, 0, SEMPERM | IPC_CREAT | IPC_EXCL)) == -1)
    {
        if (errno == EEXIST)
            semid = semget(semkey, 0, 0);
    }
    else
    {
        semun arg;
        arg.val = 1;
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
    int i, shmid;
    key_t k = ftok(".", 0);
    if (k == -1)
    {
        perror("ftok error");
        return -1;
    }
    shmid = shmget(k, SEGSIZE, IPC_CREAT | IPC_EXCL | 0664);
    if (shmid == -1)
    {
        perror("shmget error");
        return -1;
    }
    smap = (Node *)shmat(shmid, NULL, 0);

    while (1)
    {
        if (smap->t[smap->a] == 0 && smap->_exit == 1)
        {
            exit(0);
        }
        else if (smap->t[smap->a] == 1 || smap->t[smap->a] == 2)
        {
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
            sem_destroy(&sem1);
            sem_destroy(&sem2);
        }
        else
        {
            printf("当前厨房正在等待订单");
            sleep(3);
        }
        v(sem_id);
    }
    
}