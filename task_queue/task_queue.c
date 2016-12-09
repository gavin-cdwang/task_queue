//
//  task_queue.c
//  task_queue
//
//  Created by scott on 2016/12/7.
//  Copyright © 2016年 scott. All rights reserved.
//

#include "task_queue.h"

// 打印线程调度信息
void _print_thread_info()
{
    int policy;
    struct sched_param param;

    pthread_getschedparam(pthread_self(),&policy,&param);

    switch (policy){
        case SCHED_OTHER:
            printf("SCHED_OTHER\n");
            break;
        case SCHED_RR:
            printf("SCHED_RR\n");
            break;
        case SCHED_FIFO:
            printf("SCHED_FIFO\n");
            break;
        default:
            break;
    }
}

static void *_worker_thread_routine(void *arg)
{
   // _print_thread_info();
    
    // consumer
    struct tq_queue *queue = (struct tq_queue *)arg;
    
    for(;;){
        printf("THREAD:%x; FUNC:%s; INFO:%s\n",(int )pthread_self(),__func__,"I am a thread");

        pthread_mutex_lock(&queue->mutex);
        
        if(!queue->keep_alive){
            // NOTE: on macOS env; pthread_t is a struct so maybe WARNING;
            
            printf("THREAD:%x; FUNC:%s; INFO:%s\n",(int )pthread_self(),__func__,"queue do not keep_alive");
            
            pthread_mutex_unlock(&queue->mutex);
            break;
        }
    
        while(NULL == queue->fst){
            pthread_cond_signal(&queue->empty);
            pthread_cond_wait(&queue->has_task, &queue->mutex);
            // 最后销毁队列时回broadcast 这里收到处理退出。
            if(!queue->accept_new){
                pthread_mutex_unlock(&queue->mutex);
                pthread_exit(NULL);
            }
        }
        
        struct tq_task *task = queue->fst;
        if(queue->fst || queue->tail){
             queue->fst = queue->fst->next;
        }
       
        queue->cur_task_num--;
        
        pthread_mutex_unlock(&queue->mutex);
        
        // now process the task
        task->run(task->arg);
        
        // finish processing
        tq_destroy_task(task);
        task = NULL;
    }

    pthread_exit(NULL);
}


// TODO: assign a "queue_id" to make queue unique ;better to use HASH to make a unique quque_id

/**
 use this to create a task queue

 @param worker_thread_num  to create X worker threads

 @return the task queue you just created
 */

struct tq_queue *tq_create_queue(u_int worker_thread_num)
{
    struct tq_queue *queue = NULL;
    
    queue = (struct tq_queue *)malloc(sizeof(struct tq_queue));
    if(NULL == queue){
        fprintf(stderr, "FUNC:%s; failed to alloc mem for queue\n",__func__);
        return NULL;
    }
    
    memset(queue,0,sizeof(struct tq_queue));

    queue->worker_thread_num = worker_thread_num;

    if(pthread_mutex_init(&queue->mutex, NULL)){
        fprintf(stderr, "FUNC:%s; failed to init mutex\n",__func__);
        return NULL;
    }
    
    if(pthread_cond_init(&queue->has_task, NULL)){
        fprintf(stderr, "FUNC:%s; failed to init cond has_task\n",__func__);
        return NULL;
    }
    
    if(pthread_cond_init(&queue->empty, NULL)){
        fprintf(stderr, "FUNC:%s; failed to init cond empty\n",__func__);
        return NULL;
    }
    
    
    queue->fst = queue->tail = NULL;
    queue->cur_task_num = 0;
    queue->keep_alive = 1;
    queue->accept_new = 1;

    queue->worker_thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * worker_thread_num);
    if(NULL == queue->worker_thread_ids){
        fprintf(stderr, "FUNC:%s; failed to alloc mem for worker_thread_num\n",__func__);
        return NULL;
    }
    
    int i;
    pthread_attr_t *attr = NULL;
    for(i = 0; i < queue->worker_thread_num; i++){
        if(pthread_create(&queue->worker_thread_ids[i], attr, _worker_thread_routine,queue)){
            fprintf(stderr,"FUNC:%s; failed to create thread\n",__func__);
            return NULL;
        }
    }
    
    return queue;
}


/**
 use this to create a task with a func name and an arg

 @param start_routine name of func  to run

 @return the task you just created
 */
struct tq_task *tq_create_task(void *( *start_routine)(void *),void *arg)
{
    struct tq_task *task = NULL;
    
    task = (struct tq_task *)malloc(sizeof(struct tq_task));
    if(NULL == task){
        fprintf(stderr, "FUNC:%s; failed to alloc mem for task\n",__func__);
        return NULL;
    }
    
    memset(task, 0, sizeof(struct tq_task));
    task->run = start_routine;
    task->arg = arg;
    task->next = NULL;
    
    return task;
}


/**
 use this to dispatch a task;the func will add you task into the task queue;
 so you task will be run in thread;

 @param queue the queue you want to add task into
 @param task  the task you want to add

 @return OK-0 ERR-1
 */
int tq_dispatch_task(struct tq_queue *queue, struct tq_task *task)
{
    // producer
    if(NULL == queue){
        fprintf(stderr, "FUNC:%s; task queue is NULL\n",__func__);
        return -1;
    }
    
    if(NULL == task){
        fprintf(stderr, "FUNC:%s; task is NULL\n",__func__);
        return -2;

    }
    pthread_mutex_lock(&queue->mutex);
    
    if(!queue->keep_alive){
        fprintf(stderr, "FUNC:%s; task queue do not keep alive\n",__func__);
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    if(!queue->accept_new){
        fprintf(stderr, "FUNC:%s; task queue do not accept new task\n",__func__);
        pthread_mutex_unlock(&queue->mutex);
        return -3;
    }
    
    if(!queue->fst || !queue->tail){
        queue->fst = queue->tail = task;
    }else{
        queue->tail->next = task;
        queue->tail = queue->tail->next;
    }
    queue->tail->next = NULL;
    
    queue->cur_task_num++;
    
    if(NULL != queue->fst){
        fprintf(stderr,"FUNC:%s; signal queue->has_task\n",__func__);
        pthread_cond_signal(&queue->has_task);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}


/**
 destroy a task

 @param task task obj

 @return OK-0 ERR-!0
 */
int tq_destroy_task(struct tq_task *task)
{
    // TODO:
    free(task);
    return 0;
}


static void *_destroy_queue(void *arg)
{
//    _print_thread_info();
    struct tq_queue *queue = (struct tq_queue *)arg;
    
    pthread_mutex_lock(&queue->mutex);
    // 等待所有任务完成
    while(queue->cur_task_num > 0){
        pthread_cond_wait(&queue->empty, &queue->mutex);
    }
    
    fprintf(stderr, "FUNC:%s; task queue empty\n",__func__);

    // 设置队列死亡
    queue->keep_alive = 0;
    //唤醒所有线程准备退出
    pthread_cond_broadcast(&queue->has_task);
    pthread_mutex_unlock(&queue->mutex);


    // 线程合并，回收线程资源
    int i;
    for(i = 0; i < queue->worker_thread_num; i++){
        pthread_join(queue->worker_thread_ids[i], NULL);
    }
    
    // free resources
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->has_task);
    pthread_cond_destroy(&queue->empty);
    free(queue);
    
    fprintf(stderr, "FUNC:%s; finish destroy task queue!\n",__func__);
    return (void *)0;
}

/**
 销毁队列，如果队列中还有任务，则等待所有任务只执行完再销毁。并且使得队列不能够再添加任务。

 @param queue <#queue description#>

 @return <#return value description#>
 */
int tq_destroy_queue(struct tq_queue *queue,u_int destroy_time)
{
    // 防止重复销毁
    if(NULL == queue){
        fprintf(stderr, "FUNC:%s; task queue is NULL\n",__func__);
    }
 
    pthread_mutex_lock(&queue->mutex);
    
    if(!queue->keep_alive){
        fprintf(stderr, "FUNC:%s; task queue do not keep alive ever\n",__func__);
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    // 设置队列不接受新任务
    queue->accept_new = 0;
    fprintf(stderr, "FUNC:%s; task queue dont accept new task\n",__func__);
    
    pthread_mutex_unlock(&queue->mutex);
    
    // 销毁任务
    int i;
    pthread_t id;
    pthread_attr_t *attr = NULL;
    
    switch (destroy_time) {
        // 立刻停止所有任务销毁任务队列
        case DESTROY_RIGHT_NOW:
            for(i = 0; i < queue->worker_thread_num; i++){
                pthread_kill(queue->worker_thread_ids[i], SIGKILL);
            }

            pthread_mutex_destroy(&queue->mutex);
            pthread_cond_destroy(&queue->has_task);
            pthread_cond_destroy(&queue->empty);
            
            free(queue);
            fprintf(stderr, "FUNC:%s; finish destroy task queue!\n",__func__);

            break;
        
        // 异步等待所有任务完成再销毁，为了防止主线程阻塞，开辟新线程去释放资源
        case WAIT_ALL_TASKS_FINISHED_ASYNC:
            if(pthread_create(&id, attr, _destroy_queue, queue)){
                fprintf(stderr, "FUNC:%s; failed to create thread\n",__func__);
                return -1;
            }
            break;
            
        // 等待所有任务完成再销毁
        case WAIT_ALL_TASKS_FINISHED_SYNC:
            _destroy_queue(queue);
            break;
            
        default:
            break;
    }
    
    // 这一句无意义，并不能改变外部queue指针指向;
    // queue = NULL;
    
    return 0;
}

