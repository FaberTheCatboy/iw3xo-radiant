#include "std_include.hpp"

namespace ggui
{
	bool mesh_painter_dialog::gui()
	{
		const auto MIN_WINDOW_SIZE = ImVec2(360.0f, 360.0f);
		const auto INITIAL_WINDOW_SIZE = ImVec2(380.0f, 650.0f);

		imgui::SetNextWindowSizeConstraints(MIN_WINDOW_SIZE, ImVec2(FLT_MAX, FLT_MAX));
		imgui::SetNextWindowSize(INITIAL_WINDOW_SIZE, ImGuiCond_FirstUseEver);
		imgui::SetNextWindowPos(ggui::get_initial_window_pos(), ImGuiCond_FirstUseEver);

		const auto painter = components::mesh_painter::get();

		if (!imgui::Begin("Mesh Painter##window", this->get_p_open(), ImGuiWindowFlags_NoCollapse))
		{
			imgui::End();
			return false;
		}

		// #

		SPACING(0.0f, 2.0f);
		imgui::Indent(4.0f);

		if (imgui::Button("Toggle Painter", ImVec2(imgui::CalcItemWidth(), imgui::GetFrameHeight())))
		{
			painter->toggle();
		}

		if (imgui::DragFloat("Radius", &painter->m_circle_radius, 0.1f, 10.0f, 4096.0f, "%.1f"))
		{
			painter->m_circle_radius = ImClamp(painter->m_circle_radius, 10.0f, 4096.0f);
		} TT("Radius of the painting circle");

		if (imgui::DragFloat("Paint Threshold", &painter->m_drag_threshold, 0.005f, 0.5f, 100.0f, "%.2f"))
		{
			painter->m_drag_threshold = ImClamp(painter->m_drag_threshold, 0.5f, 100.0f);
		} TT("Mouse-drag distance needed to do the next paint");

		if (imgui::DragInt("Paint Density", &painter->m_paint_loop_count, 0.02f, 1, 10))
		{
			painter->m_paint_loop_count = ImClamp(painter->m_paint_loop_count, 1, 10);
		} TT("Amount of objects spawned per paint");

		SPACING(0.0f, 4.0f);

		imgui::PushFontFromIndex(REGULAR_14PX);
		imgui::TextUnformatted("Listbox supports Drag & Drop (from Model Browser)");
		imgui::PopFont();

		const float listbox_height = 140.0f;
		if (imgui::BeginListBox("##object_listbox", ImVec2(0, listbox_height)))
		{
			if (m_object_list_selected_index >= painter->m_objects.size())
			{
				m_object_list_selected_index = 0;
			}

			for (std::uint32_t n = 0; n < painter->m_objects.size(); n++)
			{
				const bool is_selected = (m_object_list_selected_index == n);
				if (imgui::Selectable(painter->m_objects[n].name.c_str(), is_selected))
				{
					m_object_list_selected_index = n;

					// select model in modelselector if its open
					const auto m_selector = GET_GUI(ggui::modelselector_dialog);

					if (m_selector->is_active() && !m_selector->is_inactive_tab())
					{
						for (auto i = 0; i < m_selector->m_xmodel_filecount; i++)
						{
							if (m_selector->m_xmodel_filelist[i] == painter->m_objects[n].name)
							{
								m_selector->m_xmodel_selection = i;
								m_selector->m_preview_model_name = m_selector->m_xmodel_filelist[i];
								break;
							}
						}
						
					}
				}

				// initial focus
				if (is_selected)
				{
					imgui::SetItemDefaultFocus();
				}
			}

			imgui::EndListBox();
		}

		// #
		// model selection drop target

		if (imgui::BeginDragDropTarget())
		{
			if (imgui::AcceptDragDropPayload("MODEL_SELECTOR_ITEM"))
			{
				const auto m_selector = GET_GUI(ggui::modelselector_dialog);

				if (!painter->list_object_exists(m_selector->m_preview_model_name))
				{
					painter->m_objects.emplace_back(m_selector->m_preview_model_name);
				}
				else
				{
					imgui::Toast(ImGuiToastType_Info, "Mesh Painter", "Object is already part of the list!");
				}
			}

			imgui::EndDragDropTarget();
		}

		imgui::SameLine();
		const auto post_listbox_cursor = imgui::GetCursorPos();

		if (imgui::Button("..##filepromt", ImVec2(28, imgui::GetFrameHeight())))
		{
			std::string path_str;

			const auto egui = GET_GUI(ggui::entity_dialog);
			path_str = egui->get_value_for_key_from_epairs(game::g_qeglobals->d_project_entity->epairs, "basepath");
			path_str += "\\raw\\xmodel";


			const auto file = GET_GUI(ggui::file_dialog);
			file->set_default_path(path_str);
			file->set_file_handler(CUSTOM);
			file->set_file_op_type(ggui::file_dialog::FileDialogType::OpenFile);
			file->set_file_ext("");
			file->set_blocking();
			file->set_callback([]
				{
					const auto dlg = GET_GUI(ggui::file_dialog);

					const std::string replace_path = "raw\\xmodel\\";

					if (dlg->get_path_result().contains(replace_path))
					{
						std::string loc_filepath = dlg->get_path_result().substr(
							dlg->get_path_result().find(replace_path) + replace_path.length());

						utils::replace(loc_filepath, "\\", "/");

						const auto& painter = components::mesh_painter::get();

						if (!painter->list_object_exists(loc_filepath))
						{
							painter->m_objects.emplace_back(loc_filepath);
						}
						else
						{
							imgui::Toast(ImGuiToastType_Info, "Mesh Painter", "Object is already part of the list!");
						}
					}
					else
					{
						game::printf_to_console("[ERR] [Mesh Painter] invalid file?");
					}
				});

			file->open();
		}

		imgui::SetCursorPos(ImVec2(post_listbox_cursor.x, post_listbox_cursor.y + imgui::GetFrameHeight() + 4.0f));

		const bool can_delete_object = !painter->m_objects.empty();
		imgui::BeginDisabled(!can_delete_object);
		{
			if (imgui::Button("x##delete_object", ImVec2(28, imgui::GetFrameHeight())))
			{
				if (m_object_list_selected_index >= painter->m_objects.size())
				{
					m_object_list_selected_index = 0;
				}

				if (!painter->m_objects.empty() && m_object_list_selected_index < painter->m_objects.size())
				{
					painter->m_objects.erase(painter->m_objects.begin() + m_object_list_selected_index);

					if (m_object_list_selected_index)
					{
						m_object_list_selected_index--;
					}
				}
			}

			imgui::EndDisabled();
		}

		imgui::SetCursorPosY(post_listbox_cursor.y + listbox_height + 8.0f);

		SPACING(0.0f, 4.0f);

		// per object settings
		if (!painter->m_objects.empty())
		{
			auto& obj = painter->m_objects[m_object_list_selected_index];

			imgui::PushFontFromIndex(BOLD_18PX);
			imgui::TextUnformatted("Per Object Settings:");
			imgui::PopFont();
			imgui::SameLine(0, 8.0f);
			imgui::TextUnformatted(obj.name.c_str());

			SPACING(0.0f, 4.0f);

			const float per_object_settings_width = 160.0f;

			imgui::SetNextItemWidth(per_object_settings_width);
			imgui::DragFloat("Paint Weight", &obj.paint_weight, 0.005f, 0.0f, 100.0f, "%.2f"); TT("Heighest weight of all objects will be the most often painted object.");

			SPACING(0.0f, 4.0f);

			imgui::Checkbox("Random Size", &obj.random_size);
			imgui::BeginDisabled(!obj.random_size);
			{
				imgui::SetNextItemWidth(per_object_settings_width);
				imgui::DragFloat2("Size Range", obj.size_range, 0.005f, 0.1f, 100.0f, "%.2f"); TT("min <-> max size of object");
				imgui::EndDisabled();
			}

			SPACING(0.0f, 4.0f);

			imgui::Checkbox("Random Rotation", &obj.random_rotation); TT("Only affects Y-Angle");
			imgui::Checkbox("Align to Ground", &obj.align_to_ground);

			imgui::DragFloat("Ground Offset", &obj.z_offset, 0.005f, -100.0f, 100.0f, "%.2f");
			TT("Additional ground offset (eg. if model origin is not at the bottom)\n(Negative value will make object sink into the ground)");

			// TODO!
			/*imgui::BeginDisabled(!obj.align_to_ground);
			{
				imgui::SetNextItemWidth(per_object_settings_width);
				imgui::DragFloat("Max Ground X-Angle", &obj.max_align_to_ground_angle[0], 0.005f, 0.0f, 90.0f, "%.2f");
				TT("Max X Angle of aligned object. Objects placed with larger angles will be clamped to this value.\n(Absolute Value)");

				imgui::SetNextItemWidth(per_object_settings_width);
				imgui::DragFloat("Max Ground Z-Angle", &obj.max_align_to_ground_angle[1], 0.005f, 0.0f, 90.0f, "%.2f");
				TT("Max Z Angle of aligned object. Objects placed with larger angles will be clamped to this value.\n(Absolute Value)");
				imgui::EndDisabled();
			}*/
		}

		imgui::End();
		
		return true;
	}

	void mesh_painter_dialog::on_init()
	{ }

	void mesh_painter_dialog::on_open()
	{ }

	void mesh_painter_dialog::on_close()
	{
		components::mesh_painter::get()->toggle(true, false);
	}

	REGISTER_GUI(mesh_painter_dialog);
}
