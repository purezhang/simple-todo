# 代码库信息

## GitHub

- 仓库地址: https://github.com/purezhang/simple-todo.git
- 用户名: purezhang
- 访问方式: HTTPS

## Gitea (本地)

- 仓库地址: http://192.168.86.150:8810/yu/simple-todo.git
- 用户名: yu
- Token: [请在 local/env/gitea-token.txt 中填写]

## 远程仓库配置

```bash
# GitHub
git remote add github https://github.com/purezhang/simple-todo.git

# Gitea (本地)
git remote add gitea http://192.168.86.150:8810/yu/simple-todo.git

# 推送到远程
git push github v0.5.0
git push gitea v0.5.0
```

## Token 设置

### GitHub
1. Settings → Developer settings → Personal access tokens → Tokens (classic)
2. 生成 token，保存到 `local/env/github-token.txt`

### Gitea
1. 设置 → 应用 → 管理 OAuth2 应用程序 / 管理访问令牌
2. 生成 token，保存到 `local/env/gitea-token.txt`
