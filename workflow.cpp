#include "workflow.h"

#define TYPE_PARAM "type"
#define TEMPL_PARAM "template"
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
    delete e;
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
  if(!m_jWorkflow.HasMember(TEMPL_PARAM) || !m_jWorkflow[TEMPL_PARAM].IsString() )
    return false;

  // name
  if(m_jWorkflow.HasMember(NAME_PARAM) && m_jWorkflow[NAME_PARAM].IsString())
    m_name = m_jWorkflow[NAME_PARAM].GetString();

  // uid
  if(m_jWorkflow.HasMember(UID_PARAM) && m_jWorkflow[UID_PARAM].IsString())
    m_uid = m_jWorkflow[UID_PARAM].GetString();

  const char *templ = m_jWorkflow[TEMPL_PARAM].GetString();
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

  // itareta workers
  for(auto e : m_workers)
    e->stop();

  // clear jobs
  for(auto e: m_jobs)
    delete e;
  m_jobs.clear();

  // state change
  m_status = EWorkflowStatus::WF_ST__Stopped;

  return true;
}

bool workflow::addJob(job *j)
{
  m_jobs.push_back(j);

  return true;
}

bool workflow::removeJob(const char *jobUID)
{
  // TODO

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
