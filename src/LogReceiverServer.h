#ifndef LR_SERVER_H
#define LR_SERVER_H

#include <vector>
#include <map>

#include <boost/noncopyable.hpp>

#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

#include <muduo/base/AsyncLogFile.h>
#include <muduo/base/AsyncLogWriter.h>

namespace lr {

class LogReceiverServer : boost::noncopyable
{
public:
	LogReceiverServer(int flushInterval,
					  int writerThreadNum);
	~LogReceiverServer()
	{
	    if (running_)
	    {
	    	stop();
	    }
	}

	int start(const char *ip, const int port);
	void stop();

private:
	// declare but not define, prevent compiler-synthesized functions
  	LogReceiverServer(const LogReceiverServer&);  // ptr_container
  	void operator=(const LogReceiverServer&);  // ptr_container

  	void threadFunc();

	int flushInterval_;
	int writerThreadNum_;
	int udpSockFD_;

	bool running_;
  	muduo::Thread thread_;
 	muduo::CountDownLatch latch_;

	std::vector<muduo::AsyncLogWriterPtr> logWriters_;
	std::map<std::string, muduo::AsyncLogFilePtr> logFileMap_;
};

}  // End of namespace lr ...

#endif

