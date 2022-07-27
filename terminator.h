#pragma once

#include "worker.h"

class terminator : public worker
{
public:
  terminator();

protected:
  bool processJob(job *j);
};
