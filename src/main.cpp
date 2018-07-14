#include <iostream>

#include "BsApplication.h"

#include "Audio/BsAudioClip.h"
#include "Audio/BsAudioClipImportOptions.h"
#include "Audio/BsAudio.h"
#include "Components/BsCAudioSource.h"
#include "Components/BsCAudioListener.h"
#include "Importer/BsImporter.h"
#include "Scene/BsSceneObject.h"
#include "Utility/BsTime.h"

#include <eye.hpp>
#include <openvr.h>
#include <score.hpp>

vr::IVRSystem* g_hmd;
vr::IVRCompositor* g_compositor;

bs::HSceneObject g_scene;

std::shared_ptr<eye> g_left_eye;
std::shared_ptr<eye> g_right_eye;

// This program
class app : public bs::Application {
public:
	app(const bs::START_UP_DESC& d): bs::Application(d) {}
	~app() = default;

public:
	void preUpdate() override {
		// get the juice
		vr::TrackedDevicePose_t hmd_pose{};
		vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
		
		g_compositor->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
		for (auto i = 0u; i != vr::k_unMaxTrackedDeviceCount; ++i) {
			if (poses[i].bPoseIsValid)
				if (g_hmd->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_HMD) {
					hmd_pose = poses[i];
				}
		}

		auto vr_transform = hmd_pose.mDeviceToAbsoluteTracking;
		bs::Matrix4 bs_transform(
			vr_transform.m[0][0], vr_transform.m[1][0], vr_transform.m[2][0], 0.f,
			vr_transform.m[0][1], vr_transform.m[1][1], vr_transform.m[2][1], 0.f,
			vr_transform.m[0][2], vr_transform.m[1][2], vr_transform.m[2][2], 0.f,
			vr_transform.m[0][3], vr_transform.m[1][3], vr_transform.m[2][3], 1.f
		);
		bs_transform = bs_transform.transpose();

		bs::Vector3 pos, scale;
		bs::Quaternion rot;
		bs_transform.decomposition(pos, rot, scale);

		auto t = bs::Transform(pos, rot, bs::Vector3::ONE);
		g_scene->setPosition(pos);
		g_scene->setRotation(rot);
	}

	void postUpdate() override {
		g_left_eye->present();
		g_right_eye->present();
	}
};

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cerr << "usage: " << std::endl;
		std::cerr << "\t" << argv[0] << ": /path/to/midi /path/to/soundfont" << std::endl;
		return 1;
	}

	vr::EVRInitError vr_error;
	g_hmd = vr::VR_Init(&vr_error, vr::VRApplication_Scene);
	if (vr_error != vr::VRInitError_None)
		throw std::runtime_error("Could not initialize OpenVR");
		
	g_compositor = vr::VRCompositor();
	if (!g_compositor)
		throw std::runtime_error("Could not initialize compositor");

	uint32_t width, height;
	g_hmd->GetRecommendedRenderTargetSize(&width, &height);
	
	// Set up the instance of BF::S
	bs::VideoMode vm(width, height);
	bs::Application::startUp<app>(vm, "Audio", false);

	// Create the score
	auto s = score(argv[1], argv[2]);

	// Create the audio objects
	auto stream_options = bs::AUDIO_CLIP_DESC{};
	stream_options.readMode = bs::AudioReadMode::Stream;
	stream_options.is3D = true;
	stream_options.numChannels = 1;
	stream_options.keepSourceData = false;

	g_scene = bs::SceneObject::create("MainCamera");
	auto lis = g_scene->addComponent<bs::CAudioListener>();

	// Audio enumeration
	bs::Vector<bs::AudioDevice> devices = bs::gAudio().getAllDevices();
	for (const auto& dev : devices)
		std::cout << "Dev: " << dev.name << std::endl;
	
	bs::gAudio().setActiveDevice(devices[1]);

	std::vector<bs::HSceneObject> mus;
	std::vector<bs::GameObjectHandle<bs::CAudioSource>> src;

	for (auto& [n, c] : s.get_channels()) {
		mus.push_back(bs::SceneObject::create(("Instr[" + std::to_string(n) + "]").c_str()));
	
		src.push_back(mus.back()->addComponent<bs::CAudioSource>());
		src.back()->setIsLooping(true);
		src.back()->setClip(bs::AudioClip::create(c, 0, 882 * 2, stream_options));
	}

	for (const auto& s : src) {
		s->play();
	}

	auto head      = bs::SceneObject::create("Head");
	g_left_eye  = std::make_shared<eye>(g_hmd, head, vr::Eye_Left);
	g_right_eye = std::make_shared<eye>(g_hmd, head, vr::Eye_Right);

	// Clean up
	bs::Application::instance().runMainLoop();
	bs::Application::shutDown();

	return 0;
}
