/* SPDX-License-Identifier: MIT */
#include "typing_heatmap_state.h"
#include <errno.h>
#include <limits.h>
#include <string.h>

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void write_u32(uint8_t *p, uint32_t value) {
    for (unsigned int i = 0; i < 4; i++) {
        p[i] = value >> (8 * i);
    }
}
static bool bump(struct heatmap_state *s, uint32_t cell) {
    if (cell >= s->cell_count || s->counts[cell] == UINT32_MAX) {
        return false;
    }
    s->counts[cell]++;
    s->generation++;
    return true;
}
static uint32_t chunk_cells(const struct heatmap_state *s, uint32_t chunk) {
    uint32_t start = chunk * HEATMAP_CHUNK_CELLS;
    uint32_t rest = s->cell_count - start;
    return rest < HEATMAP_CHUNK_CELLS ? rest : HEATMAP_CHUNK_CELLS;
}

void heatmap_init(struct heatmap_state *s, uint32_t *counts, uint32_t positions, uint32_t layers,
                  bool persistent) {
    *s = (struct heatmap_state){.counts = counts,
                                .positions = positions,
                                .layers = layers,
                                .cell_count = heatmap_cells(positions, layers),
                                .persistent = persistent};
    memset(counts, 0, sizeof(*counts) * s->cell_count);
}
void heatmap_press(struct heatmap_state *s, uint32_t position, bool pressed) {
    if (!pressed || position >= s->positions || !bump(s, position)) {
        return;
    }
    if (s->persistent && s->dirty != UINT64_MAX) {
        s->dirty++;
    }
}
void heatmap_resolved(struct heatmap_state *s, uint32_t position, uint32_t layer, uint8_t mods) {
    if (position >= s->positions) {
        return;
    }
    if (layer < s->layers) {
        bump(s, s->positions * (1U + layer) + position);
    }
    mods &= 0x0F;
    if (mods) {
        bump(s, s->positions * (1U + s->layers + (mods - 1U)) + position);
    }
}
bool heatmap_due(const struct heatmap_state *s, uint64_t now_ms, uint32_t interval_seconds,
                 uint32_t min_presses) {
    return s->persistent && s->dirty >= min_presses &&
           now_ms - s->last_attempt_ms >= (uint64_t)interval_seconds * 1000;
}
size_t heatmap_encode_header(const struct heatmap_state *s, uint8_t *blob, bool persistent) {
    write_u32(blob, HEATMAP_FORMAT_VERSION);
    write_u32(blob + 4, s->positions);
    write_u32(blob + 8, s->layers);
    write_u32(blob + 12, HEATMAP_MOD_COMBOS);
    write_u32(blob + 16, persistent);
    return HEATMAP_HEADER_SIZE;
}
size_t heatmap_encode_chunk(const struct heatmap_state *s, uint32_t chunk, uint8_t *blob) {
    if (chunk >= heatmap_chunk_count(s)) {
        return 0;
    }
    uint32_t start = chunk * HEATMAP_CHUNK_CELLS;
    uint32_t n = chunk_cells(s, chunk);
    for (uint32_t i = 0; i < n; i++) {
        write_u32(blob + 4 * i, s->counts[start + i]);
    }
    return 4 * n;
}
int heatmap_restore_header(struct heatmap_state *s, const uint8_t *blob, size_t length) {
    if (length != HEATMAP_HEADER_SIZE || read_u32(blob) != HEATMAP_FORMAT_VERSION ||
        read_u32(blob + 4) != s->positions || read_u32(blob + 8) != s->layers ||
        read_u32(blob + 12) != HEATMAP_MOD_COMBOS || read_u32(blob + 16) > 1) {
        return -EINVAL;
    }
    s->persistent = read_u32(blob + 16);
    if (!s->persistent) {
        s->dirty = 0;
    }
    s->generation++;
    return 0;
}
int heatmap_restore_chunk(struct heatmap_state *s, uint32_t chunk, const uint8_t *blob,
                          size_t length) {
    if (chunk >= heatmap_chunk_count(s) || length != 4 * chunk_cells(s, chunk)) {
        return -EINVAL;
    }
    /* Restore once at boot; input may already have arrived before settings_load(). */
    uint32_t start = chunk * HEATMAP_CHUNK_CELLS;
    for (uint32_t i = 0; i < length / 4; i++) {
        uint32_t stored = read_u32(blob + 4 * i);
        uint32_t *c = &s->counts[start + i];
        *c = stored > UINT32_MAX - *c ? UINT32_MAX : *c + stored;
    }
    s->generation++;
    return 0;
}
void heatmap_saved(struct heatmap_state *s, uint64_t dirty_snapshot, uint64_t now_ms, int error) {
    s->storage_error = error;
    s->last_attempt_ms = now_ms;
    if (!error) {
        s->dirty = s->dirty >= dirty_snapshot ? s->dirty - dirty_snapshot : 0;
    }
}
void heatmap_reset_done(struct heatmap_state *s, uint64_t now_ms, int error) {
    s->storage_error = error;
    if (!error) {
        memset(s->counts, 0, 4 * s->cell_count);
        s->dirty = 0;
        s->last_attempt_ms = now_ms;
        s->generation++;
    }
}
void heatmap_mode_done(struct heatmap_state *s, bool enabled, uint64_t now_ms, int error) {
    s->storage_error = error;
    if (!error) {
        s->persistent = enabled;
        s->dirty = 0;
        s->last_attempt_ms = now_ms;
        s->generation++;
    }
}
int heatmap_page(const struct heatmap_state *s, uint32_t offset, uint32_t *counts, size_t *length) {
    if (offset >= s->cell_count) {
        return -EINVAL;
    }
    *length = s->cell_count - offset;
    if (*length > HEATMAP_PAGE_SIZE) {
        *length = HEATMAP_PAGE_SIZE;
    }
    memcpy(counts, s->counts + offset, *length * sizeof(*counts));
    return 0;
}
