#include "tasksys.h"
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

/* Base Classes */
IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/* Serial */
TaskSystemSerial::TaskSystemSerial(int num_threads) : ITaskSystem(num_threads) {}
TaskSystemSerial::~TaskSystemSerial() {}

const char* TaskSystemSerial::name() { return "Serial"; }

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks)
{
    for (int i = 0; i < num_total_tasks; i++)
        runnable->runTask(i, num_total_tasks);
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable*, int, const std::vector<TaskID>&) { return 0; }
void TaskSystemSerial::sync() {}

/* Parallel Spawn */
TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads)
    : ITaskSystem(num_threads), num_threads(num_threads) {}
TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

const char* TaskSystemParallelSpawn::name() { return "Parallel Spawn"; }

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks)
{
    std::vector<std::thread> workers;
    for (int t = 0; t < num_threads; t++)
    {
        workers.emplace_back([=]()
        {
            for (int i = t; i < num_total_tasks; i += num_threads)
                runnable->runTask(i, num_total_tasks);
        });
    }

    for (auto& t : workers)
        t.join();
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable*, int, const std::vector<TaskID>&) { return 0; }
void TaskSystemParallelSpawn::sync() {}

/* Thread Pool Spinning */
TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads)
    : ITaskSystem(num_threads), num_threads(num_threads), stop(false), nextTask(0), totalTasks(0), currentRunnable(nullptr)
{
    for (int i = 0; i < num_threads; i++)
    {
        workers.emplace_back([this]()
        {
            while (!stop)
            {
                int task = nextTask.fetch_add(1);
                if (task < totalTasks)
                    currentRunnable->runTask(task, totalTasks);
            }
        });
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning()
{
    stop = true;
    for (auto& t : workers)
        t.join();
}

const char* TaskSystemParallelThreadPoolSpinning::name() { return "Parallel ThreadPool Spinning"; }

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks)
{
    currentRunnable = runnable;
    totalTasks = num_total_tasks;
    nextTask = 0;

    while (nextTask < totalTasks)
        ;
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable*, int, const std::vector<TaskID>&) { return 0; }
void TaskSystemParallelThreadPoolSpinning::sync() {}

/* Thread Pool Sleeping */
TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
    : ITaskSystem(num_threads), num_threads(num_threads), stop(false), nextTask(0), finishedTasks(0), totalTasks(0)
{
    for (int i = 0; i < num_threads; i++)
    {
        workers.emplace_back([this]()
        {
            while (true)
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this]() { return nextTask < totalTasks || stop; });

                if (stop)
                    return;

                int task = nextTask++;
                lock.unlock();

                currentRunnable->runTask(task, totalTasks);

                lock.lock();
                finishedTasks++;
                if (finishedTasks == totalTasks)
                    finished.notify_one();
            }
        });
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
    }

    cv.notify_all();
    for (auto& t : workers)
        t.join();
}

const char* TaskSystemParallelThreadPoolSleeping::name() { return "Parallel ThreadPool Sleeping"; }

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks)
{
    std::unique_lock<std::mutex> lock(mtx);
    currentRunnable = runnable;
    totalTasks = num_total_tasks;
    nextTask = 0;
    finishedTasks = 0;
    cv.notify_all();
    finished.wait(lock, [this]() { return finishedTasks == totalTasks; });
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable*, int, const std::vector<TaskID>&) { return 0; }
void TaskSystemParallelThreadPoolSleeping::sync() {}