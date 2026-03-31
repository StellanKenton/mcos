# DrvGpio BSP 接入提示词

当需要为新的 MCU 增加 GPIO BSP 支持时，使用下面这份提示词。

## 目标

先完成目标 MCU 的 GPIO BSP 层，再把稳定的公共 GPIO 接口整理到 `DrvGpio` 中，这样应用层只依赖 `drvGpio*` 接口，不需要关心具体使用的是哪一款 MCU。

## 提示词模板

```md
你正在 Windows 环境下。

任务要求：
1. 先检查 `rule.md`，并遵守仓库中的编码规范。
2. 检查 `DrvGpio`，理解当前 GPIO 驱动抽象方式。
3. 在目标 MCU 的 BSP 层中创建或补全 GPIO 初始化和访问实现。
4. 所有 MCU 相关细节，例如时钟使能、引脚复用、上下拉、速度、有效电平和寄存器访问，都必须放在 BSP 层内部。
5. BSP GPIO 实现完成后，再通过 `DrvGpio` 暴露稳定的跨 MCU 公共接口。
6. 如果当前逻辑引脚列表不完整，先更新 `drvgpio_pinmap.h`，再让 BSP 实现与之对应。
7. 不要让应用层直接调用 MCU SDK 的 GPIO 接口。应用层只能调用 `drvGpio*` 接口。
8. 改动尽量小，不要重构无关模块。

需要产出：
1. 目标 MCU 对应的完整 BSP GPIO 源文件和头文件，或者对现有 BSP GPIO 文件的补全。
2. 对 `DrvGpio/drvgpio_pinmap.h` 的必要更新。
3. 对 `DrvGpio/drvgpio.h` 和 `drvgpio.c` 的必要更新，保证它们可以干净地转发到 BSP 实现。
4. 一段简短说明，描述逻辑引脚和物理引脚的映射关系。
5. 一段验证说明，描述已经检查了什么，以及还剩下哪些风险或缺口。

实现约束：
- 代码中的注释必须使用英文。
- 每个文件只负责一个清晰职责。
- 不要增加占位接口或伪实现。
- 除非 GPIO 抽象本身需要，否则不要改变应用层行为。
- 保持现有 `drvGpio*` 调用方式的向后兼容。

推荐工作流程：
1. 先读取 `DrvGpio/drvgpio_pinmap.h` 中当前的逻辑引脚枚举。
2. 找到目标 MCU BSP 中应该负责 GPIO 时钟、模式和引脚路由的文件。
3. 基于逻辑引脚映射，实现 BSP 层的 init/write/read/toggle 等函数。
4. 将 `drvgpio.c` 连接到 BSP 层函数。
5. 验证应用层不再依赖任何 MCU 专有 GPIO 调用。
6. 总结最终的逻辑引脚到物理引脚映射关系。
```

## 期望架构

- BSP 层：负责 MCU 相关的 GPIO 配置和寄存器访问。
- `drvgpio_pinmap.h`：负责项目级逻辑引脚标识。
- `drvgpio.h` 和 `drvgpio.c`：负责稳定的跨平台 GPIO 驱动接口，以及通过结构体统一管理 GPIO 状态缓存和 BSP 函数指针。
- 应用层：只调用 `drvGpioInit`、`drvGpioWrite`、`drvGpioRead` 和 `drvGpioToggle`。

## BSP 函数指针约定

`DrvGpio` 中建议使用如下结构体承载 GPIO 状态缓存和 BSP 回调：

```c
typedef struct stDrvGpioBspInterface {
	eDrvGpioPinState pinStates[DRVGPIO_MAX];
	void (*init)(void);
	void (*write)(eDrvGpioPinMap pin, eDrvGpioPinState state);
	eDrvGpioPinState (*read)(eDrvGpioPinMap pin);
	void (*toggle)(eDrvGpioPinMap pin);
} stDrvGpioBspInterface;
```

其中 `pinStates` 由 `DrvGpio` 维护，用来缓存每个逻辑引脚最近一次读写后的状态；函数指针由 BSP 层提供具体实现。

### `init`

- 输入：无。
- 输出：无返回值。
- BSP 需要完成的事情：初始化所有已经在 `drvgpio_pinmap.h` 中声明的逻辑引脚，包括时钟、模式、上下拉、默认电平和必要的复用配置。
- 语义要求：调用完成后，`DrvGpio` 应该可以立刻对所有有效逻辑引脚执行 `read/write/toggle`。

### `write`

- 输入：`pin` 表示逻辑引脚编号，`state` 表示目标逻辑电平，取值只能是 `DRVGPIO_PIN_RESET` 或 `DRVGPIO_PIN_SET`。
- 输出：无返回值。
- BSP 需要完成的事情：把逻辑电平转换成目标 MCU/板级的真实物理电平，并写入对应物理 GPIO。
- 语义要求：该函数只负责执行写操作，不应要求上层了解有效高低电平或物理引脚号。

### `read`

- 输入：`pin` 表示逻辑引脚编号。
- 输出：返回该逻辑引脚当前逻辑状态，必须是 `DRVGPIO_PIN_RESET`、`DRVGPIO_PIN_SET` 或在无法判定时返回 `DRVGPIO_PIN_STATE_INVALID`。
- BSP 需要完成的事情：从目标 MCU 的物理 GPIO 读取真实电平，再映射回项目统一的逻辑电平语义。
- 语义要求：返回值必须是逻辑状态，不是芯片原始寄存器值。

### `toggle`

- 输入：`pin` 表示逻辑引脚编号。
- 输出：无返回值。
- BSP 需要完成的事情：翻转目标逻辑引脚当前输出状态；如果底层没有直接翻转能力，也可以通过 `read + write` 在 BSP 内部实现。
- 语义要求：翻转后再次执行 `read`，应得到翻转后的逻辑状态。

## BSP 侧最少输入与最少输出

- BSP 的最少输入是：逻辑引脚枚举 `eDrvGpioPinMap` 和逻辑状态枚举 `eDrvGpioPinState`。
- BSP 的最少输出是：一组可赋给 `stDrvGpioBspInterface` 的函数实现，它们必须覆盖 `init/write/read/toggle` 四类操作。
- 所有物理 GPIO 编号、方向、上下拉、有效电平和寄存器/SDK 访问，都必须封装在 BSP 内部，不能泄漏到 `DrvGpio` 或应用层。

## 使用说明

在让 AI 为新的 MCU 增加 GPIO 支持之前，先把模板中的目标 MCU 信息补全，并明确写清楚 BSP 所在路径。