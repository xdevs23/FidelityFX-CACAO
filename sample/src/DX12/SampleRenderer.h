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
#pragma once

#include "ffx_cacao.h"

static const int backBufferCount = 2;

#define USE_VID_MEM true

using namespace CAULDRON_DX12;

//
// This class deals with the GPU side of the sample.
//
class SampleRenderer
{
public:
    struct Spotlight
    {
        Camera light;
        XMVECTOR color;
        float intensity;
    };

    struct State
    {
        float time;
        Camera camera;

        float exposure;
        float iblFactor;
        float emmisiveFactor;

        int   toneMapper;
        int   skyDomeType;
        bool  bDrawBoundingBoxes;

        uint32_t  spotlightCount;
        Spotlight spotlight[4];
        bool  bDrawLightFrustum;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
		bool bUseDownsampledSSAO;
#endif
		bool bDisplayCacaoDirectly;
		bool bUseCACAO;
		FfxCacaoSettings cacaoSettings;
    };

    void OnCreate(Device* pDevice, SwapChain *pSwapChain);
    void OnDestroy();

    void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
    void OnDestroyWindowSizeDependentResources();

    int LoadScene(GLTFCommon *pGLTFCommon, int stage = 0);
    void UnloadScene();

    const std::vector<TimeStamp> &GetTimingValues() { return m_TimeStamps; }

    void OnRender(State *pState, SwapChain *pSwapChain);

#ifdef FFX_CACAO_ENABLE_PROFILING
	void GetCacaoTimings(State *pState, FfxCacaoDetailedTiming* timings, uint64_t* gpuTicksPerSecong);
#endif

private:
    Device                         *m_pDevice;

    uint32_t                        m_Width;
    uint32_t                        m_Height;
	uint32_t                        m_HalfWidth;
	uint32_t                        m_HalfHeight;
    D3D12_VIEWPORT                  m_viewPort;
    D3D12_RECT                      m_RectScissor;
    bool                            m_HasTAA = false;

    // Initialize helper classes
    ResourceViewHeaps               m_resourceViewHeaps;
    UploadHeap                      m_UploadHeap;
    DynamicBufferRing               m_ConstantBufferRing;
    StaticBufferPool                m_VidMemBufferPool;
    CommandListRing                 m_CommandListRing;
    GPUTimestamps                   m_GPUTimer;

    //gltf passes
	GltfPbrPass                    *m_gltfPBRNonMsaa;
    GltfPbrPass                    *m_gltfPBR;
    GltfBBoxPass                   *m_gltfBBox;
    GltfDepthPass                  *m_gltfDepth;
    GLTFTexturesAndBuffers         *m_pGLTFTexturesAndBuffers;

    // effects
    Bloom                           m_bloom;
    SkyDome                         m_skyDome;
    DownSamplePS                    m_downSample;
    SkyDomeProc                     m_skyDomeProc;
    ToneMapping                     m_toneMapping;
    PostProcCS                      m_motionBlur;
    Sharpen                         m_sharpen;
    TAA                             m_taa;

	// ================================
	// CACAO stuff
	CBV_SRV_UAV                     m_applyDirectInput;
	PostProcPS                      m_applyDirect;
	PostProcCS                      m_cacaoUavClear;

	Texture                         m_FfxCacaoOutput;
	CBV_SRV_UAV                     m_FfxCacaoOutputUAV;
	CBV_SRV_UAV                     m_FfxCacaoOutputSRV;

#ifdef FFX_CACAO_ENABLE_NATIVE_RESOLUTION
	FfxCacaoD3D12Context           *m_pFfxCacaoContextNative;
#endif
	FfxCacaoD3D12Context           *m_pFfxCacaoContextDownsampled;

    // GUI
    ImGUI                           m_ImGUI;

	// Deferred pass buffers
	Texture     m_depthBufferNonMsaa;
	DSV         m_depthBufferNonMsaaDSV;
	CBV_SRV_UAV m_depthBufferNonMsaaSRV;

	Texture     m_normalBufferNonMsaa;
	RTV         m_normalBufferNonMsaaRTV;
	CBV_SRV_UAV m_normalBufferNonMsaaSRV;

    // depth buffer
    Texture                         m_depthBuffer;
    DSV                             m_depthBufferDSV;
    CBV_SRV_UAV                     m_depthBufferSRV;

    // TAA buffer
    Texture                         m_TAABuffer;
    CBV_SRV_UAV                     m_TAABufferSRV;
    CBV_SRV_UAV                     m_TAABufferUAV;
    CBV_SRV_UAV                     m_TAAInputsSRV;
    Texture                         m_HistoryBuffer;
    RTV                             m_HistoryBufferRTV;

    // shadowmaps
    Texture                         m_ShadowMap;
    DSV                             m_ShadowMapDSV;
    CBV_SRV_UAV                     m_ShadowMapSRV;

    // MSAA RT
    Texture                         m_HDRMSAA;
    RTV                             m_HDRRTVMSAA;

    // Resolved RT
    Texture                         m_HDR;
    CBV_SRV_UAV                     m_HDRSRV;
    RTV                             m_HDRRTV;

    // widgets
    Wireframe                       m_wireframe;
    WireframeBox                    m_wireframeBox;

    std::vector<TimeStamp>          m_TimeStamps;
};
