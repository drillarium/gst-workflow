#include "job.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "logger.h"
#include "notifier.h"

std::list< jobListener *> job::s_listeners;

void PRINT_JSON(const char *func, rapidjson::Document &doc)
{
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  printf("%s - %s", func, buffer.GetString());
}

job::job()
{
  m_jJob.SetObject();

  /* uid */
  static unsigned long long uid = 0;
  char aux[256] = { '\0' };
  sprintf_s(aux, "%llu", uid++);
  setUID(aux);

  /* status */
  setStatus(EJobStatus::JOB_ST__Running);

  /* log */
  rapidjson::Value v(rapidjson::kArrayType);
  m_jJob.AddMember("log", v, m_jJob.GetAllocator());

  /* error */
  setError("");
}

job::job(const char *jobData)
{
  m_jJob.Parse(jobData);
}

void job::setUID(const char *value)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::Value v;
  std::string s(value);
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), m_jJob.GetAllocator());
  m_jJob.AddMember("uid", v, m_jJob.GetAllocator());

  PRINT_JSON(__FUNCTION__, m_jJob);
}

const char * job::UID()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  return m_jJob["uid"].GetString();
}

void job::setName(const char *value)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::Value v;
  std::string s(value);
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), m_jJob.GetAllocator());
  m_jJob.AddMember("name", v, m_jJob.GetAllocator());

  PRINT_JSON(__FUNCTION__, m_jJob);
}

const char * job::name()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  return m_jJob["name"].GetString();
}

void job::setStatus(EJobStatus status)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::Value v;
  std::string s(jobStatusToText(status));
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), m_jJob.GetAllocator());
  if(m_jJob.HasMember("status"))
    m_jJob["status"] = v;
  else
    m_jJob.AddMember("status", v, m_jJob.GetAllocator());

  // notify
  wf_notify("job_updated", serialize().c_str());

  PRINT_JSON(__FUNCTION__, m_jJob);
}

EJobStatus job::status()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  return jobTextToStatus(m_jJob["status"].GetString());
}

bool job::updateStatus(const char *worker, const char *type, rapidjson::Document &status)
{
  bool found = false;

  // name
  rapidjson::Value v;
  v.SetString(worker, (rapidjson::SizeType) strlen(worker), status.GetAllocator());
  status.AddMember("name", v, status.GetAllocator());

  // type
  v.SetString(type, (rapidjson::SizeType) strlen(type), status.GetAllocator());
  status.AddMember("type", v, status.GetAllocator());

  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  v.CopyFrom(status, m_jJob.GetAllocator());

  for(unsigned int i = 0; i < m_jJob["log"].GetArray().Size() && !found; i++)
  {
    rapidjson::Value &val = m_jJob["log"].GetArray()[i].GetObject();
    found = !_stricmp(worker, val["name"].GetString());
    if(found)
      m_jJob["log"].GetArray()[i] = v;
  }
  if(!found)
    m_jJob["log"].GetArray().PushBack(v, m_jJob.GetAllocator());

  wf_notify("job_updated", serialize().c_str());

  PRINT_JSON(__FUNCTION__, m_jJob);

  return true;
}

rapidjson::Document job::status(const char *worker)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::Document doc;
  for(unsigned int i = 0; i < m_jJob["log"].GetArray().Size(); i++)
  {
    rapidjson::Value &val = m_jJob["log"].GetArray()[i].GetObject();
    if(!_stricmp(worker, val["name"].GetString()))
    {
      doc.CopyFrom(m_jJob["log"].GetArray()[i], doc.GetAllocator());
      return doc;
    }
  }

  return doc;
}

std::string job::serialize()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  m_jJob.Accept(writer);
  return buffer.GetString();
}

bool job::abort()
{
  setError(ABORTED_ERROR);

  for(auto it = s_listeners.begin(); it != s_listeners.end(); it++)
    (*it)->onJobAborted(UID());
   
  return true;
}

bool job::aborted()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);
  if(!m_jJob.HasMember("error"))
    return false;

  return !strcmp(m_jJob["error"].GetString(), ABORTED_ERROR);
}

bool job::setError(const char *error)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::Value v;
  std::string s(error);
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), m_jJob.GetAllocator());
  if(m_jJob.HasMember("error"))
    m_jJob["error"] = v;
  else
    m_jJob.AddMember("error", v, m_jJob.GetAllocator());

  return true;
}

bool job::hasError()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);
  if(!m_jJob.HasMember("error"))
    return false;

  return (strlen(m_jJob["error"].GetString()) > 0);
}
