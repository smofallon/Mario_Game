//--------------------------------------------------------------------------------------
// File: Tutorial022.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer VS_CONSTANT_BUFFER : register(b0)
	{
	float cb_a;
	float cb_b;
	float cb_c;
	float cb_d;
	};

struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;	
	output.Pos = input.Pos;
	output.Pos.x *= cb_c;
	output.Pos.y *= cb_c;

	output.Pos.x += cb_a;
	output.Pos.y += cb_b;

	output.Tex = input.Tex;

	if(output.Tex.x == 0.1 && cb_d == 1000) // correcting Mario texture
		output.Tex.x += .9;
	else if (cb_d != 1000){ // Moving background
		output.Tex.x -= cb_d/30.0;
		output.Pos.x -= cb_d;
	}
	
	return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	
	float2 texture_coordinates = input.Tex;
	float4 color = txDiffuse.Sample(samLinear, texture_coordinates);
	
	return color;
}
