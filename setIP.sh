#!/bin/sh
IP=`ifconfig en0|grep "inet "| awk '{print $2}'`
VPN=`ifconfig utun10|grep "inet "| awk '{print $2}'`
MAC=`ifconfig en0|grep "ether "| awk '{print $2}'`
AUTH=SD12-XF67AD

wallyc 1109 text 1 75% 1% 4% 66666 1 IP : $IP
wallyc 1109 text 2 75% 6% 4% 66666 1 VPN : $VPN
wallyc 1109 text 3 75% 11% 4% 66666 1 MAC : $MAC
wallyc 1109 text 4 75% 16% 4% 66666 1 AUTH : $AUTH
