#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif // IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <cstdio>

#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>

#include "image_view.h"
#include "misc/cpp/imgui_stdlib.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "ytt_generator.h"
#include "fonts/lucon.hpp"
#include <nfd.h>

constexpr float FONT_LOAD_SIZE = 96;
constexpr float FONT_SIZE = 20;
constexpr float FONT_SCALE_CONSTANT = FONT_SIZE / FONT_LOAD_SIZE;

bool first_time_layout = true;
static ImGuiID dockspace_id = 0;

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void *data, size_t data_size, GLuint *out_texture, int *out_width, int *out_height) {
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data = stbi_load_from_memory(static_cast<const unsigned char *>(data),
                                                      static_cast<int>(data_size), &image_width,
                                                      &image_height, nullptr, 4);
    if (image_data == nullptr)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char *file_name, GLuint *out_texture, int *out_width, int *out_height) {
    FILE *f = fopen(file_name, "rb");
    if (f == nullptr)
        return false;
    fseek(f, 0, SEEK_END);
    auto file_size = static_cast<size_t>(ftell(f));
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void *file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

static void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static float GetDPIScale(GLFWwindow *window) {
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    return xscale; // Assuming uniform scaling for simplicity
}

void LoadFonts() {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    auto font = io.Fonts->AddFontFromMemoryCompressedTTF(LuCon_compressed_data, LuCon_compressed_size, FONT_LOAD_SIZE,
                                                         &font_cfg,
                                                         io.Fonts->GetGlyphRangesCyrillic());
    font->Scale = FONT_SCALE_CONSTANT;
    io.Fonts->Build();
}

void ApplyDPI(float dpi_scale) {
    static const ImGuiStyle default_style = ImGui::GetStyle();
    ImGui::GetStyle() = default_style;
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(dpi_scale);

}


void ShowDockSpace() {
    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags wflags =
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("##MainDockSpace", nullptr, wflags);

    dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    if (first_time_layout) {
        first_time_layout = false;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(dockspace_id, vp->Size);

        ImGuiID left_id, right_id;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.7f, &left_id, &right_id);

        ImGui::DockBuilderDockWindow("Preview", left_id);
        ImGui::DockBuilderDockWindow("Settings", right_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End();
}

void ShowColorEdit(const char *label, Color &color) {
    float col[4] = {color.r / 255.f,
                    color.g / 255.f,
                    color.b / 255.f,
                    color.a / 255.f
    };

    // ImGui color edit widget
    if (ImGui::ColorEdit4(label, col)) {
        color.r = col[0] * 255;
        color.g = col[1] * 255;
        color.b = col[2] * 255;
        color.a = col[3] * 255;
    }
}


int main(int, char **) {
    NFD_Init();
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "SubChat Config Generator", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    float dpi_scale = GetDPIScale(window);
    LoadFonts();
    ApplyDPI(dpi_scale);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    int preview_width = 0;
    int preview_height = 0;
    GLuint preview_texture = 0;

    PreloadPreviewFont();
    InteractiveTextOverlay text_overlay{};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        float new_dpi_scale = GetDPIScale(window);
        if (fabs(dpi_scale - new_dpi_scale) > 0.1f * std::max(fabs(dpi_scale), fabs(new_dpi_scale))) {
            dpi_scale = new_dpi_scale;
            ApplyDPI(dpi_scale);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ShowDockSpace();

        ShowInteractiveImage(preview_texture, preview_width, preview_height, &text_overlay);

        auto &p = text_overlay.params;

        ImGui::Begin("Settings");
        if (ImGui::Button("Load Image for Preview")) {
            nfdu8char_t *outPath = nullptr;
            nfdu8filteritem_t filters[1] = {{"Image Files", "png,jpg,jpeg"}};
            nfdopendialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 1;
            // Set the parent window handle using the GLFW binding
            //NFD_GetNativeWindowFromGLFWWindow(window, &args.parentWindow);
            nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
            if (result == NFD_OKAY) {
                if (preview_texture != 0) {
                    glDeleteTextures(1, &preview_texture);
                    preview_texture = 0;
                }
                bool ret = LoadTextureFromFile(outPath, &preview_texture, &preview_width, &preview_height);
                if (!ret)
                    printf("Failed to load image: %s\n", outPath);
                NFD_FreePathU8(outPath);
            } else if (result == NFD_CANCEL) {

            } else {
                printf("Error: %s\n", NFD_GetError());
            }
        }
        if (preview_texture == 0) ImGui::BeginDisabled();
        ImGui::SliderInt("X", &p.horizontalMargin, 0, 100);
        ImGui::SliderInt("Y", &p.verticalMargin, 0, 100);
        ImGui::SliderInt("Font size", &p.fontSizePercent, 0, 300);
        ImGui::SliderInt("Vertical\nspacing", &p.verticalSpacing, 0, 25);
//        TODO add more options to GUI
//        ImGui::Checkbox("Bold", &p.textBold);
//        ImGui::SameLine();
//        ImGui::Checkbox("Italic", &p.textItalic);
//        ImGui::SameLine();
//        ImGui::Checkbox("Underline", &p.textUnderline);
        ShowColorEdit("Text Color", p.textForegroundColor);
        text_overlay.revalidatePreview += ImGui::SliderInt("Characters\nper line", &p.maxCharsPerLine, 5, 50);
        text_overlay.revalidatePreview += ImGui::SliderInt("Line\nCount", &p.totalDisplayLines, 1, 50);
        text_overlay.revalidatePreview += ImGui::InputText("Username\nSeparator", &p.usernameSeparator);
        if (ImGui::Button("Load Config")) {
            nfdu8char_t *outPath = nullptr;
            nfdu8filteritem_t filters[1] = {{"Chat configs", "ini"}};
            nfdopendialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 1;
            // TODO Set the parent window handle using the GLFW binding
            //NFD_GetNativeWindowFromGLFWWindow(window, &args.parentWindow);
            nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
            if (result == NFD_OKAY) {
                p.loadFromFile(outPath);
                text_overlay.revalidatePreview = true;
                NFD_FreePathU8(outPath);
            } else if (result == NFD_CANCEL) {
                printf("User pressed cancel on load config.\n");
            } else {
                printf("Error (load config): %s\n", NFD_GetError());
            }
        }
        ImGui::SameLine();
        if (!text_overlay.isInsidePicture)
            ImGui::BeginDisabled();
        if (ImGui::Button("Save Config")) {
            nfdu8char_t *outPath = nullptr;
            nfdu8filteritem_t filters[1] = {{"Chat configs", "ini"}};
            nfdsavedialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 1;
            args.defaultName = "config.ini";
            // TODO NFD_GetNativeWindowFromGLFWWindow(window, &args.parentWindow);
            nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
            if (result == NFD_OKAY) {
                p.saveToFile(outPath);
                NFD_FreePathU8(outPath);
            } else if (result == NFD_CANCEL) {
                printf("User pressed cancel on save configs.\n");
            } else {
                printf("Error (save configs): %s\n", NFD_GetError());
            }
        }
        if (!text_overlay.isInsidePicture) ImGui::EndDisabled();
        if (preview_texture == 0) ImGui::EndDisabled();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    NFD_Quit();
    return 0;
}
