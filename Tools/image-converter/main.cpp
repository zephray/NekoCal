#include <opencv2/opencv.hpp>
#include <string>
#include <stdint.h>

using namespace std;
using namespace cv;

class config {
public:
	int target_witdh = 800;
	int target_height = 520;
	string srcFile;
	string destFile;
};

config cfg;

static inline uchar fix(int a, int b) {
	if (a + b < 0) return 0;
	if (a + b > 255) return 255;
	return a + b;
}

inline int find_closest_palette_color(int oldPixel) {
	return ((oldPixel & 0xF0) | (oldPixel >> 4));
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		cfg.srcFile = "C:\\Users\\ntzyz\\Desktop\\4b3255.jpg";
	}
	else {
		cfg.srcFile = argv[1];
	}
	Mat src = imread(cfg.srcFile);
	
	double target_scale = static_cast<double>(cfg.target_witdh) / cfg.target_height;
	double src_scale = static_cast<double>(src.size().width) / src.size().height;

	Mat srcRoi;

	if (src_scale > target_scale) {
		int temp_height = src.size().height;
		int temp_width = temp_height * target_scale;
		srcRoi = src(Rect((src.size().width - temp_width) / 2, 0, temp_width, temp_height));
	}
	else {
		int temp_width = src.size().width;
		int temp_height = temp_width / target_scale;
		srcRoi = src(Rect(0, (src.size().height - temp_height) / 2, temp_width, temp_height));
	}

	Mat resied, gray;
	resize(srcRoi, resied, Size(cfg.target_witdh, cfg.target_height));
	cvtColor(resied, gray, CV_BGR2GRAY);
	
	for (int y = 0; y != gray.rows - 1; ++y) {
		for (int x = 1; x != gray.cols - 1; ++x) {
			int oldPixel = gray.at<uchar>(y, x);
			int newPixel = find_closest_palette_color(oldPixel) & 0xff;
			int quantError = oldPixel - newPixel;

			gray.at<uchar>(y, x) = newPixel;
			gray.at<uchar>(y, x + 1) = fix(gray.at<uchar>(y, x + 1), quantError * 7 / 16);
			gray.at<uchar>(y + 1, x - 1) = fix(gray.at<uchar>(y + 1, x - 1), quantError * 3 / 16);
			gray.at<uchar>(y + 1, x) = fix(gray.at<uchar>(y + 1, x), quantError * 5 / 16);
			gray.at<uchar>(y + 1, x + 1) = fix(gray.at<uchar>(y + 1, x + 1), quantError * 1 / 16);
		}
	}
	
	uint8_t *bitmap = new uint8_t[cfg.target_height * cfg.target_witdh / 2];

	for (int y = 0; y != gray.rows; ++y) {
		for (int x = 0; x != gray.cols; x += 2) {
			uint8_t pixel = ((gray.at<uchar>(y, x) & 0xF0 ) | (gray.at<uchar>(y, x + 1) >> 4) );
			bitmap[(y * gray.cols + x) / 2] = pixel;
		}
	}

	cfg.destFile = string(cfg.srcFile) + ".ebm";
	FILE *fp = fopen(cfg.destFile.c_str(), "wb+");
	fwrite(bitmap, cfg.target_height * cfg.target_witdh / 2, 1, fp);
	fclose(fp);
	/*
	namedWindow("image", CV_WINDOW_AUTOSIZE);
	imshow("image", gray);
	*/
	waitKey();

	return EXIT_SUCCESS;
}

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

