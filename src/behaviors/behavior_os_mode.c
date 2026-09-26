/*
 * Windows/Mac モード切替
 *
 * 押すと Mac モード用レイヤーを有効/無効にし、状態を設定領域に保存する。
 * 起動時(スリープ復帰を含む)に保存済みの状態を読み戻すので、モードは電源を切っても消えない。
 */

#define DT_DRV_COMPAT cline46_behavior_os_mode

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// すべてのインスタンスで同じレイヤーを指す前提
#define MAC_LAYER DT_INST_PROP(0, layer)

struct behavior_os_mode_config {
    bool mac_mode;
};

static uint8_t mac_mode = 0;

bool cline46_os_mode_is_mac(void) { return mac_mode; }

static void apply_mode(void) {
    if (mac_mode) {
        zmk_keymap_layer_activate(MAC_LAYER);
    } else {
        zmk_keymap_layer_deactivate(MAC_LAYER);
    }
}

#if IS_ENABLED(CONFIG_SETTINGS)

static void save_mode_work_cb(struct k_work *work) {
    int ret = settings_save_one("os_mode/mac", &mac_mode, sizeof(mac_mode));
    if (ret < 0) {
        LOG_ERR("Failed to save OS mode (%d)", ret);
    }
}

static K_WORK_DELAYABLE_DEFINE(save_mode_work, save_mode_work_cb);

static int os_mode_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    const char *next;

    if (settings_name_steq(name, "mac", &next) && !next) {
        if (len != sizeof(mac_mode)) {
            return -EINVAL;
        }

        int ret = read_cb(cb_arg, &mac_mode, sizeof(mac_mode));
        return MIN(ret, 0);
    }

    return -ENOENT;
}

static int os_mode_settings_commit(void) {
    apply_mode();
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(os_mode, "os_mode", NULL, os_mode_settings_set,
                               os_mode_settings_commit, NULL);

#endif // IS_ENABLED(CONFIG_SETTINGS)

static int on_os_mode_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_os_mode_config *cfg = dev->config;

    mac_mode = cfg->mac_mode ? 1 : 0;
    apply_mode();

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_reschedule(&save_mode_work, K_SECONDS(2));
#endif

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_os_mode_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_os_mode_driver_api = {
    .binding_pressed = on_os_mode_binding_pressed,
    .binding_released = on_os_mode_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define OS_MODE_INST(n)                                                                            \
    BUILD_ASSERT(DT_INST_PROP(n, layer) == MAC_LAYER,                                              \
                 "All os-mode instances must use the same layer");                                 \
    static const struct behavior_os_mode_config behavior_os_mode_config_##n = {                    \
        .mac_mode = DT_INST_PROP(n, mac_mode),                                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_os_mode_config_##n, POST_KERNEL,        \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_os_mode_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OS_MODE_INST)

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
