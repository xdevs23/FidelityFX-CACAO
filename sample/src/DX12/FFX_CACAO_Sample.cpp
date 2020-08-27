// AMD SampleDX12 sample code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"

#include "FFX_CACAO_Sample.h"
#include "FFX_CACAO_Common.h"

static inline void SetWindowClientSize(HWND hWnd, LONG width, LONG height)
{
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;
	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	AdjustWindowRect(&rect, style, FALSE);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	SetWindowPos(hWnd, NULL, -1, -1, width, height, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}


const bool VALIDATION_ENABLED = false;

FfxCacaoSample::FfxCacaoSample(LPCSTR name) : FrameworkWindows(name)
{
    m_lastFrameTime = MillisecondsNow();
    m_time = 0;
    m_bPlay = true;

    m_pGltfLoader = NULL;
}

//--------------------------------------------------------------------------------------
//
// OnParseCommandLine
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight, bool *pbFullScreen)
{
	// set some default values
	*pWidth = 1920;
	*pHeight = 1080;
	m_activeScene = 0; //load the first one by default
	*pbFullScreen = false;
	m_activeCamera = 0;

#ifdef FFX_CACAO_ENABLE_PROFILING
	m_isBenchmarking = false;
#endif

	//read globals
	auto process = [&](json jData)
	{
		*pWidth = jData.value("width", *pWidth);
		*pHeight = jData.value("height", *pHeight);
		*pbFullScreen = jData.value("fullScreen", *pbFullScreen);
		m_presetIndex = jData.value("preset", m_presetIndex);
		m_activeScene = jData.value("activeScene", m_activeScene);
		m_activeCamera = jData.value("activeCamera", m_activeCamera);
		m_isCpuValidationLayerEnabled = jData.value("CpuValidationLayerEnabled", m_isCpuValidationLayerEnabled);
		m_isGpuValidationLayerEnabled = jData.value("GpuValidationLayerEnabled", m_isGpuValidationLayerEnabled);
#ifdef FFX_CACAO_ENABLE_PROFILING
		m_isBenchmarking = jData.value("benchmark", m_isBenchmarking);
#endif
	};

	// read config file (and override values from commandline if so)
	//
	{
		std::ifstream f("FFX_CACAO_Sample.json");
		if (!f)
		{
			MessageBox(NULL, "Config file not found!\n", "Cauldron Panic!", MB_ICONERROR);
			exit(0);
		}

		try
		{
			f >> m_jsonConfigFile;
		}
		catch (json::parse_error)
		{
			MessageBox(NULL, "Error parsing FFX_CACAO_Sample.json!\n", "Cauldron Panic!", MB_ICONERROR);
			exit(0);
		}
	}


	json globals = m_jsonConfigFile["globals"];
	process(globals);

	// get the list of scenes
	for (const auto & scene : m_jsonConfigFile["scenes"])
		m_sceneNames.push_back(scene["name"]);

	//read json globals from commandline
	//
	try
	{
		if (strlen(lpCmdLine) > 0)
		{
			auto j3 = json::parse(lpCmdLine);
			process(j3);
		}
	}
	catch (json::parse_error)
	{
		Trace("Error parsing commandline\n");
		exit(0);
	}

#ifdef FFX_CACAO_ENABLE_PROFILING
	if (m_isBenchmarking)
	{
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		bool downsampled = FFX_CACAO_PRESETS[m_presetIndex].useDownsampledSsao;
#endif
		uint32_t quality = FFX_CACAO_PRESETS[m_presetIndex].settings.qualityLevel;
		m_benchmarkScreenWidth = *pWidth;
		m_benchmarkScreenHeight = *pHeight;
		m_benchmarkWarmUpFramesToRun = 100;
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		snprintf(m_benchmarkFilename, _countof(m_benchmarkFilename), "FFX_CACAO_DX12_Benchmark_%s_%ux%u_Q%u.csv", downsampled ? "downsampled" : "native", *pWidth, *pHeight, quality);
#else
		snprintf(m_benchmarkFilename, _countof(m_benchmarkFilename), "FFX_CACAO_DX12_Benchmark_downsampled_%ux%u_Q%u.csv", *pWidth, *pHeight, quality);
#endif
		m_vsyncEnabled = false;
		m_isGpuValidationLayerEnabled = false;
		m_isCpuValidationLayerEnabled = false;
	}
#endif
}

//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnCreate(HWND hWnd)
{
	m_hWnd = hWnd;

	m_displayGUI = true;
	m_cameraControlSelected = 1;

	m_state.cacaoSettings = FFX_CACAO_PRESETS[m_presetIndex].settings;

	DWORD dwAttrib = GetFileAttributes("..\\media\\");
    if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) == 0)
    {
        MessageBox(NULL, "Media files not found!\n\nPlease check the readme on how to get the media files.", "Cauldron Panic!", MB_ICONERROR);
        exit(0);
    }

	m_fullscreen = false;

    // Create Device
    //
    m_device.OnCreate("myapp", "myEngine", m_isCpuValidationLayerEnabled, m_isGpuValidationLayerEnabled, hWnd);
    m_device.CreatePipelineCache();

    //init the shader compiler
	InitDirectXCompiler();
    CreateShaderCache();

    // Create Swapchain
    //

    // Init FS2 and choose format
    fsHdrInit(m_device.GetAGSContext(), m_device.GetAGSGPUInfo(), hWnd);

    uint32_t dwNumberOfBackBuffers = 2;
    m_swapChain.OnCreate(&m_device, dwNumberOfBackBuffers, hWnd);

    // Create a instance of the renderer and initialize it, we need to do that for each GPU
    //
    m_Node = new SampleRenderer();
    m_Node->OnCreate(&m_device, &m_swapChain);

    // init GUI (non gfx stuff)
    //
    ImGUI_Init((void *)hWnd);

    // Init Camera, looking at the origin
    //
    m_roll = 0.0f;
    m_pitch = 0.0f;
    m_distance = 3.5f;

    // init GUI state
    m_state.toneMapper = 0;
    m_state.skyDomeType = 0;
    m_state.exposure = 1.0f;
    m_state.iblFactor = 10.0f;
    m_state.emmisiveFactor = 1.0f;
    m_state.bDrawLightFrustum = false;
    m_state.bDrawBoundingBoxes = false;
    m_state.camera.LookAt(m_roll, m_pitch, m_distance, XMVectorSet(0, 0, 0, 0));

    m_state.spotlightCount = 1;

    m_state.spotlight[0].intensity = 5.0f;
    m_state.spotlight[0].color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    m_state.spotlight[0].light.SetFov(XM_PI / 2.0f, 1024, 1024, 0.1f, 100.0f);
    m_state.spotlight[0].light.LookAt(XM_PI / 2.0f, 0.58f, 3.5f, XMVectorSet(0, 0, 0, 0));

	m_state.bUseCACAO = true;
	m_state.bDisplayCacaoDirectly = true;
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnDestroy()
{
#ifdef FFX_CACAO_ENABLE_PROFILING
	m_isBenchmarking = false;
#endif

    ImGUI_Shutdown();

    m_device.GPUFlush();

    // Fullscreen state should always be false before exiting the app.
    m_swapChain.SetFullScreen(false);

    m_Node->UnloadScene();
    m_Node->OnDestroyWindowSizeDependentResources();
    m_Node->OnDestroy();

    delete m_Node;

    m_swapChain.OnDestroyWindowSizeDependentResources();
    m_swapChain.OnDestroy();

    //shut down the shader compiler 
    DestroyShaderCache(&m_device);

    if (m_pGltfLoader)
    {
        delete m_pGltfLoader;
        m_pGltfLoader = NULL;
    }

    m_device.OnDestroy();
}

//--------------------------------------------------------------------------------------
//
// OnEvent
//
//--------------------------------------------------------------------------------------
bool FfxCacaoSample::OnEvent(MSG msg)
{
    if (ImGUI_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam))
        return true;

	return true;
}

//--------------------------------------------------------------------------------------
//
// SetFullScreen
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::SetFullScreen(bool fullscreen)
{
    m_device.GPUFlush();

    m_swapChain.SetFullScreen(fullscreen);
}

//--------------------------------------------------------------------------------------
//
// OnResize
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnResize(uint32_t width, uint32_t height, bool force)
{
#ifdef FFX_CACAO_ENABLE_PROFILING
	if (m_isBenchmarking && !m_benchmarkWarmUpFramesToRun)
	{
		if (width != m_benchmarkScreenWidth || height != m_benchmarkScreenHeight)
		{
			MessageBox(NULL, "Attempt to change screen resolution when benchmarking!\n", "Benchmark Failed!", MB_ICONERROR);
			exit(0);
		}
	}
#endif

    if (m_Width != width || m_Height != height || force)
    {
        // Flush GPU
        //
        m_device.GPUFlush();

        // If resizing but no minimizing
        //
        if (m_Width > 0 && m_Height > 0)
        {
            if (m_Node!=NULL)
            {
                m_Node->OnDestroyWindowSizeDependentResources();
            }
            m_swapChain.OnDestroyWindowSizeDependentResources();
        }

        m_Width = width;
        m_Height = height;

        // if resizing but not minimizing the recreate it with the new size
        //
        if (m_Width > 0 && m_Height > 0)
        {
            m_swapChain.OnCreateWindowSizeDependentResources(m_Width, m_Height, m_vsyncEnabled, DISPLAYMODE_SDR);
            if (m_Node != NULL)
            {
                m_Node->OnCreateWindowSizeDependentResources(&m_swapChain, m_Width, m_Height);
            }
        }
    }
    m_state.camera.SetFov(XM_PI / 4, m_Width, m_Height, 0.1f, 1000.0f);
}

//--------------------------------------------------------------------------------------
//
// BuildUI, also loads the scene!
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::BuildUI()
{
	if (m_requiresLoad)
	{
		m_requiresLoad = false;
		json scene = m_jsonConfigFile["scenes"][m_activeScene];

		// release everything and load the GLTF, just the light json data, the rest (textures and geometry) will be done in the main loop
		if (m_pGltfLoader != NULL)
		{
			m_Node->UnloadScene();
			m_Node->OnDestroyWindowSizeDependentResources();
			m_Node->OnDestroy();
			m_pGltfLoader->Unload();
			m_Node->OnCreate(&m_device, &m_swapChain);
			m_Node->OnCreateWindowSizeDependentResources(&m_swapChain, m_Width, m_Height);
		}

		delete(m_pGltfLoader);
		m_pGltfLoader = new GLTFCommon();
		if (m_pGltfLoader->Load(scene["directory"], scene["filename"]) == false)
		{
			MessageBox(NULL, "The selected model couldn't be found, please check the documentation", "Cauldron Panic!", MB_ICONERROR);
			exit(0);
		}

		// Load the UI settings, and also some defaults cameras and lights, in case the GLTF has none
		{
#define LOAD(j, key, val) val = j.value(key, val)

			// global settings
			LOAD(scene, "toneMapper", m_state.toneMapper);
			LOAD(scene, "skyDomeType", m_state.skyDomeType);
			LOAD(scene, "exposure", m_state.exposure);
			LOAD(scene, "iblFactor", m_state.iblFactor);
			LOAD(scene, "emmisiveFactor", m_state.emmisiveFactor);
			LOAD(scene, "skyDomeType", m_state.skyDomeType);

			// Add a default light in case there are none
			if (m_pGltfLoader->m_lights.size() == 0)
			{
				tfNode n;
				n.m_tranform.LookAt(PolarToVector(XM_PI / 2.0f, 0.58f)*3.5f, XMVectorSet(0, 0, 0, 0));

				tfLight l;
				l.m_type = tfLight::LIGHT_SPOTLIGHT;
				l.m_intensity = scene.value("intensity", 1.0f);
				l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
				l.m_range = 15;
				l.m_outerConeAngle = XM_PI / 4.0f;
				l.m_innerConeAngle = (XM_PI / 4.0f) * 0.9f;

				m_pGltfLoader->AddLight(n, l);
			}

			// default camera (in case the gltf has none)
			json camera = scene["camera"];
			LOAD(camera, "yaw", m_roll);
			LOAD(camera, "pitch", m_pitch);
			LOAD(camera, "distance", m_distance);
			XMVECTOR lookAt = GetVector(GetElementJsonArray(camera, "lookAt", { 0.0, 0.0, 0.0 }));
			m_state.camera.LookAt(m_roll, m_pitch, m_distance, lookAt);
			m_activeCamera = scene.value("activeCamera", m_activeCamera);

#ifdef FFX_CACAO_ENABLE_PROFILING
			if (m_isBenchmarking)
			{
				std::string deviceName;
				std::string driverVersion;
				json benchmarkJson = scene["BenchmarkSettings"];
				benchmarkJson["resultsFilename"] = m_benchmarkFilename;
				m_device.GetDeviceInfo(&deviceName, &driverVersion);
				BenchmarkConfig(benchmarkJson, m_activeCamera, m_pGltfLoader, deviceName, driverVersion);
			}
#endif

			// indicate the mainloop we started loading a GLTF and it needs to load the rest (textures and geometry)
			m_loadingStage = 1;

			//bail out as we need to reload everything
			ImGui::EndFrame();
			ImGui::NewFrame();
			return;
		}
	}

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	bool opened = true;
	ImGui::Begin("CACAO Sample", &opened);

	if (ImGui::CollapsingHeader("Sample Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Resolution       : %ix%i", m_Width, m_Height);


		const char *cameraControl[] = { "WASD", "Orbit" };
		ImGui::Combo("Camera", &m_cameraControlSelected, cameraControl, _countof(cameraControl));

		if (ImGui::Checkbox("VSync", &m_vsyncEnabled))
		{
			OnResize(m_Width, m_Height, true);
		}
	}

	if (ImGui::CollapsingHeader("FFX CACAO Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Combo("Preset", &m_presetIndex, FFX_CACAO_PRESET_NAMES, _countof(FFX_CACAO_PRESET_NAMES)) && m_presetIndex < _countof(FFX_CACAO_PRESETS))
		{
			m_state.cacaoSettings = FFX_CACAO_PRESETS[m_presetIndex].settings;
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
			m_state.bUseDownsampledSSAO = FFX_CACAO_PRESETS[m_presetIndex].useDownsampledSsao;
#endif
		}

		FfxCacaoSettings *settings = &m_state.cacaoSettings;
		ImGui::SliderFloat("Radius", &settings->radius, 0.0f, 10.0f);
		ImGui::SliderFloat("Shadow Multiplier", &settings->shadowMultiplier, 0.0f, 5.0f);
		ImGui::SliderFloat("Shadow Power", &settings->shadowPower, 0.5f, 5.0f);
		ImGui::SliderFloat("Shadow Clamp", &settings->shadowClamp, 0.0f, 1.0f);
		ImGui::SliderFloat("Horizon Angle Threshold", &settings->horizonAngleThreshold, 0.0f, 0.2f);
		ImGui::SliderFloat("Fade Out From", &settings->fadeOutFrom, 1.0f, 20.0f);
		ImGui::SliderFloat("Fade Out To", &settings->fadeOutTo, 1.0f, 40.0f);
		const char *qualityLevels[] = { "Lowest", "Low", "Medium", "High", "Highest" };
		int idx = (int)settings->qualityLevel;
		ImGui::Combo("Quality Level", &idx, qualityLevels, _countof(qualityLevels));
		settings->qualityLevel = (FfxCacaoQuality)idx;
		if (settings->qualityLevel == FFX_CACAO_QUALITY_HIGHEST)
		{
			ImGui::SliderFloat("Adaptive Quality Limit", &settings->adaptiveQualityLimit, 0.5f, 1.0f);
		}
		ImGui::SliderInt("Blur Pass Count", (int*)&settings->blurPassCount, 0, 8);
		ImGui::SliderFloat("Sharpness", &settings->sharpness, 0.0f, 1.0f);
		ImGui::SliderFloat("Detail Shadow Strength", &settings->detailShadowStrength, 0.0f, 5.0f);

		bool generateNormals = settings->generateNormals ? true : false;
		ImGui::Checkbox("Generate Normal Buffer From Depth Buffer", &generateNormals);
		settings->generateNormals = generateNormals ? FFX_CACAO_TRUE : FFX_CACAO_FALSE;
#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		ImGui::Checkbox("Use Downsampled SSAO", &m_state.bUseDownsampledSSAO);
		if (m_state.bUseDownsampledSSAO)
#endif
		{
			ImGui::SliderFloat("Bilateral Sigma Squared", &settings->bilateralSigmaSquared, 0.0f, 10.0f);
			ImGui::SliderFloat("Bilateral Similarity Distance Sigma", &settings->bilateralSimilarityDistanceSigma, 0.1f, 1.0f);
		}

		ImGui::Checkbox("Display FFX CACAO Output Directly", &m_state.bDisplayCacaoDirectly);
		if (!m_state.bDisplayCacaoDirectly)
		{
			ImGui::Checkbox("Use FFX CACAO", &m_state.bUseCACAO);
		}
		m_state.bUseCACAO |= m_state.bDisplayCacaoDirectly;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		if (m_presetIndex < _countof(FFX_CACAO_PRESETS) && (memcmp(&m_state.cacaoSettings, &FFX_CACAO_PRESETS[m_presetIndex].settings, sizeof(m_state.cacaoSettings)) || m_state.bUseDownsampledSSAO != FFX_CACAO_PRESETS[m_presetIndex].useDownsampledSsao))
#else
		if (m_presetIndex < _countof(FFX_CACAO_PRESETS) && memcmp(&m_state.cacaoSettings, &FFX_CACAO_PRESETS[m_presetIndex].settings, sizeof(m_state.cacaoSettings)))
#endif
		{
			m_presetIndex = _countof(FFX_CACAO_PRESETS);
		}
	}

#ifdef FFX_CACAO_ENABLE_PROFILING
	if (m_vsyncEnabled || !m_state.bUseCACAO)
	{
		ImGui::CollapsingHeader("Profiler Disabled (enable CACAO and turn off vsync)");
	}
	else
	{
		bool displayProfiling = ImGui::CollapsingHeader("Profiler", ImGuiTreeNodeFlags_DefaultOpen);

		FfxCacaoDetailedTiming timings;
		uint64_t gpuTicksPerMicrosecond;
		m_Node->GetCacaoTimings(&m_state, &timings, &gpuTicksPerMicrosecond);
		gpuTicksPerMicrosecond /= 1000000;

		for (uint32_t i = 0; i < timings.numTimestamps; ++i)
		{
			FfxCacaoTimestamp *t = &timings.timestamps[i];
			if (displayProfiling)
			{
				ImGui::Text("%-32s: %7.1f us", t->label, ((double)t->ticks) / ((double)gpuTicksPerMicrosecond));
			}
		}
	}
#endif
	ImGui::End();

	// If the mouse was not used by the GUI then it's for the camera
	//
	ImGuiIO& io = ImGui::GetIO();

	if (io.WantCaptureMouse == false)
	{
		if ((io.KeyCtrl == false) && (io.MouseDown[0] == true))
		{
			m_roll -= io.MouseDelta.x / 100.f;
			m_pitch += io.MouseDelta.y / 100.f;
		}

		// Choose camera movement depending on setting
		//

		if (m_cameraControlSelected == 0)
		{
			//  WASD
			//
			m_state.camera.UpdateCameraWASD(m_roll, m_pitch, io.KeysDown, io.DeltaTime);
		}
		else if (m_cameraControlSelected == 1)
		{
			//  Orbiting
			//
			m_distance -= (float)io.MouseWheel / 3.0f;
			m_distance = std::max<float>(m_distance, 0.1f);

			bool panning = (io.KeyCtrl == true) && (io.MouseDown[0] == true);

			m_state.camera.UpdateCameraPolar(m_roll, m_pitch, panning ? -io.MouseDelta.x / 100.0f : 0.0f, panning ? io.MouseDelta.y / 100.0f : 0.0f, m_distance);
		}
		else if (m_cameraControlSelected > 1)
		{
			// Use a camera from the GLTF
			// 
			m_pGltfLoader->GetCamera(m_cameraControlSelected - 2, &m_state.camera);
			m_roll = m_state.camera.GetYaw();
			m_pitch = m_state.camera.GetPitch();
		}
	}
}

//--------------------------------------------------------------------------------------
//
// OnRender, updates the state from the UI, animates, transforms and renders the scene
//
//--------------------------------------------------------------------------------------
void FfxCacaoSample::OnRender()
{
	// Get timings
	//
	double timeNow = MillisecondsNow();
	m_deltaTime = timeNow - m_lastFrameTime;
	m_lastFrameTime = timeNow;

	// Set animation time
	//
	if (m_bPlay)
	{
		m_time += (float)m_deltaTime / 1000.0f;
	}

	// Build UI and set the scene state. Note that the rendering of the UI happens later.
	//
	ImGUI_UpdateIO();
	ImGui::NewFrame();

	if (m_loadingStage)
	{
        // LoadScene needs to be called a number of times, the scene is not fully loaded until it returns 0
        // This is done so we can display a progress bar when the scene is loading
        m_loadingStage = m_Node->LoadScene(m_pGltfLoader, m_loadingStage);
		if (m_loadingStage == 0)
		{
			m_time = 0.0f;
		}
    }
#if FFX_CACAO_ENABLE_PROFILING
	else if (m_pGltfLoader && m_isBenchmarking)
	{
		// benchmarking takes control of the time, and exits the app when the animation is done

		if (m_benchmarkWarmUpFramesToRun)
		{
			--m_benchmarkWarmUpFramesToRun;
		}
		else
		{
			if (m_benchmarkScreenWidth != m_Width || m_benchmarkScreenHeight != m_Height || m_vsyncEnabled)
			{
				MessageBox(NULL, "Screen resolution not correct or vsync enabled!\n", "Benchmark Failed!", MB_ICONERROR);
				exit(0);
			}

			uint64_t gpuTicksPerSecond;
			FfxCacaoDetailedTiming timings = {};
			m_Node->GetCacaoTimings(&m_state, &timings, &gpuTicksPerSecond);
			double microsecondsPerGpuTick = 1000000.0 / (double)gpuTicksPerSecond;

			if (timings.numTimestamps)
			{
				std::vector<TimeStamp> timestamps;
				timestamps.reserve(timings.numTimestamps);

				for (uint32_t i = 0; i < timings.numTimestamps; ++i)
				{
					TimeStamp timestamp;
					timestamp.m_label = timings.timestamps[i].label;
					timestamp.m_microseconds = (float)(microsecondsPerGpuTick * (double)timings.timestamps[i].ticks);
					timestamps.push_back(timestamp);
				}

				std::string *pFilename = NULL;
				m_time = BenchmarkLoop(timestamps, &m_state.camera, (const std::string**)&pFilename);
			}
		}
	}
#endif
	else
	{
		BuildUI();
		if (m_bPlay)
		{
			m_time += (float)m_deltaTime / 1000.0f;
		}
	}


    // Animate and transform the scene
    //
    if (m_pGltfLoader)
    {
        m_pGltfLoader->SetAnimationTime(0, m_time);
        m_pGltfLoader->TransformScene(0, XMMatrixIdentity());
    }

    m_state.time = m_time;

    // Do Render frame using AFR 
    //
	m_Node->OnRender(&m_state, &m_swapChain);

    m_swapChain.Present();
}

//--------------------------------------------------------------------------------------
//
// WinMain
//
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    LPCSTR Name = "FFX CACAO DirectX 12 Sample v1.0";
    uint32_t Width = 1280;
    uint32_t Height = 720;

    // create new DX sample
    return RunFramework(hInstance, lpCmdLine, nCmdShow, new FfxCacaoSample(Name));
}
