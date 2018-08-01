#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <muduo/base/Logging.h>

#include <LogReceiverServer.h>

using namespace lr;
using namespace muduo;

LogReceiverServer::LogReceiverServer(const muduo::string& baseDir,
									 int flushInterval,
									 int writerThreadNum)
	: baseDir_(baseDir),
	  flushInterval_(flushInterval),
	  writerThreadNum_(writerThreadNum),
	  udpSockFD_(-1),
	  running_(false),
	  thread_(boost::bind(&LogReceiverServer::threadFunc, this), "LogReceiverServer"),
      latch_(1)
{
	logWriters_.reserve(writerThreadNum);
	srand((unsigned int) time(NULL));
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
	socklen_t serverAddrLen = sizeof(serverAddr);

	if (bind(udpSockFD_, (struct sockaddr *) &serverAddr, serverAddrLen) < 0)
	{
		LOG_ERROR << "UDP socket bind failed, errno: " << errno;
		return -2;
	}

	struct timeval sendTimeout = {0, 100000}; //100ms
    if (setsockopt(udpSockFD_, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)) < 0)
    {
    	close(udpSockFD_);
		udpSockFD_ = -1;
    	LOG_ERROR << "set socekt send timeout failed, errno: " << errno;
		return -3;
    }

    struct timeval recvTimeout = {10, 0}; //10s
    if (setsockopt(udpSockFD_, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout)) < 0)
    {
    	close(udpSockFD_);
		udpSockFD_ = -1;
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
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	bzero(&clientAddr, sizeof(clientAddr));

	int runCount = 0;
	time_t lastCheckTime = time(NULL);

	while (running_)
	{
		if (runCount++ >= kCheckIntervalRunCnt)
		{
			runCount = 0;
			time_t now = time(NULL);
			if (now - lastCheckTime >= kCheckIntervalSeconds)
			{
				LOG_INFO << "begin clear expired log file...";

				clearExpiredLogFile(now);
				lastCheckTime = now;
			}
		}

		int recvNum = recvfrom(udpSockFD_, recvBuf, sizeof(recvBuf), 0, (struct sockaddr*) &clientAddr, &clientAddrLen);
		if (recvNum < 0)
		{
			if (errno == EAGAIN)
			{
				LOG_DEBUG << "recvfrom timeout, errno: " << errno;
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
			LOG_ERROR << "recvfrom ret 0, socket may shutdown";
			break;
		}

		LOG_DEBUG << "recv packet from: " << inet_ntoa(clientAddr.sin_addr);

		process(recvBuf, recvNum, clientAddr, clientAddrLen);
	}

	close(udpSockFD_);
	udpSockFD_ = -1;
}

void LogReceiverServer::process(const char* pktBuf, const int pktLen, const struct sockaddr_in& clientAddr, socklen_t clientAddrLen)
{
	const char* clientIp = (const char*) inet_ntoa(clientAddr.sin_addr);

	int commaPos = 0;
	for (; commaPos < pktLen; ++commaPos)
	{
		if (pktBuf[commaPos] == ',')
		{
			break;
		}
	}

	if (commaPos == 0 || commaPos > 100 || commaPos >= pktLen - 1)
	{
		LOG_ERROR << "commaPos invalid: commaPos = " << commaPos << ", pktLen = " << pktLen;
		return;
	}

	muduo::string moduleName(pktBuf, commaPos);
	muduo::string logFileMapKey = moduleName.append("-").append(clientIp);

	AsyncLogFilePtr logFile;
	LogFileMap::iterator it = logFileMap_.find(logFileMapKey);
	if (it != logFileMap_.end()) {
		logFile = it->second;
	}
	else
	{
		muduo::string logFileName = baseDir_ + logFileMapKey;
		logFile = AsyncLogFilePtr(new AsyncLogFile(logFileName, 500*1000*1000, 3));

		AsyncLogWriterPtr usedWriter = logWriters_[rand() % logWriters_.size()];
		logFile->attachToLogWriter(usedWriter);

		logFileMap_.insert(std::make_pair(logFileMapKey, logFile));
	}

	logFile ->append(pktBuf + commaPos + 1, pktLen - commaPos - 1);

	if (sendto(udpSockFD_, "RECV SUCC", 9, 0, (struct sockaddr*) &clientAddr, clientAddrLen) < 0)
	{
		LOG_ERROR << "sendto failed, errno: " << errno;
	}
}

void LogReceiverServer::stop()
{
	logFileMap_.clear();

	for (size_t i = 0; i < logWriters_.size(); ++i)
	{
		logWriters_[i]->stop();
	}

	logWriters_.clear();

	running_ = false;
	thread_.join();
}

void LogReceiverServer::clearExpiredLogFile(time_t now)
{
	for (LogFileMap::iterator it = logFileMap_.begin(); it != logFileMap_.end(); ++it)
	{
		if ( (now - (it->second->getLastAppendTime())) >= kLogFileExpiredSeconds)
		{
			logFileMap_.erase(it);

			LOG_INFO << "clear a expired log file";
		}
	}
}

