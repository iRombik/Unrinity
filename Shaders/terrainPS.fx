[[vk::binding(30)]] Texture2D texShadowMap;

void main(out float4 outColor : SV_Target) {
    outColor = float4(0.05f, 0.4f, 0.2f, 1.f);
}