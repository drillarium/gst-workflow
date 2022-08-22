#include "fakesrc.h"
#include "workflow.h"
#include <chrono>

/*
 * register class
 */
class fakesrc_register
{
public:
  fakesrc_register() { worker::register_worker("fakesrc", [] { return new fakesrc; }, [] (worker *w) { delete w; }); }
} s_register;

/*
 *
 */
fakesrc::fakesrc()
{
  m_type = "fakesrc";
}

bool fakesrc::start()
{
  bool ret = worker::start();
  if(ret)
    newJob();
  
  return ret;
}

bool fakesrc::processJob(job *j)
{
  if(!worker::processJob(j))
    return false;

  /* add work done */
  rapidjson::Document work = buildWork();
  j->updateWork(DONE_WORK, m_name.c_str(), work);

  rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100);
  j->update(m_name.c_str(), status);

  /* register in workflow */
  m_workflow->addJob(j);

  /* send to the nexts workers */
  propagateJob(j);

  /* iterate */
    // newJob();

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

rapidjson::Document fakesrc::buildWork()
{
  rapidjson::Document doc;
  doc.SetObject();

  // name
  rapidjson::Value v;
  v.SetString(m_name.c_str(), (rapidjson::SizeType) m_name.length(), doc.GetAllocator());
  doc.AddMember("name", v, doc.GetAllocator());

  // type
  v.SetString(m_type.c_str(), (rapidjson::SizeType) m_type.length(), doc.GetAllocator());
  doc.AddMember("type", v, doc.GetAllocator());

  return doc;
}
