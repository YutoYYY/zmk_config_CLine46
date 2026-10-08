# CLine46 引き継ぎメモ

最終更新: 2026-10-08。このファイルは作業を引き継ぐ人(Claude を含む)向けです。やり取りは日本語で行います。

## 作業の進め方(必ず守る)

1. **本人が実際に使っている配列 = DYAStudio で本体に保存した配列 が正。** ファーム既定(`config/CLine46.keymap`)とは一部ずれている(下の「ファーム既定とのずれ」を参照)。
2. **`KEYMAP_REV` を上げるファームは、作る前に「保存配列とファーム既定の差」を洗い出して本人に確認する。** REV を上げると、本体に保存したキーマップが消えてファーム既定に戻る。以前この確認を怠って BT の接続先が勝手に変わり、強く叱られている。
3. 頼まれていないキーは変えない。図や説明は実機に合わせる。食い違いがあれば本人に確認する。
4. キーマップの変更は **提案 → 本人が OK → 実装 → 実機確認 → マージ** の順で進める。実機で本人が OK と言った PR はマージまで進めてよい。
5. ファームは手間の少ない方法で渡す(Actions の Artifacts など)。DL ページなどを別に作らない。

## 目的と現状

自作の分割キーボード CLine46 を改良している。ZMK、XIAO nRF52840 BLE を使い、右手がセントラルで、右手にトラックボールがある。

| 目的 | 状態 |
|---|---|
| Windows / Mac / iOS / iPadOS で、同じ指の動きで同じ操作ができる | 実装済み(Mac モード) |
| DYAStudio を Bluetooth で使う | ファームの変更なしで使えることを確認済み |
| キーの使用回数を記録し、配置を見直す | 記録用のファームが稼働中。1週間分のデータ待ちで、**それまでキーマップは変更しない** |

## リポジトリ

- 元: https://github.com/takamaru-fpv/zmk_config_CLine46
- 作業用のフォーク(公開): https://github.com/YutoYYY/zmk_config_CLine46
- ZMK 本体: cormoran/zmk の `v0.3-branch+dya`(DYAStudio 対応版。`config/west.yml` を参照)

| ファイル | 内容 |
|---|---|
| `config/CLine46.keymap` | キーマップ(ファーム既定) |
| `src/behaviors/behavior_os_mode.c` | Windows / Mac モードの切替。状態は本体に保存される |
| `src/behaviors/behavior_app_tab.c` | Ctrl の位置 + Tab で、Windows では Alt+Tab、Mac では ⌘+Tab を送る |
| `src/keymap_migrate.c` | `KEYMAP_REV`(現在 **4**)。値を上げたファームを初めて起動したときだけ、保存キーマップを消す(BT ペアリングと OS モードは残る) |
| `docs/mac-mode.md` | Windows / Mac モードの説明と初回設定 |
| `build.yaml`, `.github/workflows/build.yml` | GitHub Actions のビルド設定 |

### PR とブランチ

| PR / ブランチ | 状態 |
|---|---|
| #1 Mac モード(`claude/mac-mode`) | マージ済み。ブランチには BT 枠を入れ替えて戻しただけのコミットが2つ残っている |
| #4 長押しの左右鏡写しとウィンドウ操作(`claude/window-symmetric`) | マージ済み |
| #3 キー使用回数の記録(`claude/typing-heatmap`) | マージ済み(2026-10-08、`claude/debounce` 経由) |
| `claude/debounce` | チャタリング対策(下記)と HANDOFF.md。PR は作らず、2026-10-08 に main へ直接マージ。**今は main = 左右の実機のファーム** |
| #2 L3 の BT 枠の入れ替え(`claude/swap-monitor-keys`) | 2026-10-08 にマージせず閉じた。並びが実機と違っていた(J=BT0 / K=BT1 / L=BT2)。BT の並びは DYAStudio の保存分で運用している |
| `firmware` | ビルド済みファームの置き場 |

## キーマップの設計(確定事項)

### レイヤー

| 番号 | 内容 |
|---|---|
| L0 | 基本 |
| L1 | 記号・数字 |
| L2 | 矢印・マウス操作 |
| L3 | Bluetooth・設定 |
| L4 | メディア・F キー |
| L5 | ウィンドウ操作 |
| L6 | オートマウス |
| L7 | Mac(Mac モードのとき常に ON) |
| L8〜L13 | Mac モードのとき、L2 / L4 / L5 / L3 / L6 / L1 の上に重なる条件付きレイヤー |

L7 に置いたキーは、L1〜L6 の同じ位置のキーを隠すので注意する。

### 長押し(レイヤータップ)は左右鏡写し

| 左 | 右 | 長押し | 補足 |
|---|---|---|---|
| Space | Enter | L2 | |
| A | 右 Tab の位置 | L5 | 右のタップはかな |
| Z | / | L3 | 本体では Z の長押しを外している(下の「ファーム既定とのずれ」を参照) |
| 英数 | 右 Shift の位置 | L4 | 右のタップは英数 |
| かな | 🔍 | L1 | 🔍 と Del は入れ替え済み。🔍 のタップは Windows では PowerToys Run(Ctrl+Alt+Space)、Mac では Spotlight |

- すべてのレイヤータップに `require-prior-idle-ms=150` を設定している。直前のキーから 0.15 秒以内に押したら長押し判定をせず、すぐタップにする誤爆対策で、効果は確認済み。`tapping-term` は 200ms。

### 指の制約(本人の説明)

- 右親指は Backspace と Enter まで届く(Enter の右はトラックボール)。Backspace は連打したいので長押しにしない。
- 左親指は Ctrl / 英数 / Space / かな まで届く。
- Win と Alt は薬指・小指で押すので、長押しにはできない。

### 各レイヤー

- **L2 左手**: Q / A = 文書の先頭 / 末尾、W / R = Home / End、T / G = PgUp / PgDn、X / V = 単語単位で左 / 右、E / S / D / F = 矢印。
- **L2 右手**: 修飾キー付きクリック(Ctrl / Shift)、ダブルクリック、戻る / 進む、タブ切替。
- **L3**: W = Windows モード、A = Mac モード(状態は本体に保存)、B / N = その手の書き込みモード、H = USB / BT 切替、I = 接続ランプ、, = 電池ランプ、Backspace の位置 = Studio Unlock。
- **L4**: H = PrtSc、Y = クリップボード履歴、N = タスクマネージャー。ほかに音量・メディア操作・F1〜F12、F20 / F21 = 画面の明るさ。
- **L5(左右同じ配置)**: 左の WER / SDF / XCV / TGB と、右の UIO / JKL / M,. / YHN が同じ働きをする。
  - E / I = 最大化(Alt+Space → X のマクロで、間に 0.4 秒待つ。スナップ中でも一発で最大化できる)
  - S / F = 左右にスナップ、D = Win+↓
  - W / R = 仮想デスクトップの切替、X / V = 別のモニタへ移動
  - C = タスクビュー、T = F11(全画面)、G = デスクトップ表示、B = スナップレイアウト
  - 親指 = スタート / 縦に最大化
  - P = Win+↑(Windows 専用)
- **L6 オートマウス**: 切れるまでの時間はファームではなく、DYAStudio の Temp Layer「Deactivation Delay」= 500ms で決まる(本体に保存)。

### ファーム既定とのずれ(KEYMAP_REV を上げる前に必ず照合)

| 場所 | 本体(正) | ファーム既定(main) |
|---|---|---|
| L0 Z | 普通の Z(Google 日本語入力の z+h / l による矢印入力とぶつかるため) | `&lt 3 Z` |
| L3 BT 枠 | J = BT2、K = BT1、L = BT0(右下の L が 0) | J = BT1、K = BT0、L = BT2 |
| L6 の Deactivation Delay | 500ms | (ファーム側には無い設定) |

## Mac モード

- OS 側の修飾キーの入れ替えはしない(Mac / iPhone / iPad とも元に戻しておく)。ファーム側で Ctrl の位置を ⌘ に、Win の位置を Control にしている。
- ウィンドウ操作は Rectangle の「推奨」ショートカットが前提。音声入力は Control の 2 回押し(Mac 側で設定する)。
- アプリ一覧(L5 + M)は ⌃⌥⌘A を、クリップボード履歴は ⌃⌥⌘V を送る。どちらも Mac 側でショートカットを割り当てる。
- 🌐(fn)系の操作は送れないので対応していない。
- 詳細と初回設定は `docs/mac-mode.md` を参照。

## DYAStudio

- **Bluetooth での接続手順**: その PC とペアリング済みの BT 枠を選んでおく → L3 の Studio Unlock を押す → DYAStudio で Bluetooth 接続 → CLine46 を選ぶ。
- 使えるブラウザは Chrome と Edge だけ(iOS では Bluefy)。
- `CONFIG_ZMK_STUDIO_LOCKING` は無効にしない。
- Studio の接続口は 1 つしかない。ヒートマップ画面など別のタブが本体につながっている間は(USB でも BT でも)DYAStudio がつながらない。Disconnect では直らないので、そのタブを閉じる。

## LED

- **右手(L3 + I)**: 青 = 接続中、黄 = 選択中の枠が空(ペアリング待ち)、赤 = ペアリング済みだが相手がいない。
- **左手**: 青 = 右手と接続中、赤 = 切断。
- **電池(L3 + ,)**: 緑 = 30% 以上、黄 = 20〜30%、赤 = 20% 未満。

## ビルドと書き込み

- **推奨は GitHub Actions。** push と PR のときに自動でビルドされ、workflow_dispatch でも実行できる。フォークでも有効なことを確認済み(2026-09-30 の実行が成功)。成果物は Artifacts の zip に入った uf2。
  - 左手: `CLine46_L rgbled_adapter`
  - 右手: `CLine46_R rgbled_adapter`(`studio-rpc-usb-uart` スニペット付き)
  - `settings_reset`
- **実機に相当するのは main(左右とも)。** 記録機能とチャタリング対策も main に入っている。
- **ローカルでビルドする場合**(参考): west + Zephyr SDK 0.16.8、`setuptools<70`、`-DZMK_EXTRA_MODULES=<リポジトリ直下>`、右手は `-S studio-rpc-usb-uart`。キーマップだけ変えても反映されないことがあるので、`-p` を付けて再ビルドする。
- **社用 PC でのローカルビルド**: Docker で `zmkfirmware/zmk-build-arm:3.5` を使う(west や SDK は入れていない)。west のワークスペースは Docker ボリューム `zmk-cline46-ws` に置いてある。初回は約 5GB をダウンロードする。uf2 は `cliine46/firmware-local/` に出力する。
- **社用 PC からのプッシュ**: この PC の GitHub ログインは KobayashiYut0 で、フォークへの書き込み権限がない。リモート URL を `https://YutoYYY@github.com/...` にして、このリポジトリだけ YutoYYY でプッシュする。
- **書き込み**: L3 の B(左手)/ N(右手)で書き込みモードにし、現れたドライブに uf2 をコピーする。`KEYMAP_REV` を変えていなければ、DYAStudio で保存した配列は残る。

## キー使用回数の記録(PR #3)

- 右手のファームが、キー位置 × レイヤー × 修飾キーの組み合わせごとの回数だけを保存する。押した順序や時刻は残さない。
- 閲覧と書き出しは、`docs/typing-heatmap.html` をローカルで開き、Chrome から USB か BT で接続して行う。
- **9/30 の初回データ**(約 4100 打鍵、Windows のみ。参考程度):
  - 多い: L6 の左クリック、A の長押し(L5)、Enter の長押し(L2)
  - ほぼ使われていない: Q、右 Esc、Win、Alt、Del、L4 の大半
  - 修飾キーの組み合わせ: Shift+Enter、Ctrl+V など
- 本人が仕事で 1 週間ほど使ったデータを書き出す予定。**それまでキーマップは変更しない。**

## チャタリング対策(2026-10-08)

- **症状**: 購入当初から、両手で同じ文字が勝手に2回入る(IME で「っ」になる)。押している Shift が一瞬外れて、Shift+Enter が Enter として送信される。電源の入れ直し、電池の交換、USB 接続のどれでも直らなかった。
- **原因**: キーの読み取り方式はチャーリープレックスで、debounce が ZMK の既定値(押す・離すとも 5ms)のままだったため、接点のバタつきを打鍵として拾っていた。
- **対策**: `boards/shields/CLine46/CLine46.dtsi` の kscan0 で、`debounce-press-ms = <10>`、`debounce-release-ms = <20>` にした。キーマップと KEYMAP_REV は変えていない。テストでは約 150 文字で二重入力が 0 回(対策前は約 300 文字で 10 回ほど)。
- **Shift が強く押さないと効かなかった件**: スイッチの差し込みが甘かったためと思われる。差し直して様子見中。再発したら、Q などあまり使わないキーとスイッチを入れ替えて、スイッチ側かソケット側かを切り分ける。
- **まだ出る場合の次の手**: `CONFIG_ZMK_KSCAN_CHARLIEPLEX_WAIT_BEFORE_INPUTS`(信号が安定するまで待ってから読む設定)を試す。

## 未解決・保留の事項


- [ ] **ヒートマップの分析とキーマップの見直し**(データ待ち)。候補は、使われていない位置に頻出の操作を移すこと、Shift+Enter を 1 キーにすること。変更は「提案 → 本人が OK → 実装」の順で行う。
- [ ] **左手が反応しないことがある**: 2 時間スリープした後、電源を入れ直しても反応しないことがある。9/30 に 1 回起き、もう一度入れ直したら直った。再発待ち。再発したら、入れ直した直後の左手の LED の色と電池ランプの色を聞き、電池側かファーム側かを切り分ける。
- [ ] **不要になったブランチの削除**(本人が了承済み。この PC からは削除できないのでブラウザで行う): `claude/mac-mode`、`claude/swap-monitor-keys`、`claude/debounce`、`claude/typing-heatmap`、`claude/window-symmetric`。`firmware` は残す。
- [ ] **ファーム既定と実機のずれ**(L3 の BT の並び、Z の長押し)を既定にも反映するかどうかは、次に `KEYMAP_REV` を上げるときに本人に確認する。本人の配列図(https://claude.ai/artifact/9bcjPfto5HWd2BP4fUe6Jc)は実機と同じ並び(J=BT2 / K=BT1 / L=BT0、U=BT3 / O=BT4)。
- [ ] **ドキュメントのずれ**: `docs/mac-mode.md` の L5 の表が #4 より前の配置のまま(Q = タスクマネージャー、W = なし、X = 範囲スクショ など)。PrtSc が L2 の節に書かれているが、実際は L4 の H。keymap 冒頭のコメントと mac-mode.md の「レイヤー 0〜12」に L13(Mac L1)が抜けている。

## 解決済み(参考)

- 社用 PC の VSCode で全画面(F11)が効かなかったのは、ショートカットが登録されていなかったため。
- Claude Code 拡張でタブを移動するとモードが切り替わる件は、拡張側の不具合だった。
