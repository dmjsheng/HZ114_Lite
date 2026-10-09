# HZ114 Lite C 命名规范

本规范服务于 Lite V1：两路输入（TL/PL）→ 软件识别 → 优先级/延时仲裁 → 两路 GPIO 使能。它不定义故障反馈域。

## 1. 总原则

1. **一个名字只表达一个层级。** `tl_on`、`tl_wants_on`、`tl_on_out` 不能互换。
2. **C 标识符用下划线，不用连字符。** 写 `tl_on`，不写 `tl-on`。
3. **先写对象，再写状态。** 写 `tl_on`，不写 `on_tl`。
4. **名字说明事实，不说明希望。** 采样失败时不能用 `tl_off` 假装已经确认关闭。
5. **宏表达语义，底层寄存器细节留在驱动层。** 业务层写 `TL_EN_ON()`，不直接写端口寄存器。
6. **每个常量带单位或计数含义。** 例如 `_MS`、`_US`、`_HZ`、`_COUNT`、`_TICKS`、`_THRESHOLD`。
7. **V1 不创建故障反馈名字。** 不新建 `fault_*`、`fb_*`、`FAULT_*`、`FB_*` 的业务变量、接口或宏。

## 2. 领域缩写

| 缩写 | 唯一含义 | 可用示例 | 不要这样用 |
|---|---|---|---|
| `tl` / `TL` | Turn Lamp，转向灯功能 | `tl_on`、`TL_EN_ON()` | 用于温度、定时器或任意 LED |
| `pl` / `PL` | Position Lamp，位置灯功能 | `pl_avg`、`PL_EN_OFF()` | 用于电源电平或 PWM 占空比 |
| `adc` / `ADC` | ADC 采样及处理域 | `adc_state_t`、`ADC_5ms_Task()` | 泛指所有输入 |
| `gpio` / `GPIO` | GPIO 驱动与硬件映射域 | `gpio_drv.c`、`GPIO_Write_High()` | 业务仲裁状态 |
| `os` / `OS` | QuarkTS 调度、周期任务域 | `os_5ms_task`、`OS_5ms_Task_Callback()` | 灯光业务状态 |

## 3. 状态链命名：从输入到输出

同一路 TL 或 PL 信号，必须按以下语义逐层命名。后面的层不能反过来修改前面的层含义。

| 层级 | TL 示例 | PL 示例 | 类型/值 | 定义 |
|---|---|---|---|---|
| 原始 ADC | `tl_raw` | `pl_raw` | `uint16_t` | 单次 ADC 数字结果 |
| 滤波结果 | `tl_avg` | `pl_avg` | `uint16_t` | 滤波后的 ADC 数值 |
| 输入识别 | `tl_on` | `pl_on` | `uint8_t`，仅 `0u/1u` | 软件判断输入是否存在 |
| 业务请求 | `tl_wants_on` | `pl_wants_on` | `uint8_t`，仅 `0u/1u` | 交给仲裁层的请求 |
| 最终命令 | `tl_on_out` | `pl_on_out` | `uint8_t`，仅 `0u/1u` | 仲裁后允许给 GPIO 的命令 |
| GPIO 动作 | `TL_EN_ON()` | `PL_EN_ON()` | 宏/函数 | 对外部驱动发出开启语义 |
| 实际结果 | 不使用上述变量命名 | 不使用上述变量命名 | 测量记录 | 必须由样板测量确认 |

### 必须理解的差别

```text
tl_on          = 软件确认转向输入存在
tl_wants_on    = 业务层希望点亮转向
tl_on_out      = 仲裁后允许输出转向使能
TL_EN_ON()     = GPIO 层执行“开启 TL 使能”动作
```

例如，`tl_on = 1u` 不证明 PB5 已经为高；`tl_on_out = 1u` 不证明外部灯已经点亮。

## 4. 通用变量后缀和前缀

| 规则 | 含义 | 正确示例 | 不推荐示例 |
|---|---|---|---|
| `*_raw` | 单次、未经处理的硬件数值 | `tl_raw` | `tl_value1` |
| `*_avg` | 滤波后的平均或代表值 | `pl_avg` | `pl_data` |
| `*_on` | 软件识别出的逻辑状态 | `tl_on` | `tl_led` |
| `*_wants_on` | 送入业务仲裁层的请求 | `pl_wants_on` | `pl_enable_flag` |
| `*_on_out` | 业务仲裁后的最终输出命令 | `tl_on_out` | `tl_status` |
| `*_valid` | 数据是否可用于本周期判断 | `sample_valid` | `adc_ok_flag` |
| `*_active` | 一个条件或保持状态当前是否生效 | `tl_priority_hold_active` | `hold_flag` |
| `*_cnt` | 正在递增的计数器 | `pl_recover_cnt` | `count1` |
| `*_count` | 常量/最终计数数量 | `FILTER_SAMPLE_COUNT` | `num` |
| `prev_*` | 上一有效周期的状态 | `prev_tl_wants_on` | `old_tl` |
| `*_state` | 可枚举的状态机状态 | `lamp_state` | `state1` |
| `*_ms` / `*_us` | 变量值的时间单位 | `elapsed_ms` | `delay` |
| `*_ticks` | 按任务周期换算后的 tick 数 | `pl_recover_ticks` | `delay_count` |

## 5. 类型、结构体和对象

### 类型名

- 所有自定义类型使用小写 `snake_case`，结尾必须是 `_t`。
- 类型表达“它是什么”，不要表达“它要做什么”。

```c
typedef struct
{
    uint8_t tl_wants_on;
    uint8_t pl_wants_on;
    uint8_t sample_valid;
} lamp_input_t;

typedef struct
{
    uint8_t tl_on_out;
    uint8_t pl_on_out;
} lamp_output_t;

typedef struct
{
    uint16_t tl_raw;
    uint16_t pl_raw;
    uint16_t tl_avg;
    uint16_t pl_avg;
    uint8_t tl_on;
    uint8_t pl_on;
    uint8_t sample_valid;
} adc_state_t;
```

### 对象名

- 结构体实例用小写 `snake_case`，例如 `input`、`output`、`adc_state`。
- 模块内部状态默认 `static`，例如 `static adc_state_t adc_state;`。
- 不使用没有语义的全局实例名，如 `adc_filter_pro`、`g_data`、`value`、`temp`。
- 需要跨模块传递的数据，优先通过函数参数或受控的只读接口传递；不要为图省事暴露全局变量。

### 布尔值

- Lite V1 使用 `uint8_t` 表示布尔状态，仅赋值 `0u` 或 `1u`。
- 变量名必须带完整语义：`sample_valid`、`tl_on`；不要使用 `flag`、`enable`、`status` 这类孤立名字。
- 条件优先写正向语义：`if (input->tl_wants_on)` 比 `if (!tl_off)` 更清楚。

## 6. 函数命名

### 对外模块接口

沿用项目中容易读懂的 HZ114 风格：首字母大写、单词之间下划线分隔。

| 目的 | 格式 | 示例 |
|---|---|---|
| 初始化模块 | `Init_<Module>()` | `Init_GPIO()`、`Init_ADC()`、`Init_Output()` |
| 固定周期任务 | `<Module>_<period>_Task()` | `ADC_5ms_Task()`、`Output_5ms_Task()` |
| 任务回调 | `OS_<period>_Task_Callback()` | `OS_5ms_Task_Callback()` |
| 纯仲裁一步 | `<Module>_Logic_<period>_Step()` | `Output_Logic_5ms_Step()` |

### 模块内私有辅助函数

- 使用全小写 `snake_case`，并加 `static`。
- 名字以动词开头，说明局部动作。

```c
static uint8_t apply_hysteresis(...);
static void adc_start_scan(void);
static uint16_t calculate_average(...);
static void reset_pl_recover_counter(void);
```

避免 `DoTask()`、`Handle()`、`Process()`、`Func1()` 等没有边界的名字。若确实是完整业务周期，使用明确模块名，例如 `Output_5ms_Task()`。

## 7. 宏、常量和 GPIO 语义

### 配置常量

配置宏全大写，单词用下划线，并说明单位或来源。

```c
#define OUTPUT_TASK_PERIOD_MS       5u
#define FILTER_SAMPLE_COUNT         6u
#define TL_ON_THRESHOLD             800u
#define TL_OFF_THRESHOLD            400u
#define PL_RECOVER_DELAY_MS         480u
#define PL_RECOVER_DELAY_TICKS \
    (PL_RECOVER_DELAY_MS / OUTPUT_TASK_PERIOD_MS)
```

上例仅展示命名格式；Lite V1 的数值必须在输入电压、任务周期和产品需求确认后填写，不能直接复制示例数值。

### 硬件映射宏

硬件位置只放在 `hz114_lite_hw_config.h`，业务层不直接写 `GPIOB`、`GPIO_Pin_5`。

```c
#define TL_EN_GPIO_PORT             GPIOB
#define TL_EN_GPIO_PIN              GPIO_Pin_5
#define TL_EN_GPIO_CLOCK            RCC_AHBPeriph_GPIOB_CKEN
```

上例只展示格式，**不是** Lite V1 的已确认引脚定义。

### 语义 GPIO 宏

业务层只能使用语义宏；宏内部再根据实际极性决定高/低。

```c
#define TL_EN_ON()                  /* 根据已确认极性驱动 TL 使能 */
#define TL_EN_OFF()                 /* 根据已确认极性关闭 TL 使能 */
#define PL_EN_ON()                  /* 根据已确认极性驱动 PL 使能 */
#define PL_EN_OFF()                 /* 根据已确认极性关闭 PL 使能 */
```

如需配置极性，名称使用 `TL_OUTPUT_ACTIVE_HIGH`、`PL_OUTPUT_ACTIVE_HIGH`。不要在业务逻辑中写“高电平就是灯开”的假设。

## 8. 文件和模块边界

| 文件 | 唯一职责 | 不应包含 |
|---|---|---|
| `main.c` | 启动顺序、时钟、进入 OS | ADC 阈值、灯光优先级 |
| `pro_os.c` | QuarkTS 注册、周期回调顺序 | GPIO 寄存器细节、仲裁规则 |
| `hz114_lite_config.h` | 周期、滤波、延时等业务配置 | GPIO 端口和引脚号 |
| `hz114_lite_hw_config.h` | 已确认的端口、引脚、ADC 通道、极性 | 灯光业务 if/else |
| `gpio_drv.c/.h` | GPIO 初始化、语义开关宏/函数 | ADC 滤波、TL/PL 优先级 |
| `adc_drv.c/.h` | ADC 采样、滤波、滞环、有效性 | GPIO 输出动作 |
| `output_logic.c/.h` | TL/PL 优先级、恢复延时、纯逻辑输入输出 | 寄存器调用 |
| `output.c/.h` | 连接 ADC 状态、仲裁结果和 GPIO 动作 | 时钟初始化 |

## 9. 注释规范

注释必须解释“为什么”或“这个变量/数字的来源”，不重复代码字面意思。

```c
/* TL 优先期间禁止 PL，避免两个外部驱动同时使能。 */
result->pl_on_out = 0u;

/* 由产品需求定义；实际板级延时需单独测量。 */
#define PL_RECOVER_DELAY_MS  480u
```

不推荐：

```c
/* 给 tl_on 赋值。 */
tl_on = 1u;
```

## 10. 禁止或保留的名字

| 不要使用 | 原因 | 替代方式 |
|---|---|---|
| `flag`、`flag1` | 不知道它代表什么 | `sample_valid`、`tl_on` |
| `temp` | 不知道是临时变量还是温度 | `tl_raw`、`pl_recover_cnt` |
| `data`、`value` | 不知道来源和单位 | `tl_avg`、`adc_result` |
| `status` | 可能是输入、输出或故障 | `tl_on_out`、`sample_valid` |
| `delay` | 不知道单位或用途 | `pl_recover_cnt`、`PL_RECOVER_DELAY_MS` |
| `LED1`、`LED2` | 脱离业务语义 | `TL`、`PL` |
| `TL_ON` 作为变量 | 容易与宏/实际灯亮混淆 | `tl_on` 或 `TL_EN_ON()` |

## 11. 写代码前的自检问题

新增一个名字前，先回答：

1. 它属于原始输入、滤波结果、业务请求、最终输出还是实测结果？
2. 它的单位、极性、有效条件是否能从名字或注释看出来？
3. 它是否会和 TL/PL 的另一个层级混淆？
4. 它是否属于 V1 范围；如果属于故障反馈，先不要创建？
5. 同事只看名字，不看实现，能不能猜出它在链路中的位置？
