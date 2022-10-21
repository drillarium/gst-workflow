#include <chrono>
#include "workflow.h"

class fakesrc : public worker
{
public:
  fakesrc()
  {
    m_type = "fakesrc";
  }
  bool start()
  {
    bool ret = worker::start();
    if (ret)
      newJob();

    return ret;
  }

protected:
  bool load(const char *param, const char *value)
  {
    if(worker::load(param, value))
      return true;

    if(!_stricmp(param, "delay"))
    {
      m_delay = atoi(value);
      return true;
    }

    return false;
  }

  bool doJob(job *j, std::string &condition, std::string &error)
  {
    if(m_delay > 0)
    {
      for(int i = 0; i < m_delay && !j->aborted(); i++)
        std::this_thread::sleep_for(std::chrono::seconds(1));
      newJob();
    }

    return true;
  }

  void newJob()
  {
    char name[256] = { '\0' };
    static int s_counter = 0;
    sprintf_s(name, "fakesrc_job#%04d", s_counter++);

    job *j = new job;
    j->setName(name);

    /* register in workflow */
    m_workflow->addJob(j);

    /* execute job */
    pushJob(j);
  }
  int m_delay = 5;
};

/*
 * register class
 */
class fakesrc_register
{
public:
  fakesrc_register() { worker::register_worker("fakesrc", "name,delay", [] { return new fakesrc; }, [] (worker *w) { delete w; }); }
} s_register;
