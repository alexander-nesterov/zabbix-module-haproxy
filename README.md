## Loadable module for HAproxy

Note: Only *nix

Official Documentation: 

https://www.zabbix.com/documentation/4.0/manual/config/items/loadablemodules

Fisrt, you need to enable stats
```bash
stats socket /run/haproxy/admin.sock mode 666 level admin expose-fd listeners
stats timeout 30s
```
to check it:
```bash
echo "show stat" | socat /run/haproxy/admin.sock stdio
```
or
```perl
use strict;
use warnings;
use IO::Socket::UNIX;

my $SOCK_PATH = "/run/haproxy/admin.sock";
my $CMD = "show stat\n";

my $client = IO::Socket::UNIX->new(Type => SOCK_STREAM(), Peer => $SOCK_PATH);

print $client "$CMD";

while (my $line = <$client>) 
{
        print $line;
}
close($client);
```
or
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 2000
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

/*
to build it: gcc stats.c -o stats -Wall
to use it: ./stats "/run/haproxy/admin.sock" "show stat"
*/

int main(int argc, char **argv)
{
    char *socketPath = NULL;
    char *cmd = NULL;
    int sock;
    struct sockaddr_un addr;
    int ret;
    char buffer[BUFFER_SIZE];

    if (argc != 3)
    {
        printf(RED "%s\n", "Usage: stats <sock> <command>" RESET);
        return EXIT_FAILURE;
    }

    socketPath = argv[1];
    cmd = argv[2];

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock < 0)
    {
        printf(RED "%s\n", "Cannot create socket" RESET);
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

    ret = connect (sock, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if (ret == -1)
    {
        printf(RED "%s\n", "The server is down" RESET);
        return EXIT_FAILURE;
    }

    strcat(cmd, "\n");
    ret = write(sock, cmd, strlen(cmd));
    if (ret == -1)
    {
        printf(RED "%s\n", "Cannot write to socket" RESET);
        return EXIT_FAILURE;
    }

    ret = read(sock, buffer, BUFFER_SIZE);
    if (ret == -1)
    {
        printf(RED "%s\n", "Cannot read from socket" RESET);
        return EXIT_FAILURE;
    }
    printf("%s", buffer);

    close(sock);
    return EXIT_SUCCESS;
}
```
