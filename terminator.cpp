#include "terminator.h"

/*
 * register class
 */
class terminator_register
{
public:
  terminator_register() { worker::register_worker("terminator", [] { return new terminator; }, [] (worker *w) { delete w; }); }
} s_register;

/*
 *
 */
terminator::terminator()
{
  m_type = "terminator";
}

bool terminator::processJob(job *j)
{
  if(!worker::processJob(j))
    return false;

  // completed
  rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100);
  j->update(m_name.c_str(), status);

  // propagate
  propagateJob(j);

  return true;
}