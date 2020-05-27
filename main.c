//
// Created by acmery on 2020/5/27.
//

#include "threadPool.h"

void testFun(void* arg){
    printf("i = %d\n", *(int *)arg);
}

int main(){
    nThreadPool *pool = (nThreadPool*)malloc(sizeof(nThreadPool));
    int ret = nThreadPoolCreate(pool, 1000);
    if (ret != 1){
        perror("线程池初始化失败");
        printf("ret = %d\n", ret);
        return -1;
    }
    printf("线程池初始化成功\n");
    for (int i = 0; i < 1000; ++i)
        nThreadPush(pool, testFun, &i);

    nThreadPoolDestroy(pool);
}
