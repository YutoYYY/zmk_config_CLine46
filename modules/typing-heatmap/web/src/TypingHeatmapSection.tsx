import { useCallback, useContext, useEffect, useRef, useState } from "react";
import {
  ZMKAppContext,
  useCustomSubsystem,
  useStudioLockState,
  isUnlockRequiredError,
} from "@cormoran/zmk-studio-react-hook";
import { Request } from "./proto/cormoran/feature-typing-heatmap/feature_typing_heatmap";
import {
  HEATMAP_CODEC,
  type HeatmapSnapshot,
  readHeatmap,
  requireResponse,
  exportHeatmap,
  splitTables,
  modComboName,
} from "./heatmap";
import { LAYOUT } from "./layout";

const SUBSYSTEM_IDENTIFIER = "cline46_typing_heatmap";
const LAYER_NAMES = LAYOUT.layers.map((l) => l.name);
const LABELS = LAYOUT.layers.map((l) => [...l.labels]);

type View =
  | { kind: "total" }
  | { kind: "layer"; layer: number }
  | { kind: "mods"; combo: number };

function heat(count: number, max: number) {
  return count === 0
    ? "var(--key-empty)"
    : `hsl(24 90% ${92 - (count / max) * 45}%)`;
}

export function Keyboard({
  counts,
  labels,
}: {
  counts: number[];
  labels: string[];
}) {
  const max = Math.max(1, ...counts);
  const width = Math.max(...LAYOUT.keys.map((k) => k.x + k.w));
  const height = Math.max(...LAYOUT.keys.map((k) => k.y + k.h));
  return (
    <div className="kb" style={{ aspectRatio: `${width} / ${height}` }}>
      {LAYOUT.keys.map((k, i) => (
        <div
          key={i}
          className="kb-key"
          title={`位置 ${i}: ${labels[i]} = ${counts[i] ?? 0}回`}
          style={{
            left: `${(k.x / width) * 100}%`,
            top: `${(k.y / height) * 100}%`,
            width: `${(k.w / width) * 100}%`,
            height: `${(k.h / height) * 100}%`,
            background: heat(counts[i] ?? 0, max),
          }}
        >
          <span className="kb-label">{labels[i]}</span>
          <strong>{(counts[i] ?? 0).toLocaleString()}</strong>
        </div>
      ))}
    </div>
  );
}

function sum(values: number[]) {
  return values.reduce((a, b) => a + b, 0);
}

export function TypingHeatmapSection() {
  const app = useContext(ZMKAppContext);
  const connection = app?.state.connection;
  const { ready, subsystem, call } = useCustomSubsystem(
    SUBSYSTEM_IDENTIFIER,
    HEATMAP_CODEC
  );
  const subsystemIndex = subsystem?.index;
  const { locked } = useStudioLockState();
  const [snapshot, setSnapshot] = useState<HeatmapSnapshot | null>(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [awaitingUnlock, setAwaitingUnlock] = useState(false);
  const [confirmReset, setConfirmReset] = useState(false);
  const [view, setView] = useState<View>({ kind: "layer", layer: 0 });
  const session = useRef(0);
  const inFlight = useRef(false);
  const retryRead = useRef(false);
  const previouslyLocked = useRef(locked);

  const perform = useCallback(
    async (mutation?: Request) => {
      if (!ready || inFlight.current) return;
      const identity = session.current;
      inFlight.current = true;
      setBusy(true);
      setError(null);
      setAwaitingUnlock(false);
      let mutationSucceeded = false;
      try {
        if (mutation) {
          const result = requireResponse(await call(mutation));
          if (!result.mutation)
            throw new Error("Unexpected mutation response.");
          mutationSucceeded = true;
          if (identity === session.current) setSnapshot(null);
        }
        if (identity !== session.current) return;
        const next = await readHeatmap(call);
        if (identity === session.current) {
          setSnapshot(next);
          retryRead.current = false;
        }
      } catch (failure) {
        if (identity !== session.current) return;
        if (isUnlockRequiredError(failure)) {
          retryRead.current = !mutation;
          setAwaitingUnlock(true);
        } else {
          const message =
            failure instanceof Error ? failure.message : "Request failed.";
          setError(
            mutationSucceeded
              ? `Operation succeeded, but refreshing statistics failed: ${message}`
              : message
          );
        }
      } finally {
        if (identity === session.current) {
          inFlight.current = false;
          setBusy(false);
        }
      }
    },
    [ready, call]
  );

  useEffect(() => {
    const identity = ++session.current;
    inFlight.current = false;
    retryRead.current = false;
    // Connection changes reflect an external device, so clear the old device's view.
    // eslint-disable-next-line react-hooks/set-state-in-effect
    setSnapshot(null);
    setError(null);
    setAwaitingUnlock(false);
    setConfirmReset(false);
    setBusy(false);
    if (ready) {
      // StrictMode replays setup/cleanup in development. Start only if this
      // session survives that replay, as well as a same-tick disconnect.
      void Promise.resolve().then(() => {
        if (session.current === identity) void perform();
      });
    }
    return () => {
      session.current = identity + 1;
    };
  }, [connection, subsystemIndex, ready, perform]);

  useEffect(() => {
    const becameUnlocked = previouslyLocked.current && !locked;
    previouslyLocked.current = locked;
    if (awaitingUnlock && becameUnlocked && retryRead.current) {
      retryRead.current = false;
      void perform();
    }
  }, [awaitingUnlock, locked, perform]);

  if (!app) return null;
  if (subsystemIndex === undefined) {
    return (
      <section className="card">
        <p>
          このキーボードのファームには記録機能(「{SUBSYSTEM_IDENTIFIER}
          」)が入っていません。
          記録機能入りのファームを右手側に書き込んでください。
        </p>
      </section>
    );
  }

  const tables = snapshot ? splitTables(snapshot) : null;
  const total = tables ? sum(tables.total) : 0;
  let counts: number[] = [];
  let labels: string[] = LABELS[0] ?? [];
  let caption = "";
  if (tables) {
    if (view.kind === "total") {
      counts = tables.total;
      caption =
        "物理キーごとの押下回数(レイヤーを問わず、コンボの構成キーも含む)。刻印はL0。";
    } else if (view.kind === "layer") {
      counts = tables.byLayer[view.layer] ?? [];
      labels = LABELS[view.layer] ?? labels;
      caption = `${LAYER_NAMES[view.layer] ?? "L" + view.layer} で実行されたキー。▽のキーは下のレイヤーに数えられます。`;
    } else {
      counts =
        view.combo === 0
          ? tables.positions
            ? tables.total.map((_, p) => sum(tables.byMods.map((t) => t[p])))
            : []
          : (tables.byMods[view.combo - 1] ?? []);
      caption = `${view.combo === 0 ? "いずれかの修飾キー" : modComboName(view.combo)} を押しながら押したキー。刻印はL0(実際には他のレイヤーのキーの場合もあります)。`;
    }
  }
  const topLayers = tables
    ? tables.byLayer
        .flatMap((t, l) => t.map((count, p) => ({ l, p, count })))
        .filter((e) => e.count > 0)
        .sort((a, b) => b.count - a.count)
        .slice(0, 15)
    : [];
  const topMods = tables
    ? tables.byMods
        .flatMap((t, m) => t.map((count, p) => ({ m: m + 1, p, count })))
        .filter((e) => e.count > 0)
        .sort((a, b) => b.count - a.count)
        .slice(0, 15)
    : [];

  return (
    <section className="card statistics" aria-busy={busy}>
      <h2>キー使用回数</h2>
      <p>
        押した回数の合計だけを記録しています(打った順番や時刻は残りません)。
      </p>
      <div className="statistics-actions">
        <button
          className="btn btn-primary"
          disabled={busy || !ready}
          onClick={() => void perform()}
        >
          更新
        </button>
        <button
          className="btn"
          disabled={busy || !snapshot}
          onClick={() =>
            snapshot && exportHeatmap(snapshot, LAYER_NAMES, LABELS)
          }
        >
          JSONで書き出す
        </button>
        <button
          className="btn"
          disabled={busy || !snapshot}
          onClick={() => setConfirmReset(true)}
        >
          リセット
        </button>
      </div>
      {busy && <p role="status">読み込み中…</p>}
      {error && (
        <p role="alert" className="error-message">
          {error}
        </p>
      )}
      {awaitingUnlock && (
        <div className="unlock-prompt">
          <p>
            キーボードの Studio Unlock(L3)を押してから、再試行してください。
          </p>
          <button
            className="btn"
            disabled={busy}
            onClick={() => void perform()}
          >
            再試行
          </button>
        </div>
      )}
      {snapshot && tables && (
        <>
          <p className="total">
            合計{" "}
            <strong data-testid="total-presses">
              {total.toLocaleString()}
            </strong>{" "}
            回
          </p>
          <div className="view-tabs" role="tablist">
            <button
              className={view.kind === "total" ? "tab active" : "tab"}
              onClick={() => setView({ kind: "total" })}
            >
              物理キー
            </button>
            {tables.byLayer.map((t, l) => (
              <button
                key={l}
                className={
                  view.kind === "layer" && view.layer === l
                    ? "tab active"
                    : "tab"
                }
                onClick={() => setView({ kind: "layer", layer: l })}
              >
                {LAYER_NAMES[l] ?? `L${l}`}{" "}
                <small>{sum(t).toLocaleString()}</small>
              </button>
            ))}
          </div>
          <div className="view-tabs" role="tablist">
            <span className="tab-caption">修飾キー:</span>
            {[0, ...tables.byMods.map((_, m) => m + 1)].map((combo) => {
              const n =
                combo === 0
                  ? sum(tables.byMods.map(sum))
                  : sum(tables.byMods[combo - 1]);
              if (combo !== 0 && n === 0) return null;
              return (
                <button
                  key={combo}
                  className={
                    view.kind === "mods" && view.combo === combo
                      ? "tab active"
                      : "tab"
                  }
                  onClick={() => setView({ kind: "mods", combo })}
                >
                  {combo === 0 ? "すべて" : modComboName(combo)}{" "}
                  <small>{n.toLocaleString()}</small>
                </button>
              );
            })}
          </div>
          <p className="hint-message">{caption}</p>
          <Keyboard counts={counts} labels={labels} />
          <div className="rankings">
            <div>
              <h3>よく使うキー(レイヤー別)</h3>
              <ol>
                {topLayers.map((e) => (
                  <li key={`${e.l}-${e.p}`}>
                    {LAYER_NAMES[e.l] ?? `L${e.l}`}:{" "}
                    <b>{LABELS[e.l]?.[e.p] ?? e.p}</b> —{" "}
                    {e.count.toLocaleString()}
                  </li>
                ))}
              </ol>
            </div>
            <div>
              <h3>よく使う修飾キーの組み合わせ</h3>
              <ol>
                {topMods.map((e) => (
                  <li key={`${e.m}-${e.p}`}>
                    <b>
                      {modComboName(e.m)} + {LABELS[0]?.[e.p] ?? e.p}
                    </b>{" "}
                    — {e.count.toLocaleString()}
                  </li>
                ))}
              </ol>
            </div>
          </div>
          <label className="persistence-toggle">
            <input
              type="checkbox"
              checked={snapshot.persistenceEnabled}
              disabled={busy || !snapshot.persistenceSupported}
              onChange={(event) =>
                void perform({
                  setPersistence: { enabled: event.target.checked },
                })
              }
            />
            電源を切っても記録を残す
          </label>
          {!snapshot.persistenceSupported && (
            <p className="warning-message">
              このファームでは記録を保存できません。
            </p>
          )}
          {snapshot.persistenceEnabled ? (
            <p>
              本体への保存は、前回から
              {Math.round(snapshot.saveIntervalSeconds / 60)}分以上経ち、かつ
              {snapshot.minPresses}回以上押したときに行います。未保存:{" "}
              {snapshot.unsavedPresses.toLocaleString()}
              回(再起動すると消えます)。
            </p>
          ) : (
            <p>保存しない設定です。再起動すると記録は消えます。</p>
          )}
          {snapshot.storageError !== 0 && (
            <p role="alert" className="error-message">
              保存エラー({snapshot.storageError}
              )。記録はメモリ上には残っています。
            </p>
          )}
        </>
      )}
      {confirmReset && (
        <div
          className="reset-confirmation"
          role="group"
          aria-label="Confirm statistics reset"
        >
          <p>記録をすべて消去します。元に戻せません。よろしいですか?</p>
          <button
            className="btn"
            disabled={busy}
            onClick={() => {
              setConfirmReset(false);
              void perform({ reset: {} });
            }}
          >
            消去する
          </button>
          <button
            className="btn"
            disabled={busy}
            onClick={() => setConfirmReset(false)}
          >
            やめる
          </button>
        </div>
      )}
    </section>
  );
}
