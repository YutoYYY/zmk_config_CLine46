# typing-heatmap(CLine46 用に改造)

[cormoran/zmk-feature-typing-heatmap](https://github.com/cormoran/zmk-feature-typing-heatmap)(MIT)を元に、CLine46 のキーマップ改善に使えるよう改造したものです。

記録する内容(すべて押した回数の合計だけで、順番や時刻は残しません):

- 物理キーごとの回数(コンボの構成キーも含む)
- 実際にキーを処理したレイヤー × キー位置
- そのとき押していた修飾キー(Ctrl / Shift / Alt / Win・⌘ の組み合わせ15通り)× キー位置

元のモジュールからの主な変更:

- レイヤーと修飾キーは、キーマップがキーを処理する瞬間(`zmk_behavior_invoke_binding` を `--wrap`)で数える。ホールドタップの判定後なので、`&lt` で切り替えたレイヤーのキーも正しく数えられる。キーマップの枠以外(ホールドタップの中身、マクロ、コンボ)から呼ばれたものは数えない。
- 表が大きくなったので、保存は 1KB ごとの複数レコード(`typing_heatmap2/state`, `typing_heatmap2/c0` 以降)に分けた。元モジュールの `typing_heatmap/state` があれば起動時に消す。
- RPC の識別子を `cline46_typing_heatmap` に変更し、1回で32セル読むようにした(`CONFIG_ZMK_STUDIO_RPC_TX_BUF_SIZE=512` が必要)。
- 表示画面(`web/`)を CLine46 の配置・レイヤー切り替え・修飾キー別の表示に変更。

保存は元と同じく「前回から30分以上」かつ「100回以上押した」ときだけ行うので、それまでの分は再起動で消えます。
