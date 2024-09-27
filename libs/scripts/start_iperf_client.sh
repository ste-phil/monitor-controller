#!/bin/bash

# Usage: ./start_iperf_client.sh <computer> <server_host> <port> <udp> <bandwidth>
# Example: ./start_iperf_client.sh ssh-computer-ip server-ip 5001

REMOTE_HOST=$1
SERVER_HOST=$2
PORT=$3
L4S=$4
USE_UDP=${5:-"false"}
BANDWIDTH=$6

# Construct the base iperf command

if [ "$L4S" = "true" ]; then
  CCA_CMD="sudo sysctl net.ipv4.tcp_congestion_control=prague; sudo sysctl net.ipv4.tcp_ecn=3;"
else
  CCA_CMD="sudo sysctl net.ipv4.tcp_congestion_control=cubic; sudo sysctl net.ipv4.tcp_ecn=0;"
fi

IPERF_CMD="iperf3 -c $SERVER_HOST -p $PORT"
# Add UDP flag if USE_UDP is true
if [ "$USE_UDP" = "true" ]; then
  IPERF_CMD="$IPERF_CMD -u"
fi

# Add bandwidth only if provided
if [ -n "$BANDWIDTH" ]; then
  IPERF_CMD="$IPERF_CMD -b $BANDWIDTH"
fi

# Execute the command remotely
ssh "$REMOTE_HOST" "$CCA_CMD $IPERF_CMD &"
echo "iperf3 client started connecting to $SERVER_HOST:$PORT with UDP mode: $USE_UDP and bandwidth: ${BANDWIDTH:-not specified}"