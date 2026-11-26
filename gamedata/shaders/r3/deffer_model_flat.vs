#include "common.h"
#include "skin.h"
#include "check_screenspace.h"
#ifdef SSFX_TAA
	#include "screenspace_mvectors.h"
#endif

v2p_flat _main(v_model I, float is_skinned_entity)
{
	v2p_flat O;
	float3 Pe = mul(m_WV, I.P);
	O.hpos = mul(m_WVP, I.P);
	O.N = mul((float3x3)m_WV, (float3)I.N);
	O.RDrops = float4(0.25f, 0.50f, 0.75f, is_skinned_entity);

#ifdef SSFX_TAA
	O.vel_curr = O.hpos;
	O.vel_prev = mul(m_wvp_prev, I.P_prev);
	O.hpos.xy = ssfx_taa_jitter(O.hpos);
#endif

	O.tcdh = float4(I.tc.xyyy);

	float3 Nw = mul((float3x3)m_W, (float3)I.N);
	float3 hc_pos = (float3)hemi_cube_pos_faces;
	float3 hc_neg = (float3)hemi_cube_neg_faces;
	float3 hc_mixed = (Nw < 0) ? hc_neg : hc_pos;
	float hemi_val = saturate(dot(hc_mixed, abs(Nw)));

	O.position = float4(Pe, hemi_val);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w = L_material.y;
#endif

#ifdef USE_TDETAIL
	O.tcdbump = O.tcdh * dt_params;
#endif

	return O;
}

#ifdef SKIN_NONE
v2p_flat main(v_model v)           { return _main(v, 0.0f); }
#endif

#ifdef SKIN_0
v2p_flat main(v_model_skinned_0 v) { return _main(skinning_0(v), 1.0f); }
#endif

#ifdef SKIN_1
v2p_flat main(v_model_skinned_1 v) { return _main(skinning_1(v), 1.0f); }
#endif

#ifdef SKIN_2
v2p_flat main(v_model_skinned_2 v) { return _main(skinning_2(v), 1.0f); }
#endif

#ifdef SKIN_3
v2p_flat main(v_model_skinned_3 v) { return _main(skinning_3(v), 1.0f); }
#endif

#ifdef SKIN_4
v2p_flat main(v_model_skinned_4 v) { return _main(skinning_4(v), 1.0f); }
#endif

FXVS;
