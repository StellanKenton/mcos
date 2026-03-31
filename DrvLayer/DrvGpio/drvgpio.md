# 项目引脚配置
当前物理实际映射如下：
| `DRVGPIO_LEDR` | `GPIO_NUM_9`  | OUT | 浮空 | 高有效 | `DRVGPIO_PIN_RESET` |
| `DRVGPIO_LEDG` | `GPIO_NUM_10` | OUT | 浮空 | 高有效 | `DRVGPIO_PIN_RESET` |
| `DRVGPIO_LEDB` | `GPIO_NUM_11` | OUT | 浮空 | 高有效 | `DRVGPIO_PIN_RESET` |
| `DRVGPIO_KEY1` | `GPIO_NUM_12` | OUT | 上拉 | 低有效 | `DRVGPIO_PIN_RESET` |


# DrvGpio BSP 接入提示词

当需要让 AI 为新的 MCU、SoC 或板级平台补全 GPIO BSP 时，使用下面这份说明。目标不是重写应用层，而是让 AI 在现有 `DrvGpio` 抽象之下，准确实现对应的 BSP 接口、初始化逻辑和逻辑引脚映射。

## 目标

先完成目标平台的 GPIO BSP 层，再让 `DrvGpio` 继续作为稳定公共接口对外提供服务。应用层只能感知逻辑引脚和 `drvGpio*` 接口，不能直接依赖 MCU SDK 的 GPIO API。

## 当前 DrvGpio 结构

当前目录中的关键文件职责如下：

- `drvgpio_port.h`：定义项目级逻辑引脚枚举 `eDrvGpioPinMap` 和逻辑电平枚举 `eDrvGpioPinState`。
- `drvgpio.h`：定义公共接口 `drvGpioInit`、`drvGpioWrite`、`drvGpioRead`、`drvGpioToggle`，以及 BSP 回调接口结构 `stDrvGpioBspInterface`。
- `drvgpio.c`：负责参数检查和将公共接口转发给 BSP 实现。

当前 `drvgpio.c` 里已经固定使用如下 BSP 函数名：

- `bspGpioInit`
- `bspGpioWrite`
- `bspGpioRead`
- `bspGpioToggle`

如果为新的平台实现 GPIO BSP，优先保持这组函数名不变，这样 `drvgpio.c` 可以继续直接绑定。

## 当前逻辑引脚定义

当前公共逻辑引脚来自 `drvgpio_port.h`：

```c
typedef enum eDrvGpioPinState {
    DRVGPIO_PIN_RESET = 0,
    DRVGPIO_PIN_SET,
    DRVGPIO_PIN_STATE_INVALID
} eDrvGpioPinState;

typedef enum eDrvGpioPinMap {
    DRVGPIO_LEDR = 0,
    DRVGPIO_LEDG,
    DRVGPIO_LEDB,
    DRVGPIO_KEY1,
    DRVGPIO_MAX,
} eDrvGpioPinMap;
```

这几个枚举值是当前 AI 生成 BSP 时必须对齐的输入。如果目标平台需要新增逻辑引脚，必须先更新 `drvgpio_port.h`，再实现 BSP 映射。

## 当前公共接口语义

AI 生成 BSP 时，必须与下面这些公共语义保持一致。

### `drvGpioInit(void)`

- 由系统启动过程调用。
- 内部会检查 BSP 回调是否完整。
- 然后调用 `bspGpioInit()` 完成所有逻辑引脚初始化。

因此 `bspGpioInit()` 必须一次性完成所有已定义逻辑引脚的 GPIO 配置，而不是只初始化其中一部分。

### `drvGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state)`

- `pin` 必须是有效逻辑引脚。
- `state` 只能是 `DRVGPIO_PIN_RESET` 或 `DRVGPIO_PIN_SET`。
- BSP 层负责把逻辑电平转换成物理电平并写到目标 GPIO。

### `drvGpioRead(eDrvGpioPinMap pin)`

- BSP 必须返回逻辑语义下的状态，而不是原始寄存器值。
- 返回值必须是 `DRVGPIO_PIN_RESET`、`DRVGPIO_PIN_SET` 或异常情况下的无效状态语义。

### `drvGpioToggle(eDrvGpioPinMap pin)`

- 如果底层支持，BSP 直接实现翻转。
- 如果底层没有现成翻转能力，也可以在 BSP 内部用 `read + write` 完成。

## BSP 接口约定

AI 生成的 BSP 代码，必须能满足下面这个结构体绑定要求：

```c
typedef struct stDrvGpioBspInterface {
    eDrvGpioPinState pinStates[DRVGPIO_MAX];
    drvGpioBspInitFunc init;
    drvGpioBspWriteFunc write;
    drvGpioBspReadFunc read;
    drvGpioBspToggleFunc toggle;
} stDrvGpioBspInterface;
```

也就是说，BSP 最少需要提供以下四个函数：

```c
void bspGpioInit(void);
void bspGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state);
eDrvGpioPinState bspGpioRead(eDrvGpioPinMap pin);
void bspGpioToggle(eDrvGpioPinMap pin);
```

除非当前仓库已经明确改了绑定方式，否则不要改动这四个函数签名。

## 提示词模板

```md
你正在 Windows 环境下。

任务要求：
1. 先检查 `rule.md`，并遵守仓库中的编码规范。
2. 检查 `DrvGpio` 目录，理解当前 GPIO 驱动抽象方式、逻辑引脚枚举和 BSP 接口绑定方式。
3. 检查当前平台或目标平台对应的 BSP GPIO 文件路径，补全或新建 GPIO BSP 源文件和头文件。
4. 目标 BSP 必须实现这四个函数：`bspGpioInit`、`bspGpioWrite`、`bspGpioRead`、`bspGpioToggle`。
5. 所有 MCU 相关细节，例如时钟使能、GPIO 端口号、引脚复用、上下拉、输入输出方向、默认电平、有效高低电平、寄存器访问或 SDK API 调用，都必须封装在 BSP 层内部。
6. 应用层只能调用 `drvGpioInit`、`drvGpioWrite`、`drvGpioRead` 和 `drvGpioToggle`，不要让应用层直接调用 MCU SDK 的 GPIO 接口。
7. 如果当前逻辑引脚列表不够用，先更新 `DrvGpio/drvgpio_port.h` 中的 `eDrvGpioPinMap`，再补全 BSP 映射。
8. BSP `read` 返回值必须是逻辑状态，不能直接返回原始物理电平或寄存器值。
9. BSP `write` 必须支持逻辑电平到物理电平的转换，尤其要处理低有效引脚。
10. `bspGpioInit` 必须初始化所有已定义逻辑引脚，确保 `drvGpioInit` 调用完成后，所有逻辑 GPIO 都可以立刻读写或读取。
11. 改动尽量小，不要重构无关模块。

需要产出：
1. 目标平台对应的完整 BSP GPIO 源文件和头文件，或者对现有 BSP GPIO 文件的补全。
2. 对 `DrvGpio/drvgpio_port.h` 的必要更新。
3. 如果确有必要，对 `DrvGpio/drvgpio.h` 和 `drvgpio.c` 做最小改动，保证它们可以干净地转发到 BSP 实现。
4. 一段简短说明，描述逻辑引脚和物理引脚、方向、上下拉、有效电平之间的映射关系。
5. 一段验证说明，描述已经检查了什么，以及还剩下哪些风险或缺口。

实现约束：
- 代码中的注释必须使用英文。
- 每个文件只负责一个清晰职责。
- 不要增加占位接口或伪实现。
- 除非 GPIO 抽象本身需要，否则不要改变应用层行为。
- 保持现有 `drvGpio*` 调用方式向后兼容。
- 如果平台存在低有效按键、低有效 LED 或板级反相电路，必须在 BSP 内部完成逻辑与物理电平转换，不能把这种差异泄漏给应用层。
- 如果底层平台没有原生 toggle 接口，可以在 BSP 中使用 `read + write` 实现 `bspGpioToggle`。

推荐工作流程：
1. 先读取 `DrvGpio/drvgpio_port.h`，确认当前逻辑引脚枚举和逻辑电平定义。
2. 再读取 `DrvGpio/drvgpio.h` 和 `drvgpio.c`，确认公共接口和 BSP 函数签名。
3. 找到目标平台 BSP 中负责 GPIO 时钟、引脚模式、上下拉和引脚读写的文件。
4. 建立逻辑引脚到物理 GPIO 的映射表，明确每个引脚的方向、默认状态和有效电平。
5. 实现 `bspGpioInit`、`bspGpioWrite`、`bspGpioRead` 和 `bspGpioToggle`。
6. 检查 `drvGpioInit` 调用后，所有逻辑引脚是否都已完成初始化。
7. 验证应用层没有直接依赖任何 MCU 专有 GPIO 接口。
8. 总结最终的逻辑引脚到物理 GPIO 的映射关系。
```

## 当前 ESP32 参考实现

当前仓库里的 ESP32 参考 BSP 在 `main/bsp_gpio.c`，它展示了 AI 应该生成出的目标风格。

当前映射如下：

| 逻辑引脚 | 物理 GPIO | 方向 | 上下拉 | 有效电平 | 默认状态 |
| --- | --- | --- | --- | --- | --- |
| `DRVGPIO_LEDR` | `GPIO_NUM_9` | 输出 | 浮空 | 高有效 | `DRVGPIO_PIN_RESET` |
| `DRVGPIO_LEDG` | `GPIO_NUM_10` | 输出 | 浮空 | 高有效 | `DRVGPIO_PIN_RESET` |
| `DRVGPIO_LEDB` | `GPIO_NUM_11` | 输出 | 浮空 | 高有效 | `DRVGPIO_PIN_RESET` |
| `DRVGPIO_KEY1` | `GPIO_NUM_12` | 输入 | 上拉 | 低有效 | `DRVGPIO_PIN_RESET` |

这个参考实现体现了几个关键点：

- BSP 内部用配置表统一描述每个逻辑引脚。
- BSP 内部处理 `activeHigh`，保证应用层始终只看到统一的逻辑电平语义。
- 输出引脚在初始化时会被设置到默认逻辑状态。
- 输入引脚的读值会先读取物理电平，再转换成逻辑状态返回。

## AI 生成 BSP 时必须遵守的边界

- `DrvGpio` 负责稳定公共接口和参数检查。
- BSP 负责全部平台相关细节。
- 应用层不直接依赖 GPIO 物理编号、寄存器位定义或 SDK 专有接口。

如果 AI 生成的代码让应用层直接出现 `gpio_set_level`、`HAL_GPIO_WritePin`、`nrf_gpio_pin_write` 之类平台 API，就说明抽象边界做错了。
