# HZ114 Lite V1：TL/PL 硬件映射

## 证据与适用范围

Lite 与 `HZ114_Demo` 使用同一块 HZ114 板和同一套硬件。下表的端口、引脚、ADC 通道和扫描序号来自当前 `HZ114_Demo/App/hz114_hw_config.h`，可作为 Lite V1 的硬件映射基线。

这不自动证明 Lite 程序已经正确驱动硬件。输入电压、输出极性、GPIO 实际波形和灯具状态仍需按 [V1_SCOPE.md](V1_SCOPE.md) 测量。

## V1 使用的信号

| 功能 | 板上网络 | MCU 引脚 | 外设功能 | 当前 HZ114 参考映射 | Lite V1 用法 |
|---|---|---|---|---|---|
| TL 输入 | `PWRTL` | PB1 | AIN11 | ADC 扫描 CH1 | ADC 模拟输入 |
| PL 输入 | `PWRPL` | PA7 | AIN15 | ADC 扫描 CH0 | ADC 模拟输入 |
| TL 输出 | `GPIO_TL` | PB5 | GPIO | 转向驱动使能 | 推挽 GPIO 输出，语义为 `TL_EN_ON/OFF()` |
| PL 输出 | `GPIO_PL` | PB4 | GPIO | 位置灯驱动使能 | 推挽 GPIO 输出，语义为 `PL_EN_ON/OFF()` |

## V1 明确不使用的信号

| 板上网络 | MCU 引脚 | 原工程角色 | Lite V1 处理 |
|---|---|---|---|
| `FAULTB` | PB2 | 转向驱动故障输入 | 不初始化、不读取、不参与业务逻辑 |
| `GPIO_FB` / `FB_TL` 路径 | PC3 | 向 BCM 输出故障反馈 | 不初始化、不输出、不参与业务逻辑 |

## 必须遵守的复用限制

### PB5：TL 输出与 MCO 冲突

`HZ114_Lite/App/pro_os.c` 当前的 `Clock_MCO_config()` 会把 PB5 配置为 `GPIO_AF_CLK_MCO_PB5`，也就是时钟输出功能。Lite V1 同时要求 PB5 作为 TL GPIO 输出，因此两者不能共存。

在你开始写 `gpio_drv.c` 前，先完成这条设计决定：

```text
Lite V1 选择 PB5 作为 TL 使能 GPIO。
因此不调用 Clock_MCO_config()，也不把 PB5 配置为 MCO 复用功能。
```

只有这样，`Init_GPIO()` 才能安全地把 PB5 配置为普通输出。

### PB1：TL ADC 输入与 UART 复用冲突

PB1 用作 AIN11/PWRTL 输入时，不能再初始化为 UART 或其他数字复用功能。Lite V1 中 PB1 只属于 ADC 输入模块。

### PA7：PL ADC 输入

PA7 用作 AIN15/PWRPL 输入时，Lite V1 应配置为模拟输入，不开内部上下拉，不当作数字 GPIO 使用。

## 输出极性：参考配置与实测边界

当前 HZ114 参考 `gpio_drv.h` 使用：

```c
#define TL_OUTPUT_ACTIVE_HIGH  1u
#define PL_OUTPUT_ACTIVE_HIGH  1u
```

这说明参考程序的语义是：`TL_EN_ON()` / `PL_EN_ON()` 最终写高对应的 GPIO。Lite V1 可以采用相同的**命名和配置形式**，但第一次上板时必须在 PB5、PB4 对系统 GND 实测确认，再把它称为已验证的有效极性。

无论极性最终如何，业务层只写：

```c
TL_EN_ON();
TL_EN_OFF();
PL_EN_ON();
PL_EN_OFF();
```

不要在 `output_logic.c` 中直接写 `GPIO_SetBits(GPIOB, GPIO_Pin_5)`。

## ADC 扫描顺序

同一块 HZ114 硬件上的参考扫描顺序如下：

```text
CH0 = PA7 / AIN15 / PWRPL
CH1 = PB1 / AIN11 / PWRTL
```

如果 Lite 保留这个顺序，读取时必须保持一致：CH0 的结果命名为 `pl_raw`，CH1 的结果命名为 `tl_raw`。绝不能因变量书写顺序不同而把两路结果接反。

## 第一轮板上测量顺序

在向任何线束施加输入前，先做被动、低风险验证：

1. 不启用 MCO，测 PB5/PB4 的上电默认关闭电平。
2. 用已知安全的输入条件逐路测 PA7/PB1 对 GND 的电压；确认没有超过 MCU/分压网络允许范围。
3. 记录每个输入条件对应的 `pl_raw`、`tl_raw`，确认 CH0/CH1 没接反。
4. 仅在 GPIO 输出逻辑已测得正确后，再连接或观察外部驱动和灯具。

不要为验证而向 PB1、PA7、PB5、PB4、PB2 或 PC3 任意外加 5 V/12 V；先确认原理图、共地、分压路径和允许电压。
