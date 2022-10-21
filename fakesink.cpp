#include "worker.h"

class fakesink : public worker
{
public:
  fakesink() { m_type = "fakesink"; }

protected:
  bool doJob(job *j, std::string &condition, std::string &error) { return true; }
};

/*
 * register class
 */
class fakesink_register
{
public:
  fakesink_register() { worker::register_worker("fakesink", "name", [] { return new fakesink; }, [] (worker *w) { delete w; }); }
} s_register;
