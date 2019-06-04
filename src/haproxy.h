#define MODULE_NAME    "haproxy.so"

int connect_unix(const char *sockPath, int *sockOut);
int connect_net(const char *host, int port, int *sockOut);
int send_command(int sock, char *cmd, char **data);