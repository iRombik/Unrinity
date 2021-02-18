void main(int vertexId : SV_VertexID, out float4 projPos : SV_Position, out float2 texCoord : TEXCOORD0)
{
    texCoord = float2((vertexId << 1) & 2, vertexId & 2);
    projPos = float4(texCoord * 2.0f + -1.0f, 0.0f, 1.0f);
}
