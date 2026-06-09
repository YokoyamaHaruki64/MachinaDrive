#include "pch.h"

#include "System/RendererBase.h"
#include "Component/Transform.h"
#include "System/renderer.h"

const XMMATRIX RendererBase::CalcWorldMatrix()
{
    Transform* t = transform();
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX scale, rotation, translation;
    // 回転以外の変換行列を作成
    scale = XMMatrixScaling(t->GetScale().x, t->GetScale().y, t->GetScale().z);
    translation = XMMatrixTranslation(t->GetPosition().x, t->GetPosition().y, t->GetPosition().z);
    // ビルボードの種類に応じて回転行列を作成
    switch (m_billboardType)
    {
    case BillboardType::CameraFacing:
    {
        XMMATRIX inverseView = XMMatrixInverse(nullptr, Renderer::GetViewMatrix());
        // カメラの回転を抽出してビルボードの回転行列を作成
        inverseView.r[3] = XMVectorSet(0, 0, 0, 1); // 平行移動成分をゼロにする
        rotation = inverseView;
    }
    break;
    case BillboardType::YAxisFixed:
    {
        XMMATRIX inverseView = XMMatrixInverse(nullptr, Renderer::GetViewMatrix());
        inverseView.r[3] = XMVectorSet(0, 0, 0, 1); // 平行移動成分をゼロにする
        // Y軸を固定して回転行列を作成
        inverseView.r[1] = XMVectorSet(0, 1, 0, 0); // Y軸を固定
        rotation = inverseView;
    }
    break;
    default:
        rotation = XMMatrixRotationRollPitchYaw(t->GetRotation().x, t->GetRotation().y, t->GetRotation().z);
        break;
    }
    world = scale * rotation * translation;
    return world;
}
