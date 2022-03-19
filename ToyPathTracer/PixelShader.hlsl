float4 main(float2 uv : TEXCOORD0) : SV_Target
{
	return float4(uv.x, uv.y, 0.0f, 1.0f);
}