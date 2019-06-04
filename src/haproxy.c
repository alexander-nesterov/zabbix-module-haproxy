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
#include "haproxy.h"

#define ERROR -1

/******************************************************************************
******************************************************************************/
int connect_unix(const char *sockPath, int *sockOut)
{
    const char *__function_name = "connect_unix";
    int sock;
    struct sockaddr_un addrUn;
    int ret;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock < 0)
    {
        zabbix_log(LOG_LEVEL_TRACE,
                   "Module: %s, function: %s - Cannot create socket (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        return SYSINFO_RET_FAIL;
    }

    memset(&addrUn, 0, sizeof(struct sockaddr_un));
    addrUn.sun_family = AF_UNIX;
    zbx_strlcpy(addrUn.sun_path, sockPath, sizeof(addrUn.sun_path) - 1);

    ret = connect(sock, (struct sockaddr *) &addrUn, sizeof(addrUn));
    if (ret == ERROR)
    {
        zabbix_log(LOG_LEVEL_TRACE,
                   "Module: %s, function: %s - The server is down (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        return SYSINFO_RET_FAIL;
    }
    *sockOut = sock;

    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
int connect_net(const char *host, int port, int *sockOut)
{
    const char *__function_name = "connect_net";
    int sock;
    struct sockaddr_in addrIn;
    int ret;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        zabbix_log(LOG_LEVEL_TRACE,
                   "Module: %s, function: %s - Cannot create socket (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        return SYSINFO_RET_FAIL;
    }

    addrIn.sin_family = AF_INET;
    addrIn.sin_port = htons(port);
    inet_pton(AF_INET, host, &addrIn.sin_addr);

    ret = connect(sock, (struct sockaddr *) &addrIn, sizeof(addrIn));
    if (ret == ERROR)
    {
        zabbix_log(LOG_LEVEL_TRACE,
                   "Module: %s, function: %s - The server is down (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        close(sock);
        return SYSINFO_RET_FAIL;
    }
    *sockOut = sock;

    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
int send_command(int sock, char *cmd, char **data)
{
    const char *__function_name = "send_command";
    int ret;
    char buffer[BUFSIZ];

    ret = write(sock, cmd, strlen(cmd));
    if (ret == ERROR)
    {
        zabbix_log(LOG_LEVEL_TRACE,
                   "Module: %s, function: %s - Cannot write to socket (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        close(sock);
        return SYSINFO_RET_FAIL;
    }
    ret = read(sock, buffer, BUFSIZ);
    if (ret == ERROR)
    {
        zabbix_log(LOG_LEVEL_TRACE,
                   "Module: %s, function: %s - Cannot read from socket (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        close(sock);
        return SYSINFO_RET_FAIL;
    }
    close(sock);
    *data = buffer;

    return SYSINFO_RET_OK;
}