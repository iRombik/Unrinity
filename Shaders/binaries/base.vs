#      <                 GLSL.std.450                      main                      	     
   C:\Users\Admin\Desktop\Unrinity\Shaders\basevs.fx    7   �  
   #line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\basevs.fx"
#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders/baseCommon.fx"
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
#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders/baseCommon.fx"


struct VERTEX_INPUT
{
 float3 position : POSITION;
 float2 texCoord : TEXCOORD0;
 float3 normal : NORMAL;
};

struct VERTEX_OUTPUT
{
    float3 worldPos : POSITION;
    float3 worldNormal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct PIXEL_OUTPUT
{
    float4 outColor : SV_Target;
};
#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\basevs.fx"


[[vk::push_constant]]
struct PUSH_CONSTANT {
    float4x4 modelMatrix;
} pushConstant;

void main(in VERTEX_INPUT vertexIn, out float4 projPos : SV_Position, out VERTEX_OUTPUT vertexOut) {
    float4 worldPos = mul(pushConstant.modelMatrix, float4(vertexIn.position, 1.0f));
    projPos = mul(worldViewProj, worldPos);

    vertexOut.worldPos = worldPos.xyz;
    vertexOut.worldNormal = normalize(mul(pushConstant.modelMatrix, float4(vertexIn.normal, 0.f))).xyz;
    vertexOut.texCoord = vertexIn.texCoord;
}
      C:\Users\Admin\Desktop\Unrinity\Shaders/common.fx        C:\Users\Admin\Desktop\Unrinity\Shaders/baseCommon.fx        type.COMMON_BUFFER           viewPos         curTime         worldViewProj        COMMON_BUFFER    
    type.PushConstant.PUSH_CONSTANT          modelMatrix      pushConstant         in.var.POSITION      in.var.TEXCOORD0         in.var.NORMAL        out.var.POSITION         out.var.NORMAL    	   out.var.TEXCOORD0        main    G            G            G           G           G            G           G  	         G     "       G     !       H         #       H        #      H        #      H              H           G        H         #       H               H            G                 +            +          �?            +            +                                                                                    	                                                                     !           "   !  #   "      $   	         %         ;           ;        	              ;                      ;                      ;             
      :   ;                      ;                       ;                       ;  !   	        
         6  "          #   �  &           	   =     '              	   =     (              	   =     )        
   	      A  $   *           
   	   (   =     +   *     
   	   <   Q     ,   '       Q     -   '      Q     .   '      P     /   ,   -   .        
   	      �     0   /   +     
   
      A  %   1         =     2   1     
   
      �     3   0   2   O     4   0   0               
      L   Q     5   )       Q     6   )      Q     7   )      P     8   5   6   7        
      '   �     9   8   +     
              :      E   9   O     ;   :   :               
      :   >     3              >     4              >     ;              >  	   (   �  8  