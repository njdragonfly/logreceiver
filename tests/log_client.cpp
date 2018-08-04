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

int main(int argc, const char* argv[])
{
	if (argc < 5)
	{
		LOG_ERROR << "Usage: log_client servr_ip server_port send_num send_string";
		return 0;
	}

	int clientFD = -1;
	if ((clientFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		LOG_ERROR << "Create client UDP socket failed, errno: " << errno;
		return -1;
	}

	struct timeval sendTimeout = {0, 100000}; //100ms
    if (setsockopt(clientFD, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)) < 0)
    {
    	close(clientFD);
		clientFD = -1;
    	LOG_ERROR << "set socekt send timeout failed, errno: " << errno;
		return -2;
    }

    struct timeval recvTimeout = {10, 0}; //10s
    if (setsockopt(clientFD, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout)) < 0)
    {
    	close(clientFD);
		clientFD = -1;
    	LOG_ERROR << "set socekt recv timeout failed, errno: " << errno;
		return -3;
    }

    const char* serverIP = argv[1];
    int serverPort = atoi(argv[2]);
    int sendNum = atoi(argv[3]);

    muduo::string sendBuf(argv[4]);
    sendBuf.append("\n");

    struct sockaddr_in serverAddr;
	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	socklen_t serverAddrLen = sizeof(serverAddr);

	struct sockaddr_in srcAddr;
	bzero(&srcAddr, sizeof(srcAddr));
	socklen_t srcAddrLen = sizeof(srcAddr);

	char recvBuf[65536];

	while (sendNum-- > 0)
	{
		if (sendto(clientFD, sendBuf.data(), sendBuf.length(), 0, (struct sockaddr *) &serverAddr, serverAddrLen) < 0)
		{
			LOG_ERROR << "sendto failed, errno: " << errno;
			break;;
		}

		int recvNum = recvfrom(clientFD, recvBuf, sizeof(recvBuf), 0, (struct sockaddr*) &srcAddr, &srcAddrLen);
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
			LOG_ERROR << "recvfrom ret 0, server socket may shutdown";
			break;
		}
		else
		{
			muduo::string recvStr(recvBuf, recvNum);
			LOG_DEBUG << "Send SUCC, ret: " << recvStr;
		}
	}

	close(clientFD);
	clientFD = -1;

	return 0;
}