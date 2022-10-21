#include "logger.h"

const char * severityToText(ELogSeverity severity)
{
  switch(severity)
  {
    case LOG_SEV_DEBUG: return "debug";
    case LOG_SEV_INFO: return "info";
    case LOG_SEV_WARNING: return "warning";
    case LOG_SEV_ERROR: return "error";
    case LOG_SEV_CRITICAL: return "critical";
    default: break;
  }

  return "???";
}

static void(*s_fn) (ELogSeverity, const char *, void *_private) = NULL;
static void *s_fn_param = NULL;

void log_set_callback(void(*fn) (ELogSeverity severity, const char *message, void *_private), void *param)
{
  s_fn = fn;
  s_fn_param = param;
}

void log_(ELogSeverity severity, const char *message)
{
  if(s_fn)
    s_fn(severity, message, s_fn_param);
}

#define MAX_LOG_MESSAGE_SIZE 1024

void log_critical(const char *format, ...)
{
  char message[MAX_LOG_MESSAGE_SIZE];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(message, MAX_LOG_MESSAGE_SIZE - 1, format, argptr);
  va_end(argptr);

  log_(LOG_SEV_CRITICAL, message);
}

void log_error(const char *format, ...)
{
  char message[MAX_LOG_MESSAGE_SIZE];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(message, MAX_LOG_MESSAGE_SIZE - 1, format, argptr);
  va_end(argptr);

  log_(LOG_SEV_ERROR, message);
}

void log_warn(const char *format, ...)
{
  char message[MAX_LOG_MESSAGE_SIZE];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(message, MAX_LOG_MESSAGE_SIZE - 1, format, argptr);
  va_end(argptr);

  log_(LOG_SEV_WARNING, message);
}

void log_info(const char *format, ...)
{
  char message[MAX_LOG_MESSAGE_SIZE];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(message, MAX_LOG_MESSAGE_SIZE - 1, format, argptr);
  va_end(argptr);

  log_(LOG_SEV_INFO, message);
}

void log_debug(const char *format, ...)
{
  char message[MAX_LOG_MESSAGE_SIZE];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(message, MAX_LOG_MESSAGE_SIZE - 1, format, argptr);
  va_end(argptr);

  log_(LOG_SEV_DEBUG, message);
}
