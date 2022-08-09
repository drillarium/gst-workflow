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
}

void job::setUID(const char *value)
{
  rapidjson::Value v;
  std::string s(value);
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), m_jJob.GetAllocator());
  m_jJob.AddMember("uid", v, m_jJob.GetAllocator());

  PRINT_JSON(__FUNCTION__, m_jJob);
}

const char * job::UID()
{
  return m_jJob["uid"].GetString();
}

void job::setStatus(EJobStatus status)
{
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
  return jobTextToStatus(m_jJob["status"].GetString());
}

bool job::update(const char *worker, rapidjson::Document &status)
{
  bool found = false;

  std::unique_lock<std::mutex> lock(m_jobMutex);

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

  PRINT_JSON(__FUNCTION__, m_jJob);

  return true;
}

void job::log(const char *message)
{
  PRINT_JSON(message, m_jJob);
}

bool job::hasError(const char *worker)
{
  bool error = false;

  bool found = false;
  for(unsigned int i = 0; i < m_jJob["log"].GetArray().Size() && !found; i++)
  {
    rapidjson::Value &val = m_jJob["log"].GetArray()[i].GetObject();
    found = !_stricmp(worker, val["name"].GetString());
    if(found)
      error = (val["error"].GetStringLength() > 0);
  }

  return error;
}

std::string job::serialize()
{
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  m_jJob.Accept(writer);
  return buffer.GetString();
}
