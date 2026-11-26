#include "common.h"
#include "check_screenspace.h"
#ifdef SSFX_TAA
	#include "screenspace_mvectors.h"
#endif

uniform float4 consts;
uniform float4 array[61*4];

v2p_flat main(v_detail v)
{
	v2p_flat O;
	int i     = v.misc.w;
	float4 m0 = array[i+0];
	float4 m1 = array[i+1];
	float4 m2 = array[i+2];
	float4 c0 = array[i+3];

	float4 pos;
	pos.x = dot(m0, v.pos);
	pos.y = dot(m1, v.pos);
	pos.z = dot(m2, v.pos);
	pos.w = 1.0f;

	float3 norm;
	norm.x = pos.x - m0.w;
	norm.y = pos.y - m1.w + 0.75f;
	norm.z = pos.z - m2.w;

	float4 Pp  = mul(m_WVP, pos);
	O.hpos     = Pp;
	float3 orig = mul(m_WV, normalize(norm));
	O.RDrops   = float4(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef SSFX_TAA
	O.vel_curr = O.hpos;
	O.vel_prev = mul(m_wvp_prev, pos);
	O.hpos.xy  = ssfx_taa_jitter(O.hpos);
#endif

	O.N        = lerp(orig, mul((float3x3)m_WV, v.pos), 0.25f);
	float3 Pe  = mul(m_WV, pos);
	O.tcdh     = float4((v.misc * consts).xyyy);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w = c0.x;
#endif

#ifdef USE_TDETAIL
	O.tcdbump = O.tcdh * dt_params;
#endif

#ifdef USE_LM_HEMI
	O.lmh = 0.0f;
#endif

	O.position = float4(Pe, c0.w);
	return O;
}
FXVS;
