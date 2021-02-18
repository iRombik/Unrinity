#      (                              main                           C:\Users\Admin\Desktop\Unrinity\Shaders\uivs.fx  ~    �     #include "common.fx"

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
    projPos = float4(vertexInput.pos * uiScale + uiTranslate, 0.f, 1.f);
}
    	   type.UI_BUFFER    	       uiScale   	      uiTranslate   
   UI_BUFFER        in.var.POSITION      in.var.TEXCOORD0         in.var.COLOR0        out.var.COLOR0       out.var.TEXCOORD0        main    G            G            G           G           G            G           G  
   "       G  
   !      H  	       #       H  	      #      G  	               +            +          �?            +            +                                   	                  	                                                        !                    ;     
      ;           ;           ;           ;           ;           ;           6               �     =           =           =                   (   A        
      =                    &   �     !                  2   A     "   
      =     #   "           0   �     $   !   #              Q     %   $       Q     &   $      P     '   %   &         = >        >        >     '              �  8  