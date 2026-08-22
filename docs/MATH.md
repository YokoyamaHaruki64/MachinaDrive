# 数学機能の設計

## Quaternion

Quaternionは3次元回転を表す型である。内部値は`x`、`y`、`z`、`w`の4要素を持ち、回転なしは`{ 0.0f, 0.0f, 0.0f, 1.0f }`とする。回転を表すQuaternionは正規化済みであることを前提とし、演算結果や外部入力をTransformへ設定する時点で正規化する。

Euler角は編集時・既存API互換時の入出力表現としてのみ用いる。角度の単位はラジアンで、成分の順序は既存の`XMMatrixRotationRollPitchYaw()`と同じく`x: Pitch`、`y: Yaw`、`z: Roll`とする。

### 採用するAPI

```cpp
class Quaternion
{
public:
    float x;
    float y;
    float z;
    float w;

    static Quaternion Identity();

    static Quaternion EulerToQuat(const Vector3& eulerRadians);
    Vector3 QuatToEuler() const;

    static Quaternion FromAxisAngle(const Vector3& axis, float radians);
    static Quaternion LookRotation(const Vector3& forward, const Vector3& up);
    static Quaternion Slerp(const Quaternion& from, const Quaternion& to, float t);

    Quaternion Normalized() const;
    Quaternion Inversed() const;
    Matrix4x4 ToMatrix() const;

    Quaternion operator*(const Quaternion& rhs) const;
    Vector3 RotateVector(const Vector3& vector) const;
};
```

| API | 用途 |
| --- | --- |
| `Identity()` | 回転なしのQuaternionを得る。 |
| `EulerToQuat()` / `QuatToEuler()` | Inspectorや既存コードが扱うEuler角との相互変換を行う。 |
| `FromAxisAngle()` | 指定軸を指定角度だけ回転するQuaternionを作る。軸は正規化して扱う。 |
| `LookRotation()` | 指定した前方方向と上方向から回転を作る。`forward`がゼロベクトル、または`up`と平行に近い場合は安全な既定方向を用いる。 |
| `Slerp()` | 2つの回転を一定角速度に近い形で補間する。`t`は`0.0f`から`1.0f`へクランプする。 |
| `Normalized()` | 単位Quaternionへ正規化した値を返す。 |
| `Inversed()` | 逆回転を返す。 |
| `ToMatrix()` | 回転行列へ変換する。行列の並びと合成順はフレームワークのMatrix4x4規約に従う。 |
| `operator*` | 回転を合成する。合成順は`ToMatrix()`の行列積と等価になるよう実装する。 |
| `RotateVector()` | QuaternionでVector3を回転する。 |

`Slerp()`は、2つの回転の内積が負の場合に一方を反転し、常に短い経路で補間する。十分に近い回転同士は数値安定性のため線形補間後に正規化してよい。

Quaternion同士の加算・減算、Euler角を直接加算する演算子は提供しない。回転合成には`operator*`、補間には`Slerp()`、Eulerによる設定には変換APIを使用する。
