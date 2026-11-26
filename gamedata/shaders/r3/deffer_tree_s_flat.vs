// NOTE: Used by map vehicles (e.g. yellow-roof car).
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

v2p_flat main(v_tree I)
{
	I.Nh = unpack_D3DCOLOR(I.Nh);
	I.T  = unpack_D3DCOLOR(I.T);
	I.B  = unpack_D3DCOLOR(I.B);

	v2p_flat O;
	float3 pos  = mul(m_xform, I.P);
	float4 wpos = float4(pos, 1.0f);

	float3 Pe   = mul(m_V, wpos);
	float hemi  = I.Nh.w * c_scale.w + c_bias.w;

	O.hpos      = mul(m_VP, wpos);
	O.N         = mul((float3x3)m_xform_v, unpack_bx2(I.Nh));
	O.tcdh      = float4((I.tc * consts).xyyy);
	O.position  = float4(Pe, hemi);
	O.RDrops    = float4(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef SSFX_TAA
	O.vel_curr = O.hpos;
	O.vel_prev = mul(m_vp_prev, wpos);
	O.hpos.xy  = ssfx_taa_jitter(O.hpos);
#endif

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w = I.Nh.w * c_sun.x + c_sun.y;
#endif

#ifdef USE_TDETAIL
	O.tcdbump = O.tcdh * dt_params;
#endif

	return O;
}
FXVS;
