#      6                              main                  C:\Users\Admin\Desktop\Unrinity\Shaders\terrainvs.fx     W   �     #line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\terrainvs.fx"
#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders/common.fx"



struct POINT_LIGHT {
    float3 pos;
    float areaLight;
    float3 color;
    float intensity;
};

struct DIRECTIONAL_LIGHT{
    float3 pos;
    float attenuationParam;
    float3 color;
    float intensity;
    float3 direction;
};

[[vk::binding(0)]]
cbuffer COMMON_BUFFER : register(b0)
{
    float3 viewPos;
    float curTime;
    float4x4 worldViewProj;
};

[[vk::binding(1)]]
cbuffer LIGHT_BUFFER : register (b1) {
    DIRECTIONAL_LIGHT dirLight;
    POINT_LIGHT pointLight0;
    float4x4 dirLightViewProj;
    float4x4 pointLightViewProj;
    float3 ambientColor;
};

[[vk::binding(3)]]
cbuffer MATERIAL_BUFFER : register (b3) {
    float3 materialDbgMetalness;
    float materialDbgRoughness;
};


[[vk::binding(30)]] Texture2D texShadowMap;


[[vk::binding(120)]] SamplerState anisoSampler;
[[vk::binding(121)]] SamplerState linearSampler;
[[vk::binding(122)]] SamplerState linearClampSampler;
[[vk::binding(123)]] SamplerState pointClampSampler;
[[vk::binding(124)]] SamplerComparisonState cmpPointClampSampler;

static const float4x4 projToScreenMat = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\terrainvs.fx"

#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders/terrainCommon.fx"
[[vk::binding(2)]]
cbuffer terrainCommon : register (b2) {
    float2 terrainStartPos;
};

static const float TERRAIN_BLOCK_SIZE = 12.f;
static const float TERRAIN_VERTEX_PER_COLUMN_LINE = 4.f;
static const float TERRAIN_VERTEX_OFFSET = TERRAIN_BLOCK_SIZE / (TERRAIN_VERTEX_PER_COLUMN_LINE - 1.f);

struct VERTEX_OUTPUT
{
    float4 worldPos : POSITION;
};
#line 2 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\terrainvs.fx"


[[vk::push_constant]]
struct PUSH_CONSTANT {
    float2 terrainBlockPos;
} pushConstant;

void main(int vertexId : SV_VertexID, out VERTEX_OUTPUT vertexOut, out float4 projPos : SV_Position)
{
    float2 offset = float2((vertexId >> 2) & 3, vertexId & 3) * TERRAIN_VERTEX_OFFSET;
    vertexOut.worldPos = float4(
        pushConstant.terrainBlockPos.x + offset.x,
        0.f,
        pushConstant.terrainBlockPos.y + offset.y,
        1.f);
    projPos = mul(worldViewProj, vertexOut.worldPos);
}
        C:\Users\Admin\Desktop\Unrinity\Shaders/common.fx        C:\Users\Admin\Desktop\Unrinity\Shaders/terrainCommon.fx         type.COMMON_BUFFER           viewPos         curTime         worldViewProj     	   COMMON_BUFFER    
 
   type.PushConstant.PUSH_CONSTANT   
       terrainBlockPos      pushConstant         out.var.POSITION         main    G        *   G            G            G  	   "       G  	   !       H         #       H        #      H        #      H              H           G        H  
       #       G  
               +            +          �?+          �@            +           +           +            +                                                                                   
            	   
                                !              	                   ;     	      ;        	        	      ;                      ;                	   Y   ;                	      6               �  !        	      =     "      �     #   "              ,   �     $   #                 o     %   $           :   �     &   "              1   o     '   &              P     (   %   '           ?   �     )   (      A     *            =     +   *   Q     ,   )               (   �     -   +   ,   A     .            =     /   .   Q     0   )              (   �     1   /   0           	   P     2   -      1                 A      3   	      =     4   3              �     5   2   4              >     2        	   Y   >     5   �  8  