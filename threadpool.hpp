//
// Created by acmery on 2020/6/29.
//

#ifndef TINYSERVER_THREADPOOL_HPP
#define TINYSERVER_THREADPOOL_HPP

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <list>

#include <pthread.h>

class ThreadPool{
private:
    struct NWORKER{
        pthread_t threadid;
        bool terminate;
        int isWorking;
        ThreadPool *pool;
    } *m_workers;

    struct NJOB{
        void (*func)(void *arg);     //任务函数
        void *user_data;
    };
public:
    //线程池初始化
    //numWorkers:线程数量
    ThreadPool(int numWorkers, int max_jobs);
    //销毁线程池
    ~ThreadPool();
    //面向用户的添加任务
    int pushJob(void (*func)(void *data), void *arg, int len);

private:
    //向线程池中添加任务
    bool _addJob(NJOB* job);
    static void* _run(void *arg);
    void _threadLoop(void *arg);

private:
    std::list<NJOB*> m_jobs_list;
    int m_max_jobs;
    int m_sum_thread;
    int m_free_thread;
    pthread_cond_t m_jobs_cond;           //线程条件等待
    pthread_mutex_t m_jobs_mutex;         //为任务加锁防止一个任务被两个线程执行
};

ThreadPool::ThreadPool(int numWorkers, int max_jobs = 10) : m_sum_thread(numWorkers), m_free_thread(numWorkers), m_max_jobs(max_jobs){   //numWorkers:线程数量
    if (numWorkers < 1 || max_jobs < 1){
        perror("workers num error");
    }
    //初始化jobs_cond
    if (pthread_cond_init(&m_jobs_cond, NULL) != 0)
        perror("init m_jobs_cond fail\n");

    //初始化jobs_mutex
    if (pthread_mutex_init(&m_jobs_mutex, NULL) != 0)
        perror("init m_jobs_mutex fail\n");

    //初始化workers
    m_workers = new NWORKER[numWorkers];
    if (!m_workers){
        perror("create workers failed!\n");
    }

    for (int i = 0; i < numWorkers; ++i){

        m_workers[i].pool = this;

        int ret = pthread_create(&(m_workers[i].threadid), NULL, _run, &m_workers[i]);
        if (ret){
            delete[] m_workers;
            perror("create worker fail\n");
        }
        if (pthread_detach(m_workers[i].threadid)){
            delete[] m_workers;
            perror("detach worder fail\n");
        }
        m_workers[i].terminate = 0;
    }
}

ThreadPool::~ThreadPool(){
    for (int i = 0; i < m_sum_thread; i++){
        m_workers[i].terminate = 1;
    }
    pthread_mutex_lock(&m_jobs_mutex);
    pthread_cond_broadcast(&m_jobs_cond);
    pthread_mutex_unlock(&m_jobs_mutex);
    delete[] m_workers;
}

//向线程池中添加任务
bool ThreadPool::_addJob(NJOB *job) {
    pthread_mutex_lock(&m_jobs_mutex);
    if (m_jobs_list.size() >= m_max_jobs){
        pthread_mutex_unlock(&m_jobs_mutex);
        return false;
    }
    m_jobs_list.push_back(job);
    //唤醒休眠的线程
    pthread_cond_signal(&m_jobs_cond);
    pthread_mutex_unlock(&m_jobs_mutex);

}

//面向用户的添加任务
int ThreadPool::pushJob(void (*func)(void *), void *arg, int len) {
    struct NJOB *job = (struct NJOB*)malloc(sizeof(struct NJOB));
    if (job == NULL){
        perror("malloc");
        return -2;
    }

    memset(job, 0, sizeof(struct NJOB));

    job->user_data = malloc(len);
    memcpy(job->user_data, arg, len);
    job->func = func;

    _addJob(job);

    return 1;
}

void* ThreadPool::_run(void *arg) {
    NWORKER *worker = (NWORKER *)arg;
    worker->pool->_threadLoop(arg);
}

void ThreadPool::_threadLoop(void *arg) {
    NWORKER *worker = (NWORKER*)arg;
    while (1){
        //线程只有两个状态：执行\等待
        pthread_mutex_lock(&m_jobs_mutex);
        while (m_jobs_list.size() == 0) {
            if (worker->terminate) break;
            pthread_cond_wait(&m_jobs_cond,&m_jobs_mutex);
        }
        if (worker->terminate){
            pthread_mutex_unlock(&m_jobs_mutex);
            break;
        }
        struct NJOB *job = m_jobs_list.front();
        m_jobs_list.pop_front();

        pthread_mutex_unlock(&m_jobs_mutex);

        m_free_thread--;
        worker->isWorking = true;
        job->func(job->user_data);
        worker->isWorking = false;

        free(job->user_data);
        free(job);
    }

    free(worker);
    pthread_exit(NULL);
}

#endif //TINYSERVER_THREADPOOL_HPP
