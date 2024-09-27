#!/bin/bash

# Usage: ./start_iperf_server.sh <remote_host> <port>
# Example: ./start_iperf_server.sh user@remote-ip 5001

REMOTE_HOST=$1
PORT=$2
L4S=$3

if [ "$L4S" = "true" ]; then
  CCA_CMD="sudo sysctl net.ipv4.tcp_congestion_control=prague; sudo sysctl net.ipv4.tcp_ecn=3;"
else
  CCA_CMD="sudo sysctl net.ipv4.tcp_congestion_control=cubic; sudo sysctl net.ipv4.tcp_ecn=0;"
fi

IPERF_CMD="iperf3 -s -p $PORT"

ssh $REMOTE_HOST "nohup $CCA_CMD $IPERF_CMD > /dev/null 2>&1 &"
echo "iperf3 server started on $REMOTE_HOST:$PORT"
