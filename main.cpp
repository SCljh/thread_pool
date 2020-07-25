//
// Created by acmery on 2020/5/27.
//

#include "threadpool.h"
#include <iostream>

void testFun(void* arg){
    printf("i = %d\n", *(int *)arg);
    //std::cout << "i = " << *(int *)arg << std::endl;
}

int main(){
    nThreadPool *pool = (nThreadPool*)malloc(sizeof(nThreadPool));
    int ret = nThreadPoolCreate(pool, 1);
    if (ret != 1){
        perror("线程池初始化失败");
        printf("ret = %d\n", ret);
        return -1;
    }
    printf("线程池初始化成功\n");
    int i = 0;
    for (i = 0; i < 1000; ++i) {
        nThreadPush(pool, testFun, &i, sizeof(int));
    }

    nThreadPoolDestroy(pool);
}
