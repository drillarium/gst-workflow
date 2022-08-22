#include "mformats_worker.h"
#include "job.h"
#include <thread>
#include <atlbase.h> // CComPtr
#include "MFormats.h"
#include "MLProtect_MFormats SDK.(subscription_valid_until_25_May_2023).h"
#include "rapidjson/writer.h"

class MFormatsTranscoderWorker
{
public:
  bool load(const char *param)
  {
    std::string token, value;
    bool ret = parse_token(param, token, value);
    if(ret)
    {
      if(!token.compare("name"))
        m_name = value;

      // TODO: transcoding parameters
      //       output folder
    }

    return ret;
  }
  bool abort(const char *jobUID)
  {
    m_abort = true;
    return true;
  }
  bool process(const char *jobData, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted, const std::function<void(const char *, const char *)> &onWork)
  {
    // build job
    job j(jobData);

    // only one previous worker supported
    rapidjson::Document prevWork = j.getWork(DONE_WORK, m_prevWorkers.front().c_str());

    // process
    for(int i = 0; i < 100 && !m_abort; i += 10)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      // status
      onProgress(i);
    }

    // status
    onCompleted("" /* error */, "" /* condition */);

    // notify work done
    rapidjson::Document doc = buildWork();
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    onWork("done", buffer.GetString());

    return true;
  }
  void setWorkflow(const SWorkflowPad &workflow)
  {
    // workflow
    m_workflow = workflow;

    // prev workers
    searchPrevWorkers(m_workflow);
  }

protected:
  void searchPrevWorkers(const SWorkflowPad &wp)
  {
    for(auto n : wp.next)
    {
      if(!n.worker.compare(m_name))
        m_prevWorkers.push_back(wp.worker);
      else
        searchPrevWorkers(n);
    }
  }

protected:
  rapidjson::Document buildWork()
  {
    rapidjson::Document doc;
    doc.SetObject();

    // name
    rapidjson::Value v;
    v.SetString(m_name.c_str(), (rapidjson::SizeType) m_name.length(), doc.GetAllocator());
    doc.AddMember("name", v, doc.GetAllocator());

    // type
    std::string type("mf_transcoder");
    v.SetString(type.c_str(), (rapidjson::SizeType) type.length(), doc.GetAllocator());
    doc.AddMember("type", v, doc.GetAllocator());

    return doc;
  }

protected:
  std::string m_name;
  std::list<std::string> m_prevWorkers;   // prev workers name
  bool m_abort = false;
  SWorkflowPad m_workflow;                // workflow json data
};

/*
 *
 */
const pluginWorker trancoder = {
  "mf_transcoder",
  NULL
};

/*
 *
 */
void init_lib()
{
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if(SUCCEEDED(hr))
    hr = MFormatsSDKLic::IntializeProtection();

  log_info("mformats library init, hr: 0x%08x", hr);
}

void deinit_lib()
{
  MFormatsSDKLic::CloseProtection();

  log_info("mformats library deinit");
}

const pluginWorker * register_workers()
{
  return &trancoder;
}

void * create_worker(const char *type)
{
  if(!strcmp(type, "mf_transcoder"))
    return new MFormatsTranscoderWorker;

  return NULL;
}

bool destroy_worker(void *w)
{
  delete w;

  return true;
}

bool load_worker(void *w, const char *param)
{
  MFormatsTranscoderWorker *tw = static_cast<MFormatsTranscoderWorker *> (w);
  return tw->load(param);
}

void log_set_callback(void(*fn) (ELogSeverity severity, const char *message, void *_private), void *param)
{
  wf_log_set_callback(fn, param);
}

bool abort_job(void *w, const char *jobUID)
{
  MFormatsTranscoderWorker *tw = static_cast<MFormatsTranscoderWorker *> (w);
  return tw->abort(jobUID);
}

bool process_job(void *w, const char *job, const std::function<void(int)> &onProgress, const std::function<void(const char *, const char *)> &onCompleted, const std::function<void(const char *, const char *)> &onWork)
{
  MFormatsTranscoderWorker *tw = static_cast<MFormatsTranscoderWorker *> (w);
  return tw->process(job, onProgress, onCompleted, onWork);
}

void set_workflow(void *w, const SWorkflowPad &wp)
{
  MFormatsTranscoderWorker *tw = static_cast<MFormatsTranscoderWorker *> (w);
  return tw->setWorkflow(wp);
}
