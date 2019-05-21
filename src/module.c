#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"
#include "version.h"

static int item_timeout = 0;
static int zbx_module_haproxy_backend_autodiscovery(AGENT_REQUEST *request, AGENT_RESULT *result);

static ZBX_METRIC keys[] =
/*                KEY                    FLAG                FUNCTION              TEST PARAMETERS */
{
    {"haproxy.backend.autodiscovery", CF_HAVEPARAMS, zbx_module_haproxy_backend_autodiscovery, NULL},
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
static int zbx_module_haproxy_backend_autodiscovery(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    const char *__function_name = "zbx_module_haproxy_backend_autodiscovery";

    zabbix_log(LOG_LEVEL_DEBUG, 
               "Module: %s - param num: %d (%s:%d)",
               MODULE_NAME, request->nparam, __FILE__, __LINE__);

    if (3 != request->nparam)
    {
        SET_MSG_RESULT(result, strdup("Invalid number of parameters, see log"));

        zabbix_log(LOG_LEVEL_DEBUG, "Module: %s, function: %s - invalid number of parameters (%s:%d)",
                   MODULE_NAME, __function_name, __FILE__, __LINE__);

        return SYSINFO_RET_FAIL;
    }

    return SYSINFO_RET_OK;
}


