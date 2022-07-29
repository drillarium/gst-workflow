#include "fakesink.h"

/*
 * register class
 */
class fakesink_register
{
public:
  fakesink_register() { worker::register_worker("fakesink", [] { return new fakesink; }, [] (worker *w) { delete w; }); }
} s_register;

/*
 *
 */
fakesink::fakesink()
{
  m_type = "fakesink";
}

bool fakesink::processJob(job *j)
{
  if(!worker::processJob(j))
    return false;

  // has errors
  if(hasError(j))
  {
    // status
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100, "Error in previous worker");
    j->update(m_name.c_str(), status);
  }
  else
  {
    // completed
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100);
    j->update(m_name.c_str(), status);
  }

  // propagate
  propagateJob(j);

  return true;
}