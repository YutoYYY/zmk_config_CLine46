# CLine46 独自ビヘイビア(Windows/Macモード切替、Mac用Alt+Tab)
# キーマップを処理するのはセントラル(右手側)だけなので、そこでのみビルドする
if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE src/behaviors/behavior_os_mode.c)
  target_sources(app PRIVATE src/behaviors/behavior_app_tab.c)
endif()
