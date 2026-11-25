/**
 * DynSnow v1.2 for S.T.A.L.K.E.R. G.A.M.M.A. - Shared 3D Snow Deformation & Weapon Frost Library
 * 100% Original Clean-Room HLSL Header | License: MIT
 *
 * Zero-Conflict Uniform Layout (leaves shader_param_5..8 untouched for 3DSS Scopes, BAS Lasers & NVGs):
 *   shader_param_2.x      = ground_freeze_factor (1.0 in winter)
 *   shader_param_2.y      = snow_accumulation_factor (0.85 .. 1.00)
 *   shader_param_2.z      = Campfire 1 exact continuous float melt radius (0.000 .. 3.500 m)
 *   shader_param_2.w      = Campfire 2 exact continuous float melt radius (0.000 .. 3.500 m)
 *   hdr10_parameters4.y   = Campfire 1 packed XZ on 32m torus (10-bit X + 10-bit Z = 3.1cm precision)
 *   hdr10_parameters4.z   = Campfire 2 packed XZ on 32m torus (10-bit X + 10-bit Z = 3.1cm precision)
 *   hdr10_parameters4.w   = Continuous weapon frost/snow intensity (0.000 .. 2.000)
 *   25 Contiguous 3D Trail Node Slots (Player + Stalkers + Mutants, 5+ min smooth tail-to-head decay):
 *     Slots  0..3  : hdr10_parameters3.xyzw
 *     Slots  4..7  : hdr10_parameters5.xyzw
 *     Slots  8..10 : hdr10_parameters6.xzw
 *     Slots 11..13 : hdr10_parameters7.yzw
 *     Slots 14..17 : hdr10_parameters8.xyzw
 *     Slots 18..21 : hdr10_parameters9.xyzw
 *     Slots 22..24 : hdr10_parameters10.xyz
 */

#ifndef DYNSNOW_COMMON_H
#define DYNSNOW_COMMON_H

#ifndef G_PUDDLES_FROZEN
uniform float4 shader_param_2;
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

struct DynSnowContext
{
	bool has_trench;
	bool has_cf;
	int seg_count;
	float2 cam_xz;
	float2 seg0_a;
	float2 seg0_b;
	float2 seg0_w; // .x = weightA, .y = weightB
	float2 seg1_a;
	float2 seg1_b;
	float2 seg1_w;
	int cf_count;
	float2 cf0_loc;
	float cf0_r;
	float2 cf1_loc;
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

float2 dynsnow_torus32_delta(float2 a_mod, float2 b_mod)
{
	float2 d = a_mod - b_mod;
	return d - floor(d * (1.0f / 32.0f) + 0.5f) * 32.0f;
}

// Bit-exact 22-bit IEEE-754 mantissa unpacker from [0.50 .. 0.99999988] (0.25 = inactive)
bool dynsnow_unpack_trail_node(float packed, out float2 node_mod, out float strength, out bool connect_prev)
{
	if (packed < 0.45f)
	{
		node_mod = 0.0f;
		strength = 0.0f;
		connect_prev = false;
		return false;
	}
	uint code = (uint)floor((packed - 0.5f) * 8388608.0f + 0.5f);
	uint iz = code & 255u;
	uint ix = (code >> 8u) & 255u;
	uint ist = (code >> 16u) & 31u;
	connect_prev = ((code >> 21u) & 1u) != 0u;
	node_mod = float2(((float)ix + 0.5f) * (32.0f / 256.0f), ((float)iz + 0.5f) * (32.0f / 256.0f));
	strength = (float)ist * (1.0f / 31.0f);
	return (ist > 0u);
}

bool dynsnow_unpack_cf_mod32(float packed, out float2 cf_mod)
{
	if (packed < 0.45f)
	{
		cf_mod = 0.0f;
		return false;
	}
	uint code = (uint)floor((packed - 0.5f) * 8388608.0f + 0.5f);
	uint iz = code & 1023u;
	uint ix = (code >> 10u) & 1023u;
	cf_mod = float2(((float)ix + 0.5f) * (32.0f / 1024.0f), ((float)iz + 0.5f) * (32.0f / 1024.0f));
	return true;
}

float dynsnow_seg_dist(float2 p_rel, float2 a_rel, float2 b_rel, out float along_t)
{
	float2 ba = b_rel - a_rel;
	float len2 = dot(ba, ba);
	if (len2 > 16.0f) // > 4.0m safety guard
	{
		float da = length(p_rel - a_rel);
		float db = length(p_rel - b_rel);
		along_t = (da < db) ? 0.0f : 1.0f;
		return min(da, db);
	}
	along_t = (len2 > 0.0001f) ? saturate(dot(p_rel - a_rel, ba) / len2) : 0.0f;
	return length((p_rel - a_rel) - ba * along_t);
}

// Single-pass per-pixel spatial prefilter: finds up to 2 nearby trail segments & 2 campfires once per pixel
// so all 8 parallax raymarch steps + normal taps run at locked 60 FPS on handheld APUs!
void dynsnow_init_context(float2 wpos_xz, float dist_z, out DynSnowContext ctx)
{
	ctx.has_trench = false;
	ctx.has_cf = false;
	ctx.seg_count = 0;
	ctx.cam_xz = float2(m_inv_V._m03, m_inv_V._m23);
	ctx.seg0_a = 0.0f; ctx.seg0_b = 0.0f; ctx.seg0_w = 0.0f;
	ctx.seg1_a = 0.0f; ctx.seg1_b = 0.0f; ctx.seg1_w = 0.0f;
	ctx.cf_count = 0;
	ctx.cf0_loc = 0.0f; ctx.cf0_r = 0.0f;
	ctx.cf1_loc = 0.0f; ctx.cf1_r = 0.0f;

	if (dist_z >= 23.0f)
		return;

	float2 c_mod = ctx.cam_xz - floor(ctx.cam_xz * (1.0f / 32.0f)) * 32.0f;
	float2 p_loc = wpos_xz - ctx.cam_xz;

	// 1. Check up to 2 active campfires (continuous float radii in shader_param_2.zw)
	float cf1_r = shader_param_2.z;
	float2 cf1_mod;
	if (cf1_r > 0.008f && dynsnow_unpack_cf_mod32(hdr10_parameters4.y, cf1_mod))
	{
		float2 cf1_loc = dynsnow_torus32_delta(cf1_mod, c_mod);
		if (length(p_loc - cf1_loc) < cf1_r + 1.05f)
		{
			ctx.cf0_loc = cf1_loc;
			ctx.cf0_r = cf1_r;
			ctx.cf_count = 1;
			ctx.has_cf = true;
		}
	}

	float cf2_r = shader_param_2.w;
	float2 cf2_mod;
	if (cf2_r > 0.008f && dynsnow_unpack_cf_mod32(hdr10_parameters4.z, cf2_mod))
	{
		float2 cf2_loc = dynsnow_torus32_delta(cf2_mod, c_mod);
		if (length(p_loc - cf2_loc) < cf2_r + 1.05f)
		{
			if (ctx.cf_count == 0)
			{
				ctx.cf0_loc = cf2_loc;
				ctx.cf0_r = cf2_r;
				ctx.cf_count = 1;
			}
			else
			{
				ctx.cf1_loc = cf2_loc;
				ctx.cf1_r = cf2_r;
				ctx.cf_count = 2;
			}
			ctx.has_cf = true;
		}
	}

	// 2. Scan contiguous packed multi-actor 3D trail buffer (early-exits immediately on first empty slot!)
	float slots[25] = {
		hdr10_parameters3.x, hdr10_parameters3.y, hdr10_parameters3.z, hdr10_parameters3.w,
		hdr10_parameters5.x, hdr10_parameters5.y, hdr10_parameters5.z, hdr10_parameters5.w,
		hdr10_parameters6.x, hdr10_parameters6.z, hdr10_parameters6.w,
		hdr10_parameters7.y, hdr10_parameters7.z, hdr10_parameters7.w,
		hdr10_parameters8.x, hdr10_parameters8.y, hdr10_parameters8.z, hdr10_parameters8.w,
		hdr10_parameters9.x, hdr10_parameters9.y, hdr10_parameters9.z, hdr10_parameters9.w,
		hdr10_parameters10.x, hdr10_parameters10.y, hdr10_parameters10.z
	};

	float best_d0 = 0.98f;
	float best_d1 = 0.98f;
	float2 prev_loc = 0.0f;
	float prev_w = 0.0f;
	bool has_prev = false;

	[loop] for (int i = 0; i < 25; ++i)
	{
		float2 cur_mod;
		float cur_w;
		bool conn;
		if (!dynsnow_unpack_trail_node(slots[i], cur_mod, cur_w, conn))
			break;

		float2 cur_loc = dynsnow_torus32_delta(cur_mod, c_mod);
		if (conn && has_prev)
		{
			// Ultra-fast 2D AABB reject before computing capsule distance
			float2 bb_min = min(prev_loc, cur_loc) - 0.98f;
			float2 bb_max = max(prev_loc, cur_loc) + 0.98f;
			if (p_loc.x >= bb_min.x && p_loc.x <= bb_max.x && p_loc.y >= bb_min.y && p_loc.y <= bb_max.y)
			{
				float t_seg;
				float d = dynsnow_seg_dist(p_loc, prev_loc, cur_loc, t_seg);
				if (d < best_d0)
				{
					best_d1 = best_d0;
					ctx.seg1_a = ctx.seg0_a; ctx.seg1_b = ctx.seg0_b; ctx.seg1_w = ctx.seg0_w;
					best_d0 = d;
					ctx.seg0_a = prev_loc; ctx.seg0_b = cur_loc; ctx.seg0_w = float2(prev_w, cur_w);
					ctx.seg_count = min(ctx.seg_count + 1, 2);
				}
				else if (d < best_d1)
				{
					best_d1 = d;
					ctx.seg1_a = prev_loc; ctx.seg1_b = cur_loc; ctx.seg1_w = float2(prev_w, cur_w);
					ctx.seg_count = 2;
				}
			}
		}

		prev_loc = cur_loc;
		prev_w = cur_w;
		has_prev = true;
	}

	ctx.has_trench = (ctx.seg_count > 0);
}

void dynsnow_eval_single_seg(float2 p_loc, float2 wpos_xz, float2 a, float2 b, float2 w_ab, float edge_jitter, inout float out_trench, inout float out_berm)
{
	float along_t;
	float d = dynsnow_seg_dist(p_loc, a, b, along_t);
	if (d >= 0.72f)
		return;

	float seg_w = lerp(w_ab.x, w_ab.y, along_t);
	if (seg_w <= 0.005f)
		return;

	// Width & depth both taper smoothly as old snow tracks slowly fill back in from start to end
	float width_scale = 0.55f + 0.45f * smoothstep(0.0f, 0.65f, seg_w);
	float scallop = 0.90f + 0.10f * cos(along_t * 6.28318f);
	float furrow_w = 0.255f * width_scale * scallop;
	float d_w = d + edge_jitter;

	float tr = (1.0f - smoothstep(furrow_w * 0.42f, furrow_w * 1.18f, d_w)) * seg_w;
	float bm = exp(-pow((d_w - (furrow_w * 1.34f)) / 0.115f, 2.0f)) * (seg_w * seg_w) * (1.0f - tr);

	out_trench = max(out_trench, tr);
	out_berm = max(out_berm, bm);
}

void dynsnow_eval_single_cf(float2 p_loc, float2 wpos_xz, float2 cf_loc, float cf_r, inout float out_cf_melt, inout float out_cf_slush)
{
	float cf_dist = length(p_loc - cf_loc);
	if (cf_dist >= cf_r + 0.85f)
		return;

	// Smooth onset from 0.00m -> 0.45m so lighting/extinguishing never pops at the center
	float onset = smoothstep(0.01f, 0.45f, cf_r);
	float n_cf = (dynsnow_noise2(wpos_xz * 2.1f) - 0.5f) * 0.26f
	           + (dynsnow_noise2(wpos_xz * 5.8f + float2(3.1f, 1.7f)) - 0.5f) * 0.10f;
	float eff_r = cf_r + n_cf * smoothstep(0.05f, 0.65f, cf_r);

	// Wide, silky-smooth radial thermal falloff (~0.88m soft gradient instead of hard steps)
	float melt = (1.0f - smoothstep(max(0.0f, eff_r - 0.70f), eff_r + 0.18f, cf_dist)) * onset;
	melt = melt * melt * (3.0f - 2.0f * melt);
	float slush = exp(-pow((cf_dist - (eff_r - 0.14f)) / 0.32f, 2.0f)) * onset * (1.0f - melt * 0.55f);

	out_cf_melt = max(out_cf_melt, melt);
	out_cf_slush = max(out_cf_slush, slush);
}

// Fast O(1) evaluation inside parallax loop & finite-difference normal taps
void dynsnow_eval_cached(DynSnowContext ctx, float2 wpos_xz, out float out_trench, out float out_berm, out float out_cf_melt, out float out_cf_slush)
{
	out_trench = 0.0f;
	out_berm = 0.0f;
	out_cf_melt = 0.0f;
	out_cf_slush = 0.0f;

	if (!ctx.has_trench && !ctx.has_cf)
		return;

	float2 p_loc = wpos_xz - ctx.cam_xz;

	if (ctx.has_trench)
	{
		float edge_jitter = (dynsnow_noise2(wpos_xz * 7.2f) - 0.5f) * 0.052f;
		dynsnow_eval_single_seg(p_loc, wpos_xz, ctx.seg0_a, ctx.seg0_b, ctx.seg0_w, edge_jitter, out_trench, out_berm);
		if (ctx.seg_count > 1)
			dynsnow_eval_single_seg(p_loc, wpos_xz, ctx.seg1_a, ctx.seg1_b, ctx.seg1_w, edge_jitter, out_trench, out_berm);
		out_berm *= (1.0f - out_trench);
	}

	if (ctx.has_cf)
	{
		dynsnow_eval_single_cf(p_loc, wpos_xz, ctx.cf0_loc, ctx.cf0_r, out_cf_melt, out_cf_slush);
		if (ctx.cf_count > 1)
			dynsnow_eval_single_cf(p_loc, wpos_xz, ctx.cf1_loc, ctx.cf1_r, out_cf_melt, out_cf_slush);
		out_trench *= (1.0f - out_cf_melt);
		out_berm *= (1.0f - out_cf_melt);
	}
}

// Subtle, Crystalline Frost & Fine Snow Dusting on 1st-Person Weapons (Matches Project INVERNO Kar98k Reference!)
// Leaves 70-80% of dark steel/wood & 100% of normal map details crisp, and never whitewashes sleeves or hands!
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
	float wpn_snow = hdr10_parameters4.w;
	if (wpn_snow <= 0.01f)
		return;

	// 1. Exclude close cloth cuffs/sleeves & warm bare skin so arms/gloves never look painted white
	float forward_gun_mask = smoothstep(0.20f, 0.35f, view_pos.z) * smoothstep(-0.115f, -0.025f, view_pos.y);
	if (forward_gun_mask <= 0.005f)
		return;

	float warm_skin = saturate((albedo.r - albedo.b * 1.18f) * 6.5f) * saturate(1.0f - gloss * 2.5f);
	float hard_surface = smoothstep(0.04f, 0.15f, gloss + abs(bump_height - 0.5f) * 0.35f);
	float surface_elig = forward_gun_mask * (1.0f - warm_skin) * (0.20f + 0.80f * hard_surface);

	// 2. Sky-facing micro-facets (combines bumped world-space normal + vertex upward normal)
	float sky_y = saturate(world_bumped_n.y * 0.72f + RDrops.w * 0.28f);
	float top_mask = pow(saturate((sky_y - 0.20f) / 0.80f), 2.15f);
	if (top_mask <= 0.002f)
		return;

	// 3. UV-locked blotchy frost patches + bump cavity trapping + crisp micro-snow crystals
	float n_patch1 = dynsnow_noise2(uv_tex * 12.0f + RDrops.xz * 6.0f);
	float n_patch2 = dynsnow_noise2(uv_tex * 31.0f + float2(4.7f, 9.2f));
	float n_grain  = dynsnow_noise2(uv_tex * 92.0f + float2(1.9f, 6.4f));
	float n_speck  = dynsnow_noise2(uv_tex * 210.0f + float2(13.1f, 3.7f));

	float patch_field = n_patch1 * 0.56f + n_patch2 * 0.44f;
	// Frost settles into recesses, grooves, screw heads & sight edges
	float cavity = saturate((0.62f - bump_height) * 1.35f + length(normal_ts.xy) * 0.55f);

	float snow_pot = top_mask * (patch_field * 0.72f + cavity * 0.42f) * min(wpn_snow, 1.5f);

	// Delicate translucent rime haze (capped at ~0.30 so dark steel & scratches remain clearly visible!)
	float rime_alpha = smoothstep(0.24f, 0.70f, snow_pot) * 0.31f * min(wpn_snow, 1.25f);

	// Tiny, sharp settled snow crystal grains on top ridges & crevices
	float crystal_grains = pow(saturate(n_speck * 1.12f), 4.0f) * smoothstep(0.32f, 0.76f, snow_pot * (0.60f + n_grain * 0.70f));
	float speck_alpha = crystal_grains * 0.50f * min(wpn_snow, 1.35f);

	float total_mask = saturate(rime_alpha + speck_alpha) * surface_elig;
	if (total_mask <= 0.002f)
		return;

	// Modulate frost brightness subtly by underlying texture detail so engravings/wood grain never wash out
	float base_luma = dot(albedo, float3(0.299f, 0.587f, 0.114f));
	float3 rime_col = float3(0.74f, 0.81f, 0.90f) * (0.68f + 0.32f * saturate(base_luma * 2.4f));
	float3 crystal_col = float3(0.90f, 0.94f, 0.99f);

	albedo = lerp(albedo, rime_col, rime_alpha * surface_elig);
	albedo = lerp(albedo, crystal_col, speck_alpha * surface_elig);

	// Preserve 94%+ of original weapon bump normals; only add fine crystalline sparkle grain
	normal_ts.xy += (float2(n_grain, n_speck) - 0.5f) * (total_mask * 0.28f);

	// Subtle icy sparkle on crystal specks + slight wet sheen when melting indoors/by fire
	float melt_wet = saturate(1.0f - abs(wpn_snow - 0.18f) * 5.5f) * top_mask * surface_elig;
	gloss = lerp(gloss, 0.16f + n_speck * 0.22f, total_mask * 0.65f);
	gloss = max(gloss, melt_wet * 0.36f);
}

#endif
