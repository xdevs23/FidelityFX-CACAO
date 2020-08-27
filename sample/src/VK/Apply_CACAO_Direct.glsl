// AMD AMDUtils code
// 
// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
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

#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
layout (set=0, binding = 0, std140) uniform perFrame
{
    int u_dummy;
} myPerFrame;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outColor;

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
layout(set=0, binding=1) uniform sampler2D sSampler;

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
void main() 
{
    float color = texture(sSampler, inTexCoord.st).r;
	color = pow(color, 2.2);
	outColor = vec4(color, color, color, 1.0f);
}
