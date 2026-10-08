/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Counter cells are laid out as three tables over the same key positions:
 *   [0, P)                    total presses by position
 *   [P, P + L*P)              presses by (resolved layer index, position)
 *   [P + L*P, P + L*P + M*P)  presses by (held modifier combo 1..15, position)
 * Modifier combo bits: 1=Ctrl 2=Shift 4=Alt 8=GUI (left/right merged).
 */
#define HEATMAP_MOD_COMBOS 15U
#define HEATMAP_HEADER_SIZE 20U
#define HEATMAP_CHUNK_CELLS 256U
#define HEATMAP_CHUNK_SIZE (4U * HEATMAP_CHUNK_CELLS)
#define HEATMAP_PAGE_SIZE 32U
#define HEATMAP_FORMAT_VERSION 2U

struct heatmap_state {
    uint32_t *counts;
    uint32_t positions;
    uint32_t layers;
    uint32_t cell_count;
    uint32_t generation;
    uint64_t dirty;
    uint64_t last_attempt_ms;
    bool persistent;
    int storage_error;
};

static inline uint32_t heatmap_cells(uint32_t positions, uint32_t layers) {
    return positions * (1U + layers + HEATMAP_MOD_COMBOS);
}
static inline uint32_t heatmap_chunk_count(const struct heatmap_state *s) {
    return (s->cell_count + HEATMAP_CHUNK_CELLS - 1) / HEATMAP_CHUNK_CELLS;
}

void heatmap_init(struct heatmap_state *s, uint32_t *counts, uint32_t positions, uint32_t layers,
                  bool persistent);
/* A physical press seen before any behavior can capture it. */
void heatmap_press(struct heatmap_state *s, uint32_t position, bool pressed);
/* A press as resolved by the keymap: the layer that handled it and the held modifiers. */
void heatmap_resolved(struct heatmap_state *s, uint32_t position, uint32_t layer, uint8_t mods);
bool heatmap_due(const struct heatmap_state *s, uint64_t now_ms, uint32_t interval_seconds,
                 uint32_t min_presses);
size_t heatmap_encode_header(const struct heatmap_state *s, uint8_t *blob, bool persistent);
size_t heatmap_encode_chunk(const struct heatmap_state *s, uint32_t chunk, uint8_t *blob);
/* Validates the header record and applies its mode; returns -EINVAL for other layouts. */
int heatmap_restore_header(struct heatmap_state *s, const uint8_t *blob, size_t length);
int heatmap_restore_chunk(struct heatmap_state *s, uint32_t chunk, const uint8_t *blob,
                          size_t length);
void heatmap_saved(struct heatmap_state *s, uint64_t dirty_snapshot, uint64_t now_ms, int error);
void heatmap_reset_done(struct heatmap_state *s, uint64_t now_ms, int error);
void heatmap_mode_done(struct heatmap_state *s, bool enabled, uint64_t now_ms, int error);
int heatmap_page(const struct heatmap_state *s, uint32_t offset, uint32_t *counts, size_t *length);
