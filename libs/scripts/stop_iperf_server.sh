#!/bin/bash

# Usage: ./stop_iperf_server.sh <remote_host> <port>
# Example: ./stop_iperf_server.sh user@remote-ip 5001

REMOTE_HOST=$1
PORT=$2

ssh $REMOTE_HOST "pkill -f 'iperf3 -s -p $PORT'"
echo "iperf3 server stopped on $REMOTE_HOST:$PORT"
