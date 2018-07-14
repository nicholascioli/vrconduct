#include <eye.hpp>

#include "Components/BsCCamera.h"
#include "CoreThread/BsCoreThread.h"

#include <GL/gl.h>

eye::eye(vr::IVRSystem* hmd, bs::HSceneObject head, vr::Hmd_Eye e)
: m_hmd{hmd}, m_id{e} {
	m_scene_object = bs::SceneObject::create((e == vr::Eye_Left) ? "Eye_Left" : "Eye_Right");
	m_scene_object->setParent(head);

	auto make_bs_matrix = [](auto vr_transform) {
		bs::Matrix4 bs_transform(
			vr_transform.m[0][0], vr_transform.m[1][0], vr_transform.m[2][0], 0.f,
			vr_transform.m[0][1], vr_transform.m[1][1], vr_transform.m[2][1], 0.f,
			vr_transform.m[0][2], vr_transform.m[1][2], vr_transform.m[2][2], 0.f,
			vr_transform.m[0][3], vr_transform.m[1][3], vr_transform.m[2][3], 1.f
		);
		
		return bs_transform.transpose();
	};

	auto vr_view = m_hmd->GetEyeToHeadTransform(e);
	auto bs_view = make_bs_matrix(vr_view);

	bs::Vector3 pos, scale; bs::Quaternion rot;
	bs_view.decomposition(pos, rot, scale);

	m_scene_object->setPosition(pos);
	m_scene_object->setRotation(rot);

	uint32_t width, height;
	m_hmd->GetRecommendedRenderTargetSize(&width, &height);

	bs::TEXTURE_DESC color_desc;
	color_desc.type   = bs::TEX_TYPE_2D;
	color_desc.width  = width;
	color_desc.height = height;
	color_desc.format = bs::PF_RGB8;
	color_desc.usage  = bs::TU_RENDERTARGET;

	// might not be needed if deferred rendering
	bs::TEXTURE_DESC depth_desc;
	depth_desc.type   = bs::TEX_TYPE_2D;
	depth_desc.width  = width;
	depth_desc.height = height;
	depth_desc.format = bs::PF_RGBA8;
	depth_desc.usage  = bs::TU_DEPTHSTENCIL;

	auto color = bs::Texture::create(color_desc);
	auto depth = bs::Texture::create(depth_desc);

	bs::RENDER_TEXTURE_DESC desc;
	desc.colorSurfaces[0].texture    = color;
	desc.colorSurfaces[0].face       = 0;
	desc.colorSurfaces[0].mipLevel   = 0;
	desc.depthStencilSurface.texture  = depth;
	desc.depthStencilSurface.face     = 0;
	desc.depthStencilSurface.mipLevel = 0;

	m_render_texture = bs::RenderTexture::create(desc);
	bs::HCamera cam = m_scene_object->addComponent<bs::CCamera>();
	cam->getViewport()->setTarget(m_render_texture);

	auto vr_proj = m_hmd->GetProjectionMatrix(e, 0.1f, 100.0f);
	cam->setCustomProjectionMatrix(true, make_bs_matrix(vr_proj));
}

// Forward declare hidden inner class stuff
namespace bs::ct {
	class GLTexture {
	public:
		GLuint getGLID() const;
	};
}

void eye::present() {
	vr::Texture_t submit_texture;
	submit_texture.eColorSpace = vr::ColorSpace_Auto;

	auto* raw = m_render_texture->getColorTexture(0)->getCore().get();
#ifdef WIN32
	if (auto* d3d11_raw = (bs::ct::D3D11Texture)raw) {
		submit_texture.handle = (void*)d3d11_raw->getDX11Resource();
		submit_texture.eType  = vr::TextureType_DirectX;
	} else
#endif
	if (auto* opengl_raw = (bs::ct::GLTexture*)raw) {
		submit_texture.handle = (void*)opengl_raw->getGLID();
		submit_texture.eType  = vr::TextureType_OpenGL;
	// } else if (auto* vulkan_raw = dynamic_cast<bs::ct::VulkanTexture*>(raw)) {
	// 	submit_texture.handle = (void*)vulkan_raw->getResource(0);
	// 	submit_texture.eType  = vr::TextureType_Vulkan;
	} else {
		assert(false);
	}

	vr::VRCompositor()->Submit(m_id, &submit_texture);
}
