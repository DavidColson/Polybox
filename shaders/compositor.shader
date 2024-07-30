// Copyright 2020-2024 David Colson. All rights reserved.

@ctype mat4 Matrixf 
@ctype vec2 Vec2f 

@vs vs_compositor
uniform vs_compositor_params {
	mat4 mvp;
};

layout(location=0) in vec3 pos;
layout(location=1) in vec4 color0;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 normal;

out vec4 color;
out vec2 texcoords;

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);
	texcoords = texcoord;
}
@end

@fs fs_compositor
in vec4 color;
in vec2 texcoords;
out vec4 frag_color;

uniform fs_compositor_params {
	vec2 screenResolution;
	float time;
};

uniform texture2D core2DFrame;
uniform texture2D core3DFrame;
uniform sampler nearestSampler;

vec2 curve(vec2 uv)
{
	// To apply to mouse positions, remap mouse into 0-1 range, then do this, then back to pixel space
	uv = (uv - 0.5) * 2.0; // Put in -1 to 1 range
	uv *= 1.1;			// Decrease size slightly to give space for bulge
	uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0); // The actual bulge
	uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0); 
	uv  = (uv / 2.0) + 0.5; // Remap into right space again (0-1)
	uv =  uv *0.92 + 0.04; // increase size
	return uv;
}

void main()
{
	// Warp the UV coordiantes to make the screen warp effect
	vec2 q = texcoords.xy;
	vec2 uv = q;
	uv = curve( uv );

	// This offsets the UV lookuups over time and makes pixels move around a little bit
	float x =  
		  sin(0.3*time + uv.y*10.0)
		* sin(0.7*time + uv.y*30.0)
		* sin(0.3+0.33*time + uv.y*20.0)
		* 0.001;

	// Vignette
	float vig = (0.0 + 16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y));

	// Offsets for each colour channel lookup, use the vignette value to make the chromatic abberation worse at the edge of the screen
	vec4 colorOffX = vec4(0.0015, 0.0, -0.0017, 0.0) * (1.0 - pow(vig, 0.8) + 0.3);
	vec4 colorOffY = vec4(0.0, -0.0016, 0.0, 0.0) * (1.0 - pow(vig, 0.8) + 0.3);
	//colorOffX = vec4(0.0, 0.0, 0.0, 0.0);
	//colorOffY = vec4(0.0, 0.0, 0.0, 0.0);

	vec4 col2Dpixel;
	vec4 col3Dpixel;
	
	col2Dpixel.r = texture(sampler2D(core2DFrame, nearestSampler), vec2(x + uv.x + colorOffX.r, uv.y + colorOffY.r)).x + 0.1;
	col2Dpixel.g = texture(sampler2D(core2DFrame, nearestSampler), vec2(x + uv.x + colorOffX.g, uv.y + colorOffY.g)).y + 0.1;
	col2Dpixel.b = texture(sampler2D(core2DFrame, nearestSampler), vec2(x + uv.x + colorOffX.b, uv.y + colorOffY.b)).z + 0.1;
	col2Dpixel.a = texture(sampler2D(core2DFrame, nearestSampler), vec2(x + uv.x + colorOffX.a, uv.y + colorOffY.a)).w;

	col3Dpixel.r = texture(sampler2D(core3DFrame, nearestSampler), vec2(x + uv.x + colorOffX.r, uv.y + colorOffY.r)).x + 0.1;
	col3Dpixel.g = texture(sampler2D(core3DFrame, nearestSampler), vec2(x + uv.x + colorOffX.g, uv.y + colorOffY.g)).y + 0.1;
	col3Dpixel.b = texture(sampler2D(core3DFrame, nearestSampler), vec2(x + uv.x + colorOffX.b, uv.y + colorOffY.b)).z + 0.1;
	col3Dpixel.a = texture(sampler2D(core3DFrame, nearestSampler), vec2(x + uv.x + colorOffX.a, uv.y + colorOffY.a)).w;

	// alpha blend the two framebuffers together
	vec4 col = mix(col3Dpixel, col2Dpixel, col2Dpixel.a);

	// Some kind of pre vignette tonemapping
	col = clamp(col*0.6+0.4*col*col*1.0,0.0,1.0);

	// Apply the vignette darkening
	col *= pow(vig, 0.3);
	col *= 2.5;

	// Draw scanlines
	float scans = clamp( 0.35 + 0.35 * sin(3.5 * time + uv.y * screenResolution.y * 1.5), 0.0, 1.0);
	float s = pow(scans,1.7);
	col = col*(0.5 + 0.2*s) ;

	// Makes the screen flash subtley
	col *= 1.0+0.01*sin(110.0*time);

	// Set colour in corners to black
	if (uv.x < 0.0 || uv.x > 1.0)
		col *= 0.0;
	if (uv.y < 0.0 || uv.y > 1.0)
		col *= 0.0;
	
	// Seems to be normalizing output?
	col*=1.0-0.65*clamp((mod(texcoords.x, 2.0)-1.0)*2.0,0.0,1.0);
	
	frag_color = vec4(col.rgb, 1.0);
}
@end

@program compositor vs_compositor fs_compositor
