#pragma once

namespace components
{
	class physx_impl : public component
	{
	public:
		physx::PxDefaultAllocator      mDefaultAllocatorCallback;
		physx::PxDefaultErrorCallback  mDefaultErrorCallback;
		physx::PxDefaultCpuDispatcher* mDispatcher = NULL;
		physx::PxTolerancesScale       mToleranceScale;

		physx::PxFoundation* mFoundation = NULL;
		physx::PxPhysics* mPhysics = NULL;

		physx::PxScene* mScene = NULL;
		physx::PxMaterial* mMaterial = NULL;

		physx::PxPvd* mPvd = NULL;

		bool m_simulation_running;
		bool m_effect_is_using_physics;

		int m_time_last_snapshot;
		int m_time_last_update;
		float m_time_now_lerp_frac;
		int m_phys_msec_step;
		uint32_t m_active_body_count;

	public:
		physx_impl();
		~physx_impl();
		const char* get_name() override { return "physx_impl"; };

		static physx_impl* p_this;
		static physx_impl* get() { return p_this; }

		void run_frame(float seconds);
		void frame();
		physx::PxMaterial* create_material(game::PhysPreset* preset);
		void obj_destroy(int id);
		void obj_get_interpolated_state(int id, float* out_pos, float* out_quat);

	};
}
