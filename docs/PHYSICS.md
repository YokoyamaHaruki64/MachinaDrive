# 物理システム

## 概要

物理シミュレーションにはJolt Physics 5.6.0を使用する。`Physics`がJoltの初期化、Body管理、固定ステップ、Transform同期、接触イベントを担当し、Jolt固有型はComponentやScriptへ公開しない。ゲーム固有のPlayerやEnemyは通常のScriptであり、Physicsから参照しない。

初期対応はBox、Sphere、Capsule、Static／Kinematic／Dynamic Body、重力、反発、摩擦、Sleep、Trigger、Layerフィルターとする。Mesh Collider、CharacterVirtual、Constraint、Raycastは後続機能とする。

## 更新順

Play中の1フレームは次の順で更新する。

1. Sceneと全Componentの`Update()`
2. Collider構成とStatic／Kinematic TransformをJoltへ同期
3. 60 Hzの固定物理ステップを最大4回実行
4. Dynamic BodyのWorld姿勢をTransformへ同期
5. 接触イベントをメインスレッドで配信
6. 全Componentの`LateUpdate()`

JoltのContactListenerは複数のワーカースレッドから呼ばれる。コールバックでは接触情報だけをPhysics内部のキューへ保存し、GameObjectやComponentには触れない。イベントはJoltの更新完了後にメインスレッドで配信する。

## BodyとCollider階層

Rigidbodyは所有GameObjectを基準とする1つのJolt Bodyを持つ。そのGameObjectと子階層にあるColliderをCompound Shapeへまとめる。Colliderは上方向へ探索して最初に見つかったRigidbodyに所属し、子階層に別のRigidbodyがある場合はそこで親Bodyとの所属を分ける。

親階層にRigidbodyがないColliderは独立したStatic Bodyとして扱う。RigidbodyにColliderが1つも属さない場合もJoltの`EmptyShape`を持つBodyを生成する。EmptyShapeは他のBodyと接触せず、Rigidbodyの速度、重力、Sleep、Transform同期だけを利用できる。Colliderの追加時はEmptyShapeを実際のShapeへ差し替え、Colliderがなくなった場合は再びEmptyShapeへ戻す。

子Colliderの形状は子TransformのWorld行列から求め、Rigidbody所有者のWorld行列に対する相対行列へ変換してCompound Shapeへ格納する。JoltのSubShapeと元のColliderを対応付けることで、イベントでは実際に接触した子Colliderを通知する。親子構造、相対Transform、center、形状寸法が変化した場合は固定ステップ前にShapeを更新する。

## Transform同期

StaticとKinematic BodyはComponentの`Update()`後にTransformのWorld姿勢をJoltへ反映する。Dynamic BodyはJoltを正とし、物理ステップ後にBodyのWorld姿勢をRigidbody所有者のTransformへ反映する。親を持つTransformでもWorld値を使用し、既存のWorld setterによってLocal値へ逆変換する。

非一様Scaleは各Collider形状へ焼き込む。Boxは各軸、Sphereは最大軸、CapsuleはXZの最大軸を半径、Y軸を高さへ適用する。負のScaleは絶対値を使用する。Colliderのcenterは既存仕様どおりScaleせず、Collider Transformの回転と移動を適用する。

## Rigidbody

RigidbodyはJolt Bodyを操作するエンジン側Componentであり、Joltの`BodyID`を公開しない。MotionType、mass、velocity、bounciness、friction、useGravity、allowSleep、freezeRotationを保存対象とする。MotionTypeはStatic、Kinematic、Dynamicを持ち、実行中の切り替えを許可する。

速度設定や加算は次の物理ステップへ反映する。Dynamic Bodyの位置をScriptから直接変更した場合はテレポートとしてJoltへ同期し、Bodyを起床させる。

## LayerとTrigger

Colliderは所属する`CollisionLayer`と`IsTrigger`を持つ。Layer番号と衝突可能なLayer番号の組み合わせは`Physics.h`のコンパイル時定数へ定義し、ColliderごとのMaskは持たせない。Compound Body内でColliderごとにLayerが異なるため、実際の可否はSubShape単位で判定する。

衝突可能なLayerペアに登録されていないCollider同士は、物理反応も接触イベントも発生させない。ペアの順番は区別しない。

接触したColliderのどちらかがTriggerなら、その接触だけをSensorとして扱う。Compound Body全体をSensorにはしないため、同じBody内にSolid ColliderとTrigger Colliderを共存できる。Triggerは物理的な押し戻しを行わないが、通常の接触イベントを発生させる。

## 接触イベント

接触イベントはColliderペア単位で集約し、両GameObjectへ受信側基準の`HitInfo`を通知する。`objectA`、`colliderA`、`rigidbodyA`は受信側、Bは相手側を表す。normalは相手から受信側を押し戻す方向とする。

- 接触した最初の固定ステップは`OnHitBegin()`だけを呼ぶ。
- 次の固定ステップ以降、接触中は`OnHit()`を毎ステップ呼ぶ。
- 実際に分離したときは`OnHitEnd()`を1回呼ぶ。

GameObjectはイベント時点で有効な全Componentへ通知する。通知中にDestroyが要求された場合は、そのGameObjectの残りのComponentを呼ばない。JoltがSleep時に出す接触削除は分離として扱わず、休眠接触をキャッシュする。起床後に実際の分離を確認した場合だけ`OnHitEnd()`を通知する。

Scene破棄、Play終了、Physics終了では接触キャッシュを通知なしで破棄する。個別Colliderの無効化または破棄では、生存している相手側にだけ`OnHitEnd()`を通知する。

## 外部ライブラリ

JoltはVisual Studio 2022、x64、静的MSVC RuntimeでDebug／Releaseライブラリを個別にビルドする。プロジェクトと合わせてDebugは`/MTd`、Releaseは`/MT`とする。RTTI、例外、Jolt DebugRenderer、Profiler、ObjectStream、GPU Compute、LTOは無効化する。

Gitには使用するJoltヘッダー、静的ライブラリ、MIT License、バージョンとビルド条件を記録した資料を含める。Joltのサンプルやビルド生成物全体は含めない。
