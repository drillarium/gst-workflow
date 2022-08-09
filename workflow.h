#pragma once

#include <string>
#include <list>
#include "rapidjson/document.h"
#include "worker.h"
#include "job.h"
#include "logger.h"

/*
 *
 */
enum EWorkflowStatus
{
  WF_ST__Stopped = 0,
  WF_ST__Running
};

/*
 * workflow class
 */
class workflow
{
public:
  workflow();
  ~workflow();

  bool load(const char *workflow);
  bool start();
  bool stop();
  EWorkflowStatus status() { return m_status; }
  bool addJob(job *j);
  bool removeJob(const char *jobUID);

protected:
  worker * getWorkerByName(const char *name);
  job * getJob(const char *uid);
  void onJobAborted(const char *jobUID);

protected:
  std::string m_uid;                                          // workflow uid
  std::string m_name;                                         // workflow name
  std::list<worker *>  m_workers;                             // workers
  rapidjson::Document m_jWorkflow;                            // loaded workflow
  EWorkflowStatus m_status = EWorkflowStatus::WF_ST__Stopped; // status
  std::list<job *> m_jobs;                                    // jobs
  std::mutex m_jobsMutex;
};
