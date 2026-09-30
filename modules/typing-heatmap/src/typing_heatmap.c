/* SPDX-License-Identifier: MIT */
/*
 * Based on cormoran/zmk-feature-typing-heatmap (MIT). CLine46 changes: counts are also kept by
 * the layer that handled each press and by the modifiers held at that time, and the counters are
 * persisted in chunks because the table no longer fits a single settings record.
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/workqueue.h>
#include <cormoran/feature-typing-heatmap/typing_heatmap.h>
#include "typing_heatmap_state.h"

#if IS_ENABLED(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define POSITIONS ZMK_KEYMAP_LEN
#define LAYERS ZMK_KEYMAP_LAYERS_LEN
#define CELLS (POSITIONS * (1 + LAYERS + HEATMAP_MOD_COMBOS))
#define CHUNKS ((CELLS + HEATMAP_CHUNK_CELLS - 1) / HEATMAP_CHUNK_CELLS)
BUILD_ASSERT(CHUNKS <= 32, "Too many typing heatmap chunks");

static uint32_t counters[CELLS];
static struct heatmap_state state;
static struct k_spinlock state_lock;
K_MUTEX_DEFINE(storage_lock);
static bool ready;
/* The flash write buffer is not on the workqueue's small stack. */
static uint8_t chunk_blob[HEATMAP_CHUNK_SIZE];
static uint8_t header_blob[HEATMAP_HEADER_SIZE];
#if IS_ENABLED(CONFIG_SETTINGS)
static uint8_t loaded_header[HEATMAP_HEADER_SIZE];
static bool header_loaded;
static uint8_t loaded_chunks[4 * CELLS];
static size_t loaded_chunk_len[CHUNKS];
static uint32_t loaded_chunk_mask;
#endif
static struct k_work_delayable checkpoint_work;

static uint64_t now_ms(void) { return (uint64_t)k_uptime_get(); }

#if IS_ENABLED(CONFIG_SETTINGS)
static void chunk_key(char *buf, size_t size, uint32_t chunk) {
    snprintf(buf, size, "typing_heatmap2/c%u", (unsigned int)chunk);
}
#endif

static int store_header(bool persistent) {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    size_t len = heatmap_encode_header(&state, header_blob, persistent);
    k_spin_unlock(&state_lock, key);
    return settings_save_one("typing_heatmap2/state", header_blob, len);
#else
    ARG_UNUSED(persistent);
    return -ENOTSUP;
#endif
}

/* Writes every counter chunk (with_counts) or removes them. Caller holds storage_lock. */
static int store_chunks(bool with_counts) {
#if IS_ENABLED(CONFIG_SETTINGS)
    char name[32];
    for (uint32_t c = 0; c < CHUNKS; c++) {
        chunk_key(name, sizeof(name), c);
        int rc;
        if (with_counts) {
            k_spinlock_key_t key = k_spin_lock(&state_lock);
            size_t len = heatmap_encode_chunk(&state, c, chunk_blob);
            k_spin_unlock(&state_lock, key);
            rc = settings_save_one(name, chunk_blob, len);
        } else {
            rc = settings_delete(name);
        }
        if (rc) {
            return rc;
        }
    }
    return 0;
#else
    ARG_UNUSED(with_counts);
    return -ENOTSUP;
#endif
}

int typing_heatmap_get_stats(uint32_t offset, struct typing_heatmap_stats *stats) {
    if (!stats) {
        return -EINVAL;
    }
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    int rc = heatmap_page(&state, offset, stats->counts, &stats->count);
    if (!rc) {
        stats->offset = offset;
        stats->position_count = state.cell_count;
        stats->key_positions = state.positions;
        stats->layer_count = state.layers;
        stats->mod_combo_count = HEATMAP_MOD_COMBOS;
        stats->persistence_enabled = state.persistent;
        stats->persistence_supported = IS_ENABLED(CONFIG_SETTINGS);
        stats->unsaved_presses = MIN(state.dirty, UINT32_MAX);
        stats->save_interval_seconds = CONFIG_ZMK_FEATURE_TYPING_HEATMAP_SAVE_INTERVAL_SECONDS;
        stats->min_presses = CONFIG_ZMK_FEATURE_TYPING_HEATMAP_MIN_PRESSES;
        stats->storage_error = state.storage_error;
        stats->generation = state.generation;
    }
    k_spin_unlock(&state_lock, key);
    return rc;
}

static bool is_ready(void) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    bool r = ready;
    k_spin_unlock(&state_lock, key);
    return r;
}

int typing_heatmap_reset(void) {
    k_mutex_lock(&storage_lock, K_FOREVER);
    if (!is_ready()) {
        k_mutex_unlock(&storage_lock);
        return -EAGAIN;
    }
    int rc = 0;
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        /* Reset the persisted data even when it is below automatic thresholds. */
        rc = store_chunks(false);
    }
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    heatmap_reset_done(&state, now_ms(), rc);
    k_spin_unlock(&state_lock, key);
    k_mutex_unlock(&storage_lock);
    return rc;
}

int typing_heatmap_set_persistence(bool enabled) {
    if (enabled && !IS_ENABLED(CONFIG_SETTINGS)) {
        return -ENOTSUP;
    }
    k_mutex_lock(&storage_lock, K_FOREVER);
    if (!is_ready()) {
        k_mutex_unlock(&storage_lock);
        return -EAGAIN;
    }
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    bool same = state.persistent == enabled;
    k_spin_unlock(&state_lock, key);
    if (same) {
        k_mutex_unlock(&storage_lock);
        return 0;
    }
    /* The header carries the mode; RAM mode also drops previously saved counts. */
    int rc = store_header(enabled);
    if (!rc && !enabled) {
        rc = store_chunks(false);
    }
    key = k_spin_lock(&state_lock);
    heatmap_mode_done(&state, enabled, now_ms(), rc);
    k_spin_unlock(&state_lock, key);
    k_mutex_unlock(&storage_lock);
    return rc;
}

static void checkpoint(struct k_work *work) {
    ARG_UNUSED(work);
    k_mutex_lock(&storage_lock, K_FOREVER);
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    bool due = ready && heatmap_due(&state, now_ms(),
                                    CONFIG_ZMK_FEATURE_TYPING_HEATMAP_SAVE_INTERVAL_SECONDS,
                                    CONFIG_ZMK_FEATURE_TYPING_HEATMAP_MIN_PRESSES);
    uint64_t dirty_snapshot = state.dirty;
    k_spin_unlock(&state_lock, key);
    if (due) {
        int rc = store_header(true);
        if (!rc) {
            rc = store_chunks(true);
        }
        key = k_spin_lock(&state_lock);
        heatmap_saved(&state, dirty_snapshot, now_ms(), rc);
        k_spin_unlock(&state_lock, key);
        if (rc) {
            LOG_WRN("Typing statistics save failed: %d", rc);
        }
    }
    k_mutex_unlock(&storage_lock);
    k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &checkpoint_work, K_SECONDS(60));
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int settings_set(const char *name, size_t length, settings_read_cb read_cb, void *cb_arg) {
    /* This owner applies only its initial boot load. */
    if (is_ready()) {
        return 0;
    }
    uint8_t *dest;
    size_t expected;
    int chunk = -1;
    if (!strcmp(name, "state")) {
        dest = loaded_header;
        expected = HEATMAP_HEADER_SIZE;
    } else if (name[0] == 'c') {
        char *end;
        unsigned long c = strtoul(name + 1, &end, 10);
        if (*end || c >= CHUNKS) {
            return -ENOENT;
        }
        chunk = c;
        dest = loaded_chunks + chunk * HEATMAP_CHUNK_SIZE;
        expected = MIN(HEATMAP_CHUNK_SIZE, sizeof(loaded_chunks) - chunk * HEATMAP_CHUNK_SIZE);
    } else {
        return -ENOENT;
    }
    if (length != expected) {
        return -EINVAL;
    }
    int bytes = read_cb(cb_arg, dest, length);
    if (bytes < 0 || (size_t)bytes != length) {
        return bytes < 0 ? bytes : -EIO;
    }
    if (chunk < 0) {
        header_loaded = true;
    } else {
        loaded_chunk_len[chunk] = length;
        loaded_chunk_mask |= BIT(chunk);
    }
    return 0;
}
static int heatmap_settings_commit(void) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    if (!ready) {
        if (header_loaded) {
            int rc = heatmap_restore_header(&state, loaded_header, HEATMAP_HEADER_SIZE);
            /* Missing chunks mean zero counts (they are removed on reset and in RAM mode). */
            for (uint32_t c = 0; !rc && state.persistent && c < CHUNKS; c++) {
                if (loaded_chunk_mask & BIT(c)) {
                    rc = heatmap_restore_chunk(&state, c, loaded_chunks + c * HEATMAP_CHUNK_SIZE,
                                               loaded_chunk_len[c]);
                }
            }
            state.storage_error = rc;
        }
        state.last_attempt_ms = now_ms();
        ready = true;
    }
    k_spin_unlock(&state_lock, key);
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(typing_heatmap2, "typing_heatmap2", NULL, settings_set,
                               heatmap_settings_commit, NULL);

/* Drop the single-record format written by the upstream module, if it was ever flashed. */
static bool legacy_present;
static int legacy_set(const char *name, size_t length, settings_read_cb read_cb, void *cb_arg) {
    ARG_UNUSED(length);
    ARG_UNUSED(read_cb);
    ARG_UNUSED(cb_arg);
    if (!strcmp(name, "state")) {
        legacy_present = true;
    }
    return 0;
}
static struct k_work legacy_work;
static void legacy_cleanup(struct k_work *work) {
    ARG_UNUSED(work);
    settings_delete("typing_heatmap/state");
}
static int legacy_commit(void) {
    if (legacy_present) {
        legacy_present = false;
        k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &legacy_work);
    }
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(typing_heatmap_legacy, "typing_heatmap", NULL, legacy_set,
                               legacy_commit, NULL);
#endif

/* Every physical press, seen before combos and hold-taps can capture it. */
static int position_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);
    if (event) {
        k_spinlock_key_t key = k_spin_lock(&state_lock);
        heatmap_press(&state, event->position, event->state);
        k_spin_unlock(&state_lock, key);
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(typing_heatmap, position_listener);
ZMK_SUBSCRIPTION(typing_heatmap, zmk_position_state_changed);

/*
 * The keymap resolves a press by invoking the binding stored for (layer, position), falling
 * through transparent bindings. Wrapping the invoke call (-Wl,--wrap) observes exactly that
 * resolution after hold-taps have decided, with the layer state the keymap used. Bindings invoked
 * by other behaviors (hold-tap halves, macros, combos) are not keymap slots and are ignored.
 */
int __real_zmk_behavior_invoke_binding(const struct zmk_behavior_binding *src_binding,
                                       struct zmk_behavior_binding_event event, bool pressed);

static int layer_index_of(zmk_keymap_layer_id_t id) {
    for (int i = 0; i < LAYERS; i++) {
        if (zmk_keymap_layer_index_to_id(i) == id) {
            return i;
        }
    }
    return -1;
}

int __wrap_zmk_behavior_invoke_binding(const struct zmk_behavior_binding *src_binding,
                                       struct zmk_behavior_binding_event event, bool pressed) {
    bool keymap_slot = pressed && event.layer >= 0 && event.position < POSITIONS &&
                       src_binding ==
                           zmk_keymap_get_layer_binding_at_idx(event.layer, event.position);
    uint8_t mods = 0;
    if (keymap_slot) {
        zmk_mod_flags_t explicit_mods = zmk_hid_get_explicit_mods();
        mods = (explicit_mods | (explicit_mods >> 4)) & 0x0F;
    }
    int ret = __real_zmk_behavior_invoke_binding(src_binding, event, pressed);
    if (keymap_slot && ret == ZMK_BEHAVIOR_OPAQUE) {
        int index = layer_index_of(event.layer);
        k_spinlock_key_t key = k_spin_lock(&state_lock);
        heatmap_resolved(&state, event.position, index < 0 ? UINT32_MAX : (uint32_t)index, mods);
        k_spin_unlock(&state_lock, key);
    }
    return ret;
}

static int init(void) {
    heatmap_init(&state, counters, POSITIONS, LAYERS, IS_ENABLED(CONFIG_SETTINGS));
    ready = !IS_ENABLED(CONFIG_SETTINGS);
#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init(&legacy_work, legacy_cleanup);
#endif
    k_work_init_delayable(&checkpoint_work, checkpoint);
    k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &checkpoint_work, K_SECONDS(60));
    return 0;
}
SYS_INIT(init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
