$input v_color0, v_texcoord0

#include "common.sh"

SAMPLER2D(colorTextureSampler, 0);

void main()
{	
    gl_FragColor = float4(v_color0.r, v_color0.g, v_color0.b, v_color0.a * texture2D(colorTextureSampler, v_texcoord0.xy).r);
    gl_FragColor = ditherAndPosterize(gl_FragCoord.xy, gl_FragColor, 32);
    clip(gl_FragColor.a - 0.00001);
}
