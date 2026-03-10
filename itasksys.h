#ifndef _ITASKSYS_H
#define _ITASKSYS_H

#include <vector>

typedef int TaskID;

class IRunnable
{
public:
    virtual ~IRunnable();
    virtual void runTask(int task_id, int num_total_tasks) = 0;
};

class ITaskSystem
{
public:
    ITaskSystem(int num_threads);
    virtual ~ITaskSystem();
    virtual const char* name() = 0;

    virtual void run(IRunnable* runnable, int num_total_tasks) = 0;

    virtual TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                    const std::vector<TaskID>& deps) = 0;

    virtual void sync() = 0;
};

#endif