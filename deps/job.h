#pragma once

#include <string>
#include <mutex>
#include <list>
#include "rapidjson/document.h"

/*
{
  uid: { JOB_UID },
  name: { JOB_NAME },
  status: { JOB_STATUS },
  error: { JOB_ERROR }
  log: [
    { WORKER_NAME } : {
      name: { WORKER_NAME },
      type: { WORKER_TYPE },
      status: { JOB_STATUS },
      progress: { JOB_PROGRESS },
      error: { JOB_ERROR }
    }
  ]
}
*/

/*
 * status
 */
enum EJobStatus
{
  JOB_ST__Running = 0,
  JOB_ST__Completed,
  JOB_ST__Waiting
};

static const char * jobStatusToText(EJobStatus status)
{
  if(status == JOB_ST__Running) return "running";
  if(status == JOB_ST__Completed) return "completed";
  if(status == JOB_ST__Waiting) return "waiting";
  return "running";
}

static EJobStatus jobTextToStatus(const char *status)
{
  if(!_stricmp(status, "running")) return JOB_ST__Running;
  if(!_stricmp(status, "completed")) return JOB_ST__Completed;
  if(!_stricmp(status, "waiting")) return JOB_ST__Waiting;
  return JOB_ST__Running;
}

const char ABORTED_ERROR[] = "aborted";

/*
 *
 */
class jobListener
{
public:
  virtual void onJobAborted(const char *jobUID) = 0;
};

/*
 *
 */
class job
{
public:
  job();
  job(const char *jobData);
  void setUID(const char *value);
  const char * UID();
  void setName(const char *value);
  const char * name();
  void setStatus(EJobStatus status);
  EJobStatus status();
  bool updateStatus(const char *worker, const char *type, rapidjson::Document &status);
  rapidjson::Document status(const char *worker);
  bool isCompleted() { return status() == EJobStatus::JOB_ST__Completed; }
  bool abort();
  bool aborted();
  bool setError(const char *error);
  bool hasError();
  std::string serialize();
  static void registerListener(jobListener *listener) { s_listeners.push_back(listener); }
  static void unregisterListener(jobListener *listener) { s_listeners.remove(listener); }

protected:
  rapidjson::Document m_jJob;           // JSON job
  std::recursive_mutex m_jobMutex;      // prevents data races to the job
  static std::list< jobListener *> s_listeners;
};

