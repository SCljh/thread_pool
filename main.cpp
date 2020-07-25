//
// Created by acmery on 2020/5/27.
//

#define CPP_TEST

#include <stdio.h>

#ifndef CPP_TEST

#include "threadpool.h"

void testFun(void* arg){
    printf("i = %d\n", *(int *)arg);
    //注意：std::cout非线程安全
    //std::cout << "i = " << *(int *)arg << std::endl;
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
    int i = 0;
    for (i = 0; i < 1000; ++i) {
        nThreadPush(pool, testFun, &i, sizeof(int));
    }

    nThreadPoolDestroy(pool);
}

#else

#include "threadpool.hpp"

void testFun(void* arg){
    printf("i = %d\n", *(int *)arg);
}


int main(){
    ThreadPool *pool = new ThreadPool(1000, 2000);

    printf("线程池初始化成功\n");
    int i = 0;
    for (i = 0; i < 1000; ++i) {
        pool->pushJob(testFun, &i, sizeof(int));
    }
}

#endif