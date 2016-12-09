//
//  main.c
//  task_queue
//
//  Created by scott on 2016/12/7.
//  Copyright © 2016年 scott. All rights reserved.
//

#include <stdio.h>
#include "task_queue.h"

#define FOREVER_RUN while (1) {}

void *run(void *arg)
{
    int count = 0;
    for (; ; ) {
        printf("Thread:%x; FUNC:%s; %s ;%d\n",(int)pthread_self(),__func__,(char *)arg,count);
        sleep(1);
        count++;
        if(count == 5){
            break;
        }
    }
    return (void *)0;
}

int main(int argc, const char * argv[]) {
    
    struct tq_queue *queue = NULL;
    queue = tq_create_queue(10);
    if(NULL == queue){
        exit(-1);
    }
    
    struct tq_task *task0 = tq_create_task(run, "test0");
    struct tq_task *task1 = tq_create_task(run, "test1");
    struct tq_task *task2 = tq_create_task(run, "test2");
    struct tq_task *task3 = tq_create_task(run, "test3");
    struct tq_task *task4 = tq_create_task(run, "test4");

    tq_dispatch_task(queue, task0);
    tq_dispatch_task(queue, task1);
    tq_dispatch_task(queue, task2);
//    tq_destroy_queue(queue,WAIT_ALL_TASKS_FINISHED_ASYNC);
    tq_destroy_queue(queue,WAIT_ALL_TASKS_FINISHED_SYNC);
//    tq_destroy_queue(queue, DESTROY_RIGHT_NOW);
    printf("main !!\n");
    tq_dispatch_task(queue, task3);
    tq_dispatch_task(queue, task4);

    FOREVER_RUN
    
    return 0;
}
