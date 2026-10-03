/*
 * PS5 Vulkan Template - the UI module: the samples' settings windows in the title's theme.
 *
 * The samples draw their settings with Dear ImGui (base/VulkanUIOverlay.cpp).
 * These two functions, which the overlay calls on the console, give ImGui the
 * title's theme (ps5ui::active_theme): its palette, its corners and borders,
 * and its typeface. ImGui has no frosted glass, glows or bevels, so a theme
 * reaches it as colours and shapes; the kit's own screens draw the rest.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"

namespace {

ImVec4 colour(hui::gfx::Color c, float alpha = 1.0f)
{
	return ImVec4(c.r, c.g, c.b, c.a * alpha);
}

ImVec4 blend(hui::gfx::Color a, hui::gfx::Color b, float t, float alpha = 1.0f)
{
	return colour(hui::gfx::mix(a, b, t), alpha);
}

} // namespace

void ps5StyleOverlay(ImGuiStyle &style)
{
	const hui::ui::Theme &t = ps5ui::active_theme();
	ImVec4 *c = style.Colors;
	c[ImGuiCol_Text] = colour(t.text);
	c[ImGuiCol_TextDisabled] = colour(t.text_muted);
	c[ImGuiCol_WindowBg] = colour(t.surface, 0.94f);
	c[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	c[ImGuiCol_PopupBg] = colour(t.surface, 0.98f);
	c[ImGuiCol_Border] = colour(t.outline, t.border > 0.0f ? 1.0f : 0.5f);
	c[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	c[ImGuiCol_FrameBg] = colour(t.surface_high);
	c[ImGuiCol_FrameBgHovered] = blend(t.surface_high, t.accent, 0.15f);
	c[ImGuiCol_FrameBgActive] = blend(t.surface_high, t.accent, 0.3f);
	// The title bar carries the window's name in the text colour: a raised surface
	c[ImGuiCol_TitleBg] = colour(t.surface_high);
	c[ImGuiCol_TitleBgActive] = blend(t.surface_high, t.primary, 0.2f);
	c[ImGuiCol_TitleBgCollapsed] = colour(t.surface_high, 0.7f);
	c[ImGuiCol_MenuBarBg] = colour(t.surface_high);
	c[ImGuiCol_ScrollbarBg] = colour(t.surface, 0.5f);
	c[ImGuiCol_ScrollbarGrab] = colour(t.outline);
	c[ImGuiCol_ScrollbarGrabHovered] = colour(t.text_muted);
	c[ImGuiCol_ScrollbarGrabActive] = colour(t.accent);
	c[ImGuiCol_CheckMark] = colour(t.accent);
	c[ImGuiCol_SliderGrab] = colour(t.accent);
	c[ImGuiCol_SliderGrabActive] = colour(t.primary);
	c[ImGuiCol_Button] = colour(t.secondary);
	c[ImGuiCol_ButtonHovered] = blend(t.secondary, t.primary, 0.35f);
	c[ImGuiCol_ButtonActive] = colour(t.primary);
	c[ImGuiCol_Header] = colour(t.accent, 0.3f);
	c[ImGuiCol_HeaderHovered] = colour(t.accent, 0.45f);
	c[ImGuiCol_HeaderActive] = colour(t.accent, 0.6f);
	c[ImGuiCol_Separator] = colour(t.outline);
	c[ImGuiCol_SeparatorHovered] = colour(t.accent, 0.7f);
	c[ImGuiCol_SeparatorActive] = colour(t.accent);
	c[ImGuiCol_ResizeGrip] = colour(t.outline, 0.4f);
	c[ImGuiCol_ResizeGripHovered] = colour(t.accent, 0.6f);
	c[ImGuiCol_ResizeGripActive] = colour(t.accent);
	c[ImGuiCol_PlotLines] = colour(t.accent);
	c[ImGuiCol_PlotHistogram] = colour(t.accent);
	c[ImGuiCol_TextSelectedBg] = colour(t.accent, 0.35f);
	c[ImGuiCol_NavHighlight] = colour(t.focus.a > 0.0f ? t.focus : t.accent);
	c[ImGuiCol_ModalWindowDimBg] = colour(t.shadow, 0.5f);

	// Shapes, in ImGui's units before the overlay's scale (2.5 at 3840x2160,
	// so one unit is 1.25 of the kit's 1920x1080 pixels). Square and notched
	// corners stay square; a pill radius becomes a full round.
	const bool square = t.corner != hui::ui::Corner::round;
	const float unit = 1.0f / 1.25f;
	const float control = square ? 0.0f : t.radius >= 100.0f ? 12.0f : t.radius * unit;
	style.WindowRounding = square ? 0.0f : std::min(t.radius_card * unit, 16.0f);
	style.ChildRounding = style.WindowRounding;
	style.PopupRounding = style.WindowRounding;
	style.FrameRounding = std::min(control, 12.0f);
	style.GrabRounding = style.FrameRounding;
	style.ScrollbarRounding = square ? 0.0f : 9.0f;
	style.WindowBorderSize = t.border > 0.0f ? 1.0f : 0.0f;
	style.FrameBorderSize = (t.button_border >= 0.0f ? t.button_border : t.border) > 0.0f ? 1.0f : 0.0f;
	style.PopupBorderSize = style.WindowBorderSize;
}

bool ps5OverlayFont(std::string &path, float &size)
{
	// The theme's label face, in the TTF the kit was baked from
	const std::string fonts = std::string(VK_EXAMPLE_ASSETS_DIR) + "hui/fonts/";
	switch (ps5ui::active_theme().label) {
	case hui::ui::FontRole::pixel:
		path = fonts + "PressStart2P-Regular.ttf";
		size = 10.0f;
		break;
	case hui::ui::FontRole::hand:
		path = fonts + "PatrickHand-Regular.ttf";
		size = 20.0f;
		break;
	case hui::ui::FontRole::regular:
		path = fonts + "Inter-Regular.ttf";
		size = 16.0f;
		break;
	case hui::ui::FontRole::mono:
		path = fonts + "DejaVuSansMono.ttf";
		size = 15.0f;
		break;
	default:
		path = fonts + "Inter-SemiBold.ttf";
		size = 16.0f;
		break;
	}
	return true;
}
