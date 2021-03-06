// AMD SampleDX12 sample code
// 
// Copyright(c) 2017 Advanced Micro Devices, Inc.All rights reserved.
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
#pragma once

#include "SampleRenderer.h"

#include "ffx_cacao.h"

class FfxCacaoSample : public FrameworkWindows
{
public:
    FfxCacaoSample(LPCSTR name);
    void OnCreate(HWND hWnd);
    void OnDestroy();
	void BuildUI();
	void OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight, bool *pbFullScreen);
	void OnRender();
    bool OnEvent(MSG msg);
	void OnResize(uint32_t width, uint32_t height) { OnResize(width, height, false); }
    void OnResize(uint32_t Width, uint32_t Height, bool force);
    void SetFullScreen(bool fullscreen);
    
private:
	HWND                  m_hWnd;

    Device                m_device;
    SwapChain             m_swapChain;

    GLTFCommon           *m_pGltfLoader = NULL;

    SampleRenderer       *m_Node = NULL;
    SampleRenderer::State m_state;

	int                   m_loadingStage = 0;
	bool                  m_requiresLoad = true;
	int                   m_preset;

    float                 m_distance;
    float                 m_roll;
    float                 m_pitch;

    float                 m_time;             // WallClock in seconds.
    double                m_deltaTime;        // The elapsed time in milliseconds since the previous frame.
    double                m_lastFrameTime;

	bool                  m_isCapturing = false;
	bool                  m_vsyncEnabled = false;
	int                   m_cameraControlSelected = 0;
    bool                  m_bPlay;
	bool                  m_displayGUI;
	bool                  m_fullscreen;

	// json config file
	json                        m_jsonConfigFile;
	std::vector<std::string>    m_sceneNames;
	int                         m_activeScene;
	int                         m_activeCamera;
	bool                        m_isCpuValidationLayerEnabled;
	bool                        m_isGpuValidationLayerEnabled;

	int                         m_presetIndex = 0;

#ifdef FFX_CACAO_ENABLE_PROFILING
	char     m_benchmarkFilename[1024];
	bool     m_isBenchmarking;
	uint32_t m_benchmarkScreenWidth;
	uint32_t m_benchmarkScreenHeight;
	uint32_t m_benchmarkWarmUpFramesToRun;
#endif
};
