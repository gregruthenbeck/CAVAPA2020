#define WIN32_LEAN_AND_MEAN
#include <windows.h> // handles for receiving images from clipboard... not needed but part of FreeImage
#include <fstream>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <FreeImage.h>
#include <../Wrapper/FreeImagePlus/FreeImagePlus.h>

using namespace std;
using namespace boost::filesystem;
namespace po = boost::program_options;

//void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
//	if (fif != FIF_UNKNOWN) {
//		printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
//	}
//	else {
//		cout << "FreeImage error: \"Unknown image format\"" << endl;
//	}
//}
//
//class FreeImageWrapper {
//public:
//	FreeImageWrapper() {
//		FreeImage_Initialise(); // Call once
//		FreeImage_SetOutputMessage(FreeImageErrorHandler);
//	}
//	~FreeImageWrapper() {
//		FreeImage_DeInitialise(); // Call once
//	}
//};


int main(int ac, char** av) {
	//FreeImageWrapper fiw;
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("frames,f", po::value<string>(), "folder containing JPG files of video frames")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
		return EXIT_SUCCESS;
	}

	string inFolder = "";
	if (vm.count("frames")) {
		inFolder = vm["frames"].as<string>();
		cout << "Frames images folder " << inFolder << "." << endl;
	}
	else {
		cout << "Frames images input folder was not set." << endl;
		return EXIT_FAILURE;
	}

	path inFolderPath(inFolder.c_str());

	try {
		if (exists(inFolderPath)) {
			//if (is_regular_file(inFolderPath))
			//	cout << p << " size is " << file_size(p) << '\n';

			if (is_directory(inFolderPath)) {
				cout << inFolderPath << " is a directory containing:" << endl;
				for (directory_entry& x : directory_iterator(inFolderPath)) {
					if (x.path().extension() != "jpg") // must be jpg
						continue;

					try {
						fipImage img;
						if (!img.load((char*)x.path().c_str()))
							cout << "Error. Failed to load image: \"" << x.path() << "\"" << endl;
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