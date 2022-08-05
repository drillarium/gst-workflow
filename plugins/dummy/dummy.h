#pragma once

#include "plugin_helper.h"
#include "logger.h"

#ifdef DUMMY_EXPORTS
  #define DUMMY_API __declspec(dllexport)
#else
  #define DUMMY_API __declspec(dllimport)
#endif

extern "C" DUMMY_API void init_lib();
extern "C" DUMMY_API void deinit_lib();
extern "C" DUMMY_API const SPluginWorker * register_workers();
extern "C" DUMMY_API void log_set_callback(void (*fn) (ELogSeverity severity, const char *message, void *_private), void *param);
extern "C" DUMMY_API void * create_worker(const char *type);
extern "C" DUMMY_API bool destroy_worker(void *w);
extern "C" DUMMY_API bool start_worker(void *w);
extern "C" DUMMY_API bool stop_worker(void *w);
extern "C" DUMMY_API bool load_worker(void *w, const char *param);
