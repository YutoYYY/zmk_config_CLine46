import { TypingHeatmapSection } from "./TypingHeatmapSection";
import "./App.css";
import { connect as gattConnect } from "@zmkfirmware/zmk-studio-ts-client/transport/gatt";
import {
  ZMKConnection,
  isWebSerialSupported,
  isWebBluetoothSupported,
  connectSerial,
} from "@cormoran/zmk-studio-react-hook";

export const SUBSYSTEM_IDENTIFIER = "cline46_typing_heatmap";

// Template placeholder: `scripts/init_module.py` rewrites this literal to
// `{owner}/{repo}`. Never write the full
// `...-with-custom-studio-rpc` repo name in a URL built from this constant --
// the replacement targets this exact string first, which would otherwise
// leave the owner unreplaced.
export const GITHUB_REPO = "YutoYYY/zmk_config_CLine46";

// Unlike GITHUB_REPO above, this always credits the original template
// project, regardless of which repo this module was forked into. The
// trailing comment is scripts/init_module.py's IGNORE_MARKER: it keeps this
// line from being rewritten (like GITHUB_REPO is) or flagged as a leftover
// placeholder once initialized.
export const TEMPLATE_CREDIT_REPO = "cormoran/zmk-module-template"; // zmk-module-template:keep

function App() {
  return (
    <div className="app">
      <header className="app-header">
        <h1>CLine46 キー使用回数</h1>
        <p>キーボード本体に記録した押下回数を表示します</p>
      </header>

      <ZMKConnection
        autoReconnect
        renderDisconnected={({ connect, isLoading, error }) => (
          <section className="card">
            <h2>接続</h2>
            {isLoading && <p>⏳ 接続中...</p>}
            {error && (
              <div className="error-message">
                <p>🚨 {error}</p>
              </div>
            )}
            {!isLoading && (
              <>
                <div className="connect-buttons">
                  {isWebSerialSupported() && (
                    <button
                      className="btn btn-primary"
                      onClick={() => connect(connectSerial)}
                    >
                      🔌 USBで接続
                    </button>
                  )}
                  {isWebBluetoothSupported() && (
                    <button
                      className="btn btn-primary"
                      onClick={() => connect(gattConnect)}
                    >
                      📶 Bluetoothで接続
                    </button>
                  )}
                  {!isWebSerialSupported() && !isWebBluetoothSupported() && (
                    <div className="warning-message">
                      <p>
                        ⚠️ Web Serial and Web Bluetooth are unavailable here.
                        Use a Chromium-based browser (Chrome, Edge, ...) over
                        HTTPS or localhost to connect to your keyboard.
                      </p>
                    </div>
                  )}
                </div>
                {isWebBluetoothSupported() && (
                  <p className="hint-message">
                    📶 Bluetoothの一覧に出ないときは、先にキーボードの Studio
                    Unlock(L3)を押してください。 DYAStudio
                    とは同時に接続できません。
                  </p>
                )}
              </>
            )}
          </section>
        )}
        renderConnected={({ disconnect, deviceName }) => (
          <>
            <section className="card">
              <h2>接続</h2>
              <div className="device-info">
                <h3>✅ 接続中: {deviceName}</h3>
              </div>
              <p className="hint-message">
                Bluetoothで接続した場合、切断してもブラウザが接続を握ったままになり、DYAStudio
                がつながらないことがあります。DYAStudio
                を使う前にこのタブを閉じてください。
              </p>
              <button className="btn btn-secondary" onClick={disconnect}>
                切断
              </button>
            </section>

            <TypingHeatmapSection />
          </>
        )}
      />

      <footer className="app-footer">
        <p>
          <strong>CLine46</strong> — cormoran/zmk-feature-typing-heatmap を改造
        </p>
        <p>
          <a
            href={`https://github.com/${GITHUB_REPO}`}
            target="_blank"
            rel="noreferrer"
          >
            {GITHUB_REPO}
          </a>
        </p>
        <p className="template-credit">
          Built from{" "}
          <a
            href={`https://github.com/${TEMPLATE_CREDIT_REPO}`}
            target="_blank"
            rel="noreferrer"
          >
            {TEMPLATE_CREDIT_REPO}
          </a>{" "}
          - AI ready ZMK module template by{" "}
          <a
            href="https://github.com/cormoran"
            target="_blank"
            rel="noreferrer"
          >
            @cormoran
          </a>
        </p>
      </footer>
    </div>
  );
}

export default App;
