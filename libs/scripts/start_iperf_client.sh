#!/bin/bash

# Usage: ./start_iperf_client.sh <computer> <server_host> <port>
# Example: ./start_iperf_client.sh ssh-computer-ip server-ip 5001

REMOTE_HOST=$1
SERVER_HOST=$2
PORT=$3

ssh $REMOTE_HOST "iperf3 -c $SERVER_HOST -p $PORT &"
echo "iperf3 client started connecting to $SERVER_HOST:$PORT"
