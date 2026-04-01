# DrvUart BSP 接入提示词

当需要为新的 MCU 或新的开发板生成 UART BSP 文件时，使用下面这份提示词。本文档重点说明 `DrvUart` 期望的 BSP 输入、输出、缓冲边界和功能语义，用来指导生成 `bsp_uart.c` 和 `bsp_uart.h`，并将其接入现有 UART 抽象层。

## 目标

先完成目标 MCU 的 UART BSP 层，再通过 `DrvUart` 暴露稳定的跨 MCU UART 接口。应用层和日志层只依赖 `drvUart*` 接口，不直接接触 MCU SDK 的 UART、GPIO、DMA 和中断细节。

当前 `DrvUart` 的公共层依赖以下 BSP 钩子：

- `init`
- `transmit`
- `transmitIt`
- `transmitDma`
- `getDataLen`
- `receive`

其中根据 `drvUartHasValidBspInterface()` 的当前实现，以下钩子是初始化必需项：

- `init`
- `transmit`
- `getDataLen`
- `receive`

以下钩子当前是可选项：

- `transmitIt`
- `transmitDma`

如果可选钩子为 `NULL`，公共层应返回 `DRVUART_STATUS_UNSUPPORTED`。如果 BSP 已经提供这些能力，建议完整实现并绑定，保持接口能力一致。

## 当前接口约束

`DrvUart` 当前的公共接口、状态码和逻辑 UART 枚举来自：

- `drvuart.h`
- `drvuart_port.h`
- `drvuart_port.c`

当前工程中的逻辑 UART 定义为：

- `DRVUART_DEBUG`

当前注释说明该逻辑 UART 映射到：

- `PA9`：TX
- `PA10`：RX

如果目标板卡不止一个 UART，或者当前逻辑名称还不能准确表达用途，应先更新 `drvuart_port.h` 中的 `eDrvUartPortMap`，再让 BSP 层与之对应。

## 当前数据路径约束

当前 UART 接收路径不是单层缓冲，而是两层结构：

1. `bsp_uart.c` 维护 MCU 侧原始 DMA 循环接收缓冲区。
2. `bspUartGetDataLen()` 返回 BSP 当前可读字节数。
3. `bspUartReceive()` 从 BSP 自己的 DMA 缓冲区取出数据。
4. `drvuart.c` 中的 `drvUartSyncRxData()` 再把这些数据搬运到 `DrvUart` 自己的 ring buffer。
5. 应用层和日志层通过 `drvUartGetDataLen()`、`drvUartReceive()` 或 `drvUartGetRingBuffer()` 访问同步后的公共层数据。

这意味着：

- BSP 层负责“原始硬件接收”。
- `DrvUart` 公共层负责“对外统一缓冲语义”。
- BSP 层不应直接操作 `ringbuffer` 模块内部状态。
- 应用层不应绕过 `drvUart*` 直接读取 BSP DMA 缓冲。

## BSP 文件职责

推荐生成两个 BSP 文件：

- `bsp_uart.h`：声明 UART BSP 对外提供给 `DrvUart` 绑定层使用的函数原型。
- `bsp_uart.c`：实现具体 MCU/板级 UART 时钟、GPIO 复用、波特率、DMA、中断、收发与状态管理逻辑。

所有 MCU 相关细节都必须留在 BSP 层内部，包括但不限于：

- UART 外设实例选择
- GPIO 端口、引脚和复用功能配置
- UART 外设时钟使能
- DMA 控制器、通道和子外设映射
- NVIC 中断号和优先级
- 轮询发送、DMA 发送和 DMA 循环接收实现
- 空闲中断、半传输和全传输中断的处理方式
- 硬件缓冲区、DMA 计数器和寄存器访问

`DrvUart` 公共层不应包含任何 MCU 专有寄存器访问。

## BSP 函数输入输出与功能要求

生成的 `bsp_uart.h` 应至少声明以下接口：

```c
eDrvUartStatus bspUartInit(eDrvUartPortMap uart);
eDrvUartStatus bspUartTransmit(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length, uint32_t timeoutMs);
eDrvUartStatus bspUartTransmitIt(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length);
eDrvUartStatus bspUartTransmitDma(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length);
uint16_t bspUartGetDataLen(eDrvUartPortMap uart);
eDrvUartStatus bspUartReceive(eDrvUartPortMap uart, uint8_t *buffer, uint16_t length);
```

### 1. `bspUartInit`

输入：

- `uart`：逻辑 UART 编号，类型为 `eDrvUartPortMap`

输出：

- 返回值类型为 `eDrvUartStatus`

预计功能：

- 根据逻辑 UART 完成对应硬件资源初始化。
- 配置 UART TX/RX 引脚和复用功能。
- 使能 UART、GPIO、DMA 等必要时钟。
- 配置波特率、数据位、停止位和校验方式。
- 配置接收 DMA 和相关中断。
- 让 `drvUartInit()` 成功后，后续 `drvUartTransmit()`、`drvUartGetDataLen()` 和 `drvUartReceive()` 可以直接工作。

实现要求：

- 只初始化当前逻辑 UART 实际用到的硬件资源。
- 不要把应用层业务逻辑放入初始化函数。
- 对无效映射返回 `DRVUART_STATUS_UNSUPPORTED` 或明确错误码，而不是静默成功。
- 如果模块维护初始化状态，应保证重复初始化行为可预期。

### 2. `bspUartTransmit`

输入：

- `uart`：逻辑 UART 编号，类型为 `eDrvUartPortMap`
- `buffer`：发送缓冲区指针
- `length`：发送长度
- `timeoutMs`：发送超时，单位毫秒

输出：

- 返回值类型为 `eDrvUartStatus`

预计功能：

- 使用轮询方式发送指定数据。
- 在发送前检查 UART 是否已初始化。
- 在 DMA 发送占用期间避免与 DMA 发送并发冲突。
- 在超时或外设未就绪时返回明确状态。

实现要求：

- 在模块边界检查 `buffer == NULL` 和 `length == 0U`。
- 不要假设调用方已经做过参数检查。
- 若当前硬件设计不允许轮询与 DMA 并发发送，应在 BSP 内部明确返回 `DRVUART_STATUS_BUSY`。
- 超时语义必须在整个 BSP 中保持一致。

### 3. `bspUartTransmitIt`

输入：

- `uart`：逻辑 UART 编号，类型为 `eDrvUartPortMap`
- `buffer`：发送缓冲区指针
- `length`：发送长度

输出：

- 返回值类型为 `eDrvUartStatus`

预计功能：

- 以中断发送方式启动一次 UART 发送。

实现要求：

- 如果目标平台支持中断发送，可在 BSP 内实现。
- 如果当前项目不需要该能力，允许返回 `DRVUART_STATUS_UNSUPPORTED`。
- 不要返回伪成功，也不要留下空实现。

### 4. `bspUartTransmitDma`

输入：

- `uart`：逻辑 UART 编号，类型为 `eDrvUartPortMap`
- `buffer`：发送缓冲区指针
- `length`：发送长度

输出：

- 返回值类型为 `eDrvUartStatus`

预计功能：

- 以 DMA 方式启动一次 UART 发送。
- 在传输完成或错误中断中清理 DMA 发送忙状态。

实现要求：

- 发送前必须检查初始化状态、参数合法性和 DMA 忙状态。
- 如果使用调用方提供的发送缓冲区，必须保证契约清晰，说明 DMA 完成前该缓冲区不能被调用方改写。
- 如果 DMA 通道不可用、配置错误或当前忙，返回明确状态码。
- 推荐在 DMA 完成和 DMA 错误中断中统一收尾，避免忙标志泄漏。

### 5. `bspUartGetDataLen`

输入：

- `uart`：逻辑 UART 编号，类型为 `eDrvUartPortMap`

输出：

- 返回值为当前 BSP 接收路径中可被 `bspUartReceive()` 读取的字节数

预计功能：

- 统计当前 DMA 接收缓冲中待消费的数据量。
- 在空闲中断、半传输中断、全传输中断或查询路径中更新接收计数。
- 给 `drvuart.c` 的 `drvUartSyncRxData()` 提供可靠的“还有多少字节可搬运”的依据。

实现要求：

- 返回值语义必须与 `bspUartReceive()` 一致，也就是“当前真实可读字节数”。
- 无效 UART、未初始化或硬件不可用时，返回 `0U`。
- 不要把 `DrvUart` 自己 ring buffer 中的数据量混入这个返回值。

### 6. `bspUartReceive`

输入：

- `uart`：逻辑 UART 编号，类型为 `eDrvUartPortMap`
- `buffer`：接收输出缓冲区指针
- `length`：希望读取的字节数

输出：

- 返回值类型为 `eDrvUartStatus`

预计功能：

- 从 BSP 自己的接收缓冲区中取出指定长度的数据。
- 处理 DMA 循环缓冲区回卷。
- 在成功读取后推进 BSP 内部读指针并减少待消费计数。

实现要求：

- 当 `length` 大于当前待读字节数时，返回 `DRVUART_STATUS_NOT_READY` 或等价明确错误。
- 必须保证 `bspUartGetDataLen()` 与 `bspUartReceive()` 对同一份数据计数保持一致。
- 不能越界读取 DMA 缓冲区。
- 不要在这个接口里直接写 `DrvUart` 的 ring buffer。

## 推荐的 BSP 内部设计

为了让 `bsp_uart.c` 更容易扩展，推荐在 BSP 内部维护逻辑 UART 到物理资源的集中配置，至少把下面这些信息集中管理：

- UART 外设实例
- GPIO 端口与 TX/RX 引脚
- GPIO 复用编号
- UART 时钟
- DMA 控制器、通道和子外设编号
- NVIC 中断号
- 波特率和缓冲区大小

如果当前项目只有一个调试串口，也建议保持“逻辑 UART -> 物理资源”的集中映射，不要把资源编号散落到多个函数里。

对于接收状态，推荐维护一份局部上下文，至少覆盖：

- DMA 原始接收缓冲区
- 当前 BSP 读位置
- 最近一次 DMA 采样位置
- 当前待消费字节数
- 初始化状态
- DMA 发送忙状态

当前工程中的现有实现已经采用了这种方式。

## 与 `DrvUart` 的对接方式

生成 `bsp_uart.c/.h` 后，还需要把它们接到现有端口绑定层。当前 `drvuart_port.c` 中的 `gDrvUartBspInterface` 需要最终类似这样：

```c
stDrvUartBspInterface gDrvUartBspInterface[DRVUART_MAX] = {
	{
		.init = bspUartInit,
		.transmit = bspUartTransmit,
		.transmitIt = bspUartTransmitIt,
		.transmitDma = bspUartTransmitDma,
		.getDataLen = bspUartGetDataLen,
		.receive = bspUartReceive,
		.Buffer = gUartRxStorageDebug,
	}
};
```

这意味着：

- `drvuart_port.c` 负责绑定 BSP 钩子和 `DrvUart` 公共层缓存。
- `bsp_uart.c` 负责具体 UART、GPIO、DMA 和 IRQ 实现。
- `drvuart.c` 负责参数校验、状态管理和把 BSP 数据同步到公共层 ring buffer。

注意：

- `Buffer` 字段不是 BSP DMA 原始缓冲区，而是 `DrvUart` 公共层 ring buffer 的底层存储区。
- 当前 `DRVUART_DEBUG` 的公共层 ring buffer 容量是 `DRVUART_RECVLEN_DEBUGUART`，即 `1024` 字节。
- 当前 BSP DMA 原始接收缓冲区大小是 `256` 字节，两者不是同一层概念。

## 日志层依赖约束

当前日志模块会通过 `drvUartLogInit()`、`drvUartLogWrite()` 和 `drvUartLogGetInputBuffer()` 复用 `DRVUART_DEBUG`。因此 UART BSP 在设计时还应考虑：

- 默认日志输出可能依赖 `drvUartTransmitDma()`。
- 默认日志输入可能依赖 `drvUartGetRingBuffer()`。
- 如果关闭 UART 控制台功能，应优先在 port 层通过宏开关控制，而不是破坏公共 UART 接口语义。

## 提示词模板

```md
你正在 Windows 环境下。

任务要求：
1. 先检查 `rule.md`，并遵守仓库中的编码规范。
2. 检查 `DrvUart`，理解当前 UART 驱动抽象方式、逻辑 UART 枚举、BSP 钩子表以及接收缓冲分层。
3. 创建或补全 `bsp_uart.c` 和 `bsp_uart.h`，实现 UART BSP 层。
4. 所有 MCU 相关细节，例如 UART 外设、GPIO 复用、时钟、DMA、IRQ、寄存器访问和 SDK 调用，都必须放在 BSP 层内部。
5. `bsp_uart.h` 至少要声明 `bspUartInit`、`bspUartTransmit`、`bspUartTransmitIt`、`bspUartTransmitDma`、`bspUartGetDataLen` 和 `bspUartReceive`。
6. `bspUartInit` 的输入是 `eDrvUartPortMap uart`，输出是 `eDrvUartStatus`；它的职责是初始化指定逻辑 UART 对应的硬件资源。
7. `bspUartTransmit` 的输入是 `eDrvUartPortMap uart`、`const uint8_t *buffer`、`uint16_t length` 和 `uint32_t timeoutMs`，输出是 `eDrvUartStatus`；它的职责是完成阻塞或轮询发送。
8. `bspUartTransmitIt` 的输入是 `eDrvUartPortMap uart`、`const uint8_t *buffer` 和 `uint16_t length`，输出是 `eDrvUartStatus`；如不支持，可明确返回 `DRVUART_STATUS_UNSUPPORTED`。
9. `bspUartTransmitDma` 的输入是 `eDrvUartPortMap uart`、`const uint8_t *buffer` 和 `uint16_t length`，输出是 `eDrvUartStatus`；它的职责是启动 DMA 发送并处理忙状态。
10. `bspUartGetDataLen` 的输入是 `eDrvUartPortMap uart`，输出是 `uint16_t`；它的职责是返回 BSP 当前真实可读的接收字节数。
11. `bspUartReceive` 的输入是 `eDrvUartPortMap uart`、`uint8_t *buffer` 和 `uint16_t length`，输出是 `eDrvUartStatus`；它的职责是从 BSP 自己的接收缓冲区读取数据，并推进读指针。
12. 如果当前 `drvuart_port.h` 中的逻辑 UART 枚举不完整，先更新它，再让 BSP 实现与之对应。
13. 生成 BSP 文件后，更新 `drvuart_port.c`，将 `gDrvUartBspInterface` 绑定到对应 BSP 函数，并为 `DrvUart` 公共层 ring buffer 提供存储区。
14. 不要让应用层、日志层或业务模块直接调用 MCU SDK 的 UART 接口。应用层只能调用 `drvUart*` 接口。
15. 改动尽量小，不要重构无关模块。

需要产出：
1. 完整的 `bsp_uart.c` 和 `bsp_uart.h`。
2. 对 `drvuart_port.c` 的必要更新，用于绑定 BSP 钩子和公共层接收存储区。
3. 对 `drvuart_port.h` 的必要更新，用于补充逻辑 UART 枚举或接收容量配置。
4. 一段简短说明，描述逻辑 UART 与物理 UART、GPIO、DMA、IRQ 和缓冲区之间的映射关系。
5. 一段验证说明，描述已经检查了什么，以及还剩哪些硬件相关风险。

实现约束：
- 代码中的注释必须使用英文。
- 每个文件只负责一个清晰职责。
- 不要增加占位接口或伪实现。
- 保持现有 `drvUart*` 调用方式向后兼容。
- 不要把 MCU 相关宏、寄存器和 SDK 细节泄漏到 `drvuart.c`。
- `bspUartGetDataLen` 与 `bspUartReceive` 的数据语义必须在整个模块内保持一致。
- `bspUartReceive` 只负责读取 BSP 原始接收缓冲，不直接写 `DrvUart` 的 ring buffer。
- 如果实现 DMA 发送，必须处理 DMA 忙状态和完成收尾。

推荐工作流程：
1. 先读取 `drvuart_port.h` 中当前的逻辑 UART 枚举和接收容量配置。
2. 再读取 `drvuart.h` 和 `drvuart.c`，确认公共接口、状态码和 BSP 钩子表的使用方式。
3. 读取现有 `bsp_uart.c`、`bsp_uart.h` 或板级 UART 文件，确认目标 MCU 的 GPIO、UART、DMA 和 IRQ 资源。
4. 在 `bsp_uart.c` 中实现逻辑 UART 到物理资源的映射。
5. 实现 `bspUartInit`、`bspUartTransmit`、`bspUartTransmitIt`、`bspUartTransmitDma`、`bspUartGetDataLen` 和 `bspUartReceive`。
6. 在 `drvuart_port.c` 中完成 `gDrvUartBspInterface` 绑定，并配置 `DrvUart` 公共层 ring buffer 的底层存储区。
7. 检查接收路径的两层缓冲语义是否一致，避免重复计数、越界或数据丢失。
8. 总结最终的逻辑 UART 到物理 UART、GPIO、DMA、IRQ 和缓冲区大小的对应关系。
```

## 期望架构

- BSP 层：负责 UART、GPIO、DMA、IRQ、轮询发送、DMA 发送和原始接收缓冲实现。
- `bsp_uart.h`：负责声明 BSP UART 接口。
- `bsp_uart.c`：负责封装目标 MCU 的 UART 细节。
- `drvuart_port.h`：负责项目级逻辑 UART 枚举和容量配置。
- `drvuart_port.c`：负责将 `DrvUart` 与 `bsp_uart` 绑定，并提供公共层 ring buffer 存储。
- `drvuart.h` 和 `drvuart.c`：负责稳定的跨平台 UART 驱动接口、参数校验和接收同步逻辑。
- 应用层与日志层：只调用 `drvUartInit`、`drvUartTransmit`、`drvUartTransmitIt`、`drvUartTransmitDma`、`drvUartGetDataLen`、`drvUartReceive` 和 `drvUartGetRingBuffer`。
