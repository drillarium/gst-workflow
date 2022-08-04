#pragma once

#include "worker.h"

class plugin : public worker
{
public:
  plugin(const char *module, const char *type);

protected:
  bool processJob(job *j);

protected:
  std::string m_module;
};
