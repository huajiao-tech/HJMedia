#include "HJDetectUtils.h"

NS_HJ_BEGIN

void HJFaceDetectUtils::rectangle(void* data_rgba, int image_height, int image_width, int x0, int y0, int x1, int y1, float scale_x, float scale_y)
{
	HJRGBAStruct* image_rgba = (HJRGBAStruct*)data_rgba;

	int x_min = std::min(x0, x1) * scale_x;
	int x_max = std::max(x0, x1) * scale_x;
	int y_min = std::min(y0, y1) * scale_y;
	int y_max = std::max(y0, y1) * scale_y;

	x_min = std::min(std::max(x_min, 0), image_width - 1);
	x_max = std::min(std::max(x_max, 0), image_width - 1);
	y_min = std::min(std::max(y_min, 0), image_height - 1);
	y_max = std::min(std::max(y_max, 0), image_height - 1);

	// top bottom
	if (x_max > x_min) {
		for (int x = x_min; x <= x_max; x++) {
			int offset = y_min * image_width + x;
			image_rgba[offset] = { 0, 255, 0, 0 };
			image_rgba[offset + image_width] = { 0, 255, 0, 0 };

			offset = y_max * image_width + x;
			image_rgba[offset] = { 0, 255, 0, 0 };
			if (offset >= image_width) {
				image_rgba[offset - image_width] = { 0, 255, 0, 0 };
			}
		}
	}

	// left right
	if (y_max > y_min) {
		for (int y = y_min; y <= y_max; y++) {
			int offset = y * image_width + x_min;
			image_rgba[offset] = { 0, 255, 0, 0 };
			image_rgba[offset + 1] = { 0, 255, 0, 0 };

			offset = y * image_width + x_max;
			image_rgba[offset] = { 0, 255, 0, 0 };
			if (offset >= 1) {
				image_rgba[offset - 1] = { 0, 255, 0, 0 };
			}
		}
	}
}

NS_HJ_END