# echo_server_epoll.c

基于 epoll 的 echo_server 测试 



```
# 编译
make

# 运行
./bin/echo_server_epoll 8000 
```



在另一个终端中 测试：

```
nc localhost 8000 # will connect to the echo server
```