#ifndef MUDUO_BASE_ROLLFILE_H
#define MUDUO_BASE_ROLLFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{

namespace FileUtil
{
class AppendFile;
}

class RollFile : boost::noncopyable
{
 public:
  RollFile(const string& basename,
          off_t rollSize = 1000*1000*1000,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 10*1000*1000);
  ~RollFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  static string getLogFileName(const string& basename, time_t* now);

  const string basename_;
  const off_t rollSize_;
  const int flushInterval_;
  const int checkEveryN_;

  int count_;

  boost::scoped_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  boost::scoped_ptr<FileUtil::AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;
};

}
#endif  // MUDUO_BASE_RollFile_H
