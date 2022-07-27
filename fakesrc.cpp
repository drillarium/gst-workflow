#include "fakesrc.h"
#include "workflow.h"
#include <chrono>

fakesrc::fakesrc()
{
  m_type = "fakesrc";
}

bool fakesrc::start()
{
  bool ret = worker::start();
  if(ret)
    newJob();
  
  return true;
}

bool fakesrc::processJob(job *j)
{
  if(!worker::processJob(j))
    return false;

  rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100);
  j->update(m_name.c_str(), status);

  /* register in workflow */
  m_workflow->addJob(j);

  /* send to the nexts workers */
  propagateJob(j);

  /* iterate */
  newJob();

  return true;
}

void fakesrc::newJob()
{
  m_threadPool.queueJob([this] {
    job *j = new job;
    processJob(j);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  });
}
