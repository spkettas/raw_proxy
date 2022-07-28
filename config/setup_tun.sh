#!/bin/sh

echo 1 > /proc/sys/net/ipv4/ip_forward

ip tuntap add dev tun_hp mode tun
ip addr add 172.29.11.1/32 dev tun_hp
ip link set tun_hp up

ip route add default dev tun_hp table 200
ip rule add from 172.29.11.1/24 table 200

