#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"
#include "version.h"
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

/* autodiscovery */
static int zbx_module_haproxy_frontend_autodiscovery_unix(AGENT_REQUEST *request, AGENT_RESULT *result);
static int zbx_module_haproxy_frontend_autodiscovery_net(AGENT_REQUEST *request, AGENT_RESULT *result);
static int zbx_module_haproxy_backend_autodiscovery_unix(AGENT_REQUEST *request, AGENT_RESULT *result);
static int zbx_module_haproxy_backend_autodiscovery_net(AGENT_REQUEST *request, AGENT_RESULT *result);

/* stat */
static int zbx_module_haproxy_stat_unix(AGENT_REQUEST *request, AGENT_RESULT *result);      /* show stat */
static int zbx_module_haproxy_stat_net(AGENT_REQUEST *request, AGENT_RESULT *result);       /* show stat */
/* https://www.zabbix.com/documentation/4.2/manual/appendix/items/preprocessing */
static int zbx_module_haproxy_stat_json_unix(AGENT_REQUEST *request, AGENT_RESULT *result); /* show stat json */
static int zbx_module_haproxy_stat_json_net(AGENT_REQUEST *request, AGENT_RESULT *result);  /* show stat json */

/* info */
static int zbx_module_haproxy_info_unix(AGENT_REQUEST *request, AGENT_RESULT *result);      /* show info */
static int zbx_module_haproxy_info_net(AGENT_REQUEST *request, AGENT_RESULT *result);       /* show info */
/* https://www.zabbix.com/documentation/4.2/manual/appendix/items/preprocessing */
static int zbx_module_haproxy_info_json_unix(AGENT_REQUEST *request, AGENT_RESULT *result); /* show info json */
static int zbx_module_haproxy_info_json_net(AGENT_REQUEST *request, AGENT_RESULT *result);  /* show info json */

/* pools */
static int zbx_module_haproxy_pools_unix(AGENT_REQUEST *request, AGENT_RESULT *result);      /* show pools */
static int zbx_module_haproxy_pools_net(AGENT_REQUEST *request, AGENT_RESULT *result);      /* show pools */

static ZBX_METRIC keys[] =
/*                    KEY                       FLAG                    FUNCTION                           TEST PARAMETERS */
{
    {"haproxy.frontend.autodiscovery.unix", CF_HAVEPARAMS, zbx_module_haproxy_frontend_autodiscovery_unix, NULL},
    {"haproxy.frontend.autodiscovery.net",  CF_HAVEPARAMS, zbx_module_haproxy_frontend_autodiscovery_net,  NULL},
    {"haproxy.backend.autodiscovery.unix",  CF_HAVEPARAMS, zbx_module_haproxy_backend_autodiscovery_unix,  NULL},
    {"haproxy.backend.autodiscovery.net",   CF_HAVEPARAMS, zbx_module_haproxy_backend_autodiscovery_net,   NULL},
    {"haproxy.stat.unix",                   CF_HAVEPARAMS, zbx_module_haproxy_stat_unix,                   NULL},
    {"haproxy.stat.net",                    CF_HAVEPARAMS, zbx_module_haproxy_stat_net,                    NULL},
    {"haproxy.stat.json.unix",              CF_HAVEPARAMS, zbx_module_haproxy_stat_json_unix,              NULL},
    {"haproxy.stat.json.net",               CF_HAVEPARAMS, zbx_module_haproxy_stat_json_net,               NULL},
    {"haproxy.info.unix",                   CF_HAVEPARAMS, zbx_module_haproxy_info_unix,                   NULL},
    {"haproxy.info.net",                    CF_HAVEPARAMS, zbx_module_haproxy_info_net,                    NULL},
    {"haproxy.info.json.unix",              CF_HAVEPARAMS, zbx_module_haproxy_info_json_unix,              NULL},
    {"haproxy.info.json.net",               CF_HAVEPARAMS, zbx_module_haproxy_info_json_net,               NULL},
    {"haproxy.pools.unix",                  CF_HAVEPARAMS, zbx_module_haproxy_pools_unix,                  NULL},
    {"haproxy.pools.net",                   CF_HAVEPARAMS, zbx_module_haproxy_pools_net,                   NULL},
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
               "Module: %s - build with agent: %d.%d.%d; OS: %s; Release: %s; Hostname: %s (%s:%d)",
               MODULE_NAME, ZABBIX_VERSION_MAJOR, ZABBIX_VERSION_MINOR, ZABBIX_VERSION_PATCH, 
               "", "", "",
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
static int zbx_module_haproxy_frontend_autodiscovery_unix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_frontend_autodiscovery_net(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_backend_autodiscovery_unix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_backend_autodiscovery_unix";
    char *sockPath = NULL;
    int ret;
    int sock;
    char *cmd = "show stat\n";
    char *data = NULL;

    if (1 != request->nparam)
    {
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log"));

        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);

        return SYSINFO_RET_FAIL;
    }

    /*
    key: haproxy.backend.autodiscovery.unix["/run/haproxy/admin.sock"]
    */
    sockPath = get_rparam(request, 0);

    ret = connect_unix(sockPath, &sock);
    if (ret != SYSINFO_RET_OK)
    {
        SET_MSG_RESULT(result, strdup("Cannot connect, see log"));
        return SYSINFO_RET_FAIL;
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
static int zbx_module_haproxy_backend_autodiscovery_net(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_backend_autodiscovery_net";
    char *host = NULL; 
    char *param2 = NULL;
    int port;
    int ret;
    int sock;
    char *cmd = "show stat\n";
    char *data = NULL;

    if (2 != request->nparam)
    {
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log for details"));

        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);

        return SYSINFO_RET_FAIL;
    }

    /*
    key: haproxy.backend.autodiscovery.net[192.168.1.00, 9999]
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
static int zbx_module_haproxy_stat_unix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_stat_net(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_stat_json_unix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_stat_json_net(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_info_unix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_info_net(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_info_json_unix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_info_json_net(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_pools_unix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

/******************************************************************************
******************************************************************************/
static int zbx_module_haproxy_pools_net(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    return SYSINFO_RET_OK;
}

