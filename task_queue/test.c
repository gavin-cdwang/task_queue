//
//  main.c
//  task_queue
//
//  Created by scott on 2016/12/7.
//  Copyright © 2016年 scott. All rights reserved.
//

#include <stdio.h>
#include "task_queue.h"

void *run(void *arg)
{

    for (; ; ) {
        printf("Thread:%x; FUNC:%s; %s\n",pthread_self(),__func__,(char *)arg);
//        sleep(1);
    }
}

int main(int argc, const char * argv[]) {
    
    struct tq_queue *queue = NULL;
    queue = tq_create_queue(20, 10);
    if(NULL == queue){
        exit(-1);
    }
    
    struct tq_task *task0 = tq_create_task(run, "test0");
    struct tq_task *task1 = tq_create_task(run, "test1");
    struct tq_task *task2 = tq_create_task(run, "test2");
    struct tq_task *task3 = tq_create_task(run, "test3");
    struct tq_task *task4 = tq_create_task(run, "test4");
    struct tq_task *task5 = tq_create_task(run, "test5");
    struct tq_task *task6 = tq_create_task(run, "test6");

    tq_dispatch_task(queue, task0);
    tq_dispatch_task(queue, task1);
    tq_dispatch_task(queue, task2);
    tq_dispatch_task(queue, task3);
    tq_dispatch_task(queue, task4);
    tq_dispatch_task(queue, task5);
    tq_dispatch_task(queue, task6);

    while (1) {
        
    }
    return 0;
}
