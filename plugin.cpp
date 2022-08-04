#include <filesystem>
#if _WIN32
  #include <Windows.h>
#else
  #include <dlfcn.h> //dlopen
#endif
#include "plugin.h"
#include "plugin_helper.h"

// filesystem
namespace fs = std::filesystem;

/*
 *
 */
struct SWFModules
{
  void *h;      // library handle
  // entry points
  void (* init)();
  void (* deinit)();
  const SPluginWorker * (* workers) ();
  
  bool valid()
  {
    return init && deinit && workers;
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
            if(mod.h)
            {
              // register
              mod.init = (void (*)()) loadSymbol(mod.h, "init_lib");
              mod.deinit = (void(*)()) loadSymbol(mod.h, "deinit_lib");
              mod.workers = (const SPluginWorker * (*) ()) loadSymbol(mod.h, "register_workers");

              if(mod.valid())
              {
                // init
                mod.init();

                // register
                const SPluginWorker *w = mod.workers();

                // type
                std::string type;

                // register plugins
                worker::register_worker("delay", [filenameStr, type] {
                  return new plugin(filenameStr.c_str(), type.c_str());
                }, [](worker *w) {
                  delete w;
                });
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
  return true;
}
