#include <iostream>
#include "workflow.h"
#include "rapidjson/writer.h"

const char START_DELIMITER[] = "A1ADF546";
const char END_DELIMITER[] = "277223DE";

class workflowStarter
{
  public:
  workflowStarter()
  {
    log_set_callback(workflowStarter::log_stdout, this);
    wf_set_callback(workflowStarter::notification_stdout, this);
  }

protected:
  static void log_stdout(ELogSeverity severity, const char *message, void *_private)
  {
    rapidjson::Document doc;

    doc.SetObject();

    rapidjson::Value v;
    std::string s("log_message");
    v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), doc.GetAllocator());
    doc.AddMember("message", v, doc.GetAllocator());

    rapidjson::Document argsDoc;
    argsDoc.SetObject();

    s = severityToText(severity);
    v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), argsDoc.GetAllocator());
    argsDoc.AddMember("severity", v, argsDoc.GetAllocator());

    s = message;
    v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), argsDoc.GetAllocator());
    argsDoc.AddMember("message", v, argsDoc.GetAllocator());

    doc.AddMember("args", argsDoc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::cout << START_DELIMITER << buffer.GetString() << END_DELIMITER << std::flush;
  }

  static void notification_stdout(const char *message, const char *args, void *_private)
  {
    rapidjson::Document doc;

    doc.SetObject();

    rapidjson::Value v;
    std::string s(message);
    v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), doc.GetAllocator());
    doc.AddMember("message", v, doc.GetAllocator());

    rapidjson::Document argsDoc;
    argsDoc.Parse(args);
    doc.AddMember("args", argsDoc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::cout << START_DELIMITER << buffer.GetString() << END_DELIMITER << std::flush;
  }

} s_starter;

#define TYPE_PARAM "type"
#define PIPE_PARAM "pipe"
#define NAME_PARAM "name"
#define UID_PARAM "uid"

std::list<std::string> split_template(const char *_pipe)
{
  std::list<std::string> l;
  std::string s(_pipe);
  std::string delimiter = " ";

  size_t pos = 0;
  while( (pos = s.find(delimiter)) != std::string::npos)
  {
    std::string token = s.substr(0, pos);
    l.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  if(s.length() > 0)
    l.push_back(s);

  return l;
}

workflow::workflow()
{

}

workflow::~workflow()
{
  stop();

  // itareta workers
  for(auto e : m_workers)
    worker::destroy(e);
  m_workers.clear();
}

bool workflow::load(const char *workflow)
{
  if(!workflow)
    return false;

  // parser error
  m_jWorkflow.Parse(workflow);
  if(m_jWorkflow.HasParseError())
    return false;

  // type error
  if( !m_jWorkflow.HasMember(TYPE_PARAM) || !m_jWorkflow[TYPE_PARAM].IsString() || _stricmp(m_jWorkflow[TYPE_PARAM].GetString(), "workflow") )
    return false;

  // template
  if(!m_jWorkflow.HasMember(PIPE_PARAM) || !m_jWorkflow[PIPE_PARAM].IsString() )
    return false;

  // name
  if(m_jWorkflow.HasMember(NAME_PARAM) && m_jWorkflow[NAME_PARAM].IsString())
    m_name = m_jWorkflow[NAME_PARAM].GetString();

  // uid
  if(m_jWorkflow.HasMember(UID_PARAM) && m_jWorkflow[UID_PARAM].IsString())
    m_uid = m_jWorkflow[UID_PARAM].GetString();

  const char *templ = m_jWorkflow[PIPE_PARAM].GetString();
  if(!templ)
    return false;

  // parse template string and create worker
  // name1 param1=value1 ! name2 param1=value1 name1. ! 0 ! name3
  std::list<std::string> l = split_template(templ);
  std::string delimiter = "!";
  worker *w = NULL;
  bool isDelimiter = false;
  std::string condition;
  for(auto e : l)
  {
    // delimiter
    if(!e.compare(delimiter))
      isDelimiter = true;
    else if(e[0] == '{')
      condition = e.substr(1, e.length() - 2);
    else
    {
      bool isWorker = (e.find("=") == std::string::npos);
      if(isWorker)
      {
        // previous worker
        worker *nw = NULL;
        if(e[e.length() - 1] == '.')
          nw = getWorkerByName(e.substr(0, e.length()-1).c_str());
        else
        {
          nw = worker::create(e.c_str(), this);
          if(nw)
            m_workers.push_back(nw);
          else
          {
            // TODO
          }
        }
        if(isDelimiter && nw && w)
        {
          w->setNextWorker(nw, condition.c_str());
          nw->setPrevWorker(w, condition.c_str());
        }
        w = nw;
        isDelimiter = false;
      }
      else
      {
       if(w)
         w->load(e.c_str());
      }
    }
  }

  return true;
}

bool workflow::start()
{
  if(m_status == EWorkflowStatus::WF_ST__Running)
    return true;

  // itareta workers
  for(auto e : m_workers)
    e->start();

  // state change
  m_status = EWorkflowStatus::WF_ST__Running;
  
  return true;
}

bool workflow::stop()
{
  if(m_status == EWorkflowStatus::WF_ST__Stopped)
    return true;

  // abort running jobs
  {
    std::unique_lock<std::recursive_mutex> lock(m_jobsMutex);
    for(auto j : m_jobs)
    {
      if(!j->isCompleted())
        j->abort();
    }
  }

  // iterate workers
  for(auto e : m_workers)
    e->stop();

  // clear jobs
  {
    std::unique_lock<std::recursive_mutex> lock(m_jobsMutex);
    for(auto e: m_jobs)
      delete e;
    m_jobs.clear();
  }

  // state change
  m_status = EWorkflowStatus::WF_ST__Stopped;

  return true;
}

bool workflow::addJob(job *j)
{
  // notify
  wf_notify("job_added", j->serialize().c_str());

  // add
  std::unique_lock<std::recursive_mutex> lock(m_jobsMutex);
  m_jobs.push_back(j);

  return true;
}

job * workflow::getJob(const char *jobUID)
{
  std::unique_lock<std::recursive_mutex> lock(m_jobsMutex);
  job *j = NULL;
  for(auto e : m_jobs)
  {
    if(!_stricmp(e->UID(), jobUID))
    {
      j = e;
      break;
    }
  }

  return j;
}

bool workflow::removeJob(const char *jobUID)
{
  // mark job as aborted. Workflow will remove it when current worker return job
  // case completed job can be removed
  job *j = getJob(jobUID);
  if(!j)
    return false;

  if(j->isCompleted())
  {
    // notify
    wf_notify("job_removed", j->serialize().c_str());

    m_jobs.remove(j);
    delete j;
  }
  else
    j->abort();

  return true;
}

worker * workflow::getWorkerByName(const char *name)
{
  for(auto e : m_workers)
  {
    if(!strcmp(e->name(), name))
      return e;
  }

  return NULL;
}

const char * workflow::pipe()
{
  return m_jWorkflow[PIPE_PARAM].GetString();

}
