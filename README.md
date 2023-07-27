# mymuduo11
用C++11实现muduo网络库的一部分net内容


## 编译
```cc
sudo sh build.sh
```

## 使用
查看`./example`目录下的EchoServer的例子
进入到`example`目录下使用`make`编译出可执行文件`EchoServer`
`./EchoServer`运行，在另一个终端窗口`telnet 127.0.0.1 8002`连接