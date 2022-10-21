#pragma once

#include "worker.h"

class plugin : public worker, public jobListener
{
public:
  plugin(const char *module, const char *type);
  ~plugin();

  bool load(const char *param, const char *value);

  /* jobListener overrides */
  void onJobAborted(const char *jobUID);

protected:
  bool doJob(job *j, std::string &condition, std::string &error);

protected:
  std::string m_module;
  void *m_context = NULL;
};
