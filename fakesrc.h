#pragma once

#include "worker.h"

class fakesrc : public worker
{
public:
  fakesrc();
  bool start();

protected:
  bool processJob(job *j);
  void newJob();
  rapidjson::Document buildWork();
};
