/**
 * DynSnow v1.4 for S.T.A.L.K.E.R. G.A.M.M.A. - Full-Map 3D Snow Deformation & Weapon Frost Library
 * 100% Original Clean-Room HLSL Header | License: MIT
 *
 * Full-Map Absolute World-Space Encoding ([-1024m .. +1024m] = 2048m x 2048m, ZERO Torus Wrap, ZERO Distance Cutoff!):
 *   shader_param_2.x      = ground_freeze_factor (1.0 in winter)
 *   shader_param_2.y      = snow_accumulation_factor (0.85 .. 1.00)
 *   shader_param_2.z      = Campfire 1 exact continuous float melt radius (0.000 .. 3.500 m)
 *   shader_param_2.w      = Campfire 2 exact continuous float melt radius (0.000 .. 3.500 m)
 *   hdr10_parameters4.y   = Campfire 1 lower 23 bits (16-bit Z + low 7-bit X)
 *   hdr10_parameters4.z   = Campfire 2 lower 23 bits (16-bit Z + low 7-bit X)
 *   heatvision_params2.z  = Campfire 1 & 2 upper X bits + active flags (3.1cm precision anywhere on 2km map!)
 *   hdr10_parameters4.w   = 1.0 + Continuous weapon frost/snow intensity ([1.000 .. 3.000])
 *   11 Full-Map Node-Pair Triplets = 22 Absolute World-Space Nodes (34 bits/node: 14-bit X, 14-bit Z = 12.5cm
 *   precision across 2048m map, 5-bit smooth fade 1..31, 1-bit connect_prev) across:
 *     hdr10_parameters3.xyzw, hdr10_parameters5.xyzw, hdr10_parameters6.xzw, hdr10_parameters7.yzw,
 *     hdr10_parameters8.xyzw, hdr10_parameters9.xyzw, hdr10_parameters10.xyz,
 *     heatvision_params3.xyzw, heatvision_params4.xyzw
 */

#ifndef DYNSNOW_COMMON_H
#define DYNSNOW_COMMON_H

#ifndef G_PUDDLES_FROZEN
uniform float4 shader_param_2;
#define G_PUDDLES_FROZEN
#endif

// Safe fallback snow & ice constants when Project INVERNO is not installed or overwritten by Atmospherics/SSS MCM
#ifndef G_TERRAIN_SNOW_STRENGTH
#define G_TERRAIN_SNOW_STRENGTH 1.25f
#endif
#ifndef G_ROOF_SNOW_STRENGTH
#define G_ROOF_SNOW_STRENGTH 0.82f
#endif
#ifndef G_TERRAIN_SNOW_SLOPE
#define G_TERRAIN_SNOW_SLOPE 0.38f
#endif
#ifndef G_TERRAIN_SNOW_COLOR
#define G_TERRAIN_SNOW_COLOR float3(0.93f, 0.96f, 1.00f)
#endif
#ifndef G_TERRAIN_SNOW_GLOSS
#define G_TERRAIN_SNOW_GLOSS 0.06f
#endif
#ifndef G_TERRAIN_SNOW_BUMP
#define G_TERRAIN_SNOW_BUMP 2.15f
#endif
#ifndef G_PUDDLES_RIPPLES_SCALE
#define G_PUDDLES_RIPPLES_SCALE 0.75f
#endif
#ifndef G_PUDDLES_CRACKS_INTENSITY
#define G_PUDDLES_CRACKS_INTENSITY 0.80f
#endif
#ifndef G_PUDDLES_ICE_ROUGHNESS
#define G_PUDDLES_ICE_ROUGHNESS 0.48f
#endif
#ifndef G_PUDDLES_TINT
#define G_PUDDLES_TINT float3(0.74f, 0.83f, 0.94f)
#endif
#ifndef G_PUDDLES_REFLECTIVITY
#define G_PUDDLES_REFLECTIVITY 0.28f
#endif

#ifndef HDR10_H_
uniform float4 hdr10_parameters3;
uniform float4 hdr10_parameters4;
uniform float4 hdr10_parameters5;
uniform float4 hdr10_parameters6;
uniform float4 hdr10_parameters7;
uniform float4 hdr10_parameters8;
uniform float4 hdr10_parameters9;
uniform float4 hdr10_parameters10;
#endif

uniform float4 heatvision_params2;
uniform float4 heatvision_params3;
uniform float4 heatvision_params4;
uniform float ssfx_is_underground;

#define DYNSNOW_WORLD_HALF 1600.0f
#define DYNSNOW_WORLD_SPAN 3200.0f

// Returns 0.0f on all underground/indoor levels (Agroprom UG, X-18, X-16, X-10, X-8, Jupiter UG, Sarcophagus, Warlab, Poselok UG, etc.)
// while providing instant 0.88f snow on outdoor frame 0 before Lua initializes shader_param_2.
float dynsnow_get_terrain_snow_accum()
{
	if (ssfx_is_underground > 0.5f)
		return 0.0f;
	// Lua sets heatvision_params2.x in [0.50 .. 2.50]; when underground, Lua explicitly sends shader_param_2.xy = (0, 0)
	bool lua_active = (heatvision_params2.x >= 0.45f && heatvision_params2.x <= 3.5f);
	if (lua_active)
	{
		if (shader_param_2.x < 0.01f && shader_param_2.y < 0.01f)
			return 0.0f;
		return saturate(shader_param_2.y);
	}
	return max(saturate(shader_param_2.y), 0.88f);
}

struct DynSnowContext
{
	bool has_trench;
	bool has_cf;
	float width_boost;
	int seg_count;
	float2 seg0_a;
	float2 seg0_b;
	float2 seg0_w;
	float2 seg1_a;
	float2 seg1_b;
	float2 seg1_w;
	int cf_count;
	float2 cf0_pos;
	float cf0_r;
	float2 cf1_pos;
	float cf1_r;
};

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

bool dynsnow_decode_one_fullmap_node(float f_coord, uint meta11, out float2 wpos_xz, out float strength, out bool conn)
{
	wpos_xz = 0.0f;
	strength = 0.0f;
	conn = false;

	uint ist = (meta11 >> 5u) & 31u;
	if (ist == 0u || f_coord < 0.49f || f_coord >= 0.99999994f)
		return false;

	uint c23 = (uint)floor((f_coord - 0.5f) * 16777216.0f + 0.5f);
	uint iz = c23 & 16383u;
	uint ix = ((c23 >> 14u) & 511u) | ((meta11 & 31u) << 9u);
	conn = ((meta11 >> 10u) & 1u) != 0u;

	wpos_xz = float2(
		-DYNSNOW_WORLD_HALF + ((float)ix + 0.5f) * (DYNSNOW_WORLD_SPAN / 16384.0f),
		-DYNSNOW_WORLD_HALF + ((float)iz + 0.5f) * (DYNSNOW_WORLD_SPAN / 16384.0f)
	);
	strength = (float)ist * (1.0f / 31.0f);
	return true;
}

bool dynsnow_unpack_fullmap_campfires(out float2 cf1_wpos, out bool cf1_ok, out float2 cf2_wpos, out bool cf2_ok)
{
	cf1_wpos = 0.0f; cf1_ok = false;
	cf2_wpos = 0.0f; cf2_ok = false;

	float f_hi = heatvision_params2.z;
	if (f_hi < 0.49f || f_hi >= 0.99999994f)
		return false;

	uint hi_code = (uint)floor((f_hi - 0.5f) * 8388608.0f + 0.5f);
	uint m1 = hi_code & 1023u;         // bit 9 = active1, bits 0..8 = ix1_hi9
	uint m2 = (hi_code >> 10u) & 1023u; // bit 9 = active2, bits 0..8 = ix2_hi9

	if (((m1 >> 9u) & 1u) != 0u && hdr10_parameters4.y >= 0.49f && hdr10_parameters4.y < 0.99999994f)
	{
		uint lo1 = (uint)floor((hdr10_parameters4.y - 0.5f) * 16777216.0f + 0.5f);
		uint iz1 = lo1 & 65535u;
		uint ix1 = ((lo1 >> 16u) & 127u) | ((m1 & 511u) << 7u);
		cf1_wpos = float2(-DYNSNOW_WORLD_HALF + ((float)ix1 + 0.5f) * (DYNSNOW_WORLD_SPAN / 65536.0f), -DYNSNOW_WORLD_HALF + ((float)iz1 + 0.5f) * (DYNSNOW_WORLD_SPAN / 65536.0f));
		cf1_ok = true;
	}

	if (((m2 >> 9u) & 1u) != 0u && hdr10_parameters4.z >= 0.49f && hdr10_parameters4.z < 0.99999994f)
	{
		uint lo2 = (uint)floor((hdr10_parameters4.z - 0.5f) * 16777216.0f + 0.5f);
		uint iz2 = lo2 & 65535u;
		uint ix2 = ((lo2 >> 16u) & 127u) | ((m2 & 511u) << 7u);
		cf2_wpos = float2(-DYNSNOW_WORLD_HALF + ((float)ix2 + 0.5f) * (DYNSNOW_WORLD_SPAN / 65536.0f), -DYNSNOW_WORLD_HALF + ((float)iz2 + 0.5f) * (DYNSNOW_WORLD_SPAN / 65536.0f));
		cf2_ok = true;
	}

	return (cf1_ok || cf2_ok);
}

float dynsnow_seg_dist(float2 p_w, float2 a_w, float2 b_w, out float along_t)
{
	float2 ba = b_w - a_w;
	float len2 = dot(ba, ba);
	if (len2 > 14400.0f) // Up to 120m resampled straight map segments supported!
	{
		float da = length(p_w - a_w);
		float db = length(p_w - b_w);
		along_t = (da < db) ? 0.0f : 1.0f;
		return min(da, db);
	}
	along_t = (len2 > 0.0001f) ? saturate(dot(p_w - a_w, ba) / len2) : 0.0f;
	return length((p_w - a_w) - ba * along_t);
}

// Single-pass per-pixel full-map spatial prefilter (0m .. 1500m+ across HIGH, MID, and LOW terrain shaders!)
void dynsnow_init_context(float2 wpos_xz, float dist_z, out DynSnowContext ctx)
{
	ctx.has_trench = false;
	ctx.has_cf = false;
	// Perspective-conserving anti-aliasing width boost so distant snow trenches (50m..1000m) always span >= 1.5-2 pixels cleanly!
	ctx.width_boost = 1.0f + clamp((dist_z - 25.0f) * 0.0095f, 0.0f, 3.6f);
	ctx.seg_count = 0;
	ctx.seg0_a = 0.0f; ctx.seg0_b = 0.0f; ctx.seg0_w = 0.0f;
	ctx.seg1_a = 0.0f; ctx.seg1_b = 0.0f; ctx.seg1_w = 0.0f;
	ctx.cf_count = 0;
	ctx.cf0_pos = 0.0f; ctx.cf0_r = 0.0f;
	ctx.cf1_pos = 0.0f; ctx.cf1_r = 0.0f;

	// 1. Full-map campfires
	float cf1_r = shader_param_2.z;
	float cf2_r = shader_param_2.w;
	if (cf1_r > 0.008f || cf2_r > 0.008f)
	{
		float2 cf1_p, cf2_p;
		bool ok1, ok2;
		if (dynsnow_unpack_fullmap_campfires(cf1_p, ok1, cf2_p, ok2))
		{
			if (ok1 && cf1_r > 0.008f && length(wpos_xz - cf1_p) < cf1_r + 1.2f * ctx.width_boost)
			{
				ctx.cf0_pos = cf1_p;
				ctx.cf0_r = cf1_r;
				ctx.cf_count = 1;
				ctx.has_cf = true;
			}
			if (ok2 && cf2_r > 0.008f && length(wpos_xz - cf2_p) < cf2_r + 1.2f * ctx.width_boost)
			{
				if (ctx.cf_count == 0)
				{
					ctx.cf0_pos = cf2_p;
					ctx.cf0_r = cf2_r;
					ctx.cf_count = 1;
				}
				else
				{
					ctx.cf1_pos = cf2_p;
					ctx.cf1_r = cf2_r;
					ctx.cf_count = 2;
				}
				ctx.has_cf = true;
			}
		}
	}

	// 2. Unpack 11 full-map triplets (33 floats = 22 absolute world-space trail nodes across the entire 2km map!)
	float raw[33] = {
		hdr10_parameters3.x, hdr10_parameters3.y, hdr10_parameters3.z,
		hdr10_parameters3.w, hdr10_parameters5.x, hdr10_parameters5.y,
		hdr10_parameters5.z, hdr10_parameters5.w, hdr10_parameters6.x,
		hdr10_parameters6.z, hdr10_parameters6.w, hdr10_parameters7.y,
		hdr10_parameters7.z, hdr10_parameters7.w, hdr10_parameters8.x,
		hdr10_parameters8.y, hdr10_parameters8.z, hdr10_parameters8.w,
		hdr10_parameters9.x, hdr10_parameters9.y, hdr10_parameters9.z,
		hdr10_parameters9.w, hdr10_parameters10.x, hdr10_parameters10.y,
		hdr10_parameters10.z, heatvision_params3.x, heatvision_params3.y,
		heatvision_params3.z, heatvision_params3.w, heatvision_params4.x,
		heatvision_params4.y, heatvision_params4.z, heatvision_params4.w
	};

	float search_r = 0.96f * ctx.width_boost;
	float best_d0 = search_r;
	float best_d1 = search_r;
	float2 prev_pos = 0.0f;
	float prev_w = 0.0f;
	bool has_prev = false;

	[loop] for (int t = 0; t < 11; ++t)
	{
		float f0 = raw[t * 3 + 0];
		float f1 = raw[t * 3 + 1];
		float f2 = raw[t * 3 + 2];

		if (f2 < 0.35f)
			break; // End of active node pairs
		if (f2 < 0.49f || f2 >= 0.99999994f)
		{
			has_prev = false;
			continue;
		}

		uint mpair = (uint)floor((f2 - 0.5f) * 8388608.0f + 0.5f);
		uint meta0 = mpair & 2047u;
		uint meta1 = (mpair >> 11u) & 2047u;

		[unroll] for (int sub = 0; sub < 2; ++sub)
		{
			float fc = (sub == 0) ? f0 : f1;
			uint mt  = (sub == 0) ? meta0 : meta1;
			float2 cur_pos;
			float cur_w;
			bool conn;
			if (!dynsnow_decode_one_fullmap_node(fc, mt, cur_pos, cur_w, conn))
			{
				has_prev = false;
			}
			else
			{
				if (conn && has_prev)
				{
					float2 bb_min = min(prev_pos, cur_pos) - search_r;
					float2 bb_max = max(prev_pos, cur_pos) + search_r;
					if (wpos_xz.x >= bb_min.x && wpos_xz.x <= bb_max.x && wpos_xz.y >= bb_min.y && wpos_xz.y <= bb_max.y)
					{
						float t_seg;
						float d = dynsnow_seg_dist(wpos_xz, prev_pos, cur_pos, t_seg);
						if (d < best_d0)
						{
							best_d1 = best_d0;
							ctx.seg1_a = ctx.seg0_a; ctx.seg1_b = ctx.seg0_b; ctx.seg1_w = ctx.seg0_w;
							best_d0 = d;
							ctx.seg0_a = prev_pos; ctx.seg0_b = cur_pos; ctx.seg0_w = float2(prev_w, cur_w);
							ctx.seg_count = min(ctx.seg_count + 1, 2);
						}
						else if (d < best_d1)
						{
							best_d1 = d;
							ctx.seg1_a = prev_pos; ctx.seg1_b = cur_pos; ctx.seg1_w = float2(prev_w, cur_w);
							ctx.seg_count = 2;
						}
					}
				}
				prev_pos = cur_pos;
				prev_w = cur_w;
				has_prev = true;
			}
		}
	}

	ctx.has_trench = (ctx.seg_count > 0);
}

void dynsnow_eval_single_seg(float2 wpos_xz, float2 a, float2 b, float2 w_ab, float width_boost, float edge_jitter, inout float out_trench, inout float out_berm)
{
	float along_t;
	float d = dynsnow_seg_dist(wpos_xz, a, b, along_t);
	float max_r = 0.72f * width_boost;
	if (d >= max_r)
		return;

	float seg_w = lerp(w_ab.x, w_ab.y, along_t);
	if (seg_w <= 0.005f)
		return;

	float width_scale = (0.55f + 0.45f * smoothstep(0.0f, 0.65f, seg_w)) * width_boost;
	// World-space stride ripple (works identically on 0.7m near segments AND 40m distant resampled segments!)
	float scallop = 0.93f + 0.07f * sin(dot(wpos_xz, float2(3.1f, 2.7f)));
	float furrow_w = 0.255f * width_scale * scallop;
	float d_w = d + edge_jitter * width_boost;

	float tr = (1.0f - smoothstep(furrow_w * 0.42f, furrow_w * 1.18f, d_w)) * seg_w;
	float bm = exp(-pow((d_w - (furrow_w * 1.34f)) / (0.115f * width_boost), 2.0f)) * (seg_w * seg_w) * (1.0f - tr);

	out_trench = max(out_trench, tr);
	out_berm = max(out_berm, bm);
}

void dynsnow_eval_single_cf(float2 wpos_xz, float2 cf_pos, float cf_r, float width_boost, inout float out_cf_melt, inout float out_cf_slush)
{
	float cf_dist = length(wpos_xz - cf_pos);
	if (cf_dist >= cf_r + 0.85f * width_boost)
		return;

	float onset = smoothstep(0.01f, 0.45f, cf_r);
	float n_cf = (dynsnow_noise2(wpos_xz * 2.1f) - 0.5f) * 0.26f
	           + (dynsnow_noise2(wpos_xz * 5.8f + float2(3.1f, 1.7f)) - 0.5f) * 0.10f;
	float eff_r = cf_r + n_cf * smoothstep(0.05f, 0.65f, cf_r);

	float melt = (1.0f - smoothstep(max(0.0f, eff_r - 0.70f), eff_r + 0.18f * width_boost, cf_dist)) * onset;
	melt = melt * melt * (3.0f - 2.0f * melt);
	float slush = exp(-pow((cf_dist - (eff_r - 0.14f)) / (0.32f * width_boost), 2.0f)) * onset * (1.0f - melt * 0.55f);

	out_cf_melt = max(out_cf_melt, melt);
	out_cf_slush = max(out_cf_slush, slush);
}

void dynsnow_eval_cached(DynSnowContext ctx, float2 wpos_xz, out float out_trench, out float out_berm, out float out_cf_melt, out float out_cf_slush)
{
	out_trench = 0.0f;
	out_berm = 0.0f;
	out_cf_melt = 0.0f;
	out_cf_slush = 0.0f;

	if (!ctx.has_trench && !ctx.has_cf)
		return;

	if (ctx.has_trench)
	{
		float edge_jitter = (dynsnow_noise2(wpos_xz * 7.2f) - 0.5f) * 0.052f;
		dynsnow_eval_single_seg(wpos_xz, ctx.seg0_a, ctx.seg0_b, ctx.seg0_w, ctx.width_boost, edge_jitter, out_trench, out_berm);
		if (ctx.seg_count > 1)
			dynsnow_eval_single_seg(wpos_xz, ctx.seg1_a, ctx.seg1_b, ctx.seg1_w, ctx.width_boost, edge_jitter, out_trench, out_berm);
		out_berm *= (1.0f - out_trench);
	}

	if (ctx.has_cf)
	{
		dynsnow_eval_single_cf(wpos_xz, ctx.cf0_pos, ctx.cf0_r, ctx.width_boost, out_cf_melt, out_cf_slush);
		if (ctx.cf_count > 1)
			dynsnow_eval_single_cf(wpos_xz, ctx.cf1_pos, ctx.cf1_r, ctx.width_boost, out_cf_melt, out_cf_slush);
		out_trench *= (1.0f - out_cf_melt);
		out_berm *= (1.0f - out_cf_melt);
	}
}

// Subtle, Crystalline Frost & Fine Snow Dusting on 1st-Person Weapons
void dynsnow_apply_hud_snow(
	inout float3 albedo,
	inout float3 normal_ts,
	inout float gloss,
	float bump_height,
	float2 uv_tex,
	float4 RDrops,
	float3 view_pos,
	float3 world_bumped_n)
{
	if (hdr10_parameters4.w > 5.0f)
		return;
	float wpn_snow = hdr10_parameters4.w - 1.0f;
	if (wpn_snow <= 0.01f)
		return;

	float forward_gun_mask = smoothstep(0.20f, 0.35f, view_pos.z) * smoothstep(-0.115f, -0.025f, view_pos.y);
	if (forward_gun_mask <= 0.005f)
		return;

	float warm_skin = saturate((albedo.r - albedo.b * 1.18f) * 6.5f) * saturate(1.0f - gloss * 2.5f);
	float hard_surface = smoothstep(0.04f, 0.15f, gloss + abs(bump_height - 0.5f) * 0.35f);
	float surface_elig = forward_gun_mask * (1.0f - warm_skin) * (0.20f + 0.80f * hard_surface);

	float sky_y = saturate(world_bumped_n.y * 0.72f + RDrops.w * 0.28f);
	float top_mask = pow(saturate((sky_y - 0.20f) / 0.80f), 2.15f);
	if (top_mask <= 0.002f)
		return;

	float n_patch1 = dynsnow_noise2(uv_tex * 12.0f + RDrops.xz * 6.0f);
	float n_patch2 = dynsnow_noise2(uv_tex * 31.0f + float2(4.7f, 9.2f));
	float n_grain  = dynsnow_noise2(uv_tex * 92.0f + float2(1.9f, 6.4f));
	float n_speck  = dynsnow_noise2(uv_tex * 210.0f + float2(13.1f, 3.7f));

	float patch_field = n_patch1 * 0.56f + n_patch2 * 0.44f;
	float cavity = saturate((0.62f - bump_height) * 1.35f + length(normal_ts.xy) * 0.55f);

	float snow_pot = top_mask * (patch_field * 0.72f + cavity * 0.42f) * min(wpn_snow, 1.5f);
	float rime_alpha = smoothstep(0.24f, 0.70f, snow_pot) * 0.31f * min(wpn_snow, 1.25f);

	float crystal_grains = pow(saturate(n_speck * 1.12f), 4.0f) * smoothstep(0.32f, 0.76f, snow_pot * (0.60f + n_grain * 0.70f));
	float speck_alpha = crystal_grains * 0.50f * min(wpn_snow, 1.35f);

	float total_mask = saturate(rime_alpha + speck_alpha) * surface_elig;
	if (total_mask <= 0.002f)
		return;

	float base_luma = dot(albedo, float3(0.299f, 0.587f, 0.114f));
	float3 rime_col = float3(0.74f, 0.81f, 0.90f) * (0.68f + 0.32f * saturate(base_luma * 2.4f));
	float3 crystal_col = float3(0.90f, 0.94f, 0.99f);

	albedo = lerp(albedo, rime_col, rime_alpha * surface_elig);
	albedo = lerp(albedo, crystal_col, speck_alpha * surface_elig);

	normal_ts.xy += (float2(n_grain, n_speck) - 0.5f) * (total_mask * 0.28f);

	float melt_wet = saturate(1.0f - abs(wpn_snow - 0.18f) * 5.5f) * top_mask * surface_elig;
	gloss = lerp(gloss, 0.16f + n_speck * 0.22f, total_mask * 0.65f);
	gloss = max(gloss, melt_wet * 0.36f);
}

// Separate live MCM multipliers for Skinned Characters (NPCs & Mutants) vs World Objects (Cars, Roofs, Props)
// Encoded in heatvision_params2.xy as (0.50 + slider_value) in [0.50 .. 2.50]; > 3.5f = uninitialized user.ltx default
float dynsnow_get_model_snow_mult(bool is_skinned_npc_or_mutant)
{
	if (is_skinned_npc_or_mutant)
	{
		if (heatvision_params2.x > 3.5f || heatvision_params2.x < 0.45f)
			return 0.25f;
		return max(0.0f, heatvision_params2.x - 0.50f);
	}
	else
	{
		if (heatvision_params2.y > 3.5f || heatvision_params2.y < 0.45f)
			return 1.00f;
		return max(0.0f, heatvision_params2.y - 0.50f);
	}
}

#endif
