#define MODULE_NAME    "haproxy.so"

int connect_unix(const char *socketPath);
int connect_net(const char *host, int port);
int send_command(int sock, char *cmd);