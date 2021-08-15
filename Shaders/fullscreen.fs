$input v_texcoord0

#include "common.sh"

SAMPLER2D(fullscreenFrameSampler, 0);

void main()
{
	gl_FragColor = texture2D(fullscreenFrameSampler, v_texcoord0.xy);
}
