#include "dummy.h"
#include <thread>

class dummyWorker
{
public:
  virtual bool load(const char *param) = 0;
  bool abort(const char *jobUID)
  {
    m_abort = true;
    return true;
  }
  virtual bool process(const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted) = 0;

protected:
  bool m_abort = false;
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

  bool load(const char *param)
  {
    return true;
  }

  bool process(const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted)
  {
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

  bool load(const char *param)
  {
    return true;
  }

  bool abort(const char *jobUID)
  {
    return true;
  }

  bool process(const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted)
  {
    return true;
  }
};

/*
 *
 */
const SPluginWorker dummy2 = {
  "dummy_2",
  NULL
};

const SPluginWorker s_workers = {
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

const SPluginWorker * register_workers()
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
