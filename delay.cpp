#include <chrono>
#include "workflow.h"

class delay : public worker
{
public:
  delay()
  {
    m_type = "delay";
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
    if(!_stricmp(param, "abort"))
    {
      m_abort = atoi(value);
      return true;
    }
    if(!_stricmp(param, "error"))
    {
      m_error = atoi(value);
      return true;
    }

    return false;
  }

  bool doJob(job *j, std::string &condition, std::string &error)
  {
    for(int i = 0; i < m_delay && !j->aborted(); i++)
    {
      if( (m_abort > 0) && (i >= m_abort) )
        j->abort();
      if( (m_error > 0) && (i >= m_error) )
      {
        error = "generic error";
        return false;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return true;
  }

protected:
  int m_delay = 10; // delay in seconds
  int m_abort = -1; // abort job in seconds
  int m_error = -1; // error job in seconds
};

/*
 * register class
 */
class delay_register
{
public:
  delay_register() { worker::register_worker("delay", "name,delay,abort", [] { return new delay; }, [] (worker *w) { delete w; }); }
} s_register;
