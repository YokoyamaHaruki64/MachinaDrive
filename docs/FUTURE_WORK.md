# 今後の課題

本資料は、現在の機能完了条件には含めない拡張・最適化項目を集約する。着手時は内容を具体的な作業タスクへ分解し、関連する設計資料と検証項目を更新する。

## Transform と数学

- World / Local設定時の不要な行列分解と、dirty伝播後の不要なキャッシュ再計算を削減する。
- Quaternionを正とする回転APIと、Euler角のInspector表示・編集規則を見直す。
- `Matrix4x4`のSIMD対応への置換を可能にするため、TransformからDirectXMath依存をさらに局所化する。

## 描画

- Mesh / Material / Pass単位のDrawItem化とBindキャッシュを導入する。
- Shadow Phaseの実描画キューを追加する。
- 透過MaterialとCull方針を整理する。
- TextureCube、Texture2DArray、Buffer系Resource Propertyを追加する。

## ModelAsset

- FBX / glTFのPBR MaterialをDefaultLit Propertyへ対応付け、複数Texture種別とMaterialごとの描画Stateを反映する。
- Skeleton、Animation、SkinningをModelAssetとShaderへ追加する。
- Morph Target / Blend Shapeを読み込む。
- 大規模モデル向けにMesh結合、Index format、頂点StreamとフォールバックBuffer容量を見直す。

## Serialize / Deserialize

- `YAMLCodec<T>`、固定長配列Traits、Field登録時の型消去コールバックが重なっている現行実装を見直し、新しい型を短く直感的に追加できる記述方式へ簡素化する。
- 簡素化時も、rapidyaml依存の局所化、配列要素Codecの再利用、全Field検証後の一括反映、参照解決エラーの継続処理は維持する。

## 検証

- OBJ、FBX、glTF、透明Materialを含むVRoid、Skinning / Animationを含むモデルで表示を確認する。
- Transform最適化の着手時は、親子Transformを含むシーンでフレーム時間と挙動を確認する。
