#include "imgui_curve_custom.h"

namespace
{
    enum CurveControlPoint
    {
        CurveControlPoint_None = 0,
        CurveControlPoint_In = 1,
        CurveControlPoint_Out = 2
    };

    struct CurveCanvasState
    {
        int selectedKeyIndex;
        int selectedControlPoint;

        CurveCanvasState()
            : selectedKeyIndex(-1)
            , selectedControlPoint(CurveControlPoint_None)
        {
        }
    };

    struct MultiCurveState
    {
        int selectedChannel;
        int channelCount;
        bool visible[4];

        MultiCurveState()
            : selectedChannel(0)
            , channelCount(0)
        {
            visible[0] = true;
            visible[1] = true;
            visible[2] = true;
            visible[3] = true;
        }
    };

    static std::unordered_map<ImGuiID, CurveCanvasState> g_canvasStates;
    static std::unordered_map<ImGuiID, MultiCurveState> g_multiCurveStates;

    static float ClampFloat(float value, float minValue, float maxValue)
    {
        if (value < minValue)
        {
            return minValue;
        }

        if (value > maxValue)
        {
            return maxValue;
        }

        return value;
    }

    static float SaturateFloat(float value)
    {
        return ClampFloat(value, 0.0f, 1.0f);
    }

    static int MaxInt(int a, int b)
    {
        return a > b ? a : b;
    }

    static float AbsFloat(float value)
    {
        return value < 0.0f ? -value : value;
    }

    static float LerpFloat(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    static float NormalizeValue(float value, float minValue, float maxValue)
    {
        const float range = maxValue - minValue;
        if (AbsFloat(range) <= 0.000001f)
        {
            return 0.0f;
        }

        return (value - minValue) / range;
    }

    static float DenormalizeValue(float normalized, float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * normalized;
    }

    static ImVec2 ResolveSize(ImVec2 requestedSize, float defaultHeight)
    {
        ImVec2 available = ImGui::GetContentRegionAvail();
        ImVec2 resolved = requestedSize;

        if (resolved.x <= 0.0f)
        {
            resolved.x = available.x;
        }

        if (resolved.y <= 0.0f)
        {
            resolved.y = defaultHeight;
        }

        if (resolved.x < 1.0f)
        {
            resolved.x = 1.0f;
        }

        if (resolved.y < 1.0f)
        {
            resolved.y = 1.0f;
        }

        return resolved;
    }

    static const char* FindRenderedLabelEnd(const char* label)
    {
        const char* current = label;

        if (current == nullptr)
        {
            return nullptr;
        }

        while (*current != '\0')
        {
            if (current[0] == '#' && current[1] == '#')
            {
                return current;
            }

            ++current;
        }

        return current;
    }

    static void DrawDisplayLabel(const char* label)
    {
        const char* end = FindRenderedLabelEnd(label);

        if (label == nullptr || label == end)
        {
            return;
        }

        ImGui::TextUnformatted(label, end);
    }

    static ImVec2 CurveToScreen(
        const ImVec2& origin,
        const ImVec2& size,
        float time,
        float value,
        float minValue,
        float maxValue)
    {
        float normalizedValue = NormalizeValue(value, minValue, maxValue);
        normalizedValue = SaturateFloat(normalizedValue);

        return ImVec2(
            origin.x + SaturateFloat(time) * size.x,
            origin.y + (1.0f - normalizedValue) * size.y);
    }

    static float ScreenToTime(
        const ImVec2& origin,
        const ImVec2& size,
        float screenX)
    {
        if (size.x <= 0.0f)
        {
            return 0.0f;
        }

        return SaturateFloat((screenX - origin.x) / size.x);
    }

    static float ScreenToValue(
        const ImVec2& origin,
        const ImVec2& size,
        float screenY,
        float minValue,
        float maxValue,
        bool clampValue)
    {
        float normalizedValue = 1.0f;

        if (size.y > 0.0f)
        {
            normalizedValue = 1.0f - ((screenY - origin.y) / size.y);
        }

        if (clampValue)
        {
            normalizedValue = SaturateFloat(normalizedValue);
        }

        return DenormalizeValue(normalizedValue, minValue, maxValue);
    }

    static int GetKeyCount(const Curve<float>& curve)
    {
        return static_cast<int>(curve.GetKeyCount());
    }

    static int FindNearestKeyIndex(
        const Curve<float>& curve,
        float time,
        float value)
    {
        const std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());

        if (keyCount <= 0)
        {
            return -1;
        }

        int bestIndex = 0;
        float bestScore = AbsFloat(keys[0].time - time) + AbsFloat(keys[0].value - value);

        for (int i = 1; i < keyCount; ++i)
        {
            const float score = AbsFloat(keys[i].time - time) + AbsFloat(keys[i].value - value);
            if (score < bestScore)
            {
                bestScore = score;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    static int FindKeyAtPosition(
        const Curve<float>& curve,
        const ImVec2& mousePosition,
        const ImVec2& origin,
        const ImVec2& size,
        float minValue,
        float maxValue,
        float radius)
    {
        const std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());
        const float radiusSqr = radius * radius;

        int bestIndex = -1;
        float bestDistanceSqr = radiusSqr;

        for (int i = 0; i < keyCount; ++i)
        {
            const ImVec2 keyPosition = CurveToScreen(
                origin,
                size,
                keys[i].time,
                keys[i].value,
                minValue,
                maxValue);

            const float dx = keyPosition.x - mousePosition.x;
            const float dy = keyPosition.y - mousePosition.y;
            const float distanceSqr = dx * dx + dy * dy;

            if (distanceSqr <= bestDistanceSqr)
            {
                bestDistanceSqr = distanceSqr;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    static bool GetControlPointScreenPosition(
        const Curve<float>& curve,
        int keyIndex,
        int controlPoint,
        const ImVec2& origin,
        const ImVec2& size,
        float minValue,
        float maxValue,
        ImVec2* outPosition)
    {
        const std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());

        if (outPosition == nullptr)
        {
            return false;
        }

        if (keyIndex < 0 || keyIndex >= keyCount)
        {
            return false;
        }

        if (controlPoint == CurveControlPoint_In)
        {
            if (keyIndex <= 0)
            {
                return false;
            }

            const Curve<float>::Key& previousKey = keys[keyIndex - 1];
            const Curve<float>::Key& key = keys[keyIndex];
            const float handleTime = LerpFloat(previousKey.time, key.time, 2.0f / 3.0f);

            *outPosition = CurveToScreen(
                origin,
                size,
                handleTime,
                key.inControl,
                minValue,
                maxValue);

            return true;
        }

        if (controlPoint == CurveControlPoint_Out)
        {
            if (keyIndex + 1 >= keyCount)
            {
                return false;
            }

            const Curve<float>::Key& key = keys[keyIndex];
            const Curve<float>::Key& nextKey = keys[keyIndex + 1];
            const float handleTime = LerpFloat(key.time, nextKey.time, 1.0f / 3.0f);

            *outPosition = CurveToScreen(
                origin,
                size,
                handleTime,
                key.outControl,
                minValue,
                maxValue);

            return true;
        }

        return false;
    }

    static int FindControlPointAtPosition(
        const Curve<float>& curve,
        int keyIndex,
        const ImVec2& mousePosition,
        const ImVec2& origin,
        const ImVec2& size,
        float minValue,
        float maxValue,
        float radius)
    {
        const float radiusSqr = radius * radius;
        int bestControlPoint = CurveControlPoint_None;
        float bestDistanceSqr = radiusSqr;

        ImVec2 handlePosition;

        if (GetControlPointScreenPosition(
            curve,
            keyIndex,
            CurveControlPoint_In,
            origin,
            size,
            minValue,
            maxValue,
            &handlePosition))
        {
            const float dx = handlePosition.x - mousePosition.x;
            const float dy = handlePosition.y - mousePosition.y;
            const float distanceSqr = dx * dx + dy * dy;

            if (distanceSqr <= bestDistanceSqr)
            {
                bestDistanceSqr = distanceSqr;
                bestControlPoint = CurveControlPoint_In;
            }
        }

        if (GetControlPointScreenPosition(
            curve,
            keyIndex,
            CurveControlPoint_Out,
            origin,
            size,
            minValue,
            maxValue,
            &handlePosition))
        {
            const float dx = handlePosition.x - mousePosition.x;
            const float dy = handlePosition.y - mousePosition.y;
            const float distanceSqr = dx * dx + dy * dy;

            if (distanceSqr <= bestDistanceSqr)
            {
                bestDistanceSqr = distanceSqr;
                bestControlPoint = CurveControlPoint_Out;
            }
        }

        return bestControlPoint;
    }

    static int SetKeyTimeValue(
        Curve<float>& curve,
        int keyIndex,
        float time,
        float value,
        const ImGuiCurveEditorConfig& config)
    {
        std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());

        if (keyIndex < 0 || keyIndex >= keyCount)
        {
            return -1;
        }

        Curve<float>::Key key = keys[keyIndex];

        if (config.clampValue)
        {
            value = ClampFloat(value, config.minValue, config.maxValue);
        }

        const float oldValue = key.value;
        const float deltaValue = value - oldValue;

        key.time = SaturateFloat(time);
        key.value = value;
        key.inControl += deltaValue;
        key.outControl += deltaValue;

        if (config.clampValue)
        {
            key.inControl = ClampFloat(key.inControl, config.minValue, config.maxValue);
            key.outControl = ClampFloat(key.outControl, config.minValue, config.maxValue);
        }

        curve.SetKey(static_cast<std::size_t>(keyIndex), key);

        return FindNearestKeyIndex(curve, key.time, key.value);
    }

    static int SetKeyControlValue(
        Curve<float>& curve,
        int keyIndex,
        int controlPoint,
        float value,
        const ImGuiCurveEditorConfig& config)
    {
        std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());

        if (keyIndex < 0 || keyIndex >= keyCount)
        {
            return -1;
        }

        if (controlPoint != CurveControlPoint_In && controlPoint != CurveControlPoint_Out)
        {
            return keyIndex;
        }

        if (config.clampValue)
        {
            value = ClampFloat(value, config.minValue, config.maxValue);
        }

        Curve<float>::Key key = keys[keyIndex];

        if (controlPoint == CurveControlPoint_In)
        {
            key.inControl = value;
        }
        else if (controlPoint == CurveControlPoint_Out)
        {
            key.outControl = value;
        }

        curve.SetKey(static_cast<std::size_t>(keyIndex), key);

        return FindNearestKeyIndex(curve, key.time, key.value);
    }

    static int ResetKeyControlValue(
        Curve<float>& curve,
        int keyIndex,
        int controlPoint,
        const ImGuiCurveEditorConfig& config)
    {
        const std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());

        if (keyIndex < 0 || keyIndex >= keyCount)
        {
            return -1;
        }

        return SetKeyControlValue(
            curve,
            keyIndex,
            controlPoint,
            keys[keyIndex].value,
            config);
    }

    static bool RemoveKey(Curve<float>& curve, int keyIndex)
    {
        if (keyIndex < 0 || keyIndex >= GetKeyCount(curve))
        {
            return false;
        }

        curve.RemoveKey(static_cast<std::size_t>(keyIndex));
        return true;
    }

    static void DrawCanvasBackground(
        ImDrawList* drawList,
        const ImVec2& origin,
        const ImVec2& size)
    {
        const ImVec2 bottomRight(origin.x + size.x, origin.y + size.y);

        const ImU32 backgroundColor = IM_COL32(35, 35, 35, 255);
        const ImU32 borderColor = IM_COL32(120, 120, 120, 255);
        const ImU32 gridColor = IM_COL32(255, 255, 255, 28);
        const ImU32 centerColor = IM_COL32(255, 255, 255, 55);

        drawList->AddRectFilled(origin, bottomRight, backgroundColor);

        for (int i = 1; i < 4; ++i)
        {
            const float tx = static_cast<float>(i) / 4.0f;
            const float ty = static_cast<float>(i) / 4.0f;

            const float x = origin.x + size.x * tx;
            const float y = origin.y + size.y * ty;

            drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, bottomRight.y), gridColor);
            drawList->AddLine(ImVec2(origin.x, y), ImVec2(bottomRight.x, y), gridColor);
        }

        const float centerY = origin.y + size.y * 0.5f;
        drawList->AddLine(ImVec2(origin.x, centerY), ImVec2(bottomRight.x, centerY), centerColor);
        drawList->AddRect(origin, bottomRight, borderColor);
    }

    static void DrawCurveLine(
        ImDrawList* drawList,
        const Curve<float>& curve,
        const ImVec2& origin,
        const ImVec2& size,
        float minValue,
        float maxValue,
        int sampleCount,
        ImU32 color,
        float thickness)
    {
        if (curve.GetKeyCount() <= 0)
        {
            return;
        }

        sampleCount = MaxInt(sampleCount, 2);

        ImVec2 previous = CurveToScreen(
            origin,
            size,
            0.0f,
            curve.Evaluate(0.0f),
            minValue,
            maxValue);

        for (int i = 1; i <= sampleCount; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(sampleCount);
            const float value = curve.Evaluate(t);
            const ImVec2 current = CurveToScreen(origin, size, t, value, minValue, maxValue);

            drawList->AddLine(previous, current, color, thickness);
            previous = current;
        }
    }

    static void DrawSelectedControlPoints(
        ImDrawList* drawList,
        const Curve<float>& curve,
        const ImVec2& origin,
        const ImVec2& size,
        float minValue,
        float maxValue,
        int selectedKeyIndex,
        int selectedControlPoint)
    {
        const std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());

        if (selectedKeyIndex < 0 || selectedKeyIndex >= keyCount)
        {
            return;
        }

        const Curve<float>::Key& selectedKey = keys[selectedKeyIndex];
        const ImVec2 keyPosition = CurveToScreen(
            origin,
            size,
            selectedKey.time,
            selectedKey.value,
            minValue,
            maxValue);

        const ImU32 lineColor = IM_COL32(255, 255, 255, 95);
        const ImU32 inFillColor = selectedControlPoint == CurveControlPoint_In
            ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(255, 165, 80, 255);
        const ImU32 outFillColor = selectedControlPoint == CurveControlPoint_Out
            ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(80, 210, 255, 255);
        const ImU32 outlineColor = IM_COL32(0, 0, 0, 230);

        ImVec2 handlePosition;

        if (GetControlPointScreenPosition(
            curve,
            selectedKeyIndex,
            CurveControlPoint_In,
            origin,
            size,
            minValue,
            maxValue,
            &handlePosition))
        {
            drawList->AddLine(keyPosition, handlePosition, lineColor, 1.0f);
            drawList->AddRectFilled(
                ImVec2(handlePosition.x - 5.0f, handlePosition.y - 5.0f),
                ImVec2(handlePosition.x + 5.0f, handlePosition.y + 5.0f),
                outlineColor);
            drawList->AddRectFilled(
                ImVec2(handlePosition.x - 4.0f, handlePosition.y - 4.0f),
                ImVec2(handlePosition.x + 4.0f, handlePosition.y + 4.0f),
                inFillColor);
        }

        if (GetControlPointScreenPosition(
            curve,
            selectedKeyIndex,
            CurveControlPoint_Out,
            origin,
            size,
            minValue,
            maxValue,
            &handlePosition))
        {
            drawList->AddLine(keyPosition, handlePosition, lineColor, 1.0f);
            drawList->AddRectFilled(
                ImVec2(handlePosition.x - 5.0f, handlePosition.y - 5.0f),
                ImVec2(handlePosition.x + 5.0f, handlePosition.y + 5.0f),
                outlineColor);
            drawList->AddRectFilled(
                ImVec2(handlePosition.x - 4.0f, handlePosition.y - 4.0f),
                ImVec2(handlePosition.x + 4.0f, handlePosition.y + 4.0f),
                outFillColor);
        }
    }

    static void DrawCurveKeys(
        ImDrawList* drawList,
        const Curve<float>& curve,
        const ImVec2& origin,
        const ImVec2& size,
        float minValue,
        float maxValue,
        int selectedKeyIndex,
        ImU32 color)
    {
        const std::vector<Curve<float>::Key>& keys = curve.GetKeys();
        const int keyCount = static_cast<int>(keys.size());

        for (int i = 0; i < keyCount; ++i)
        {
            const ImVec2 position = CurveToScreen(
                origin,
                size,
                keys[i].time,
                keys[i].value,
                minValue,
                maxValue);

            const bool selected = i == selectedKeyIndex;
            const float radius = selected ? 5.0f : 4.0f;
            const ImU32 fillColor = selected ? IM_COL32(255, 255, 255, 255) : color;
            const ImU32 outlineColor = IM_COL32(0, 0, 0, 220);

            drawList->AddCircleFilled(position, radius + 1.0f, outlineColor);
            drawList->AddCircleFilled(position, radius, fillColor);
        }
    }

    static CurveCanvasState& GetCanvasState(ImGuiID id)
    {
        std::unordered_map<ImGuiID, CurveCanvasState>::iterator it = g_canvasStates.find(id);

        if (it == g_canvasStates.end())
        {
            CurveCanvasState state;
            std::pair<std::unordered_map<ImGuiID, CurveCanvasState>::iterator, bool> result =
                g_canvasStates.insert(std::make_pair(id, state));
            return result.first->second;
        }

        return it->second;
    }

    static MultiCurveState& GetMultiCurveState(ImGuiID id, int channelCount)
    {
        std::unordered_map<ImGuiID, MultiCurveState>::iterator it = g_multiCurveStates.find(id);

        if (it == g_multiCurveStates.end())
        {
            MultiCurveState state;
            state.channelCount = channelCount;

            for (int i = 0; i < 4; ++i)
            {
                state.visible[i] = i < channelCount;
            }

            std::pair<std::unordered_map<ImGuiID, MultiCurveState>::iterator, bool> result =
                g_multiCurveStates.insert(std::make_pair(id, state));
            return result.first->second;
        }

        MultiCurveState& state = it->second;
        state.channelCount = channelCount;

        if (state.selectedChannel < 0)
        {
            state.selectedChannel = 0;
        }

        if (state.selectedChannel >= channelCount)
        {
            state.selectedChannel = channelCount - 1;
        }

        for (int i = channelCount; i < 4; ++i)
        {
            state.visible[i] = false;
        }

        return state;
    }

    static bool DrawChannelCombo(
        const char* label,
        int* selectedChannel,
        const char* const* channelNames,
        int channelCount)
    {
        bool changed = false;

        if (selectedChannel == nullptr || channelNames == nullptr || channelCount <= 0)
        {
            return false;
        }

        if (*selectedChannel < 0)
        {
            *selectedChannel = 0;
        }

        if (*selectedChannel >= channelCount)
        {
            *selectedChannel = channelCount - 1;
        }

        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::BeginCombo(label, channelNames[*selectedChannel]))
        {
            for (int i = 0; i < channelCount; ++i)
            {
                const bool selected = i == *selectedChannel;
                if (ImGui::Selectable(channelNames[i], selected))
                {
                    *selectedChannel = i;
                    changed = true;
                }

                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        return changed;
    }

    static void DrawCurvePreview(
        const char* id,
        const Curve<float>* const* curves,
        const bool* visible,
        const ImU32* colors,
        const char* const* channelNames,
        int curveCount,
        const ImGuiCurveEditorConfig& config)
    {
        ImGui::PushID(id);

        const ImVec2 size = ResolveSize(config.previewSize, 80.0f);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 bottomRight(origin.x + size.x, origin.y + size.y);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::Dummy(size);

        DrawCanvasBackground(drawList, origin, size);

        for (int i = 0; i < curveCount; ++i)
        {
            if (visible != nullptr && !visible[i])
            {
                continue;
            }

            if (curves[i] == nullptr)
            {
                continue;
            }

            DrawCurveLine(
                drawList,
                *curves[i],
                origin,
                size,
                config.minValue,
                config.maxValue,
                config.sampleCount,
                colors[i],
                2.0f);
        }

        float labelX = origin.x + 6.0f;
        const float labelY = origin.y + 5.0f;
        for (int i = 0; i < curveCount; ++i)
        {
            if (visible != nullptr && !visible[i])
            {
                continue;
            }

            if (channelNames == nullptr)
            {
                continue;
            }

            drawList->AddText(ImVec2(labelX, labelY), colors[i], channelNames[i]);
            labelX += ImGui::CalcTextSize(channelNames[i]).x + 10.0f;
        }

        drawList->AddRect(origin, bottomRight, IM_COL32(120, 120, 120, 255));

        ImGui::PopID();
    }

    static void DrawCheckerboard(
        ImDrawList* drawList,
        const ImVec2& origin,
        const ImVec2& size,
        float cellSize)
    {
        const ImU32 colorA = IM_COL32(95, 95, 95, 255);
        const ImU32 colorB = IM_COL32(145, 145, 145, 255);

        int yIndex = 0;
        for (float y = origin.y; y < origin.y + size.y; y += cellSize)
        {
            int xIndex = 0;
            for (float x = origin.x; x < origin.x + size.x; x += cellSize)
            {
                const bool even = ((xIndex + yIndex) % 2) == 0;
                const ImU32 color = even ? colorA : colorB;

                const ImVec2 p0(x, y);
                const ImVec2 p1(
                    ClampFloat(x + cellSize, origin.x, origin.x + size.x),
                    ClampFloat(y + cellSize, origin.y, origin.y + size.y));

                drawList->AddRectFilled(p0, p1, color);
                ++xIndex;
            }

            ++yIndex;
        }
    }

    static ImU32 ColorToImU32(const Color& color, bool useAlpha)
    {
        float r = SaturateFloat(color.r);
        float g = SaturateFloat(color.g);
        float b = SaturateFloat(color.b);
        float a = useAlpha ? SaturateFloat(color.a) : 1.0f;

        return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a));
    }

    static void DrawColorGradientPreview(
        const char* id,
        const ::CurveColor& curve,
        const ImGuiCurveEditorConfig& config)
    {
        ImGui::PushID(id);

        const ImVec2 size = ResolveSize(config.colorPreviewSize, 24.0f);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 bottomRight(origin.x + size.x, origin.y + size.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::Dummy(size);

        DrawCheckerboard(drawList, origin, size, 8.0f);

        const int steps = MaxInt(config.sampleCount, 2);
        for (int i = 0; i < steps; ++i)
        {
            const float t0 = static_cast<float>(i) / static_cast<float>(steps);
            const float t1 = static_cast<float>(i + 1) / static_cast<float>(steps);
            const Color color = curve.Evaluate(t0);
            const ImU32 imColor = ColorToImU32(color, config.showAlpha);

            const float x0 = origin.x + size.x * t0;
            const float x1 = origin.x + size.x * t1;

            drawList->AddRectFilled(
                ImVec2(x0, origin.y),
                ImVec2(x1 + 1.0f, bottomRight.y),
                imColor);
        }

        drawList->AddRect(origin, bottomRight, IM_COL32(120, 120, 120, 255));

        ImGui::PopID();
    }

    static bool DrawCurveEditor1D(
        const char* id,
        Curve<float>& curve,
        ImU32 color,
        const ImGuiCurveEditorConfig& config)
    {
        bool changed = false;

        ImGui::PushID(id);

        const ImVec2 size = ResolveSize(config.editorSize, 180.0f);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 bottomRight(origin.x + size.x, origin.y + size.y);

        ImGuiID canvasId = ImGui::GetID("canvas");
        CurveCanvasState& state = GetCanvasState(canvasId);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("canvas", size);

        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();
        const ImVec2 mousePosition = ImGui::GetIO().MousePos;

        const int keyCountBefore = GetKeyCount(curve);
        if (state.selectedKeyIndex >= keyCountBefore)
        {
            state.selectedKeyIndex = keyCountBefore - 1;
            state.selectedControlPoint = CurveControlPoint_None;
        }

        if (state.selectedKeyIndex < -1)
        {
            state.selectedKeyIndex = -1;
            state.selectedControlPoint = CurveControlPoint_None;
        }

        if (state.selectedKeyIndex < 0)
        {
            state.selectedControlPoint = CurveControlPoint_None;
        }

        if (hovered && ImGui::IsMouseClicked(0))
        {
            int controlPoint = CurveControlPoint_None;

            if (config.showControlPoints && config.allowControlPointEdit && state.selectedKeyIndex >= 0)
            {
                controlPoint = FindControlPointAtPosition(
                    curve,
                    state.selectedKeyIndex,
                    mousePosition,
                    origin,
                    size,
                    config.minValue,
                    config.maxValue,
                    8.0f);
            }

            if (controlPoint != CurveControlPoint_None)
            {
                state.selectedControlPoint = controlPoint;
            }
            else
            {
                const int keyIndex = FindKeyAtPosition(
                    curve,
                    mousePosition,
                    origin,
                    size,
                    config.minValue,
                    config.maxValue,
                    8.0f);

                if (keyIndex != state.selectedKeyIndex)
                {
                    state.selectedControlPoint = CurveControlPoint_None;
                }

                state.selectedKeyIndex = keyIndex;
            }
        }

        if (hovered &&
            config.allowAddRemoveKeys &&
            state.selectedControlPoint == CurveControlPoint_None &&
            ImGui::IsMouseDoubleClicked(0))
        {
            const int existingKeyIndex = FindKeyAtPosition(
                curve,
                mousePosition,
                origin,
                size,
                config.minValue,
                config.maxValue,
                8.0f);

            if (existingKeyIndex >= 0)
            {
                state.selectedKeyIndex = existingKeyIndex;
                state.selectedControlPoint = CurveControlPoint_None;
            }
            else
            {
                const float time = ScreenToTime(origin, size, mousePosition.x);
                float value = ScreenToValue(
                    origin,
                    size,
                    mousePosition.y,
                    config.minValue,
                    config.maxValue,
                    config.clampValue);

                if (config.clampValue)
                {
                    value = ClampFloat(value, config.minValue, config.maxValue);
                }

                curve.AddKey(value, time);
                state.selectedKeyIndex = FindNearestKeyIndex(curve, time, value);
                state.selectedControlPoint = CurveControlPoint_None;
                changed = true;
            }
        }

        if (active && state.selectedKeyIndex >= 0 && ImGui::IsMouseDragging(0))
        {
            if (state.selectedControlPoint != CurveControlPoint_None &&
                config.showControlPoints &&
                config.allowControlPointEdit)
            {
                const float controlValue = ScreenToValue(
                    origin,
                    size,
                    mousePosition.y,
                    config.minValue,
                    config.maxValue,
                    config.clampValue);

                state.selectedKeyIndex = SetKeyControlValue(
                    curve,
                    state.selectedKeyIndex,
                    state.selectedControlPoint,
                    controlValue,
                    config);

                changed = true;
            }
            else
            {
                const float time = ScreenToTime(origin, size, mousePosition.x);
                const float value = ScreenToValue(
                    origin,
                    size,
                    mousePosition.y,
                    config.minValue,
                    config.maxValue,
                    config.clampValue);

                state.selectedKeyIndex = SetKeyTimeValue(
                    curve,
                    state.selectedKeyIndex,
                    time,
                    value,
                    config);

                state.selectedControlPoint = CurveControlPoint_None;
                changed = true;
            }
        }

        if (hovered && ImGui::IsMouseClicked(1))
        {
            int resetControlPoint = CurveControlPoint_None;

            if (config.showControlPoints && config.allowControlPointEdit && state.selectedKeyIndex >= 0)
            {
                resetControlPoint = FindControlPointAtPosition(
                    curve,
                    state.selectedKeyIndex,
                    mousePosition,
                    origin,
                    size,
                    config.minValue,
                    config.maxValue,
                    8.0f);
            }

            if (resetControlPoint != CurveControlPoint_None)
            {
                state.selectedKeyIndex = ResetKeyControlValue(
                    curve,
                    state.selectedKeyIndex,
                    resetControlPoint,
                    config);
                state.selectedControlPoint = resetControlPoint;
                changed = true;
            }
            else if (config.allowAddRemoveKeys)
            {
                const int keyIndex = FindKeyAtPosition(
                    curve,
                    mousePosition,
                    origin,
                    size,
                    config.minValue,
                    config.maxValue,
                    8.0f);

                if (RemoveKey(curve, keyIndex))
                {
                    state.selectedKeyIndex = -1;
                    state.selectedControlPoint = CurveControlPoint_None;
                    changed = true;
                }
            }
        }

        if (config.allowAddRemoveKeys && state.selectedKeyIndex >= 0)
        {
            const bool deletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace);
            if (deletePressed && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                if (RemoveKey(curve, state.selectedKeyIndex))
                {
                    state.selectedKeyIndex = -1;
                    state.selectedControlPoint = CurveControlPoint_None;
                    changed = true;
                }
            }
        }

        DrawCanvasBackground(drawList, origin, size);
        DrawCurveLine(
            drawList,
            curve,
            origin,
            size,
            config.minValue,
            config.maxValue,
            config.sampleCount,
            color,
            2.0f);

        if (config.showControlPoints)
        {
            DrawSelectedControlPoints(
                drawList,
                curve,
                origin,
                size,
                config.minValue,
                config.maxValue,
                state.selectedKeyIndex,
                state.selectedControlPoint);
        }

        DrawCurveKeys(
            drawList,
            curve,
            origin,
            size,
            config.minValue,
            config.maxValue,
            state.selectedKeyIndex,
            color);

        if (GetKeyCount(curve) <= 0)
        {
            const char* text = "Double click to add key";
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            const ImVec2 textPos(
                origin.x + (size.x - textSize.x) * 0.5f,
                origin.y + (size.y - textSize.y) * 0.5f);
            drawList->AddText(textPos, IM_COL32(255, 255, 255, 120), text);
        }

        drawList->AddRect(origin, bottomRight, IM_COL32(120, 120, 120, 255));

        if (config.showKeyInputs)
        {
            const int currentKeyCount = GetKeyCount(curve);
            if (state.selectedKeyIndex >= 0 && state.selectedKeyIndex < currentKeyCount)
            {
                Curve<float>::Key key = curve.GetKeys()[state.selectedKeyIndex];
                float time = key.time;
                float value = key.value;
                bool inputChanged = false;

                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::DragFloat("Time", &time, 0.001f, 0.0f, 1.0f, "%.3f"))
                {
                    inputChanged = true;
                }

                ImGui::SameLine();

                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::DragFloat("Value", &value, 0.01f, config.minValue, config.maxValue, "%.3f"))
                {
                    inputChanged = true;
                }

                if (inputChanged)
                {
                    state.selectedKeyIndex = SetKeyTimeValue(curve, state.selectedKeyIndex, time, value, config);
                    state.selectedControlPoint = CurveControlPoint_None;
                    changed = true;
                }

                const int updatedKeyCount = GetKeyCount(curve);
                if (config.showControlPoints &&
                    config.allowControlPointEdit &&
                    state.selectedKeyIndex >= 0 &&
                    state.selectedKeyIndex < updatedKeyCount)
                {
                    Curve<float>::Key updatedKey = curve.GetKeys()[state.selectedKeyIndex];
                    float inControl = updatedKey.inControl;
                    float outControl = updatedKey.outControl;

                    if (state.selectedKeyIndex > 0)
                    {
                        ImGui::SetNextItemWidth(120.0f);
                        if (ImGui::DragFloat("In Control", &inControl, 0.01f, config.minValue, config.maxValue, "%.3f"))
                        {
                            state.selectedKeyIndex = SetKeyControlValue(
                                curve,
                                state.selectedKeyIndex,
                                CurveControlPoint_In,
                                inControl,
                                config);
                            state.selectedControlPoint = CurveControlPoint_In;
                            changed = true;
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("In Control: none");
                    }

                    ImGui::SameLine();

                    if (state.selectedKeyIndex >= 0 && state.selectedKeyIndex + 1 < GetKeyCount(curve))
                    {
                        if (state.selectedKeyIndex < GetKeyCount(curve))
                        {
                            outControl = curve.GetKeys()[state.selectedKeyIndex].outControl;
                        }

                        ImGui::SetNextItemWidth(120.0f);
                        if (ImGui::DragFloat("Out Control", &outControl, 0.01f, config.minValue, config.maxValue, "%.3f"))
                        {
                            state.selectedKeyIndex = SetKeyControlValue(
                                curve,
                                state.selectedKeyIndex,
                                CurveControlPoint_Out,
                                outControl,
                                config);
                            state.selectedControlPoint = CurveControlPoint_Out;
                            changed = true;
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("Out Control: none");
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("No key selected");
            }
        }

        ImGui::PopID();
        return changed;
    }
}

namespace ImGui
{
    bool CurveFloat(
        const char* label,
        Curve<float>& curve,
        const ImGuiCurveEditorConfig& config)
    {
        bool changed = false;

        ImGui::PushID(label);
        DrawDisplayLabel(label);

        const ImU32 color = IM_COL32(255, 210, 80, 255);
        changed |= DrawCurveEditor1D("float_editor", curve, color, config);

        ImGui::PopID();
        return changed;
    }

    bool CurveVector3(
        const char* label,
        ::CurveVector3& curve,
        const ImGuiCurveEditorConfig& config)
    {
        bool changed = false;

        ImGui::PushID(label);
        DrawDisplayLabel(label);

        const char* channelNames[3] = { "X", "Y", "Z" };
        const char* checkboxNames[3] = { "X", "Y", "Z" };
        const ImU32 colors[3] =
        {
            IM_COL32(255, 80, 80, 255),
            IM_COL32(80, 255, 80, 255),
            IM_COL32(80, 160, 255, 255)
        };

        MultiCurveState& state = GetMultiCurveState(ImGui::GetID("vector3_state"), 3);

        DrawChannelCombo("channel", &state.selectedChannel, channelNames, 3);

        for (int i = 0; i < 3; ++i)
        {
            ImGui::SameLine();
            ImGui::Checkbox(checkboxNames[i], &state.visible[i]);
        }

        const Curve<float>* previewCurves[3] = { &curve.x, &curve.y, &curve.z };
        DrawCurvePreview(
            "vector3_preview",
            previewCurves,
            state.visible,
            colors,
            channelNames,
            3,
            config);

        ImGui::Spacing();

        Curve<float>* editCurves[3] = { &curve.x, &curve.y, &curve.z };
        changed |= DrawCurveEditor1D(
            "vector3_editor",
            *editCurves[state.selectedChannel],
            colors[state.selectedChannel],
            config);

        ImGui::PopID();
        return changed;
    }

    bool CurveColor(
        const char* label,
        ::CurveColor& curve,
        const ImGuiCurveEditorConfig& config)
    {
        bool changed = false;

        ImGui::PushID(label);
        DrawDisplayLabel(label);

        const int channelCount = config.showAlpha ? 4 : 3;
        const char* channelNames[4] = { "R", "G", "B", "A" };
        const char* checkboxNames[4] = { "R", "G", "B", "A" };
        const ImU32 colors[4] =
        {
            IM_COL32(255, 80, 80, 255),
            IM_COL32(80, 255, 80, 255),
            IM_COL32(80, 160, 255, 255),
            IM_COL32(220, 220, 220, 255)
        };

        MultiCurveState& state = GetMultiCurveState(ImGui::GetID("color_state"), channelCount);

        DrawChannelCombo("channel", &state.selectedChannel, channelNames, channelCount);

        for (int i = 0; i < channelCount; ++i)
        {
            ImGui::SameLine();
            ImGui::Checkbox(checkboxNames[i], &state.visible[i]);
        }

        DrawColorGradientPreview("color_gradient", curve, config);
        ImGui::Spacing();

        const Curve<float>* previewCurves[4] = { &curve.r, &curve.g, &curve.b, &curve.a };
        DrawCurvePreview(
            "color_curve_preview",
            previewCurves,
            state.visible,
            colors,
            channelNames,
            channelCount,
            config);

        ImGui::Spacing();

        Curve<float>* editCurves[4] = { &curve.r, &curve.g, &curve.b, &curve.a };
        changed |= DrawCurveEditor1D(
            "color_editor",
            *editCurves[state.selectedChannel],
            colors[state.selectedChannel],
            config);

        ImGui::PopID();
        return changed;
    }
}
