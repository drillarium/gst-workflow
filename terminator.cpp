#include "terminator.h"

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