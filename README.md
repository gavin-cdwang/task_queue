# task_queue
自己实现的轻量级任务队列（线程池），使用pthread.h提供的API实现遵循POSIX标准。使用简单。

###头文件
task_queue.h

###用法
####1.先创建任务队列
```
struct tq_queue *queue = NULL;
queue = tq_create_queue(10);
if(NULL == queue){
    exit(-1);
}
```

####2.创建任务
```
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
```
tq_dispatch_task(queue, task);

```



