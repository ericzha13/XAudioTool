
# XAudioTool

一个音频处理相关的C++仓库，后续会实现：\
1、常用音频格式转换，用原生方法和ffmpeg都实现下。\
2、用于加密音频(网易云等音乐平台下载的加密音频格式)的解密[该功能不开放]

# Requirement

使用到的glog、等开源库，windows系统下最好安装vcpkg来安装相关的库。

1、电脑终端若不支持访问github需要在power shell设置下代理，能访问则忽略第一步：

```shell
$env:HTTP_PROXY="http://127.0.0.1:xxxx"
$env:HTTPS_PROXY="http://127.0.0.1:xxxx"
```

2、安装vcpkg：\

```shell
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
```

3、安装用到的库：glog和ffmpeg

```shell
vcpkg install glog ffmpeg
```

4、将vcpkg继承到visual studio里

```shell
vcpkg integrate install
```
