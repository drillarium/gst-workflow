#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <queue>

/*
 *
 */
class threadPool
{
public:
  threadPool();
  void start(int numThreads);
  void queueJob(const std::function<void()>& job);
  void stop();
  bool busy();

protected:
  void threadLoop();

protected:
  bool m_shouldTerminate = false;           // tells threads to stop looking for jobs
  std::mutex m_queueMutex;                  // prevents data races to the job queue
  std::condition_variable m_mutexCondition; // allows threads to wait on new jobs or termination 
  std::vector<std::thread> m_threads;
  std::queue<std::function<void()>> m_jobs;
};
