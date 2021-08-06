$input v_texcoord0

#include "common.sh"

SAMPLER2D(m_textureSampler, 0);

void main()
{
	gl_FragColor = texture2D(m_textureSampler, v_texcoord0.xy);
}
