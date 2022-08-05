#include "dummy.h"

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
  return 0;
}

bool destroy_worker(void *w)
{
  return true;
}

bool start_worker(void *w)
{
  return true;
}

bool stop_worker(void *w)
{
  return true;
}

bool load_worker(void *w, const char *param)
{
  return true;
}

void log_set_callback(void(*fn) (ELogSeverity severity, const char *message, void *_private), void *param)
{
  wf_log_set_callback(fn, param);
}

