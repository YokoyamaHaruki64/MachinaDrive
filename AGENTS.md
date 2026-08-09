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

- `.editorconfig` を正とする。C++ / ヘッダーは UTF-8 BOM・CRLF・4 スペース、HLSL は BOM なし UTF-8・4 スペースを守る。
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

## 変更時の注意

- `Assets/`、`BuildedShader/`、`config/`、`.vs/` は `.gitignore` 対象。生成物やローカル設定を追跡対象へ変更しない。
- 既存の未コミット変更には触れず、対象外の整形やファイル名変更は行わない。
- 外部ライブラリや SDK を追加・更新する前に確認を取る。
- コミット、push、外部サービスへの操作はユーザーの明示的な承認後に行う。
