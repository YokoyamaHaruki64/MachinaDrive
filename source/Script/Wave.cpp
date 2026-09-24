#include "pch.h"
#include "Component/ComponentInc.h"
#include "Graphics/ModelManager.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/GraphicInc.h"
#include "Graphics/Material.h"

#include "Wave.h"

FIELDS_BEGIN(Wave)
    GM_FIELD(m_division);

    GM_FIELD(m_amplitude1);
    GM_FIELD(m_frequency1);
    GM_FIELD(m_speed1);
    GM_FIELD(m_direction1);

    GM_FIELD(m_amplitude2);
    GM_FIELD(m_frequency2);
    GM_FIELD(m_speed2);
    GM_FIELD(m_direction2);

    GM_FIELD(m_amplitude3);
    GM_FIELD(m_frequency3);
    GM_FIELD(m_speed3);
    GM_FIELD(m_direction3);

    GM_FIELD(m_textureScale);
FIELDS_END()

void Wave::OnStart()
{
    ModelManager::RuntimeMeshData meshData;
    std::vector<PositionVertex>& positions = meshData.positions;
    std::vector<uint32_t>& indices = meshData.indices;

    m_division = Math::Clamp(m_division, 1, 2000);
    float step = 1.0f / static_cast<float>(m_division - 1);

    for(int z = 0; z < m_division; ++z)
    {
        for(int x = 0; x < m_division; ++x)
        {

            positions.push_back({ Vector3(x * step, 0.0f, z * step) });
        }
    }

    for( int z = 0; z < m_division - 1; ++z)
    {
        for(int x = 0; x < m_division - 1; ++x)
        {
            uint32_t i0 = x + z * m_division;
            uint32_t i1 = i0 + 1;

            uint32_t i2 = x + (z + 1) * m_division;

            uint32_t i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    Material* mat = MaterialManager::Get("WaveMaterial");
    if(!mat)
    {
        GraphicsShader* shader = ShaderManager::LoadShaderFromFile("Assets/Shaders/WaveShader.shader");
        mat = MaterialManager::Create("WaveMaterial", *shader);

        if( !mat)
        {
            Logger::Error("Failed to create WaveMaterial.");
            return;
        }
    }

    mat->SetTexturePath("_MainTex", "Assets/Local/Textures/water.jpg");

    m_mesh = ModelManager::CreateRuntimeModel(
        "WaveMesh",
        meshData,
        mat
    );

    m_meshRenderer = m_owner->AddComponent<ModelRenderer>();
    m_meshRenderer->SetModel(m_mesh);
}

void Wave::Update()
{
    MaterialPropertyOverride& override = m_meshRenderer->GetMaterialPropertyOverride();

    override.SetFloat("_Amplitude1", m_amplitude1);
    override.SetFloat("_Frequency1", m_frequency1);
    override.SetFloat("_Speed1", m_speed1);
    override.SetVector2("_Direction1", m_direction1);

    override.SetFloat("_Amplitude2", m_amplitude2);
    override.SetFloat("_Frequency2", m_frequency2);
    override.SetFloat("_Speed2", m_speed2);
    override.SetVector2("_Direction2", m_direction2);

    override.SetFloat("_Amplitude3", m_amplitude3);
    override.SetFloat("_Frequency3", m_frequency3);
    override.SetFloat("_Speed3", m_speed3);
    override.SetVector2("_Direction3", m_direction3);

    override.SetFloat("_TextureWorldScale", m_textureScale);

}
