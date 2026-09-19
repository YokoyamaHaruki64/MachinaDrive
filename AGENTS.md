# Project_Drive 開発ガイド

詳細な構造と実行フローは [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) を参照する。

## 資料運用

- 恒久的に残す設計・仕様資料は `docs/` に置く。
- 引継ぎ、作業メモ、調査途中、TODOなどの一時資料は `.codex-work/` に置き、Gitで管理しない。
- 一時資料で確定した内容は、必要に応じて `docs/` の該当資料へ反映する。

## 開発環境

- ソリューションは `Project_Drive.sln`、プロジェクトは `Project_Drive.vcxproj`。
- 標準の開発・検証構成は `Debug|x64`。Visual Studio 2022（Platform Toolset v143）、C++20、Windows SDK 10 を使用する。
- 実行時の相対パスを解決するため、デバッグ実行時の作業ディレクトリはリポジトリ直下にする。

## コーディングとファイル追加

- `.editorconfig` を正とし、変更直後に変更箇所を含む対象ファイルごとに文字コード・BOM・改行・末尾改行・行末空白を確認する。共通設定は UTF-8 BOM、CRLF、最終改行あり、行末空白の削除である。ただし、実行時に読み込むテキストデータとHLSLはBOMなしUTF-8とする。
- C++ ソース / ヘッダー（`.cpp`、`.cxx`、`.cc`、`.c`、`.h`、`.hpp`、`.hxx`、`.inl`）は UTF-8 BOM・CRLF・スペース4個インデントとする。
- HLSL（`.hlsl`、`.hlsli`、`.fx`、`.fxh`）は BOM なし UTF-8・CRLF・スペース4個インデントとする。fxc互換性のため BOM を付けない。
- 実行時に読み込むテキストデータ（`.json`、`.yml`、`.yaml`、`.scene`、`.prefab`、`.material`、`.shader`、`.ini`、`.toml`）は BOM なし UTF-8・CRLFとする。Parser間のBOM対応差を避けるため、BOMを付けない。
- Visual Studio プロジェクト設定（`.vcxproj`、`.vcxproj.filters`、`.vcxproj.props`、`.props`、`.targets`）と XML、CMake、JSON、YAML、Markdown はスペース2個インデントとする。ソリューション（`.sln`、`.slnx`）はタブインデントとする。
- Markdown は行末空白を保持してよい。YAML系の実行時データ（`.yml`、`.yaml`、`.scene`、`.prefab`、`.material`）も、出力ライブラリがNullを`key: `として出力する場合があるため、行末空白の検査・削除対象外とする。その他の対象ファイルでは行末空白を残さない。
- Visual Studio のフィルタ構造は実際の `source/`・`include/` 配下のフォルダ構造に対応させる。ファイル追加・移動時は、`.vcxproj.filters` の登録先も同じ階層へ更新する。
- 関数内は処理の目的が変わる単位で空白行を分け、意図がコードだけでは読み取りにくい処理には簡潔な日本語コメントを付ける。コメントは処理の再説明ではなく、責務・前提・理由を示す。
- 関数間には空白行を置き、関連する初期化・検証・GPU設定・描画などの処理群を視覚的に区切る。過剰な装飾や自明なコメントは避ける。
- 新規 `.cpp`、`.h`、`.hlsl` は Visual Studio プロジェクトに明示登録されているため、追加・削除時は `.vcxproj` と `.vcxproj.filters` も更新する。
- 新しい HLSL は `.vcxproj` の `FxCompile` に追加し、種類（Vertex / Pixel）、Shader Model 5.0、出力先を x64 の Debug / Release の両方に設定する。
- `source/ImGui/` と `include/ImGui/` は外部ライブラリ本体のため、必要性を確認したうえでのみ変更する。

## ビルドと確認

- Visual Studio で `Project_Drive.sln` を開き、`Debug`・`x64` を選択してビルド・実行する。
- コマンドラインでは、Visual Studio 開発者コマンド プロンプトから次を実行できる。

  ```powershell
  msbuild .\Project_Drive.sln /m /p:Configuration=Debug /p:Platform=x64
  ```

- 自動テスト基盤は現時点で確認できない。変更時は少なくとも対象構成のビルドを行い、起動後に影響範囲のシーン、入力、描画、音声、ImGui 操作を手動確認する。

## 開発用スクリプト

- `.codex-work\verify.ps1` は、差分検査、変更・未追跡ファイルの文字コード／BOM／改行検査、Visual Studio プロジェクト設定、未完了タスクの件数をまとめて確認する。文字コード検査はリポジトリ全体ではなく、`HEAD`との差分と未追跡ファイルだけを対象にする。
- 検証のみを行う場合は `powershell -ExecutionPolicy Bypass -File .\.codex-work\verify.ps1` を実行する。`-Build -BuildConfiguration Debug|Release|Both -Platform x64` を付けると、検証後に指定構成をビルドする。
- `.codex-work\build.ps1` はMSBuildの場所を自動検出してビルドする。例: `powershell -ExecutionPolicy Bypass -File .\.codex-work\build.ps1 -Configuration Both -Platform x64`。`Both` は Debug（Editor）→Release（Runtime）の順で実行する。
- `.codex-work\status.ps1` は、ブランチ、未コミット変更、差分統計、直近コミット、未完了タスクを短く表示する。作業開始時は `powershell -ExecutionPolicy Bypass -File .\.codex-work\status.ps1` を優先し、引継ぎ本文が必要なときだけ `-ShowNextSession` を付ける。
- `.codex-work\changes.ps1` は、差分本文を出さずに変更ファイルとhunk位置を表示する。`-Cached` でステージ済み差分、`-Path <path>` で対象の絞り込み、内容が必要なときだけ `-ShowPatch -ContextLines <n>` を使用する。
- 生成ファイルの改行を意図的に除外する場合だけ `verify.ps1 -SkipTextFormatCheck` を使用する。通常は省略し、`-SkipDiffCheck` は差分検査を明示的に省略する必要がある場合だけ使用する。
- これらは開発環境向けの一時スクリプトであり、`.codex-work/` と同様にGit管理対象外である。MSBuildやWindows SDKへのアクセス時は、実行環境によって権限確認が表示されることがある。

## 変更時の注意

- `Assets/`、`BuildedShader/`、`.vs/` は `.gitignore` 対象。`config/`はテーマ・実行時設定・同梱フォントを追跡し、開発者ごとの差分になる`config/imgui.ini`だけを追跡しない。
- 既存の未コミット変更には触れず、対象外の整形やファイル名変更は行わない。
- 外部ライブラリや SDK を追加・更新する前に確認を取る。
- コミット、push、外部サービスへの操作はユーザーの明示的な承認後に行う。

## ゲームリポジトリのGit運用

- このリポジトリは`GM_Framework`の`v0.2.0`を起点にしたゲーム制作リポジトリである。
- `origin`はゲーム側、`upstream`はフレームワーク側として登録する。リモートURLは`git remote -v`で確認し、資料へ固定記載しない。
- `main`はゲームの安定版、`dev`はゲーム開発用とする。ゲーム機能は`feature/*`、再利用可能なエンジン変更は`engine/*`で作業する。
- 通常のpush先は`origin`とし、フレームワーク側へ誤pushしないよう`git push origin <branch>`を明示する。`upstream`は原則としてfetch・差分確認用である。
- フレームワーク更新は`git fetch upstream --tags`で取得し、内容を確認してから`upstream/dev`または指定タグをゲーム側へ取り込む。
- ゲーム制作中に先行実装したエンジン改善は、可能なら`upstream/dev`を起点に`engine/*`ブランチを作る。汎用化できた変更はフレームワーク側へPRし、ゲーム固有の変更はゲーム側に残す。
- エンジン変更とゲーム検証用変更は、`[Engine]`と`[Game]`のコミットに分ける。コミットメッセージは既存の`[Category] 日本語の要約`形式を守る。
- フレームワークのタグとゲームのリリースタグは分離する。ゲーム側では`game-v0.1.0`のようにゲーム用タグ名を付ける。
- Codex開始時は、この`AGENTS.md`、`.codex-work/NEXT_SESSION.md`、`docs/GAME_DEVELOPMENT.md`、`docs/ARCHITECTURE.md`、`docs/PHYSICS.md`、`docs/SERIALIZATION.md`を確認する。
