#include "worker.h"
#include "fakesrc.h"
#include "fakesink.h"
#include "delay.h"
#include "terminator.h"
#include "workflow.h"

std::string get_param_value_string(const std::list<std::string> &params, const char *key, const char *defValue)
{
  std::string delimiter = "=";

  for(auto e : params)
  {
    size_t pos = e.find(delimiter);
    if(pos != std::string::npos)
    {
      std::string token = e.substr(0, pos);
      if(!token.compare(key))
      {
        std::string value = e.substr(pos + 1, -1);
        value.erase(0, 1);
        value.erase(value.size() - 1);
        return value;
      }
    }
  }

  return defValue;
}

worker::worker()
{

}

bool worker::start()
{
  /* create worker threads */
  m_threadPool.start(m_numWorkers);

  return true;
}

bool worker::stop()
{
  /* destroy worker threads */
  m_threadPool.stop();

  return true;
}

worker * worker::create(const char *name, workflow *wf)
{
  worker *w = NULL;
  if(!strcmp(name, "fakesrc"))
    w = new fakesrc;
  else if(!strcmp(name, "fakesink"))
    w = new fakesink;
  else if(!strcmp(name, "delay"))
    w = new delay;
  else if (!strcmp(name, "terminator"))
    w = new terminator;

  if(w)
    w->m_workflow = wf;

  return w;
}

bool worker::setNextWorker(worker *nextWorker, const char *condition)
{
  m_nextWorker.push_back( {nextWorker, condition} );
  return true;
}

bool worker::setPrevWorker(worker *prevWorker, const char *condition)
{
  m_prevWorker.push_back( {prevWorker, condition} );
  return true;
}

bool worker::load(const char *_param)
{
  std::string delimiter = "=";
  std::string param(_param);
  size_t pos = param.find(delimiter);
  if(pos != std::string::npos)
  {
    std::string token = param.substr(0, pos);
    std::string value = param.substr(pos + 1, -1);
    if(token == "name")
    {
      value.erase(0, 1);
      value.erase(value.size() - 1);
      if(value.length() == 0)
      {
        char aux[256];
        sprintf_s(aux, "%p", (void *) this);
        value = aux;
      }
      m_name = value;
    }
    else if(token == "workers")
      value = atoi(value.c_str());
  }

  return true;
}

bool worker::pushJob(job *j)
{
  if(!j)
    return false;

  // waiting
  rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Waiting);
  j->update(m_name.c_str(), status);

  m_threadPool.queueJob([this, j] {
    // status
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Running, 0);
    j->update(m_name.c_str(), status);

    processJob(j);
  });

  return true;
}

bool worker::propagateJob(job *j, const char *condition)
{
  // push job
  if(m_nextWorker.size() > 0)
  {
    for(auto e : m_nextWorker)
    {
      // check condition
      if(e.condition.empty() || !e.condition.compare(condition))
        e.w->pushJob(j);
    }
  }

  if(m_nextWorker.size() == 0)
  {
    // mark as completed
    j->setStatus(EJobStatus::JOB_ST__Completed);

    // log
    j->log(__FUNCTION__);
  }

  return true;
}

rapidjson::Document worker::buildStatus(EJobStatus status, int progress, const char *error)
{
  rapidjson::Document doc;
  doc.SetObject();

  // name
  rapidjson::Value v;
  v.SetString(m_name.c_str(), (rapidjson::SizeType) m_name.length(), doc.GetAllocator());
  doc.AddMember("name", v, doc.GetAllocator());

  // name
  v.SetString(m_type.c_str(), (rapidjson::SizeType) m_type.length(), doc.GetAllocator());
  doc.AddMember("type", v, doc.GetAllocator());

  // status
  std::string s(jobStatusToText(status));
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), doc.GetAllocator());
  doc.AddMember("status", v, doc.GetAllocator());

  // progress
  v.SetInt(progress);
  doc.AddMember("progress", v, doc.GetAllocator());

  // error
  s = error;
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), doc.GetAllocator());
  doc.AddMember("error", v, doc.GetAllocator());

  return doc;
}

bool worker::processJob(job *j)
{
  int numPrevWorkers = (int) m_prevWorker.size();
  
  // only one previous worker, job can be consumed
  if(numPrevWorkers <= 1)
    return true;

  // sync job with num previous workers
  const char *uid = j->UID();
  if(m_syncJobs.find(uid) == m_syncJobs.end())
    m_syncJobs[uid] = 1;
  else
    m_syncJobs[uid]++;
  
  // job can be propagated
  if(m_syncJobs[uid] >= numPrevWorkers)
  {
    m_syncJobs.erase(uid);
    return true;
  }

  return false;
}
