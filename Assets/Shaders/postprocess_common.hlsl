#ifndef POSTPROCESS_COMMON_HLSL
#define POSTPROCESS_COMMON_HLSL

// PostProcess共通の入力画面。Material Propertyとは別に、PostProcessがt0へ設定する。
Texture2D PostProcessSource : register(t0);

float4 SamplePostProcessSource(float2 texCoord)
{
    // 通常のテクスチャはWrapを維持し、画面処理だけClampサンプラーを使う。
    return PostProcessSource.Sample(ClampSampler, texCoord);
}

#endif
