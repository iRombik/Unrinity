#      )                              main                           C:\Users\Admin\Desktop\Unrinity\Shaders\uivs.fx  �   �     #line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\uivs.fx"
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
#line 1 "C:\\Users\\Admin\\Desktop\\Unrinity\\Shaders\\uivs.fx"


struct VERTEX_INPUT {
 float2 pos : POSITION;
 float2 uv : TEXCOORD0;
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
    projPos = float4(vertexInput.pos * scale + translate, 0.f, 1.f);
}
   	   C:\Users\Admin\Desktop\Unrinity\Shaders/common.fx     
   type.UI_BUFFER    
       scale     
      translate        UI_BUFFER        in.var.POSITION      in.var.TEXCOORD0         in.var.COLOR0        out.var.COLOR0       out.var.TEXCOORD0        main    G            G            G           G           G            G           G     "       G     !      H  
       #       H  
      #      G  
               +            +          �?            +            +                                   
                  
                                                        !                    ;                      ;                      ;                      ;                
      ;                      ;                   ]   ;                      6               �             	   =                   	   =                   	   =                   (   A               =     !               &   �     "      !           0   A     #         =     $   #           .   �     %   "   $              Q     &   %       Q     '   %      P     (   &   '              
      >                   >                ]   >     (   �  8  