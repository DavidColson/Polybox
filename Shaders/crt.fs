// Copyright 2020-2022 David Colson. All rights reserved.

$input v_texcoord0

#include "common.sh"

uniform vec4 u_crtData; // x, y are screen resolutions, z is time

SAMPLER2D(fullscreenFrameSampler, 0);

float2 curve(float2 uv)
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
	float2 q = v_texcoord0.xy;
	float2 uv = q;
	uv = curve( uv );

	float3 col;
	
	float time = u_crtData.z;
	float2 resolution = u_crtData.xy; 

	// This offsets the UV lookuups over time and makes pixels move around a little bit
	float x =  
		  sin(0.3*time + uv.y*10.0)
		* sin(0.7*time + uv.y*30.0)
		* sin(0.3+0.33*time + uv.y*20.0)
		* 0.001;

	// Vignette
	float vig = (0.0 + 16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y));

	// Offsets for each colour channel lookup, use the vignette value to make the chromatic abberation worse at the edge of the screen
	float3 colorOffX = float3(0.0015, 0.0, -0.0017) * (1.0 - pow(vig, 0.8) + 0.3);
	float3 colorOffY = float3(0.0, -0.0016, 0.0) * (1.0 - pow(vig, 0.8) + 0.3);
	/*float3 colorOffX = float3(0.0, 0.0, 0.0);
	float3 colorOffY = float3(0.0, 0.0, 0.0);*/

	// Sample the frame texture, and then offset the channels to create some chromatic abberation
	col.r = texture2D(fullscreenFrameSampler, float2(x + uv.x + colorOffX.r, uv.y + colorOffY.r)).x + 0.1;
	col.g = texture2D(fullscreenFrameSampler, float2(x + uv.x + colorOffX.g, uv.y + colorOffY.g)).y + 0.1;
	col.b = texture2D(fullscreenFrameSampler, float2(x + uv.x + colorOffX.b, uv.y + colorOffY.b)).z + 0.1;

	// Some kind of pre vignette tonemapping
	col = clamp(col*0.6+0.4*col*col*1.0,0.0,1.0);

	// Apply the vignette darkening
	col *= pow(vig, 0.3);
	col *= 2.5;

	// Draw scanlines
	float scans = clamp( 0.35 + 0.35 * sin(3.5 * time + uv.y * resolution.y * 1.5), 0.0, 1.0);
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
	col*=1.0-0.65*clamp((fmod(v_texcoord0.x, 2.0)-1.0)*2.0,0.0,1.0);
	
	gl_FragColor = float4(col,1.0);
}
