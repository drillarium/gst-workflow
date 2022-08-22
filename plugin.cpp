#include <filesystem>
#if _WIN32
  #include <Windows.h>
#else
  #include <dlfcn.h> //dlopen
#endif
#include "plugin.h"
#include "workflow_helper.h" // pluginWorker
#include "workflow.h"

// filesystem
namespace fs = std::filesystem;

/*
 *
 */
struct SWFModules
{
  std::string moduleName;
  void *h;                      // library handle
  const pluginWorker *wl;      // from workers()
  // entry points
  void (* init)();
  void (* deinit)();
  const pluginWorker * (* workers) ();
  void (* log_set_callback) (void (*fn) (ELogSeverity severity, const char *message, void *_private), void *param);
  void * (* create) (const char *type);
  bool (* destroy) (void *worker);
  void (* set_workflow) (void *worker, const SWorkflowPad &wp);
  bool (* load) (void *worker, const char *param);
  bool (* abort) (void *worker, const char *jobUID);
  bool (* process) (void *worker, const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted, const std::function<void(const char *, const char *)> &onWork);
  
  bool valid()
  {
    return init && deinit && workers && log_set_callback && create && destroy && set_workflow && load && abort && process;
  }
};

void * openLib(const std::string &libName)
{
#if _WIN32
  return LoadLibraryA(libName.c_str());
#else
  return dlopen(libName.c_str(), RTLD_LAZY);
#endif
}

int closeLib(void *libHandle)
{
#if _WIN32
  return FreeLibrary((HMODULE)libHandle);
#else
  return dlclose(libHandle);
#endif
}

void * loadSymbol(void *libHandle, const char *sym)
{
#if _WIN32
  return (void *) GetProcAddress((HMODULE)libHandle, sym);
#else
  return dlsym(libHandle, sym);
#endif
}

void plugin_log(ELogSeverity severity, const char *message, void *_private)
{
  log_(severity, message);
}

/*
 * register class
 */
class plugin_register
{
public:
  plugin_register()
  {
    // plugin folder      
    std::string cp = fs::current_path().string() + "\\plugins";
    fs::create_directories(cp);
    const fs::path pathToShow{ cp };
    try
    {
      for(const auto &entry : fs::directory_iterator(pathToShow))
      {
        const auto filenameStr = entry.path().filename().string();
        if(entry.is_regular_file())
        {
          const auto ext = entry.path().extension().string();
#ifdef _DEBUG
          if(!ext.compare(".wfd"))
#else
          if(!ext.compare(".wf"))
#endif
          {
            SWFModules mod;
            mod.h = openLib(entry.path().string());
            mod.moduleName = filenameStr;
            if(mod.h)
            {
              // register
              mod.init = (void (*)()) loadSymbol(mod.h, "init_lib");
              mod.deinit = (void (*)()) loadSymbol(mod.h, "deinit_lib");
              mod.workers = (const pluginWorker * (*) ()) loadSymbol(mod.h, "register_workers");
              mod.log_set_callback = (void (*) (void (*) (ELogSeverity, const char *, void *), void *)) loadSymbol(mod.h, "log_set_callback");
              mod.create = (void * (*) (const char *)) loadSymbol(mod.h, "create_worker");
              mod.destroy = (bool (*) (void *)) loadSymbol(mod.h, "destroy_worker");
              mod.set_workflow = (void (*) (void *, const SWorkflowPad &wp)) loadSymbol(mod.h, "set_workflow");
              mod.load = (bool (*) (void *, const char *)) loadSymbol(mod.h, "load_worker");
              mod.abort = (bool (*) (void *, const char *)) loadSymbol(mod.h, "abort_job");
              mod.process = (bool (*) (void *, const char *, const std::function<void(int)> &, const std::function<void(const char *, const char *)> &, const std::function<void(const char *, const char *)> &)) loadSymbol(mod.h, "process_job");

              if(mod.valid())
              {
                // redirect log
                mod.log_set_callback(plugin_log, NULL);
               
                // init
                mod.init();

                // register
                mod.wl = mod.workers();
                const pluginWorker *w = mod.wl;
                while(w)
                {
                  // type
                  std::string type = w->name;

                  // register plugins
                  worker::register_worker(type.c_str(), [filenameStr, type] {
                    return new plugin(filenameStr.c_str(), type.c_str());
                  }, [](worker *w) {
                    delete w;
                  });

                  w = w->next;
                }

                m_modules.push_back(mod);
              }
              else
                closeLib(mod.h);
            }
          }
        }
      }
    }
    catch(...)
    {
      // folder does not exits!!!
    }
  }

  ~plugin_register()
  {
    for(auto e : m_modules)
    {
      e.deinit();
      closeLib(e.h);
    }
    m_modules.clear();
  }

  SWFModules * findModule(const char *name)
  {
    SWFModules *mod = NULL;
    for(auto &e : m_modules)
    {
      const pluginWorker *w = e.wl;
      while(w)
      {
        if(!w->name.compare(name))
          return &e;
        w = w->next;
      }
    }

    return NULL;
  }

protected:
  std::list<SWFModules> m_modules;

} s_modules;

/*
 *
 */
plugin::plugin(const char *module, const char *type)
:m_module(module)
{
  m_type = type;

  // create
  SWFModules *mod = s_modules.findModule(m_type.c_str());
  if(mod)
    m_context = mod->create(type);
}

plugin::~plugin()
{
  // destroy
  SWFModules *mod = s_modules.findModule(m_type.c_str());
  if(mod)
    mod->destroy(m_context);
  m_context = NULL;
}

bool plugin::processJob(job *j)
{
  if(!worker::processJob(j))
    return false;

  std::string condition;

  // has errors
  if(hasError(j))
  {
    // status
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100, "Error in previous worker");
    j->update(m_name.c_str(), status);
  }
  else
  {
    if(!m_context)
    {
      // status
      rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100, "Error invalid plugin");
      j->update(m_name.c_str(), status);
    }
    else
    {
      // status
      rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Running, 0);
      j->update(m_name.c_str(), status);

      // process job
      SWFModules *mod = s_modules.findModule(m_type.c_str());
      if(mod)
      {
        std::string jJob = j->serialize();
        mod->process(m_context, jJob.c_str(),        
        [this, &status, j] (int progress) {
          // progress
          status = buildStatus(EJobStatus::JOB_ST__Running, progress);
          j->update(m_name.c_str(), status);
        },
        [this, &status, &condition, j] (const char *error, const char *cond) {
          // check abort
          status = buildStatus(EJobStatus::JOB_ST__Completed, 100, j->aborted() ? "Aborted" : (error? error : ""));
          j->update(m_name.c_str(), status);

          condition = cond;
        },
        [this, j](const char *status, const char *work) {
          // notify work status
          rapidjson::Document doc;
          doc.Parse(work);
          if(!doc.HasParseError())
            j->updateWork(status, m_name.c_str(), doc);
        }
        );
      }
    }
  }

  propagateJob(j, condition.c_str());

  return true;
}

bool plugin::load(const char *param)
{
  bool ret = worker::load(param);
  if(ret)
  {
    SWFModules *mod = s_modules.findModule(m_type.c_str());
    if(mod)
      ret = mod->load(m_context, param);
  }

  return ret;
}

void plugin::onJobAborted(const char *jobUID)
{
  worker::onJobAborted(jobUID);
  SWFModules *mod = s_modules.findModule(m_type.c_str());
  if(mod)
    mod->abort(m_context, jobUID);
}

void plugin::onWorkflowParsed()
{ 
  SWorkflowPad wp = m_workflow->serializeWorkflow();
  SWFModules *mod = s_modules.findModule(m_type.c_str());
  if(mod)
    mod->set_workflow(m_context, wp);
}