# task_queue
自己实现的轻量级任务队列（线程池），使用pthread.h提供的API实现遵循POSIX标准。使用简单。

###头文件
task_queue.h

###用法
####1.先创建任务队列
```C
struct tq_queue *queue = NULL;
queue = tq_create_queue(10);
if(NULL == queue){
    exit(-1);
}
```

####2.创建任务
```C
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

struct tq_task *task = tq_create_task(run, "test_task");

```

####3.调度任务
```C
tq_dispatch_task(queue, task);

```

####4.销毁队列
//异步方式销毁队列，销毁时会等待任务队列中所有任务都完成，但不会阻塞主线程
tq_destroy_queue(queue,WAIT_ALL_TASKS_FINISHED_ASYNC);
//销毁队列，销毁时会等待任务队列中所有任务都完成，会阻塞主线程
tq_destroy_queue(queue,WAIT_ALL_TASKS_FINISHED_SYNC);

####5.注意
当调用tq_destroy_queue销毁队列后，再往队列中添加任务会遭到拒绝。返回非0值。




