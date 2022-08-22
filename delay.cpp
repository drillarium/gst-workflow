#include "delay.h"

/*
 * register class
 */
class delay_register
{
public:
  delay_register() { worker::register_worker("delay", [] { return new delay; }, [] (worker *w) { delete w; }); }
} s_register;

/*
 *
 */
delay::delay()
{
  m_type = "delay";
}

bool delay::processJob(job *j)
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
    // work from previous worker
    rapidjson::Document prevWork = j->getWork(DONE_WORK, m_prevWorker.front().w->name());
    rapidjson::Value v;
    v.SetString(m_name.c_str(), (rapidjson::SizeType) m_name.length(), prevWork.GetAllocator());
    prevWork["name"] = v;
    v.SetString(m_type.c_str(), (rapidjson::SizeType) m_type.length(), prevWork.GetAllocator());
    prevWork["type"] = v;

    // process
    int i = 0;
    for(i = 0; i<100 && !j->aborted(); i+=10)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    
      // status
      rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Running, i);
      j->update(m_name.c_str(), status);
    }

    // status
    // rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100);
    // j->update(m_name.c_str(), status);
  
    // status error
    // rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100, "Generic error");
    // j->update(m_name.c_str(), status);

    // check abort
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, i, j->aborted()? "Aborted" : "");      
    j->update(m_name.c_str(), status);

    // update work done
    j->updateWork(DONE_WORK, m_name.c_str(), prevWork);
  }

  // condition
  // stati bool s_switch = false;
  // propagateJob(j, s_switch? "ok" : "error");
  // s_switch = !s_switch;

  propagateJob(j);

  return true;
}
