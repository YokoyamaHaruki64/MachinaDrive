---
name: format-build-flow
description: Project_Driveでテキスト形式の検査・修正後に、差分検査とDebug/Releaseビルドを行う。C++、Scene、Shader、プロジェクト設定を変更した時に使う。
---

# 形式修正とビルド検証

Project_Driveのテキスト形式を整え、変更内容をビルドで検証する時に使う。ゲーム仕様の判断、UI配置、アセット制作には使わない。

## 実行順

1. まず `.codex-work/verify.ps1` を実行し、対象ファイルと形式違反を確認する。
2. 形式違反があれば、`.codex-work/format-text.ps1` を使って変更済み・未追跡ファイルを整形する。個別に直す必要がある場合だけ `-Path` を指定する。
3. `.codex-work/verify.ps1` を再実行し、形式検査と `git diff --check` が通ることを確認する。
4. C++、Shader、Visual Studioプロジェクト設定を変更した場合は、Debug|x64をビルドする。Releaseへの影響がある変更はRelease|x64もビルドする。

```powershell
# 検査
pwsh -NoProfile -File .\.codex-work\verify.ps1

# 変更対象を一括整形
pwsh -NoProfile -File .\.codex-work\format-text.ps1

# 個別整形
pwsh -NoProfile -File .\.codex-work\format-text.ps1 -Path source\Component\UIText.cpp

# Debugビルドを含む検証
pwsh -NoProfile -File .\.codex-work\verify.ps1 -Build -BuildConfiguration Debug -Platform x64
```

## 形式規約

- C++とPowerShellはUTF-8 BOM・CRLF。
- HLSLと実行時に読むYAML／JSON等はBOMなしUTF-8・CRLF。
- `.editorconfig` と `AGENTS.md` の規約を優先する。
- `format-text.ps1` はUTF-8として読めないファイルとリポジトリ外のパスを中止する。エラーを回避するために別のエンコーディングで上書きしない。

## 注意

- 既存の未コミット変更は消去・復元しない。整形対象を限定したい場合は `-Path` を使う。
- Windows SDKへのアクセスが必要なビルドでは権限昇格を要求する。
- 自動テスト基盤はないため、描画・入力・Sceneに影響する変更後はビルドに加えて該当箇所を手動確認する。
