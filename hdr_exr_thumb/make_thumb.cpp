#include <iostream>
#include <algorithm>
#include <chrono>
//#include <omp.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

//#define TINYEXR_USE_MINIZ 0
//#include <zlib.h>
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

using namespace std;

float gammaCorrection(float& value) {
	return pow(value, 1 / 2.2);
}

char toChar(float value) {
	value = gammaCorrection(value);
	return min(max(int(value * 255), 0), 255);
}

char* toLDR(float* hdr_image, int width, int height, int channels, int &ldr_width, int &ldr_height) {
	int reduction;
	reduction = max(width / 1280, 1);
	ldr_width = width / reduction;
	ldr_height = height / reduction;

	// メモリ確保
	size_t size = ldr_width * ldr_height * channels;
	char* ldr_image;
	try {
		ldr_image = new char[size];
	}
	catch (bad_alloc) {
		cerr << "this file is too big..." << endl;
		getchar();
		return 0;
	}

	// 変換
	//#pragma omp parallel for
	for (int h = 0; h < ldr_height; h++) {
		for (int w = 0; w < ldr_width; w++) {
			int big_idx = ((h * reduction) * width + (w * reduction)) * channels;
			int small_idx = (h * ldr_width + w) * channels;

			for (int c = 0; c < channels; c++) {
				ldr_image[small_idx + c] = toChar(hdr_image[big_idx + c]);
			}
		}
	}

	return ldr_image;
}

string getExtension(string filename) {
	return filename.substr(filename.find_last_of(".") + 1);
}

bool isHDR(string filename) {
	return getExtension(filename) == "hdr";
}

bool isEXR(string filename) {
	return getExtension(filename) == "exr";
}

void convertFilename(char* filename, int size) {
	filename[size - 3] = 'j';
	filename[size - 2] = 'p';
	filename[size - 1] = 'g';
}



int main(int argc, char* argv[]) {
	auto start = std::chrono::system_clock::now(); // 計測開始時間

	

	for (int i = 1; i < argc; i++) {
		int width, height, channels;

		string filename = argv[i];
		cout << filename << endl;
		//if (!isHDR(filename)) {
		//	cout << "not .hdr file" << endl;
		//	continue;
		//}

		char* ldr_image;
		int ldr_width, ldr_height;

		if (isHDR(filename)) {
			// 読み込み
			float* hdr_image = stbi_loadf(argv[i], &width, &height, &channels, 0);

			// jpg変換
			ldr_image = toLDR(hdr_image, width, height, channels, ldr_width, ldr_height);
			stbi_image_free(hdr_image);
		}
		else if (isEXR(filename)) {
			// 読み込み
			float* exr_image;
			const char* err = nullptr;
			channels = 4;

			int ret = LoadEXR(&exr_image, &width, &height, argv[i], &err);
			if (ret != TINYEXR_SUCCESS) {
				if (err) {
					fprintf(stderr, "ERR : %s\n", err);
					FreeEXRErrorMessage(err);
					getchar();
				}
				continue;
			}

			// jpg変換
			ldr_image = toLDR(exr_image, width, height, channels, ldr_width, ldr_height);
			free(exr_image);
		}
		else {
			cout << "this file is NOT .hdr or .exr file." << endl;
			continue;
		}

		// ファイル名変換
		convertFilename(argv[i], filename.size());

		// 書き出し
		stbi_write_jpg(argv[i], ldr_width, ldr_height, channels, ldr_image, 100);

		// メモリ解放
		delete[] ldr_image;
	}

	auto end = chrono::system_clock::now();  // 計測終了時間
	double elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count(); //処理に要した時間をミリ秒に変換
	cout << "elapsed time: " << elapsed << " ms" << endl;
	
}