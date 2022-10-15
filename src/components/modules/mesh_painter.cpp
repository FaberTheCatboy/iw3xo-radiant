#include "std_include.hpp"
#include <random>

namespace components
{
	mesh_painter* mesh_painter::p_this = nullptr;

	void mesh_painter::set_circle_color(const float r, const float g, const float b, const float a)
	{
		if (!m_circle_verts.empty())
		{
			game::vec4_t	color = { r, g, b, a };
			game::GfxColor	color_packed = {};
			game::Byte4PackPixelColor(color, &color_packed);

			for (auto& p : m_circle_verts)
			{
				p.vert[0].color.packed = color_packed.packed;
				p.vert[1].color.packed = color_packed.packed;
			}
		}
	}

	void mesh_painter::circle_get_coord_for_angle(float* coord, const float angle)
	{
		// sin / cos uses radians ...
		const float radians = utils::vector::deg_to_rad(angle + 90);

		coord[0] = (sinf(radians) * m_circle_radius) * m_circle_vup[0] + m_circle_origin[0];
		coord[1] = (sinf(radians) * m_circle_radius) * m_circle_vup[1] + m_circle_origin[1];
		coord[2] = (sinf(radians) * m_circle_radius) * m_circle_vup[2] + m_circle_origin[2];

		coord[0] = (cosf(radians) * m_circle_radius) * m_circle_vright[0] + coord[0];
		coord[1] = (cosf(radians) * m_circle_radius) * m_circle_vright[1] + coord[1];
		coord[2] = (cosf(radians) * m_circle_radius) * m_circle_vright[2] + coord[2];
	}

	void mesh_painter::random_point_on_circle(float& x, float& y, float& z)
	{
		// generate a random number within a certain range
		std::random_device	rd;
		std::mt19937		gen(rd());

		std::uniform_real_distribution<float> distr(0.0f, 1.0f);

		const float r = m_circle_radius * sqrt(distr(gen));
		const float theta = distr(gen) * 2.0f * M_PI;

		x = sinf(theta) * r * m_circle_vup[0] + m_circle_origin[0];
		y = sinf(theta) * r * m_circle_vup[1] + m_circle_origin[1];
		z = sinf(theta) * r * m_circle_vup[2] + m_circle_origin[2];

		x = cosf(theta) * r * m_circle_vright[0] + x;
		y = cosf(theta) * r * m_circle_vright[1] + y;
		z = cosf(theta) * r * m_circle_vright[2] + z;

	}

	void mesh_painter::draw_circle()
	{
		const auto camera_gui = GET_GUI(ggui::camera_dialog);
		const auto camera = &cmainframe::activewnd->m_pCamWnd->camera;

		// get direction into the scene
		float dir_into_scene[3];
		ccamwnd::calculate_ray_direction(camera_gui->rtt_get_cursor_pos_cpoint().x, static_cast<int>(camera_gui->rtt_get_size().y) - camera_gui->rtt_get_cursor_pos_cpoint().y, dir_into_scene);

		// trace along direction
		game::trace_t cam_trace = {};

		// 0x200 seems to ignore general entites
		game::Test_Ray(camera->origin, dir_into_scene, 0x200, &cam_trace, 1);

		if (cam_trace.dist <= 5000.0f)
		{
			// calculate hit origin
			utils::vector::scale(dir_into_scene, cam_trace.dist, m_circle_origin);
			utils::vector::add(m_circle_origin, camera->origin, m_circle_origin);

			// slightly offset circle from the hit face
			game::vec3_t height_offset = {};
			utils::vector::scale(cam_trace.face_normal, 4.0f, height_offset);
			utils::vector::add(m_circle_origin, height_offset, m_circle_origin);
			utils::vector::copy(cam_trace.face_normal, m_circle_normal);

			// calculate right and up vectors of the hit face normal to align the circle to the face
			if (utils::vector::length_squared(cam_trace.face_normal) != 0.0f)
			{
				game::PerpendicularVector(cam_trace.face_normal, m_circle_vright);
				utils::vector::cross_product(cam_trace.face_normal, m_circle_vright, m_circle_vup);
			}

			// radius changed
			if (m_circle_radius != m_circle_radius_old)
			{
				m_circle_verts.clear();
				m_circle_verts_angles.clear();
				m_circle_line_count = 0;

				const int num_points = 360 / 4;
				for (auto p = 0; p < num_points; p += 2)
				{
					// cache angles
					// separated from line_s vector so that vertex points are continuous and can be draw with a single render command
					m_circle_verts_angles.emplace_back(angle_s
						{
							360.0f * static_cast<float>((p + 0)) / static_cast<float>(num_points),
							360.0f * static_cast<float>((p + 1)) / static_cast<float>(num_points),
						});

					auto pt = mesh_painter::line_s{};
					mesh_painter::circle_get_coord_for_angle(pt.vert[0].xyz, m_circle_verts_angles[m_circle_line_count].angles[0]);
					mesh_painter::circle_get_coord_for_angle(pt.vert[1].xyz, m_circle_verts_angles[m_circle_line_count].angles[1]);

					m_circle_verts.emplace_back(pt);
					m_circle_line_count++;
				}

				set_circle_color(1.0f, 1.0f, 0.0f, 1.0f);
				m_circle_radius_old = m_circle_radius;
			}
			else // unchanged radius
			{
				for (auto line = 0; line < m_circle_line_count; line++)
				{
					mesh_painter::circle_get_coord_for_angle(m_circle_verts[line].vert[0].xyz, m_circle_verts_angles[line].angles[0]);
					mesh_painter::circle_get_coord_for_angle(m_circle_verts[line].vert[1].xyz, m_circle_verts_angles[line].angles[1]);
				}
			}

			if (m_circle_line_count)
			{
				renderer::R_AddLineCmd(static_cast<std::uint16_t>(m_circle_line_count), 4, 3, m_circle_verts[0].vert);

				// visualize object tracing
				/*renderer::R_AddPointCmd(1, 8, 3, &random_point);
				renderer::R_AddLineCmd(1, 2, 3, random_point_trace_to_face.vert);
				renderer::R_AddLineCmd(1, 2, 3, random_point_trace_to_upper.vert);*/
			}
		}
	}

	bool allow_painting()
	{
		const auto painter = mesh_painter::get();
		const auto camera_gui = GET_GUI(ggui::camera_dialog);
		const auto& io = ImGui::GetIO();

		if (!painter->is_active())
		{
			return false;
		}

		if (!camera_gui->rtt_is_hovered())
		{
			return false;
		}

		if (io.KeyMods == ImGuiKeyModFlags_Shift)
		{
			return false;
		}

		if (imgui::IsMouseDown(ImGuiMouseButton_Right))
		{
			return false;
		}

		// do not paint when something is being dragged/dropped
		if (const auto data = imgui::GetDragDropPayload(); data)
		{
			return false;
		}

		return true;
	}

	/**
	 * @brief		choose a random object based on weights
	 * @param rand	a random value between 0 and 1
	 * @return		a pointer to the object
	 */
	const mesh_painter::list_object* mesh_painter::get_random_weighted_object(const float rand)
	{
		if (!m_objects.empty())
		{
			float total_weight = 0.0f;

			for (const auto& obj : m_objects)
			{
				if (obj.paint_weight == 0.0f)
				{
					continue;
				}

				total_weight += obj.paint_weight;
			}

			float start_value = 0;

			//foreach(GameObject t in options)
			for (const auto& obj : m_objects)
			{
				if (obj.paint_weight == 0.0f)
				{
					continue;
				}

				const float range = obj.paint_weight / total_weight;
				if (rand > start_value && rand <= start_value + range)
				{
					return &obj;
				}
				start_value += range;
			}
		}

		return nullptr;
	}

	/**
	 * @brief	mesh placement logic \n
	 *			called from called from => camera_dialog::camera_gui()
	 */
	void mesh_painter::paint_frame()
	{
		if (!allow_painting())
		{
			return;
		}

		bool started_dragging = false;

		// drag start
		if (!m_drag_down && imgui::IsMouseDown(ImGuiMouseButton_Left))
		{
			m_drag_down = true;
			started_dragging = true;

			game::Select_Deselect(true);
			game::Undo_ClearRedo();
			game::Undo_GeneralStart("mesh painter start");
			//game::Undo_AddBrushList_Selected();
		}

		const auto drag_delta = imgui::GetMouseDragDelta(ImGuiMouseButton_Left);

		// on drag start or future dragging
		// drag delta needs to be greater then 'm_drag_threshold'
		if (started_dragging || fabs(drag_delta.x) >= m_drag_threshold || fabs(drag_delta.y) >= m_drag_threshold /*imgui::IsMouseDragPastThreshold(ImGuiMouseButton_Left, m_drag_threshold)*/)
		{
			// reset drag delta so that the next move needs to reach the threshold again
			imgui::ResetMouseDragDelta(ImGuiMouseButton_Left);

			for (auto p = 0; p < m_paint_loop_count; p++)
			{
				game::trace_t cam_trace = {};

				float x, y, z;
				mesh_painter::random_point_on_circle(x, y, z);

				random_point.xyz[0] = x;
				random_point.xyz[1] = y;
				random_point.xyz[2] = z;
				random_point.color.packed = static_cast<unsigned>(PxDebugColor::eARGB_RED);

				// TODO - only do trace 2 if trace 1 was valid - doh

				bool trace_one_valid = false;
				bool trace_two_valid = false;


				// TODO



				const float face_trace_dist = 60.0f;
				const float upward_trace_dist = 30.0f;

				// #
				// check if point is valid (towards the initial face)

				{
					game::vec3_t inv_face_normal = {};
					utils::vector::scale(m_circle_normal, -1.0f, inv_face_normal);

					game::Test_Ray(random_point.xyz, inv_face_normal, 0x200, &cam_trace, 1);

					// from random point
					utils::vector::copy(random_point.xyz, random_point_trace_to_face.vert[0].xyz);
					random_point_trace_to_face.vert[0].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_RED);

					// point towards face
					const float max_dist = cam_trace.dist > face_trace_dist ? face_trace_dist : cam_trace.dist;
					utils::vector::scale(inv_face_normal, max_dist, random_point_trace_to_face.vert[1].xyz);
					utils::vector::add(random_point.xyz, random_point_trace_to_face.vert[1].xyz, random_point_trace_to_face.vert[1].xyz);
					random_point_trace_to_face.vert[1].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_RED);

					// valid if distance is smaller then X
					if (cam_trace.dist < face_trace_dist)
					{
						random_point.color.packed = static_cast<unsigned>(PxDebugColor::eARGB_GREEN);
						random_point_trace_to_face.vert[0].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_GREEN);
						random_point_trace_to_face.vert[1].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_GREEN);

						trace_one_valid = true;
					}
				}

				// #
				// check if we can trace to +Z above random point without hitting anything

				game::vec3_t model_spawn_point = {};

				{
					game::Test_Ray(random_point.xyz, game::vec3_t(0.0f, 0.0f, 1.0f), 0x200, &cam_trace, 1);

					auto& p_upper = random_point_trace_to_upper;

					// from random point
					utils::vector::copy(random_point.xyz, p_upper.vert[0].xyz);
					p_upper.vert[0].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_RED);

					// upwards trace point
					const float max_dist = cam_trace.dist > upward_trace_dist ? upward_trace_dist : cam_trace.dist;
					utils::vector::scale(game::vec3_t(0.0f, 0.0f, 1.0f), max_dist, p_upper.vert[1].xyz);
					utils::vector::add(random_point.xyz, p_upper.vert[1].xyz, p_upper.vert[1].xyz);
					p_upper.vert[1].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_RED);

					// valid if distance was greater then X (nothing in the way)
					if (cam_trace.dist > upward_trace_dist)
					{
						p_upper.vert[0].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_GREEN);
						p_upper.vert[1].color.packed = static_cast<unsigned>(PxDebugColor::eARGB_GREEN);

						utils::vector::add(random_point.xyz, game::vec3_t(0.0f, 0.0f, upward_trace_dist), p_upper.vert[1].xyz);

						trace_two_valid = true;

						utils::vector::copy(p_upper.vert[1].xyz, model_spawn_point);
					}
				}

				if (trace_one_valid && trace_two_valid)
				{
					// #
					// chose random model and settings

					std::random_device	rd;
					std::mt19937		gen(rd());
					std::uniform_real_distribution<float> rd_object(0.0f, 1.0f);

					if (const auto random_model = mesh_painter::get_random_weighted_object(rd_object(gen));
						random_model)
					{
						const auto entity_gui = GET_GUI(ggui::entity_dialog);

						// reset manual left mouse capture
						ggui::dragdrop_reset_leftmouse_capture();
						ggui::entity_dialog::addprop_helper_s no_undo = {};

						game::Select_Deselect(true);


						game::Test_Ray(model_spawn_point, game::vec3_t(0.0f, 0.0f, -1.0f), 0x200, &cam_trace, 1);

						if ((cam_trace.brush || cam_trace.face)
							&& cam_trace.dist <= face_trace_dist + upward_trace_dist * 2.0f
							&& utils::vector::length_squared(cam_trace.face_normal) > 0.0f)
						{
							game::vec3_t spawn_org = {};

							// calculate hit origin
							utils::vector::scale(game::vec3_t(0.0f, 0.0f, -1.0f), cam_trace.dist, spawn_org);
							utils::vector::add(spawn_org, model_spawn_point, spawn_org);


							if ((DWORD*)game::g_selected_brushes_next() == game::currSelectedBrushes)
							{
								game::CreateEntityBrush(0, 0, cmainframe::activewnd->m_pXYWnd);
							}

							// do not open the original modeldialog for this use-case, see: create_entity_from_name_intercept()
							// + do not add brush to undo lists
							g_block_radiant_modeldialog = true;
							game::CreateEntityFromName_DisableUndo = true;
							game::CreateEntityFromName("misc_model"); // takes ~ 15ms
							game::CreateEntityFromName_DisableUndo = false;
							g_block_radiant_modeldialog = false;

							entity_gui->add_prop("model", random_model->name.c_str(), &no_undo);

							if (random_model->random_size)
							{
								std::uniform_real_distribution<float> rd_scale(random_model->size_range[0], random_model->size_range[1]);
								entity_gui->add_prop("modelscale", utils::va("%.2f", rd_scale(gen)), &no_undo);
							}

							// z_offset

							if (random_model->z_offset != 0.0f)
							{
								game::vec3_t offset_along_dir = {};
								utils::vector::scale(cam_trace.face_normal, random_model->z_offset, offset_along_dir);

								utils::vector::add(spawn_org, offset_along_dir, spawn_org);
							}

							char prop_str_buf[64] = {};
							if (sprintf_s(prop_str_buf, "%.3f %.3f %.3f", spawn_org[0], spawn_org[1], spawn_org[2]))
							{
								entity_gui->add_prop("origin", prop_str_buf, &no_undo);
							}
							

							if (random_model->random_rotation)
							{
								std::uniform_real_distribution<float> rd_angle(0.0f, 360.0f);
								entity_gui->add_prop("angles", utils::va("0 %.2f 0", rd_angle(gen)), &no_undo);
							}

							if (random_model->align_to_ground)
							{
								game::AlignEntityToFace(game::g_edit_entity(), cam_trace.face_normal);

								// TODO! - very wrong (because 355 == -5 but 355 is greater 90 so X will be clamped to 90)
								// => work with vectors

								/*game::vec3_t ground_angles = {};
								if (entity_gui->get_vec3_for_key_from_entity(reinterpret_cast<game::entity_s*>(game::g_edit_entity()), ground_angles, "angles"))
								{
									bool was_clamped = false;

									if (fabs(ground_angles[0]) > random_model->max_align_to_ground_angle[0])
									{
										was_clamped = true;

										ground_angles[0] = ground_angles[0] < 0.0f
											? -random_model->max_align_to_ground_angle[0]
											:  random_model->max_align_to_ground_angle[0];
									}

									if (fabs(ground_angles[2]) > random_model->max_align_to_ground_angle[1])
									{
										was_clamped = true;

										ground_angles[2] = ground_angles[2] < 0.0f
											? -random_model->max_align_to_ground_angle[1]
											:  random_model->max_align_to_ground_angle[1];
									}

									if (was_clamped)
									{
										entity_gui->add_prop("angles", utils::va("%.3f %.3f %.3f", ground_angles[0], ground_angles[1], ground_angles[2]), &no_undo);
									}
								}*/
							}
						}

						game::Select_Deselect(true);
					}
				}
			}
		}

		// drag end
		if (m_drag_down && !imgui::IsMouseDown(ImGuiMouseButton_Left))
		{
			//game::Undo_EndBrushList_Selected();
			game::Undo_End();
			m_drag_down = false;
		}
	}


	/**
	 * @brief	runs each grid and camera frame \n
	 *			called from => renderer::draw_additional_debug
	 */
	void mesh_painter::on_frame()
	{
		// TODO! - add dist param.
		// TODO! - line width param.
		// TODO! - line color param.

		const auto painter = mesh_painter::get();
		//const auto camera_gui = GET_GUI(ggui::camera_dialog);
		//const auto& io = ImGui::GetIO();

		if (!allow_painting())
		{
			return;
		}

		// TODO
		/*painter->m_stick_to_first_surface = false;
		if (io.KeyMods == ImGuiKeyModFlags_Ctrl)
		{
			painter->m_stick_to_first_surface = true;
		}*/

		painter->draw_circle();
	}


	/**
	 * @brief	allow/block og radiant left mouse button logic \n
	 *			=> czwnd::on_lbutton_down / czwnd::on_lbutton_up
	 * @return	return true to block input
	 */
	bool mesh_painter::block_lbutton_logic()
	{
		if (!allow_painting())
		{
			return false;
		}

		//game::printf_to_console("mouse up/down\n");
		return true;
	}

	mesh_painter::mesh_painter()
	{
		mesh_painter::p_this = this;
		m_circle_verts.reserve(256);
		m_circle_verts_angles.reserve(256);
	}

	mesh_painter::~mesh_painter()
	{ }
}
