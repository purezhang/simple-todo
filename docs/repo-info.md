# 代码库信息

## GitHub

- 仓库地址: https://github.com/purezhang/simple-todo.git
- 用户名: purezhang
- 访问方式: HTTPS (通过 Git Credential Manager 自动认证)
- 远程名: `github`, `simple-todo@github`

## Gitea (本地)

- 仓库地址: http://192.168.86.150:8810/yu/simple-todo.git
- 用户名: yu
- Token: b6a94c5fb7876128e1dfcde347ff3768e9b4f0e3 (已保存到 `local/env/gitea-token.txt`)
- 远程名: `origin`

## 远程仓库配置

```bash
# 查看所有远程
git remote -v

# GitHub
git remote add github https://github.com/purezhang/simple-todo.git
git push github v0.5.0

# Gitea (本地，带 token)
git remote add origin http://192.168.86.150:8810/yu/simple-todo.git
git push origin v0.5.0
```

## Token 设置

### GitHub
1. Settings → Developer settings → Personal access tokens → Tokens (classic)
2. 生成 token，选择 `repo` 权限
3. Windows Credential Manager 会自动保存，无需手动填写

### Gitea
1. 设置 → 应用 → 管理访问令牌
2. 生成 token，保存到 `local/env/gitea-token.txt`
