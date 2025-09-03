#!/bin/bash

# Tạo thư mục config nếu chưa có
CONFIG_DIR="$HOME/.config/code-server"
mkdir -p "$CONFIG_DIR"

# Cài code-server và cloudflared nếu chưa có
if ! command -v code-server &>/dev/null; then
    echo "Installing code-server..."
    curl -fsSL https://code-server.dev/install.sh | sh
fi

if ! command -v cloudflared &>/dev/null; then
    echo "Installing cloudflared..."
    curl -L https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64 -o cloudflared
    chmod +x cloudflared
    sudo mv cloudflared /usr/local/bin/
fi

# Tạo config code-server nếu chưa có
CONFIG_FILE="$CONFIG_DIR/config.yaml"
if [ ! -f "$CONFIG_FILE" ]; then
cat > "$CONFIG_FILE" <<EOF
bind-addr: 127.0.0.1:8080
auth: none
cert: false
EOF
fi

# Chạy code-server ẩn background
echo "Starting code-server..."
nohup code-server --bind-addr 127.0.0.1:8080 --auth none --cert false > "$HOME/code-server.log" 2>&1 &

sleep 2

# Chạy cloudflared ẩn background, log vào file
echo "Starting cloudflared tunnel..."
LOG_FILE="$HOME/cloudflared.log"
nohup cloudflared tunnel --url http://127.0.0.1:8080 > "$LOG_FILE" 2>&1 &

# Chờ và lấy URL từ log
echo "Waiting for Cloudflared URL..."
URL=""
for i in {1..10}; do
    sleep 2
    URL=$(grep -o 'https://.*trycloudflare.com' "$LOG_FILE" | tail -n1)
    if [ -n "$URL" ]; then
        break
    fi
done

if [ -n "$URL" ]; then
    echo "✅ Your public VS Code URL: $URL/?folder=/home/runner/work/vps/vps"
else
    echo "⚠️ URL chưa có, kiểm tra log: $LOG_FILE"
fi
