# DrvUart BSP 接入提示词

当需要为新的 MCU 增加 UART BSP 支持时，使用下面这份提示词。

## 目标

先完成目标 MCU 的 UART BSP 层，再把稳定的公共 UART 接口整理到 `DrvUart` 中，这样应用层只依赖 `drvUart*` 接口，不需要关心具体使用的是哪一款 MCU。

## 提示词模板

```md
你正在 Windows 环境下。

任务要求：
1. 先检查 `rule.md`，并遵守仓库中的编码规范。
2. 检查 `DrvUart`，理解当前 UART 驱动抽象方式、状态码和接收缓冲设计。
3. 在目标 MCU 的 BSP 层中创建或补全 UART 初始化、发送、接收中断处理和可选 DMA 发送实现。
4. 所有 MCU 相关细节，例如时钟使能、GPIO 复用、波特率、FIFO、DMA 通道、IRQ 号和寄存器访问，都必须放在 BSP 层内部。
5. BSP UART 实现完成后，再通过 `DrvUart` 暴露稳定的跨 MCU 公共接口。
6. 如果当前逻辑串口列表不完整，先更新 `drvuart_portmap.h`，再让 BSP 实现与之对应。
7. 不要让应用层直接调用 MCU SDK 的 UART 接口。应用层只能调用 `drvUart*` 接口。
8. 接收路径要和 `DrvUart` 中的环形缓冲设计对齐，ISR 或回调收到的数据必须进入对应 UART 的 RX ring buffer。
9. 改动尽量小，不要重构无关模块。

需要产出：
1. 目标 MCU 对应的完整 BSP UART 源文件和头文件，或者对现有 BSP UART 文件的补全。
2. 对 `DrvUart/drvuart_portmap.h` 的必要更新。
3. 对 `DrvUart/drvuart.h` 和 `drvuart.c` 的必要更新，保证它们可以干净地转发到 BSP 实现。
4. 一段简短说明，描述逻辑 UART 和物理 UART 外设、TX/RX 引脚、波特率配置之间的映射关系。
5. 一段验证说明，描述已经检查了什么，以及还剩下哪些风险或缺口。

实现约束：
- 代码中的注释必须使用英文。
- 每个文件只负责一个清晰职责。
- 不要增加占位接口或伪实现。
- 除非 UART 抽象本身需要，否则不要改变应用层行为。
- 保持现有 `drvUart*` 调用方式的向后兼容。
- `drvUartTransmit`、`drvUartTransmitIt` 和 `drvUartTransmitDma` 的语义要清晰区分，不能混用阻塞和异步行为。
- `drvUartReceive` 和 `drvUartGetDataLen` 必须基于接收缓冲的真实状态工作，不能直接暴露 MCU SDK 细节。

推荐工作流程：
1. 先读取 `DrvUart/drvuart_portmap.h` 中当前的逻辑 UART 枚举。
2. 再读取 `DrvUart/drvuart.h` 和 `drvuart.c`，确认公共接口、状态码和 RX ring buffer 的使用方式。
3. 找到目标 MCU BSP 中应该负责 UART 时钟、引脚复用、中断和 DMA 的文件。
4. 基于逻辑 UART 映射，实现 BSP 层的 init/transmit/transmitIt/transmitDma 和 RX ISR 或回调处理。
5. 在 BSP 接收中断或回调里，把收到的数据写入 `DrvUart` 对应端口的 ring buffer。
6. 将 `drvuart.c` 连接到 BSP 层函数，并保证错误码在公共层可控。
7. 验证应用层不再依赖任何 MCU 专有 UART 调用。
8. 总结最终的逻辑 UART 到物理 UART、引脚和中断资源的映射关系。
```

## 期望架构

- BSP 层：负责 MCU 相关的 UART 配置、发送实现、中断处理和 DMA 接入。
- `drvuart_portmap.h`：负责项目级逻辑 UART 标识。
- `drvuart.h` 和 `drvuart.c`：负责稳定的跨平台 UART 驱动接口和 RX ring buffer 管理。
- 应用层：只调用 `drvUartInit`、`drvUartTransmit`、`drvUartTransmitIt`、`drvUartTransmitDma`、`drvUartReceive` 和 `drvUartGetDataLen`。
- bsp层的程序需要放在这两行注释中间
```
/*************************Bsp Area***********************/

/*******************************************************/
```

## 使用说明

在让 AI 为新的 MCU 增加 UART 支持之前，先把模板中的目标 MCU 信息补全，并明确写清楚 BSP 所在路径、使用的物理 UART 实例以及接收数据进入 ring buffer 的方式。
