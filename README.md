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
