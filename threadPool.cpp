#include "threadPool.h"

threadPool::threadPool()
{

}

void threadPool::start(int numThreads)
{
  // const uint32_t num_threads = std::thread::hardware_concurrency(); // Max # of threads the system supports
  m_threads.resize(numThreads);
  for(int i = 0; i < numThreads; i++)
    m_threads.at(i) = std::thread([&] { threadLoop(); });
}

void threadPool::stop()
{
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_shouldTerminate = true;
  }
  m_mutexCondition.notify_all();
  for(std::thread& active_thread : m_threads)
    active_thread.join();
  m_threads.clear();
}

void threadPool::threadLoop()
{
  while(true)
  {
    std::function<void()> job;
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_mutexCondition.wait(lock, [this] {
        return !m_jobs.empty() || m_shouldTerminate;
      });
      if(m_shouldTerminate)
        return;
            
      job = m_jobs.front();
      m_jobs.pop();
    }
    job();
  }
}

void threadPool::queueJob(const std::function<void()>& job)
{
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_jobs.push(job);
  }
  m_mutexCondition.notify_one();
}

bool threadPool::busy()
{
  bool poolbusy = false;
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    poolbusy = m_jobs.empty();
  }
  return poolbusy;
}

