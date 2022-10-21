#pragma once

#include <string>

/* plugin exposed workers */
struct pluginWorker
{
  std::string name;
  std::string params;
  const pluginWorker *next;
};

/* parameter parser */
static bool parse_token(const char *_param, std::string &token, std::string &value)
{
  std::string delimiter = "=";
  std::string param(_param);
  size_t pos = param.find(delimiter);
  if(pos == std::string::npos)
    return false;

  token = param.substr(0, pos);
  value = param.substr(pos + 1, -1);
  if(value[0] == '\'')
  {
    value.erase(0, 1);
    value.erase(value.size() - 1);
  }

  return true;
}
