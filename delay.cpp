#include "delay.h"

delay::delay()
{
  m_type = "delay";
}

bool delay::processJob(job *j)
{
  if(!worker::processJob(j))
    return false;

  for(int i = 0; i<100; i+=25)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // status
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Running, i);
    j->update(m_name.c_str(), status);
  }

  // status
  rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100);
  j->update(m_name.c_str(), status);

  // condition
  static bool s_switch = false;
  propagateJob(j, s_switch? "ok" : "error");
  s_switch = !s_switch;

  return true;
}
