#include "worker.h"
#include "workflow.h"
#include "workflow_helper.h"

std::map<std::string, workerRegister> *worker::s_register = NULL;

bool worker::register_worker(const char *name, const std::function<worker * ()> &creator, const std::function<void(worker *)> &destructor)
{
  printf("Worker: \"%s\" registered\n", name);
  static std::map<std::string, workerRegister> r;
  s_register = &r;
  (*s_register)[name] = { name, creator, destructor};

  return true;
}

worker * worker::create(const char *name, workflow *wf)
{
  if((*s_register).find(name) == (*s_register).end())
    return NULL;

  worker *w = (*s_register)[name].constr();
  if(w)
    w->setWorkflow(wf);

  return w;
}

bool worker::destroy(worker *w)
{
  if((*s_register).find(w->name()) == (*s_register).end())
    return false;

  (*s_register)[w->name()].destr(w);

  return true;
}

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
  std::string token, value;
  bool ret = parse_token(_param, token, value);
  if(ret)
  {
    if(token == "name")
      m_name = value;
    else if(token == "workers")
      m_numWorkers = atoi(value.c_str());
  }

  return ret;
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

    // aborted?
    if(j->aborted())
      m_workflow->removeJob(j->UID());
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
  bool process = false;

  // only one previous worker, job can be consumed
  if(numPrevWorkers <= 1)
    process = true;
  else
  {
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
      process = true;
    }
  }

  // if process but abort, return false and delete job
  if(process && j->aborted())
  {
    process = false;
    
    // check abort
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 0, "Aborted");
    j->update(m_name.c_str(), status);

    j->setStatus(EJobStatus::JOB_ST__Completed);
    m_workflow->removeJob(j->UID());
  }

  return process;
}

bool worker::hasError(job *j)
{
  // check if previous worker finish job with any error
  bool error = false;
  for(auto e : m_prevWorker)
    error |= j->hasError(e.w->name());

  return error;
}

void worker::serializeWorker(SWorkflowPad &wp)
{
  for(auto wc : m_nextWorker)
  {
    SWorkflowPad wpn = { wc.condition, wc.w->name() };
    wc.w->serializeWorker(wpn);
    wp.next.push_back(wpn);
  }
}
