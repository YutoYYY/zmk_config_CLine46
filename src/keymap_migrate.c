/*
 * キーマップ更新時の一回限りのリセット
 *
 * DYAStudio で保存したキーはファームの配列より優先されるので、ファーム側で配列を
 * 変えても反映されないことがある。KEYMAP_REV を上げたファームを初めて起動したときだけ、
 * DYAStudio で保存したキーマップを消してファームの配列に戻す(Bluetooth のペアリングや
 * Windows/Mac モードなど、キーマップ以外の設定は消さない)。
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_SETTINGS) && IS_ENABLED(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE)

// ファームの配列を必ず反映させたいときに上げる
#define KEYMAP_REV 1

static uint8_t saved_rev = 0;

static void migrate_work_cb(struct k_work *work) {
    if (saved_rev == KEYMAP_REV) {
        return;
    }

    LOG_INF("Keymap rev %d -> %d: restoring firmware keymap", saved_rev, KEYMAP_REV);

    int ret = zmk_keymap_reset_settings();
    if (ret < 0) {
        LOG_ERR("Failed to reset keymap settings (%d)", ret);
        return;
    }

    saved_rev = KEYMAP_REV;
    ret = settings_save_one("cline46/keymap_rev", &saved_rev, sizeof(saved_rev));
    if (ret < 0) {
        LOG_ERR("Failed to save keymap rev (%d)", ret);
    }
}

static K_WORK_DELAYABLE_DEFINE(migrate_work, migrate_work_cb);

static int cline46_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    const char *next;

    if (settings_name_steq(name, "keymap_rev", &next) && !next) {
        if (len != sizeof(saved_rev)) {
            return -EINVAL;
        }

        int ret = read_cb(cb_arg, &saved_rev, sizeof(saved_rev));
        return MIN(ret, 0);
    }

    return -ENOENT;
}

static int cline46_settings_commit(void) {
    // キーマップ側の読み込みが全部終わってから実行する
    k_work_reschedule(&migrate_work, K_SECONDS(1));
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(cline46, "cline46", NULL, cline46_settings_set,
                               cline46_settings_commit, NULL);

#endif
