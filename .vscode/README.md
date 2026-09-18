# VS Code / Cursor ST-Link 烧录与调试

本工程在 WSL 中用 `arm-none-eabi-gcc` 编译，用 **ST-Link (SWD)** 烧录 STM32F103C8。

## 推荐扩展

- `marus25.cortex-debug`
- `llvm-vs-code-extensions.vscode-clangd`

## Ubuntu 软件包

```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch cmake ninja-build openocd
```

## 烧录方式

1. **STM32CubeProgrammer（推荐，走 Windows ST-Link 驱动）**  
   任务：`Flash firmware (ST-Link CubeProgrammer)`
2. **OpenOCD（ST-Link 已通过 usbipd 挂到 WSL 时）**  
   任务：`Flash firmware (ST-Link OpenOCD)`

若 CubeProgrammer 路径不同，改 `.vscode/settings.json` 里的 `metal.cubeProgrammer`。

## 调试

- `ST-Link Flash + GDB Debug (Inc Build)`：增量编译后用 OpenOCD 下载并调试
- `ST-Link Attach`：连接已在跑的目标，不重新下载

OpenOCD 需要能访问 ST-Link USB。WSL2 下可用：

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

## 工具路径

集中在 `.vscode/settings.json`：

```jsonc
"metal.armToolchainBin": "/usr/bin",
"metal.gdbPath": "/usr/bin/gdb-multiarch",
"metal.buildDir": "build/Debug",
"metal.targetName": "BASE_PROJECT"
```
