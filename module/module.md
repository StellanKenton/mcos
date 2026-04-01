# Module 通用生成指南

## 1. 目标

本文档基于 `mpu6050` 模块的实现方式抽象出一套通用的 module 层生成规范，用于指导后续生成其他可复用模块。

module 层的核心目标如下：

- 对上提供清晰、稳定、易复用的业务接口。
- 对下只依赖 drv 层公共接口，不直接依赖 bsp 层实现。
- 将可复用逻辑放在模块核心文件中，将板级适配、总线映射、延时和底层接口选择放在 port 层。
- 支持同一模块实例在运行时绑定不同 drv 实现。
- 保持接口参数校验、状态返回和错误处理风格一致。

## 2. 推荐目录结构

每个模块建议放在独立目录中，目录结构如下：

```text
<module>/
    <module>.h
    <module>.c
    <module>_port.h
    <module>_port.c
    <module>.md
```

各文件职责如下：

- `<module>.h`：模块公共类型、状态码、配置结构、实例结构、业务接口声明。
- `<module>.c`：模块核心逻辑、参数校验、状态流转、寄存器或协议读写封装。
- `<module>_port.h`：port 层公共类型、drv 绑定结构、底层接口函数指针、时序相关配置。
- `<module>_port.c`：将模块绑定到具体 drv 实现，处理默认映射、状态转换、延时和平台适配。
- `<module>.md`：模块说明文档，描述模块用途、文件职责、初始化流程、公共 API 和验证方式。

## 3. 分层原则

### 3.1 module 核心层

`<module>.c` 和 `<module>.h` 应只关心模块本身的业务逻辑，不应直接包含 bsp 相关头文件。

核心层应负责：

- 默认配置初始化。
- 模块实例合法性检查。
- 模块 ready 状态管理。
- 对底层 drv 状态码进行统一映射。
- 对外提供读、写、配置、控制、查询类接口。
- 纯逻辑数据解析与结果转换。

核心层不应负责：

- 具体硬件引脚定义。
- 具体外设编号硬编码。
- 平台延时实现。
- 直接调用 bsp 层函数。

### 3.2 port 适配层

`<module>_port.c` 和 `<module>_port.h` 负责将 module 层与具体 drv 层连接起来。

port 层应负责：

- 定义可选底层类型，例如硬件接口、软件接口、空接口。
- 提供默认绑定。
- 提供切换到底层 drv 的辅助函数。
- 封装底层 drv 的 init、read、write 等适配函数。
- 提供模块运行依赖的延时函数或其他平台钩子。
- 将底层 drv 返回值映射到 module 可识别的中间状态。

port 层不应负责：

- 实现模块业务规则。
- 暴露不必要的板级细节到模块核心层。

## 4. 命名约定

以模块名 `<module>` 为统一前缀，遵循项目 camelCase 风格。

推荐命名模式如下：

- 状态枚举：`e<Module>Status`
- 配置或实例结构体：`st<Module>Device`、`st<Module>Config`、`st<Module>Sample`
- port 绑定结构：`st<Module>PortXxxBinding`
- port 接口表：`st<Module>PortXxxInterface`
- 获取默认配置：`<module>GetDefaultConfig()`
- 初始化：`<module>Init()`
- 就绪检查：`<module>IsReady()`
- 读接口：`<module>ReadXxx()`
- 写接口：`<module>WriteXxx()`
- 控制接口：`<module>SetXxx()`
- port 默认绑定：`<module>PortGetDefaultBinding()`
- port 切换底层：`<module>PortSetHardwareXxx()`、`<module>PortSetSoftwareXxx()`

如果模块没有“设备”语义，也可以将 `st<Module>Device` 替换为更合适的实例名，但仍建议保留一个实例结构体承载运行状态。

## 5. 推荐数据结构

### 5.1 状态码

每个模块建议定义自己的状态枚举，避免直接暴露底层 drv 状态码。推荐至少包含以下成员：

- `OK`
- `INVALID_PARAM`
- `NOT_READY`
- `BUSY`
- `TIMEOUT`
- `UNSUPPORTED`
- `ERROR`

如果模块依赖外设 ID、自检、校验和等流程，可以增加以下成员：

- `DEVICE_ID_MISMATCH`
- `CRC_ERROR`
- `OUT_OF_RANGE`

### 5.2 实例结构体

推荐让一个实例结构体同时承载：

- 底层绑定信息。
- 用户可配置参数。
- 模块当前在线或就绪状态。
- 必要的缓存或上下文字段。

可以参考如下字段组织方式：

```c
typedef struct st<Module>Device {
    st<Module>PortBinding binding;
    uint8_t address;
    uint8_t option;
    bool isOnline;
    struct data;
} st<Module>Device;
```

如果配置项较多，也可以拆为 `st<Module>Config` 和 `st<Module>Device` 两层结构，但要保持接口清晰。

### 5.3 port 接口表

当 module 需要适配多个 drv 实现时，推荐在 port 层定义函数指针表。例如：

```c
typedef e<Module>DrvStatus (*<module>PortInitFunc)(uint8_t bus);
typedef e<Module>DrvStatus (*<module>PortWriteFunc)(uint8_t bus,
                                                    uint8_t address,
                                                    const uint8_t *buffer,
                                                    uint16_t length);
typedef e<Module>DrvStatus (*<module>PortReadFunc)(uint8_t bus,
                                                   uint8_t address,
                                                   uint8_t *buffer,
                                                   uint16_t length);

typedef struct st<Module>PortInterface {
    <module>PortInitFunc init;
    <module>PortWriteFunc write;
    <module>PortReadFunc read;
} st<Module>PortInterface;
```

如果模块需要寄存器读写，可以将 `registerBuffer` 和 `registerLength` 也纳入接口。

## 6. 推荐公共 API 形态

一个通用 module 建议优先提供以下接口：

1. `<module>GetDefaultConfig()`
2. `<module>Init()`
3. `<module>IsReady()`
4. `<module>ReadXxx()`
5. `<module>WriteXxx()` 或 `<module>SetXxx()`
6. `<module>GetXxx()`

说明如下：

- `GetDefaultConfig`：初始化实例或配置结构，避免调用方遗漏字段。
- `Init`：完成底层初始化、设备探测、自检、默认配置下发等步骤。
- `IsReady`：仅用于快速判断实例当前是否可正常访问。
- `ReadXxx`：读取模块数据、状态或寄存器。
- `WriteXxx` 或 `SetXxx`：设置寄存器、模式或控制位。
- `GetXxx`：获取运行状态、配置、派生值或转换结果。

如果某个模块只需要极少接口，也应尽量保留 `GetDefaultConfig` 和 `Init` 这两个入口，保持使用方式一致。

## 7. 推荐初始化流程

可以按下面顺序组织初始化逻辑：

1. 校验实例指针和基础参数是否合法。
2. 校验底层绑定是否合法。
3. 获取 port 层接口表，并确认接口函数完整可用。
4. 调用底层 `init`。
5. 清除实例在线状态，避免旧状态残留。
6. 读取关键标识信息，例如设备 ID、版本号或握手结果。
7. 如需复位，发送复位命令并等待必要延时。
8. 下发模块关键配置。
9. 全部成功后将实例标记为 ready 或 online。
10. 任一步骤失败时立即返回明确状态码。

这套流程适用于传感器、通信外设、协议封装器以及其他需要显式 bring-up 的模块。

## 8. `.h` 文件编写建议

头文件建议包含以下内容：

- include guard
- 必要标准库头文件
- 对应的 `<module>_port.h`
- `extern "C"` 兼容块
- 宏定义
- 状态枚举
- 配置或实例结构体
- 公开数据结构体
- 公共函数声明

头文件中应避免：

- 定义内部静态辅助函数
- 暴露不必要的私有寄存器细节
- 包含与公共接口无关的重量级头文件

## 9. `.c` 文件编写建议

源文件建议包含以下内容：

- 必要的内部 `static` 辅助函数声明
- 参数合法性检查函数
- ready 条件检查函数
- 状态映射函数
- 底层接口获取函数
- 对外公共函数实现
- 纯内部辅助逻辑

推荐的内部辅助函数模式：

- `<module>IsValidXxx()`
- `<module>IsReadyForTransfer()`
- `<module>MapDrvStatus()`
- `<module>GetXxxInterface()`
- `<module>ReadInternal()`
- `<module>WriteInternal()`

内部辅助函数统一使用 `static`，不要暴露到头文件。

## 10. `port` 层编写建议

`<module>_port.h` 和 `<module>_port.c` 建议至少提供以下内容：

- 默认绑定宏或延时宏。
- 底层类型枚举。
- 底层绑定结构体。
- drv 中间状态枚举。
- 函数指针接口表。
- 获取默认绑定函数。
- 设置具体 drv 绑定函数。
- 校验绑定是否合法的函数。
- 获取接口表函数。
- 延时函数或其他平台相关钩子。

如果一个模块可能同时跑在硬件接口和软件接口上，应优先用接口表而不是在核心层堆叠 `switch`。

## 11. 错误处理要求

生成模块时应遵循以下要求：

- 模块边界必须校验指针、长度、枚举值和关键参数。
- 不要静默忽略底层初始化失败、通信失败、超时或设备不匹配。
- 公共接口返回明确状态码，不要用隐式全局状态表达错误。
- 初始化失败后，应让实例保持不可用状态。
- 对于可恢复错误，可以保留重试空间，但不要在底层无限阻塞。

## 12. 可复用性要求

为了让模块可以迁移到其他项目，生成时应满足以下约束：

- 核心层不依赖具体芯片寄存器头文件。
- 核心层不直接调用 `bspXxx()` 接口。
- 底层访问统一经由 port 层函数指针表。
- 默认配置通过显式函数写入，不依赖隐式静态初始化。
- 一个模块实例应能独立持有自己的底层绑定信息。

## 13. 生成新模块时的建议步骤

1. 明确模块对外职责，是传感器、通信设备还是纯协议模块。
2. 确定模块核心依赖的是哪一类 drv 公共接口。
3. 先定义 `<module>_port.h` 中的绑定结构和接口表。
4. 再定义 `<module>.h` 中的状态码、实例结构体和对外 API。
5. 在 `<module>.c` 中实现默认配置、初始化流程和核心业务接口。
6. 在 `<module>_port.c` 中完成底层适配、默认映射和延时函数。
7. 编写 `<module>.md`，说明用途、文件职责、初始化流程和验证方式。
8. 最后进行编译验证，并记录仍需硬件验证的部分。

## 14. 新模块文档模板

每个模块目录下的 `<module>.md` 建议采用如下结构：

```md
# <MODULE> Module

## Overview

- 说明模块作用。
- 说明模块依赖的 drv 抽象。
- 说明实例是否支持运行时切换底层接口。
- 说明默认地址、默认端口或默认模式。

## Files

- `<module>.h`：说明公共接口职责。
- `<module>.c`：说明核心逻辑职责。
- `<module>_port.h`：说明 port 公共接口职责。
- `<module>_port.c`：说明底层适配职责。

## Initialization Flow

1. 调用 `<module>GetDefaultConfig()`。
2. 按需修改默认配置。
3. 按需切换到底层 drv 绑定。
4. 调用 `<module>Init()`。

## Public API

- `<module>Init(...)`：说明初始化行为。
- `<module>ReadXxx(...)`：说明读取行为。
- `<module>WriteXxx(...)`：说明写入行为。
- `<module>SetXxx(...)`：说明控制行为。

## Verification Notes

- 说明编译验证情况。
- 说明仍需硬件验证或联调验证的内容。
```

## 15. 生成检查清单

生成新模块后，至少检查以下项目：

- 是否只有 module 核心层依赖 `<module>_port.h`，而不直接依赖 bsp。
- 是否所有公共接口都带有模块名前缀。
- 是否存在 `GetDefaultConfig` 和 `Init`。
- 是否对参数和状态进行了充分校验。
- 是否有明确的 ready 或 online 状态。
- 是否对底层状态做了统一映射。
- 是否将平台相关逻辑隔离在 port 层。
- 是否补充了模块说明文档。
- 是否完成编译验证。
- 是否明确标注仍需硬件验证的内容。

## 16. 适用范围

本模板特别适合以下类型的模块：

- 传感器模块
- 外设芯片模块
- 基于 IIC、SPI、UART 的器件模块
- 对底层 drv 有明确依赖的协议封装模块

对于纯算法模块或完全不涉及底层驱动的工具模块，可以简化 port 层，但仍建议保留清晰的实例结构体、状态返回和文档结构。