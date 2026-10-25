# 仓库维护与 Insights

## GitHub 入口

| 入口 | 用途 |
|---|---|
| [About](https://github.com/valiidea/fluorescent-fiber-thermometer) | 简介、主题标签与文档链接 |
| [Wiki](https://github.com/valiidea/fluorescent-fiber-thermometer/wiki) | 按主题阅读导航 |
| [Actions](https://github.com/valiidea/fluorescent-fiber-thermometer/actions) | 构建、测试和工作流检查记录 |
| [Community Standards](https://github.com/valiidea/fluorescent-fiber-thermometer/community) | 社区健康文件检查 |
| [Pulse](https://github.com/valiidea/fluorescent-fiber-thermometer/pulse) | GitHub 自动汇总的近期活动 |
| [Traffic](https://github.com/valiidea/fluorescent-fiber-thermometer/graphs/traffic) | 有权限的维护者查看访问与克隆统计 |
| [Security](https://github.com/valiidea/fluorescent-fiber-thermometer/security/policy) | 安全政策与私密报告入口 |

Insights 不是可填充的介绍页面。贡献者、提交频率和流量等由 GitHub 根据真实活动生成，可能有统计延迟。维护社区健康文件、清晰的提交和可复现检查，比人为制造活动更有用。

## 检查与依赖

CI 在 Ubuntu 的 GCC/Clang 和 macOS 的 Clang 上构建程序与静态库，运行原始演示、C/Python 测试及完整实验，并保存数值报告。另以 Ubuntu Clang 执行 AddressSanitizer/UndefinedBehaviorSanitizer 检查。工作流只有仓库读取权限。Dependabot 每月检查 GitHub Actions 依赖；项目没有第三方 C 包依赖。Matplotlib 仅用于可选绘图。

## 发布 Wiki

`docs/wiki/` 是 Wiki 导航源文件；技术内容链接到主仓库 `docs/`，避免复制后漂移。首次使用需在 GitHub Wiki 页面创建首页，之后可在独立临时目录中操作：

```sh
git clone git@github.com:valiidea/fluorescent-fiber-thermometer.wiki.git /tmp/fft-wiki
cp docs/wiki/*.md /tmp/fft-wiki/
git -C /tmp/fft-wiki diff
# 确认只包含预期文档变更后：
git -C /tmp/fft-wiki add '*.md'
git -C /tmp/fft-wiki commit -m "docs: refresh wiki navigation"
git -C /tmp/fft-wiki push
```

上述目标目录必须尚不存在；已有 Wiki 内容先检查并合并，避免覆盖他人修改。不为同步额外创建长期访问令牌。

Gitee 使用独立的 `git@gitee.com:valiidea/fluorescent-fiber-thermometer.wiki.git`。同步导航时将仓库文档链接替换为 Gitee 地址、`/wiki/` 替换为 `/wikis/`；Gitee 不将 `_Sidebar` / `_Footer` 自动作为 GitHub 风格部件使用，应将导航放在 Home 页面。主仓库通过 `git push origin main` 和 `git push gitee main` 分别同步；目前没有后台自动镜像。

## 版本发布

仅在明确的版本里程碑完成后创建标签与 Release，说明实际变更、验证环境和已知限制。未发布的改动记在 [CHANGELOG](../CHANGELOG.md)，不把文档补齐视为硬件验证或产品发布。
