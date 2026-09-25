/*
 * アプリ切替用の Tab キー(Windows / Mac 共通)
 *
 * Mac の Cmd+Tab と同じ指(Space 寄りの Ctrl の位置)でアプリを切り替えられるようにする。
 *   Windows モード: Ctrl を押したまま Tab → Alt+Tab
 *   Mac モード:     Alt を押したまま Tab → Cmd+Tab(Ctrl の位置は Mac モードでは Cmd なので元から Cmd+Tab)
 * 押している修飾キーを離すまで切替先の修飾キーを押しっぱなしにするので、Tab を続けて押すと
 * 次々にアプリを選べる。Shift も押せば逆順。それ以外のときは普通の Tab。
 */

#define DT_DRV_COMPAT cline46_behavior_app_tab

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>

#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// behavior_os_mode.c
bool cline46_os_mode_is_mac(void);

#define ALT_MODS (MOD_LALT | MOD_RALT)
#define CTRL_MODS (MOD_LCTL | MOD_RCTL)

// 切替の途中なら、押しっぱなしにしている修飾キーと、それを終わらせる修飾キー
static bool switching = false;
static uint32_t held_key;
static zmk_mod_flags_t trigger_mods;

static int on_app_tab_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    if (!switching) {
        bool mac = cline46_os_mode_is_mac();
        zmk_mod_flags_t mods = mac ? ALT_MODS : CTRL_MODS;

        if (zmk_hid_get_explicit_mods() & mods) {
            switching = true;
            trigger_mods = mods;
            held_key = mac ? LGUI : LALT;
            // 元の修飾キーが OS に見えると別の操作になるので、離すまで隠す
            zmk_hid_masked_modifiers_set(mods);
            raise_zmk_keycode_state_changed_from_encoded(held_key, true, event.timestamp);
        }
    }

    return raise_zmk_keycode_state_changed_from_encoded(TAB, true, event.timestamp);
}

static int on_app_tab_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    return raise_zmk_keycode_state_changed_from_encoded(TAB, false, event.timestamp);
}

static bool is_trigger_key(uint32_t keycode) {
    if (trigger_mods == ALT_MODS) {
        return keycode == HID_USAGE_KEY_KEYBOARD_LEFTALT ||
               keycode == HID_USAGE_KEY_KEYBOARD_RIGHTALT;
    }
    return keycode == HID_USAGE_KEY_KEYBOARD_LEFTCONTROL ||
           keycode == HID_USAGE_KEY_KEYBOARD_RIGHTCONTROL;
}

static int app_tab_keycode_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev == NULL || !switching || ev->state || ev->usage_page != HID_USAGE_KEY) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (is_trigger_key(ev->keycode)) {
        switching = false;
        raise_zmk_keycode_state_changed_from_encoded(held_key, false, ev->timestamp);
        zmk_hid_masked_modifiers_clear();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(behavior_app_tab, app_tab_keycode_listener);
ZMK_SUBSCRIPTION(behavior_app_tab, zmk_keycode_state_changed);

static const struct behavior_driver_api behavior_app_tab_driver_api = {
    .binding_pressed = on_app_tab_binding_pressed,
    .binding_released = on_app_tab_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define APP_TAB_INST(n)                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_app_tab_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APP_TAB_INST)

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
