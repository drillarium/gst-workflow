#pragma once

#include "worker.h"

class plugin : public worker
{
public:
  plugin(const char *module, const char *type);
  ~plugin();

  bool load(const char *param);
  void onJobAborted(const char *jobUID);
  void onWorkflowParsed();

protected:
  bool processJob(job *j);

protected:
  std::string m_module;
  void *m_context = NULL;
};
