#define WIN32_LEAN_AND_MEAN
#include <windows.h> // handles for receiving images from clipboard... not needed but part of FreeImage
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <FreeImage.h>
#include <../Wrapper/FreeImagePlus/FreeImagePlus.h>

using namespace std;
using namespace boost::filesystem;
namespace po = boost::program_options;

void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
	if (fif != FIF_UNKNOWN) {
		printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
	}
	else {
		cout << "FreeImage error: \"Unknown image format\"" << endl;
	}
}

class FreeImageWrapper {
public:
	FreeImageWrapper() {
		FreeImage_Initialise(); // Call once
		FreeImage_SetOutputMessage(FreeImageErrorHandler);
	}
	~FreeImageWrapper() {
 		FreeImage_DeInitialise(); // Call once
	}
};

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress(double percentage) {
	int val = (int)(percentage * 100);
	int lpad = (int)(percentage * PBWIDTH);
	int rpad = PBWIDTH - lpad;
	printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
	fflush(stdout);
}


int main(int ac, char** av) {
	FreeImageWrapper fiw;
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("inputFolder,i", po::value<string>(), "folder containing JPG files of video frames")
		("outputFolder,o", po::value<string>(), "output folder")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
		return EXIT_SUCCESS;
	}

	string inFolder = "";
	if (vm.count("inputFolder")) {
		inFolder = vm["inputFolder"].as<string>();
		cout << "Input images folder " << inFolder << "." << endl;
	}
	else {
		cout << "Input folder must be set." << endl;
		return EXIT_FAILURE;
	}

	string outFolder = "";
	if (vm.count("outputFolder")) {
		outFolder = vm["outputFolder"].as<string>();
		cout << "Output images folder " << inFolder << "." << endl;
	}
	else {
		cout << "Output folder must be set." << endl;
		return EXIT_FAILURE;
	}

	path inFolderPath(inFolder.c_str());
	path outFolderPath(outFolder.c_str());

	if (!exists(outFolderPath) || !is_directory(outFolderPath)) {
		cout << "Output folder must exist. Folder=" << outFolderPath << endl;
		return EXIT_FAILURE;
	}

	try {
		// Clear the output folder
		for (directory_entry& x : directory_iterator(outFolderPath)) {
			remove(x.path());
		}

		fipImage prevDelta;

		if (exists(inFolderPath)) {
			if (is_directory(inFolderPath)) {
				cout << inFolderPath << " is a directory containing:" << endl;
				path lastFramePath;

				int numFiles = 0;
				for (directory_entry& x : directory_iterator(inFolderPath)) {
					if (x.path().extension() != ".jpg") {// must be jpg
						++numFiles;
						continue;
					}
					lastFramePath = x.path();
				}

				fipImage imgBG;
				if (!imgBG.load((char*)lastFramePath.string().c_str()))
					throw(exception((stringstream("Error. Failed to load image: ") << lastFramePath).str().c_str()));

				int fileCount = 0;
				for (directory_entry& x : directory_iterator(inFolderPath)) {
					++fileCount;
					//printProgress((double)fileCount / (double)numFiles);
					if (x.path().extension() != ".jpg") // must be jpg
						continue;

					try {
						fipImage img;
						if (!img.load((char*)x.path().string().c_str()))
							throw(exception((stringstream("Error. Failed to load image: ") << x.path()).str().c_str()));

						const unsigned width = FreeImage_GetWidth(img);
						const unsigned height = FreeImage_GetHeight(img);
						const int bpp = FreeImage_GetBPP(img)/8;
						fipImage imgDelta(FIT_BITMAP, width, height, 8);
						const BYTE* srcA = img.accessPixels();
						const BYTE* srcB = imgBG.accessPixels();
						BYTE* dst = imgDelta.accessPixels();
						for (unsigned yi = 0; yi < height; yi++) {
							for (unsigned xi = 0; xi < width; xi++, srcA+=bpp,srcB+=bpp,++dst) {
								int r = ((int) * (srcA + 0)) - ((int) * (srcB + 0));
								int g = ((int) * (srcA + 1)) - ((int) * (srcB + 1));
								int b = ((int) * (srcA + 2)) - ((int) * (srcB + 2));
								float delta = sqrtf((float)(r * r + g * g + b * b));
								BYTE c = (delta > 48.0f) ? 255 : 0;
								//BYTE deltaByte = (BYTE)min(255.0f, delta);
								*dst = c;
								//*(dst + 1) = c;
								//*(dst + 2) = c;
							}
						}

						// Blur
						for (int i = 0; i < 3; ++i) {
							imgDelta.rescale(width / 4, height / 4, FREE_IMAGE_FILTER::FILTER_BILINEAR);
							imgDelta.rescale(width, height, FREE_IMAGE_FILTER::FILTER_BICUBIC);
						}

						// DD is Delta of the Deltas
						fipImage imgDD(FIT_BITMAP, width, height, 8);
						if (prevDelta.isValid()) {
							const BYTE* srcA = imgDelta.accessPixels();
							const BYTE* srcB = prevDelta.accessPixels();
							BYTE* dst = imgDD.accessPixels();
							for (unsigned yi = 0; yi < height; yi++) {
								for (unsigned xi = 0; xi < width; xi++, ++srcA, ++srcB, ++dst) {
									*dst = min(255, 2 * (BYTE)abs((int)* srcA - (int)* srcB));
								}
							}
						}
						prevDelta = imgDelta;

						if (!imgDD.isValid())
							continue;

						string outFilename = x.path().filename().string();
						path outFilepath = outFolderPath;
						outFilepath.append(outFilename);
						if (!imgDD.save(outFilepath.string().c_str()))
							throw(exception((stringstream("Error. Failed to save image: ") << outFilepath).str().c_str()));
					}
					catch (std::exception const& e) {
						std::cerr << e.what() << std::endl;
					}
				}
			}
		}
		else {
			cout << inFolderPath << " does not exist." << endl;
			return EXIT_FAILURE;
		}
	}
	catch (const filesystem_error& ex) {
		cout << ex.what() << '\n';
	}
}