#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <thread>
#include "tasksys.h"

/**
 * ComputeTask: A workload implementation of IRunnable.
 * It simulates a heavy CPU task using trigonometric calculations.
 */
class ComputeTask : public IRunnable {
public:
    std::vector<double> results;
    int workload_intensity;

    ComputeTask(int num_tasks, int intensity) 
        : results(num_tasks, 0.0), workload_intensity(intensity) {}

    void runTask(int taskID, int num_total_tasks) override {
        double val = 0.0;
        for (int i = 0; i < workload_intensity; ++i) {
            val += std::sin(i * 0.01 + taskID) * std::cos(i * 0.02 + taskID);
        }
        results[taskID] = val;
    }
};

/**
 * runBenchmark: Helper function to measure execution time of a specific task system.
 */
void runBenchmark(ITaskSystem* system, IRunnable* task, int num_tasks, const std::string& name) {
    std::cout << "Testing [" << name << "]..." << std::flush;
    
    auto start = std::chrono::high_resolution_clock::now();
    system->run(task, num_tasks);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;
    std::cout << " Done. Time: " << std::fixed << std::setprecision(4) << elapsed.count() << "s" << std::endl;
}

int main() {
    // Configuration
    int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;
    
    int num_tasks = 10000;
    int workload_intensity = 100;

    std::cout << "========================================" << std::endl;
    std::cout << "Task System Benchmark" << std::endl;
    std::cout << "Threads: " << num_threads << ", Tasks: " << num_tasks << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Serial System
    ComputeTask task1(num_tasks, workload_intensity);
    ITaskSystem* serialSystem = new TaskSystemSerial(num_threads);
    runBenchmark(serialSystem, &task1, num_tasks, "Serial System");
    delete serialSystem;

    // 2. Parallel Spawn (Creates/Destroys threads per run)
    ComputeTask task2(num_tasks, workload_intensity);
    ITaskSystem* spawnSystem = new TaskSystemParallelSpawn(num_threads);
    runBenchmark(spawnSystem, &task2, num_tasks, "Parallel Spawn");
    delete spawnSystem;

    // 3. ThreadPool Spinning (Threads stay alive, use busy-waiting)
    ComputeTask task3(num_tasks, workload_intensity);
    ITaskSystem* spinningSystem = new TaskSystemParallelThreadPoolSpinning(num_threads);
    runBenchmark(spinningSystem, &task3, num_tasks, "Parallel ThreadPool Spinning");
    delete spinningSystem;

    // 4. ThreadPool Sleeping (Threads use condition variables to sleep)
    ComputeTask task4(num_tasks, workload_intensity);
    ITaskSystem* sleepingSystem = new TaskSystemParallelThreadPoolSleeping(num_threads);
    runBenchmark(sleepingSystem, &task4, num_tasks, "Parallel ThreadPool Sleeping");
    delete sleepingSystem;

    return 0;
}