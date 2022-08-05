#include <filesystem>
#if _WIN32
  #include <Windows.h>
#else
  #include <dlfcn.h> //dlopen
#endif
#include "plugin.h"
#include "plugin_helper.h" // SPluginWorker

// filesystem
namespace fs = std::filesystem;

/*
 *
 */
struct SWFModules
{
  std::string moduleName;
  void *h;      // library handle
  // entry points
  void (* init)();
  void (* deinit)();
  const SPluginWorker * (* workers) ();
  void (* log_set_callback) (void (*fn) (ELogSeverity severity, const char *message, void *_private), void *param);
  
  bool valid()
  {
    return init && deinit && workers && log_set_callback;
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
              mod.workers = (const SPluginWorker * (*) ()) loadSymbol(mod.h, "register_workers");
              mod.log_set_callback = (void (*) (void (*) (ELogSeverity, const char *, void *), void *)) loadSymbol(mod.h, "log_set_callback");

              if(mod.valid())
              {
                // redirect log
                mod.log_set_callback(plugin_log, NULL);
               
                // init
                mod.init();

                // register
                const SPluginWorker *w = mod.workers();
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

protected:
  std::list<SWFModules> m_modules;

} s_register;

/*
 *
 */
plugin::plugin(const char *module, const char *type)
:m_module(module)
{
  m_type = type;
}

bool plugin::processJob(job *j)
{
  if(!worker::processJob(j))
    return false;

  // has errors
  if(hasError(j))
  {
    // status
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Completed, 100, "Error in previous worker");
    j->update(m_name.c_str(), status);
  }
  else
  {
    // status
    rapidjson::Document status = buildStatus(EJobStatus::JOB_ST__Running, 0);
    j->update(m_name.c_str(), status);

    // TODO: process job
    bool ok = false;

    // check abort
    status = buildStatus(EJobStatus::JOB_ST__Completed, 100, j->aborted() ? "Aborted" : "");
    j->update(m_name.c_str(), status);
  }

  propagateJob(j);

  return true;
}
