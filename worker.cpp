#include "worker.h"
#include "workflow.h"
#include "workflow_helper.h"

std::map<std::string, workerRegister> *worker::s_register = NULL;

bool worker::register_worker(const char *name, const char *params, const std::function<worker * ()> &creator, const std::function<void(worker *)> &destructor)
{
  printf("Worker: \"%s\" registered\n", name);
  static std::map<std::string, workerRegister> r;
  s_register = &r;
  (*s_register)[name] = { name, params, creator, destructor};

  return true;
}

worker * worker::create(const char *name, workflow *wf)
{
  if((*s_register).find(name) == (*s_register).end())
    return NULL;

  worker *w = (*s_register)[name].constr();
  if(w)
    w->m_workflow = wf;

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
    load(token.c_str(), value.c_str());

  return ret;
}

bool worker::load(const char *param, const char *value)
{
  if(!_stricmp(param, "name"))
  {
    m_name = value;
    return true;
  }

  if(!_stricmp(param, "workers"))
  {
    m_numWorkers = atoi(value);
    return true;
  }

  return false;
}

bool worker::pushJob(job *j)
{
  if(!j)
    return false;

  // waiting
  updateStatus(j, EJobStatus::JOB_ST__Waiting);

  m_threadPool.queueJob([this, j] {
    processJob(j);
  });

  return true;
}

bool worker::propagateJob(job *j, const char *condition)
{
  bool completed = (m_nextWorker.size() == 0);

  // push job
  if(!completed)
  {
    // case no next found, mark as completed
    completed = true;
    for(auto e : m_nextWorker)
    {
      // check condition
      if(e.condition.empty() || !e.condition.compare(condition))
      {
        completed = false;
        e.w->pushJob(j);
      }
    }
  }

  if(completed)
  {
    // mark as completed
    j->setStatus(EJobStatus::JOB_ST__Completed);
  }

  return true;
}

bool worker::updateStatus(job *j, EJobStatus status, int progress, const char *error)
{
  rapidjson::Document doc = updateStatus(status, progress, error);
 
  // keep work
  rapidjson::Document currentStatus = j->status(m_name.c_str());
  if(!currentStatus.IsNull())
  {
    if(currentStatus.HasMember("work"))
      doc.AddMember("work", currentStatus["work"], doc.GetAllocator());
  }

  // update job
  j->updateStatus(m_name.c_str(), m_type.c_str(), doc);

  return true;
}

rapidjson::Document worker::updateStatus(EJobStatus status, int progress, const char *error)
{
  // new document
  rapidjson::Document doc;
  doc.SetObject();

  // status
  rapidjson::Value v;
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
  // status
  updateStatus(j, EJobStatus::JOB_ST__Running, 0);

  // workers
  int numPrevWorkers = (int) m_prevWorker.size();
  bool process = false;

  // only one previous worker, job can be consumed
  if(numPrevWorkers <= 1)
    process = true;
  // the work will be done when slowest previos worker ends (case parallel workers)
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
  if(process)
  {
    bool success = true;
    std::string error, condition;

    // Do the job. check aborted flag
    if(!j->aborted())
    {
      success = doJob(j, condition, error);
      // completed
      updateStatus(j, EJobStatus::JOB_ST__Completed, 100, j->aborted() ? ABORTED_ERROR : error.c_str());
    }

    // mark as completed
    if(j->aborted() || !success || (error.length() > 0) )
    {
      j->setStatus(EJobStatus::JOB_ST__Completed);
      j->setError(j->aborted()? ABORTED_ERROR : error.c_str());
    }

    /* send to the nexts workers */
    if(j->status() != EJobStatus::JOB_ST__Completed)
      propagateJob(j, condition.c_str());
  }

  return true;
}
