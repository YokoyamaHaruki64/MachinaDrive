#include "Script/EffectConfig.h"

#include "imgui.h"
#include "imgui_curve_custom.h"

#include "System/GM_time.h"

void EffectConfig::OnStart()
{
    if( m_defaultBoundBallConfig.LoadFromFile() )
        m_boundBallConfig = m_defaultBoundBallConfig;

    if( m_defaultPatternTrailConfig.LoadFromFile() )
        m_patternTrailConfig = m_defaultPatternTrailConfig;

    if( m_defaultFireWorkConfig.LoadFromFile() )
        m_fireWorkConfig = m_defaultFireWorkConfig;

    if (m_defaultImpactBurstConfig.LoadFromFile())
        m_impactBurstConfig = m_defaultImpactBurstConfig;
}

void EffectConfig::Update()
{
    float fps = 1.0f / Time::GetDeltaTime();
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("DeltaTime: %.4f", Time::GetDeltaTime());
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Global"))
    {
        float timeScale = Time::GetTimeScale();
        if (ImGui::DragFloat("TimeScale", &timeScale, 0.01f, 0.0f, 10.0f))
        {
            Time::SetTimeScale(timeScale);
        }

        if (ImGui::Button("Save", BUTTON_SIZE))
        {
            AllSaveToFile();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load", BUTTON_SIZE))
        {
            AllLoadFromFile();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", BUTTON_SIZE))
        {
            m_boundBallConfig = m_defaultBoundBallConfig;
            m_patternTrailConfig = m_defaultPatternTrailConfig;
            m_fireWorkConfig = m_defaultFireWorkConfig;
            m_impactBurstConfig = m_defaultImpactBurstConfig;
        }
        ImGui::SameLine();
        if (ImGui::Button("Default", BUTTON_SIZE))
        {
            m_defaultBoundBallConfig = m_boundBallConfig;
            m_defaultPatternTrailConfig = m_patternTrailConfig;
            m_defaultFireWorkConfig = m_fireWorkConfig;
            m_defaultImpactBurstConfig = m_impactBurstConfig;
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("Set the current configuration as the default configuration.");
            ImGui::EndTooltip();
        }

        ImGui::TreePop();
    }
    ImGui::Separator();

    if( ImGui::BeginTabBar("EffectConfig"))
    {
        if (ImGui::BeginTabItem("BallConfig"))
        {
            BoundBallConfigImGui();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("PatternTrailConfig"))
        {
            TrailConfigImGui();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem("FireWorkConfig"))
        {
            FireWorkConfigImGui();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem("ImpactBurstConfig"))
        {
            ImpactBurstConfigImGui();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void EffectConfig::BoundBallConfigImGui()
{
    // ImGuiで設定を表示
    if(ImGui::Button("Save", BUTTON_SIZE))
    {
        m_boundBallConfig.SaveToFile();
    }
    ImGui::SameLine();
    if( ImGui::Button("Load", BUTTON_SIZE))
    {
        m_boundBallConfig.LoadFromFile();
    }
    ImGui::SameLine();
    if( ImGui::Button("Reset", BUTTON_SIZE))
    {
        m_boundBallConfig = m_defaultBoundBallConfig;
    }
    ImGui::SameLine();
    if( ImGui::Button("Default", BUTTON_SIZE))
    {
        m_defaultBoundBallConfig = m_boundBallConfig;
    }
    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text("Set the current configuration as the default configuration.");
        ImGui::EndTooltip();
    }

    ImGui::PushItemWidth(100);

    ImGui::DragFloat("Bounciness", &m_boundBallConfig.bounciness, 0.01f, 0.0f, 1.0f);
    ImGui::SetNextItemWidth(200);
    ImGui::DragFloat2("Life Range", &m_boundBallConfig.lifetimeMin, 0.1f, 0.1f, FLT_MAX);
    float min = m_boundBallConfig.lifetimeMin;
    float max = m_boundBallConfig.lifetimeMax;
    if (min > max) m_boundBallConfig.lifetimeMin = max;
    if (max < min) m_boundBallConfig.lifetimeMax = min;
    ImGui::DragFloat("Gravity", &m_boundBallConfig.gravity, 0.1f, FLT_MIN, FLT_MAX);
    ImGui::PopItemWidth();
}

void EffectConfig::TrailConfigImGui()
{
    // ImGuiで設定を表示
    if (ImGui::Button("Save", BUTTON_SIZE))
    {
        m_patternTrailConfig.SaveToFile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", BUTTON_SIZE))
    {
        m_patternTrailConfig.LoadFromFile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", BUTTON_SIZE))
    {
        m_patternTrailConfig = m_defaultPatternTrailConfig;
    }
    ImGui::SameLine();
    if (ImGui::Button("Default", BUTTON_SIZE))
    {
        m_defaultPatternTrailConfig = m_patternTrailConfig;
    }
    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text("Set the current configuration as the default configuration.");
        ImGui::EndTooltip();
    }

    ImGui::PushItemWidth(100);

    ImGui::DragInt("Amount", &m_patternTrailConfig.trailCount, 0, 1, 128);
    ImGui::DragFloat("Interval", &m_patternTrailConfig.spawnInterval, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat("Width", &m_patternTrailConfig.width, 0.01f, 0.01f, 1.0f);
    ImGui::DragFloat("Length", &m_patternTrailConfig.length, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat("Speed", &m_patternTrailConfig.speed, 0.01f, 0.1f, 10.0f);
    ImGui::DragFloat("Trail Life", &m_patternTrailConfig.trailLifeTime, 0.01f, 0.1f, 10.0f);
    ImGui::PopItemWidth();

    ImGui::EditCurveVector3("Position", m_patternTrailConfig.positionCurve);
    ImGui::EditCurveColor("Color", m_patternTrailConfig.colorCurve);
    ImGui::EditCurveFloat("Width", m_patternTrailConfig.widthCurve);
}

void EffectConfig::FireWorkConfigImGui()
{
    if (ImGui::Button("Save", BUTTON_SIZE))
    {
        m_fireWorkConfig.SaveToFile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", BUTTON_SIZE))
    {
        m_fireWorkConfig.LoadFromFile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", BUTTON_SIZE))
    {
        m_fireWorkConfig = m_defaultFireWorkConfig;
    }
    ImGui::SameLine();
    if (ImGui::Button("Default", BUTTON_SIZE))
    {
        m_defaultFireWorkConfig = m_fireWorkConfig;
    }
    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text("Set the current configuration as the default configuration.");
        ImGui::EndTooltip();
    }

    ImGui::PushItemWidth(100);

    ImGui::DragInt("Burst Count", &m_fireWorkConfig.burstCount, 1, 1, 10000);
    ImGui::DragFloat("Drag", &m_fireWorkConfig.drag, 0.01f, 0.0f, 100.0f);
    ImGui::DragFloat("Gravity", &m_fireWorkConfig.gravity, 0.1f, -FLT_MAX, FLT_MAX);
    ImGui::DragFloat("Size", &m_fireWorkConfig.size, 0.01f, 0.01f, 10.0f);
    ImGui::PushItemWidth(200);

    ImGui::DragFloat2("Life Range", &m_fireWorkConfig.lifetimeMin, 0.1f, 0.1f, FLT_MAX);
    float min = m_fireWorkConfig.lifetimeMin;
    float max = m_fireWorkConfig.lifetimeMax;
    if(min > max) m_fireWorkConfig.lifetimeMin = max;
    if(max < min) m_fireWorkConfig.lifetimeMax = min;

    ImGui::DragFloat2("Speed Range", &m_fireWorkConfig.speedMin, 0.1f, 0.1f, FLT_MAX);
    min = m_fireWorkConfig.speedMin;
    max = m_fireWorkConfig.speedMax;
    if (min > max) m_fireWorkConfig.speedMin = max;
    if (max < min) m_fireWorkConfig.speedMax = min;


    ImGui::PopItemWidth();
    ImGui::PopItemWidth();

    ImGui::EditCurveColor("Color", m_fireWorkConfig.colorCurve);
    ImGui::EditCurveFloat("Scale", m_fireWorkConfig.scaleCurve);
}

void EffectConfig::ImpactBurstConfigImGui()
{
    if (ImGui::Button("Save", BUTTON_SIZE))
    {
        m_impactBurstConfig.SaveToFile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", BUTTON_SIZE))
    {
        m_impactBurstConfig.LoadFromFile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", BUTTON_SIZE))
    {
        m_impactBurstConfig = m_defaultImpactBurstConfig;
    }
    ImGui::SameLine();
    if (ImGui::Button("Default",BUTTON_SIZE))
    {
        m_defaultImpactBurstConfig = m_impactBurstConfig;
    }
    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text("Set the current configuration as the default configuration.");
        ImGui::EndTooltip();
    }

    ImGui::PushItemWidth(100);

    ImGui::DragInt("Burst Count", &m_impactBurstConfig.burstCount, 1, 1, 10000);
    ImGui::DragFloat("Drag", &m_impactBurstConfig.drag, 0.01f, 0.0f, 100.0f);
    ImGui::DragFloat("Gravity", &m_impactBurstConfig.gravity, 0.1f, -FLT_MAX, FLT_MAX);
    ImGui::DragFloat("Size", &m_impactBurstConfig.size, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("Cone Angle", &m_impactBurstConfig.coneAngle, 0.1f, 0.0f, 180.0f);

    ImGui::PushItemWidth(200);

    ImGui::DragFloat2("Life Range", &m_impactBurstConfig.lifetimeMin, 0.1f, 0.1f, FLT_MAX);
    float min = m_impactBurstConfig.lifetimeMin;
    float max = m_impactBurstConfig.lifetimeMax;
    if (min > max) m_impactBurstConfig.lifetimeMin = max;
    if (max < min) m_impactBurstConfig.lifetimeMax = min;

    ImGui::DragFloat2("Speed Range", &m_impactBurstConfig.speedMin, 0.0f, 0.1f, FLT_MAX);
    min = m_impactBurstConfig.speedMin;
    max = m_impactBurstConfig.speedMax;
    if (min > max) m_impactBurstConfig.speedMin = max;
    if (max < min) m_impactBurstConfig.speedMax = min;

    ImGui::PopItemWidth();
    ImGui::PopItemWidth();

    ImGui::EditCurveColor("Color", m_impactBurstConfig.colorCurve);
    ImGui::EditCurveFloat("Scale", m_impactBurstConfig.scaleCurve);
}

void EffectConfig::AllSaveToFile()
{
    m_boundBallConfig.SaveToFile();
    m_patternTrailConfig.SaveToFile();
    m_fireWorkConfig.SaveToFile();
    m_impactBurstConfig.SaveToFile();
}

void EffectConfig::AllLoadFromFile()
{
    m_boundBallConfig.LoadFromFile();
    m_patternTrailConfig.LoadFromFile();
    m_fireWorkConfig.LoadFromFile();
    m_impactBurstConfig.LoadFromFile();
}

bool EffectConfig::LoadLine(std::ifstream& ifs, const std::string& expectedLine)
{
    std::string line;
    ifs >> line;
    if (line != expectedLine) return false;
    return true;
}

void EffectConfig::PatternTrailConfig::SaveToFile()
{
    std::ofstream ofs("config/PatternTrailConfig.ini");

    ofs << "PositionCurve" << std::endl;
    positionCurve.Serialize(ofs);

    ofs << "ColorCurve" << std::endl;
    colorCurve.Serialize(ofs);

    ofs << "WidthCurve" << std::endl;
    widthCurve.Serialize(ofs);

    ofs << "TrailCount " << trailCount << std::endl;
    ofs << "SpawnInterval " << spawnInterval << std::endl;
    ofs << "Width " << width << std::endl;
    ofs << "Speed " << speed << std::endl;
    ofs << "TrailLifeTime " << trailLifeTime << std::endl;
    ofs << "Length " << length << std::endl;
}

bool EffectConfig::PatternTrailConfig::LoadFromFile()
{
    std::ifstream ifs("config/PatternTrailConfig.ini");

    PatternTrailConfig temp;

    if (!LoadLine(ifs, "PositionCurve")) return false;
    if (!temp.positionCurve.Deserialize(ifs)) return false;

    if (!LoadLine(ifs, "ColorCurve")) return false;
    if (!temp.colorCurve.Deserialize(ifs)) return false;

    if (!LoadLine(ifs, "WidthCurve")) return false;
    if (!temp.widthCurve.Deserialize(ifs)) return false;

    if (!LoadLine(ifs, "TrailCount")) return false;
    ifs >> temp.trailCount;

    if (!LoadLine(ifs, "SpawnInterval")) return false;
    ifs >> temp.spawnInterval;

    if (!LoadLine(ifs, "Width")) return false;
    ifs >> temp.width;

    if (!LoadLine(ifs, "Speed")) return false;
    ifs >> temp.speed;

    if (!LoadLine(ifs, "TrailLifeTime")) return false;
    ifs >> temp.trailLifeTime;

    if (!LoadLine(ifs, "Length")) return false;
    ifs >> temp.length;

    *this = temp;

    return true;
}

void EffectConfig::BoundBallConfig::SaveToFile()
{
    std::ofstream ofs("config/BoundBallConfig.ini");
    ofs << "Bounciness " << bounciness << std::endl;
    ofs << "LifetimeMin " << lifetimeMin << std::endl;
    ofs << "LifetimeMax " << lifetimeMax << std::endl;
    ofs << "Gravity " << gravity << std::endl;
}

bool EffectConfig::BoundBallConfig::LoadFromFile()
{
    std::ifstream ifs("config/BoundBallConfig.ini");
    BoundBallConfig temp;

    if (!LoadLine(ifs, "Bounciness")) return false;
    ifs >> temp.bounciness;

    if (!LoadLine(ifs, "LifetimeMin")) return false;
    ifs >> temp.lifetimeMin;

    if (!LoadLine(ifs, "LifetimeMax")) return false;
    ifs >> temp.lifetimeMax;

    if (!LoadLine(ifs, "Gravity")) return false;
    ifs >> temp.gravity;

    *this = temp;

    return true;
}

void EffectConfig::FireWorkConfig::SaveToFile()
{
    std::ofstream ofs("config/FireWorkConfig.ini");
    ofs << "BurstCount " << burstCount << std::endl;
    ofs << "LifetimeMin " << lifetimeMin << std::endl;
    ofs << "LifetimeMax " << lifetimeMax << std::endl;
    ofs << "SpeedMin " << speedMin << std::endl;
    ofs << "SpeedMax " << speedMax << std::endl;
    ofs << "Drag " << drag << std::endl;
    ofs << "Gravity " << gravity << std::endl;
    ofs << "Size " << size << std::endl;
    ofs << "ColorCurve" << std::endl;
    colorCurve.Serialize(ofs);
    ofs << "ScaleCurve" << std::endl;
    scaleCurve.Serialize(ofs);
}

bool EffectConfig::FireWorkConfig::LoadFromFile()
{
    std::ifstream ifs("config/FireWorkConfig.ini");
    FireWorkConfig temp;

    if (!LoadLine(ifs, "BurstCount")) return false;
    ifs >> temp.burstCount;

    if (!LoadLine(ifs, "LifetimeMin")) return false;
    ifs >> temp.lifetimeMin;

    if (!LoadLine(ifs, "LifetimeMax")) return false;
    ifs >> temp.lifetimeMax;

    if (!LoadLine(ifs, "SpeedMin")) return false;
    ifs >> temp.speedMin;

    if (!LoadLine(ifs, "SpeedMax")) return false;
    ifs >> temp.speedMax;

    if (!LoadLine(ifs, "Drag")) return false;
    ifs >> temp.drag;

    if (!LoadLine(ifs, "Gravity")) return false;
    ifs >> temp.gravity;

    if (!LoadLine(ifs, "Size")) return false;
    ifs >> temp.size;

    // ColorCurveの読み込み
    if (!LoadLine(ifs, "ColorCurve")) return false;
    if (!temp.colorCurve.Deserialize(ifs)) return false;
    // ScaleCurveの読み込み
    if (!LoadLine(ifs, "ScaleCurve")) return false;
    if (!temp.scaleCurve.Deserialize(ifs)) return false;
    
    *this = temp;
    
    return true;
}

void EffectConfig::ImpactBurstConfig::SaveToFile()
{
    std::ofstream ofs("config/ImpactBurstConfig.ini");
    ofs << "BurstCount " << burstCount << std::endl;
    ofs << "LifetimeMin " << lifetimeMin << std::endl;
    ofs << "LifetimeMax " << lifetimeMax << std::endl;
    ofs << "SpeedMin " << speedMin << std::endl;
    ofs << "SpeedMax " << speedMax << std::endl;
    ofs << "ConeAngle " << coneAngle << std::endl;
    ofs << "Drag " << drag << std::endl;
    ofs << "Gravity " << gravity << std::endl;
    ofs << "Size " << size << std::endl;
    ofs << "ColorCurve" << std::endl;
    colorCurve.Serialize(ofs);
    ofs << "ScaleCurve" << std::endl;
    scaleCurve.Serialize(ofs);
}

bool EffectConfig::ImpactBurstConfig::LoadFromFile()
{
    std::ifstream ifs("config/ImpactBurstConfig.ini");
    ImpactBurstConfig temp;

    if (!LoadLine(ifs, "BurstCount")) return false;
    ifs >> temp.burstCount;

    if (!LoadLine(ifs, "LifetimeMin")) return false;
    ifs >> temp.lifetimeMin;

    if (!LoadLine(ifs, "LifetimeMax")) return false;
    ifs >> temp.lifetimeMax;

    if (!LoadLine(ifs, "SpeedMin")) return false;
    ifs >> temp.speedMin;

    if (!LoadLine(ifs, "SpeedMax")) return false;
    ifs >> temp.speedMax;

    if (!LoadLine(ifs, "ConeAngle")) return false;
    ifs >> temp.coneAngle;

    if (!LoadLine(ifs, "Drag")) return false;
    ifs >> temp.drag;

    if (!LoadLine(ifs, "Gravity")) return false;
    ifs >> temp.gravity;

    if (!LoadLine(ifs, "Size")) return false;
    ifs >> temp.size;

    // ColorCurveの読み込み
    if (!LoadLine(ifs, "ColorCurve")) return false;
    if (!temp.colorCurve.Deserialize(ifs)) return false;
    // ScaleCurveの読み込み
    if (!LoadLine(ifs, "ScaleCurve")) return false;
    if (!temp.scaleCurve.Deserialize(ifs)) return false;
    
    *this = temp;

    return true;
}
