#include "notifier.h"

static void(*s_fn) (const char *, const char *, void *_private) = NULL;
static void *s_fn_param = NULL;

void wf_set_callback(void(*fn) (const char *message, const char *args, void *_private), void *param)
{
  s_fn = fn;
  s_fn_param = param;
}

void wf_notify(const char *message, const char *args)
{
  if (s_fn)
    s_fn(message, args, s_fn_param);
}
