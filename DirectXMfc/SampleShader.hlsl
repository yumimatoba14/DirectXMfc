cbuffer ShaderParam : register(b0)
{
	matrix viewMatrix;
	matrix projectionMatrix;
	float pointSizeX;
	float pointSizeY;
	float pixelSizeX;
	float pixelSizeY;
};

struct VS_INPUT
{
	float3 Pos : POSITION;
	float4 Col : COLOR;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
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

float decideHalfPointSize(float givenPointSize, float pixelSize, float w)
{
	if (givenPointSize < 0) {
		return -0.5f * givenPointSize * w;	// givenPointSize has included pixelSize.
	}
	// draw as 1 pixel at minimum.
	return 0.5f * max(givenPointSize, pixelSize * w);
}

[maxvertexcount(4)]   // ジオメトリシェーダーで出力する最大頂点数
// ジオメトリシェーダー
void gsMain(point PS_INPUT inPoint[1],                       // ポイント プリミティブの入力情報
	inout TriangleStream<PS_INPUT> triStream  // トライアングル プリミティブの出力ストリーム
)
{
	PS_INPUT outPoint;
	const float halfSizeX = decideHalfPointSize(pointSizeX, pixelSizeX, inPoint[0].Pos[3]);
	const float halfSizeY = decideHalfPointSize(pointSizeY, pixelSizeY, inPoint[0].Pos[3]);

	outPoint.Pos = float4(inPoint[0].Pos[0] + halfSizeX, inPoint[0].Pos[1] + halfSizeY, inPoint[0].Pos[2], inPoint[0].Pos[3]);
	outPoint.Col = inPoint[0].Col;
	triStream.Append(outPoint);

	outPoint.Pos = float4(inPoint[0].Pos[0] - halfSizeX, inPoint[0].Pos[1] + halfSizeY, inPoint[0].Pos[2], inPoint[0].Pos[3]);
	outPoint.Col = inPoint[0].Col;
	triStream.Append(outPoint);

	outPoint.Pos = float4(inPoint[0].Pos[0] + halfSizeX, inPoint[0].Pos[1] - halfSizeY, inPoint[0].Pos[2], inPoint[0].Pos[3]);
	outPoint.Col = inPoint[0].Col;
	triStream.Append(outPoint);

	outPoint.Pos = float4(inPoint[0].Pos[0] - halfSizeX, inPoint[0].Pos[1] - halfSizeY, inPoint[0].Pos[2], inPoint[0].Pos[3]);
	outPoint.Col = inPoint[0].Col;
	triStream.Append(outPoint);

	triStream.RestartStrip();
}

float4 psMain(PS_INPUT input) : SV_TARGET
{
	return input.Col;
}
