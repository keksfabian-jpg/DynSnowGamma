// NOTE: This shader is used by static vehicles (cars, trucks, BTRs) on the map.
#include "common.h"
#include "check_screenspace.h"
#ifdef SSFX_TAA
	#include "screenspace_mvectors.h"
#endif

uniform float3x4 m_xform;
uniform float3x4 m_xform_v;
uniform float4 consts;
uniform float4 c_scale, c_bias;
uniform float2 c_sun;

v2p_bumped main(v_tree I)
{
	I.Nh = unpack_D3DCOLOR(I.Nh);
	I.T  = unpack_D3DCOLOR(I.T);
	I.B  = unpack_D3DCOLOR(I.B);

	float3 pos  = mul(m_xform, I.P);
	float2 tc   = (I.tc * consts).xy;
	float hemi  = I.Nh.w * c_scale.w + c_bias.w;
	float4 wpos = float4(pos, 1.0f);

	v2p_bumped O;
	float3 Pe   = mul(m_V, wpos);
	O.tcdh      = float4(tc.xyyy);
	O.hpos      = mul(m_VP, wpos);
	O.position  = float4(Pe, hemi);
	O.RDrops    = float4(0.0f, 0.0f, 0.0f, 0.0f);
	O.sss_extra = 0.0f;

#ifdef SSFX_TAA
	O.vel_curr = O.hpos;
	O.vel_prev = mul(m_vp_prev, wpos);
	O.hpos.xy  = ssfx_taa_jitter(O.hpos);
#endif

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w = I.Nh.w * c_sun.x + c_sun.y;
#endif

	float3 N = unpack_bx4(I.Nh);
	float3 T = unpack_bx4(I.T);
	float3 B = unpack_bx4(I.B);
	float3x3 xform = mul((float3x3)m_xform_v, float3x3(
		T.x, B.x, N.x,
		T.y, B.y, N.y,
		T.z, B.z, N.z
	));

	O.M1 = xform[0];
	O.M2 = xform[1];
	O.M3 = xform[2];

#ifdef USE_TDETAIL
	O.tcdbump = O.tcdh * dt_params;
#endif

	return O;
}
FXVS;
