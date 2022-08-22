#include "dummy.h"
#include "job.h"
#include <thread>

class dummyWorker
{
public:
  bool load(const char *param)
  {
    std::string token, value;
    bool ret = parse_token(param, token, value);
    if(ret)
    {
      if(!token.compare("name"))
        m_name = value;
    }

    return ret;
  }
  bool abort(const char *jobUID)
  {
    m_abort = true;
    return true;
  }
  virtual bool process(const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted) = 0;
  void setWorkflow(const SWorkflowPad &workflow)
  {
    m_workflow = workflow;

    // prev workers
    searchPrevWorkers(m_workflow);
  }

protected:
  void searchPrevWorkers(const SWorkflowPad &wp)
  {
    for(auto n : wp.next)
    {
      if(!n.worker.compare(m_name))
        m_prevWorkers.push_back(wp.worker);
      else
        searchPrevWorkers(n);
    }
  }

protected:
  std::string m_name;
  std::list<std::string> m_prevWorkers;   // prev workers name
  bool m_abort = false;
  SWorkflowPad m_workflow;                // workflow json data
};

/*
 *
 */
class dummy1Worker : public dummyWorker
{
public:
  dummy1Worker()
  {

  }

  bool process(const char *jobData, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted)
  {
    // build job
    job j(jobData);

    // work from previous worker required
    // list of previous workers by name
    for(auto e: m_prevWorkers)
      rapidjson::Document prevWork = j.getWork(DONE_WORK, e.c_str());

    // process
    for(int i = 0; i < 100 && !m_abort; i += 10)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      // status
      onProgress(i);
    }

    // status
    onCompleted("" /* error */, "" /* condition */);

    return true;
  }
};

/*
 *
 */
class dummy2Worker : public dummyWorker
{
public:
  dummy2Worker()
  {

  }

  bool process(const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted)
  {
    return true;
  }
};

/*
 *
 */
const pluginWorker dummy2 = {
  "dummy_2",
  NULL
};

const pluginWorker s_workers = {
  "dummy_1",
  &dummy2
};

/*
 *
 */
void init_lib()
{
  log_info("dummy library init");
}

void deinit_lib()
{
  log_info("dummy library deinit");
}

const pluginWorker * register_workers()
{
  return &s_workers;
}

void * create_worker(const char *type)
{
  if(!strcmp(type, "dummy_1"))
    return new dummy1Worker;
  if(!strcmp(type, "dummy_2"))
    return new dummy2Worker;

  return 0;
}

bool destroy_worker(void *w)
{
  delete w;

  return true;
}

bool load_worker(void *w, const char *param)
{
  dummyWorker *dw = static_cast<dummyWorker *> (w);
  return dw->load(param);
}

void log_set_callback(void(*fn) (ELogSeverity severity, const char *message, void *_private), void *param)
{
  wf_log_set_callback(fn, param);
}

bool abort_job(void *w, const char *jobUID)
{
  dummyWorker *dw = static_cast<dummyWorker *> (w);
  return dw->abort(jobUID);
}

bool process_job(void *w, const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted)
{
  dummyWorker *dw = static_cast<dummyWorker *> (w);
  return dw->process(job, onProgress, onCompleted);
}

void set_workflow(void *w, const SWorkflowPad &wp)
{
  dummyWorker *dw = static_cast<dummyWorker *> (w);
  return dw->setWorkflow(wp);
}
