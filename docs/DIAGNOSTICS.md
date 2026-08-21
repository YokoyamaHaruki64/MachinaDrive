# 診断システム

## 概要

診断システムは、実行中のログ収集、ファイル出力、性能計測、ImGui表示を担当する。ゲーム側の処理から診断情報の出力処理を分離し、ログファイルと画面上の情報から現在の動作状況を確認できるようにする。

```text
診断システム
  ├─ Logger
  │    └─ スレッドセーフなログ記録キュー
  ├─ LogSystem
  │    ├─ 秒単位の同一ログ集約
  │    ├─ LogFileWorkerへの配送
  │    └─ ImGui Logタブ
  ├─ PerformanceMonitor
  │    ├─ フレーム処理時間の計測
  │    ├─ オブジェクト数の記録
  │    └─ ImGui Performanceタブ
  └─ DebugWindow
       └─ 診断用ImGuiタブの統括
```

## 責務

| クラス | 責務 |
| --- | --- |
| `Logger` | 任意のスレッドからログを記録する公開API |
| `LogSystem` | ログキューの取り出し、同一ログの集約、各出力先への配送、Logタブの描画 |
| `LogFileWorker` | 確定したログを専用スレッドでファイルへ書き出す |
| `PerformanceMonitor` | メインループの処理時間とオブジェクト数を記録し、Performanceタブを描画する |
| `DebugWindow` | `Debug`ウィンドウとタブバーを所有し、各診断タブを配置する |

`DebugWindow`だけがImGuiの`Debug`ウィンドウとタブバーを開始・終了する。`LogSystem`と`PerformanceMonitor`は、それぞれが担当するタブの内容だけを描画する。新しい診断表示を追加する場合も同じ構造に従う。

## 実行フロー

```text
起動
  └─ LogSystem::Init
       └─ LogFileWorkerを開始

毎フレーム
  ├─ Time::Update
  ├─ PerformanceMonitor::RecordFrameTime
  ├─ Manager::Update
  │    ├─ Update区間を計測
  │    ├─ LateUpdate区間を分離して計測
  │    └─ Active / Pendingオブジェクト数を記録
  ├─ Manager::Draw
  │    ├─ Render区間を計測
  │    └─ DebugWindowからPerformance / Logタブを描画
  └─ LogSystem::Flush
       └─ 最大LogsPerFrame件を処理

終了
  └─ LogSystem::Shutdown
       ├─ Loggerの残件をすべて配送
       └─ LogFileWorkerの書き込み完了後にスレッドを終了
```

## ログ記録

ScriptやComponentは`SystemInc.h`から公開される`Logger`だけを利用し、出力タイミングを制御する`LogSystem`は直接操作しない。

`Logger::Log()`、`Logger::Warning()`、`Logger::Error()`は、次の情報をスレッドセーフなFIFOへ記録する。

- ログレベル
- メッセージ
- 記録時刻
- 呼び出し元のファイルと行番号
- 記録した側のスレッドID
- 同一ログの発生回数

FIFOは最大`Logger::MaxQueuedLogs`件とする。上限到達時は古い低重要度ログから置き換え、重要度の高いログが残りやすい状態を保つ。破棄した件数はLogタブに表示する。

## 同一ログの集約

`LogSystem::Flush()`はフレーム終端で最大`LogSystem::LogsPerFrame`件をFIFOから取り出す。次の情報が一致するログを同じ発生元として識別する。

- 記録された秒
- ログレベル
- ファイル
- 行番号
- スレッドID

発生元が一致した後にメッセージのハッシュを比較する。ハッシュ一致時にはメッセージ本体も比較し、衝突による誤集約を防ぐ。同一ログは現在秒の間、発生回数だけを更新する。秒が変わった時点でログを確定し、ファイル出力へ配送する。

## ファイル出力

`LogFileWorker`は確定した構造化ログを受け取り、専用スレッドで文字列へ整形して`Logs/Log_日時_PID.txt`へ書き込む。

- 待ちキューは最大`LogFileWorker::MaxQueuedLogs`件
- ファイルの`flush()`は1秒間隔と終了時
- ImGui用履歴は最大`LogSystem::ImGuiHistoryCount`件
- Logタブのフィルター未使用時は、画面内の項目だけを整形・描画

ログはメタ情報と本文を分け、複数行の本文も各行をスペース4個分インデントする。

```text
[02:10:15.123][Error][60][CameraController.cpp:20][Thread 12345]
    Playerコンポーネントを持つGameObjectが見つかりません。
```

## 性能計測

`PerformanceMonitor`は`std::chrono::steady_clock`を用いてメインスレッドのCPU経過時間を計測する。各時間は直近`PerformanceMonitor::SampleCount`フレームを保持し、現在値、平均値、最大値をミリ秒で表示する。履歴が規定数に達するまでは、収集済みのサンプルだけで平均値と最大値を求める。

| 項目 | 計測範囲 |
| --- | --- |
| FPS | 直近の平均Frame Timeから`1000 / 平均ミリ秒`で算出 |
| Frame Time | 前回フレームから今回フレームまでの非スケーリング実時間 |
| Update Time | `Manager::Update()`全体からLateUpdate区間だけを除いた時間 |
| LateUpdate Time | 全GameObjectの`LateUpdate()`ループに要した時間 |
| Render Time | `Manager::Draw()`の開始から`Renderer::End()`完了までの時間 |
| Active Objects | 破棄処理後の稼働GameObject数 |
| Pending Objects | 次の有効化を待つGameObject数 |

Update Timeには入力、ImGuiフレーム開始、シーン遷移、保留オブジェクトの`OnStart()`、SceneとGameObjectの`Update()`、破棄処理を含む。Update中に呼ばれた`OnStart()`もUpdate負荷として扱う。

Render TimeにはSceneとRendererの描画、ポストエフェクト、ImGui描画、`Present(1, 0)`の待機時間を含む。この値はメインスレッド側の経過時間であり、GPU単体の処理時間ではない。

## スレッド境界

- `Logger`の記録APIは任意のスレッドから呼び出せる。
- ログの取り出しと集約、ImGui履歴の更新、性能計測、Debugウィンドウの描画はメインスレッドで行う。
- ログファイルの文字列整形と書き込みは`LogFileWorker`の専用スレッドで行う。
