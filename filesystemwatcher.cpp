#include "workflow.h"
#include "worker.h"
#include "filewatcher.h"

class filesystemwatcher : public worker
{
public:
  filesystemwatcher()
  {
    m_type = "filesystemwatcher";
  }

  bool start()
  {
    bool ret = worker::start();
    if(!ret)
      return false ;

    m_threadPool.queueJob([this] {
      // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
      FileWatcher fw{ m_path.c_str(), std::chrono::milliseconds(5000)};
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
            j->setName(path_to_watch.c_str());

            /* add work */
            updateWork(j, status, path_to_watch.c_str());

            /* register in workflow */
            m_workflow->addJob(j);

            /* send to the nexts workers */
            pushJob(j);
          }
          break;
          case FileStatus::modified:
          {
            log_info("File modified : %s", path_to_watch.c_str());

            /* create job */
            job *j = new job;
            j->setName(path_to_watch.c_str());

            /* add work*/
            updateWork(j, status, path_to_watch.c_str());

            /* register in workflow */
            m_workflow->addJob(j);

            /* send to the nexts workers */
            pushJob(j);
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

    return true;
  }

  bool doJob(job *j, std::string &condition, std::string &error) { return true; }

  bool stop()
  {
    // stop filesystemwatcher
    if(m_fw)
      m_fw->stop();

    // stop worker
    return worker::stop();
  }

  bool load(const char *param, const char *value)
  {
    if(worker::load(param, value))
      return true;

    if(!_stricmp(param, "path"))
    {
      m_path = value;
      return true;
    }

    return false;
  }

protected:
  static const char * fileStatusToText(FileStatus status)
  {
    if(status == FileStatus::created) return "created";
    if(status == FileStatus::modified) return "modified";
    if(status == FileStatus::erased) return "erased";
    return "???";
  }

  bool updateWork(job *j, FileStatus status, const char *file)
  {
    rapidjson::Document doc = updateStatus();

    rapidjson::Document work;
    work.SetObject();

    // file
    rapidjson::Value v;
    std::string f(file);
    v.SetString(f.c_str(), (rapidjson::SizeType) f.length(), work.GetAllocator());
    work.AddMember("file", v, work.GetAllocator());

    // status
    std::string s(fileStatusToText(status));
    v.SetString(s.c_str(), (rapidjson::SizeType) s.length(), work.GetAllocator());
    work.AddMember("status", v, work.GetAllocator());

    // work
    doc.AddMember("work", work, doc.GetAllocator());

    // update job
    j->updateStatus(m_name.c_str(), m_type.c_str(), doc);

    return true;
  }

protected:
  FileWatcher *m_fw = NULL;
  std::string m_path = "./";
};

/*
 * register class
 */
class filesystemwatcher_register
{
public:
  filesystemwatcher_register() { worker::register_worker("filesystemwatcher", "name,path", [] { return new filesystemwatcher; }, [](worker *w) { delete w; }); }
} s_register;

