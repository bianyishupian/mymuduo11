#include <sys/eventfd.h>    // eventfd
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "mymuduo11/net/eventloop.h"
#include "mymuduo11/net/poller.h"
#include "mymuduo11/net/channel.h"
#include "mymuduo11/base/logger.h"

__thread EventLoop *t_loopInThisThread = nullptr;

const int PollTimeMs = 10000;       // 10s

// TODO