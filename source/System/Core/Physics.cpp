#include "pch.h"

#include "System/Core/Physics.h"

#include "Component/BoxCollider.h"
#include "Component/CapsuleCollider.h"
#include "Component/Rigidbody.h"
#include "Component/SphereCollider.h"
#include "Component/Transform.h"
#include "System/Core/GameObject.h"

#include <array>
#include <limits>

namespace
{
    constexpr float COLLISION_EPSILON = 1.0e-6f;
    constexpr float POSITION_CORRECTION_PERCENT = 0.8f;
    constexpr float POSITION_CORRECTION_SLOP = 1.0e-4f;

    // 衝突点そのものではなく「最近接点間の距離」と
    // 「重なり深度」を返す簡易形。解決側はこの深度と法線だけを使う。

    struct BoxShape
    {
        // 回転したBoxを中心・World上の直交3軸・各軸の半サイズで表す。
        // この表現に変換しておくと、軸投影（SAT）を形状判定ごとに共通利用できる。
        Vector3 center{ Vector3::Zero() };
        std::array<Vector3, 3> axes{
            Vector3::Right(),
            Vector3::Up(),
            Vector3::Forward(),
        };
        std::array<float, 3> halfExtents{ 0.5f, 0.5f, 0.5f };
    };

    struct CapsuleShape
    {
        // Capsuleは「中心線の有限線分を半径分だけ膨らませた形状」として扱う。
        // 以降の判定では、まず線分同士/線分とBoxの最近接距離を求め、半径と比較する。
        Vector3 pointA{ Vector3::Zero() };
        Vector3 pointB{ Vector3::Zero() };
        Vector3 center{ Vector3::Zero() };
        float radius{ 0.5f };
    };

    void SetHitResult(HitInfo& hitInfo, const Vector3& normal, float penetration)
    {
        hitInfo.hit = true;
        hitInfo.normal = normal.LengthSqr() > COLLISION_EPSILON
            ? normal.Normalized()
            : Vector3::Right();
        hitInfo.penetration = std::max(penetration, 0.0f);
    }

    bool IsUsableCollider(const Collider* collider)
    {
        // PhysicsWorldを持たない設計なので、渡されたPairだけをここで検査する。
        // 無効化中・非アクティブ・破棄予約中のColliderは衝突対象にしない。
        if (!collider || !collider->IsEnabled() || !collider->GetOwner())
        {
            return false;
        }

        const GameObject* owner = collider->GetOwner();
        return owner->IsActive() && !owner->IsDestroyed();
    }

    float GetMaximumAbsScale(const Transform* transform)
    {
        if (!transform)
        {
            return 1.0f;
        }

        const Vector3 scale = transform->GetScale();
        // 非一様Scaleを球へ正確に反映すると楕円体になるため、最大軸の外接球を採用する。
        return std::max({ std::abs(scale.x), std::abs(scale.y), std::abs(scale.z) });
    }

    float GetSphereRadius(const SphereCollider& collider)
    {
        return std::abs(collider.GetRadius()) * GetMaximumAbsScale(collider.transform());
    }

    BoxShape GetBoxShape(const BoxCollider& collider)
    {
        BoxShape shape;
        shape.center = collider.GetWorldCenter();

        // Boxの回転を3本のLocal軸へ反映し、OBBとして扱う。
        // サイズは半分にしておくと、軸へ投影したときの距離計算がそのまま使える。
        const Transform* transform = collider.transform();
        const Quaternion rotation = transform ? transform->GetRotation() : Quaternion::Identity();
        const Vector3 scale = transform ? transform->GetScale() : Vector3::One();
        const Vector3 size = collider.GetSize();

		// 各軸をrotationで回転させ、Scaleを反映する。
        shape.axes[0] = rotation.RotateVector(Vector3::Right()).Normalized();
        shape.axes[1] = rotation.RotateVector(Vector3::Up()).Normalized();
        shape.axes[2] = rotation.RotateVector(Vector3::Forward()).Normalized();
        shape.halfExtents = {
            std::abs(size.x * scale.x) * 0.5f,
            std::abs(size.y * scale.y) * 0.5f,
            std::abs(size.z * scale.z) * 0.5f,
        };
        return shape;
    }

    CapsuleShape GetCapsuleShape(const CapsuleCollider& collider)
    {
        CapsuleShape shape;
        shape.center = collider.GetWorldCenter();

        const Transform* transform = collider.transform();
        const Quaternion rotation = transform ? transform->GetRotation() : Quaternion::Identity();
        const Vector3 scale = transform ? transform->GetScale() : Vector3::One();
        const float radialScale = std::max(std::abs(scale.x), std::abs(scale.z));
        shape.radius = std::abs(collider.GetRadius()) * radialScale;

        // Capsuleの中心線はLocal Y軸。中心線方向にはYScale、径方向にはX/Zの大きい方を使う。
        const Vector3 axis = rotation.RotateVector(Vector3::Up()).Normalized();
        const float halfHeight = std::abs(collider.GetHeight() * scale.y) * 0.5f;
        const float halfSegment = std::max(halfHeight - shape.radius, 0.0f);
        shape.pointA = shape.center - axis * halfSegment;
        shape.pointB = shape.center + axis * halfSegment;
        return shape;
    }

	// 線分上の最近接点を求める。線分の長さが0の場合は線分の開始点を返す。
    Vector3 GetClosestPointOnSegment(
        const Vector3& point,
        const Vector3& segmentStart,
        const Vector3& segmentEnd)
    {
        // 線分を作る
        const Vector3 segment = segmentEnd - segmentStart;
        const float segmentLengthSqr = segment.LengthSqr();
        if (segmentLengthSqr <= COLLISION_EPSILON)
        {
            // 長さ0の線分は点として扱い、ゼロ除算を避ける。
            return segmentStart;
        }

		// Dot = 0未満なら線分の開始点、1より大きければ終了点、0～1なら線分上の位置になる

        const float parameter = std::clamp(
            Vector3::Dot(point - segmentStart, segment) / segmentLengthSqr,
            0.0f,
            1.0f);

		// 線分上の最近接点を計算する
        return segmentStart + segment * parameter;
    }

	// 線分とAABBの交差判定。交差する場合は線分上の交差位置を返す。
    bool SegmentIntersectsBox(
        const Vector3& segmentStart,
        const Vector3& segmentEnd,
        const std::array<float, 3>& halfExtents,
        float& intersectionParameter)
    {
        const std::array<float, 3> start{
            segmentStart.x,
            segmentStart.y,
            segmentStart.z,
        };
        const std::array<float, 3> direction{
            segmentEnd.x - segmentStart.x,
            segmentEnd.y - segmentStart.y,
            segmentEnd.z - segmentStart.z,
        };

        // 線分をP(t)=start+direction*t (0<=t<=1)と表し、各軸で許容される
        // tの区間を切り詰める。3軸の共通区間が残れば、線分はAABBと交差する。
        float minimumParameter = 0.0f;
        float maximumParameter = 1.0f;
        for (size_t i = 0; i < 3; ++i)
        {
            if (std::abs(direction[i]) <= COLLISION_EPSILON)
            {
				// その軸へ進まない線分はAABBの範囲内に開始点があるかで判定
                if (start[i] < -halfExtents[i]
                    || start[i] > halfExtents[i])
                {
                    return false;
                }
                continue;
            }

			// 交差する場合のtの範囲を求める。AABBの面に対して線分がどこで交わるかを計算する。
			// parameterA = tの最小値、parameterB = tの最大値
            float parameterA = (-halfExtents[i] - start[i]) / direction[i];
            float parameterB = (halfExtents[i] - start[i]) / direction[i];
            if (parameterA > parameterB)
            {
                std::swap(parameterA, parameterB);
            }

            minimumParameter = std::max(minimumParameter, parameterA);
            maximumParameter = std::min(maximumParameter, parameterB);
            if (minimumParameter > maximumParameter)
            {
                return false;
            }
        }

        intersectionParameter = minimumParameter;
        return true;
    }

    Vector3 ClampPointToBox(const Vector3& point, const BoxShape& box)
    {
        return Vector3(
            std::clamp(point.x, -box.halfExtents[0], box.halfExtents[0]),
            std::clamp(point.y, -box.halfExtents[1], box.halfExtents[1]),
            std::clamp(point.z, -box.halfExtents[2], box.halfExtents[2]));
    }

	// 線分とBoxの最近接点を求める。交差する場合は交差位置を返す。
    void GetClosestPointsSegmentBox(
        const Vector3& segmentStart,
        const Vector3& segmentEnd,
        const BoxShape& box,
        Vector3& outSegmentPoint,
        Vector3& outBoxPoint)
    {
        float intersectionParameter = 0.0f;
        if (SegmentIntersectsBox(
            segmentStart,
            segmentEnd,
            box.halfExtents,
            intersectionParameter))
        {
            // 中心線がBox内部へ入る場合、最近接距離は0。交差位置を両方の最近接点にする。
            outSegmentPoint = segmentStart
                + (segmentEnd - segmentStart) * intersectionParameter;
            outBoxPoint = outSegmentPoint;
            return;
        }

        float minimumDistanceSqr = std::numeric_limits<float>::max();

		// 線分の両端点を考慮する。線分の端点がBox内部にある場合、最近接距離は0になる。
        auto considerParameter = [&](float parameter)
        {
            const Vector3 segmentPoint = segmentStart
                + (segmentEnd - segmentStart) * parameter;
            const Vector3 boxPoint = ClampPointToBox(segmentPoint, box);
            const float distanceSqr = (boxPoint - segmentPoint).LengthSqr();

			// 最小距離と最近接点を更新
            if (distanceSqr < minimumDistanceSqr)
            {
                minimumDistanceSqr = distanceSqr;
                outSegmentPoint = segmentPoint;
                outBoxPoint = boxPoint;
            }
        };

        considerParameter(0.0f);
        considerParameter(1.0f);

		// 線分上の最近接点を三分探索で求める
        // 線分がBox内部に入らない場合、最近接点は線分上にある。
        float minimumParameter = 0.0f;
        float maximumParameter = 1.0f;

        for (int i = 0; i < 16; ++i)
        {
			// 三分探索の中点
            // 線分上のパラメータを3等分して、距離が小さい方の区間に絞る。
            const float parameterA = minimumParameter
                + (maximumParameter - minimumParameter) / 3.0f;

            const float parameterB = maximumParameter
                - (maximumParameter - minimumParameter) / 3.0f;

			// 線分上の点を計算
            const Vector3 pointA = segmentStart
                + (segmentEnd - segmentStart) * parameterA;

            const Vector3 pointB = segmentStart
                + (segmentEnd - segmentStart) * parameterB;

			// 線分上の点とBoxの最近接点を求める
            const float distanceA = (ClampPointToBox(pointA, box) - pointA).LengthSqr();
            const float distanceB = (ClampPointToBox(pointB, box) - pointB).LengthSqr();

			// 距離が小さい方の区間に絞る
            if (distanceA < distanceB)
            {
                maximumParameter = parameterB;
            }
            else
            {
                minimumParameter = parameterA;
            }
        }
        // 三分探索の結果を反映
        considerParameter((minimumParameter + maximumParameter) * 0.5f);
    }

    void GetInsideBoxNormal(
        const BoxShape& box,
        const Vector3& localPoint,
        Vector3& localNormal,
        float& distanceToFace)
    {
        // 点がBox内部にあると外側の最近接点が一意に定義できないため、最も近い面を選ぶ。
        // 内包状態では、ResolveCollisionが点側（A）をBox外へ押し出せるように、
        // Boxの外向き法線とは逆向き（Box内部へ向く向き）を返す。
        const float distanceToXFace = box.halfExtents[0] - std::abs(localPoint.x);
        const float distanceToYFace = box.halfExtents[1] - std::abs(localPoint.y);
        const float distanceToZFace = box.halfExtents[2] - std::abs(localPoint.z);

        distanceToFace = distanceToXFace;
        if (distanceToYFace < distanceToFace)
        {
            distanceToFace = distanceToYFace;
            localNormal = Vector3(0.0f, localPoint.y >= 0.0f ? -1.0f : 1.0f, 0.0f);
        }
        else
        {
            localNormal = Vector3(localPoint.x >= 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
        }

        if (distanceToZFace < distanceToFace)
        {
            distanceToFace = distanceToZFace;
            localNormal = Vector3(0.0f, 0.0f, localPoint.z >= 0.0f ? -1.0f : 1.0f);
        }
    }

    bool CheckSphereSphere(
        const SphereCollider& sphereA,
        const SphereCollider& sphereB,
        Vector3& normal,
        float& penetration)
    {
        // 2つの中心間距離が半径の和以下なら、球同士が重なっている。
        const Vector3 delta = sphereB.GetWorldCenter() - sphereA.GetWorldCenter();
        const float distanceSqr = delta.LengthSqr();
        const float combinedRadius = GetSphereRadius(sphereA) + GetSphereRadius(sphereB);
        if (distanceSqr > combinedRadius * combinedRadius)
        {
            return false;
        }

        const float distance = std::sqrt(distanceSqr);
        normal = distance > COLLISION_EPSILON ? delta / distance : Vector3::Right();
        penetration = combinedRadius - distance;
        return true;
    }

    bool CheckSphereBox(
        const SphereCollider& sphere,
        const BoxCollider& box,
        Vector3& normal,
        float& penetration)
    {
        // Sphere中心をBoxのLocal座標へ移し、回転したOBBを軸平行AABBとして扱う。
        // AABB上の最近接点とSphere中心の距離が半径以下なら接触している。
        const BoxShape boxShape = GetBoxShape(box);
        const Transform* boxTransform = box.transform();
        const Quaternion boxRotation = boxTransform
            ? boxTransform->GetRotation()
            : Quaternion::Identity();

		// BoxのLocal座標系でのSphere中心を求める。
        // Boxの回転を逆にして、Boxの中心からの相対位置を計算する。
        const Vector3 localSphereCenter = boxRotation.Inversed().RotateVector(
            sphere.GetWorldCenter() - boxShape.center);

		// BoxのAABB上の最近接点を求める
        const Vector3 closestPoint(
            std::clamp(localSphereCenter.x, -boxShape.halfExtents[0], boxShape.halfExtents[0]),
            std::clamp(localSphereCenter.y, -boxShape.halfExtents[1], boxShape.halfExtents[1]),
            std::clamp(localSphereCenter.z, -boxShape.halfExtents[2], boxShape.halfExtents[2]));

		// 最近接点からSphere中心へのベクトルを求める
        const Vector3 sphereToBox = closestPoint - localSphereCenter;
        const float distanceSqr = sphereToBox.LengthSqr();
        const float radius = GetSphereRadius(sphere);

		// 距離が半径より大きい場合は接触していない
        if (distanceSqr > radius * radius)
        {
            return false;
        }

        // 接触
        if (distanceSqr > COLLISION_EPSILON)
        {
            // Boxの最近接点からSphere中心へ向かう方向ではなく、SphereからBoxへ向かう方向を返す。
            const float distance = std::sqrt(distanceSqr);
			normal = boxRotation.RotateVector(sphereToBox / distance);// BoxのLocal座標系での法線をWorld座標系へ変換
            penetration = radius - distance;
            return true;
        }

		// Sphere中心がBox内部にある場合は、Boxの面の法線を仮想接触面として返す。
        Vector3 localNormal;
        float distanceToFace = 0.0f;
        // 中心がBox内部にある場合は最短の面を仮想接触面として選ぶ。
        GetInsideBoxNormal(boxShape, localSphereCenter, localNormal, distanceToFace);

        normal = boxRotation.RotateVector(localNormal);
        penetration = radius + std::max(distanceToFace, 0.0f);
        return true;
    }

    bool CheckCapsuleSphere(
        const CapsuleCollider& capsule,
        const SphereCollider& sphere,
        Vector3& normal,
        float& penetration)
    {
        // Capsuleの中心線上でSphere中心に最も近い点を求め、そこからの距離を
        // Capsule半径 + Sphere半径と比較する。これは線分を半径分膨らませる考え方である。
        const CapsuleShape capsuleShape = GetCapsuleShape(capsule);

        const Vector3 sphereCenter = sphere.GetWorldCenter();

        const Vector3 closestPoint = GetClosestPointOnSegment(
            sphereCenter,
            capsuleShape.pointA,
            capsuleShape.pointB);

        const Vector3 delta = sphereCenter - closestPoint;
        const float distanceSqr = delta.LengthSqr();
        const float combinedRadius = capsuleShape.radius + GetSphereRadius(sphere);

        if (distanceSqr > combinedRadius * combinedRadius)
        {
            return false;
        }

        const float distance = std::sqrt(distanceSqr);
        if (distance > COLLISION_EPSILON)
        {
            normal = delta / distance;
        }
        else
        {
            // 距離が小さい場合はCapsule中心からの方向を法線とする
            const Vector3 centerDelta = sphereCenter - capsuleShape.center;
            normal = centerDelta.LengthSqr() > COLLISION_EPSILON
                ? centerDelta.Normalized()
                : Vector3::Right();
        }
        penetration = combinedRadius - distance;
        return true;
    }

    void GetClosestPointsSegments(
        const Vector3& segmentAStart,
        const Vector3& segmentAEnd,
        const Vector3& segmentBStart,
        const Vector3& segmentBEnd,
        Vector3& outPointA,
        Vector3& outPointB)
    {
        // 各線分をP(t)=start+direction*tで表し、2本の無限直線の最近接係数を求める。
        // その後、係数を0..1へ拘束して端点を含む有限線分の最近接点にする。
        const Vector3 directionA = segmentAEnd - segmentAStart;
        const Vector3 directionB = segmentBEnd - segmentBStart;
        const Vector3 offset = segmentAStart - segmentBStart;

        const float lengthASqr = directionA.LengthSqr();
        const float lengthBSqr = directionB.LengthSqr();

        const float directionDot = Vector3::Dot(directionA, directionB);

		// 線分の方向と差分ベクトルの内積を計算することで、線分同士の位置関係を導出する
        const float offsetDotA = Vector3::Dot(directionA, offset);
        const float offsetDotB = Vector3::Dot(directionB, offset);

        float parameterA = 0.0f;
        float parameterB = 0.0f;
        if (lengthASqr <= COLLISION_EPSILON && lengthBSqr <= COLLISION_EPSILON)
        {
            // 両方が点の場合は初期値のまま扱う。以降の除算も避けられる。
        }
        else if (lengthASqr <= COLLISION_EPSILON)
        {
			// Aが点の場合はBの線分上で最近接点を求める。Bの線分が点の場合は初期値のまま扱う。
            parameterB = std::clamp(offsetDotB / lengthBSqr, 0.0f, 1.0f);
        }
        else if (lengthBSqr <= COLLISION_EPSILON)
        {
            parameterA = std::clamp(-offsetDotA / lengthASqr, 0.0f, 1.0f);
        }
        else
        {
            const float denominator = lengthASqr * lengthBSqr - directionDot * directionDot;
            if (std::abs(denominator) > COLLISION_EPSILON)
            {
				// 2本の線分が平行でない場合、最近接係数を計算
				// 0,1の範囲に収めることで、線分の端点を含む最近接点を求める
                parameterA = std::clamp(
                    (directionDot * offsetDotB - offsetDotA * lengthBSqr) / denominator,
                    0.0f,
                    1.0f);
            }

			// parameterAを使ってparameterBを計算する。parameterBが0..1の範囲外の場合は、端点に拘束する。
            parameterB = (directionDot * parameterA + offsetDotB) / lengthBSqr;
            if (parameterB < 0.0f)
            {
                parameterB = 0.0f;
                parameterA = std::clamp(-offsetDotA / lengthASqr, 0.0f, 1.0f);
            }
            else if (parameterB > 1.0f)
            {
                parameterB = 1.0f;
                parameterA = std::clamp(
                    (directionDot - offsetDotA) / lengthASqr,
                    0.0f,
                    1.0f);
            }
        }

        outPointA = segmentAStart + directionA * parameterA;
        outPointB = segmentBStart + directionB * parameterB;
    }

    bool CheckCapsuleCapsule(
        const CapsuleCollider& capsuleA,
        const CapsuleCollider& capsuleB,
        Vector3& normal,
        float& penetration)
    {
        // 2本の中心線分の最近接距離が、2つのCapsule半径の和以下なら接触する。
        const CapsuleShape shapeA = GetCapsuleShape(capsuleA);
        const CapsuleShape shapeB = GetCapsuleShape(capsuleB);

        Vector3 closestPointA;
        Vector3 closestPointB;

        GetClosestPointsSegments(
            shapeA.pointA,
            shapeA.pointB,
            shapeB.pointA,
            shapeB.pointB,
            closestPointA,
            closestPointB);

        const Vector3 delta = closestPointB - closestPointA;
        const float distanceSqr = delta.LengthSqr();
        const float combinedRadius = shapeA.radius + shapeB.radius;

        if (distanceSqr > combinedRadius * combinedRadius)
        {
            return false;
        }

        const float distance = std::sqrt(distanceSqr);
        normal = distance > COLLISION_EPSILON ? delta / distance : Vector3::Right();
        penetration = combinedRadius - distance;
        return true;
    }

    bool CheckCapsuleBox(
        const CapsuleCollider& capsule,
        const BoxCollider& box,
        Vector3& normal,
        float& penetration)
    {
        // Capsuleを中心線分へ縮約し、線分とOBBの最近接距離を求める。
        // その距離がCapsule半径以下なら、線分を半径分膨らませたCapsuleとBoxが接触する。
        const CapsuleShape capsuleShape = GetCapsuleShape(capsule);
        const BoxShape boxShape = GetBoxShape(box);

        const Transform* boxTransform = box.transform();
        const Quaternion boxRotation = boxTransform
            ? boxTransform->GetRotation()
            : Quaternion::Identity();

        const Quaternion inverseBoxRotation = boxRotation.Inversed();
        const Vector3 localSegmentStart = inverseBoxRotation.RotateVector(
            capsuleShape.pointA - boxShape.center);

        const Vector3 localSegmentEnd = inverseBoxRotation.RotateVector(
            capsuleShape.pointB - boxShape.center);

        Vector3 localSegmentPoint;
        Vector3 localBoxPoint;

        GetClosestPointsSegmentBox(
            localSegmentStart,
            localSegmentEnd,
            boxShape,
            localSegmentPoint,
            localBoxPoint);

        const Vector3 segmentToBox = localBoxPoint - localSegmentPoint;
        const float distanceSqr = segmentToBox.LengthSqr();

        if (distanceSqr > capsuleShape.radius * capsuleShape.radius)
        {
            return false;
        }

        if (distanceSqr > COLLISION_EPSILON)
        {
            // 線分からBoxへ向かう方向を、A（Capsule）からB（Box）の法線にする。
            const float distance = std::sqrt(distanceSqr);
            normal = boxRotation.RotateVector(segmentToBox / distance);
            penetration = capsuleShape.radius - distance;
            return true;
        }

        const Vector3 localCapsuleCenter = inverseBoxRotation.RotateVector(
            capsuleShape.center - boxShape.center);
        Vector3 localNormal;
        float distanceToFace = 0.0f;

        // 中心線がBox内部へ入った場合は最近接面が一意でないため、Capsule中心から
        // 最短の面を選ぶ。penetrationには半径と面までの距離を足し、法線はAを外へ
        // 押し出せるようBox内部向きにする。
        GetInsideBoxNormal(boxShape, localCapsuleCenter, localNormal, distanceToFace);
        normal = boxRotation.RotateVector(localNormal);
        penetration = capsuleShape.radius + std::max(distanceToFace, 0.0f);
        return true;
    }

    float ProjectBoxRadius(const BoxShape& box, const Vector3& axis)
    {
        // OBBを軸へ投影した半径は、各Local軸の半サイズと軸の内積の絶対値の総和になる。
        float radius = 0.0f;
        for (size_t i = 0; i < box.axes.size(); ++i)
        {
            radius += box.halfExtents[i] * std::abs(Vector3::Dot(axis, box.axes[i]));
        }
        return radius;
    }

    bool CheckBoxBox(
        const BoxShape& boxA,
        const BoxShape& boxB,
        Vector3& normal,
        float& penetration)
    {
        // Separating Axis Theorem（分離軸判定）を使う。
        // 2つの投影区間が1本でも分離していれば非接触、全軸で重なれば接触している。
        // OBB同士の候補軸はAの3軸、Bの3軸、各軸の外積9本の計15本。
        const Vector3 centerDelta = boxB.center - boxA.center;
        float minimumOverlap = std::numeric_limits<float>::max();
        Vector3 minimumAxis = Vector3::Right();

		// 軸をテストするラムダ関数。重なりが最小の軸を記録する。
        auto testAxis = [&](const Vector3& candidateAxis)
        {
            // 平行な軸の外積は0ベクトルになるため、重複した候補として無視する。
            if (candidateAxis.LengthSqr() <= COLLISION_EPSILON)
            {
                return true;
            }

            const Vector3 axis = candidateAxis.Normalized();
            const float projectedA = ProjectBoxRadius(boxA, axis);
            const float projectedB = ProjectBoxRadius(boxB, axis);
            const float centerDistance = std::abs(Vector3::Dot(centerDelta, axis));
            const float overlap = projectedA + projectedB - centerDistance;
            if (overlap < 0.0f)
            {
                return false;
            }

            if (overlap < minimumOverlap)
            {
                // 最小の重なり軸が、最小押し戻しベクトル（MTV）の方向になる。
                minimumOverlap = overlap;
                minimumAxis = axis;
            }
            return true;
        };

        for (const Vector3& axis : boxA.axes)
        {
            if (!testAxis(axis))
            {
                return false;
            }
        }
        for (const Vector3& axis : boxB.axes)
        {
            if (!testAxis(axis))
            {
                return false;
            }
        }
        for (const Vector3& axisA : boxA.axes)
        {
            for (const Vector3& axisB : boxB.axes)
            {
                if (!testAxis(Vector3::Cross(axisA, axisB)))
                {
                    return false;
                }
            }
        }

        // 法線はAからBへ向ける。SATの軸は向きを持たないため、中心差で符号を決める。
        if (Vector3::Dot(centerDelta, minimumAxis) < 0.0f)
        {
            minimumAxis *= -1.0f;
        }
        normal = minimumAxis;
        penetration = minimumOverlap;
        return true;
    }
}

HitInfo Physics::CheckCollision(Collider* colliderA, Collider* colliderB)
{
    HitInfo hitInfo;
    hitInfo.colliderA = colliderA;
    hitInfo.colliderB = colliderB;
    hitInfo.objectA = colliderA ? colliderA->GetOwner() : nullptr;
    hitInfo.objectB = colliderB ? colliderB->GetOwner() : nullptr;
    hitInfo.rigidbodyA = colliderA ? colliderA->GetRigidbody() : nullptr;
    hitInfo.rigidbodyB = colliderB ? colliderB->GetRigidbody() : nullptr;

    if (!IsUsableCollider(colliderA)
        || !IsUsableCollider(colliderB)
        || colliderA == colliderB
        || colliderA->GetOwner() == colliderB->GetOwner())
    {
        // 同一Object内のCollider同士は自己衝突として無視
        return hitInfo;
    }

    Vector3 normal;
    float penetration = 0.0f;
    bool isHit = false;

    // 個別関数は法線をAからBへ返す。判定順を入れ替えて再利用する組み合わせだけ、
    // 最後に法線を反転してHitInfoの規約へ戻す。
    if (colliderA->GetShapeType() == Collider::ShapeType::Sphere
        && colliderB->GetShapeType() == Collider::ShapeType::Sphere)
    {
        const auto* sphereA = dynamic_cast<const SphereCollider*>(colliderA);
        const auto* sphereB = dynamic_cast<const SphereCollider*>(colliderB);
        isHit = sphereA && sphereB && CheckSphereSphere(*sphereA, *sphereB, normal, penetration);
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Sphere
        && colliderB->GetShapeType() == Collider::ShapeType::Box)
    {
        const auto* sphereA = dynamic_cast<const SphereCollider*>(colliderA);
        const auto* boxB = dynamic_cast<const BoxCollider*>(colliderB);
        isHit = sphereA && boxB && CheckSphereBox(*sphereA, *boxB, normal, penetration);
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Box
        && colliderB->GetShapeType() == Collider::ShapeType::Sphere)
    {
        const auto* boxA = dynamic_cast<const BoxCollider*>(colliderA);
        const auto* sphereB = dynamic_cast<const SphereCollider*>(colliderB);
        isHit = sphereB && boxA && CheckSphereBox(*sphereB, *boxA, normal, penetration);
        normal *= -1.0f;
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Capsule
        && colliderB->GetShapeType() == Collider::ShapeType::Sphere)
    {
        const auto* capsuleA = dynamic_cast<const CapsuleCollider*>(colliderA);
        const auto* sphereB = dynamic_cast<const SphereCollider*>(colliderB);
        isHit = capsuleA && sphereB && CheckCapsuleSphere(*capsuleA, *sphereB, normal, penetration);
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Sphere
        && colliderB->GetShapeType() == Collider::ShapeType::Capsule)
    {
        const auto* sphereA = dynamic_cast<const SphereCollider*>(colliderA);
        const auto* capsuleB = dynamic_cast<const CapsuleCollider*>(colliderB);
        isHit = capsuleB && sphereA && CheckCapsuleSphere(*capsuleB, *sphereA, normal, penetration);
        normal *= -1.0f;
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Box
        && colliderB->GetShapeType() == Collider::ShapeType::Box)
    {
        const auto* boxA = dynamic_cast<const BoxCollider*>(colliderA);
        const auto* boxB = dynamic_cast<const BoxCollider*>(colliderB);
        isHit = boxA && boxB && CheckBoxBox(GetBoxShape(*boxA), GetBoxShape(*boxB), normal, penetration);
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Capsule
        && colliderB->GetShapeType() == Collider::ShapeType::Capsule)
    {
        const auto* capsuleA = dynamic_cast<const CapsuleCollider*>(colliderA);
        const auto* capsuleB = dynamic_cast<const CapsuleCollider*>(colliderB);
        isHit = capsuleA && capsuleB && CheckCapsuleCapsule(*capsuleA, *capsuleB, normal, penetration);
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Capsule
        && colliderB->GetShapeType() == Collider::ShapeType::Box)
    {
        const auto* capsuleA = dynamic_cast<const CapsuleCollider*>(colliderA);
        const auto* boxB = dynamic_cast<const BoxCollider*>(colliderB);
        isHit = capsuleA && boxB && CheckCapsuleBox(*capsuleA, *boxB, normal, penetration);
    }
    else if (colliderA->GetShapeType() == Collider::ShapeType::Box
        && colliderB->GetShapeType() == Collider::ShapeType::Capsule)
    {
        const auto* boxA = dynamic_cast<const BoxCollider*>(colliderA);
        const auto* capsuleB = dynamic_cast<const CapsuleCollider*>(colliderB);
        isHit = capsuleB && boxA && CheckCapsuleBox(*capsuleB, *boxA, normal, penetration);
        normal *= -1.0f;
    }

    if (isHit)
    {
        // ここで全形状共通の正規化と退化ケースの補正を行う。
        SetHitResult(hitInfo, normal, penetration);
    }
    return hitInfo;
}

bool Physics::ResolveCollision(const HitInfo& hitInfo)
{
    if (!hitInfo.hit || !hitInfo.colliderA || !hitInfo.colliderB)
    {
        return false;
    }

    Rigidbody* rigidbodyA = hitInfo.rigidbodyA;
    Rigidbody* rigidbodyB = hitInfo.rigidbodyB;
    const float inverseMassA = rigidbodyA ? rigidbodyA->GetInverseMass() : 0.0f;
    const float inverseMassB = rigidbodyB ? rigidbodyB->GetInverseMass() : 0.0f;
    const float inverseMassSum = inverseMassA + inverseMassB;
    if (inverseMassSum <= 0.0f)
    {
        // 両方が静的なら、位置も速度も変える対象がない。
        return false;
    }

    // まずめり込みを位置補正で分離する。補正量を逆質量で按分するため、
    // 軽いObjectほど大きく動き、静的Object（逆質量0）は動かない。
    const Vector3 normal = hitInfo.normal.LengthSqr() > COLLISION_EPSILON
        ? hitInfo.normal.Normalized()
        : Vector3::Right();
    // correction = (penetration - slop) * percent / (inverseMassA + inverseMassB)
    // slopは微小な誤差による振動を抑え、percentは一度に全量補正しないための係数。
    const float correctionDistance = std::max(
        hitInfo.penetration - POSITION_CORRECTION_SLOP,
        0.0f) * POSITION_CORRECTION_PERCENT / inverseMassSum;

    if (inverseMassA > 0.0f)
    {
        if (Transform* transformA = hitInfo.colliderA->transform())
        {
            transformA->SetPosition(transformA->GetPosition() - normal * (correctionDistance * inverseMassA));
        }
    }
    if (inverseMassB > 0.0f)
    {
        if (Transform* transformB = hitInfo.colliderB->transform())
        {
            transformB->SetPosition(transformB->GetPosition() + normal * (correctionDistance * inverseMassB));
        }
    }

    const Vector3 velocityA = rigidbodyA && inverseMassA > 0.0f
        ? rigidbodyA->GetVelocity()
        : Vector3::Zero();
    const Vector3 velocityB = rigidbodyB && inverseMassB > 0.0f
        ? rigidbodyB->GetVelocity()
        : Vector3::Zero();
    const float velocityAlongNormal = Vector3::Dot(velocityB - velocityA, normal);
    if (velocityAlongNormal >= 0.0f)
    {
        // 既に離れる方向へ動いている場合、もう一度Impulseを加えると不自然に加速する。
        return true;
    }

    // 接近速度だけを反転させる法線Impulseを計算する。
    // impulse = -(1 + restitution) * relativeNormalVelocity / inverseMassSum
    // 反発係数は両者のうち大きい方を採用し、接線方向の摩擦はまだ扱わない。
    const float bouncinessA = rigidbodyA ? rigidbodyA->GetBounciness() : 0.0f;
    const float bouncinessB = rigidbodyB ? rigidbodyB->GetBounciness() : 0.0f;
    const float impulseMagnitude = -(1.0f + std::max(bouncinessA, bouncinessB))
        * velocityAlongNormal / inverseMassSum;
    const Vector3 impulse = normal * impulseMagnitude;

    if (rigidbodyA && inverseMassA > 0.0f)
    {
        rigidbodyA->SetVelocity(rigidbodyA->GetVelocity() - impulse * inverseMassA);
    }
    if (rigidbodyB && inverseMassB > 0.0f)
    {
        rigidbodyB->SetVelocity(rigidbodyB->GetVelocity() + impulse * inverseMassB);
    }
    return true;
}
