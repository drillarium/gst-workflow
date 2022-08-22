#pragma once

#include <stdio.h>
#include <stdarg.h>

enum ELogSeverity
{
  LOG_SEV_DEBUG,
  LOG_SEV_INFO,
  LOG_SEV_WARNING,
  LOG_SEV_ERROR,
  LOG_SEV_CRITICAL
};

const char * severityToText(ELogSeverity severity);
void wf_log_set_callback(void(*fn) (ELogSeverity severity, const char *message, void *_private), void *param);

void log_(ELogSeverity severity, const char *message);
void log_critical(const char *format, ...);
void log_error(const char *format, ...);
void log_warn(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);
