[[vk::push_constant]]
struct pushConstant {
    float2 scale;
    float2 translate;
} pc;

struct VERTEX_INPUT {
	float2 pos   : POSITION;
	float2 uv    : TEXCOORD0;
	float4 color : COLOR0;
};

struct VERTEX_OUTPUT {
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

void main(in VERTEX_INPUT vertexInput, out VERTEX_OUTPUT vertexOutput, out float4 projPos : SV_Position)
{
    vertexOutput.color = vertexInput.color;
	vertexOutput.uv = vertexInput.uv;
    projPos = float4(vertexInput.pos * pc.scale + pc.translate, 0.f, 1.f);
}
