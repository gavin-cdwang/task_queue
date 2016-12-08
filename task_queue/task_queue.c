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
            //TODO:print
            break;
        }
    
        while(NULL == queue->fst){
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        
        struct tq_task *task = queue->fst;
        queue->fst = queue->fst->next;
        
        pthread_mutex_unlock(&queue->mutex);
        
        //now process the task
        task->run(task->arg);
        free(task);
        task = NULL;
    }

    return (void *)0;
}

//TODO: assign a "queue_id" to make queue unique
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

int tq_dispatch_task(struct tq_queue *queue, struct tq_task *task)
{
    //producer
    pthread_mutex_lock(&queue->mutex);
    
    if(!queue->fst || !queue->tail){
        queue->fst = queue->tail = task;
    }else{
        queue->tail->next = task;
        queue->tail = queue->tail->next;
    }
    queue->tail->next = NULL;
    queue->cur_task_num++;
    
    pthread_mutex_unlock(&queue->mutex);
    
    if(NULL != queue->fst){
        // TBD: WHICH ONE ?
        fprintf(stderr,"FUNC:%s; cond signal\n",__func__);
        pthread_cond_signal(&queue->cond);
        //pthread_cond_broadcast(&queue->cond);
    }
    

    
    return 0;
}

int tq_destroy_task(struct tq_queue *task)
{
    //TODO:
    free(task);
    return 0;
}

int tq_destroy_queue(struct tq_queue *queue)
{
    //TODO:
    return 0;
}

