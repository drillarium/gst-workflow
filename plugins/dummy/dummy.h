#pragma once

#include <list>
#include <functional>
#include "workflow_helper.h"
#include "logger.h"

#ifdef DUMMY_EXPORTS
  #define DUMMY_API __declspec(dllexport)
#else
  #define DUMMY_API __declspec(dllimport)
#endif

extern "C" DUMMY_API void init_lib();
extern "C" DUMMY_API void deinit_lib();
extern "C" DUMMY_API const pluginWorker * register_workers();
extern "C" DUMMY_API void log_set_callback_(void (*fn) (ELogSeverity severity, const char *message, void *_private), void *param);
extern "C" DUMMY_API void * create_worker(const char *type);
extern "C" DUMMY_API bool destroy_worker(void *w);
extern "C" DUMMY_API void set_workflow(void *w, const char *workflow);
extern "C" DUMMY_API bool load_worker(void *w, const char *param, const char *value);
extern "C" DUMMY_API bool abort_job(void *w, const char *jobUID);
extern "C" DUMMY_API bool process_job(void *w, const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted, const std::function<void(const char *, const char *)> &onWork);
