to build it:
```bash
gcc stats.c -o stats -Wall
```
to use it:
```bash
./stats "/run/haproxy/admin.sock" "show stat"
```
or
```bash
./stats 127.0.0.1 9999 "show stat"
```
