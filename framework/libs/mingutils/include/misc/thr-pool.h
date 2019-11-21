#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <list>
#include <vector>
#include <algorithm>

#define THRPOOL_INITIALIZED 1
#define THRPOOL_MAX_THREADS 1024
#define TASKTHREAD_FLAG_CREATED 1
#define TASKTHREAD_FLAG_SHOULD_EXIT 2
#define TASK_OP_QUEUE 0
#define TASK_OP_DOING 1
#define TASK_OP_DONE 2
#define TASK_OP_DISCARD 3

class ThreadPool {
private:
  typedef std::function<void()> TaskFunc;
  /// \param op  0: queue  1: doing  2: done  3: discard
  typedef std::function<void(int32_t op)> TaskCallback;

  class TaskInfo {
  public:
    TaskInfo() {
    }

    TaskInfo(TaskFunc f, TaskCallback c) : func(f), cb(c) {
    }

    TaskFunc func;
    TaskCallback cb;
  };

  class TaskThread {
  public:
    TaskThread() {
    }

    TaskThread(const TaskThread& o) {
    }

    void setThreadPool(ThreadPool *pool) {
      thePool = pool;
    }

    void awake() {
      std::unique_lock<std::mutex> locker(thrMutex);
      if ((flags & TASKTHREAD_FLAG_CREATED) == 0
          && (flags & TASKTHREAD_FLAG_SHOULD_EXIT) == 0) {
        // init
        thr = std::thread([this]() { run(); });
        flags |= TASKTHREAD_FLAG_CREATED;
      }
    }

    void work() {
      std::lock_guard<std::mutex> locker(thrMutex);
      thrCond.notify_one();
    }

    void sleep() {
      std::unique_lock<std::mutex> locker(thrMutex);
      flags |= TASKTHREAD_FLAG_SHOULD_EXIT;
      if (thr.joinable()) {
        thrCond.notify_one();
        locker.unlock();
        thr.join();
        locker.lock();
      }
      flags = 0;
    }

  private:
    void run() {
      std::unique_lock<std::mutex> locker(thrMutex);
      TaskInfo task;

      while ((flags & TASKTHREAD_FLAG_SHOULD_EXIT) == 0) {
        if (thePool->getPendingTask(task)) {
          if (task.cb)
            task.cb(TASK_OP_DOING);
          if (task.func)
            task.func();
          if (task.cb)
            task.cb(TASK_OP_DONE);
        } else {
          thePool->pushIdleThread(this);
          thrCond.wait(locker);
        }
      }
    }

  private:
    ThreadPool *thePool{nullptr};
    std::thread thr;
    std::mutex thrMutex;
    std::condition_variable thrCond;
    uint32_t flags{0};
  };

public:
  ThreadPool() {
  }

  explicit ThreadPool(uint32_t max) {
    init(max);
  }

  ~ThreadPool() {
    finish();
  }

  void init(uint32_t max) {
    std::lock_guard<std::mutex> locker(poolMutex);
    if (max == 0 || max > THRPOOL_MAX_THREADS || status & THRPOOL_INITIALIZED)
      return;
    status |= THRPOOL_INITIALIZED;
    threadArray.resize(max);

    uint32_t i;
    for (i = 0; i < max; ++i) {
      threadArray[i].setThreadPool(this);
    }
    initSleepThreads();
  }

  void push(TaskFunc task, TaskCallback cb = nullptr) {
    std::lock_guard<std::mutex> locker(poolMutex);
    std::unique_lock<std::mutex> taskLocker(taskMutex);
    pendingTasks.push_back({ task, cb });
    taskLocker.unlock();
    if (cb)
      cb(TASK_OP_QUEUE);
    taskLocker.lock();
    if (!idleThreads.empty()) {
      auto thr = idleThreads.front();
      idleThreads.pop_front();
      thr->work();
      return;
    }
    taskLocker.unlock();
    if (!sleepThreads.empty()) {
      auto thr = sleepThreads.front();
      sleepThreads.pop_front();
      thr->awake();
    }
  }

  void finish() {
    std::lock_guard<std::mutex> locker(poolMutex);
    std::unique_lock<std::mutex> taskLocker(taskMutex);
    while (!pendingTasks.empty()) {
      tasksDone.wait(taskLocker);
    }
    taskLocker.unlock();
    clearNolock();
  }

  void clear() {
    std::lock_guard<std::mutex> locker(poolMutex);
    clearNolock();
  }

private:
  void initSleepThreads() {
    size_t sz = threadArray.size();
    size_t i;
    sleepThreads.clear();
    for (i = 0; i < sz; ++i) {
      sleepThreads.push_back(threadArray.data() + i);
    }
  }

  bool getPendingTask(TaskInfo& task) {
    std::lock_guard<std::mutex> locker(taskMutex);
    if (pendingTasks.empty()) {
      task.func = nullptr;
      task.cb = nullptr;
      tasksDone.notify_one();
      return false;
    }
    task = pendingTasks.front();
    pendingTasks.pop_front();
    return true;
  }

  void pushIdleThread(TaskThread* thr) {
    std::lock_guard<std::mutex> locker(taskMutex);
    idleThreads.push_back(thr);
  }

  void clearNolock() {
    size_t sz = threadArray.size();
    size_t i;
    for (i = 0; i < sz; ++i) {
      threadArray[i].sleep();
    }
    taskMutex.lock();
    for_each(pendingTasks.begin(), pendingTasks.end(), [](TaskInfo& task) {
      if (task.cb)
        task.cb(TASK_OP_DISCARD);
    });
    pendingTasks.clear();
    idleThreads.clear();
    taskMutex.unlock();
    initSleepThreads();
  }

private:
  std::list<TaskInfo> pendingTasks;
  std::list<TaskThread*> idleThreads;
  std::list<TaskThread*> sleepThreads;
  std::mutex poolMutex;
  std::mutex taskMutex;
  std::condition_variable tasksDone;
  std::vector<TaskThread> threadArray;
  uint32_t status{0};

  friend TaskThread;
};
