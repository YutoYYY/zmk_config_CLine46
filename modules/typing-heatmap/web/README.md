# キー使用回数の表示画面

右手側(セントラル)に記録されたキー使用回数を、USB(Web Serial)または Bluetooth(Web Bluetooth)で読み出して表示する画面です。Chrome / Edge で `docs/typing-heatmap.html` を開いて使います(ファイルをダブルクリックで開けば動きます)。

## 作り直し方

```sh
NPM_CONFIG_ALLOW_GIT=all npm ci
npm run layout        # キーマップ(config/CLine46.keymap)を変えたら、刻印の表示を作り直す
npm run build:single  # docs/typing-heatmap.html を作り直す
```

刻印はリポジトリのキーマップから作るので、DYAStudio で本体だけ変えたキーは表示と一致しないことがあります(回数はキー位置で数えているので正しいままです)。
