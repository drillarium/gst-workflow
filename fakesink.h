#pragma once

#include "worker.h"

class fakesink : public worker
{
public:
  fakesink();

protected:
  bool processJob(job *j);
};
