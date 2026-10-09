# HZ114_Lite - 汽车车灯控制系统 (V1.0)

本项目是基于芯弦半导体 **CPS32K212**（ARM Cortex-M0+ 内核，48MHz）微控制器的汽车车灯控制系统精简版。  
**V1.0 版本为纯裸机架构**，实现了完整的两路模拟量信号采集、软件滑动滤波滤波、双阈值滞环判断、转向灯绝对优先仲裁状态机，以及可配置的 450ms 位置灯恢复延时消隐保护，并在 HZ114 硬件样板上实测验证通过。

---

## 📌 V1.0 功能实现清单

1. **两路 ADC 模拟量输入采集与滤波（`adc_drv.c`）**：
   - **PA7（AIN15）**：`PWRPL` 位置灯（Position Lamp, PL）输入采样。
   - **PB1（AIN11）**：`PWRTL` 转向灯（Turn Lamp, TL）输入采样。
   - **双通道顺序扫描（Scan Mode）**。
   - **6 样本滑动平均滤波窗口（Moving Average Filter）**：平滑滤除瞬态电源纹波与点火毛刺。
   - **双阈值滞环消抖（Hysteresis Thresholds）**：
     - 开门阈值（OFF $\to$ ON）：$\ge 800$（对应分压输入约 0.98V）。
     - 关门阈值（ON $\to$ OFF）：$\le 400$（对应分压输入约 0.49V）。
     - 彻底消除临界电压振荡导致的频闪。
   - **失效保护与冷启动校准**：开机执行预热灌满均值环，单次采样设 5000 次死区保护，采样失效时主动拉低全部输出。

2. **两路 GPIO 推挽输出驱动（`gpio_drv.c`）**：
   - **PB5（`TL_EN`）**：转向灯驱动控制输出（Push-Pull，高电平有效）。
   - **PB4（`PL_EN`）**：位置灯驱动控制输出（Push-Pull，高电平有效）。
   - **安全防呆机制**：引脚切换前预拉低输出寄存器，上电与复位默认低电平关断，杜绝上电微小毛刺误触发。

3. **四状态决策仲裁状态机（`output.c`）**：
   - **全关态（`LAMP_STATE_ALL_OFF`）**：无灯光请求时的低功耗静默状态。
   - **转向态（`LAMP_STATE_TL_ON`）**：**转向灯绝对优先**！转向有效时，强制关断位置灯 PL。
   - **位置灯态（`LAMP_STATE_PL_ON`）**：纯位置灯点亮状态。
   - **恢复延时态（`LAMP_STATE_TL_HOLD_DELAY`）**：
     - 当转向信号撤销后，不立即点亮位置灯，而是进入 **450ms（90 个 5ms ticks）消隐保护等待**。
     - 若在 450ms 倒计时期间又有新的转向信号到达，立即重新清零计时并切回转向态；
     - 只有倒计时跑满且位置灯信号依然有效时，才自动无缝重新恢复位置灯点亮。

4. **架构解耦与设计规范**：
   - **输入驱动层**（`adc_drv`） $\to$ **业务决策层**（`output`） $\to$ **输出驱动层**（`gpio_drv`）严格分层解耦。
   - 业务逻辑绝不直接操作硬件底层寄存器，统一使用语义化接口。

---

## 🛠️ 关键底层踩坑与硬件适配指南（必读）

在本项目实机调试（Bring-up）过程中，攻克了以下几个极其隐蔽的底层核心硬件缺陷，在此明确记录供后续维护与迭代参考：

### 1. 芯片型号与物理 RAM 边界限制（HardFault 根因）
* **硬件事实**：本板搭载的 MCU 型号为 **CPS32K212**，其物理 SRAM 大小仅为 **6KB**（地址范围：`0x20000000` $\sim$ `0x20001800`）。
* **避坑记录**：官方默认工程模板使用了 CPS32K214（8KB RAM）的链接脚本，导致栈顶指针 `__stack` 被计算在不存在的 `0x20002000`。单片机一上电进 `main()` 执行函数入栈操作时，CPU 立即访问越界总线地址触发 **HardFault（硬件错误中断死锁）**。
* **修正方案**：链接脚本 [`GCC_cps32k11x.ld`](Device/CPS32K214LSLFMT/GCC_cps32k11x.ld) 中必须强制指定：
  ```ld
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 32k
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 6k
  ```
  保证栈顶安全落在 `0x20001800`。

### 2. 时钟源选择（HXT vs HIRC）
* **硬件事实**：板载未外接高频晶振（无 HXT 外部晶振电路）。
* **避坑记录**：若使用 `PLL_INPUT_SOURCE_HXT`，底层库函数将在 `RCC_HXT_Cmd()` 中死等外部晶振起振，超时计数器长达 42.9 亿次（需 50 多分钟），导致系统假死。
* **修正方案**：必须使用内部高速 RC 振荡器倍频至 48MHz：
  ```c
  status = RCC_SysClk_SwitchTo_PLL(RCC, PLL_INPUT_SOURCE_HIRC, SET_PLL_FREQ_48MHz);
  ```

### 3. 引脚复用冲突（PB5 与 MCO 时钟输出）
* **硬件事实**：PB5 在当前硬件中作为转向灯驱动使能引脚（`TL_EN`）。
* **避坑记录**：`pro_os.c` 中的 `Clock_MCO_config()` 函数会强行把 PB5 配置为 `GPIO_AF_CLK_MCO_PB5`（时钟输出功能）。
* **修正方案**：在后续开启操作系统（QuarkTS）时，切勿在 `Init_OS()` 中调用 `Clock_MCO_config()`，确保 PB5 独占作为普通 GPIO 输出使用。

---

## 📂 项目目录结构

```text
HZ114_Lite/
├── App/                    # 应用层代码
│   ├── main.c              # 裸机 5ms 周期大循环入口
│   ├── adc_drv.c / .h      # ADC 输入采集、滑动均值滤波与滞环处理
│   ├── gpio_drv.c / .h     # GPIO 推挽输出驱动及语义化控制宏
│   ├── output.c / .h       # 四状态车灯决策仲裁状态机
│   ├── pro_os.c / .h       # 系统时钟与系统任务接口
│   └── interrupt.c / .h    # 中断服务函数
├── Device/                 # 芯片架构支持
│   └── CPS32K214LSLFMT/    # 芯海 CPS32K 系列 CMSIS 文件、启动汇编与链接脚本
├── Driver/                 # MCU 外设固件标准库（RCC、GPIO、ADC 等）
├── RTOS/                   # QuarkTS-7.3.3 源码（V2.0 阶段启用）
├── hex/                    # 编译产物发布归档
│   └── V1.0/               # V1.0 验证通过的完整 HEX/BIN 固件
├── CMakeLists.txt          # CMake 构建工程脚本
└── README.md               # 本工程技术文档
```

---

## 💻 编译与构建

本工程基于 **ARM GNU Toolchain (GCC) + CMake + Ninja** 构建：

```bash
# 1. 生成构建系统
cmake -B build -S . -G Ninja

# 2. 编译生成目标文件
cmake --build build

# 生成固件位于：
# build/CPS32K212_Demo.hex
# build/CPS32K212_Demo.bin
```

---

## 🚀 后续规划 (Roadmap to V2.0)

- [ ] **引入 QuarkTS 协同式多任务操作系统**：将裸机 5ms 延时循环重构为 OS 周期任务管理。
- [ ] **看门狗守护**：加入 20ms IWDG 独立看门狗刷新任务与硬件超时防护。
- [ ] **故障诊断与反馈扩展**：接入 PB2（FAULTB 输入）与 PC3（FB_TL 故障反馈）扩展诊断。
