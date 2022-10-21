#include <filesystem>
#include "process.h"
#include "workflow.h"

// filesystem
namespace fs = std::filesystem;

struct SWFProcesses
{
  std::string process;
  std::list<std::string> workers;
};

/*
 *
 */
class process_register
{
public:
  process_register()
  {
    // plugin folder      
    std::string cp = fs::current_path().string() + "\\apps";
    fs::create_directories(cp);
    const fs::path pathToShow{ cp };
    try
    {
      for(const auto &entry : fs::directory_iterator(pathToShow))
      {
        const auto filenameStr = entry.path().filename().string();
        if(entry.is_directory())
        {
          std::string js = entry.path().string() + "\\" + entry.path().filename().string() + ".js";
          std::string exe = entry.path().string() + "\\" + entry.path().filename().string() + ".exe";
          const fs::path jsPath(js), exePath(exe);
          std::string pgc;
          if(fs::exists(jsPath))
            pgc = "node " + js;
          else if (fs::exists(exePath))
            pgc = exe;

          if(pgc.length() > 0)
          {
            std::string pgca = pgc + " -i";
            std::string result;
            FILE* pipe = _popen(pgca.c_str(), "r");
            if(pipe)
            {
              try
              {
                char buffer[1024];
                while(fgets(buffer, sizeof buffer, pipe) != NULL)
                  result += buffer;
              }
              catch(...)
              {

              }
              _pclose(pipe);
            }

            // parse result
            if(result.length() > 0)
            {
              rapidjson::Document doc;              
              doc.Parse(result.c_str());
              if(doc.IsArray())
              {
                std::list<std::string> workers;
                for(unsigned int i = 0; i < doc.GetArray().Size(); i++)
                {
                  rapidjson::Value &val = doc.GetArray()[i].GetObject();
                  std::string type = val["name"].GetString();                
                  std::string params;
                  for(unsigned int j = 0; j < val["params"].GetArray().Size(); j++)
                  {
                    if(params.length() > 0)
                      params += ",";
                    params += val["params"].GetArray()[j].GetString();
                  }

                  workers.push_back(type);

                  // register plugins
                  worker::register_worker(type.c_str(), params.c_str(), [type] {
                    return new process(type.c_str());
                  }, [](worker *w) {
                    delete w;
                  });
                }

                // execute to know worker name and params
                SWFProcesses p;
                p.process = pgc;
                p.workers = workers;
                m_processes.push_back(p);
              }
            }
          }
        }
      }
    }
    catch (...)
    {
      // folder does not exits!!!
    }
  }

  SWFProcesses * findProcess(const char *name)
  {
    SWFProcesses *prod = NULL;
    for(auto &p : m_processes)
    {
      for(auto &n: p.workers)
      {
        if(!n.compare(name))
          return &p;
      }
    }

    return NULL;
  }

protected:
  std::list<SWFProcesses> m_processes;

} s_processes;

/*
 *
 */
process::process(const char *type)
{
  m_type = type;

  // register
  job::registerListener(this);
}

process::~process()
{
  // unregister
  job::unregisterListener(this);
}

std::string replaceAll(std::string str, const std::string& from, const std::string& to)
{
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
  }

  return str;
}

bool process::doJob(job *j, std::string &condition, std::string &error)
{
  // create
  SWFProcesses *process = s_processes.findProcess(m_type.c_str());
  if(!process)
  {
    error = "invalid process name";
    return false;
  }

  // params (as args)
  std::string processArgs = process->process;
  for(auto e : m_params)
    processArgs += " --" + e.first + " " + e.second;

  // workflow
  std::string pipeline = m_workflow->pipe();
  pipeline = replaceAll(pipeline, "\"", "\"\"");
  processArgs += " -w \"" + pipeline + "\"";

  // job
  std::string js = j->serialize();
  js = replaceAll(js, "\"", "\"\"");
  processArgs += " -j \"" + js + "\"";

  // worker
  processArgs += " --worker " + m_type;

  // process
  bool finished = false, success = false;
  m_pipe = _popen(processArgs.c_str(), "r");
  if(m_pipe)
  {
    try
    {
      int jobProgress = 0;
      char buffer[1024];
      while(fgets(buffer, sizeof buffer, m_pipe) != NULL)
      {
        rapidjson::Document doc;
        doc.Parse(buffer);
        if(doc.IsObject())
        {
          if(doc.HasMember("notify"))
          {
            const char *notification = doc["notify"].GetString();
            if(!_stricmp(notification, "progress"))
            {
              jobProgress = doc["value"].GetInt();
              updateStatus(j, EJobStatus::JOB_ST__Running, jobProgress);
            }
            else if(!_stricmp(notification, "completion"))
            {
              finished = true;
              condition = doc["condition"].GetString();
              error = doc["condition"].GetString();
              success = doc["success"].GetBool();
            }
            else if(!_stricmp(notification, "work"))
            {
              rapidjson::Document doc_ = updateStatus(EJobStatus::JOB_ST__Running, jobProgress);

              // work
              doc_.AddMember("work", doc["work"], doc_.GetAllocator());

              j->updateStatus(m_name.c_str(), m_type.c_str(), doc_);
            }
            else if(!_stricmp(notification, "log"))
            {
              const char *severity = doc["severity"].GetString();
              const char *message = doc["message"].GetString();
              if(!_stricmp(severity, "info")) log_info(message);
              else if(!_stricmp(severity, "warning")) log_warn(message);
              else if(!_stricmp(severity, "error")) log_error(message);
            }
          }
        }
      }
    }
    catch (...)
    {

    }

    _pclose(m_pipe);
  }

  if(!finished)
    error = "unespected process exit";

  return success;
}

bool process::load(const char *param, const char *value)
{
  worker::load(param, value);

  // save
  m_params[param] = value;

  return true;
}

void process::onJobAborted(const char *jobUID)
{
  if(!m_pipe) return;

 fputs("abort", m_pipe);
}
