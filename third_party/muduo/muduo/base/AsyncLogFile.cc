#include <muduo/base/AsyncLogFile.h>
#include <muduo/base/RollFile.h>
#include <muduo/base/Timestamp.h>

#include <stdio.h>

using namespace muduo;

AsyncLogFile::AsyncLogFile(const string& basename,
                           off_t rollSize,
                           AsyncLogWriter* writer,
                           int flushInterval)
  : flushInterval_(flushInterval),
    basename_(basename),
    rollSize_(rollSize),
    writer_(writer),
    mutex_(),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_(),
    newBuffer1_(new Buffer),
    newBuffer2_(new Buffer),
    output_(basename, rollSize, false),
    buffersToWrite_(),
    lastFlushTime_(0)
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
  newBuffer1_->bzero();
  newBuffer2_->bzero();
  buffersToWrite_.reserve(16);

  idxInWriter_ = writer_->addLogFile(shared_from_this());
}

AsyncLogFile::~AsyncLogFile()
{
  writer_->removeLogFile(idxInWriter_);
  flush(true);
}

void AsyncLogFile::append(const char* logline, int len)
{
  muduo::MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len)
  {
    currentBuffer_->append(logline, len);
  }
  else
  {
    buffers_.push_back(currentBuffer_.release());

    if (nextBuffer_)
    {
      currentBuffer_ = boost::ptr_container::move(nextBuffer_);
    }
    else
    {
      currentBuffer_.reset(new Buffer); // Rarely happens
    }
    currentBuffer_->append(logline, len);

    writer_->notify(idxInWriter_);
  }
}

void AsyncLogFile::flush(bool force)
{
  assert(newBuffer1_ && newBuffer1_->length() == 0);
  assert(newBuffer2_ && newBuffer2_->length() == 0);
  assert(buffersToWrite_.empty());

  {
    muduo::MutexLockGuard lock(mutex_);
    time_t now = time(NULL);
    if (buffers_.empty() && lastFlushTime_ + flushInterval_ > now && !force)
    {
      return;
    }
    lastFlushTime_ = now;

    buffers_.push_back(currentBuffer_.release());
    currentBuffer_ = boost::ptr_container::move(newBuffer1_);
    buffersToWrite_.swap(buffers_);
    if (!nextBuffer_)
    {
      nextBuffer_ = boost::ptr_container::move(newBuffer2_);
    }
  }

  assert(!buffersToWrite_.empty());

  if (buffersToWrite_.size() > 25)
  {
    char buf[256];
    snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
             Timestamp::now().toFormattedString().c_str(),
             buffersToWrite_.size()-2);
    fputs(buf, stderr);
    output_.append(buf, static_cast<int>(strlen(buf)));
    buffersToWrite_.erase(buffersToWrite_.begin()+2, buffersToWrite_.end());
  }

  for (size_t i = 0; i < buffersToWrite_.size(); ++i)
  {
    // FIXME: use unbuffered stdio FILE ? or use ::writev ?
    output_.append(buffersToWrite_[i].data(), buffersToWrite_[i].length());
  }

  if (buffersToWrite_.size() > 2)
  {
    // drop non-bzero-ed buffers, avoid trashing
    buffersToWrite_.resize(2);
  }

  if (!newBuffer1_)
  {
    assert(!buffersToWrite_.empty());
    newBuffer1_ = buffersToWrite_.pop_back();
    newBuffer1_->reset();
  }

  if (!newBuffer2_)
  {
    assert(!buffersToWrite_.empty());
    newBuffer2_ = buffersToWrite_.pop_back();
    newBuffer2_->reset();
  }

  buffersToWrite_.clear();

  output_.flush();
}

