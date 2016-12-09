//
//  task_queue.h
//  task_queue
//
//  Created by scott on 2016/12/7.
//  Copyright © 2016年 scott. All rights reserved.
//

#ifndef task_queue_h
#define task_queue_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
typedef void *(*start_routine)(void  *);


/**
 任务结构体
 */
struct tq_task {
    start_routine run;             //任务实体
    void *arg;                      //参数
    struct tq_task *next;
};



/**
 任务队列结构体 (为了降低复杂度，结构体设计的比较简单)
 */
struct tq_queue {
    u_int worker_thread_num;        //线程队列中线程数
    u_int max_task_num;             //最大任务数
    u_int cur_task_num;             //当前任务数
    pthread_t *worker_thread_ids;   //记录工作线程的所有 id
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    struct tq_task *fst;            //任务队列头指针
    struct tq_task *tail;           //任务队列尾指针
    u_int keep_alive;               //当前队列是否存活
};

extern struct tq_queue *tq_create_queue(u_int max_task_num,u_int worker_thread_num);
extern struct tq_task *tq_create_task(void *( *start_routine)(void *),void *arg);
extern int tq_dispatch_task(struct tq_queue *queue, struct tq_task *task);
extern int tq_destroy_task(struct tq_queue *task);
extern int tq_destroy_queue(struct tq_queue *queue);

#endif /* task_queue_h */
