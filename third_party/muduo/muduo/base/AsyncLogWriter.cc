#include <muduo/base/AsyncLogWriter.h>
#include <muduo/base/AsyncLogFile.h>

#include <muduo/base/LogFile.h>
#include <muduo/base/Timestamp.h>

#include <stdio.h>

using namespace muduo;

AsyncLogWriter::AsyncLogWriter(int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    thread_(boost::bind(&AsyncLogWriter::threadFunc, this), "AsyncLogWriter"),
    latch_(1),
    mutex_(),
    cond_(mutex_)
{
}

size_t AsyncLogWriter::addLogFile(const AsyncLogFileWeakPtr& logFile)
{
  muduo::MutexLockGuard lock(mutex_);

  size_t i = 0;
  for (; i < logFiles_.size(); ++i)
  {
    if (logFiles_[i].expired())
    {
      logFiles_[i] = logFile;
      return i;
    }
  }

  logFiles_.push_back(logFile);
  return i;
}

void AsyncLogWriter::removeLogFile(size_t idxInWriter)
{
  muduo::MutexLockGuard lock(mutex_);

  if (idxInWriter > logFiles_.size())
  {
    return;
  }

  logFiles_[idxInWriter].reset();

  for (std::vector<size_t>::iterator it = pendingFlushLogFiles_.begin(); it != pendingFlushLogFiles_.end(); ++it)
  {
    if (idxInWriter == *it)
    {
      it = pendingFlushLogFiles_.erase(it);
    }
  }
}

void AsyncLogWriter::notify(size_t idxInWriter)
{
  muduo::MutexLockGuard lock(mutex_);

  if (idxInWriter > logFiles_.size())
  {
    return;
  }

  for (size_t i = 0; i < pendingFlushLogFiles_.size(); ++i)
  {
    if (pendingFlushLogFiles_[i] == idxInWriter)
    {
      return;
    }
  }

  pendingFlushLogFiles_.push_back(idxInWriter);
  cond_.notify();
}

void AsyncLogWriter::threadFunc()
{
  assert(running_ == true);
  latch_.countDown();

  while (running_)
  {
    std::vector<AsyncLogFileWeakPtr> logFilesToFlush;

    {
      muduo::MutexLockGuard lock(mutex_);
      bool isTimeout = false;
      if (pendingFlushLogFiles_.empty())
      {
        isTimeout = cond_.waitForSeconds(flushInterval_);
      }

      if (isTimeout)
      {
        logFilesToFlush.assign(logFiles_.begin(), logFiles_.end());
        pendingFlushLogFiles_.clear();
      }
      else
      {
        for (size_t i = 0; i < pendingFlushLogFiles_.size(); ++i)
        {
          logFilesToFlush.push_back(logFiles_[pendingFlushLogFiles_[i]]);
        }
        pendingFlushLogFiles_.clear();
      }
    }

    for (size_t i = 0; i < logFilesToFlush.size(); ++i)
    {
      if (AsyncLogFilePtr logFile = logFilesToFlush[i].lock())
      {
        logFile->flush();
      }
    }

  }

}

