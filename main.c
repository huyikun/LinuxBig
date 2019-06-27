#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/sem.h>

#define SEGSIZE 4096
typedef struct node {
    int* t[5] ;
    int a ;
    int b ;
    int _exit;
}Node;
Node * smap;
void func1() {
    if(smap->b == smap->a) {
        printf("订单已满");
        return;
    } else if(smap->a == -1){
        smap->a = 0;
        smap->t[smap->b++] = 1;
    } else {
        smap->t[smap->b++] = 1;
        smap->b %= 5;
    }
}
void func2() {
    if(smap->b == smap->a) {
        printf("订单已满");
        return;
    } else if(smap->a == -1){
        smap->a = 0;
        smap->t[smap->b++] = 2;
    } else {
        smap->t[smap->b++] = 2;
        smap->b %= 5;
    }
}
void funcexit() {
    signal(SIGUSR1,SIG_IGN);
    signal(SIGUSR2,SIG_IGN);
    printf("现在开始不接受订单");

}
int main()
{
    int i, shmid;
    pid_t p ;
    key_t k = ftok(".",0);
    if(k == -1 ) {
        perror("ftok error");
        return -1;
    }
    shmid = shmget(k,SEGSIZE,IPC_CREAT|IPC_EXCL|0664);
    if(shmid == -1) {
        perror("shmget error");
        return -1;
    }  
    smap = (Node *)shmat(shmid,NULL, 0);
    smap->a = -1 ;
    smap->b = 0 ;
    smap->_exit = 0;                                // 共享内存初始化完成

    signal(SIGUSR1,func1);
    signal(SIGUSR2,func2);
    for(int i = 0 ;i < 2; i++) {
        if(( p = fork ) == 0) {
            break;
        }
        printf("chef %d's pid = %d\n",i,p);
    }
    if(i == 0) {
        execl("chef2.c",NULL);
    } else if(i == 1) {
        execl("chef2.c",NULL);
    }
    signal(SIGINT,funcexit);

    if(shmdt(smap) == -1) {
        perror("解除绑定错误");
        return -1;
    }
    shmctl(shmid, IPC_RMID,0);
}