#pragma once

#include "worker.h"
#include "filewatcher.h"

class filesystemwatcher : public worker
{
public:
  filesystemwatcher();
  bool start();
  bool stop();
  bool load(const char *_param);

protected:
  rapidjson::Document buildWork(FileStatus fileStatus, const char *file);

protected:
  FileWatcher *m_fw = NULL;
};

