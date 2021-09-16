#include "std_include.hpp"

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

cmainframe* cmainframe::activewnd;

void cmainframe::routine_processing()
{
	if (!this->m_bDoLoop)
	{
		return;
	}

	if (0.0 == game::g_time)
	{
		game::g_time = 0.0;
	}

	if (0.0 == game::g_oldtime)
	{
		game::g_oldtime = 0.0;
	}

	const double time = clock() / 1000.0;
	double oldtime = time - game::g_time;

	game::g_time = time;

	if (oldtime > 2.0)
	{
		oldtime = 0.1;
	}

	game::g_oldtime = oldtime;

	if (oldtime > 0.2)
	{
		oldtime = 0.2;
	}

	if (this->m_pCamWnd)
	{
		const auto delta = static_cast<float>(oldtime);
		this->m_pCamWnd->mouse_control(delta);
	}

	if (game::g_nUpdateBits)
	{
		const int nBits = game::g_nUpdateBits;
		game::g_nUpdateBits = 0;
		this->update_windows(nBits);
	}

	game::glob::radiant_initiated = true;

	// toggle toolbar by dvar
	if (cmainframe::activewnd->m_wndToolBar.m_hWnd)
	{
		typedef void(__thiscall* CFrameWnd_ShowControlBar_t)(CFrameWnd*, CControlBar*, BOOL bShow, BOOL bDelay);
					  const auto CFrameWnd_ShowControlBar = reinterpret_cast<CFrameWnd_ShowControlBar_t>(0x59E9DD);
		
		bool is_visible = reinterpret_cast<CToolBar_vtbl*>(cmainframe::activewnd->m_wndToolBar.__vftable)->IsVisible(&cmainframe::activewnd->m_wndToolBar);

		if (dvars::mainframe_show_toolbar->current.enabled && !is_visible)
		{
			CFrameWnd_ShowControlBar(cmainframe::activewnd, &cmainframe::activewnd->m_wndToolBar, 1, 1);
		}
		else if(!dvars::mainframe_show_toolbar->current.enabled && is_visible)
		{
			CFrameWnd_ShowControlBar(cmainframe::activewnd, &cmainframe::activewnd->m_wndToolBar, 0, 1);
		}
	}

	// toggle menubar by dvar
	if (dvars::mainframe_show_menubar->current.enabled && !ggui::mainframe_menubar_enabled)
	{
		components::command::execute("menubar_show");
		game::CPrefsDlg_SavePrefs();
	}
	else if(!dvars::mainframe_show_menubar->current.enabled && ggui::mainframe_menubar_enabled)
	{
		components::command::execute("menubar_hide");
		game::CPrefsDlg_SavePrefs();
	}
}

void __declspec(naked) cmainframe::hk_routine_processing(void)
{
	__asm
	{
		push	ecx;
		mov		ecx, eax; // eax = this :: ecx => this(__stdcall)

		mov		cmainframe::activewnd, ecx; // update activewnd

		call	cmainframe::routine_processing;
		pop		ecx;
		retn;
	}
}

void on_createclient()
{
	const auto prefs = game::g_PrefsDlg();

	if (!prefs->m_nView || prefs->m_nView == 3)
	{
		if(!dvars::mainframe_show_console->current.enabled)
		{
			// disable the console pane
			reinterpret_cast<CSplitterWnd_vtbl*>(cmainframe::activewnd->m_wndSplit.__vftable)->DeleteRow(&cmainframe::activewnd->m_wndSplit, 1);
		}
		
		if(!dvars::mainframe_show_zview->current.enabled)
		{
			if (!prefs->m_nView)
			{
				reinterpret_cast<CSplitterWnd_vtbl*>(cmainframe::activewnd->m_wndSplit.__vftable)->DeleteColumn(&cmainframe::activewnd->m_wndSplit2, 0);
			}
			else
			{
				// if left <-> right switched (second mode / nView 3)
				reinterpret_cast<CSplitterWnd_vtbl*>(cmainframe::activewnd->m_wndSplit.__vftable)->DeleteColumn(&cmainframe::activewnd->m_wndSplit2, 2);
			}
		}
	}
}

void __declspec(naked) hk_on_createclient()
{
	const static uint32_t retn_pt = 0x4232F3;
	__asm
	{
		pushad;
		call	on_createclient;
		popad;

		mov     eax, 1; // og
		jmp		retn_pt;
	}
}


bool worldspawn_on_key_change(const game::epair_t* epair, const char* key, float* value, const int &valueSize)
{
	if (!utils::Q_stricmp(epair->key, key))
	{
		bool changed = false;
		
		std::vector<std::string> KeyValues = utils::explode(epair->value, ' ');

		int count = KeyValues.size();
		if (count > valueSize) count = valueSize;

		for (auto i = 0; i < count; i++)
		{
			float temp = utils::try_stof(KeyValues[i], true);

			if (value[i] != temp)
			{
				value[i] = temp;
				changed = true;
			}
		}

		if (changed)
		{
			return true;
		}
	}
	
	return false;
}

void track_worldspawn_settings()
{
	// track_worldspawn
	
	if (const auto world = GET_WORLDENTITY; 
		world && world->firstActive->eclass->name)
	{
		if (!utils::Q_stricmp(world->firstActive->eclass->name, "worldspawn"))
		{
			for (auto epair = world->firstActive->epairs; epair; epair = epair->next)
			{
				if (worldspawn_on_key_change(epair, "sundirection", game::glob::track_worldspawn.sundirection, 3))
				{
					if (game::glob::track_worldspawn.initiated)
					{
						components::remote_net::Cmd_SendDvar(utils::va("{\n\"dvarname\" \"%s\"\n\"value\" \"%.1f %.1f %.1f\"\n}", "r_lighttweaksundirection",
							game::glob::track_worldspawn.sundirection[0], game::glob::track_worldspawn.sundirection[1], game::glob::track_worldspawn.sundirection[2]));
					}
				}

				if (worldspawn_on_key_change(epair, "suncolor", game::glob::track_worldspawn.suncolor, 3))
				{
					if (game::glob::track_worldspawn.initiated)
					{
						components::remote_net::Cmd_SendDvar(utils::va("{\n\"dvarname\" \"%s\"\n\"value\" \"%.1f %.1f %.1f\"\n}", "r_lighttweaksuncolor",
							game::glob::track_worldspawn.suncolor[0], game::glob::track_worldspawn.suncolor[1], game::glob::track_worldspawn.suncolor[2]));
					}
				}

				if (worldspawn_on_key_change(epair, "sunlight", &game::glob::track_worldspawn.sunlight, 1))
				{
					if (game::glob::track_worldspawn.initiated)
					{
						components::remote_net::Cmd_SendDvar(utils::va("{\n\"dvarname\" \"%s\"\n\"value\" \"%.1f\"\n}", "r_lighttweaksunlight",
							game::glob::track_worldspawn.sunlight));
					}
				}
			}
		}

		if (!game::glob::track_worldspawn.initiated)
		{
			game::glob::track_worldspawn.initiated = true;
			return;
		}
	}
}


void cmainframe::update_windows(int nBits)
{
	// grab camera if not using floating windows
	if (!game::glob::radiant_floatingWindows && this->m_pCamWnd)
	{
		ccamwnd::activewnd = this->m_pCamWnd;
		game::glob::radiant_floatingWindows = true;
	}

	if (!game::g_bScreenUpdates)
	{
		return;
	}

	if (game::glob::live_connected) 
	{
		track_worldspawn_settings();
	}

	// check d3d device or we ASSERT in R_CheckHwnd_or_Device (!dx_device) when using floating windows
	if (!game::glob::d3d9_device)
	{
		return;
	}
	
	if (nBits & (W_XY | W_XY_OVERLAY))
	{
		if (this->m_pXYWnd)
		{
			m_pXYWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	if (nBits & W_CAMERA || ((nBits & W_CAMERA_IFON) && this->m_bCamPreview))
	{
		if (this->m_pCamWnd)
		{
			// redraw the camera when not running at realtime
			if(!game::glob::ccamwindow_realtime)
			{
				// Redraw the camera view
				m_pCamWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
			}
			
			game::glob::m_pCamWnd_ref = m_pCamWnd;

			// on update cam window through the 2d grid or something else
			if ((nBits & W_CAMERA_IFON) && this->m_bCamPreview || nBits < 0 || nBits == 3)
			{
			}

			// only update the remote cam when we actually move it, not when we update the cam window by doing something in the gridWnd etc.
			else if(game::glob::live_connected)
			{
				// Attempt to update the remote camera
				if (ccamwnd::activewnd)
				{
					components::remote_net::cmd_send_camera_update(ccamwnd::activewnd->camera.origin, ccamwnd::activewnd->camera.angles);
				}
			}
		}
	}

	if (nBits & (W_Z | W_Z_OVERLAY))
	{
		if (this->m_pZWnd)
		{
			m_pZWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	if (nBits & W_TEXTURE)
	{
		if (this->m_pTexWnd) 
		{
			m_pTexWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}
}





#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

BOOL is_mouse_outside_window(HWND hWnd)
{
	RECT  cr;
	DWORD msgpos = GetMessagePos();
	POINT pt = { GET_X_LPARAM(msgpos), GET_Y_LPARAM(msgpos) };

	ScreenToClient(hWnd, &pt);
	GetClientRect(hWnd, &cr);
	
	return !PtInRect(&cr, pt);
}


// typedef CFrameWnd::DefWindowProc
typedef LRESULT(__thiscall* wndproc_t)(cmainframe*, UINT Msg, WPARAM wParam, LPARAM lParam);
/* ------------------------- */ wndproc_t o_wndproc = reinterpret_cast<wndproc_t>(0x584D97);

/*
 * only active if clicked outside -> clicked imgui directly, not active if clicked into xywnd or camwnd and then into imgui
 */

// handle wm_char events for non-focused subwindows, see above msg
LRESULT __fastcall cmainframe::windowproc(cmainframe* pThis, [[maybe_unused]] void* edx, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static TRACKMOUSEEVENT tme{};
	static BOOL mouse_tracing = false;
	
	if (components::gui::all_contexts_ready())
	{
		// ! do not set imgui context for every msg

		if (Msg == WM_SETCURSOR)
		{
			// auto close mainframe menubar popups when the mouse leaves the cxy window
			if (cmainframe::activewnd && cmainframe::activewnd->m_pXYWnd)
			{
				if (is_mouse_outside_window(cmainframe::activewnd->m_pXYWnd->GetWindow()))
				{
					const auto imgui_context_old = ImGui::GetCurrentContext();

					IMGUI_BEGIN_CXYWND;
					const auto context = ImGui::GetCurrentContext();

					// TODO! we might 'loose focus' on < 5 FPS, preventing imgui mouse input 
					if (context->OpenPopupStack.Size > 0)
					{
						//printf("closing popup #%d\n", context->OpenPopupStack.Size - 1);
						ImGui::ClosePopupToLevel(context->OpenPopupStack.Size - 1, false);
					}

					// restore context
					ImGui::SetCurrentContext(imgui_context_old);
				}
			}
		}

		
		if (Msg == WM_CHAR || Msg == WM_KEYDOWN || Msg == WM_KEYUP)
		{
			IMGUI_BEGIN_CCAMERAWND;
			if (ImGui::GetIO().WantCaptureMouse)
			{
				ImGui_ImplWin32_WndProcHandler(pThis->GetWindow(), Msg, wParam, lParam);
				return true;
			}
			else // reset io.KeysDown if cursor moved out of imgui window (fixes stuck keys)
			{
				ImGuiIO& io = ImGui::GetIO();
				memset(io.KeysDown, 0, sizeof(io.KeysDown));
			}

			
			IMGUI_BEGIN_CXYWND;
			if (ImGui::GetIO().WantCaptureMouse)
			{
				ImGui_ImplWin32_WndProcHandler(pThis->GetWindow(), Msg, wParam, lParam);
				return true;
			}
			else // reset io.KeysDown if cursor moved out of imgui window (fixes stuck keys)
			{
				ImGuiIO& io = ImGui::GetIO();
				memset(io.KeysDown, 0, sizeof(io.KeysDown));
			}
		}
	}

	// => CFrameWnd::DefWindowProc
	return o_wndproc(pThis, Msg, wParam, lParam);
}


// *
// | -------------------- MSG typedefs ------------------------
// *

namespace mainframe
{
	on_cmainframe_scroll	__on_mscroll;
	on_cmainframe_keydown	__on_keydown;
	on_cmainframe_keyup		__on_keyup;
	
}

// *
// | -------------------- Mouse Scroll ------------------------
// *

BOOL __fastcall cmainframe::on_mscroll(cmainframe* pThis, [[maybe_unused]] void* edx, UINT nFlags, SHORT zDelta, CPoint point)
{
	IMGUI_BEGIN_CCAMERAWND;
	if (ImGui::GetIO().WantCaptureMouse)
	{
		ImGui::HandleKeyIO(pThis->GetWindow(), WM_MOUSEWHEEL, zDelta);
		return 1;
	}

	
	IMGUI_BEGIN_CXYWND;
	if (ImGui::GetIO().WantCaptureMouse)
	{
		ImGui::HandleKeyIO(pThis->GetWindow(), WM_MOUSEWHEEL, zDelta);
		return 1;
	}

	return mainframe::__on_mscroll(pThis, nFlags, zDelta, point);
}

// *
// | ------------------------ Key ----------------------------
// *

void __fastcall cmainframe::on_keydown(cmainframe* pThis, [[maybe_unused]] void* edx, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	IMGUI_BEGIN_CCAMERAWND;
	if (ImGui::GetIO().WantCaptureMouse)
	{
		ImGui::HandleKeyIO(cmainframe::activewnd->m_pCamWnd->GetWindow(), WM_KEYDOWN, 0, nChar);
		return;
	}
	else // reset io.KeysDown if cursor moved out of imgui window (fixes stuck keys)
	{
		ImGuiIO& io = ImGui::GetIO();
		memset(io.KeysDown, 0, sizeof(io.KeysDown));
	}


	IMGUI_BEGIN_CXYWND;

	// handle imgui-camera window
	if (ggui::cxy_camerawnd.window_hovered)
	{
		// calls the original on_keydown function
		mainframe::__on_keydown(cmainframe::activewnd, nChar, nRepCnt, nFlags);
		return;
	}

	
	// if mouse cursor above any cxy imgui window
	if (ImGui::GetIO().WantCaptureMouse)
	{
		ImGui::HandleKeyIO(cmainframe::activewnd->m_pXYWnd->GetWindow(), WM_KEYDOWN, 0, nChar);
		return;
	}

	// reset io.KeysDown if cursor moved out of imgui window (fixes stuck keys)
	else 
	{
		ImGuiIO& io = ImGui::GetIO();
		memset(io.KeysDown, 0, sizeof(io.KeysDown));
	}
	
	return mainframe::__on_keydown(pThis, nChar, nRepCnt, nFlags);
}


void __stdcall cmainframe::on_keyup(cmainframe* pThis, UINT nChar)
{
	IMGUI_BEGIN_CCAMERAWND;
	if (ImGui::GetIO().WantCaptureMouse)
	{
		ImGui::HandleKeyIO(cmainframe::activewnd->m_pCamWnd->GetWindow(), WM_KEYUP, 0, nChar);
		return;
	}

	
	IMGUI_BEGIN_CXYWND;
	
	// handle imgui-camera window
	if (ggui::cxy_camerawnd.window_hovered)
	{
		// calls the original on_keyup function
		return mainframe::__on_keyup(cmainframe::activewnd, nChar);
	}


	// if mouse cursor above any cxy imgui window
	if (ImGui::GetIO().WantCaptureMouse)
	{
		ImGui::HandleKeyIO(cmainframe::activewnd->m_pXYWnd->GetWindow(), WM_KEYUP, 0, nChar);
		return;
	}

	return mainframe::__on_keyup(pThis, nChar);
}

// *
// | ----------------------------------------------------------
// *


// check for nullptr (world_entity)
void __declspec(naked) sunlight_preview_arg_check()
{
	const static uint32_t retn_pt = 0x4067D0;
	const static uint32_t onzero_retn_pt = 0x4067E0;
	__asm
	{
		mov		[ebp - 20DCh], ecx; // og
		
		pushad;
		test	edx, edx;			// world_entity
		jz		ENT_IS_ZERO;

		popad;
		mov     esi, [edx + 8];		// og
		jmp		retn_pt;

		ENT_IS_ZERO:
		popad;
		jmp		onzero_retn_pt;
	}
}


void cmainframe::register_dvars()
{
	dvars::mainframe_show_console = dvars::register_bool(
		/* name		*/ "mainframe_show_console",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the console splitter-pane");

	dvars::mainframe_show_zview = dvars::register_bool(
		/* name		*/ "mainframe_show_zview",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the z-view splitter-pane");

	dvars::mainframe_show_toolbar = dvars::register_bool(
		/* name		*/ "mainframe_show_toolbar",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the toolbar");

	dvars::mainframe_show_menubar = dvars::register_bool(
		/* name		*/ "mainframe_show_menubar",
		/* default	*/ false,
		/* flags	*/ game::dvar_flags::saved,
		/* desc		*/ "show the menubar");
}

void cmainframe::main()
{
	// hook continuous thread
	utils::hook(0x421A90, cmainframe::hk_routine_processing, HOOK_JUMP).install()->quick();

	// handle wm_char events for non-focused subwindows
	utils::hook(0x421A7B, cmainframe::windowproc, HOOK_CALL).install()->quick();

	// hook end of createclient
	utils::hook(0x4232EE, hk_on_createclient, HOOK_JUMP).install()->quick();
	
	// *
	// detour cmainframe member functions to get imgui input

	mainframe::__on_mscroll	= reinterpret_cast<mainframe::on_cmainframe_scroll> (utils::hook::detour(0x42B850, cmainframe::on_mscroll, HK_JUMP));
	mainframe::__on_keydown	= reinterpret_cast<mainframe::on_cmainframe_keydown>(utils::hook::detour(0x422370, cmainframe::on_keydown, HK_JUMP));
	mainframe::__on_keyup	= reinterpret_cast<mainframe::on_cmainframe_keyup>  (utils::hook::detour(0x422270, cmainframe::on_keyup, HK_JUMP));

	// check for nullptr (world_entity) in a sunlight preview function. Only required with the ^ hook, see note there.
	utils::hook::nop(0x4067C7, 6);
		 utils::hook(0x4067C7, sunlight_preview_arg_check, HOOK_JUMP).install()->quick();

	
	// *
	// | ------------------------ Commands ----------------------------
	// *
	
	components::command::register_command("menubar_show"s, [](std::vector<std::string> args)
	{
		const auto menubar = LoadMenu(cmainframe::activewnd->m_pModuleState->m_hCurrentInstanceHandle, MAKEINTRESOURCE(0xD6)); // 0xD6 = IDR_MENU_QUAKE3

		SetMenu(cmainframe::activewnd->GetWindow(), menubar);
		
		ggui::mainframe_menubar_enabled = true;
		dvars::set_bool(dvars::mainframe_show_menubar, true);
	});

	components::command::register_command("menubar_hide"s, [](std::vector<std::string> args)
	{
		// destroy the menu or radiant crashes on shutdown when its trying to get the menubar style
		if (const auto menu = utils::hook::call<CMenu * (__fastcall)(cmainframe*)>(0x42EE20)(cmainframe::activewnd); menu) // GetMenuFromHandle
		{
			// CMenu::DestroyMenu
			utils::hook::call<void(__fastcall)(CMenu*)>(0x58A908)(menu);
		}

		SetMenu(cmainframe::activewnd->GetWindow(), nullptr);
		
		ggui::mainframe_menubar_enabled = false;
		dvars::set_bool(dvars::mainframe_show_menubar, false);
	});
}
