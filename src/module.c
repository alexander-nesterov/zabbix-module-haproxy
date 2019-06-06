#include "haproxy.h"

/*
LOG_LEVEL_EMPTY   0 (none)
LOG_LEVEL_CRIT    1 (critical)
LOG_LEVEL_ERR     2 (error)
LOG_LEVEL_WARNING 3 (warning)
LOG_LEVEL_DEBUG   4 (debug)
LOG_LEVEL_TRACE   5 (trace)
*/

static int item_timeout = 0;

/* 
    autodiscovery 
*/
static int zbx_module_haproxy_frontend_autodiscovery(AGENT_REQUEST *request, AGENT_RESULT *result);
static int zbx_module_haproxy_backend_autodiscovery(AGENT_REQUEST *request, AGENT_RESULT *result);

/* 
    stat - report counters for each proxy and server 
    https://cbonte.github.io/haproxy-dconv/1.9/management.html#show%20stat
*/
static int zbx_module_haproxy_stat_csv(AGENT_REQUEST *request, AGENT_RESULT *result);      /* show stat */
static int zbx_module_haproxy_stat_json(AGENT_REQUEST *request, AGENT_RESULT *result);     /* show stat json */

/* 
    info - report information about the running process
    https://cbonte.github.io/haproxy-dconv/1.9/management.html#9.3-show%20info 
*/
static int zbx_module_haproxy_info_text(AGENT_REQUEST *request, AGENT_RESULT *result);     /* show info */
static int zbx_module_haproxy_info_json(AGENT_REQUEST *request, AGENT_RESULT *result);     /* show info json */

/* 
    pools -  report information about the memory pools usage
    https://cbonte.github.io/haproxy-dconv/1.9/management.html#show%20pools 
*/
static int zbx_module_haproxy_pools_text(AGENT_REQUEST *request, AGENT_RESULT *result);    /* show pools */

/* 
    activity - show per-thread activity stats (for support/developers)
    https://cbonte.github.io/haproxy-dconv/1.9/management.html#show%20activity 
*/
static int zbx_module_haproxy_activity_text(AGENT_REQUEST *request, AGENT_RESULT *result); /* show activity */

static ZBX_METRIC keys[] =
/*                    KEY                       FLAG                    FUNCTION               TEST PARAMETERS */
{
    {"haproxy.frontend.autodiscovery",  CF_HAVEPARAMS, zbx_module_haproxy_frontend_autodiscovery,  NULL},
    {"haproxy.backend.autodiscovery",   CF_HAVEPARAMS, zbx_module_haproxy_backend_autodiscovery,   NULL},
    {"haproxy.stat.csv",                CF_HAVEPARAMS, zbx_module_haproxy_stat_csv,                NULL},
    {"haproxy.stat.json",               CF_HAVEPARAMS, zbx_module_haproxy_stat_json,               NULL},
    {"haproxy.info.text",               CF_HAVEPARAMS, zbx_module_haproxy_info_text,               NULL},
    {"haproxy.info.json",               CF_HAVEPARAMS, zbx_module_haproxy_info_json,               NULL},
    {"haproxy.pools.text",              CF_HAVEPARAMS, zbx_module_haproxy_pools_text,              NULL},
    {"haproxy.activity.text",           CF_HAVEPARAMS, zbx_module_haproxy_activity_text,           NULL},
    {NULL}
};

/******************************************************************************
*                                                                            *
* Function: zbx_module_api_version                                           *
*                                                                            *
* Purpose: returns version number of the module interface                    *
*                                                                            *
* Return value: ZBX_MODULE_API_VERSION - version of module.h module is       *
*               compiled with, in order to load module successfully Zabbix   *
*               MUST be compiled with the same version of this header file   *
*                                                                            *
******************************************************************************/
int zbx_module_api_version(void)
{
    return ZBX_MODULE_API_VERSION;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_init                                                  *	
*                                                                            *	
* Purpose: the function is called on agent startup                           *	
*          It should be used to call any initialization routines             *	
*                                                                            *	
* Return value: ZBX_MODULE_OK - success                                      *	
*               ZBX_MODULE_FAIL - module initialization failed               *	
*                                                                            *	
* Comment: the module won't be loaded in case of ZBX_MODULE_FAIL             *
*                                                                            *	
******************************************************************************/
int zbx_module_init(void)
{
    srand(time(NULL));

    zabbix_log(LOG_LEVEL_INFORMATION, 
               "Module: %s - built with: Zabbix: %d.%d.%d (%s:%d)",
               MODULE_NAME, ZABBIX_VERSION_MAJOR, ZABBIX_VERSION_MINOR, ZABBIX_VERSION_PATCH, 
               __FILE__, __LINE__);

    return ZBX_MODULE_OK;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_uninit                                                *
*                                                                            *
* Purpose: the function is called on agent shutdown                          *
*          It should be used to cleanup used resources if there are any      *
*                                                                            *
* Return value: ZBX_MODULE_OK - success                                      *
*               ZBX_MODULE_FAIL - function failed                            *
*                                                                            *
******************************************************************************/
int zbx_module_uninit(void)
{
    return ZBX_MODULE_OK;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_item_list                                             *
*                                                                            *
* Purpose: returns list of item keys supported by the module                 *
*                                                                            *
* Return value: list of item keys                                            *
*                                                                            *
******************************************************************************/
ZBX_METRIC *zbx_module_item_list()
{
    return keys;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_item_timeout                                          *
*                                                                            *
* Purpose: set timeout value for processing of items                         *
*                                                                            *
* Parameters: timeout - timeout in seconds, 0 - no timeout set               *
*                                                                            *
******************************************************************************/
void zbx_module_item_timeout(int timeout)
{
    item_timeout = timeout;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_frontend_autodiscovery(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_frontend_autodiscovery";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show stat";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.frontend.autodiscovery["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.frontend.autodiscovery[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_backend_autodiscovery(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_backend_autodiscovery";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show stat";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.backend.autodiscovery["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.backend.autodiscovery[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_stat_csv(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_stat_csv";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show stat";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.stat.csv["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.stat.csv[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_stat_json(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_stat_json";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show stat json";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.stat.json["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.stat.json[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_info_text(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_info_text";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show info";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.info.text["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.info.textt[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_info_json(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_info_json";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show stat";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.info.json["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.info.json[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_pools_text(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_pools_text";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show pools";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.pools.text["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.pools.text[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_activity_text(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_activity_text";
    char *sockPath = NULL;
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show activity";
    char *data = NULL;

    if (request->nparam == 0 || request->nparam > 2)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        return SYSINFO_RET_FAIL;
    }

    if (request->nparam == 1)
    {
        /*
            key: haproxy.activity.text["/run/haproxy/stats.sock"]
        */
        sockPath = get_rparam(request, 0);

        ret = connect_unix(sockPath, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
            return SYSINFO_RET_FAIL;
        }
    }
    else if (request->nparam == 2)
    {
        /*
            key: haproxy.activity.text[192.168.1.100, 9999]
        */
        host = get_rparam(request, 0);
        param2 = get_rparam(request, 1);
        port = atoi(param2);

        ret = connect_net(host, port, &sock);
        if (ret != SYSINFO_RET_OK)
        {
            SET_MSG_RESULT(result, strdup("Cannot connect, see log for details"));
            return SYSINFO_RET_FAIL;
        }
    }

    ret = send_command(sock, cmd, &data);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot send command, see log for details"));
        return SYSINFO_RET_FAIL;
    }

    SET_STR_RESULT(result, zbx_strdup(NULL, data));
    return SYSINFO_RET_OK;
}
