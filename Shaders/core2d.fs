$input v_color0, v_texcoord0

#include "common.sh"

SAMPLER2D(colorTextureSampler, 0);

void main()
{	
    gl_FragColor = texture2D(colorTextureSampler, v_texcoord0.xy);
}
