#include <signal.h>

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
	LogReceiverServer server("./logs/", 3, 2);

	LOG_INFO << "log_receiver starting...";
	int ret = server.start("0.0.0.0", 30003);
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

