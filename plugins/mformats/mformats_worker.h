#pragma once

#include <list>
#include <functional>
#include "workflow_helper.h"
#include "logger.h"

#ifdef MFORMATS_EXPORTS
  #define MFORMATS_API __declspec(dllexport)
#else
  #define MFORMATS_API __declspec(dllimport)
#endif

extern "C" MFORMATS_API void init_lib();
extern "C" MFORMATS_API void deinit_lib();
extern "C" MFORMATS_API const pluginWorker * register_workers();
extern "C" MFORMATS_API void log_set_callback_(void (*fn) (ELogSeverity severity, const char *message, void *_private), void *param);
extern "C" MFORMATS_API void * create_worker(const char *type);
extern "C" MFORMATS_API bool destroy_worker(void *w);
extern "C" MFORMATS_API void set_workflow(void *w, const char *pipe);
extern "C" MFORMATS_API bool load_worker(void *w, const char *param, const char *value);
extern "C" MFORMATS_API bool abort_job(void *w, const char *jobUID);
extern "C" MFORMATS_API bool process_job(void *w, const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted, const std::function<void(const char *, const char *)> &onWork);
