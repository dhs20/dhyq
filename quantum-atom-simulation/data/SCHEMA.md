# Data Schema Notes

## Core records

当前建议的核心记录对象定义在：

- [include/quantum/data/Records.h](/D:/project/dhyq/quantum-atom-simulation/include/quantum/data/Records.h)
- [include/quantum/meta/MethodMetadata.h](/D:/project/dhyq/quantum-atom-simulation/include/quantum/meta/MethodMetadata.h)

## Record groups

### Static metadata

- `ElementRecord`
- `IsotopeRecord`

### Theoretical products

- `ConfigurationRecord`
- `StateRecord`
- `TransitionRecord`

### Provenance and validation

- `MethodStamp`
- `ValidationRecord`
- `SourceRecord`
- `ProvenanceRecord`

## Design rules

- 元数据完整性不等于高精度理论完整性
- 每条能级/跃迁记录都应尽量带来源和验证状态
- 本地示例数据、外部数据库映射、用户扩展数据必须可区分
- 数据表述必须能明确说明近似方法和适用范围
