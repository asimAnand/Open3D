// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "GuiVisualizer.h"

#include "Open3D/GUI/Application.h"
#include "Open3D/GUI/Button.h"
#include "Open3D/GUI/Checkbox.h"
#include "Open3D/GUI/Color.h"
#include "Open3D/GUI/ColorEdit.h"
#include "Open3D/GUI/Combobox.h"
#include "Open3D/GUI/Dialog.h"
#include "Open3D/GUI/FileDialog.h"
#include "Open3D/GUI/Label.h"
#include "Open3D/GUI/Layout.h"
#include "Open3D/GUI/SceneWidget.h"
#include "Open3D/GUI/Slider.h"
#include "Open3D/GUI/Theme.h"
#include "Open3D/GUI/VectorEdit.h"
#include "Open3D/Geometry/BoundingVolume.h"
#include "Open3D/Geometry/Image.h"
#include "Open3D/Geometry/PointCloud.h"
#include "Open3D/Geometry/TriangleMesh.h"
#include "Open3D/IO/ClassIO/ImageIO.h"
#include "Open3D/IO/ClassIO/PointCloudIO.h"
#include "Open3D/IO/ClassIO/TriangleMeshIO.h"
#include "Open3D/Open3DConfig.h"
#include "Open3D/Utility/Console.h"
#include "Open3D/Utility/FileSystem.h"
#include "Open3D/Visualization/Rendering/Filament/FilamentResourceManager.h"
#include "Open3D/Visualization/Rendering/RenderToBuffer.h"
#include "Open3D/Visualization/Rendering/RendererStructs.h"
#include "Open3D/Visualization/Rendering/Scene.h"

#define LOAD_IN_NEW_WINDOW 0

namespace open3d {
namespace visualization {

namespace {

std::shared_ptr<gui::Dialog> createAboutDialog(gui::Window *window) {
    auto &theme = window->GetTheme();
    auto dlg = std::make_shared<gui::Dialog>("About");

    auto title = std::make_shared<gui::Label>(
            (std::string("Open3D ") + OPEN3D_VERSION).c_str());
    auto text = std::make_shared<gui::Label>(
            "The MIT License (MIT)\n"
            "Copyright (c) 2018 - 2020 www.open3d.org\n\n"

            "Permission is hereby granted, free of charge, to any person "
            "obtaining "
            "a copy of this software and associated documentation files (the "
            "\"Software\"), to deal in the Software without restriction, "
            "including "
            "without limitation the rights to use, copy, modify, merge, "
            "publish, "
            "distribute, sublicense, and/or sell copies of the Software, and "
            "to "
            "permit persons to whom the Software is furnished to do so, "
            "subject to "
            "the following conditions:\n\n"

            "The above copyright notice and this permission notice shall be "
            "included in all copies or substantial portions of the "
            "Software.\n\n"

            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, "
            "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES "
            "OF "
            "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND "
            "NONINFRINGEMENT. "
            "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR "
            "ANY "
            "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF "
            "CONTRACT, "
            "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE "
            "SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");
    auto ok = std::make_shared<gui::Button>("OK");
    ok->SetOnClicked([window]() { window->CloseDialog(); });

    gui::Margins margins(theme.fontSize);
    auto layout = std::make_shared<gui::Vert>(0, margins);
    layout->AddChild(gui::Horiz::MakeCentered(title));
    layout->AddFixed(theme.fontSize);
    layout->AddChild(text);
    layout->AddFixed(theme.fontSize);
    layout->AddChild(gui::Horiz::MakeCentered(ok));
    dlg->AddChild(layout);

    return dlg;
}

std::shared_ptr<gui::VGrid> createHelpDisplay(gui::Window *window) {
    auto &theme = window->GetTheme();

    gui::Margins margins(theme.fontSize);
    auto layout = std::make_shared<gui::VGrid>(2, 0, margins);
    layout->SetBackgroundColor(gui::Color(0, 0, 0, 0.5));

    auto addLabel = [layout](const char *text) {
        auto label = std::make_shared<gui::Label>(text);
        label->SetTextColor(gui::Color(1, 1, 1));
        layout->AddChild(label);
    };
    auto addRow = [layout, &addLabel](const char *left, const char *right) {
        addLabel(left);
        addLabel(right);
    };

    addRow("Arcball mode", " ");
    addRow("Left-drag", "Rotate camera");
    addRow("Shift + left-drag    ", "Forward/backward");

#if defined(__APPLE__)
    addLabel("Cmd + left-drag");
#else
    addLabel("Ctrl + left-drag");
#endif  // __APPLE__
    addLabel("Pan camera");

#if defined(__APPLE__)
    addLabel("Opt + left-drag (up/down)");
#else
    addLabel("Win + left-drag (up/down)");
#endif  // __APPLE__
    addLabel("Rotate around forward axis");

#if defined(__APPLE__)
    addLabel("Ctrl + left-drag");
#else
    addLabel("Alt + left-drag");
#endif  // __APPLE__
    addLabel("Rotate directional light");

    addRow("Right-drag", "Pan camera");
    addRow("Middle-drag", "Rotate directional light");
    addRow("Wheel", "Forward/backward");
    addRow("Shift + Wheel", "Change field of view");
    addRow("", "");

    addRow("Fly mode", " ");
    addRow("Left-drag", "Rotate camera");
#if defined(__APPLE__)
    addLabel("Opt + left-drag");
#else
    addLabel("Win + left-drag");
#endif  // __APPLE__
    addLabel("Rotate around forward axis");
    addRow("W", "Forward");
    addRow("S", "Backward");
    addRow("A", "Step left");
    addRow("D", "Step right");
    addRow("Q", "Step up");
    addRow("Z", "Step down");
    addRow("E", "Roll left");
    addRow("R", "Roll right");
    addRow("Up", "Look up");
    addRow("Down", "Look down");
    addRow("Left", "Look left");
    addRow("Right", "Look right");

    return layout;
}

std::shared_ptr<gui::Dialog> createContactDialog(gui::Window *window) {
    auto &theme = window->GetTheme();
    auto em = theme.fontSize;
    auto dlg = std::make_shared<gui::Dialog>("Contact Us");

    auto title = std::make_shared<gui::Label>("Contact Us");
    auto leftCol = std::make_shared<gui::Label>(
            "Web site:\n"
            "Code:\n"
            "Mailing list:\n"
            "Discord channel:");
    auto rightCol = std::make_shared<gui::Label>(
            "http://www.open3d.org\n"
            "http://github.org/intel-isl/Open3D\n"
            "http://www.open3d.org/index.php/subscribe/\n"
            "https://discord.gg/D35BGvn");
    auto ok = std::make_shared<gui::Button>("OK");
    ok->SetOnClicked([window]() { window->CloseDialog(); });

    gui::Margins margins(em);
    auto layout = std::make_shared<gui::Vert>(0, margins);
    layout->AddChild(gui::Horiz::MakeCentered(title));
    layout->AddFixed(em);

    auto columns = std::make_shared<gui::Horiz>(em, gui::Margins());
    columns->AddChild(leftCol);
    columns->AddChild(rightCol);
    layout->AddChild(columns);

    layout->AddFixed(em);
    layout->AddChild(gui::Horiz::MakeCentered(ok));
    dlg->AddChild(layout);

    return dlg;
}

std::shared_ptr<geometry::TriangleMesh> CreateAxes(double axisLength) {
    const double sphereRadius = 0.005 * axisLength;
    const double cylRadius = 0.0025 * axisLength;
    const double coneRadius = 0.0075 * axisLength;
    const double cylHeight = 0.975 * axisLength;
    const double coneHeight = 0.025 * axisLength;

    auto mesh_frame = geometry::TriangleMesh::CreateSphere(sphereRadius);
    mesh_frame->ComputeVertexNormals();
    mesh_frame->PaintUniformColor(Eigen::Vector3d(0.5, 0.5, 0.5));

    std::shared_ptr<geometry::TriangleMesh> mesh_arrow;
    Eigen::Matrix4d transformation;

    mesh_arrow = geometry::TriangleMesh::CreateArrow(cylRadius, coneRadius,
                                                     cylHeight, coneHeight);
    mesh_arrow->ComputeVertexNormals();
    mesh_arrow->PaintUniformColor(Eigen::Vector3d(1.0, 0.0, 0.0));
    transformation << 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1;
    mesh_arrow->Transform(transformation);
    *mesh_frame += *mesh_arrow;

    mesh_arrow = geometry::TriangleMesh::CreateArrow(cylRadius, coneRadius,
                                                     cylHeight, coneHeight);
    mesh_arrow->ComputeVertexNormals();
    mesh_arrow->PaintUniformColor(Eigen::Vector3d(0.0, 1.0, 0.0));
    transformation << 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1;
    mesh_arrow->Transform(transformation);
    *mesh_frame += *mesh_arrow;

    mesh_arrow = geometry::TriangleMesh::CreateArrow(cylRadius, coneRadius,
                                                     cylHeight, coneHeight);
    mesh_arrow->ComputeVertexNormals();
    mesh_arrow->PaintUniformColor(Eigen::Vector3d(0.0, 0.0, 1.0));
    transformation << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1;
    mesh_arrow->Transform(transformation);
    *mesh_frame += *mesh_arrow;

    return mesh_frame;
}

struct SmartMode {
    static bool PointCloudHasUniformColor(const geometry::PointCloud &pcd) {
        if (!pcd.HasColors()) {
            return true;
        }

        static const double e = 1.0 / 255.0;
        static const double kSqEpsilon = Eigen::Vector3d(e, e, e).squaredNorm();
        const auto &color = pcd.colors_[0];

        for (const auto &c : pcd.colors_) {
            if ((color - c).squaredNorm() > kSqEpsilon) {
                return false;
            }
        }

        return true;
    }
};

std::shared_ptr<gui::Slider> MakeSlider(const gui::Slider::Type type,
                                        const double min,
                                        const double max,
                                        const double value) {
    auto slider = std::make_shared<gui::Slider>(type);
    slider->SetLimits(min, max);
    slider->SetValue(value);
    return slider;
}

//----
class DrawTimeLabel : public gui::Label {
    using Super = Label;

public:
    DrawTimeLabel(gui::Window *w) : Label("0.0 ms") { window_ = w; }

    gui::Size CalcPreferredSize(const gui::Theme &theme) const override {
        auto h = Super::CalcPreferredSize(theme).height;
        return gui::Size(theme.fontSize * 5, h);
    }

    DrawResult Draw(const gui::DrawContext &context) override {
        char text[64];
        // double ms = window_->GetLastFrameTimeSeconds() * 1000.0;
        double ms = 0.0;
        snprintf(text, sizeof(text) - 1, "%.1f ms", ms);
        SetText(text);

        return Super::Draw(context);
    }

private:
    gui::Window *window_;
};

//----
class SmallButton : public gui::Button {
    using Super = Button;

public:
    explicit SmallButton(const char *title) : Button(title) {}

    gui::Size CalcPreferredSize(const gui::Theme &theme) const override {
        auto em = theme.fontSize;
        auto size = Super::CalcPreferredSize(theme);
        return gui::Size(size.width - em, 1.2 * em);
    }
};

//----
class SmallToggleButton : public SmallButton {
    using Super = SmallButton;

public:
    explicit SmallToggleButton(const char *title) : SmallButton(title) {
        SetToggleable(true);
    }
};

}  // namespace

struct LightingProfile {
    std::string name;
    double iblIntensity;
    double sunIntensity;
    Eigen::Vector3f sunDir;
    Eigen::Vector3f sunColor = {1.0f, 1.0f, 1.0f};
    Scene::Transform iblRotation = Scene::Transform::Identity();
    bool iblEnabled = true;
    bool useDefaultIBL = false;
    bool sunEnabled = true;
};

static const std::string kDefaultIBL = "default";
static const std::string kDefaultMaterialName = "Polished ceramic [default]";
static const std::string kPointCloudProfileName = "Cloudy day (no direct sun)";
static const bool kDefaultShowSkybox = false;
static const bool kDefaultShowAxes = false;

static const std::vector<LightingProfile> gLightingProfiles = {
        {.name = "Bright day with sun at +Y [default]",
         .iblIntensity = 100000,
         .sunIntensity = 100000,
         .sunDir = {0.577f, -0.577f, -0.577f}},
        {.name = "Bright day with sun at -Y",
         .iblIntensity = 100000,
         .sunIntensity = 100000,
         .sunDir = {0.577f, 0.577f, 0.577f},
         .sunColor = {1.0f, 1.0f, 1.0f},
         .iblRotation = Scene::Transform(
                 Eigen::AngleAxisf(M_PI, Eigen::Vector3f::UnitX()))},
        {.name = "Bright day with sun at +Z",
         .iblIntensity = 100000,
         .sunIntensity = 100000,
         .sunDir = {0.577f, 0.577f, -0.577f}},
        {.name = "Less bright day with sun at +Y",
         .iblIntensity = 75000,
         .sunIntensity = 100000,
         .sunDir = {0.577f, -0.577f, -0.577f}},
        {.name = "Less bright day with sun at -Y",
         .iblIntensity = 75000,
         .sunIntensity = 100000,
         .sunDir = {0.577f, 0.577f, 0.577f},
         .sunColor = {1.0f, 1.0f, 1.0f},
         .iblRotation = Scene::Transform(
                 Eigen::AngleAxisf(M_PI, Eigen::Vector3f::UnitX()))},
        {.name = "Less bright day with sun at +Z",
         .iblIntensity = 75000,
         .sunIntensity = 100000,
         .sunDir = {0.577f, 0.577f, -0.577f}},
        {.name = kPointCloudProfileName,
         .iblIntensity = 60000,
         .sunIntensity = 100000,
         .sunDir = {0.577f, -0.577f, -0.577f},
         .sunColor = {1.0f, 1.0f, 1.0f},
         .iblRotation = Scene::Transform::Identity(),
         .iblEnabled = true,
         .useDefaultIBL = true,
         .sunEnabled = false}};

enum MenuId {
    FILE_OPEN,
    FILE_EXPORT_RGB,
    FILE_CLOSE,
    SETTINGS_LIGHT_AND_MATERIALS,
    HELP_KEYS,
    HELP_ABOUT,
    HELP_CONTACT
};

struct GuiVisualizer::Impl {
    std::vector<visualization::GeometryHandle> geometryHandles;

    std::shared_ptr<gui::SceneWidget> scene;
    std::shared_ptr<gui::VGrid> helpKeys;

    struct LitMaterial {
        visualization::MaterialInstanceHandle handle;
        Eigen::Vector3f baseColor = {0.9f, 0.9f, 0.9f};
        float metallic = 0.f;
        float roughness = 0.7;
        float reflectance = 0.5f;
        float clearCoat = 0.2f;
        float clearCoatRoughness = 0.2f;
        float anisotropy = 0.f;
        float pointSize = 5.f;
    };

    struct UnlitMaterial {
        visualization::MaterialInstanceHandle handle;
        Eigen::Vector3f baseColor = {1.f, 1.f, 1.f};
        float pointSize = 5.f;
    };

    struct Materials {
        LitMaterial lit;
        UnlitMaterial unlit;
    };

    std::map<std::string, LitMaterial> prefabMaterials = {
            {kDefaultMaterialName, {}},
            {"Aluminum",
             {visualization::MaterialInstanceHandle::kBad,
              {0.913f, 0.921f, 0.925f},
              1.0f,
              0.5f,
              0.9f,
              0.0f,
              0.0f,
              0.0f,
              3.0f}},
            {"Gold",
             {visualization::MaterialInstanceHandle::kBad,
              {1.000f, 0.766f, 0.336f},
              1.0f,
              0.3f,
              0.9f,
              0.0f,
              0.0f,
              0.0f,
              3.0f}},
            {"Copper",
             {visualization::MaterialInstanceHandle::kBad,
              {0.955f, 0.637f, 0.538f},
              1.0f,
              0.3f,
              0.9f,
              0.0f,
              0.0f,
              0.0f,
              3.0f}},
            {"Iron",
             {visualization::MaterialInstanceHandle::kBad,
              {0.560f, 0.570f, 0.580f},
              1.0f,
              0.5f,
              0.9f,
              0.0f,
              0.0f,
              0.0f,
              3.0f}},
            {"Plastic (white)",
             {visualization::MaterialInstanceHandle::kBad,
              {1.0f, 1.0f, 1.0f},
              0.0f,
              0.5f,
              0.5f,
              0.5f,
              0.2f,
              0.0f,
              3.0f}},
            {"Glazed ceramic (white)",
             {visualization::MaterialInstanceHandle::kBad,
              {1.0f, 1.0f, 1.0f},
              0.0f,
              0.5f,
              0.9f,
              1.0f,
              0.1f,
              0.0f,
              3.0f}},
            {"Clay",
             {visualization::MaterialInstanceHandle::kBad,
              {0.7725f, 0.7725f, 0.7725f},
              0.0f,
              1.0f,
              0.5f,
              0.1f,
              0.287f,
              0.0f,
              3.0f}},
    };

    std::unordered_map<visualization::REHandle_abstract, Materials>
            geometryMaterials;

    visualization::MaterialHandle hLitMaterial;
    visualization::MaterialHandle hUnlitMaterial;

    struct Settings {
        visualization::IndirectLightHandle hIbl;
        visualization::SkyboxHandle hSky;
        visualization::TextureHandle hSkyTexture;
        visualization::LightHandle hDirectionalLight;
        visualization::GeometryHandle hAxes;

        std::shared_ptr<gui::Vert> wgtBase;
        std::shared_ptr<gui::Checkbox> wgtShowAxes;
        std::shared_ptr<gui::ColorEdit> wgtBGColor;
        std::shared_ptr<gui::Button> wgtMouseArcball;
        std::shared_ptr<gui::Button> wgtMouseFly;
        std::shared_ptr<gui::Button> wgtMouseSun;
        std::shared_ptr<gui::Button> wgtMouseIBL;
        std::shared_ptr<gui::Button> wgtMouseModel;
        std::shared_ptr<gui::Combobox> wgtLightingProfile;
        std::shared_ptr<gui::CollapsableVert> wgtAdvanced;
        std::shared_ptr<gui::Checkbox> wgtIBLEnabled;
        std::shared_ptr<gui::Checkbox> wgtSkyEnabled;
        std::shared_ptr<gui::Checkbox> wgtDirectionalEnabled;
        std::shared_ptr<gui::Combobox> wgtIBLs;
        std::shared_ptr<gui::Button> wgtLoadSky;
        std::shared_ptr<gui::Slider> wgtIBLIntensity;
        std::shared_ptr<gui::Slider> wgtSunIntensity;
        std::shared_ptr<gui::VectorEdit> wgtSunDir;
        std::shared_ptr<gui::ColorEdit> wgtSunColor;

        enum MaterialType {
            LIT = 0,
            UNLIT,
            NORMAL_MAP,
            DEPTH,
        };

        MaterialType selectedType = LIT;
        std::shared_ptr<gui::Combobox> wgtMaterialType;

        std::shared_ptr<gui::Combobox> wgtPrefabMaterial;
        std::shared_ptr<gui::Slider> wgtPointSize;

        void SetCustomProfile() {
            wgtLightingProfile->SetSelectedIndex(gLightingProfiles.size());
        }

        void SetMaterialSelected(const MaterialType type) {
            wgtMaterialType->SetSelectedIndex(type);
            wgtPrefabMaterial->SetEnabled(type == MaterialType::LIT);
        }
    } settings;

    static void SetMaterialsDefaults(Materials &materials,
                                     visualization::Renderer &renderer) {
        materials.lit.handle =
                renderer.ModifyMaterial(materials.lit.handle)
                        .SetColor("baseColor", materials.lit.baseColor)
                        .SetParameter("roughness", materials.lit.roughness)
                        .SetParameter("metallic", materials.lit.metallic)
                        .SetParameter("reflectance", materials.lit.reflectance)
                        .SetParameter("clearCoat", materials.lit.clearCoat)
                        .SetParameter("clearCoatRoughness",
                                      materials.lit.clearCoatRoughness)
                        .SetParameter("anisotropy", materials.lit.anisotropy)
                        .SetParameter("pointSize", materials.lit.pointSize)
                        .Finish();

        materials.unlit.handle =
                renderer.ModifyMaterial(materials.unlit.handle)
                        .SetColor("baseColor", materials.unlit.baseColor)
                        .SetParameter("pointSize", materials.unlit.pointSize)
                        .Finish();
    }

    void SetLightingProfile(visualization::Renderer &renderer,
                            const std::string &name) {
        for (size_t i = 0; i < gLightingProfiles.size(); ++i) {
            if (gLightingProfiles[i].name == name) {
                SetLightingProfile(renderer, gLightingProfiles[i]);
                this->settings.wgtLightingProfile->SetSelectedValue(
                        name.c_str());
                return;
            }
        }
        utility::LogWarning("Could not find lighting profile '{}'", name);
    }

    void SetLightingProfile(visualization::Renderer &renderer,
                            const LightingProfile &profile) {
        auto *renderScene = this->scene->GetScene();
        if (profile.useDefaultIBL) {
            this->SetIBL(renderer, nullptr);
            this->settings.wgtIBLs->SetSelectedValue(kDefaultIBL.c_str());
        }
        if (profile.iblEnabled) {
            renderScene->SetIndirectLight(this->settings.hIbl);
        } else {
            renderScene->SetIndirectLight(IndirectLightHandle());
        }
        renderScene->SetIndirectLightIntensity(profile.iblIntensity);
        renderScene->SetIndirectLightRotation(profile.iblRotation);
        renderScene->SetSkybox(SkyboxHandle());
        renderScene->SetEntityEnabled(this->settings.hDirectionalLight,
                                      profile.sunEnabled);
        renderScene->SetLightIntensity(this->settings.hDirectionalLight,
                                       profile.sunIntensity);
        renderScene->SetLightDirection(this->settings.hDirectionalLight,
                                       profile.sunDir);
        renderScene->SetLightColor(this->settings.hDirectionalLight,
                                   profile.sunColor);
        this->settings.wgtIBLEnabled->SetChecked(profile.iblEnabled);
        this->settings.wgtSkyEnabled->SetChecked(false);
        this->settings.wgtDirectionalEnabled->SetChecked(profile.sunEnabled);
        this->settings.wgtIBLs->SetSelectedValue(kDefaultIBL.c_str());
        this->settings.wgtIBLIntensity->SetValue(profile.iblIntensity);
        this->settings.wgtSunIntensity->SetValue(profile.sunIntensity);
        this->settings.wgtSunDir->SetValue(profile.sunDir);
        this->settings.wgtSunColor->SetValue(gui::Color(
                profile.sunColor[0], profile.sunColor[1], profile.sunColor[2]));
    }

    bool SetIBL(visualization::Renderer &renderer, const char *path) {
        visualization::IndirectLightHandle newIBL;
        std::string iblPath;
        if (path) {
            newIBL = renderer.AddIndirectLight(ResourceLoadRequest(path));
            iblPath = path;
        } else {
            iblPath =
                    std::string(
                            gui::Application::GetInstance().GetResourcePath()) +
                    "/" + kDefaultIBL + "_ibl.ktx";
            newIBL = renderer.AddIndirectLight(
                    ResourceLoadRequest(iblPath.c_str()));
        }
        if (newIBL) {
            auto *renderScene = this->scene->GetScene();
            this->settings.hIbl = newIBL;
            auto intensity = renderScene->GetIndirectLightIntensity();
            renderScene->SetIndirectLight(newIBL);
            renderScene->SetIndirectLightIntensity(intensity);

            auto skyboxPath = std::string(iblPath);
            if (skyboxPath.find("_ibl.ktx") != std::string::npos) {
                skyboxPath = skyboxPath.substr(0, skyboxPath.size() - 8);
                skyboxPath += "_skybox.ktx";
                this->settings.hSky = renderer.AddSkybox(
                        ResourceLoadRequest(skyboxPath.c_str()));
                if (!this->settings.hSky) {
                    this->settings.hSky = renderer.AddSkybox(
                            ResourceLoadRequest(iblPath.c_str()));
                }
                bool isOn = this->settings.wgtSkyEnabled->IsChecked();
                if (isOn) {
                    this->scene->GetScene()->SetSkybox(this->settings.hSky);
                }
                this->scene->SetSkyboxHandle(this->settings.hSky, isOn);
            }
            return true;
        }
        return false;
    }
};

GuiVisualizer::GuiVisualizer(
        const std::vector<std::shared_ptr<const geometry::Geometry>>
                &geometries,
        const std::string &title,
        int width,
        int height,
        int left,
        int top)
    : gui::Window(title, left, top, width, height),
      impl_(new GuiVisualizer::Impl()) {
    auto &app = gui::Application::GetInstance();
    auto &theme = GetTheme();

    // Create menu
    if (!gui::Application::GetInstance().GetMenubar()) {
        auto fileMenu = std::make_shared<gui::Menu>();
        fileMenu->AddItem("Open...", "Ctrl-O", FILE_OPEN);
        fileMenu->AddItem("Export Current Image...", nullptr, FILE_EXPORT_RGB);
        fileMenu->AddSeparator();
        fileMenu->AddItem("Close", "Ctrl-W", FILE_CLOSE);
        auto helpMenu = std::make_shared<gui::Menu>();
        helpMenu->AddItem("Show Controls", nullptr, HELP_KEYS);
        helpMenu->AddSeparator();
        helpMenu->AddItem("About", nullptr, HELP_ABOUT);
        helpMenu->AddItem("Contact", nullptr, HELP_CONTACT);
        auto settingsMenu = std::make_shared<gui::Menu>();
        settingsMenu->AddItem("Lighting & Materials", nullptr,
                              SETTINGS_LIGHT_AND_MATERIALS);
        settingsMenu->SetChecked(SETTINGS_LIGHT_AND_MATERIALS, true);
        auto menu = std::make_shared<gui::Menu>();
        menu->AddMenu("File", fileMenu);
        menu->AddMenu("Settings", settingsMenu);
#if defined(__APPLE__) && GUI_USE_NATIVE_MENUS
        // macOS adds a special search item to menus named "Help",
        // so add a space to avoid that.
        menu->AddMenu("Help ", helpMenu);
#else
        menu->AddMenu("Help", helpMenu);
#endif
        gui::Application::GetInstance().SetMenubar(menu);
    }

    // Create scene
    auto sceneId = GetRenderer().CreateScene();
    auto scene = std::make_shared<gui::SceneWidget>(
            *GetRenderer().GetScene(sceneId));
    auto renderScene = scene->GetScene();
    impl_->scene = scene;
    scene->SetBackgroundColor(gui::Color(1.0, 1.0, 1.0));

    // Create light
    const int defaultLightingProfileIdx = 0;
    auto &lightingProfile = gLightingProfiles[defaultLightingProfileIdx];
    visualization::LightDescription lightDescription;
    lightDescription.intensity = lightingProfile.sunIntensity;
    lightDescription.direction = lightingProfile.sunDir;
    lightDescription.castShadows = true;
    lightDescription.customAttributes["custom_type"] = "SUN";

    impl_->settings.hDirectionalLight =
            scene->GetScene()->AddLight(lightDescription);

    auto &settings = impl_->settings;
    std::string rsrcPath = app.GetResourcePath();
    auto iblPath = rsrcPath + "/default_ibl.ktx";
    settings.hIbl =
            GetRenderer().AddIndirectLight(ResourceLoadRequest(iblPath.data()));
    renderScene->SetIndirectLight(settings.hIbl);
    renderScene->SetIndirectLightIntensity(lightingProfile.iblIntensity);
    renderScene->SetIndirectLightRotation(lightingProfile.iblRotation);

    auto skyPath = rsrcPath + "/" + kDefaultIBL + "_skybox.ktx";
    settings.hSky =
            GetRenderer().AddSkybox(ResourceLoadRequest(skyPath.data()));
    scene->SetSkyboxHandle(settings.hSky, kDefaultShowSkybox);

    // Create materials
    auto litPath = rsrcPath + "/defaultLit.filamat";
    impl_->hLitMaterial = GetRenderer().AddMaterial(
            visualization::ResourceLoadRequest(litPath.data()));

    auto unlitPath = rsrcPath + "/defaultUnlit.filamat";
    impl_->hUnlitMaterial = GetRenderer().AddMaterial(
            visualization::ResourceLoadRequest(unlitPath.data()));

    // Setup UI
    const auto em = theme.fontSize;
    const int lm = std::ceil(0.5 * em);
    const int gridSpacing = std::ceil(0.25 * em);

    auto drawTimeLabel = std::make_shared<DrawTimeLabel>(this);
    drawTimeLabel->SetTextColor(gui::Color(0.5, 0.5, 0.5));

    AddChild(scene);

    // Add settings widget
    const int separationHeight = std::ceil(em);
    settings.wgtBase = std::make_shared<gui::Vert>(0, gui::Margins(lm));

    settings.wgtLoadSky = std::make_shared<SmallButton>("Load skybox");
    settings.wgtLoadSky->SetOnClicked([this, renderScene]() {
        auto dlg = std::make_shared<gui::FileDialog>(
                gui::FileDialog::Type::OPEN, "Open skybox", GetTheme());
        dlg->AddFilter(".ktx", "Khronos Texture (.ktx)");
        dlg->SetOnCancel([this]() { this->CloseDialog(); });
        dlg->SetOnDone([this, renderScene](const char *path) {
            this->CloseDialog();
            auto newSky = GetRenderer().AddSkybox(ResourceLoadRequest(path));
            if (newSky) {
                impl_->settings.hSky = newSky;
                impl_->settings.wgtSkyEnabled->SetChecked(true);
                impl_->settings.SetCustomProfile();

                renderScene->SetSkybox(newSky);
                impl_->scene->SetSkyboxHandle(newSky, true);
            }
        });
        ShowDialog(dlg);
    });

    gui::Margins indent(em, 0, 0, 0);
    auto viewCtrls =
            std::make_shared<gui::CollapsableVert>("View controls", 0, indent);

    // ... view manipulator buttons
    settings.wgtMouseArcball = std::make_shared<SmallToggleButton>("Arcball");
    this->impl_->settings.wgtMouseArcball->SetOn(true);
    settings.wgtMouseArcball->SetOnClicked([this]() {
        this->impl_->scene->SetViewControls(
                gui::SceneWidget::Controls::ROTATE_OBJ);
        this->SetTickEventsEnabled(false);
        this->impl_->settings.wgtMouseArcball->SetOn(true);
        this->impl_->settings.wgtMouseFly->SetOn(false);
        this->impl_->settings.wgtMouseSun->SetOn(false);
        this->impl_->settings.wgtMouseIBL->SetOn(false);
        this->impl_->settings.wgtMouseModel->SetOn(false);
    });
    settings.wgtMouseFly = std::make_shared<SmallToggleButton>("Fly");
    settings.wgtMouseFly->SetOnClicked([this]() {
        this->impl_->scene->SetViewControls(gui::SceneWidget::Controls::FPS);
        this->SetFocusWidget(this->impl_->scene.get());
        this->SetTickEventsEnabled(true);
        this->impl_->settings.wgtMouseArcball->SetOn(false);
        this->impl_->settings.wgtMouseFly->SetOn(true);
        this->impl_->settings.wgtMouseSun->SetOn(false);
        this->impl_->settings.wgtMouseIBL->SetOn(false);
        this->impl_->settings.wgtMouseModel->SetOn(false);
    });
    settings.wgtMouseModel = std::make_shared<SmallToggleButton>("Model");
    settings.wgtMouseModel->SetOnClicked([this]() {
        this->impl_->scene->SetViewControls(
                gui::SceneWidget::Controls::ROTATE_MODEL);
        this->SetTickEventsEnabled(false);
        this->impl_->settings.wgtMouseArcball->SetOn(false);
        this->impl_->settings.wgtMouseFly->SetOn(false);
        this->impl_->settings.wgtMouseSun->SetOn(false);
        this->impl_->settings.wgtMouseIBL->SetOn(false);
        this->impl_->settings.wgtMouseModel->SetOn(true);
    });
    settings.wgtMouseSun = std::make_shared<SmallToggleButton>("Sun");
    settings.wgtMouseSun->SetOnClicked([this]() {
        this->impl_->scene->SetViewControls(
                gui::SceneWidget::Controls::ROTATE_SUN);
        this->SetTickEventsEnabled(false);
        this->impl_->settings.wgtMouseArcball->SetOn(false);
        this->impl_->settings.wgtMouseFly->SetOn(false);
        this->impl_->settings.wgtMouseSun->SetOn(true);
        this->impl_->settings.wgtMouseIBL->SetOn(false);
        this->impl_->settings.wgtMouseModel->SetOn(false);
    });
    settings.wgtMouseIBL = std::make_shared<SmallToggleButton>("Environment");
    settings.wgtMouseIBL->SetOnClicked([this]() {
        this->impl_->scene->SetViewControls(
                gui::SceneWidget::Controls::ROTATE_IBL);
        this->SetTickEventsEnabled(false);
        this->impl_->settings.wgtMouseArcball->SetOn(false);
        this->impl_->settings.wgtMouseFly->SetOn(false);
        this->impl_->settings.wgtMouseSun->SetOn(false);
        this->impl_->settings.wgtMouseIBL->SetOn(true);
        this->impl_->settings.wgtMouseModel->SetOn(false);
    });

    auto cameraControls = std::make_shared<gui::Horiz>(gridSpacing);
    cameraControls->AddStretch();
    cameraControls->AddChild(settings.wgtMouseArcball);
    cameraControls->AddChild(settings.wgtMouseFly);
    cameraControls->AddChild(settings.wgtMouseModel);
    cameraControls->AddFixed(em);
    cameraControls->AddChild(settings.wgtMouseSun);
    cameraControls->AddChild(settings.wgtMouseIBL);
    cameraControls->AddStretch();
    viewCtrls->AddChild(std::make_shared<gui::Label>("Mouse Controls"));
    viewCtrls->AddChild(cameraControls);

    // ... background
    settings.wgtSkyEnabled = std::make_shared<gui::Checkbox>("Show skymap");
    settings.wgtSkyEnabled->SetChecked(kDefaultShowSkybox);
    settings.wgtSkyEnabled->SetOnChecked([this, renderScene](bool checked) {
        if (checked) {
            renderScene->SetSkybox(impl_->settings.hSky);
        } else {
            renderScene->SetSkybox(SkyboxHandle());
        }
        impl_->scene->SetSkyboxHandle(impl_->settings.hSky, checked);
        impl_->settings.wgtBGColor->SetEnabled(!checked);
    });

    impl_->settings.wgtBGColor = std::make_shared<gui::ColorEdit>();
    impl_->settings.wgtBGColor->SetValue({1, 1, 1});
    impl_->settings.wgtBGColor->OnValueChanged =
            [scene](const gui::Color &newColor) {
                scene->SetBackgroundColor(newColor);
            };
    auto bgLayout = std::make_shared<gui::VGrid>(2, gridSpacing);
    bgLayout->AddChild(std::make_shared<gui::Label>("BG Color"));
    bgLayout->AddChild(impl_->settings.wgtBGColor);

    viewCtrls->AddFixed(separationHeight);
    viewCtrls->AddChild(settings.wgtSkyEnabled);
    viewCtrls->AddFixed(0.25 * em);
    viewCtrls->AddChild(bgLayout);

    // ... show axes
    settings.wgtShowAxes = std::make_shared<gui::Checkbox>("Show axes");
    settings.wgtShowAxes->SetChecked(kDefaultShowAxes);
    settings.wgtShowAxes->SetOnChecked([this, renderScene](bool isChecked) {
        renderScene->SetEntityEnabled(this->impl_->settings.hAxes, isChecked);
    });
    viewCtrls->AddFixed(separationHeight);
    viewCtrls->AddChild(settings.wgtShowAxes);

    // ... lighting profiles
    settings.wgtLightingProfile = std::make_shared<gui::Combobox>();
    for (size_t i = 0; i < gLightingProfiles.size(); ++i) {
        settings.wgtLightingProfile->AddItem(gLightingProfiles[i].name.c_str());
    }
    settings.wgtLightingProfile->AddItem("Custom");
    settings.wgtLightingProfile->SetSelectedIndex(defaultLightingProfileIdx);
    settings.wgtLightingProfile->SetOnValueChanged(
            [this](const char *, int index) {
                if (index < int(gLightingProfiles.size())) {
                    this->impl_->SetLightingProfile(this->GetRenderer(),
                                                    gLightingProfiles[index]);
                } else {
                    this->impl_->settings.wgtAdvanced->SetIsOpen(true);
                    this->SetNeedsLayout();
                }
            });

    auto profileLayout = std::make_shared<gui::Vert>();
    profileLayout->AddChild(std::make_shared<gui::Label>("Lighting profiles"));
    profileLayout->AddChild(settings.wgtLightingProfile);
    viewCtrls->AddFixed(separationHeight);
    viewCtrls->AddChild(profileLayout);

    settings.wgtBase->AddChild(viewCtrls);
    settings.wgtBase->AddFixed(separationHeight);

    // ... advanced lighting
    settings.wgtAdvanced = std::make_shared<gui::CollapsableVert>(
            "Advanced lighting", 0, indent);
    settings.wgtAdvanced->SetIsOpen(false);
    settings.wgtBase->AddChild(settings.wgtAdvanced);

    // ....... lighting on/off
    settings.wgtAdvanced->AddChild(
            std::make_shared<gui::Label>("Light sources"));
    auto checkboxes = std::make_shared<gui::Horiz>();
    settings.wgtIBLEnabled = std::make_shared<gui::Checkbox>("HDR map");
    settings.wgtIBLEnabled->SetChecked(true);
    settings.wgtIBLEnabled->SetOnChecked([this, renderScene](bool checked) {
        impl_->settings.SetCustomProfile();
        if (checked) {
            renderScene->SetIndirectLight(impl_->settings.hIbl);
        } else {
            renderScene->SetIndirectLight(IndirectLightHandle());
        }
    });
    checkboxes->AddChild(settings.wgtIBLEnabled);
    settings.wgtDirectionalEnabled = std::make_shared<gui::Checkbox>("Sun");
    settings.wgtDirectionalEnabled->SetChecked(true);
    settings.wgtDirectionalEnabled->SetOnChecked(
            [this, renderScene](bool checked) {
                impl_->settings.SetCustomProfile();
                renderScene->SetEntityEnabled(impl_->settings.hDirectionalLight,
                                              checked);
            });
    checkboxes->AddChild(settings.wgtDirectionalEnabled);
    settings.wgtAdvanced->AddChild(checkboxes);

    settings.wgtAdvanced->AddFixed(separationHeight);

    // ....... IBL
    settings.wgtIBLs = std::make_shared<gui::Combobox>();
    std::vector<std::string> resourceFiles;
    utility::filesystem::ListFilesInDirectory(rsrcPath, resourceFiles);
    std::sort(resourceFiles.begin(), resourceFiles.end());
    int n = 0;
    for (auto &f : resourceFiles) {
        if (f.find("_ibl.ktx") == f.size() - 8) {
            auto name = utility::filesystem::GetFileNameWithoutDirectory(f);
            name = name.substr(0, name.size() - 8);
            settings.wgtIBLs->AddItem(name.c_str());
            if (name == kDefaultIBL) {
                settings.wgtIBLs->SetSelectedIndex(n);
            }
            n++;
        }
    }
    settings.wgtIBLs->AddItem("Custom...");
    settings.wgtIBLs->SetOnValueChanged([this](const char *name, int) {
        std::string path = gui::Application::GetInstance().GetResourcePath();
        path += std::string("/") + name + "_ibl.ktx";
        if (!this->SetIBL(path.c_str())) {
            // must be the "Custom..." option
            auto dlg = std::make_shared<gui::FileDialog>(
                    gui::FileDialog::Type::OPEN, "Open HDR Map", GetTheme());
            dlg->AddFilter(".ktx", "Khronos Texture (.ktx)");
            dlg->SetOnCancel([this]() { this->CloseDialog(); });
            dlg->SetOnDone([this](const char *path) {
                this->CloseDialog();
                this->SetIBL(path);
                this->impl_->settings.SetCustomProfile();
            });
            ShowDialog(dlg);
        }
    });

    settings.wgtIBLIntensity = MakeSlider(gui::Slider::INT, 0.0, 150000.0,
                                          lightingProfile.iblIntensity);
    settings.wgtIBLIntensity->OnValueChanged = [this,
                                                renderScene](double newValue) {
        renderScene->SetIndirectLightIntensity(newValue);
        this->impl_->settings.SetCustomProfile();
    };

    auto ambientLayout = std::make_shared<gui::VGrid>(2, gridSpacing);
    ambientLayout->AddChild(std::make_shared<gui::Label>("HDR map"));
    ambientLayout->AddChild(settings.wgtIBLs);
    ambientLayout->AddChild(std::make_shared<gui::Label>("Intensity"));
    ambientLayout->AddChild(settings.wgtIBLIntensity);
    // ambientLayout->AddChild(std::make_shared<gui::Label>("Skybox"));
    // ambientLayout->AddChild(settings.wgtLoadSky);

    settings.wgtAdvanced->AddChild(std::make_shared<gui::Label>("Environment"));
    settings.wgtAdvanced->AddChild(ambientLayout);
    settings.wgtAdvanced->AddFixed(separationHeight);

    // ... directional light (sun)
    settings.wgtSunIntensity = MakeSlider(gui::Slider::INT, 0.0, 500000.0,
                                          lightingProfile.sunIntensity);
    settings.wgtSunIntensity->OnValueChanged = [this,
                                                renderScene](double newValue) {
        renderScene->SetLightIntensity(impl_->settings.hDirectionalLight,
                                       newValue);
        this->impl_->settings.SetCustomProfile();
    };

    auto setSunDir = [this, renderScene](const Eigen::Vector3f &dir) {
        this->impl_->settings.wgtSunDir->SetValue(dir);
        renderScene->SetLightDirection(impl_->settings.hDirectionalLight,
                                       dir.normalized());
        this->impl_->settings.SetCustomProfile();
    };

    this->impl_->scene->SelectDirectionalLight(
            settings.hDirectionalLight, [this](const Eigen::Vector3f &newDir) {
                impl_->settings.wgtSunDir->SetValue(newDir);
                this->impl_->settings.SetCustomProfile();
            });

    settings.wgtSunDir = std::make_shared<gui::VectorEdit>();
    settings.wgtSunDir->SetValue(lightDescription.direction);
    settings.wgtSunDir->SetOnValueChanged(setSunDir);

    settings.wgtSunColor = std::make_shared<gui::ColorEdit>();
    settings.wgtSunColor->SetValue({1, 1, 1});
    settings.wgtSunColor->OnValueChanged = [this, renderScene](
                                                   const gui::Color &newColor) {
        this->impl_->settings.SetCustomProfile();
        renderScene->SetLightColor(
                impl_->settings.hDirectionalLight,
                {newColor.GetRed(), newColor.GetGreen(), newColor.GetBlue()});
    };

    auto sunLayout = std::make_shared<gui::VGrid>(2, gridSpacing);
    sunLayout->AddChild(std::make_shared<gui::Label>("Intensity"));
    sunLayout->AddChild(settings.wgtSunIntensity);
    sunLayout->AddChild(std::make_shared<gui::Label>("Direction"));
    sunLayout->AddChild(settings.wgtSunDir);
    sunLayout->AddChild(std::make_shared<gui::Label>("Color"));
    sunLayout->AddChild(settings.wgtSunColor);

    settings.wgtAdvanced->AddChild(
            std::make_shared<gui::Label>("Sun (Directional light)"));
    settings.wgtAdvanced->AddChild(sunLayout);

    // materials settings
    settings.wgtBase->AddFixed(separationHeight);
    auto materials = std::make_shared<gui::CollapsableVert>("Material settings",
                                                            0, indent);

    auto matGrid = std::make_shared<gui::VGrid>(2, gridSpacing);
    matGrid->AddChild(std::make_shared<gui::Label>("Type"));
    settings.wgtMaterialType.reset(
            new gui::Combobox({"Lit", "Unlit", "Normal map", "Depth"}));
    settings.wgtMaterialType->SetOnValueChanged([this, scene, renderScene](
                                                        const char *,
                                                        int selectedIdx) {
        using MaterialType = Impl::Settings::MaterialType;
        using ViewMode = visualization::View::Mode;
        auto selected = (Impl::Settings::MaterialType)selectedIdx;

        auto view = scene->GetView();
        impl_->settings.selectedType = selected;

        bool isLit = (selected == MaterialType::LIT);
        impl_->settings.wgtPrefabMaterial->SetEnabled(isLit);

        switch (selected) {
            case MaterialType::LIT:
                view->SetMode(ViewMode::Color);
                for (const auto &handle : impl_->geometryHandles) {
                    auto mat = impl_->geometryMaterials[handle].lit.handle;
                    renderScene->AssignMaterial(handle, mat);
                }
                break;
            case MaterialType::UNLIT:
                view->SetMode(ViewMode::Color);
                for (const auto &handle : impl_->geometryHandles) {
                    auto mat = impl_->geometryMaterials[handle].unlit.handle;
                    renderScene->AssignMaterial(handle, mat);
                }
                break;
            case MaterialType::NORMAL_MAP:
                view->SetMode(ViewMode::Normals);
                break;
            case MaterialType::DEPTH:
                view->SetMode(ViewMode::Depth);
                break;
        }
    });
    matGrid->AddChild(settings.wgtMaterialType);

    settings.wgtPrefabMaterial = std::make_shared<gui::Combobox>();
    for (auto &prefab : impl_->prefabMaterials) {
        settings.wgtPrefabMaterial->AddItem(prefab.first.c_str());
    }
    settings.wgtPrefabMaterial->SetSelectedValue(kDefaultMaterialName.c_str());
    settings.wgtPrefabMaterial->SetOnValueChanged([this, renderScene](
                                                          const char *name,
                                                          int) {
        auto &renderer = this->GetRenderer();
        auto prefabIt = this->impl_->prefabMaterials.find(name);
        if (prefabIt != this->impl_->prefabMaterials.end()) {
            auto &prefab = prefabIt->second;
            for (const auto &handle : impl_->geometryHandles) {
                auto mat = impl_->geometryMaterials[handle].lit.handle;
                mat = renderer.ModifyMaterial(mat)
                              .SetColor("baseColor", prefab.baseColor)
                              .SetParameter("roughness", prefab.roughness)
                              .SetParameter("metallic", prefab.metallic)
                              .SetParameter("reflectance", prefab.reflectance)
                              .SetParameter("clearCoat", prefab.clearCoat)
                              .SetParameter("clearCoatRoughness",
                                            prefab.clearCoatRoughness)
                              .SetParameter("anisotropy", prefab.anisotropy)
                              // Point size is part of the material for
                              // rendering reasons, and therefore
                              // prefab.pointSize exists, but conceptually (and
                              // UI-wise) it is separate. So use the current
                              // setting instead of the prefab setting for point
                              // size.
                              .SetParameter("pointSize",
                                            float(impl_->settings.wgtPointSize
                                                          ->GetDoubleValue()))
                              .Finish();
                renderScene->AssignMaterial(handle, mat);
            }
        }
    });
    matGrid->AddChild(std::make_shared<gui::Label>("Material"));
    matGrid->AddChild(settings.wgtPrefabMaterial);

    matGrid->AddChild(std::make_shared<gui::Label>("Point size"));
    settings.wgtPointSize = MakeSlider(gui::Slider::INT, 1.0, 10.0, 3);
    settings.wgtPointSize->OnValueChanged = [this](double value) {
        auto &renderer = GetRenderer();
        for (const auto &pair : impl_->geometryMaterials) {
            renderer.ModifyMaterial(pair.second.lit.handle)
                    .SetParameter("pointSize", (float)value)
                    .Finish();
            renderer.ModifyMaterial(pair.second.unlit.handle)
                    .SetParameter("pointSize", (float)value)
                    .Finish();
        }

        renderer.ModifyMaterial(FilamentResourceManager::kDepthMaterial)
                .SetParameter("pointSize", (float)value)
                .Finish();
        renderer.ModifyMaterial(FilamentResourceManager::kNormalsMaterial)
                .SetParameter("pointSize", (float)value)
                .Finish();
    };
    matGrid->AddChild(settings.wgtPointSize);
    materials->AddChild(matGrid);

    settings.wgtBase->AddChild(materials);

    AddChild(settings.wgtBase);

    // Other items
    impl_->helpKeys = createHelpDisplay(this);
    impl_->helpKeys->SetVisible(false);
    AddChild(impl_->helpKeys);

    // Set the actual geometries
    SetGeometry(geometries);  // also updates the camera
}

GuiVisualizer::~GuiVisualizer() {}

void GuiVisualizer::SetTitle(const std::string &title) {
    Super::SetTitle(title.c_str());
}

void GuiVisualizer::SetGeometry(
        const std::vector<std::shared_ptr<const geometry::Geometry>>
                &geometries) {
    auto *scene3d = impl_->scene->GetScene();
    if (impl_->settings.hAxes) {
        scene3d->RemoveGeometry(impl_->settings.hAxes);
    }
    for (auto &h : impl_->geometryHandles) {
        scene3d->RemoveGeometry(h);
    }
    impl_->geometryHandles.clear();

    auto &renderer = GetRenderer();
    for (const auto &pair : impl_->geometryMaterials) {
        renderer.RemoveMaterialInstance(pair.second.unlit.handle);
        renderer.RemoveMaterialInstance(pair.second.lit.handle);
    }
    impl_->geometryMaterials.clear();

    geometry::AxisAlignedBoundingBox bounds;
    std::vector<visualization::GeometryHandle> objects;

    std::size_t nPointClouds = 0;
    for (auto &g : geometries) {
        Impl::Materials materials;
        materials.lit.handle =
                GetRenderer().AddMaterialInstance(impl_->hLitMaterial);
        materials.unlit.handle =
                GetRenderer().AddMaterialInstance(impl_->hUnlitMaterial);
        Impl::SetMaterialsDefaults(materials, GetRenderer());

        visualization::MaterialInstanceHandle selectedMaterial;

        switch (g->GetGeometryType()) {
            case geometry::Geometry::GeometryType::PointCloud: {
                nPointClouds++;
                auto pcd =
                        std::static_pointer_cast<const geometry::PointCloud>(g);

                if (pcd->HasColors()) {
                    selectedMaterial = materials.unlit.handle;

                    if (SmartMode::PointCloudHasUniformColor(*pcd)) {
                        selectedMaterial = materials.lit.handle;
                    }
                } else {
                    selectedMaterial = materials.lit.handle;
                }
            } break;
            case geometry::Geometry::GeometryType::LineSet: {
                selectedMaterial = materials.unlit.handle;
            } break;
            case geometry::Geometry::GeometryType::TriangleMesh: {
                auto mesh =
                        std::static_pointer_cast<const geometry::TriangleMesh>(
                                g);

                if (mesh->HasVertexColors()) {
                    selectedMaterial = materials.unlit.handle;
                } else {
                    selectedMaterial = materials.lit.handle;
                }
            } break;
            default:
                utility::LogWarning("Geometry type {} not supported!",
                                    (int)g->GetGeometryType());
                break;
        }

        auto g3 = std::static_pointer_cast<const geometry::Geometry3D>(g);
        auto handle = scene3d->AddGeometry(*g3, selectedMaterial);
        bounds += scene3d->GetEntityBoundingBox(handle);
        objects.push_back(handle);

        impl_->geometryHandles.push_back(handle);

        auto viewMode = impl_->scene->GetView()->GetMode();
        if (viewMode == visualization::View::Mode::Normals) {
            impl_->settings.SetMaterialSelected(Impl::Settings::NORMAL_MAP);
        } else if (viewMode == visualization::View::Mode::Depth) {
            impl_->settings.SetMaterialSelected(Impl::Settings::DEPTH);
        } else {
            if (selectedMaterial == materials.unlit.handle) {
                impl_->settings.SetMaterialSelected(Impl::Settings::UNLIT);
            } else {
                impl_->settings.SetMaterialSelected(Impl::Settings::LIT);
            }
        }

        if (nPointClouds == geometries.size()) {
            impl_->SetLightingProfile(GetRenderer(), kPointCloudProfileName);
        }
        impl_->settings.wgtPointSize->SetEnabled(nPointClouds > 0);

        impl_->geometryMaterials.emplace(handle, materials);
    }

    // Add axes
    auto axisLength = bounds.GetMaxExtent();
    if (axisLength < 0.001) {
        axisLength = 1.0;
    }
    auto axes = CreateAxes(axisLength);
    impl_->settings.hAxes = scene3d->AddGeometry(*axes);
    scene3d->SetGeometryShadows(impl_->settings.hAxes, false, false);
    scene3d->SetEntityEnabled(impl_->settings.hAxes,
                              impl_->settings.wgtShowAxes->IsChecked());
    impl_->scene->SetModel(impl_->settings.hAxes, objects);

    impl_->scene->SetupCamera(60.0, bounds, bounds.GetCenter().cast<float>());
}

void GuiVisualizer::Layout(const gui::Theme &theme) {
    auto r = GetContentRect();
    const auto em = theme.fontSize;
    impl_->scene->SetFrame(r);

    // Draw help keys HUD in upper left
    const auto pref = impl_->helpKeys->CalcPreferredSize(theme);
    impl_->helpKeys->SetFrame(gui::Rect(0, r.y, pref.width, pref.height));
    impl_->helpKeys->Layout(theme);

    // Settings in upper right
    const auto kLightSettingsWidth = 18 * em;
    auto lightSettingsSize = impl_->settings.wgtBase->CalcPreferredSize(theme);
    gui::Rect lightSettingsRect(r.width - kLightSettingsWidth, r.y,
                                kLightSettingsWidth, lightSettingsSize.height);
    impl_->settings.wgtBase->SetFrame(lightSettingsRect);

    Super::Layout(theme);
}

bool GuiVisualizer::SetIBL(const char *path) {
    auto result = impl_->SetIBL(GetRenderer(), path);
    PostRedraw();
    return result;
}

bool GuiVisualizer::LoadGeometry(const std::string &path) {
    auto geometry = std::shared_ptr<geometry::Geometry3D>();

    auto mesh = std::make_shared<geometry::TriangleMesh>();
    bool meshSuccess = false;
    try {
        meshSuccess = io::ReadTriangleMesh(path, *mesh);
    } catch (...) {
        meshSuccess = false;
    }
    if (meshSuccess) {
        if (mesh->triangles_.size() == 0) {
            utility::LogWarning(
                    "Contains 0 triangles, will read as point cloud");
            mesh.reset();
        } else {
            mesh->ComputeVertexNormals();
            geometry = mesh;
        }
    } else {
        // LogError throws an exception, which we don't want, because this might
        // be a point cloud.
        utility::LogWarning("Failed to read %s", path.c_str());
        mesh.reset();
    }

    if (!geometry) {
        auto cloud = std::make_shared<geometry::PointCloud>();
        bool success = false;
        try {
            success = io::ReadPointCloud(path, *cloud);
        } catch (...) {
            success = false;
        }
        if (success) {
            utility::LogInfof("Successfully read %s", path.c_str());
            if (!cloud->HasNormals()) {
                cloud->EstimateNormals();
            }
            cloud->NormalizeNormals();
            geometry = cloud;
        } else {
            utility::LogWarning("Failed to read points %s", path.c_str());
            cloud.reset();
        }
    }

    if (geometry) {
        SetGeometry({geometry});
    }
    return (geometry != nullptr);
}

void GuiVisualizer::ExportCurrentImage(int width,
                                       int height,
                                       const std::string &path) {
    GetRenderer().RenderToImage(
            width, height, impl_->scene->GetView(), impl_->scene->GetScene(),
            [this, path](std::shared_ptr<geometry::Image> image) mutable {
                if (!io::WriteImage(path, *image)) {
                    this->ShowMessageBox(
                            "Error", (std::string("Could not write image to ") +
                                      path + ".")
                                             .c_str());
                }
            });
}

void GuiVisualizer::OnMenuItemSelected(gui::Menu::ItemId itemId) {
    auto menuId = MenuId(itemId);
    switch (menuId) {
        case FILE_OPEN: {
            auto dlg = std::make_shared<gui::FileDialog>(
                    gui::FileDialog::Type::OPEN, "Open Geometry", GetTheme());
            dlg->AddFilter(".ply .stl .obj .off .gltf .glb",
                           "Triangle mesh files (.ply, .stl, .obj, .off, "
                           ".gltf, .glb)");
            dlg->AddFilter(".xyz .xyzn .xyzrgb .ply .pcd .pts",
                           "Point cloud files (.xyz, .xyzn, .xyzrgb, .ply, "
                           ".pcd, .pts)");
            dlg->AddFilter(".ply", "Polygon files (.ply)");
            dlg->AddFilter(".stl", "Stereolithography files (.stl)");
            dlg->AddFilter(".obj", "Wavefront OBJ files (.obj)");
            dlg->AddFilter(".off", "Object file format (.off)");
            dlg->AddFilter(".gltf", "OpenGL transfer files (.gltf)");
            dlg->AddFilter(".glb", "OpenGL binary transfer files (.glb)");
            dlg->AddFilter(".xyz", "ASCII point cloud files (.xyz)");
            dlg->AddFilter(".xyzn", "ASCII point cloud with normals (.xyzn)");
            dlg->AddFilter(".xyzrgb",
                           "ASCII point cloud files with colors (.xyzrgb)");
            dlg->AddFilter(".pcd", "Point Cloud Data files (.pcd)");
            dlg->AddFilter(".pts", "3D Points files (.pts)");
            dlg->AddFilter("", "All files");
            dlg->SetOnCancel([this]() { this->CloseDialog(); });
            dlg->SetOnDone([this](const char *path) {
                this->CloseDialog();
                OnDragDropped(path);
            });
            ShowDialog(dlg);
            break;
        }
        case FILE_EXPORT_RGB: {
            auto dlg = std::make_shared<gui::FileDialog>(
                    gui::FileDialog::Type::SAVE, "Save File", GetTheme());
            dlg->AddFilter(".png", "PNG images (.png)");
            dlg->AddFilter("", "All files");
            dlg->SetOnCancel([this]() { this->CloseDialog(); });
            dlg->SetOnDone([this](const char *path) {
                this->CloseDialog();
                auto r = GetContentRect();
                this->ExportCurrentImage(r.width, r.height, path);
            });
            ShowDialog(dlg);
            break;
        }
        case FILE_CLOSE:
            this->Close();
            break;
        case SETTINGS_LIGHT_AND_MATERIALS: {
            auto visibility = !impl_->settings.wgtBase->IsVisible();
            impl_->settings.wgtBase->SetVisible(visibility);
            auto menubar = gui::Application::GetInstance().GetMenubar();
            menubar->SetChecked(SETTINGS_LIGHT_AND_MATERIALS, visibility);

            // We need relayout because materials settings pos depends on light
            // settings visibility
            Layout(GetTheme());

            break;
        }
        case HELP_KEYS: {
            bool isVisible = !impl_->helpKeys->IsVisible();
            impl_->helpKeys->SetVisible(isVisible);
            auto menubar = gui::Application::GetInstance().GetMenubar();
            menubar->SetChecked(HELP_KEYS, isVisible);
            break;
        }
        case HELP_ABOUT: {
            auto dlg = createAboutDialog(this);
            ShowDialog(dlg);
            break;
        }
        case HELP_CONTACT: {
            auto dlg = createContactDialog(this);
            ShowDialog(dlg);
            break;
        }
    }
}

void GuiVisualizer::OnDragDropped(const char *path) {
    auto title = std::string("Open3D - ") + path;
#if LOAD_IN_NEW_WINDOW
    auto frame = this->GetFrame();
    std::vector<std::shared_ptr<const geometry::Geometry>> nothing;
    auto vis = std::make_shared<GuiVisualizer>(nothing, title.c_str(),
                                               frame.width, frame.height,
                                               frame.x + 20, frame.y + 20);
    gui::Application::GetInstance().AddWindow(vis);
#else
    this->SetTitle(title);
    auto vis = this;
#endif  // LOAD_IN_NEW_WINDOW
    if (!vis->LoadGeometry(path)) {
        auto err = std::string("Error reading geometry file '") + path + "'";
        vis->ShowMessageBox("Error loading geometry", err.c_str());
    }
    PostRedraw();
}

}  // namespace visualization
}  // namespace open3d
