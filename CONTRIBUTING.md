# 贡献指南

欢迎改进中文说明、标准 C 示例、边界检查和可重复的测试。项目保持纯 C11、合成数据和无量纲输出的教学定位。

## 开始贡献

1. 阅读 [README](README.md)、[设计说明](docs/design.md) 和 [接口说明](docs/api.md)。
2. 搜索已有 Issue；较大的变更先通过 Issue 说明问题、预期收益和范围。
3. Fork 仓库并创建主题分支；每个 PR 聚焦一个问题。
4. 修改代码时补充能复现问题或验证新行为的测试；只改文字时检查链接和命令即可。
5. 提交 PR，描述修改原因、行为变化和实际执行的验证。

## 本地验证

```sh
make run test
make CC=clang BUILD=build/clang test
make CC=gcc BUILD=build/gcc test
```

macOS 的 `gcc` 可能也是 Apple Clang，请用 `gcc --version` 确认。CI 在 Ubuntu 上分别运行真正的 GCC 和 Clang，并在 macOS 上运行 Clang。

保留现有四空格缩进与模块边界；使用标准 C11，不引入芯片 SDK、汇编或未说明的第三方实现。新增公开接口时同步更新头文件及接口文档。不得将合成输出描述为真实测温结果或标定参数。

## 文档和 Wiki

主仓库的 `docs/` 是技术文档来源，`docs/wiki/` 保存 Wiki 导航页面。修改入口时同步检查两处链接，维护者按 [维护指南](docs/maintenance.md) 发布 Wiki。

## 评审与许可

维护者检查项目范围、可读性、测试和文档一致性。提交贡献意味着你有权提供这些内容，并同意按仓库现有 MIT 许可证分发。请勿提交密钥、个人信息或无权公开的设备资料。

交流遵循 [行为准则](CODE_OF_CONDUCT.md)。疑似安全问题请按 [安全政策](SECURITY.md) 私下报告。
