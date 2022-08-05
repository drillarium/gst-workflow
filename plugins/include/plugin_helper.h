#pragma once

#include <string>

struct SPluginWorker
{
  std::string name;
  const SPluginWorker *next;
};
