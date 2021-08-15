$input v_color0, v_texcoord0

#include "common.sh"

#if TEXTURING
	SAMPLER2D(colorTextureSampler,  0);
#endif

void main()
{	
	#if TEXTURING
		gl_FragColor = v_color0 * texture2D(colorTextureSampler, v_texcoord0.xy);
	#else
		gl_FragColor = v_color0;
	#endif
}
