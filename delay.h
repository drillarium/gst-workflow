#pragma once

#include "worker.h"

class delay : public worker
{
public:
  delay();

protected:
  bool processJob(job *j);
};
