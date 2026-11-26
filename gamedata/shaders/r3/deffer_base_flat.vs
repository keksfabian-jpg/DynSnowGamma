#include "common.h"

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
#define v_in v_static_color
#else
#define v_in v_static
#endif

#include "check_screenspace.h"
#ifdef SSFX_TAA
	#include "screenspace_mvectors.h"
#endif

v2p_flat main(v_in I)
{
	I.Nh = unpack_D3DCOLOR(I.Nh);
	I.T  = unpack_D3DCOLOR(I.T);
	I.B  = unpack_D3DCOLOR(I.B);

	v2p_flat O;
	float4 Pp = mul(m_WVP, I.P);
	O.hpos = Pp;
	O.N = mul((float3x3)m_WV, unpack_bx2(I.Nh));
	float3 Pe = mul(m_WV, I.P);

	float2 tc = unpack_tc_base(I.tc, I.T.w, I.B.w);
	O.tcdh = float4(tc.xyyy);
	O.position = float4(Pe, I.Nh.w);
	O.RDrops = float4(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef SSFX_TAA
	O.vel_curr = O.hpos;
	O.vel_prev = mul(m_wvp_prev, I.P);
	O.hpos.xy = ssfx_taa_jitter(O.hpos);
#endif

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w = I.color.w;
#endif

#ifdef USE_TDETAIL
	O.tcdbump = O.tcdh * dt_params;
#endif

#ifdef USE_LM_HEMI
	O.lmh = unpack_tc_lmap(I.lmh);
#endif

	return O;
}

FXVS;
