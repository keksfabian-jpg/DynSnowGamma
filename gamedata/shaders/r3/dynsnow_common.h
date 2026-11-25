/**
 * DynSnow for S.T.A.L.K.E.R. G.A.M.M.A. - Shared 3D Snow Deformation & Weapon Snow Library
 * 100% Original Clean-Room HLSL Header | License: MIT
 *
 * Uniform channel layout (all values strictly in [0.0 .. 95.25] for CCC_Vector4 compatibility):
 *   shader_param_2.x = ground_freeze_factor (1.0 in winter)
 *   shader_param_2.y = snow_accumulation_factor (0.80 .. 1.00)
 *   shader_param_2.z = packed Trail Node 0 (newest step XZ on 28m torus)
 *   shader_param_2.w = floor(w)*0.1 = nearest campfire melt radius (0..3.5m), frac(w) = weapon_snow (0..0.95)
 *   shader_param_5.x = packed Trail Node 1
 *   shader_param_5.y = packed Trail Node 2
 *   shader_param_5.z = packed Trail Node 3
 *   shader_param_5.w = packed Nearest Campfire XZ on 28m torus
 */

#ifndef DYNSNOW_COMMON_H
#define DYNSNOW_COMMON_H

uniform float4 shader_param_5;

float dynsnow_hash21(float2 p)
{
	p = frac(p * float2(123.34f, 456.21f));
	p += dot(p, p + 45.32f);
	return frac(p.x * p.y);
}

float dynsnow_noise2(float2 p)
{
	float2 i = floor(p);
	float2 f = frac(p);
	f = f * f * (3.0f - 2.0f * f);
	float a = dynsnow_hash21(i);
	float b = dynsnow_hash21(i + float2(1.0f, 0.0f));
	float c = dynsnow_hash21(i + float2(0.0f, 1.0f));
	float d = dynsnow_hash21(i + float2(1.0f, 1.0f));
	return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

bool dynsnow_unpack_mod28(float packed, out float2 mod_xz)
{
	if (packed < 0.20f)
	{
		mod_xz = 0.0f;
		return false;
	}
	float raw = floor(packed * 2048.0f + 0.5f);
	float ix = floor(raw / 512.0f);
	float iz = raw - ix * 512.0f;
	mod_xz = float2((ix - 1.0f) * (28.0f / 379.0f), iz * (28.0f / 511.0f));
	return true;
}

float2 dynsnow_torus_delta(float2 p_mod, float2 node_mod)
{
	float2 d = p_mod - node_mod;
	return d - floor(d / 28.0f + 0.5f) * 28.0f;
}

float dynsnow_seg_dist(float2 p_rel, float2 a_rel, float2 b_rel, out float along_t)
{
	float2 ba = b_rel - a_rel;
	float len2 = dot(ba, ba);
	if (len2 > 12.25f) // > 3.5m jump/teleport: treat endpoints separately instead of bridging
	{
		along_t = 0.0f;
		return min(length(p_rel - a_rel), length(p_rel - b_rel));
	}
	along_t = (len2 > 0.0001f) ? saturate(dot(p_rel - a_rel, ba) / len2) : 0.0f;
	return length((p_rel - a_rel) - ba * along_t);
}

// Evaluates 3D snow trench depression [0..1], raised side snow berms [0..1], and 3D campfire melt [0..1]
void dynsnow_eval_deformation(float2 wpos_xz, float cam_rel_y, out float out_trench, out float out_berm, out float out_cf_melt, out float out_cf_slush)
{
	out_trench = 0.0f;
	out_berm = 0.0f;
	out_cf_melt = 0.0f;
	out_cf_slush = 0.0f;

	float2 p_mod = wpos_xz - floor(wpos_xz / 28.0f) * 28.0f;
	float2 cam_xz = float2(m_inv_V._m03, m_inv_V._m23);
	float2 c_mod = cam_xz - floor(cam_xz / 28.0f) * 28.0f;

	// Express pixel & trail nodes in smooth local meters relative to camera XZ
	float2 p_loc = wpos_xz - cam_xz;

	// Only deform ground near player vertical stance (-2.4m .. -0.2m from camera) or trail plane
	float stance_ok = (cam_rel_y < -0.15f && cam_rel_y > -2.85f) ? 1.0f : 0.0f;

	float min_d = length(p_loc) * (stance_ok > 0.5f ? 1.0f : 99.0f);
	float best_weight = 1.0f;
	float scallop = 1.0f;

	float2 n0_mod, n1_mod, n2_mod, n3_mod;
	bool ok0 = dynsnow_unpack_mod28(shader_param_2.z, n0_mod);
	bool ok1 = dynsnow_unpack_mod28(shader_param_5.x, n1_mod);
	bool ok2 = dynsnow_unpack_mod28(shader_param_5.y, n2_mod);
	bool ok3 = dynsnow_unpack_mod28(shader_param_5.z, n3_mod);

	float t_seg = 0.0f;
	if (ok0)
	{
		float2 n0_loc = dynsnow_torus_delta(n0_mod, c_mod);
		if (stance_ok > 0.5f)
		{
			float d0 = dynsnow_seg_dist(p_loc, float2(0.0f, 0.0f), n0_loc, t_seg);
			if (d0 < min_d) { min_d = d0; best_weight = 1.0f; scallop = 0.88f + 0.12f * cos(t_seg * 6.283f); }
		}
		if (ok1)
		{
			float2 n1_loc = dynsnow_torus_delta(n1_mod, c_mod);
			float d1 = dynsnow_seg_dist(p_loc, n0_loc, n1_loc, t_seg);
			if (d1 < min_d) { min_d = d1; best_weight = 0.94f; scallop = 0.86f + 0.14f * cos(t_seg * 6.283f); }
			if (ok2)
			{
				float2 n2_loc = dynsnow_torus_delta(n2_mod, c_mod);
				float d2 = dynsnow_seg_dist(p_loc, n1_loc, n2_loc, t_seg);
				if (d2 < min_d) { min_d = d2; best_weight = 0.82f; scallop = 0.86f + 0.14f * cos(t_seg * 6.283f); }
				if (ok3)
				{
					float2 n3_loc = dynsnow_torus_delta(n3_mod, c_mod);
					float d3 = dynsnow_seg_dist(p_loc, n2_loc, n3_loc, t_seg);
					if (d3 < min_d) { min_d = d3; best_weight = 0.65f; scallop = 0.86f + 0.14f * cos(t_seg * 6.283f); }
				}
			}
		}
	}

	if (min_d < 0.68f)
	{
		float edge_jitter = (dynsnow_noise2(wpos_xz * 7.5f) - 0.5f) * 0.055f;
		float d_w = min_d + edge_jitter;
		float furrow_w = 0.25f * scallop;
		out_trench = (1.0f - smoothstep(furrow_w * 0.45f, furrow_w * 1.15f, d_w)) * best_weight;
		out_berm = exp(-pow((d_w - (furrow_w * 1.32f)) / 0.11f, 2.0f)) * best_weight * (1.0f - out_trench);
	}

	// 3D Campfire Snow Melt Pit
	float cf_r = floor(shader_param_2.w) * 0.1f;
	float2 cf_mod;
	if (cf_r > 0.25f && dynsnow_unpack_mod28(shader_param_5.w, cf_mod))
	{
		float2 cf_delta = dynsnow_torus_delta(p_mod, cf_mod);
		float cf_dist = length(cf_delta);
		if (cf_dist < cf_r + 0.65f)
		{
			float n_cf = (dynsnow_noise2(wpos_xz * 2.4f) - 0.5f) * 0.28f + (dynsnow_noise2(wpos_xz * 6.8f) - 0.5f) * 0.10f;
			float eff_r = cf_r + n_cf;
			out_cf_melt = 1.0f - smoothstep(eff_r - 0.38f, eff_r + 0.08f, cf_dist);
			out_cf_slush = exp(-pow((cf_dist - (eff_r - 0.08f)) / 0.20f, 2.0f));
			out_trench *= (1.0f - out_cf_melt);
			out_berm *= (1.0f - out_cf_melt);
		}
	}
}

// 3D Model-Locked Snow & Frost Accumulation on 1st-Person Weapons & Hands (matches Inverno Demo!)
void dynsnow_apply_hud_snow(inout float3 albedo, inout float3 normal_ts, inout float gloss, float4 RDrops)
{
	float wpn_snow = frac(shader_param_2.w);
	if (wpn_snow <= 0.01f)
		return;

	// RDrops.w = upward-facing World Y+ normal component; RDrops.xyz = 3D object-space coordinates on gun & hands
	float up_face = saturate(RDrops.w * 1.35f + 0.05f);
	if (up_face <= 0.01f)
		return;

	float2 uv1 = RDrops.xz * 24.0f + RDrops.y * 9.0f;
	float2 uv2 = RDrops.xy * 52.0f - RDrops.z * 17.0f;
	float n_coarse = dynsnow_noise2(uv1);
	float n_medium = dynsnow_noise2(uv1 * 2.4f + float2(3.7f, 8.1f));
	float n_cryst  = dynsnow_noise2(uv2 * 2.1f);

	float frost_field = n_coarse * 0.50f + n_medium * 0.32f + n_cryst * 0.18f;
	float crevice = saturate(1.0f - length(normal_ts.xy) * 0.55f);

	float snow_pot = up_face * (0.32f + frost_field * 0.68f) * (0.75f + crevice * 0.35f) * (wpn_snow * 1.38f);
	float rime_mask = smoothstep(0.16f, 0.52f, snow_pot);
	float thick_flakes = smoothstep(0.42f, 0.78f, snow_pot);

	// Frosty rime tint + bright settled snow crystals on top of barrel, receiver, optic & gloves
	float3 rime_col = float3(0.76f, 0.83f, 0.93f);
	float3 flake_col = float3(0.90f, 0.94f, 0.99f);
	float3 target_snow = lerp(rime_col, flake_col, thick_flakes * (0.65f + 0.35f * n_cryst));

	albedo = lerp(albedo, target_snow, rime_mask * 0.86f);

	// Crystalline micro-bump on settled snow
	float2 frost_n = (float2(n_medium, n_cryst) - 0.5f) * 0.65f;
	normal_ts = lerp(normal_ts, float3(frost_n, 1.0f), thick_flakes * 0.72f);

	// Slight wet melt sheen when snow is thin/melting (< 0.30), matte icy sparkle when frozen thick
	float melt_wet = saturate(1.0f - abs(wpn_snow - 0.22f) * 4.5f) * up_face;
	gloss = lerp(gloss, 0.12f + n_cryst * 0.18f, rime_mask);
	gloss = max(gloss, melt_wet * 0.48f);
}

#endif
