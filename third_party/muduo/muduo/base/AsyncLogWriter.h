#ifndef MUDUO_BASE_ASYNCLOGWRITER_H
#define MUDUO_BASE_ASYNCLOGWRITER_H

#include <muduo/base/BlockingQueue.h>
#include <muduo/base/BoundedBlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/LogStream.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

class AsyncLogFile;

typedef boost::shared_ptr<muduo::AsyncLogFile> AsyncLogFilePtr;
typedef boost::weak_ptr<muduo::AsyncLogFile> AsyncLogFileWeakPtr;

class AsyncLogWriter : boost::noncopyable
{
 public:

  AsyncLogWriter(int flushInterval = 3);

  ~AsyncLogWriter()
  {
    if (running_)
    {
      stop();
    }
  }

  size_t addLogFile(const muduo::AsyncLogFileWeakPtr& logFile);
  void removeLogFile(size_t idxInWriter);
  void notify(size_t idxInWriter);

  void start()
  {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop()
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  // declare but not define, prevent compiler-synthesized functions
  AsyncLogWriter(const AsyncLogWriter&);  // ptr_container
  void operator=(const AsyncLogWriter&);  // ptr_container

  void threadFunc();

  const int flushInterval_;
  bool running_;
  muduo::Thread thread_;
  muduo::CountDownLatch latch_;
  muduo::MutexLock mutex_;
  muduo::Condition cond_;

  std::vector<muduo::AsyncLogFileWeakPtr> logFiles_;
  std::vector<size_t> pendingFlushLogFiles_;
};

typedef boost::shared_ptr<muduo::AsyncLogWriter> AsyncLogWriterPtr;

}
#endif  // MUDUO_BASE_AsyncLogWriter_H
