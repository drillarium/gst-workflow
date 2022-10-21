#pragma once

#include <string>
#include <map>
#include <list>
#include "threadPool.h"
#include "job.h"
#include "logger.h"
#include "workflow_helper.h"

/*
 * next / prev worker with condition
 */
struct workerCondition
{
  class worker *w;
  std::string condition;
};

/*
 * register
 */
struct workerRegister
{
  std::string name;                       // worker name
  std::string params;                     // worker params
  std::function<worker * ()> constr;      // constructor
  std::function<void(worker *)> destr;    // destructor
};

/*
 * worker
 */
class workflow;
class worker
{
public:
  worker();
  static bool register_worker(const char *name, const char *params, const std::function<worker * ()> &creator, const std::function<void(worker *)> &destructor);
  static worker * create(const char *name, workflow *wf);
  static bool destroy(worker *w);
  const char *name() { return m_name.c_str(); }
  int numWorkers() { return m_numWorkers; }
  bool setNextWorker(worker *nextWorker, const char *condition);
  bool setPrevWorker(worker *prevWorker, const char *condition);
  virtual bool start();
  virtual bool stop();
  bool load(const char *param);

protected:
  virtual bool load(const char *param, const char *value);    // every worker can override and read its own parameters
  bool pushJob(job *j);
  bool processJob(job *j);                                    // the process job function
  virtual bool doJob(job *j, std::string &condition, std::string &error) { error = "not implemented"; return false; } // override by the workers to run the job. return error or not, the condition raised and a error string
  bool propagateJob(job *j, const char *condition = "");      // to the next(s) workers if available
  bool updateStatus(job *j, EJobStatus status, int progress = 0, const char *error = "");
  rapidjson::Document updateStatus(EJobStatus status = JOB_ST__Running, int progress = 0, const char *error = "");
  
protected:
  std::string m_type;                      // type
  std::string m_name;                      // worker name
  int m_numWorkers = 1;                    // num worker threads
  std::list<workerCondition> m_nextWorker; // next worker list
  std::list<workerCondition> m_prevWorker; // prev worker list
  threadPool m_threadPool;                 // thread pool
  workflow *m_workflow = NULL;             // workflow
  std::map<std::string, int> m_syncJobs;   // for sync jobs case several input workers
  static std::map<std::string, workerRegister> *s_register;
};
