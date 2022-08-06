## raw_proxy

<div align="center">
  <h1><code>raw_proxy</code></h1>
  <p>
    <a href="https://img.shields.io/badge/version-1.0.0-blue" alt="version">
      <img src="https://img.shields.io/badge/version-1.0.0-blue"/>
    </a>
    <a href="https://img.shields.io/badge/license-Apache-brightgreen" alt="Apache">
      <img src="https://img.shields.io/badge/license-Apache-brightgreen">
    </a>
  </p>
  <p>
    <strong>四层反向代理实现，透传客户端真实ip</strong>
  </p>
</div>

## 说明 

raw_proxy 是一款开源四层反向代理软件，利用数据包劫持、篡改技术实现数据包代理转发，业务端可以获取到真实的客户端ip，对业务无侵入。  

nginx七层反向代理，可通过设置`X-Forwarded-For`头部传递客户端ip。  
常用四层反向代理，业务端只能获取到代理ip，需要设计私有协议，对业务侧有侵入。 

## 目录
- [介绍](#介绍)
- [编译](#编译)


## 介绍
raw_proxy 是一款反向代理软件，包含代理服务`raw_proxy` 和 nat服务`nat_server`  

`raw_proxy`劫持网卡流量，通过udp隧道转发到目标服务器，便实现将原始L3层数据包（包含客户端ip）透传到后端  

`nat_server`用于隧道解包，数据包篡改，通过与tun虚拟设备交互，达到收发包目的


## 编译 

- 安装依赖  
`yum install -y libpcap-devel yaml-cpp-devel`

- 编译  
`sh build.sh`

## 使用

![alt text](imgs/pktflow.png "Network Topology")  

## 配置

1. 业务服务器B  

配置策略路由：  
`sh config/setup_tun.sh`

启动测试http服务，主要用于显示客户端ip：  
`./fakehttp -p tcp -a 172.29.11.1:12345`  

启动nat_server：  
`./nat_server -h "0.0.0.0" -p 62000`

2. 代理服务器A  

drop代理端口，防止内核回包：  
`iptables -I INPUT -p tcp --dport 8080 -j DROP`
  
在当前bin目录生成代理配置：
```yaml  
# vim ./conf/app.yaml

Eth: eth1       # 通讯网卡
Port: 8080      # 代理监听网卡
NatServer: 192.168.1.1  # 目标服务器ip
NatPort: 62000          # 目标服务器端口
RealServer: 172.29.11.1 # 业务服务内网地址
RealPort: 12345         # 业务服务监听端口
```

启动代理服务：  
`./raw_proxy`  


3. 代理效果 

访问代理服务器A对外8080端口：
`curl http://11.2.3.4:8080`  

目标服务器B测试服务打印：
from sip=[101.2.3.4:33841]  
