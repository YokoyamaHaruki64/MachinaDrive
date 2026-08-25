#pragma once

// ゲーム側のComponent／Script実装で直接使う定型マクロ群。
// 利用に必要な型はComponentRegistry.hまたはFieldRegistry.h経由で提供される。

// 通常は重複追加を許可しないComponentを登録する。
#define REGISTER_COMPONENT(type, displayName) \
    inline static const ComponentAutoRegistrar<type> s_componentRegistration{ #type, displayName }

#define REGISTER_COMPONENT_OPTIONS(type, displayName, allowMultiple, showInAddComponent) \
    inline static const ComponentAutoRegistrar<type> s_componentRegistration{ #type, displayName, allowMultiple, showInAddComponent }

// Scriptで共通となるComponent登録と所有者コンストラクタをまとめる。
// クラス本体の先頭でGM_SCRIPT(Type, "Display Name")として使用する。
#define GM_SCRIPT(type, displayName) \
    REGISTER_COMPONENT(type, displayName); \
public: \
    type() = delete; \
    explicit type(GameObject* owner) \
        : Script(owner) \
    { \
    }

// 重複追加やInspector表示の可否を指定するScript用のGM_SCRIPT版。
#define GM_SCRIPT_OPTION(type, displayName, allowMultiple, showInAddComponent) \
    REGISTER_COMPONENT_OPTIONS(type, displayName, allowMultiple, showInAddComponent); \
public: \
    type() = delete; \
    explicit type(GameObject* owner) \
        : Script(owner) \
    { \
    }

#define GM_FIELD(member) fields.Add(#member, member)
#define GM_FIELD_KEY(key, member) fields.Add(key, member)
#define GM_FIELD_KEY_TYPE(key, member, type) fields.Add(key, member, type)
#define GM_FIELD_TYPE(member, type) fields.Add(#member, member, type)

// Field登録フックを使うComponent／Scriptの宣言を簡潔にする。
// USE_FIELDSは宣言後にprivateへ戻す。クラス本体ではセミコロンなしで使用する。
#define USE_FIELDS \
protected: \
    void RegisterFields(FieldRegistry& fields) override; \
private:

#define FIELDS_BEGIN(type) \
void type::RegisterFields(FieldRegistry& fields) \
{

#define FIELDS_END() \
}

#define GM_FIELD_ACCESSOR(type, key, getter, setter, space) \
    fields.AddAccessor<type>(key, \
        [this] { return (getter); }, \
        [this](const type& value) { setter; }, \
        InspectorFieldSpace::space)

#define GM_FIELD_ACCESSOR_TYPE(type, key, getter, setter, space, inspectorType, serialize) \
    fields.AddAccessor<type>(key, \
        [this] { return (getter); }, \
        [this](const type& value) { setter; }, \
        InspectorFieldSpace::space, inspectorType, serialize)
