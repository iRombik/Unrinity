#      (                              main               C:\Users\Admin\Desktop\Unrinity\Shaders\shadowvs.fx  �   �     #line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\shadowvs.fx"
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

[[vk::binding(2)]]
cbuffer MATERIAL_BUFFER : register (b2) {
};

[[vk::binding(6)]]
cbuffer UI_BUFFER
{
    float2 scale;
    float2 translate;
} ;

[[vk::binding(15)]]
cbuffer DEBUG_BUFFER : register (b15) {
    int visualizedDataType;
};


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
#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\shadowvs.fx"


struct VERTEX_INPUT
{
    float3 position : POSITION;
};

[[vk::push_constant]]
struct PUSH_CONSTANT {
    float4x4 modelMatrix;
} pushConstant;

void main(in VERTEX_INPUT vertexIn, out float4 projPos : SV_Position) {
    float4 worldPos = mul(pushConstant.modelMatrix, float4(vertexIn.position, 1.0f));
    projPos = mul(dirLightViewProj, worldPos);
}
         C:\Users\Admin\Desktop\Unrinity\Shaders/common.fx        type.LIGHT_BUFFER            dirLight            pointLight0         dirLightViewProj            pointLightViewProj          ambientColor         DIRECTIONAL_LIGHT            pos         attenuationParam            color           intensity           direction        POINT_LIGHT          pos         areaLight           color           intensity     	   LIGHT_BUFFER     
 
   type.PushConstant.PUSH_CONSTANT   
       modelMatrix      pushConstant         in.var.POSITION      main    G            G            G  	   "       G  	   !      H         #       H        #      H        #      H        #      H        #       H         #       H        #      H        #      H        #      H         #       H        #   0   H        #   P   H              H           H        #   �   H              H           H        #   �   G        H  
       #       H  
             H  
          G  
               +          �?            +            +                                                                                                                   
            	   
                                !              	                  ;     	      ;        	              ;                   :   ;                      6               �                =                      A                      (   =                   <   Q                Q     !         Q     "         P     #       !   "                 �     $   #                 A     %   	      =     &   %              �     '   $   &           :   >     '   �  8  