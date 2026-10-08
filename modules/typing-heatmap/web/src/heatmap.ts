import {
  Request,
  Response,
  StatsResponse,
} from "./proto/cormoran/feature-typing-heatmap/feature_typing_heatmap";

// Keep this object stable: it is an input to useCustomSubsystem's call callback.
export const HEATMAP_CODEC = {
  encode: (request: Request) => Request.encode(request).finish(),
  decode: Response.decode,
};

export type HeatmapSnapshot = StatsResponse;
export type HeatmapCall = (request: Request) => Promise<Response | null>;

export function requireResponse(response: Response | null): Response {
  if (!response) throw new Error("The keyboard returned no response.");
  if (response.error) throw new Error(response.error.message);
  return response;
}

/** Read a coherent snapshot without polling while the user types. */
export async function readHeatmap(call: HeatmapCall): Promise<HeatmapSnapshot> {
  for (let attempt = 0; attempt < 3; attempt++) {
    let first: StatsResponse | undefined;
    const counts: number[] = [];
    let retry = false;
    do {
      const page = requireResponse(
        await call({ getStats: { offset: counts.length } })
      ).stats;
      if (!page)
        throw new Error("The keyboard returned an unexpected response.");
      if (first && page.generation !== first.generation) {
        retry = true;
        break;
      }
      first ??= page;
      if (
        page.offset !== counts.length ||
        page.positionCount !== first.positionCount ||
        page.counts.length > 32 ||
        counts.length + page.counts.length > page.positionCount ||
        (page.counts.length === 0 && counts.length < page.positionCount)
      ) {
        throw new Error("The keyboard returned an invalid statistics page.");
      }
      counts.push(...page.counts);
    } while (counts.length < first.positionCount);
    if (!retry && first) return { ...first, counts };
  }
  throw new Error(
    "Statistics changed while loading. Pause typing briefly, then press Refresh."
  );
}

/** Views over the flat counter array: [total][layer x position][modifier combo x position]. */
export interface HeatmapTables {
  positions: number;
  total: number[];
  byLayer: number[][];
  byMods: number[][]; // index 0 = combo 1 (Ctrl) ... index 14 = combo 15
}

export function splitTables(snapshot: HeatmapSnapshot): HeatmapTables {
  const p = snapshot.keyPositions;
  const c = snapshot.counts;
  const slice = (table: number) => c.slice(table * p, (table + 1) * p);
  return {
    positions: p,
    total: slice(0),
    byLayer: Array.from({ length: snapshot.layerCount }, (_, l) =>
      slice(1 + l)
    ),
    byMods: Array.from({ length: snapshot.modComboCount }, (_, m) =>
      slice(1 + snapshot.layerCount + m)
    ),
  };
}

const MOD_NAMES = ["Ctrl", "Shift", "Alt", "Win/⌘"];
export function modComboName(combo: number): string {
  return MOD_NAMES.filter((_, bit) => combo & (1 << bit)).join("+");
}

export function exportHeatmap(
  snapshot: HeatmapSnapshot,
  layerNames: string[],
  labels: string[][]
) {
  const t = splitTables(snapshot);
  const blob = new Blob(
    [
      JSON.stringify(
        {
          exportedAt: new Date().toISOString(),
          note: "counts only (no order, no timestamps). byLayer = layer that handled the press; byModifiers = modifiers held at that time. labels come from the repository keymap.",
          keyPositions: t.positions,
          totalPresses: t.total.reduce((sum, count) => sum + count, 0),
          total: t.total,
          byLayer: t.byLayer.map((counts, l) => ({
            layer: l,
            name: layerNames[l] ?? `L${l}`,
            labels: labels[l],
            counts,
          })),
          byModifiers: t.byMods.map((counts, m) => ({
            combo: modComboName(m + 1),
            counts,
          })),
          persistenceEnabled: snapshot.persistenceEnabled,
          unsavedPresses: snapshot.unsavedPresses,
        },
        null,
        1
      ),
    ],
    { type: "application/json" }
  );
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `cline46-heatmap-${new Date().toISOString().slice(0, 10)}.json`;
  link.click();
  URL.revokeObjectURL(url);
}
