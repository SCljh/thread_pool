#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

//利用宏定义实现类似c++的template功能
//使用do - while(0) 是为了解决宏定义时代码拷贝时后面的分号问题
#define LL_ADD(item, list) do {         \
    item->prev = NULL;                  \
    item->next = list;                  \
    if (list != NULL) list->prev = item; \
    list = item;                        \
} while(0)

#define LL_REMOVE(item, list) do {      \
    if (item->prev != NULL) item->prev->next = item->next;  \
    if (item->next != NULL) item->next->prev = item->prev;  \
    if (list == item) list = item->next;                    \
    item->prev = item->next = NULL;                         \
} while (0)

//执行队列
struct NWORKER{
    pthread_t threadid;
    struct NMANAGER *pool;
    int terminate;
    int isWorking;

    struct NWORKER *prev;
    struct NWORKER *next;
};

//任务队列
struct  NJOB{
    void (*func)(void *arg);     //任务函数
    void *user_data;

    struct NJOB *prev;
    struct NJOB *next;
};

//线程与任务管理
struct NMANAGER{
    struct NWORKER *workers;
    struct NJOB *jobs;

    int sum_thread;
    int free_thread;

    pthread_cond_t jobs_cond;           //线程条件等待
    pthread_mutex_t jobs_mutex;         //为任务加锁防止一个任务被两个线程执行
};

typedef  struct NMANAGER nThreadPool;

int nThreadPoolDelWorker(nThreadPool *pool, int num);
int nThreadPoolAddWorker(nThreadPool *pool, int num);

static void *nThreadCallBack(void *arg){
    struct NWORKER *worker = (struct NWORKER*)arg;

    while (1){
        //线程只有两个状态：执行\等待
        pthread_mutex_lock(&worker->pool->jobs_mutex);
        while (worker->pool->jobs == NULL) {
            if (worker->terminate) break;
            pthread_cond_wait(&worker->pool->jobs_cond, &worker->pool->jobs_mutex);
        }
        if (worker->terminate){
            pthread_mutex_unlock(&worker->pool->jobs_mutex);
            break;
        }
        struct NJOB *job = worker->pool->jobs;
        LL_REMOVE(job, worker->pool->jobs);

        pthread_mutex_unlock(&worker->pool->jobs_mutex);

        worker->pool->free_thread--;
        if ((float)(worker->pool->free_thread / worker->pool->sum_thread) > 0.8){
            nThreadPoolDelWorker(worker->pool, worker->pool->sum_thread - worker->pool->free_thread * 2);
        }

        worker->isWorking = 1;
        job->func(job->user_data);
        worker->isWorking = 0;
        worker->pool->free_thread++;
        if ((float)(worker->pool->free_thread / worker->pool->sum_thread) < 0.2){
            nThreadPoolAddWorker(worker->pool, worker->pool->free_thread * 2 - worker->pool->sum_thread);
        }

        free(job->user_data);
        free(job);
    }

    free(worker);
    pthread_exit(NULL);
}

//线程池初始化
int nThreadPoolCreate(nThreadPool *pool, int numWorkers){   //numWorkers:线程数量
    if (numWorkers < 1) numWorkers = 1;
    if (pool == NULL) return -1;
    memset(pool, 0, sizeof(nThreadPool));

    //初始化jobs_cond
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    memcpy(&pool->jobs_cond, &blank_cond, sizeof(pthread_cond_t));

    //初始化jobs_mutex
    pthread_mutex_t blank_muntex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool->jobs_mutex, &blank_muntex, sizeof(pthread_mutex_t));

    //初始化workers
    for (int i = 0; i < numWorkers; ++i){
        struct NWORKER *worker = (struct NWORKER*)malloc(sizeof(struct NWORKER));
        if (worker == NULL){
            perror("malloc");
            return -2;
        }

        memset(worker, 0, sizeof(struct NWORKER));
        worker->pool = pool;

        int ret = pthread_create(&worker->threadid, NULL, nThreadCallBack, worker);
        if (ret){
            perror("pthread_create");
            struct NWORKER *w = pool->workers;
            for (w = pool->workers; w != NULL; w = w->next)
                w->terminate = 1;
            //free(worker);
            return -3;
        }
        worker->terminate = 0;

        LL_ADD(worker, pool->workers);
    }
    pool->sum_thread = numWorkers;
    pool->free_thread = numWorkers;
    return 1;
}

//销毁线程池
void nThreadPoolDestroy(nThreadPool *pool){

    struct NWORKER *worker = NULL;
    for (worker = pool->workers; worker != NULL; worker = worker->next){
        worker->terminate = 1;
    }
    pthread_mutex_lock(&pool->jobs_mutex);
    pthread_cond_broadcast(&pool->jobs_cond);
    pthread_mutex_unlock(&pool->jobs_mutex);

}

//向线程池中添加任务
void nThreadPoolAddJob(nThreadPool *pool, struct NJOB *job){
    //唤醒休眠的线程

    pthread_mutex_lock(&pool->jobs_mutex);
    LL_ADD(job, pool->jobs);
    pthread_cond_signal(&pool->jobs_cond);
    pthread_mutex_unlock(&pool->jobs_mutex);

}

//面向用户的添加任务
int nThreadPush(nThreadPool *pool,void (*func)(void *), void *arg, int len){

    struct NJOB *job = (struct NJOB*)malloc(sizeof(struct NJOB));
    if (job == NULL){
        perror("malloc");
        return -2;
    }

    memset(job, 0, sizeof(struct NJOB));

    job->user_data = malloc(len);
    memcpy(job->user_data, arg, len);
    job->func = func;
    job->next = NULL;
    job->prev = NULL;


    nThreadPoolAddJob(pool, job);

    return 1;
}

//线程池增加线程
int nThreadPoolAddWorker(nThreadPool *pool, int num) {
    int ret = 0;
    for (int i = 0; i < num; ++i) {
        struct NWORKER *worker = (struct NWORKER *) malloc(sizeof(struct NWORKER));
        if (worker == NULL) {
            perror("malloc");
            return -2;
        }

        memset(worker, 0, sizeof(struct NWORKER));
        worker->pool = pool;

        int ret = pthread_create(&worker->threadid, NULL, nThreadCallBack, worker);
        if (ret) {
            printf("error: pthread_create\n");
            struct NWORKER *w = pool->workers;
            for (w = pool->workers; w != NULL; w = w->next)
                w->terminate = 1;
            //free(worker);
            break;
        }
        worker->terminate = 0;

        LL_ADD(worker, pool->workers);

        ret++;
    }
    pool->sum_thread+=ret;
    return ret;
}

//线程池减少线程
int nThreadPoolDelWorker(nThreadPool *pool, int num){
    if (num > pool->sum_thread) return -1;
    int ret = 0;
    struct NWORKER *worker = NULL;
    for (worker = pool->workers; worker != NULL && ret < num; worker = worker->next){
        if (!worker->isWorking)
            worker->terminate = 1;
        ret++;
    }
    pool->sum_thread -= ret;
    return ret;
}