#pragma once

#include <string>
#include <mutex>
#include "rapidjson/document.h"

/*
 *
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

/*
 *
 */
class job
{
public:
  job();
  void setUID(const char *value);
  const char * UID();
  void setStatus(EJobStatus status);
  bool update(const char *worker, rapidjson::Document &status);
  void log(const char *message);

protected:
  std::string m_uid;            // job uid
  rapidjson::Document m_jJob;   // JSON job
  std::mutex m_jobMutex;        // prevents data races to the job
};

