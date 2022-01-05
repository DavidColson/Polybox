$input v_color0, v_texcoord0

#include "common.sh"

SAMPLER2D(colorTextureSampler, 0);

void main()
{	
    gl_FragColor = v_color0 * texture2D(colorTextureSampler, v_texcoord0.xy);
    gl_FragColor = ditherAndPosterize(gl_FragCoord.xy, gl_FragColor, 32);
    clip(gl_FragColor.a - 0.00001);
}
