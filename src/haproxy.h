#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "syscall.h"
/* zabbix */
#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"
#include "zbxjson.h"

#define MODULE_NAME    "haproxy.so"

int connect_unix(const char *sockPath, int *sockOut);
int connect_net(const char *host, int port, int *sockOut);
int send_command(int sock, char *cmd, char **data);