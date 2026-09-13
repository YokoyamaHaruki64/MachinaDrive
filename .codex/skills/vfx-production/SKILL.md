---
name: vfx-production
description: Project_Driveで、ゲームイベントの意味を読み取り、ParticleSystem・Trail・Quad・Shader・テクスチャを選んでワールドVFXを設計・制作する。VFXの追加や調整、素材制作に使う。
---

# VFX Production

ゲーム上の意味が先、見た目はその後とする。対象イベント、既存の色・音・UI、カメラ距離、HitStop・Pause・破棄の順序を理解してから、最小の表現手段を選ぶ。いきなりParticleや画像を作り始めない。

このスキルはProject_Driveの現行コンポーネント構成を前提にする。エンジン側の再利用可能な機能追加が必要になった場合は、ゲームVFXの実装と混ぜず`engine/*`で扱う。

## 制作の流れ

1. 発生元のScript、Prefab、Materialと`docs/GAME_LOOP.md`を読み、いつ何を伝えるVFXかを一文で定める。効果を予兆、発生、ピーク、余韻に分ける。
2. 既存リファレンスを使う場合は、観察できた特徴と実装上の推定を分ける。固有意匠ではなく、レイヤー、時間構造、読みやすさを取り出す。
3. [renderer-selection.md](references/renderer-selection.md)を読み、ParticleSystem、Trail、Quad、UI、PostProcessから最小構成を選ぶ。時間変化にはCurveを先に設計する。
4. テクスチャが要る場合は既存素材を確認する。幾何学形状、グラデーション、ノイズ、アトラス化、切り抜き、リサイズ、アルファ／チャンネル加工は、再生成可能なPythonなどのスクリプトで作る。スクリプトは入力、出力、seed、解像度を明示する。
5. 画像生成は、複雑な有機形状、固有モチーフ、手続き的な加工で意図を満たせない素材だけに許可する。出力は透明背景・用途別・小サイズを基本とし、文字、ロゴ、不要な背景を焼き込まない。
6. 既製Shaderで不足するなら、新しいShaderを書いてよい。UVスクロール、ノイズ変位、ディゾルブ、フリップブック、Fresnel、深度フェードなどは、必要な見た目と入力パラメータを定めてから追加する。
7. Prefab、Material、最小限のScriptを実装する。複数イベントで使うPrefabは`VFX_`接頭辞を使い、生成元と寿命を明確にする。
8. Game Viewで、イベント同期、HitStop中の見え方、透明ソート、Bloom、連続発生時の視認性、破棄後の残留を確認する。密度・寿命・色・Curveを先に調整する。

## 実装判断

- 初期実装は加算合成を基本にする。大きな半透明面や濃い煙を多用すると、Renderer単位の透明ソートとオーバードローが目立つ。
- 短い命中は発光核と火花から始める。リングや残滓は読みやすさが足りない時だけ加える。
- 既存機能で表せないことをPrefab内の複雑な回避策で代用しない。新規Shaderで解くか、Component拡張が必要な理由を整理する。
- ワールドVFXとUIは役割を分ける。時間回復のような重要イベントは、ワールド位置、HUD数値、短いUIポップを同期させる。

## Shader追加の規約

- Shaderは1つの視覚的責務に絞る。用途・Blend・Depth・Cull・必要Texture・公開Propertyを明記する。
- 色替えするVFXは、白または明度中心のテクスチャと`_BaseColor`等のPropertyで分ける。
- 動的値は共有Materialを変更せず、`MaterialPropertyOverride`で渡す。
- 深度、透明、UV、発光、時間を使う場合は、VFXの見た目に必要な理由と、同時発生時の負荷を確認する。

## 変更後の報告

効果ごとに、伝える意味、選んだ表現手段、Shaderとテクスチャの役割、Curveの意図、確認した状況、残る制約を簡潔に報告する。

## 蓄積ノウハウ

- `ParticleRenderer`は粒子ごとの回転を持たない。火花や「+」のように、カメラ正対した同一形状を複数出して成立する素材を優先する。方向差を形状へ焼き込む必要がある場合は、Cone／SphereVelの運動と放射対称のTextureを組み合わせる。
- `ParticleEmitter`の`colorOverLifetime`は、Materialの既定色と粒子ごとの色に掛かる。白いTextureを用意し、色とAlphaはCurveで変えると同じ素材を属性別に再利用できる。
- `CurveColor`はR、G、B、Aが独立した`Curve<float>`で、時刻は寿命0–1である。透明度は最終キーを0にし、ピーク後の減衰を明示する。Bezier制御点を極端に動かすと、意図しない発光の跳ねが出る。
- `ParticleEmitter`の`finishedDestroy`は、そのEmitterの全粒子が消えた時にOwnerを破棄する。複数Emitterや子Objectを含む複合Prefabでは、親寿命を別Scriptで管理する。
- Prefab YAMLの`type`はInspector表示名ではなくクラス登録名を使う。たとえば表示名が`Particle Emitter`／`Particle Renderer`でも、YAMLは`ParticleEmitter`／`ParticleRenderer`である。
- 機械生成PNGは、素材本体を`Assets/Local/`へ出力し、再生成スクリプトをプロジェクト内の`.codex/tools/`へ置く。入力、出力、解像度を固定し、生成先ディレクトリを自動作成する。
- Windows PowerShellの`System.Drawing`でPNGを生成する場合、Windows PowerShell（`powershell.exe`）を使う。PowerShell 7の`System.Drawing.Common`は実行中の.NET世代と参照DLLの不一致で失敗しやすいため、生成スクリプトは対象シェルを明示して早期エラーにする。
- `Color.FromArgb`へ渡すAlphaは、丸めで256にならないよう0–255へClampしてからFloorする。透明素材の境界値で生成を止めない。
- 透明背景の白素材は白いビューア上で形が見えない。生成後はAlphaを暗い背景へ合成して、シルエットと境界を確認する。
- `TrailRenderer`はOwnerのWorld位置履歴を保持する。移動Headを外周へリセットして再利用すると、中心から外周へ不自然な長い線が繋がるため、チャージのような反復演出は「1 Head = 1移動 = 1破棄」とし、親VFXが間隔と同時数を制限して新しいHeadを生成する。
- 状態中だけ存在する親VFXは、生成元が解除時に破棄するだけでなく、親とHeadの両方が対象Scriptの状態を監視して自壊する。入力解除とUpdate順が前後しても、次フレームに残留しない。
- Trail用Textureを既存素材から作る場合、色をグレースケール化しておくと、Trail Materialの`_BaseColor`だけでゲームの属性色へ再利用できる。
- 衝突点から法線の両側へ同時に放射する場合、ParticleEmitterを「方向ごと」に分け、さらに色を混在させるなら各方向へ色別Emitterを重ねる。1つの`colorOverLifetime`はEmitter内の全粒子へ共通なので、2色混在を1Emitterだけで表現しようとしない。
- `HitInfo.normal`の向きは物理通知のA/B規約（このプロジェクトではBからAを押し戻す方向）を確認してからVFXの方向へ変換する。接触位置は推測した中点より`HitInfo.point`を優先し、攻撃判定の実体が子Objectでもワールド位置を維持する。
- 火花を炎寄りに見せるときは、加算合成で白く飛ばすよりAlphaBlendを基本にし、橙・黄橙・赤橙のような近い温度差をEmitter単位で混在させる。白い火炎シルエットTexture（`VFX_Flame.png`）と短い白ピークの`CurveColor`を使い、寿命後半のScaleとAlphaを急減衰させると先端が細く消える。
- 発光を合成方式で表現する場合、ParticleRendererの色だけではマテリアル単位の色に留まるため、将来的にParticleSystemへ`EmissionColor`を追加して粒子ごとに渡す。Blend変更と発光色の追加は別段階で検証する。
