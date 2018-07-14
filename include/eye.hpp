#pragma once

#include <openvr.h>

#include "RenderAPI/BsRenderTexture.h"
#include "Scene/BsSceneObject.h"

class eye {
public:
	eye(vr::IVRSystem* hmd, bs::HSceneObject head, vr::Hmd_Eye i);

public:
	void present();

private:
	vr::IVRSystem* m_hmd;
	vr::Hmd_Eye m_id;

	bs::HSceneObject m_scene_object;
	bs::SPtr<bs::RenderTexture> m_render_texture;
};