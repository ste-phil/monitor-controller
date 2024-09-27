#!/bin/bash

# Usage: ./stop_iperf_client.sh <computer> <port>
# Example: ./stop_iperf_client.sh 192.168.28.245 5001

REMOTE_HOST=$1
PORT=$2

if [ -z "$PORT" ]; then
    echo "Please provide a port number."
    exit 1
fi

ssh $REMOTE_HOST "pkill -f "iperf3 -c .* -p $PORT""
echo "iperf3 client stopped on port $PORT"