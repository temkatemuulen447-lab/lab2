#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

/* Serial Task System */
class TaskSystemSerial : public ITaskSystem
{
public:
    TaskSystemSerial(int num_threads);
    ~TaskSystemSerial();
    const char* name();
    void run(IRunnable* runnable, int num_total_tasks);
    TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                            const std::vector<TaskID>& deps);
    void sync();
};

/* Parallel Spawn */
class TaskSystemParallelSpawn : public ITaskSystem
{
private:
    int num_threads;

public:
    TaskSystemParallelSpawn(int num_threads);
    ~TaskSystemParallelSpawn();
    const char* name();
    void run(IRunnable* runnable, int num_total_tasks);
    TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                            const std::vector<TaskID>& deps);
    void sync();
};

/* Thread Pool + Spinning */
class TaskSystemParallelThreadPoolSpinning : public ITaskSystem
{
private:
    int num_threads;
    std::vector<std::thread> workers;
    std::atomic<int> nextTask;
    int totalTasks;
    IRunnable* currentRunnable;
    std::atomic<bool> stop;

public:
    TaskSystemParallelThreadPoolSpinning(int num_threads);
    ~TaskSystemParallelThreadPoolSpinning();
    const char* name();
    void run(IRunnable* runnable, int num_total_tasks);
    TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                            const std::vector<TaskID>& deps);
    void sync();
};

/* Thread Pool + Sleeping */
class TaskSystemParallelThreadPoolSleeping : public ITaskSystem
{
private:
    int num_threads;
    std::vector<std::thread> workers;
    IRunnable* currentRunnable;
    int totalTasks;
    int nextTask;
    int finishedTasks;
    std::mutex mtx;
    std::condition_variable cv;
    std::condition_variable finished;
    bool stop;

public:
    TaskSystemParallelThreadPoolSleeping(int num_threads);
    ~TaskSystemParallelThreadPoolSleeping();
    const char* name();
    void run(IRunnable* runnable, int num_total_tasks);
    TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                            const std::vector<TaskID>& deps);
    void sync();
};

#endif