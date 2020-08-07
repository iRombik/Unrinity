[[vk::binding(120)]] SamplerState anisoSampler;
[[vk::binding(16)]]  Texture2D texSource;

void main(in float2 texCoord : TEXCOORD0, out float4 outColor : SV_Target)
{
    outColor = texSource.Sample(anisoSampler, texCoord);
}
