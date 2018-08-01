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
	LogReceiverServer(const muduo::string& baseDir,
					  int flushInterval,
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
  	void process(const char* pktBuf, const int pktLen, const struct sockaddr_in& clientAddr, socklen_t clientAddrLen);

  	void clearExpiredLogFile(time_t now);

  	muduo::string baseDir_;
	int flushInterval_;
	int writerThreadNum_;

	int udpSockFD_;

	bool running_;
  	muduo::Thread thread_;
 	muduo::CountDownLatch latch_;

	std::vector<muduo::AsyncLogWriterPtr> logWriters_;
	typedef std::map<muduo::string, muduo::AsyncLogFilePtr> LogFileMap;
	LogFileMap logFileMap_;

	static const int kCheckIntervalRunCnt = 10000;
	static const int kCheckIntervalSeconds = 3600;
	static const int kLogFileExpiredSeconds = 7200;
};

}  // End of namespace lr ...

#endif

