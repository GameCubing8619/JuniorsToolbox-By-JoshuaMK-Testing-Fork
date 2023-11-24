#include <cmath>

#include <glad/glad.h>

#include "gui/scene/window.hpp"

#include "gui/input.hpp"
#include "gui/util.hpp"

#include <lib/bStream/bstream.h>

#include "gui/imgui_ext.hpp"
#include <imgui.h>
#include <imgui_internal.h>

#include <ImGuiFileDialog.h>

#include "gui/application.hpp"
#include "gui/util.hpp"
#include <gui/IconsForkAwesome.h>
#include <gui/modelcache.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iconv.h>

#if WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include <gui/context_menu.hpp>

#include <J3D/Material/J3DMaterialTableLoader.hpp>
#include <J3D/Material/J3DUniformBufferObject.hpp>

namespace Toolbox::UI {

    void PacketSort(J3DRendering::SortFunctionArgs packets) {
        std::sort(packets.begin(), packets.end(),
                  [](const J3DRenderPacket &a, const J3DRenderPacket &b) -> bool {
                      if ((a.SortKey & 0x01000000) != (b.SortKey & 0x01000000)) {
                          return (a.SortKey & 0x01000000) > (b.SortKey & 0x01000000);
                      } else {
                          return a.Material->Name < b.Material->Name;
                      }
                  });
    }

    /* icu

    includes:

    #include <unicode/ucnv.h>
    #include <unicode/unistr.h>
    #include <unicode/ustring.h>

    std::string Utf8ToSjis(const std::string& value)
    {
        icu::UnicodeString src(value.c_str(), "utf8");
        int length = src.extract(0, src.length(), NULL, "shift_jis");

        std::vector<char> result(length + 1);
        src.extract(0, src.length(), &result[0], "shift_jis");

        return std::string(result.begin(), result.end() - 1);
    }

    std::string SjisToUtf8(const std::string& value)
    {
        icu::UnicodeString src(value.c_str(), "shift_jis");
        int length = src.extract(0, src.length(), NULL, "utf8");

        std::vector<char> result(length + 1);
        src.extract(0, src.length(), &result[0], "utf8");

        return std::string(result.begin(), result.end() - 1);
    }
    */

    static std::string getNodeUID(std::shared_ptr<Toolbox::Object::ISceneObject> node) {
        std::string node_name = std::format("{} ({})##{}", node->type(), node->getNameRef().name(),
                                            node->getQualifiedName().toString());
        return node_name;
    }

    bool SceneTreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char *label,
                               const char *label_end, bool focused, bool *visible) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        const f32 row_y = ImGui::GetCursorPosY();

        ImGuiContext &g          = *GImGui;
        const ImGuiStyle &style  = g.Style;
        const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
        ImVec2 padding =
            (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding))
                ? style.FramePadding
                : ImVec2(style.FramePadding.x,
                         ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));
        padding.y *= 1.2f;

        if (!label_end)
            label_end = ImGui::FindRenderedTextEnd(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, label_end, false);
        const ImVec2 eye_size   = ImGui::CalcTextSize(ICON_FK_EYE, nullptr, false);

        // We vertically grow up to current line height up the typical widget height.
        const float frame_height =
            ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2),
                  label_size.y + padding.y * 2);
        const bool span_all_columns =
            (flags & ImGuiTreeNodeFlags_SpanAllColumns) != 0 && (g.CurrentTable != NULL);
        ImRect frame_bb;
        frame_bb.Min.x = span_all_columns                             ? window->ParentWorkRect.Min.x
                         : (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x
                                                                      : window->DC.CursorPos.x;
        frame_bb.Min.y = window->DC.CursorPos.y;
        frame_bb.Max.x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
        frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
        if (display_frame) {
            // Framed header expand a little outside the default padding, to the edge of
            // InnerClipRect (FIXME: May remove this at some point and make InnerClipRect align with
            // WindowPadding.x instead of WindowPadding.x*0.5f)
            frame_bb.Min.x -= IM_TRUNC(window->WindowPadding.x * 0.5f - 1.0f);
            frame_bb.Max.x += IM_TRUNC(window->WindowPadding.x * 0.5f);
        }

        const float text_offset_x =
            g.FontSize +
            (display_frame ? padding.x * 3 : padding.x * 2);  // Collapsing arrow width + Spacing
        const float text_offset_y = ImMax(
            padding.y, window->DC.CurrLineTextBaseOffset);  // Latch before ItemSize changes it
        const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2
                                                                   : 0.0f);  // Include collapsing
        ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x + label_size.y * 1.5f,
                        window->DC.CursorPos.y + text_offset_y);
        ImVec2 eye_pos(frame_bb.Min.x + padding.x, window->DC.CursorPos.y);

        ImGui::ItemSize(ImVec2(text_width, frame_height), padding.y);

        ImRect eye_interact_bb = frame_bb;
        eye_interact_bb.Max.x = frame_bb.Min.x + eye_size.x + style.ItemSpacing.x;

        // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
        ImRect interact_bb = frame_bb;
        if (!display_frame &&
            (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth |
                      ImGuiTreeNodeFlags_SpanAllColumns)) == 0)
            interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;
        interact_bb.Min.x += eye_size.x + style.TouchExtraPadding.x;

        // Modify ClipRect for the ItemAdd(), faster than doing a
        // PushColumnsBackground/PushTableBackgroundChannel for every Selectable..
        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        if (span_all_columns) {
            window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
            window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        }

        // ImGui::SetActiveID(0, ImGui::GetCurrentWindow());
        ImGuiID eye_interact_id = ImGui::GetID("eye_button##internal");
        ImGui::ItemAdd(eye_interact_bb, eye_interact_id);

        // Compute open and multi-select states before ItemAdd() as it clear NextItem data.
        bool is_open  = ImGui::TreeNodeUpdateNextOpen(id, flags);
        bool item_add = ImGui::ItemAdd(interact_bb, id);
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
        g.LastItemData.DisplayRect = frame_bb;

        if (span_all_columns) {
            window->ClipRect.Min.x = backup_clip_rect_min_x;
            window->ClipRect.Max.x = backup_clip_rect_max_x;
        }

        // If a NavLeft request is happening and ImGuiTreeNodeFlags_NavLeftJumpsBackHere enabled:
        // Store data for the current depth to allow returning to this node from any child item.
        // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between
        // TreeNode() and TreePop(). It will become tempting to enable
        // ImGuiTreeNodeFlags_NavLeftJumpsBackHere by default or move it to ImGuiStyle. Currently
        // only supports 32 level deep and we are fine with (1 << Depth) overflowing into a zero,
        // easy to increase.
        if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) &&
            !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            if (g.NavMoveDir == ImGuiDir_Left && g.NavWindow == window &&
                ImGui::NavMoveRequestButNoResultYet()) {
                g.NavTreeNodeStack.resize(g.NavTreeNodeStack.Size + 1);
                ImGuiNavTreeNodeData *nav_tree_node_data = &g.NavTreeNodeStack.back();
                nav_tree_node_data->ID                   = id;
                nav_tree_node_data->InFlags              = g.LastItemData.InFlags;
                nav_tree_node_data->NavRect              = g.LastItemData.NavRect;
                window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);
            }

        const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
        if (!item_add) {
            if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
                ImGui::TreePushOverrideID(id);
            IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label,
                                        g.LastItemData.StatusFlags |
                                            (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) |
                                            (is_open ? ImGuiItemStatusFlags_Opened : 0));
            return is_open;
        }

        if (span_all_columns)
            ImGui::TablePushBackgroundChannel();

        const float eye_hit_x1 = eye_pos.x - style.TouchExtraPadding.x;
        const float eye_hit_x2 = eye_pos.x + (eye_size.x) + style.TouchExtraPadding.x;
        const bool is_mouse_x_over_eye =
            (g.IO.MousePos.x >= eye_hit_x1 && g.IO.MousePos.x < eye_hit_x2);

        ImGuiButtonFlags eye_button_flags = ImGuiTreeNodeFlags_None;
        if ((flags & ImGuiTreeNodeFlags_AllowOverlap) ||
            (g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap))
            eye_button_flags |= ImGuiButtonFlags_AllowOverlap;
        eye_button_flags |= ImGuiButtonFlags_PressedOnClick;

        bool eye_hovered, eye_held;
        bool eye_pressed = ImGui::ButtonBehavior(eye_interact_bb, eye_interact_id, &eye_hovered,
                                                 &eye_held, eye_button_flags);
        if (eye_pressed && is_mouse_x_over_eye) {
            *visible ^= true;
        }

        ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
        if ((flags & ImGuiTreeNodeFlags_AllowOverlap) ||
            (g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap))
            button_flags |= ImGuiButtonFlags_AllowOverlap;
        if (!is_leaf)
            button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

        // We allow clicking on the arrow section with keyboard modifiers held, in order to easily
        // allow browsing a tree while preserving selection with code implementing multi-selection
        // patterns. When clicking on the rest of the tree node we always disallow keyboard
        // modifiers.
        const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
        const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) +
                                   style.TouchExtraPadding.x;
        const bool is_mouse_x_over_arrow =
            (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
        if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
            button_flags |= ImGuiButtonFlags_NoKeyModifiers;

        // Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
        // Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to
        // requirements for multi-selection and drag and drop support.
        // - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
        // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
        // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
        // - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
        // - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and
        // _OpenOnArrow=0) It is rather standard that arrow click react on Down rather than Up. We
        // set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item
        // to be active on the initial MouseDown in order for drag and drop to work.
        if (is_mouse_x_over_arrow)
            button_flags |= ImGuiButtonFlags_PressedOnClick;
        else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
            button_flags |=
                ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
        else
            button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

        bool selected           = (flags & ImGuiTreeNodeFlags_Selected) != 0;
        const bool was_selected = selected;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
        bool toggled = false;
        if (!is_leaf) {
            if (pressed && g.DragDropHoldJustPressedId != id) {
                if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow |
                              ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 ||
                    (g.NavActivateId == id))
                    toggled = true;
                if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
                    toggled |=
                        is_mouse_x_over_arrow &&
                        !g.NavDisableMouseHover;  // Lightweight equivalent of IsMouseHoveringRect()
                                                  // since ButtonBehavior() already did the job
                if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) &&
                    g.IO.MouseClickedCount[0] == 2)
                    toggled = true;
            } else if (pressed && g.DragDropHoldJustPressedId == id) {
                IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
                if (!is_open)  // When using Drag and Drop "hold to open" we keep the node
                               // highlighted after opening, but never close it again.
                    toggled = true;
            }

            if (g.NavId == id && g.NavMoveDir == ImGuiDir_Left && is_open) {
                toggled = true;
                ImGui::NavClearPreferredPosForAxis(ImGuiAxis_X);
                ImGui::NavMoveRequestCancel();
            }
            if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right &&
                !is_open)  // If there's something upcoming on the line we may want to give it the
                           // priority?
            {
                toggled = true;
                ImGui::NavClearPreferredPosForAxis(ImGuiAxis_X);
                ImGui::NavMoveRequestCancel();
            }

            if (toggled) {
                is_open = !is_open;
                window->DC.StateStorage->SetInt(id, is_open);
                g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
            }
        }

        // In this branch, TreeNodeBehavior() cannot toggle the selection so this will never
        // trigger.
        if (selected != was_selected)  //-V547
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

        // Render
        const ImU32 text_col                       = ImGui::GetColorU32(ImGuiCol_Text);
        ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
        if (display_frame) {
            // Framed type
            const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive
                                                    : hovered         ? ImGuiCol_HeaderHovered
                                                                      : ImGuiCol_Header);
            ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
            ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);

            if (flags & ImGuiTreeNodeFlags_Bullet)
                ImGui::RenderBullet(
                    window->DrawList,
                    ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f),
                    text_col);
            else if (!is_leaf)
                ImGui::RenderArrow(
                    window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y),
                    text_col,
                    is_open ? ((flags & ImGuiTreeNodeFlags_UpsideDownArrow) ? ImGuiDir_Up
                                                                            : ImGuiDir_Down)
                            : ImGuiDir_Right,
                    1.0f);
            else  // Leaf without bullet, left-adjusted text
                text_pos.x -= text_offset_x - padding.x;
            if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
                frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

            if (g.LogEnabled)
                ImGui::LogSetNextTextDecoration("###", "###");
        } else {
            // Unframed typed for tree nodes
            if (hovered || selected || focused) {
                const ImU32 bg_col =
                    ImGui::GetColorU32((held && hovered)    ? ImGuiCol_HeaderActive
                                       : hovered || focused ? ImGuiCol_HeaderHovered
                                                            : ImGuiCol_Header);
                ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
            }
            ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);

            if (flags & ImGuiTreeNodeFlags_Bullet)
                ImGui::RenderBullet(
                    window->DrawList,
                    ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f),
                    text_col);
            else if (!is_leaf)
                ImGui::RenderArrow(
                    window->DrawList,
                    ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f),
                    text_col,
                    is_open ? ((flags & ImGuiTreeNodeFlags_UpsideDownArrow) ? ImGuiDir_Up
                                                                            : ImGuiDir_Down)
                            : ImGuiDir_Right,
                    0.70f);
            if (g.LogEnabled)
                ImGui::LogSetNextTextDecoration(">", NULL);
        }

        if (span_all_columns)
            ImGui::TablePopBackgroundChannel();

        // Label
        if (display_frame)
            ImGui::RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
        else
            ImGui::RenderText(text_pos, label, label_end, false);

        ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0, 0, 0, 0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0, 0, 0, 0});

        ImGui::RenderText(eye_pos, *visible ? ICON_FK_EYE : ICON_FK_EYE_SLASH, nullptr, false);

        ImGui::PopStyleColor(3);

        if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            ImGui::TreePushOverrideID(id);
        IMGUI_TEST_ENGINE_ITEM_INFO(id, label,
                                    g.LastItemData.StatusFlags |
                                        (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) |
                                        (is_open ? ImGuiItemStatusFlags_Opened : 0));

        return is_open;
    }

    bool SceneTreeNodeEx(const char *label, ImGuiTreeNodeFlags flags, bool focused, bool *visible) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        return SceneTreeNodeBehavior(window->GetID(label), flags, label, NULL, focused, visible);
    }

    SceneWindow::~SceneWindow() {
        // J3DRendering::Cleanup();
        m_renderables.erase(m_renderables.begin(), m_renderables.end());
        m_resource_cache.m_model.clear();
        m_resource_cache.m_material.clear();

        glDeleteFramebuffers(1, &m_fbo_id);
        glDeleteRenderbuffers(1, &m_rbo_id);
        glDeleteTextures(1, &m_tex_id);
    }

    SceneWindow::SceneWindow() : DockWindow() {
        m_camera.setPerspective(150, 16 / 9, 100, 600000);
        m_camera.setOrientAndPosition({0, 1, 0}, {0, 0, 1}, {0, 0, 0});
        m_camera.updateCamera();
        J3DRendering::SetSortFunction(PacketSort);

        // Create framebuffer for this window
        glGenFramebuffers(1, &m_fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);

        buildContextMenuVirtualObj();
        buildContextMenuGroupObj();
        buildContextMenuPhysicalObj();
        buildContextMenuMultiObj();

        buildCreateObjDialog();
        buildRenameObjDialog();

        if (!m_path_renderer.initPathRenderer()) {
            // show some error
        }

        // 128x128 billboards, 10 unique images
        if (!m_billboard_renderer.initBillboardRenderer(128, 10)) {
            // show some error
        }

        m_billboard_renderer.loadBillboardTexture("res/question.png", 0);
    }

    bool SceneWindow::loadData(const std::filesystem::path &path) {
        if (!Toolbox::exists(path)) {
            return false;
        }

        if (Toolbox::is_directory(path)) {
            if (path.filename() != "scene") {
                return false;
            }

            m_resource_cache.m_model.clear();
            m_resource_cache.m_material.clear();

            m_current_scene = std::make_unique<Toolbox::Scene::SceneInstance>(path);

            m_current_scene->dump(std::cout);

            // J3DModelLoader loader;
            // for (const auto &entry : std::filesystem::directory_iterator(path / "mapobj")) {
            //     if (entry.path().extension() == ".bmd") {
            //         bStream::CFileStream modelStream(entry.path().string(),
            //         bStream::Endianess::Big,
            //                                          bStream::OpenMode::In);

            //        auto model_data = loader.Load(&modelStream, 0);
            //        m_resource_cache.m_model.insert({entry.path().stem().string(), model_data});

            //        std::filesystem::path mat_path;
            //        if (entry.path().stem() == "sandbomb") {
            //            mat_path = entry.path().parent_path() / "sandbombbase.bmt";
            //        } else {
            //            mat_path = entry.path().parent_path() / std::format("{}.bmt",
            //            entry.path().stem().string());
            //        }

            //        // TODO: Use json information to load model and material data

            //        if (std::filesystem::is_regular_file(mat_path)) {
            //            bStream::CFileStream matStream(mat_path.string(), bStream::Endianess::Big,
            //                                             bStream::OpenMode::In);

            //            J3DMaterialTableLoader bmtLoader;

            //            std::shared_ptr<J3DMaterialTable> matTable =
            //                bmtLoader.Load(&matStream, model_data);

            //            m_resource_cache.m_material.insert(
            //                {entry.path().stem().string(), matTable});
            //        }
            //    }
            //}

            // for (const auto &entry : std::filesystem::directory_iterator(path / "map" / "map")) {
            //     if (entry.path().extension() == ".bmd") {
            //         std::cout << "[gui]: loading model " << entry.path().filename().string()
            //                   << std::endl;
            //         bStream::CFileStream modelStream(entry.path().string(),
            //         bStream::Endianess::Big,
            //                                          bStream::OpenMode::In);

            //        m_resource_cache.m_model.insert(
            //            {entry.path().stem().string(), loader.Load(&modelStream, 0)});
            //    }
            //}

            if (m_current_scene != nullptr) {
                auto rail_data = m_current_scene->getRailData();

                for (auto rail : rail_data) {
                    for (auto node : rail->nodes()) {
                        std::vector<PathPoint> connections;

                        PathPoint nodePoint(node->getPosition(), glm::vec4(1.0, 0.0, 1.0, 1.0), 64);

                        for (auto connection : rail->getNodeConnections(node)) {
                            PathPoint connectionPoint(connection->getPosition(),
                                                      glm::vec4(1.0, 0.0, 1.0, 1.0), 64);
                            connections.push_back(nodePoint);
                            connections.push_back(connectionPoint);
                        };
                        m_path_renderer.m_paths.push_back(connections);
                    }
                }

                m_billboard_renderer.m_billboards.push_back(
                    Billboard(glm::vec3(0, 1000, 0), 128, 0));
            }
            m_path_renderer.updateGeometry();

            return true;
        }

        // TODO: Implement opening from archives.
        return false;
    }

    bool SceneWindow::update(f32 deltaTime) {
        if (m_is_render_window_hovered && m_is_render_window_focused) {
            if (Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
                ImVec2 mouse_delta = Input::GetMouseDelta();

                m_camera.turnLeftRight(-mouse_delta.x * 0.005f);
                m_camera.tiltUpDown(-mouse_delta.y * 0.005f);

                m_is_viewport_dirty = true;
            }

            float lr_delta = 0, ud_delta = 0;
            if (Input::GetKey(GLFW_KEY_A)) {
                lr_delta -= 1;
                m_is_viewport_dirty = true;
            }
            if (Input::GetKey(GLFW_KEY_D)) {
                lr_delta += 1;
                m_is_viewport_dirty = true;
            }
            if (Input::GetKey(GLFW_KEY_W)) {
                ud_delta += 1;
                m_is_viewport_dirty = true;
            }
            if (Input::GetKey(GLFW_KEY_S)) {
                ud_delta -= 1;
                m_is_viewport_dirty = true;
            }
            if (Input::GetKey(GLFW_KEY_LEFT_SHIFT)) {
                lr_delta *= 10;
                ud_delta *= 10;
            }

            lr_delta *= 10;
            ud_delta *= 10;

            m_camera.translateLeftRight(-lr_delta);
            m_camera.translateFwdBack(ud_delta);
        }

        if (m_render_window_size.x != m_render_window_size_prev.x ||
            m_render_window_size.y != m_render_window_size_prev.y) {
            m_camera.setAspect(m_render_size.y > 0 ? m_render_size.x / m_render_size.y
                                                   : FLT_EPSILON * 2);
            m_is_viewport_dirty = true;
        }

        if (m_is_viewport_dirty)
            m_camera.updateCamera();

        return true;
    }

    void SceneWindow::viewportBegin(bool is_dirty) {
        ImGuiStyle &style = ImGui::GetStyle();

        m_render_window_size_prev = m_render_window_size;
        m_render_window_size      = ImGui::GetWindowSize();
        m_render_size             = ImVec2(m_render_window_size.x - style.WindowPadding.x * 2,
                                           m_render_window_size.y - style.WindowPadding.y * 2 -
                                               (style.FramePadding.y * 2.0f + ImGui::GetTextLineHeight()));

        // bind the framebuffer we want to render to
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);

        if (!is_dirty) {
            glBindTexture(GL_TEXTURE_2D, m_tex_id);
            return;
        }

        auto size_x = static_cast<GLsizei>(m_render_window_size.x);
        auto size_y = static_cast<GLsizei>(m_render_window_size.y);

        // window was resized, new texture and depth storage needed for framebuffer
        glDeleteRenderbuffers(1, &m_rbo_id);
        glDeleteTextures(1, &m_tex_id);

        glGenTextures(1, &m_tex_id);
        glBindTexture(GL_TEXTURE_2D, m_tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_x, size_y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex_id, 0);

        glGenRenderbuffers(1, &m_rbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size_x, size_y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  m_rbo_id);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        glViewport(0, 0, size_x, size_y);
    }

    void SceneWindow::viewportEnd() {
        ImGui::Image(reinterpret_cast<void *>(static_cast<uintptr_t>(m_tex_id)), m_render_size,
                     {0.0f, 1.0f}, {1.0f, 0.0f});

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SceneWindow::renderBody(f32 deltaTime) {
        renderHierarchy();
        renderProperties();
        renderScene(deltaTime);
    }

    void SceneWindow::renderHierarchy() {
        ImGuiWindowClass hierarchyOverride;
        hierarchyOverride.ClassId =
            ImGui::GetID(getWindowChildUID(*this, "Hierarchy Editor").c_str());
        hierarchyOverride.ParentViewportId = m_window_id;
        hierarchyOverride.DockNodeFlagsOverrideSet =
            ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther;
        ImGui::SetNextWindowClass(&hierarchyOverride);

        ImGui::SetNextWindowSizeConstraints({300, 500}, {FLT_MAX, FLT_MAX});

        if (ImGui::Begin(getWindowChildUID(*this, "Hierarchy Editor").c_str())) {
            ImGui::Text("Find Objects");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            m_hierarchy_filter.Draw("##obj_filter");

            ImGui::Text("Map Objects");
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) /* Check if scene is loaded here*/) {
                // Add Object
            }

            ImGui::Separator();

            // Render Objects

            if (m_current_scene != nullptr) {
                auto root = m_current_scene->getObjHierarchy().getRoot();
                renderTree(root);
            }

            ImGui::Spacing();
            ImGui::Text("Scene Info");
            ImGui::Separator();

            if (m_current_scene != nullptr) {
                auto root = m_current_scene->getTableHierarchy().getRoot();
                renderTree(root);
            }
        }
        ImGui::End();

        if (m_selected_nodes.size() > 0) {
            m_create_obj_dialog.render(m_selected_nodes.back());
            m_rename_obj_dialog.render(m_selected_nodes.back());
        }
    }

    void SceneWindow::renderTree(std::shared_ptr<Toolbox::Object::ISceneObject> node) {
        constexpr auto dir_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanFullWidth;

        constexpr auto file_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

        constexpr auto node_flags = dir_flags | ImGuiTreeNodeFlags_DefaultOpen;

        bool multi_select     = Input::GetKey(GLFW_KEY_LEFT_CONTROL);
        bool needs_scene_sync = node->getTransform() ? false : true;

        std::string display_name = std::format("{} ({})", node->type(), node->getNameRef().name());
        bool is_filtered_out     = !m_hierarchy_filter.PassFilter(display_name.c_str());

        std::string node_uid_str = getNodeUID(node);
        ImGuiID tree_node_id     = ImGui::GetCurrentWindow()->GetID(node_uid_str.c_str());

        auto node_it =
            std::find_if(m_selected_nodes.begin(), m_selected_nodes.end(),
                         [&](const NodeInfo &other) { return other.m_node_id == tree_node_id; });
        bool node_already_clicked = node_it != m_selected_nodes.end();

        bool node_visible    = node->getIsPerforming();
        bool node_visibility = node->getCanPerform();

        bool node_open = false;

        NodeInfo node_info = {.m_selected         = node,
                              .m_node_id          = tree_node_id,
                              .m_hierarchy_synced = true,
                              .m_scene_synced =
                                  needs_scene_sync};  // Only spacial objects get scene selection

        if (node->isGroupObject()) {
            if (is_filtered_out) {
                auto objects = std::static_pointer_cast<Toolbox::Object::GroupSceneObject>(node)
                                   ->getChildren();
                if (objects.has_value()) {
                    for (auto object : objects.value()) {
                        renderTree(object);
                    }
                }
            } else {
                if (node_visibility) {
                    node_open = SceneTreeNodeEx(node_uid_str.c_str(),
                                                node->getParent() ? dir_flags : node_flags,
                                                node_already_clicked, &node_visible);
                    if (node->getIsPerforming() != node_visible) {
                        node->setIsPerforming(node_visible);
                        m_is_viewport_dirty = true;
                    }
                } else {
                    node_open = ImGui::TreeNodeEx(node_uid_str.c_str(),
                                                  node->getParent() ? dir_flags : node_flags,
                                                  node_already_clicked);
                }

                renderContextMenu(node_uid_str, node_info);

                if (ImGui::IsItemClicked()) {
                    m_selected_properties.clear();

                    if (multi_select) {
                        if (node_it == m_selected_nodes.end())
                            m_selected_nodes.push_back(node_info);
                    } else {
                        m_selected_nodes.clear();
                        m_selected_nodes.push_back(node_info);
                        for (auto &member : node->getMembers()) {
                            member->syncArray();
                            auto prop = createProperty(member);
                            if (prop) {
                                m_selected_properties.push_back(std::move(prop));
                            }
                        }
                    }
                }

                if (node_open) {
                    auto objects = std::static_pointer_cast<Toolbox::Object::GroupSceneObject>(node)
                                       ->getChildren();
                    if (objects.has_value()) {
                        for (auto object : objects.value()) {
                            renderTree(object);
                        }
                    }
                    ImGui::TreePop();
                }
            }
        } else {
            if (!is_filtered_out) {
                if (node_visibility) {
                    node_open = SceneTreeNodeEx(node_uid_str.c_str(), file_flags,
                                                node_already_clicked, &node_visible);
                    if (node->getIsPerforming() != node_visible) {
                        node->setIsPerforming(node_visible);
                        m_is_viewport_dirty = true;
                    }
                } else {
                    node_open =
                        ImGui::TreeNodeEx(node_uid_str.c_str(), file_flags, node_already_clicked);
                }

                renderContextMenu(node_uid_str, node_info);

                if (node_open) {
                    if (ImGui::IsItemClicked()) {
                        m_selected_properties.clear();

                        if (multi_select) {
                            if (node_it == m_selected_nodes.end())
                                m_selected_nodes.push_back(node_info);
                        } else {
                            m_selected_nodes.clear();
                            m_selected_nodes.push_back(node_info);
                            for (auto &member : node->getMembers()) {
                                member->syncArray();
                                auto prop = createProperty(member);
                                if (prop) {
                                    m_selected_properties.push_back(std::move(prop));
                                }
                            }
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }

    void SceneWindow::renderProperties() {
        ImGuiWindowClass propertiesOverride;
        propertiesOverride.DockNodeFlagsOverrideSet =
            ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther;
        ImGui::SetNextWindowClass(&propertiesOverride);

        ImGui::SetNextWindowSizeConstraints({300, 500}, {FLT_MAX, FLT_MAX});

        if (ImGui::Begin(getWindowChildUID(*this, "Properties Editor").c_str())) {
            float label_width = 0;
            for (auto &prop : m_selected_properties) {
                label_width = std::max(label_width, prop->labelSize().x);
            }
            for (auto &prop : m_selected_properties) {
                m_is_viewport_dirty |= prop->render(label_width);
                ImGui::ItemSize({0, 2});
            }
        }
        ImGui::End();
    }

    void SceneWindow::renderScene(f32 delta_time) {
        m_renderables.clear();

        std::vector<J3DLight> lights;

        // perhaps find a way to limit this so it only happens when we need to re-render?
        if (m_current_scene != nullptr) {
            m_current_scene->getObjHierarchy().getRoot()->performScene(delta_time, m_renderables,
                                                                       m_resource_cache, lights);
        }

        m_is_render_window_open = ImGui::Begin(getWindowChildUID(*this, "Scene View").c_str());
        if (m_is_render_window_open) {
            ImVec2 window_pos    = ImGui::GetWindowPos();
            m_render_window_rect = {
                window_pos,
                {window_pos.x + ImGui::GetWindowWidth(), window_pos.y + ImGui::GetWindowHeight()}
            };

            m_is_render_window_hovered = ImGui::IsWindowHovered();
            m_is_render_window_focused = ImGui::IsWindowFocused();

            if (m_is_render_window_hovered && Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
                ImGui::SetWindowFocus();

            if (m_is_render_window_focused &&
                Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {  // Mouse wrap
                ImVec2 mouse_pos   = ImGui::GetMousePos();
                ImVec2 window_pos  = ImGui::GetWindowPos();
                ImVec2 window_size = ImGui::GetWindowSize();

                bool wrapped = false;

                if (mouse_pos.x < m_render_window_rect.Min.x) {
                    mouse_pos.x += window_size.x;
                    wrapped = true;
                } else if (mouse_pos.x >= m_render_window_rect.Max.x) {
                    mouse_pos.x -= window_size.x;
                    wrapped = true;
                }

                if (mouse_pos.y < m_render_window_rect.Min.y) {
                    mouse_pos.y += window_size.y;
                    wrapped = true;
                } else if (mouse_pos.y >= m_render_window_rect.Max.y) {
                    mouse_pos.y -= window_size.y;
                    wrapped = true;
                }

                ImVec2 viewport_pos = MainApplication::instance().windowScreenPos();
                mouse_pos.x += viewport_pos.x;
                mouse_pos.y += viewport_pos.y;

                if (wrapped) {
                    Input::SetMousePosition(mouse_pos, false);
                }
            }

            viewportBegin(m_is_viewport_dirty);
            if (m_is_viewport_dirty) {
                glm::mat4 projection, view;
                projection = m_camera.getProjMatrix();
                view       = m_camera.getViewMatrix();

                J3DUniformBufferObject::SetProjAndViewMatrices(projection, view);

                glm::vec3 position;
                m_camera.getPos(position);

                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                J3DRendering::Render(delta_time, position, view, projection, m_renderables);

                m_path_renderer.drawPaths(&m_camera);
                m_billboard_renderer.drawBillboards(&m_camera);

                m_is_viewport_dirty = false;
            }
            viewportEnd();
        }
        ImGui::End();
    }

    void SceneWindow::renderContextMenu(std::string str_id, NodeInfo &info) {
        if (m_selected_nodes.size() > 0) {
            NodeInfo &info = m_selected_nodes.back();
            if (m_selected_nodes.size() > 1) {
                m_multi_node_menu.render({}, m_selected_nodes);
            } else if (info.m_selected->isGroupObject()) {
                m_group_node_menu.render(str_id, info);
            } else if (info.m_selected->hasMember("Transform")) {
                m_physical_node_menu.render(str_id, info);
            } else {
                m_virtual_node_menu.render(str_id, info);
            }
        }
    }

    void SceneWindow::buildContextMenuVirtualObj() {
        m_virtual_node_menu = ContextMenu<NodeInfo>();

        m_virtual_node_menu.addOption("Insert Object Here...", [this](NodeInfo info) {
            m_create_obj_dialog.open();
            return std::expected<void, BaseError>();
        });

        m_virtual_node_menu.addDivider();

        m_virtual_node_menu.addOption("Rename...", [this](NodeInfo info) {
            m_rename_obj_dialog.open();
            m_rename_obj_dialog.setOriginalName(info.m_selected->getNameRef().name());
            return std::expected<void, BaseError>();
        });

        m_virtual_node_menu.addDivider();

        m_virtual_node_menu.addOption("Copy", [this](NodeInfo info) {
            info.m_selected = make_deep_clone<ISceneObject>(info.m_selected);
            MainApplication::instance().getSceneClipboard().setData(info);
            return std::expected<void, BaseError>();
        });

        m_virtual_node_menu.addOption("Paste", [this](NodeInfo info) {
            auto nodes       = MainApplication::instance().getSceneClipboard().getData();
            auto this_parent = reinterpret_cast<GroupSceneObject *>(info.m_selected->getParent());
            if (!this_parent) {
                return make_error<void>("Scene Hierarchy", "Failed to get parent node for pasting");
            }
            for (auto &node : nodes) {
                this_parent->addChild(node.m_selected);
            }
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });

        m_virtual_node_menu.addDivider();

        m_virtual_node_menu.addOption("Delete", [this](NodeInfo info) {
            auto this_parent = reinterpret_cast<GroupSceneObject *>(info.m_selected->getParent());
            if (!this_parent) {
                return make_error<void>("Scene Hierarchy", "Failed to get parent node for pasting");
            }
            this_parent->removeChild(info.m_selected->getNameRef().name());
            auto node_it = std::find(m_selected_nodes.begin(), m_selected_nodes.end(), info);
            m_selected_nodes.erase(node_it);
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });
    }

    void SceneWindow::buildContextMenuGroupObj() {
        m_group_node_menu = ContextMenu<NodeInfo>();

        m_group_node_menu.addOption("Add Child Object...", [this](NodeInfo info) {
            m_create_obj_dialog.open();
            return std::expected<void, BaseError>();
        });

        m_group_node_menu.addDivider();

        m_group_node_menu.addOption("Rename...", [this](NodeInfo info) {
            m_rename_obj_dialog.open();
            m_rename_obj_dialog.setOriginalName(info.m_selected->getNameRef().name());
            return std::expected<void, BaseError>();
        });

        m_group_node_menu.addDivider();

        m_group_node_menu.addOption("Copy", [this](NodeInfo info) {
            info.m_selected = make_deep_clone<ISceneObject>(info.m_selected);
            MainApplication::instance().getSceneClipboard().setData(info);
            return std::expected<void, BaseError>();
        });

        m_group_node_menu.addOption("Paste", [this](NodeInfo info) {
            auto nodes       = MainApplication::instance().getSceneClipboard().getData();
            auto this_parent = std::reinterpret_pointer_cast<GroupSceneObject>(info.m_selected);
            if (!this_parent) {
                return make_error<void>("Scene Hierarchy", "Failed to get parent node for pasting");
            }
            for (auto &node : nodes) {
                this_parent->addChild(node.m_selected);
            }
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });

        m_group_node_menu.addDivider();

        m_group_node_menu.addOption("Delete", [this](NodeInfo info) {
            auto this_parent = reinterpret_cast<GroupSceneObject *>(info.m_selected->getParent());
            if (!this_parent) {
                return make_error<void>("Scene Hierarchy", "Failed to get parent node for pasting");
            }
            this_parent->removeChild(info.m_selected->getNameRef().name());
            auto node_it = std::find(m_selected_nodes.begin(), m_selected_nodes.end(), info);
            m_selected_nodes.erase(node_it);
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });
    }

    void SceneWindow::buildContextMenuPhysicalObj() {
        m_physical_node_menu = ContextMenu<NodeInfo>();

        m_physical_node_menu.addOption("Insert Object Here...", [this](NodeInfo info) {
            m_create_obj_dialog.open();
            return std::expected<void, BaseError>();
        });

        m_physical_node_menu.addDivider();

        m_physical_node_menu.addOption("Rename...", [this](NodeInfo info) {
            m_rename_obj_dialog.open();
            m_rename_obj_dialog.setOriginalName(info.m_selected->getNameRef().name());
            return std::expected<void, BaseError>();
        });

        m_physical_node_menu.addDivider();

        m_physical_node_menu.addOption("View in Scene", [this](NodeInfo info) {
            auto member_result = info.m_selected->getMember("Transform");
            if (!member_result) {
                return make_error<void>("Scene Hierarchy",
                                        "Failed to find transform member of physical object");
            }
            auto member_ptr = member_result.value();
            if (!member_ptr) {
                return make_error<void>("Scene Hierarchy",
                                        "Found the transform member but it was null");
            }
            Transform transform = getMetaValue<Transform>(member_ptr).value();
            f32 max_scale       = std::max(transform.m_scale.x, transform.m_scale.y);
            max_scale           = std::max(max_scale, transform.m_scale.z);

            m_camera.setOrientAndPosition({0, 1, 0}, transform.m_translation,
                                          {transform.m_translation.x, transform.m_translation.y,
                                           transform.m_translation.z + 1000 * max_scale});
            m_camera.updateCamera();
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });

        m_physical_node_menu.addOption("Move to Camera", [this](NodeInfo info) {
            auto member_result = info.m_selected->getMember("Transform");
            if (!member_result) {
                return make_error<void>("Scene Hierarchy",
                                        "Failed to find transform member of physical object");
            }
            auto member_ptr = member_result.value();
            if (!member_ptr) {
                return make_error<void>("Scene Hierarchy",
                                        "Found the transform member but it was null");
            }

            Transform transform = getMetaValue<Transform>(member_ptr).value();
            m_camera.getPos(transform.m_translation);
            setMetaValue<Transform>(member_ptr, 0, transform);

            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });

        m_physical_node_menu.addDivider();

        m_physical_node_menu.addOption("Copy", [this](NodeInfo info) {
            info.m_selected = make_deep_clone<ISceneObject>(info.m_selected);
            MainApplication::instance().getSceneClipboard().setData(info);
            return std::expected<void, BaseError>();
        });

        m_physical_node_menu.addOption("Paste", [this](NodeInfo info) {
            auto nodes       = MainApplication::instance().getSceneClipboard().getData();
            auto this_parent = reinterpret_cast<GroupSceneObject *>(info.m_selected->getParent());
            if (!this_parent) {
                return make_error<void>("Scene Hierarchy", "Failed to get parent node for pasting");
            }
            for (auto &node : nodes) {
                this_parent->addChild(node.m_selected);
            }
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });

        m_physical_node_menu.addDivider();

        m_physical_node_menu.addOption("Delete", [this](NodeInfo info) {
            auto this_parent = reinterpret_cast<GroupSceneObject *>(info.m_selected->getParent());
            if (!this_parent) {
                return make_error<void>("Scene Hierarchy", "Failed to get parent node for pasting");
            }
            this_parent->removeChild(info.m_selected->getNameRef().name());
            auto node_it = std::find(m_selected_nodes.begin(), m_selected_nodes.end(), info);
            m_selected_nodes.erase(node_it);
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });
    }

    void SceneWindow::buildContextMenuMultiObj() {
        m_multi_node_menu = ContextMenu<std::vector<NodeInfo>>();

        m_multi_node_menu.addOption("Copy", [this](std::vector<NodeInfo> infos) {
            for (auto &info : infos) {
                info.m_selected = make_deep_clone<ISceneObject>(info.m_selected);
            }
            MainApplication::instance().getSceneClipboard().setData(infos);
            return std::expected<void, BaseError>();
        });

        m_multi_node_menu.addOption("Paste", [this](std::vector<NodeInfo> infos) {
            auto nodes = MainApplication::instance().getSceneClipboard().getData();
            for (auto &info : infos) {
                auto this_parent =
                    reinterpret_cast<GroupSceneObject *>(info.m_selected->getParent());
                if (!this_parent) {
                    return make_error<void>("Scene Hierarchy",
                                            "Failed to get parent node for pasting");
                }
                for (auto &node : nodes) {
                    this_parent->addChild(node.m_selected);
                }
            }
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });

        m_multi_node_menu.addDivider();

        m_multi_node_menu.addOption("Delete", [this](std::vector<NodeInfo> infos) {
            for (auto &info : infos) {
                auto this_parent =
                    reinterpret_cast<GroupSceneObject *>(info.m_selected->getParent());
                if (!this_parent) {
                    return make_error<void>("Scene Hierarchy",
                                            "Failed to get parent node for pasting");
                }
                this_parent->removeChild(info.m_selected->getNameRef().name());
                auto node_it = std::find(m_selected_nodes.begin(), m_selected_nodes.end(), info);
                m_selected_nodes.erase(node_it);
            }
            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });
    }

    void SceneWindow::buildCreateObjDialog() {
        m_create_obj_dialog.setup();
        m_create_obj_dialog.setActionOnAccept([this](std::string_view name,
                                                     const Object::Template &template_,
                                                     std::string_view wizard_name, NodeInfo info) {
            auto new_object_result = Object::ObjectFactory::create(template_, wizard_name);
            if (!name.empty()) {
                new_object_result->setNameRef(name);
            }

            ISceneObject *this_parent;
            if (info.m_selected->isGroupObject()) {
                this_parent = info.m_selected.get();
            } else {
                this_parent = info.m_selected->getParent();
            }

            if (!this_parent) {
                return make_error<void>("Scene Hierarchy",
                                        "Failed to get parent node for obj creation");
            }

            auto result = this_parent->addChild(std::move(new_object_result));
            if (!result) {
                return make_error<void>("Scene Hierarchy",
                                        std::format("Parent already has a child named {}", name));
            }

            m_is_viewport_dirty = true;
            return std::expected<void, BaseError>();
        });
        m_create_obj_dialog.setActionOnReject([](NodeInfo) {});
    }

    void SceneWindow::buildRenameObjDialog() {
        m_rename_obj_dialog.setup();
        m_rename_obj_dialog.setActionOnAccept([this](std::string_view new_name, NodeInfo info) {
            if (new_name.empty()) {
                return make_error<void>("Scene Hierarchy", "Can not rename object to empty string");
            }

            info.m_selected->setNameRef(new_name);

            return std::expected<void, BaseError>();
        });
        m_rename_obj_dialog.setActionOnReject([](NodeInfo) {});
    }

    void SceneWindow::buildDockspace(ImGuiID dockspace_id) {
        m_dock_node_left_id =
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
        m_dock_node_up_left_id   = ImGui::DockBuilderSplitNode(m_dock_node_left_id, ImGuiDir_Down,
                                                               0.5f, nullptr, &m_dock_node_left_id);
        m_dock_node_down_left_id = ImGui::DockBuilderSplitNode(
            m_dock_node_up_left_id, ImGuiDir_Down, 0.5f, nullptr, &m_dock_node_up_left_id);
        ImGui::DockBuilderDockWindow(getWindowChildUID(*this, "Scene View").c_str(), dockspace_id);
        ImGui::DockBuilderDockWindow(getWindowChildUID(*this, "Hierarchy Editor").c_str(),
                                     m_dock_node_up_left_id);
        ImGui::DockBuilderDockWindow(getWindowChildUID(*this, "Properties Editor").c_str(),
                                     m_dock_node_down_left_id);
    }

    void SceneWindow::renderMenuBar() {}

};  // namespace Toolbox::UI