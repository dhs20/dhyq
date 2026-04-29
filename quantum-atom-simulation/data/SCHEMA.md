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

## Offline assets

- `assets/data/reference_catalog.json`
  - small curated teaching spectrum catalog
  - records atomic number, charge state, upper/lower quantum numbers, wavelength, energy, provenance
  - preferred over the legacy CSV when available
- `assets/data/isotopes.json`
  - small curated isotope catalog for teaching anchors
  - records atomic number, mass number, atomic mass, natural abundance
- `assets/data/nist_reference_lines.csv`
  - compatibility fallback for the earlier local reference-line path
