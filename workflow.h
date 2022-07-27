#pragma once

#include <string>
#include <list>
#include "rapidjson/document.h"
#include "worker.h"
#include "job.h"

/*
 * sample workflow
 * {
 *   type: "workflow",
 *   template: fakesrc name="" ! delay name="" ! fakesink name=""
 *   uid: "",
 *   name: ""
 * }
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

protected:
  std::string m_uid;                                          // workflow uid
  std::string m_name;                                         // workflow name
  std::list<worker *>  m_workers;                             // workers
  rapidjson::Document m_jWorkflow;                            // loaded workflow
  EWorkflowStatus m_status = EWorkflowStatus::WF_ST__Stopped; // status
  std::list<job *> m_jobs;                                    // jobs
};
