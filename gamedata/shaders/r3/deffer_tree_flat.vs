#include "common.h"
#include "check_screenspace.h"
#ifdef SSFX_TAA
	#include "screenspace_mvectors.h"
#endif

uniform float3x4 m_xform;
uniform float3x4 m_xform_v;
uniform float4 consts;
uniform float4 c_scale, c_bias, wind, wave;
uniform float2 c_sun;

float4 prev_wave;
float4 prev_wind;

#ifdef SSFX_WIND
	#include "screenspace_wind.h"
#endif

v2p_flat main(v_tree I)
{
	I.Nh = unpack_D3DCOLOR(I.Nh);
	I.T  = unpack_D3DCOLOR(I.T);
	I.B  = unpack_D3DCOLOR(I.B);

	v2p_flat o;

	float3 pos = mul(m_xform, I.P);
	float H    = pos.y - m_xform._24;
	float4 P_prev = float4(pos, 1.0f);

#ifndef SSFX_WIND
	float dp             = calc_cyclic(wave.w + dot(pos, (float3)wave));
	float frac_v         = I.tc.z * consts.x;
	float inten          = H * dp;
	float2 wind_result   = calc_xz_wave(wind.xz * inten, frac_v);
	dp                   = calc_cyclic(prev_wave.w + dot(pos, (float3)prev_wave));
	inten                = H * dp;
	float2 p_wind_result = calc_xz_wave(prev_wind.xz * inten, frac_v);
	P_prev               = float4(pos.x + p_wind_result.x, pos.y, pos.z + p_wind_result.y, 1.0f);
#else
	float2 wind_result      = ssfx_wind_tree_trunk(pos, H, ssfx_wind_setup(), false).xy;
	float2 prev_wind_result = ssfx_wind_tree_trunk(pos, H, ssfx_wind_setup(), true).xy;
	P_prev                  = float4(pos.x + prev_wind_result.x, pos.y, pos.z + prev_wind_result.y, 1.0f);
#endif

#ifdef USE_TREEWAVE
	wind_result = 0;
#endif

	float4 f_pos = float4(pos.x + wind_result.x, pos.y, pos.z + wind_result.y, 1.0f);

	float3 Pe   = mul(m_V, f_pos);
	float hemi  = I.Nh.w * c_scale.w + c_bias.w;
	o.hpos      = mul(m_VP, f_pos);
	o.N         = normalize(mul((float3x3)m_xform_v, I.P.xyz));
	o.tcdh      = float4((I.tc * consts).xyyy);
	o.position  = float4(Pe, hemi);
	o.RDrops    = float4(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef SSFX_TAA
	o.vel_curr = o.hpos;
	o.vel_prev = mul(m_vp_prev, P_prev);
	o.hpos.xy  = ssfx_taa_jitter(o.hpos);
#endif

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	o.tcdh.w = I.Nh.w * c_sun.x + c_sun.y;
#endif

#ifdef USE_TDETAIL
	o.tcdbump = o.tcdh * dt_params;
#endif

	return o;
}
FXVS;
