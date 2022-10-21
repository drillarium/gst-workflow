#pragma once

#include "worker.h"

class process : public worker, public jobListener
{
public:
  process(const char *type);
  ~process();

  bool load(const char *param, const char *value);

  /* jobListener overrides */
  void onJobAborted(const char *jobUID);

protected:
  bool doJob(job *j, std::string &condition, std::string &error);

protected:
  std::map<std::string, std::string> m_params;
  FILE *m_pipe = NULL;
};
