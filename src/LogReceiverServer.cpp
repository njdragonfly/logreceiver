#include <errno.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include <muduo/base/Logging.h>

#include <LogReceiverServer.h>

using namespace lr;
using namespace muduo;

LogReceiverServer::LogReceiverServer(int flushInterval,
									 int writerThreadNum)
	: flushInterval_(flushInterval),
	  writerThreadNum_(writerThreadNum),
	  udpSockFD_(-1),
	  running_(false),
	  thread_(boost::bind(&LogReceiverServer::threadFunc, this), "LogReceiverServer"),
      latch_(1)
{
	logWriters_.reserve(writerThreadNum);
}

int LogReceiverServer::start(const char *ip, const int port)
{
	if ((udpSockFD_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		LOG_ERROR << "Create UDP socket failed, errno: " << errno;
		return -1;
	}

	struct sockaddr_in serverAddr;
	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(ip);

	if (bind(udpSockFD_, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
	{
		LOG_ERROR << "UDP socket bind failed, errno: " << errno;
		return -2;
	}

	struct timeval sendTimeout = {0, 100000}; //100ms
    if (setsockopt(udpSockFD_, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)) < 0)
    {
    	LOG_ERROR << "set socekt send timeout failed, errno: " << errno;
		return -3;
    }

    struct timeval recvTimeout = {10, 0}; //10s
    if (setsockopt(udpSockFD_, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout)) < 0)
    {
    	LOG_ERROR << "set socekt recv timeout failed, errno: " << errno;
		return -4;
    }

	for (int i = 0; i < writerThreadNum_; ++i)
	{
		AsyncLogWriterPtr logWriter(new AsyncLogWriter(flushInterval_));
		logWriter->start();
		logWriters_.push_back(logWriter);
	}

	running_ = true;
	thread_.start();
	latch_.wait();

	return 0;
}

void LogReceiverServer::threadFunc()
{
	assert(running_ == true);
	latch_.countDown();

	char recvBuf[65536];

	while (running_)
	{
		struct sockaddr_in clientAddr;
		socklen_t clientAddrLen;
		int recvNum = recvfrom(udpSockFD_, recvBuf, sizeof(recvBuf), 0, (struct sockaddr*) &clientAddr, &clientAddrLen);
		if (recvNum < 0)
		{
			if (errno == EAGAIN)
			{
				LOG_ERROR << "recvfrom timeout, errno: " << errno;
				continue;
			}
			else
			{
				LOG_ERROR << "recvfrom failed, errno: " << errno;
				break;
			}
		}
		else if (recvNum == 0)
		{
			LOG_ERROR << "recvfrom ret 0";
			break;
		}

		LOG_INFO << "recv packet!";
	}

	close(udpSockFD_);
	udpSockFD_ = -1;
}

void LogReceiverServer::stop()
{
	for (size_t i = 0; i < logWriters_.size(); ++i)
	{
		logWriters_[i]->stop();
	}

	logFileMap_.clear();

	running_ = false;
	thread_.join();
}

