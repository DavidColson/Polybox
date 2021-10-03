$input v_fogDensity, v_color0, v_texcoord0

#include "common.sh"

#if TEXTURING
	SAMPLER2D(colorTextureSampler,  0);
#endif

uniform vec4 u_fogColor;

void main()
{	
	#if TEXTURING
		float4 color = v_color0 * texture2D(colorTextureSampler, v_texcoord0.xy);
	#else
		float4 color = v_color0;
	#endif
	float3 output = mix(color.rgb, u_fogColor.rgb, v_fogDensity.x);
	gl_FragColor = float4(output, color.a);
}
