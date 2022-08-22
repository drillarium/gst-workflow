#include "filesystemwatcher.h"
#include "workflow.h"

/*
 * register class
 */
class filesystemwatcher_register
{
public:
  filesystemwatcher_register() { worker::register_worker("filesystemwatcher", [] { return new filesystemwatcher; }, [](worker *w) { delete w; }); }
} s_register;

/*
 *
 */
filesystemwatcher::filesystemwatcher()
{
  m_type = "filesystemwatcher";
}

bool filesystemwatcher::load(const char *_param)
{
  return worker::load(_param);
}

bool filesystemwatcher::start()
{
  bool ret = worker::start();
  if(ret)
  {
    m_threadPool.queueJob([this] {
      // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
      FileWatcher fw{"./", std::chrono::milliseconds(5000)};
      m_fw = &fw;
 
      // Start monitoring a folder for changes and (in case of changes)
      // run a user provided lambda function
      fw.start([this] (std::string path_to_watch, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored
        if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased)
          return;

        switch(status)
        {
          case FileStatus::created:
          {
            log_info("File created : %s", path_to_watch.c_str());

            /* create job */
            job *j = new job;

            /* add work done */
            rapidjson::Document work = buildWork(status, path_to_watch.c_str());
            j->updateWork(DONE_WORK, m_name.c_str(), work);

            /* register in workflow */
            m_workflow->addJob(j);

            /* send to the nexts workers */
            propagateJob(j);
          }
          break;
          case FileStatus::modified:
          {
            log_info("File modified : %s", path_to_watch.c_str());

            /* create job */
            job *j = new job;

            /* add work done */
            rapidjson::Document work = buildWork(status, path_to_watch.c_str());
            j->updateWork(DONE_WORK, m_name.c_str(), work);

            /* register in workflow */
            m_workflow->addJob(j);

            /* send to the nexts workers */
            propagateJob(j);
          }
          break;
          case FileStatus::erased:
            log_info("File erased : %s", path_to_watch.c_str());
          break;
          default:
            log_warn("Error! Unknown file status");
        }
      });

      m_fw = NULL;
    });
  }

  return ret;
}

bool filesystemwatcher::stop()
{
  // stop filesystemwatcher
  if(m_fw)
    m_fw->stop();

  // stop worker
  worker::stop();

  return true;
}

static const char * fileStatusToText(FileStatus status)
{
  if (status == FileStatus::created) return "created";
  if (status == FileStatus::modified) return "modified";
  if (status == FileStatus::erased) return "erased";
  return "???";
}

rapidjson::Document filesystemwatcher::buildWork(FileStatus status, const char *file)
{
  rapidjson::Document doc;
  doc.SetObject();

  // name
  rapidjson::Value v;
  v.SetString(m_name.c_str(), (rapidjson::SizeType) m_name.length(), doc.GetAllocator());
  doc.AddMember("name", v, doc.GetAllocator());

  // type
  v.SetString(m_type.c_str(), (rapidjson::SizeType) m_type.length(), doc.GetAllocator());
  doc.AddMember("type", v, doc.GetAllocator());

  // file
  std::string f(file);
  v.SetString(f.c_str(), (rapidjson::SizeType) f.length(), doc.GetAllocator());
  doc.AddMember("file", v, doc.GetAllocator());

  // status
  std::string s(fileStatusToText(status));
  v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), doc.GetAllocator());
  doc.AddMember("status", v, doc.GetAllocator());

  return doc;
}
