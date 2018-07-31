#ifndef MUDUO_BASE_ASYNCLOGFILE_H
#define MUDUO_BASE_ASYNCLOGFILE_H

#include <muduo/base/BlockingQueue.h>
#include <muduo/base/BoundedBlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/LogStream.h>
#include <muduo/base/LogFile.h>
#include <muduo/base/AsyncLogWriter.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

class AsyncLogFile : boost::noncopyable,
                      public boost::enable_shared_from_this<AsyncLogFile>
{
 public:

  AsyncLogFile(const string& basename,
               off_t rollSize,
               AsyncLogWriter* writer,
               int flushInterval = 10);

  ~AsyncLogFile();

  void append(const char* logline, int len);
  void flush(bool force = false);

 private:

  // declare but not define, prevent compiler-synthesized functions
  AsyncLogFile(const AsyncLogFile&);  // ptr_container
  void operator=(const AsyncLogFile&);  // ptr_container

  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  typedef boost::ptr_vector<Buffer> BufferVector;
  typedef BufferVector::auto_type BufferPtr;

  const int flushInterval_;
  string basename_;
  off_t rollSize_;
  muduo::AsyncLogWriterPtr writer_;
  muduo::MutexLock mutex_;
  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;
  BufferPtr newBuffer1_;
  BufferPtr newBuffer2_;
  muduo::LogFile output_;
  BufferVector buffersToWrite_;
  time_t lastFlushTime_;
  size_t idxInWriter_;
};

}
#endif  // MUDUO_BASE_AsyncLogFile_H
