cbuffer ShaderParam : register(b0)
{
	matrix viewMatrix;
	matrix projectionMatrix;
};

struct VS_INPUT
{
	float3 Pos : POSITION;
	float4 Col : COLOR;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COL;
};

PS_INPUT vsMain(VS_INPUT pos)
{
	PS_INPUT o = (PS_INPUT)0;
	float4 coord = float4(pos.Pos, 1);
	coord = mul(coord, viewMatrix);
	o.Pos = mul(coord, projectionMatrix);
	o.Col = pos.Col;
	return o;
}

float4 psMain(PS_INPUT input) : SV_TARGET
{
	return input.Col;
}
