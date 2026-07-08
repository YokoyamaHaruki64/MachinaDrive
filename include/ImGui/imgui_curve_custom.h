#pragma once

#include "imgui.h"
#include "Common/Color.h"
#include "math/Curve.h"

struct ImGuiCurveEditorConfig
{
    ImVec2 editorSize;
    ImVec2 previewSize;
    ImVec2 colorPreviewSize;

    float minValue;
    float maxValue;

    int sampleCount;

    bool clampValue;
    bool showKeyInputs;
    bool showAlpha;
    bool allowAddRemoveKeys;

    // Curve<float>::Key::inControl / outControl を編集するハンドルを表示・操作します。
    // 現在の Curve は制御点の time を持たないため、ハンドルのX位置は隣接キー間の 1/3 / 2/3 に固定されます。
    bool showControlPoints;
    bool allowControlPointEdit;

    inline ImGuiCurveEditorConfig()
        : editorSize(0.0f, 180.0f)
        , previewSize(0.0f, 80.0f)
        , colorPreviewSize(0.0f, 24.0f)
        , minValue(0.0f)
        , maxValue(1.0f)
        , sampleCount(128)
        , clampValue(true)
        , showKeyInputs(true)
        , showAlpha(true)
        , allowAddRemoveKeys(true)
        , showControlPoints(true)
        , allowControlPointEdit(true)
    {
    }
};

namespace ImGui
{
    bool CurveFloat(
        const char* label,
        Curve<float>& curve,
        const ImGuiCurveEditorConfig& config = ImGuiCurveEditorConfig());

    bool CurveVector3(
        const char* label,
        ::CurveVector3& curve,
        const ImGuiCurveEditorConfig& config = ImGuiCurveEditorConfig());

    bool CurveColor(
        const char* label,
        ::CurveColor& curve,
        const ImGuiCurveEditorConfig& config = ImGuiCurveEditorConfig());
}
