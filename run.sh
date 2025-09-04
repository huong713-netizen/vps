#!/bin/bash
timestamp=$(date +"%Y%m%d-%H%M%S")
nohup ./playit > "playit-$timestamp.log" 2>&1 &
echo "✅ Playit đang chạy nền (log: playit-$timestamp.log)"
nohup ./proxy > "proxy-$timestamp.log" 2>&1 &
echo "✅ Proxy đang chạy nền (log: proxy-$timestamp.log)"
