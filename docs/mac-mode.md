# Windows / Mac モード

同じキーマップ・同じ指の動きで、Windows と Mac(iPhone / iPad も)を使い分けるための仕組みです。
OS 側の修飾キー入れ替えは不要で、キーボードだけで完結します。

## 切り替え方

| 操作 | 結果 |
|---|---|
| Layer 3(`/?` キー長押し)+ **W** の位置 | Windows モード |
| Layer 3(`/?` キー長押し)+ **A** の位置 | Mac モード(Apple) |

選んだモードは本体に保存され、スリープや電源オフのあとも残ります。

## Mac モードで変わること

### 常に変わるもの(Layer 7「Mac」)

| Windows での操作(指の動き) | Mac で送られるもの |
|---|---|
| Ctrl の位置のキー | ⌘ Command |
| Win の位置のキー | ^ Control |
| Ctrl + C / V / Z / S / F … | ⌘ + C / V / Z / S / F … |
| Ctrl + Backspace / Delete(単語削除) | ⌥ + Delete / 前方削除 |
| Alt + Tab(押したまま Tab を連打) | ⌘ + Tab(アプリ切替画面を開いたまま選べる) |

### Layer 2 を押している間(Layer 8「Mac L2」)

| Windows | Mac |
|---|---|
| ← → ↑ ↓ | そのまま |
| Ctrl + ← → ↑ ↓(単語・段落移動) | ⌥ + ← → ↑ ↓ |
| Home / End(行頭・行末) | ⌘ + ← / → |
| Ctrl + Home / End(文書の先頭・末尾) | ⌘ + ↑ / ↓ |
| 戻る / 進む(マウスボタン 4 / 5) | ⌘ + [ / ⌘ + ](Safari でも効く。Layer 6 も同じ) |
| PrtSc | ⌘ + Shift + 5(スクリーンショットツール) |
| クリック用 Ctrl / ⇧ / Alt(1回押し、5秒以内にクリック) | ⌘ / ⇧ / ⌥ |
| Ctrl+Tab / Ctrl+Shift+Tab(タブ切替) | そのまま(Mac でもタブ切替) |

### Layer 4 を押している間(Layer 9「Mac L4」)

| Windows | Mac |
|---|---|
| Win + L(画面ロック) | ⌘ + Control + Q |
| Win + H(音声入力) | Control を2回(※設定が必要) |
| F21 / F20(画面の明るさ) | 明るさ − / + |
| Alt + F4 | ⌘ + Q |

### Layer 5 を押している間(Layer 10「Mac L5」)

ウィンドウ操作は [Rectangle](https://rectangleapp.com/) の「推奨」ショートカットを前提にしています。

| 位置 | Windows | Mac |
|---|---|---|
| E | Win + ↑(最大化) | Rectangle: 最大化 |
| S / F | Win + ← / →(左右にスナップ) | Rectangle: 左半分 / 右半分 |
| D | Win + ↓ | Rectangle: 元のサイズに戻す |
| Q | Ctrl + Shift + Esc(タスクマネージャー) | 強制終了(⌘ + Option + Esc) |
| W | なし | 同じアプリの次のウィンドウ(⌘ + `) |
| Z | F11(全画面) | フルスクリーン(Control + ⌘ + F) |
| C | Win + Z(スナップレイアウト) | アプリExposé(Control + ↓) |
| G | Win + D(デスクトップ表示) | F11(デスクトップを表示) |
| X | Win + Shift + S(範囲スクショ → クリップボード) | ⌘ + Control + Shift + 4 |
| V | Win + V(クリップボード履歴) | クリップボード履歴(⌃⌥⌘V を送るので、下の初回設定でアプリに割り当てる) |
| U / O | Ctrl + Win + ← / →(仮想デスクトップ切替) | Control + ← / →(操作スペース切替) |
| I | Shift + Win + ↑(縦に最大化) | Rectangle: 高さを最大化 |
| K | Shift + Win + ↓ | Rectangle: 元のサイズに戻す |
| J / L | Shift + Win + ← / →(別モニタへ移動) | Rectangle: 前 / 次のディスプレイ |
| M | Win(スタートメニュー) | アプリ一覧(4本指ピンチと同じ。⌃⌥⌘A を送るので、下の初回設定で割り当てる) |
| , | Win + Tab(タスクビュー) | Mission Control(4本指スワイプアップと同じ。Control + ↑) |

## アプリ切替は Windows / Mac とも同じ指で

Space 寄りの親指キー(Ctrl の位置)を押したまま Tab でアプリを切り替えます。Mac の ⌘ + Tab と同じ指の動きです。

- Windows: Ctrl の位置 + Tab → Alt + Tab(Ctrl を離すまで切替画面が開いたまま。Shift も押すと逆順)
- Mac: Ctrl の位置は ⌘ なのでそのまま ⌘ + Tab
- Alt + Tab はどちらのモードでも今までどおり使えます
- Windows でブラウザのタブを切り替える Ctrl + Tab は、Layer 2(Enter 長押し)の「前のタブ / 次のタブ」を使います

## Mac 特有の機能の使い方(Mac モード時)

| やりたいこと | 押し方 |
|---|---|
| アプリ切替 | Ctrl の位置(⌘)を押したまま Tab。Alt + Tab でも可(離すまで一覧が出たまま) |
| 同じアプリの別ウィンドウへ | かな(L5)+ W |
| アプリExposé(今のアプリのウィンドウ一覧) | かな(L5)+ C |
| Mission Control(4本指スワイプアップ) | かな(L5)+ , |
| 操作スペース(デスクトップ)切替 | かな(L5)+ U / O |
| デスクトップを表示 | かな(L5)+ G |
| フルスクリーン | かな(L5)+ Z |
| 強制終了 | かな(L5)+ Q |
| Spotlight | 🔍 キー |
| アプリ一覧(4本指ピンチ) | かな(L5)+ M |
| 隠す / 最小化 / 終了 / ウィンドウを閉じる | Ctrl の位置 + H / M / Q / W |
| 設定を開く | Ctrl の位置 + ,(アプリ内) |
| 画面ロック | 英数(L4)+ Q |

地球儀(🌐 / fn)キーの操作(通知センター 🌐N、コントロールセンター 🌐C など)は、キーボードから🌐を送れないため対応できません。マウスでメニューバー右上から開いてください。

## 2026-09-25 の見直しで変わったキー

| 場所 | 変更 |
|---|---|
| 右親指(トラックボールの右) | Caps → 🔍(Windows 検索 / Mac Spotlight)。Caps は 英数 + A |
| Space(記号) | 左端の列に $ \| ~、右端の列に ^ - /(= + * と並べて電卓風) |
| Enter(矢印) | A / G = 単語単位で ← / →、Z / B = 文書の先頭 / 末尾(Mac は ⌥← / ⌥→、⌘↑ / ⌘↓) |
| / (Bluetooth) | Tab 位置の全消去を削除。H = USB と Bluetooth の切替、I = 接続状態ランプ、, = 電池残量ランプ、左下 / 右下 = その手の書き込みモード(リセット2回押しの代わり) |
| 英数(メディア) | Vol− の下 = ミュート、C = アプリ終了(Windows Alt+F4 / Mac ⌘Q) |
| 絵文字 | かな + T から Space + 🔍 に移動(Windows Win + . / Mac ⌃⌘Space) |

## 初回だけ必要な設定

### Mac

1. **修飾キーの入れ替えを戻す**: システム設定 > キーボード > キーボードショートカット > 修飾キー で CLine46 を選び、Control / Command を元(Control→Control、Command→Command)に戻す。
2. **Rectangle を入れる**: 初回起動時に「推奨(Recommended)」ショートカットを選ぶ。
3. **音声入力のショートカット**: システム設定 > キーボード > 音声入力 > ショートカット を「Control キーを2回押す」にする。
4. **アプリ一覧(かな + M)**: システム設定 > キーボード > キーボードショートカット > Spotlight > 「アプリを表示」をオンにし、ショートカット欄をクリックして かな + M を押す(⌃⌥⌘A が登録される)。項目が無い macOS では「Launchpad と Dock」>「Launchpad を表示」に同じように登録する。
5. (任意)**クリップボード履歴(かな + V)**: macOS 15 以前には標準のクリップボード履歴が無いので、[Maccy](https://maccy.app/) などのアプリを入れ、呼び出しショートカットの欄で かな + V を押す(⌃⌥⌘V が登録される)。

### iPhone / iPad

修飾キーの入れ替えはしない(していたら「設定 > 一般 > キーボード > ハードウェアキーボード > 修飾キー」で元に戻す)。Mac モードでそのまま使えます。

## DYAStudio との関係

- Layer 0〜12 すべて、これまでどおり DYAStudio で中身を変更できます。
- Mac 用レイヤー(7〜12)は、▽(透過)にしたキーは下の Windows 用レイヤーのキーがそのまま使われます。Windows 側に新しいショートカットを足したら、必要に応じて Mac 用レイヤーの同じ位置にも Mac 版を置いてください。
- 「Windows Mode」「Mac Mode」「App Tab」「Mac Left」などの独自キーも DYAStudio のキー一覧から選べます。
- Layer 11「Mac L3」は、Mac モード本体が使っている位置(Backspace の位置)を Layer 3 のキー(Studio Unlock)に戻すためのものです。Layer 3 のその位置を変えたら、Layer 11 も合わせて変えてください。
