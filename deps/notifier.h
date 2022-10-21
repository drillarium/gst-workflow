#pragma once

#include <stdio.h>
#include <stdarg.h>

void wf_set_callback(void(*fn) (const char *message, const char *args, void *_private), void *param);

void wf_notify(const char *message, const char *args);
