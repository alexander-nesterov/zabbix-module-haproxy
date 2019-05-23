## Loadable module for HAproxy

Note: Only *nix

Official Documentation: 

https://www.zabbix.com/documentation/4.0/manual/config/items/loadablemodules

http://cbonte.github.io/haproxy-dconv/1.9/configuration.html#3.1-stats%20socket

Fisrt, you need to enable stats
```bash
stats socket /run/haproxy/admin.sock mode 666 level admin
#or
stats socket 127.0.0.1:9999
stats timeout 10s
```
to check it:
```bash
echo "show stat" | socat /run/haproxy/admin.sock stdio
```
or you can use it: [checks](https://github.com/alexander-nesterov/zabbix_module_haproxy/tree/master/check)
