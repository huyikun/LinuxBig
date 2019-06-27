#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#define SEGSIZE 4096
typedef struct node {
    int* t[5] ;
    int a ;
    int b ;
    int _exit;
}Node;
char * menu[2] = {"pork", "beef"};
Node * smap;
pthread_mutex_t m;
void func1() {
    pthread_mutex_lock(&m);
    int index = smap->t[smap->a];
    printf("烹饪阶段：厨师%d开始炒 %s \n",  getpid(), menu[index]);
    sleep(3);
    printf("烹饪阶段：厨师%d 炒 %s 完成\n",  getpid(), menu[index]);
    smap->t[smap->a++] = 0;
    smap->a %= 5;
    pthread_mutex_unlock(&m);
}
int main() 
{
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
    pthread_mutex_init(&m, NULL);
    pthread_t tid1;
    if (pthread_create(&tid1, NULL, func1, NULL) != 0)
    {
        perror("chef2 线程失败");
    }
    pthread_join(tid1);
    if(smap->_exit == 1 && smap->t[smap->a] == 0 ){
        exit(1);
    }
}