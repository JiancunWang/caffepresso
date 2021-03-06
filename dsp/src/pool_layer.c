#include "pool_layer.h"
#include "struct_defs.h"
#include <float.h>
#include <stdio.h>
extern unsigned int core_id;

void unroll_maps(FIX_MAP *p_img, FIX_MAP *p_mat,
	const int n_ch, const int in_h, const int in_w,
	int win_size, int stride, int no_pad_cols, FIX_MAP pad_val) {

	int out_h, out_w, out_row, out_col, in_row, in_col, ch, kr, kc;
	out_h = (in_h - win_size) / stride + 1;
	out_w = (in_w - win_size) / stride + 1;

	for(ch = 0; ch < n_ch; ch++) {
		in_row = 0;
		for(out_row = 0; out_row < out_h; out_row++) {
			in_col = 0;
			for(out_col = 0; out_col < out_w; out_col++){
				for(kr = 0; kr < win_size; kr++) {
					for(kc = 0; kc < win_size; kc++) {
						*p_mat++ = p_img[(in_row + kr)*in_w + in_col + kc];
					}
				}
				// extra padding
				for(kc = 0; kc < no_pad_cols; kc++) {
					*p_mat++ = pad_val;
				}
				in_col += stride;
			}
			in_row += stride;
		}
	}
}

STATUS_E dsp_fix_pool_layer(FIX_MAP *p_input,	// pointer to input maps stored in flattened [maps][row][col] format.
	int in_height,			// input feature map height
	int in_width,			// input feature map width
	int no_inputs,			// number of input feature maps
	int start_map,			// the map offset to start from for this core.
	int win_size,			// kernel size. We support only square sized kernels
	int stride,				// convolution window stride in both horizontal and vertical direction.
	int pad,				// padding on all 4 sides of feature map
	POOL_TYPE_E pool_type,	// pooling type, max pooling or average pooling
	FIX_MAP *p_output		// pointer to output feature maps. Stored in [map][row][col] flattened manner.
	) {

	STATUS_E status = FAILED;
	int map, o_w, o_h, row, col, i, j;
	FIX_MAP max;
	int sum;


	o_h = (in_height + 2 * pad - win_size + 1 + stride - 1)/ stride;
	o_w = (in_width + 2 * pad - win_size + 1 + stride - 1)/ stride;

	switch(pool_type) {
		case MAX_POOL:
			for (map = start_map; map < start_map + no_inputs; map++) {
				for(row = 0; row < in_height + 2 * pad - win_size + 1; row += stride) {
					for(col = 0; col < in_width + 2 * pad  - win_size + 1; col += stride) {
						max = -32768; // TODO: define -ve min of FP_MAP_PIXEL type and use here.
						for ( i = 0; i < win_size; i++) {
							for ( j = 0; j < win_size; j++) {
								//max = MAX(max, p_input[map * in_width * in_height + (row + i) * in_width + col + j]);
								max = _max2(max, p_input[map * in_width * in_height + (row + i) * in_width + col + j]);
							}
						}
						p_output[map * o_w * o_h + o_w * (row / stride) + (col / stride)] = max;
					}
				}
			}

			break;
		case AVG_POOL:
			for (map = start_map; map < start_map + no_inputs; map++) {
				for(row = 0; row < in_height + 2 * pad  - win_size + 1; row += stride) {
					for(col = 0; col < in_width + 2 * pad  - win_size + 1; col += stride) {
						sum = 0;
						for ( i = 0; i < win_size; i++) {
							for ( j = 0; j < win_size; j++) {
								sum += p_input[map * in_width * in_height + (row + i) * in_width + col + j];
							}
						}
						p_output[map * o_w * o_h + o_w * (row / stride) + (col / stride)] =
							(FIX_MAP)(sum / (win_size * win_size));
					}
				}
			}			
			break;
		default:
			break;
	}
	return status;
}

STATUS_E dsp_flt_pool_layer(FLT_MAP *p_input,	// pointer to input maps stored in flattened [maps][row][col] format.
	int in_height,			// input feature map height
	int in_width,			// input feature map width
	int no_inputs,			// number of input feature maps
	int start_map,			// the map offset to start from for this core.
	int win_size,			// kernel size. We support only square sized kernels
	int stride,				// convolution window stride in both horizontal and vertical direction.
	int pad,				// padding on all 4 sides of feature map
	POOL_TYPE_E pool_type,	// pooling type, max pooling or average pooling
	FLT_MAP *p_output		// pointer to output feature maps. Stored in [map][row][col] flattened manner.
	) {
		
	int map, o_w, o_h, row, col, i, j;
	FLT_MAP max;
	int sum;


	o_h = (in_height + 2 * pad - win_size + 1 + stride - 1)/ stride;
	o_w = (in_width + 2 * pad - win_size + 1 + stride - 1)/ stride;
	STATUS_E status = FAILED;
	
	switch(pool_type) {
		case MAX_POOL:
			for (map = start_map; map < start_map + no_inputs; map++) {
				for(row = 0; row < in_height + 2 * pad - win_size + 1; row += stride) {
					for(col = 0; col < in_width + 2 * pad- win_size + 1; col += stride) {
						max = -FLT_MAX; // TODO: define -ve min of FP_MAP_PIXEL type and use here.
						for ( i = 0; i < win_size; i++) {
							for ( j = 0; j < win_size; j++) {
								max = MAX(max, p_input[map * in_width * in_height + (row + i) * in_width + col + j]);
							}
						}
						p_output[map * o_w * o_h + o_w * (row / stride) + (col / stride)] = max;
					}
				}
			}
			break;
		case AVG_POOL:
			for (map = start_map; map < start_map + no_inputs; map++) {
				for(row = 0; row < in_height + 2 * pad  - win_size + 1; row += stride) {
					for(col = 0; col < in_width + 2 * pad  - win_size + 1; col += stride) {
						sum = 0;
						for ( i = 0; i < win_size; i++) {
							for ( j = 0; j < win_size; j++) {
								sum += p_input[map * in_width * in_height + (row + i) * in_width + col + j];
							}
						}
						p_output[map * o_w * o_h + o_w * (row / stride) + (col / stride)] =
							(FLT_MAP)(sum / (win_size * win_size));
					}
				}
			}			
			break;
		default:
			break;
	}
	return status;
}

STATUS_E dsp_pool_layer(POOL_LYR_CTX_T *p_pool_ctx, FLT_MAP *p_flt_in_maps, FIX_MAP *p_fix_in_maps) {
	STATUS_E status = FAILED;

	if(p_pool_ctx->lyr_arith_mode == FIXED_POINT) {
		status = dsp_fix_pool_layer(p_fix_in_maps,
			p_pool_ctx->pool_info.map_w,
			p_pool_ctx->pool_info.map_h,
			p_pool_ctx->no_maps[core_id],
			p_pool_ctx->start_map[core_id],
			p_pool_ctx->pool_info.win_size,
			p_pool_ctx->pool_info.stride,
			p_pool_ctx->pool_info.pad,
			p_pool_ctx->pool_info.pool_type,
			p_pool_ctx->p_fix_output
			);
	} else {
		status = dsp_flt_pool_layer(p_flt_in_maps,
			p_pool_ctx->pool_info.map_w,
			p_pool_ctx->pool_info.map_h,
			p_pool_ctx->no_maps[core_id],
			p_pool_ctx->start_map[core_id],
			p_pool_ctx->pool_info.win_size,
			p_pool_ctx->pool_info.stride,
			p_pool_ctx->pool_info.pad,
			p_pool_ctx->pool_info.pool_type,
			p_pool_ctx->p_flt_output
			);
	}

	return status;
}
