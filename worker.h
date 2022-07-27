#pragma once

#include <string>
#include <map>
#include <list>
#include "threadPool.h"
#include "job.h"

/*
 * next / prev worker with condition
 */
struct workerCondition
{
  class worker *w;
  std::string condition;
};

/*
 * worker
 */
class workflow;
class worker
{
public:
  worker();
  static worker * create(const char *name, workflow *wf);
  const char *name() { return m_name.c_str(); }
  int numWorkers() { return m_numWorkers; }
  bool setNextWorker(worker *nextWorker, const char *condition);
  bool setPrevWorker(worker *prevWorker, const char *condition);
  bool pushJob(job *j);
  virtual bool start();
  virtual bool stop();
  virtual bool load(const char *param);
  virtual rapidjson::Document buildStatus(EJobStatus status, int progress = 0, const char *error = "");

protected:
  bool propagateJob(job *j, const char *condition = "");
  virtual bool processJob(job *j);
  
protected:
  std::string m_type;                      // type
  std::string m_name;                      // worker name
  int m_numWorkers = 1;                    // num worker threads
  std::list<workerCondition> m_nextWorker; // next worker list
  std::list<workerCondition> m_prevWorker; // prev worker list
  threadPool m_threadPool;                 // thread pool
  workflow *m_workflow = NULL;             // workflow
  std::map<std::string, int> m_syncJobs;   // for sync jobs case several input workers
};
