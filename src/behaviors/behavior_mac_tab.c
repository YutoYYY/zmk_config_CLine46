/*
 * Mac モード用の Tab キー
 *
 * Alt(Option)を押したまま Tab を押すと、Windows の Alt+Tab と同じ指の動きで
 * Mac の Cmd+Tab(アプリ切替)になる。Alt を離すまで Cmd を押しっぱなしにするので、
 * Tab を続けて押すと次々にアプリを選べる。Alt を押していないときは普通の Tab。
 */

#define DT_DRV_COMPAT cline46_behavior_mac_tab

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

#define ALT_MODS (MOD_LALT | MOD_RALT)

// Alt+Tab の途中(Cmd を押しっぱなしにしている間)なら true
static bool switching = false;

static int on_mac_tab_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    if (!switching && (zmk_hid_get_explicit_mods() & ALT_MODS)) {
        switching = true;
        // Option が Mac に見えると切替先の挙動が変わるので、Alt を離すまで隠す
        zmk_hid_masked_modifiers_set(ALT_MODS);
        raise_zmk_keycode_state_changed_from_encoded(LGUI, true, event.timestamp);
    }

    return raise_zmk_keycode_state_changed_from_encoded(TAB, true, event.timestamp);
}

static int on_mac_tab_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    return raise_zmk_keycode_state_changed_from_encoded(TAB, false, event.timestamp);
}

static int mac_tab_keycode_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev == NULL || !switching || ev->state || ev->usage_page != HID_USAGE_KEY) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->keycode == HID_USAGE_KEY_KEYBOARD_LEFTALT ||
        ev->keycode == HID_USAGE_KEY_KEYBOARD_RIGHTALT) {
        switching = false;
        raise_zmk_keycode_state_changed_from_encoded(LGUI, false, ev->timestamp);
        zmk_hid_masked_modifiers_clear();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(behavior_mac_tab, mac_tab_keycode_listener);
ZMK_SUBSCRIPTION(behavior_mac_tab, zmk_keycode_state_changed);

static const struct behavior_driver_api behavior_mac_tab_driver_api = {
    .binding_pressed = on_mac_tab_binding_pressed,
    .binding_released = on_mac_tab_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define MAC_TAB_INST(n)                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_mac_tab_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAC_TAB_INST)

#endif // DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
