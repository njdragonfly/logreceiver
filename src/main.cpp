#include <signal.h>
#include <dirent.h>

#include <iostream>

#include <muduo/base/Logging.h>

#include <LogReceiverServer.h>

using namespace lr;
using namespace std;

typedef sighandler_t SignalHandler;

static volatile bool s_signal_quit = false;
static volatile bool g_graceful_quit_on_sigterm = true;
static SignalHandler s_prev_sigint_handler = NULL;
static SignalHandler s_prev_sigterm_handler = NULL;

static void quit_handler(int signo) {
    s_signal_quit = true;
    LOG_INFO << "SIGINT singal Received!";
    if (SIGINT == signo && s_prev_sigint_handler)
    {
        s_prev_sigint_handler(signo);
    }
    if (SIGTERM == signo && s_prev_sigterm_handler)
    {
        s_prev_sigterm_handler(signo);
    }
}

static pthread_once_t register_quit_signal_once = PTHREAD_ONCE_INIT;

static void RegisterQuitSignalOrDie()
{
    // Not thread-safe.
    SignalHandler prev = signal(SIGINT, quit_handler);
    if (prev != SIG_DFL &&
        prev != SIG_IGN)
    { // shell may install SIGINT of background jobs with SIG_IGN
        if (prev == SIG_ERR)
        {
            LOG_ERROR << "Fail to register SIGINT, abort";
            abort();
        }
        else
        {
            s_prev_sigint_handler = prev;
            LOG_WARN << "SIGINT was installed with " << prev;
        }
    }

    if (g_graceful_quit_on_sigterm)
    {
        prev = signal(SIGTERM, quit_handler);
        if (prev != SIG_DFL &&
            prev != SIG_IGN)
        { // shell may install SIGTERM of background jobs with SIG_IGN
            if (prev == SIG_ERR)
            {
                LOG_ERROR << "Fail to register SIGTERM, abort";
                abort();
            }
            else
            {
                s_prev_sigterm_handler = prev;
                LOG_WARN << "SIGTERM was installed with " << prev;
            }
        }
    }
}

bool IsAskedToQuit()
{
    pthread_once(&register_quit_signal_once, RegisterQuitSignalOrDie);
    return s_signal_quit;
}

void AskToQuit()
{
    raise(SIGINT);
}

int main(int argc, const char *argv[])
{
	if (argc < 6)
	{
		LOG_ERROR << "Usage: log_receiver ip port base_dir flush_interval thread_num";
		return 0;
	}

	const char* bindIP = argv[1];
	int bindPort = atoi(argv[2]);
	muduo::string baseDir(argv[3]);
	int flushInterval = atoi(argv[4]);
	int writerThreadNum = atoi(argv[5]);

	if (baseDir.empty())
	{
		LOG_ERROR << "base_dir is empty";
		return 0;
	}

	if (baseDir[baseDir.length() - 1] != '/')
	{
		baseDir.append("/");
	}

	DIR* baseDirPtr = NULL;
	if ( (baseDirPtr = opendir(baseDir.c_str())) == NULL )
	{
		LOG_ERROR << "Directory not exists: " << baseDir;
		return 0;
	}
	else
	{
		closedir(baseDirPtr);
		baseDirPtr = NULL;
	}

	LogReceiverServer server(baseDir, flushInterval, writerThreadNum);

	LOG_INFO << "log_receiver starting...";
	int ret = server.start(bindIP, bindPort);
	LOG_INFO << "log_receiver started, ret: " << ret;

	while (!IsAskedToQuit())
	{
        sleep(1);
    }

    LOG_INFO << "log_receiver stoping..., please wait a moment";
    server.stop();
    LOG_INFO << "log_receiver stoped";

	return 0;
}

