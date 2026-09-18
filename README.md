# Metal Detector Firmware

STM32F103C8T6 金属探测板固件：FreeRTOS + USB CDC + I2C OLED，用 **ST-Link (SWD)** 烧录。应用代码在 `User_APP/`，CubeMX 生成代码尽量不要覆盖。

仓库：https://github.com/Jiangguangfu/Metal-Detector-Project

## 硬件

| 项目 | 规格 |
|---|---|
| MCU | STM32F103C8T6，72 MHz，64 KB Flash / 20 KB RAM |
| 调试 | ST-Link SWD（PA13 / PA14） |
| USB | Type-C FS CDC（PA11 DM / PA12 DP） |
| 显示 | 0.91" SSD1306 128×32，I2C1 PB6 SCL / PB7 SDA，地址 `0x3C`（失败会试 `0x3D`） |
| 探头 | LC + LM393，`OUT_LC` → PA9 `TIM_IN` |

### 引脚

| 功能 | 引脚 | 说明 |
|---|---|---|
| SW1 / SW2 / SW3 | PA0 / PA1 / PA2 | 外部下拉，按下为高 |
| LED1 / LED2 | PA3 / PA4 | 低电平点亮 |
| POWER_ADC | PA5 | ADC1_IN5，见下方电压采样 |
| BUZZER | PA8 TIM1_CH1 | Q1 高电平导通，谐振约 2.7 kHz |
| TIM_IN | PA9 TIM1_CH2 | 探头方波输入捕获 |
| OLED I2C | PB6 / PB7 | 4.7k 上拉，400 kHz |
| USB | PA11 / PA12 | 虚拟串口 |
| SWD | PA13 / PA14 | ST-Link |

TIM1 共用：固定 1 MHz 计数器。CH1 输出比较翻转驱动蜂鸣器（50% 占空比），CH2 每 8 个上升沿捕获一次，测量 LC 频率。

### 上电行为

- 延时 2 s 后开机扫频：500 Hz → 1 kHz，约 1 s
- LED1 心跳；LED2 跟随 USB 连接
- OLED 显示电压、USB、LC 频率、按键
- K1 单击蜂鸣，长按旋律；K2 / K3 切换 LED1 / LED2

### 电压采样注意

原理图 `POWER_ADC` 为 R32=0 Ω 串联 + R31=10k 对地（1:1）。`POWER_IN` 为 5 V，F103 模拟脚不耐受 5 V。建议把 **R32 换成 10k** 做成 2:1，并把 `USER_CONFIG_ADC_DIV_RTOP_OHM` 改为 `10000`。

板级参数集中在 `User_APP/inc/user_config.h`。

## 目录

```
User_APP/          应用任务（按键 / LED / ADC / 蜂鸣器 / OLED / CDC / 探头捕获）
Core/              CubeMX：GPIO、ADC、I2C、TIM1、FreeRTOS
USB_DEVICE/        USB FS CDC
cmake/             工具链与 User_APP 链接
.vscode/           ST-Link 编译、烧录、调试任务
tools/scripts/     OpenOCD / CubeProgrammer 烧录脚本
```

## 编译

依赖（WSL / Ubuntu）：

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch cmake ninja-build openocd
```

```bash
cmake -S . -B build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/Debug --target BASE_PROJECT --parallel
```

产物：`build/Debug/BASE_PROJECT.elf`（同时生成 `.hex` / `.bin`）。

F103C8 容量紧，Debug 使用 `-Os -g3`。当前大约 Flash 72%、RAM 80%。Cursor / VS Code 默认构建任务：`Build firmware inc (arm-none-eabi)`。

## 烧录与调试

推荐扩展：`marus25.cortex-debug`、`llvm-vs-code-extensions.vscode-clangd`。

| 方式 | 任务 / 脚本 |
|---|---|
| CubeProgrammer（推荐，走 Windows ST-Link 驱动） | `Flash firmware (ST-Link CubeProgrammer)` |
| OpenOCD（ST-Link 已挂进 WSL） | `Flash firmware (ST-Link OpenOCD)` |
| 调试 | `ST-Link Flash + GDB Debug (Inc Build)` |

命令行：

```bash
# Windows CubeProgrammer（WSL 路径可按本机修改）
./tools/scripts/flash_stlink_cube.sh

# OpenOCD
./tools/scripts/flash_stlink_openocd.sh
```

WSL2 使用 OpenOCD 时，先在 PowerShell 把 ST-Link 挂进来：

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

CubeProgrammer 路径在 `.vscode/settings.json` 的 `metal.cubeProgrammer`。更细的 IDE 说明见 `.vscode/README.md`。

## USB CDC

插入 Type-C 后会出现虚拟串口。命令（行尾回车）：

```
help
status
led1 on | led1 off
led2 on | led2 off
beep [hz] [ms]
```

`status` 输出电压、LC 频率、LED、采样计数。

## 许可

CubeMX / HAL / USB / FreeRTOS 遵循各自原许可。本仓库应用代码按项目约定使用。
