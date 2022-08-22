#include "job.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "logger.h"

void PRINT_JSON(const char *func, rapidjson::Document &doc)
{
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  log_debug("%s - %s", func, buffer.GetString());
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
  setError("", "");
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

  PRINT_JSON(__FUNCTION__, m_jJob);
}

EJobStatus job::status()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  return jobTextToStatus(m_jJob["status"].GetString());
}

bool job::update(const char *worker, rapidjson::Document &status)
{
  bool found = false;

  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::Value v;
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

  // propagate to job
  if(hasError(worker) && !hasError())
    setError(worker, getError(worker));

  PRINT_JSON(__FUNCTION__, m_jJob);

  return true;
}

void job::log(const char *message)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  PRINT_JSON(message, m_jJob);
}

bool job::hasError(const char *worker)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  return (strlen(getError(worker)) > 0);
}

const char * job::getError(const char *worker)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  for(unsigned int i = 0; i < m_jJob["log"].GetArray().Size(); i++)
  {
    rapidjson::Value &val = m_jJob["log"].GetArray()[i].GetObject();
    if(!_stricmp(worker, val["name"].GetString()))
      return val["error"].GetString();
  }

  return "";
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
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  return setError("", "Aborted");
}

bool job::aborted()
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  return !strcmp(m_jJob["error"].GetString(), "Aborted");
}

bool job::setError(const char *worker, const char *error)
{
  char aux[1024] = {'\0'};
  if(strlen(worker) > 0)
    sprintf_s(aux, "[%s] %s", worker, error);
  else
    sprintf_s(aux, error);

  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  rapidjson::Value v;
  std::string s(aux);
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

  return (strlen(m_jJob["error"].GetString()) > 0);
}

bool job::updateWork(const char *workStatus, const char *worker, rapidjson::Document &work)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  if(!m_jJob.HasMember(workStatus))
  {
    rapidjson::Value v(rapidjson::kArrayType);
    rapidjson::Value ws;
    std::string s(workStatus);
    ws.SetString(s.c_str(), (rapidjson::SizeType) s.length(), m_jJob.GetAllocator());
    m_jJob.AddMember(ws, v, m_jJob.GetAllocator());
  }

  rapidjson::Value v;
  v.CopyFrom(work, m_jJob.GetAllocator());

  bool found = false;
  for(unsigned int i = 0; i < m_jJob[workStatus].GetArray().Size() && !found; i++)
  {
    rapidjson::Value &val = m_jJob[workStatus].GetArray()[i].GetObject();
    found = !_stricmp(worker, val["name"].GetString());
    if(found)
      m_jJob[workStatus].GetArray()[i] = v;
  }
  if(!found)
    m_jJob[workStatus].GetArray().PushBack(v, m_jJob.GetAllocator());

  PRINT_JSON(__FUNCTION__, m_jJob);

  return true;
}

rapidjson::Document job::getWork(const char *workStatus, const char *worker)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobMutex);

  if(m_jJob.HasMember(workStatus))
  {
    for(unsigned int i = 0; i < m_jJob[workStatus].GetArray().Size(); i++)
    {
      rapidjson::Value &val = m_jJob[workStatus].GetArray()[i].GetObject();
      if(!_stricmp(worker, val["name"].GetString()))
      {
        rapidjson::Document doc;
        doc.CopyFrom(m_jJob[workStatus].GetArray()[i], m_jJob.GetAllocator());

        PRINT_JSON(__FUNCTION__, doc);

        return doc;
      }
    }
  }

  return rapidjson::Document();
}
