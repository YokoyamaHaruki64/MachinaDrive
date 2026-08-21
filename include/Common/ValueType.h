#pragma once

// Inspector、Serialize、Shader Propertyで共通して扱う値の型。
enum class ValueType
{
    Unknown,
    Bool,
    Int,
    Float,
    String,
    Vector2,
    Vector3,
    Vector4,
    Matrix3x3,
    Matrix4x4,
    Color,
    Texture2D,
};
