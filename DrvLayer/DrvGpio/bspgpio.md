# DrvGpio BSP 接入提示词

当需要为新的 MCU 或新的开发板生成 GPIO BSP 文件时，使用下面这份提示词。本文档重点说明 `DrvGpio` 期望的 BSP 输入、输出和功能语义，用来指导生成 `bspgpio.c` 和 `bspgpio.h`，并将其接入现有 GPIO 抽象层。

## 目标

先完成目标 MCU 的 GPIO BSP 层，再通过 `DrvGpio` 暴露稳定的跨 MCU GPIO 接口。应用层只依赖 `drvGpio*` 接口，不直接接触 MCU SDK 的 GPIO 细节。

当前 `DrvGpio` 的公共层依赖以下四个 BSP 钩子：

- `init`
- `write`
- `read`
- `toggle`

这四个钩子都必须提供，因为 `drvGpioInit()` 会检查 `gDrvGpioBspInterface` 是否完整；如果 `toggle` 为 `NULL`，初始化检查会失败。

## 当前接口约束

`DrvGpio` 当前的公共接口和逻辑引脚枚举来自：

- `drvgpio.h`
- `drvgpio_port.h`
- `drvgpio_port.c`

当前逻辑电平定义为：

- `DRVGPIO_PIN_RESET`
- `DRVGPIO_PIN_SET`
- `DRVGPIO_PIN_STATE_INVALID`

如果目标板卡的逻辑引脚不止这些，或者这些名称还不能准确表达用途，应先更新 `drvgpio_port.h` 中的 `eDrvGpioPinMap`，再让 BSP 层与之对应。

## BSP 文件职责

推荐生成两个 BSP 文件：

- `bspgpio.h`：声明 GPIO BSP 对外提供给 `DrvGpio` 绑定层使用的函数原型。
- `bspgpio.c`：实现具体 MCU/板级 GPIO 时钟、端口、引脚、模式、上下拉、电平读写和翻转逻辑。

所有 MCU 相关细节都必须留在 BSP 层内部，包括但不限于：

- GPIO 时钟使能
- 端口与引脚号
- 输入/输出模式
- 上拉、下拉、推挽、开漏配置
- 默认输出电平
- 读写寄存器或调用 MCU SDK 的方式
- 如果硬件支持原子翻转寄存器，其使用方式

`DrvGpio` 公共层不应包含任何 MCU 专有寄存器访问。

## BSP 函数输入输出与功能要求

生成的 `bspgpio.h` 应至少声明以下接口：

```c
void bspGpioInit(void);
void bspGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state);
eDrvGpioPinState bspGpioRead(eDrvGpioPinMap pin);
void bspGpioToggle(eDrvGpioPinMap pin);
```

### 1. `bspGpioInit`

输入：

- 无

输出：

- 无

预计功能：

- 使能 BSP 中实际使用到的 GPIO 外设时钟。
- 按 `eDrvGpioPinMap` 中的逻辑引脚逐个完成硬件初始化。
- 为 LED、按键等不同类型引脚配置正确的输入/输出模式。
- 为输入引脚配置合适的上下拉。
- 为输出引脚设置安全的默认电平，避免上电瞬间误动作。
- 保证在 `drvGpioInit()` 完成后，后续 `drvGpioWrite()`、`drvGpioRead()` 和 `drvGpioToggle()` 可以直接工作。

实现要求：

- 只初始化当前逻辑引脚实际用到的硬件资源。
- 不要把应用层业务逻辑放进初始化函数。
- 如果某个逻辑引脚未映射到物理引脚，应在 BSP 内部显式处理，而不是无声越界访问。

### 2. `bspGpioWrite`

输入：

- `pin`：逻辑引脚编号，类型为 `eDrvGpioPinMap`
- `state`：目标电平，类型为 `eDrvGpioPinState`

输出：

- 无

预计功能：

- 将逻辑引脚映射到对应的物理 GPIO 端口和引脚。
- 根据 `state` 将目标引脚输出为高电平或低电平。
- 只处理输出型逻辑引脚，例如 LED、片选、复位脚等。

实现要求：

- `state` 只应接受 `DRVGPIO_PIN_RESET` 或 `DRVGPIO_PIN_SET`。
- 当逻辑引脚对应输入脚时，不应执行错误的输出操作。
- 如果板级电路是低电平有效，电平换算应封装在 BSP 内部，不要泄漏到 `DrvGpio` 公共层。
- 不要依赖应用层知道具体端口、引脚号或是否低电平有效。

### 3. `bspGpioRead`

输入：

- `pin`：逻辑引脚编号，类型为 `eDrvGpioPinMap`

输出：

- 返回值类型为 `eDrvGpioPinState`

预计功能：

- 读取指定逻辑引脚当前的实际电平状态。
- 将 MCU SDK 的读值统一转换为 `DRVGPIO_PIN_RESET` 或 `DRVGPIO_PIN_SET`。
- 当映射无效或无法判定时，返回 `DRVGPIO_PIN_STATE_INVALID`。

实现要求：

- 对输入脚和输出脚都应能给出一致的读值语义。
- 如果硬件是低电平有效，应在返回前完成语义归一化，并在实现说明中写清楚“返回的是物理电平还是逻辑有效态”。
- 推荐返回“物理引脚当前电平”的统一语义；如果选择“逻辑有效态”，必须整份模块保持一致。

### 4. `bspGpioToggle`

输入：

- `pin`：逻辑引脚编号，类型为 `eDrvGpioPinMap`

输出：

- 无

预计功能：

- 翻转指定逻辑引脚当前输出状态。
- 用于 LED 闪烁、测试脉冲等需要快速翻转的场景。

实现要求：

- 该函数必须实现，不能留空，也不能只在公共层依赖回退逻辑。
- 如果 MCU 提供原子翻转能力，优先在 BSP 内使用。
- 如果 MCU 不支持直接翻转，可在 BSP 内部通过“先读后写”完成，但语义必须保持为一次翻转。
- 对不支持翻转的输入脚应做保护，避免错误写操作。

## 推荐的 BSP 内部设计

为了让 `bspgpio.c` 更容易扩展，推荐在 BSP 内部维护一个逻辑引脚到物理资源的映射表。可以使用类似下面的内部结构，但是否采用由当前模块局部风格决定：

```c
typedef struct stBspGpioPinConfig {
	uint32_t rcu;
	uint32_t port;
	uint32_t pin;
	bool isOutput;
	bool isActiveLow;
} stBspGpioPinConfig;
```

建议语义：

- `rcu`：对应 GPIO 端口时钟
- `port`：GPIO 端口实例
- `pin`：GPIO 引脚位
- `isOutput`：标记该逻辑引脚是否允许写和翻转
- `isActiveLow`：标记该逻辑引脚是否低电平有效

如果项目当前更适合使用 `switch-case`，也可以不用映射表，但仍需保证逻辑引脚与物理资源的对应关系集中管理，不要把映射散落到多个函数里。

## 与 `DrvGpio` 的对接方式

生成 `bspgpio.c/.h` 后，还需要把它们接到现有端口绑定层。当前 `drvgpio_port.c` 中的 `gDrvGpioBspInterface` 需要最终类似这样：

```c
stDrvGpioBspInterface gDrvGpioBspInterface = {
	.init = bspGpioInit,
	.write = bspGpioWrite,
	.read = bspGpioRead,
	.toggle = bspGpioToggle,
};
```

这意味着：

- `drvgpio_port.c` 负责绑定。
- `bspgpio.c` 负责具体实现。
- `drvgpio.c` 只负责参数校验和公共层转发。

如果生成了新的 `bspgpio.h`，应由 `drvgpio_port.c` 包含它，并完成钩子表赋值。

## 提示词模板

```md
你正在 Windows 环境下。

任务要求：
1. 先检查 `rule.md`，并遵守仓库中的编码规范。
2. 检查 `DrvGpio`，理解当前 GPIO 驱动抽象方式、逻辑引脚枚举和 BSP 钩子表设计。
3. 创建或补全 `bspgpio.c` 和 `bspgpio.h`，实现 GPIO BSP 层。
4. 所有 MCU 相关细节，例如时钟使能、GPIO 端口、引脚号、输入输出模式、上下拉、电平极性和寄存器访问，都必须放在 BSP 层内部。
5. `bspgpio.h` 至少要声明 `bspGpioInit`、`bspGpioWrite`、`bspGpioRead` 和 `bspGpioToggle`。
6. `bspGpioWrite` 的输入是 `eDrvGpioPinMap pin` 和 `eDrvGpioPinState state`，输出为无；它的职责是将逻辑引脚驱动到目标电平。
7. `bspGpioRead` 的输入是 `eDrvGpioPinMap pin`，输出是 `eDrvGpioPinState`；它的职责是返回当前引脚状态，异常或无效映射时返回 `DRVGPIO_PIN_STATE_INVALID`。
8. `bspGpioToggle` 的输入是 `eDrvGpioPinMap pin`，输出为无；它的职责是翻转输出引脚状态，而且必须真实实现，不能留空。
9. `bspGpioInit` 的输入输出均为无；它的职责是初始化所有逻辑 GPIO 对应的硬件资源，并设置安全默认状态。
10. 如果当前 `drvgpio_port.h` 中的逻辑引脚枚举不完整，先更新它，再让 BSP 实现与之对应。
11. 生成 BSP 文件后，更新 `drvgpio_port.c`，将 `gDrvGpioBspInterface` 绑定到 `bspGpioInit`、`bspGpioWrite`、`bspGpioRead` 和 `bspGpioToggle`。
12. 不要让应用层直接调用 MCU SDK 的 GPIO 接口。应用层只能调用 `drvGpio*` 接口。
13. 改动尽量小，不要重构无关模块。

需要产出：
1. 完整的 `bspgpio.c` 和 `bspgpio.h`。
2. 对 `drvgpio_port.c` 的必要更新，用于绑定 BSP 钩子。
3. 对 `drvgpio_port.h` 的必要更新，用于补充逻辑 GPIO 枚举或配置。
4. 一段简短说明，描述逻辑 GPIO 与物理端口/引脚、模式、电平极性之间的映射关系。
5. 一段验证说明，描述已经检查了什么，以及还剩哪些硬件相关风险。

实现约束：
- 代码中的注释必须使用英文。
- 每个文件只负责一个清晰职责。
- 不要增加占位接口或伪实现。
- 保持现有 `drvGpio*` 调用方式向后兼容。
- 不要把 MCU 相关宏、寄存器和 SDK 细节泄漏到 `drvgpio.c`。
- `bspGpioRead` 的返回语义必须在整个模块内保持一致。
- `bspGpioToggle` 必须可用，因为当前 `drvGpioInit()` 会要求完整钩子表。

推荐工作流程：
1. 先读取 `drvgpio_port.h` 中当前的逻辑 GPIO 枚举和状态定义。
2. 再读取 `drvgpio.h` 和 `drvgpio.c`，确认公共接口和 BSP 钩子表的使用方式。
3. 找到目标 MCU BSP 中负责 GPIO 时钟、端口初始化和引脚读写的 SDK 或现有板级文件。
4. 在 `bspgpio.c` 中实现逻辑引脚到物理资源的映射。
5. 实现 `bspGpioInit`、`bspGpioWrite`、`bspGpioRead` 和 `bspGpioToggle`。
6. 在 `drvgpio_port.c` 中完成 `gDrvGpioBspInterface` 绑定。
7. 检查所有逻辑 GPIO 是否都有明确映射，未映射项是否有防御性处理。
8. 总结最终的逻辑 GPIO 到物理端口、引脚、模式和电平极性的对应关系。
```

## 期望架构

- BSP 层：负责 GPIO 时钟、引脚初始化、读写和翻转实现。
- `bspgpio.h`：负责声明 BSP GPIO 接口。
- `bspgpio.c`：负责封装目标 MCU 的 GPIO 细节。
- `drvgpio_port.h`：负责项目级逻辑 GPIO 枚举和状态定义。
- `drvgpio_port.c`：负责将 `DrvGpio` 与 `bspgpio` 绑定。
- `drvgpio.h` 和 `drvgpio.c`：负责稳定的跨平台 GPIO 驱动接口。
- 应用层：只调用 `drvGpioInit`、`drvGpioWrite`、`drvGpioRead` 和 `drvGpioToggle`。

