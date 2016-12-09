//
//  task_queue.c
//  task_queue
//
//  Created by scott on 2016/12/7.
//  Copyright © 2016年 scott. All rights reserved.
//

#include "task_queue.h"

void *worker_thread_routine(void *arg)
{
    //consumer
    struct tq_queue *queue = (struct tq_queue *)arg;
    
    for(;;){
        
        pthread_mutex_lock(&queue->mutex);
        
        if(!queue->keep_alive){
            //NOTE: on macOS env; pthread_t is a struct so maybe WARNING;
            printf("THREAD:%x; FUNC:%s; INFO:%s\n",pthread_self(),__func__,"queue do not keep_alive");
            
            pthread_mutex_unlock(&queue->mutex);
            break;
        }
    
        while(NULL == queue->fst){
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        
        struct tq_task *task = queue->fst;
        queue->fst = queue->fst->next;
        queue->cur_task_num--;
        
        pthread_mutex_unlock(&queue->mutex);
        
        //now process the task
        task->run(task->arg);
        free(task);
        task = NULL;
    }

    return (void *)0;
}


//TODO: assign a "queue_id" to make queue unique ;better to use HASH to make a unique quque_id

/**
 use this to create a task queue

 @param max_task_num      max tasks
 @param worker_thread_num  to create X worker threads

 @return the task queue you just created
 */

struct tq_queue *tq_create_queue(u_int max_task_num,u_int worker_thread_num)
{
    struct tq_queue *queue = NULL;
    
    queue = (struct tq_queue *)malloc(sizeof(struct tq_queue));
    if(NULL == queue){
        fprintf(stderr, "FUNC:%s; failed to alloc mem for queue\n",__func__);
        return NULL;
    }
    
    memset(queue,0,sizeof(struct tq_queue));
    
    queue->max_task_num = max_task_num;
    queue->worker_thread_num = worker_thread_num;

    if(pthread_mutex_init(&queue->mutex, NULL)){
        fprintf(stderr, "FUNC:%s; failed to init mutex\n",__func__);
        return NULL;
    }
    
    if(pthread_cond_init(&queue->cond, NULL)){
        fprintf(stderr, "FUNC:%s; failed to init cond\n",__func__);
        return NULL;
    }
    
    queue->fst = queue->tail = NULL;
    queue->cur_task_num = 0;
    queue->keep_alive = 1;

    queue->worker_thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * worker_thread_num);
    if(NULL == queue->worker_thread_ids){
        fprintf(stderr, "FUNC:%s; failed to alloc mem for worker_thread_num\n",__func__);
        return NULL;
    }
    
    int i;
    pthread_attr_t *attr = NULL;
    for(i = 0; i < queue->worker_thread_num; i++){
        if(pthread_create(&queue->worker_thread_ids[i], attr, worker_thread_routine,queue)){
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
    //producer
    pthread_mutex_lock(&queue->mutex);
    
    if(!queue->keep_alive){
        fprintf(stderr, "queue never keep alive\n");
        return -1;
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
        // TBD: WHICH ONE ?
        //fprintf(stderr,"FUNC:%s; cond signal\n",__func__);
        pthread_cond_signal(&queue->cond);
        //pthread_cond_broadcast(&queue->cond);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}


/**
 destroy a task

 @param task task obj

 @return OK-0 ERR-!0
 */
int tq_destroy_task(struct tq_queue *task)
{
    //TODO:
    free(task);
    return 0;
}


/**
 销毁队列，如果队列中还有任务，则等待所有任务只执行完再销毁。并且使得队列不能够再添加任务。

 @param queue <#queue description#>

 @return <#return value description#>
 */
int tq_destroy_queue(struct tq_queue *queue)
{
    //TODO:
    pthread_mutex_lock(&queue->mutex);
    
    //防止重复销毁
    if(!queue->keep_alive){
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }
    
    queue->keep_alive = 0;
    
    while(queue->cur_task_num > 0){
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    
        //FREE
    
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

