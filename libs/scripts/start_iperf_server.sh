#!/bin/bash

# Usage: ./start_iperf_server.sh <remote_host> <port>
# Example: ./start_iperf_server.sh user@remote-ip 5001

REMOTE_HOST=$1
PORT=$2

ssh $REMOTE_HOST "iperf3 -s -p $PORT &"
echo "iperf3 server started on $REMOTE_HOST:$PORT"
