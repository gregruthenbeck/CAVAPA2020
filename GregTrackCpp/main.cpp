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
#include <ppltasks.h>
#include <concurrent_vector.h>
#include <tuple>

using namespace std;
using namespace boost::filesystem;
namespace po = boost::program_options;
typedef fipImage Image;

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

// Calls the provided work function and returns the number of milliseconds 
// that it takes to call that function.
template <class Function>
__int64 time_call(Function&& f) {
	__int64 begin = GetTickCount();
	f();
	return GetTickCount() - begin;
}


concurrency::task<tuple<int, path, Image> > CreateForegroundMaskAsync(int index, const path& imgFilepath, const Image& imgBG, const float bgThreshSq, const int blurIterations) {
	return concurrency::create_task([index, imgFilepath, imgBG, bgThreshSq, blurIterations] {
		Image img;
		try {
			if (!img.load((char*)imgFilepath.string().c_str()))
				throw(exception((stringstream("Error. Failed to load image: ") << imgFilepath).str().c_str()));

			const unsigned width = FreeImage_GetWidth(img);
			const unsigned height = FreeImage_GetHeight(img);
			const int bpp = FreeImage_GetBPP(img) / 8;
			Image imgDelta(FIT_BITMAP, width, height, 8);
			const BYTE* srcA = img.accessPixels();
			const BYTE* srcB = imgBG.accessPixels();
			BYTE* dst = imgDelta.accessPixels();
			for (unsigned yi = 0; yi < height; yi++) {
				for (unsigned xi = 0; xi < width; xi++, srcA += bpp, srcB += bpp, ++dst) {
					int r = ((int) * (srcA + 0)) - ((int) * (srcB + 0));
					int g = ((int) * (srcA + 1)) - ((int) * (srcB + 1));
					int b = ((int) * (srcA + 2)) - ((int) * (srcB + 2));
					float deltaSq = (float)(r * r + g * g + b * b);
					//BYTE c = (deltaSq > bgThreshSq) ? 255 : 0;
					BYTE c = (BYTE)min(255.0f, 255.0f*((deltaSq-bgThreshSq) / bgThreshSq));
					//BYTE deltaByte = (BYTE)min(255.0f, delta);
					*dst = c;
					//*(dst + 1) = c;
					//*(dst + 2) = c;
				}
			}

			// Blur
			for (int i = 0; i < blurIterations; ++i) {
				imgDelta.rescale(width / 4, height / 4, FREE_IMAGE_FILTER::FILTER_BILINEAR);
				imgDelta.rescale(width, height, FREE_IMAGE_FILTER::FILTER_BICUBIC);
			}
			return make_tuple(index, imgFilepath, imgDelta);
		}
		catch (std::exception const& e) {
			std::cerr << e.what() << std::endl;
		}
		return make_tuple(-1, imgFilepath, img);
		});
}

concurrency::task<tuple<int, double > > CreateAndSaveDeltaAsync(const int index, bool skipSave, const path& outFilepath, 
																const Image& imgA, const Image& imgB, 
																const path& dbgOutFilepath, const int dbgOutInterval) {
	return concurrency::create_task([index, skipSave, outFilepath, imgA, imgB, dbgOutFilepath, dbgOutInterval] {
		if (!imgA.isValid() || !imgB.isValid()) {
			return make_tuple(-1,0.0);
		}

		try {
			double pixCount = 0.0;
			const unsigned width = FreeImage_GetWidth((Image)imgA);
			const unsigned height = FreeImage_GetHeight((Image)imgA);
			const int bpp = FreeImage_GetBPP((Image)imgA) / 8;
			Image imgDD(FIT_BITMAP, width, height, 8);
			const BYTE* srcA = imgA.accessPixels();
			const BYTE* srcB = imgB.accessPixels();
			BYTE* dst = imgDD.accessPixels();
			for (unsigned yi = 0; yi < height; ++yi) {
				for (unsigned xi = 0; xi < width; ++xi, ++srcA, ++srcB, ++dst) {
					const auto delta = abs((int)* srcA - (int)* srcB);
					*dst = min(255, 2 * (BYTE)delta);
					pixCount += (double)delta;
				}
			}

			if (!skipSave) {
				if (!imgDD.isValid())
					return make_tuple(-1, 0.0);

				if (!imgDD.save(outFilepath.string().c_str()))
					throw(exception((stringstream("Error. Failed to save image: ") << outFilepath).str().c_str()));
			}

			if ((dbgOutInterval == 0 || (index % dbgOutInterval == 0)) &&
				exists(dbgOutFilepath.parent_path()) && is_directory(dbgOutFilepath.parent_path())) {
				if (!imgDD.isValid())
					return make_tuple(-1, 0.0);

				path imgADbgFilepath = dbgOutFilepath.parent_path().append(outFilepath.filename().string());

				if (!imgA.save(imgADbgFilepath.string().c_str()))
					throw(exception((stringstream("Error. Failed to save image: ") << imgADbgFilepath).str().c_str()));

				if (!imgDD.save(dbgOutFilepath.string().c_str()))
					throw(exception((stringstream("Error. Failed to save image: ") << dbgOutFilepath).str().c_str()));
			}

			return make_tuple(index, pixCount);
		}
		catch (std::exception const& e) {
			std::cerr << e.what() << std::endl;
		}
		return make_tuple(-1, 0.0);
	});
}


int main(int ac, char** av) {
	FreeImageWrapper fiw; // init and uninit of FreeImage
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("inputFolder,i", po::value<string>(), "folder containing JPG files of video frames")
		("outputFolder,o", po::value<string>(), "output folder (optional)")
		("debugFolder,t", po::value<string>(), "debug output folder (optional)")
		("dbgOutInterval,u", po::value<int>()->default_value(0), "number of frames skipped between debug output")
		("bgThreshold,g", po::value<float>()->default_value(48.0f), "used to identify tracked pixels, lower values will be noisier")
		("parallelChunkSize,p", po::value<int>()->default_value(128), "the number of files processed in parallel")
		("blurIterations,b", po::value<int>()->default_value(0), "number of times the blur is applied")
		("csvDataFilenameOut,d", po::value<string>()->default_value("data.csv"), "output CSV data filename (path)")
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

	string outFolder = "", dbgOutFolder = "";
	bool skipImgOutput = false;
	path outFolderPath, dbgOutFolderPath;
	if (vm.count("outputFolder") || vm.count("outputFolder")) {
		if (vm.count("outputFolder")) {
			outFolder = vm["outputFolder"].as<string>();
			cout << "Output images folder " << outFolder << "." << endl;
			outFolderPath = outFolder.c_str();
			if (!exists(outFolderPath) || !is_directory(outFolderPath)) {
				cout << "Output folder must exist. Folder=" << outFolderPath << endl;
				return EXIT_FAILURE;
			}
		}
		if (vm.count("debugFolder")) {
			dbgOutFolder = vm["debugFolder"].as<string>();
			cout << "Debug output images folder " << dbgOutFolder << "." << endl;
			dbgOutFolderPath = dbgOutFolder.c_str();
			if (!exists(dbgOutFolderPath) || !is_directory(dbgOutFolderPath)) {
				cout << "Debug output folder must exist. Folder=" << dbgOutFolderPath << endl;
				return EXIT_FAILURE;
			}
		}
	}
	else {
		skipImgOutput = true;
		cout << "Output images folder not set. Skipping image output." << endl;
	}

	const float bgThreshold = vm["bgThreshold"].as<float>();
	const int chunkSize = vm["parallelChunkSize"].as<int>();
	const int blurIterations = vm["blurIterations"].as<int>();
	const int dbgOutInterval = vm["dbgOutInterval"].as<int>();
	const string outCSVFilepath = vm["csvDataFilenameOut"].as<string>();

	if (exists(outCSVFilepath)) { // delete the CSV if it exists
		remove(outCSVFilepath);
	}

	path inFolderPath(inFolder.c_str());

	try {
		// Clear the output folder (if we are not skipping the image output)
		if (!skipImgOutput && exists(outFolderPath) && is_directory(outFolderPath)) {
			for (directory_entry& x : directory_iterator(outFolderPath)) {
				remove(x.path());
			}
		}
		if (!skipImgOutput && exists(dbgOutFolderPath) && is_directory(dbgOutFolderPath)) {
			for (directory_entry& x : directory_iterator(dbgOutFolderPath)) {
				remove(x.path());
			}
		}

		const float bgThreshSq = bgThreshold * bgThreshold;

		if (exists(inFolderPath)) {
			if (is_directory(inFolderPath)) {
				path lastFramePath;

				int numFiles = 0;
				for (directory_entry& x : directory_iterator(inFolderPath)) {
					if (x.path().extension() != ".jpg") {// must be jpg
						continue;
					}
					++numFiles;
					lastFramePath = x.path();
				}

				Image imgBG;
				if (!imgBG.load((char*)lastFramePath.string().c_str()))
					throw(exception((stringstream("Error. Failed to load image: ") << lastFramePath).str().c_str()));

				int fileCount = 0;
				__int64 elapsedMillis = 0L;
				tuple<int, path, Image> seam;
				vector<concurrency::task<tuple<int, path, Image> > > foregroundImageTasks;
				vector<tuple<int, double> > movementPixelTotals;
				for (directory_entry& x : directory_iterator(inFolderPath)) {
					++fileCount;
					elapsedMillis += time_call([&] {
						if (x.path().extension() != ".jpg") // must be jpg
							return;

						foregroundImageTasks.push_back(CreateForegroundMaskAsync(fileCount - 1, x.path(), imgBG, bgThreshSq, blurIterations/*, dbgOutFilepath, dbgOutInterval*/));

						if ((fileCount % chunkSize) == 0 || fileCount == numFiles) { // be careful with remainder in last chunk
							vector<tuple<int, path, Image> > foregrounds;
							for (auto& t : foregroundImageTasks) {
								foregrounds.push_back(t.get());
							}
							foregroundImageTasks.clear();
							sort(begin(foregrounds), end(foregrounds), [](auto const& t1, auto const& t2) {
								return get<0>(t1) < get<0>(t2); // sort based on the index (first element in the tuple)
								});
							vector<concurrency::task<tuple<int,double > > > deltaImageTasks;
							Image prevImg = get<2>(seam);

							for (auto& t : foregrounds) {
								path inPath = get<1>(t);
								string outFilename = inPath.filename().string();
								path outFilepath = outFolderPath;
								outFilepath.append(outFilename);
								path dbgOutFilepath = dbgOutFolderPath;
								dbgOutFilepath.append(inPath.stem().filename().string());
								dbgOutFilepath += "-dbg.jpg";
								Image img = get<2>(t);
								deltaImageTasks.push_back(CreateAndSaveDeltaAsync(get<0>(t) /*fileCount - 1*/, skipImgOutput, outFilepath, prevImg, img, dbgOutFilepath, dbgOutInterval));
								prevImg = img;
							}
							for (auto& t : deltaImageTasks) {
								movementPixelTotals.push_back(t.get());
							}
							seam = foregrounds[foregrounds.size() - 1];
						}
					});
					cout << "\r" << (int)(100.0 * (double)fileCount / (double)(numFiles-1)) << "% done. " << 
						"Processing frame " << fileCount << "/" << numFiles << ". " <<
						(int)((double)elapsedMillis / (double)fileCount) << "ms per frame.";
					fflush(stdout);
				}
				cout << endl;
				
				sort(begin(movementPixelTotals), end(movementPixelTotals), [](auto const& t1, auto const& t2) {
					return get<0>(t1) < get<0>(t2); // sort based on the index (first element in the tuple)
					});

				std::ofstream csvFile(outCSVFilepath.c_str(), std::ofstream::out);
				if (csvFile.good()) {
					for (auto& d : movementPixelTotals) {
						csvFile << get<1>(d) << "," << endl;
					}
					csvFile.flush();
					csvFile.close();
					cout << "Data written to CSV file " << canonical(path(outCSVFilepath)) << endl;
				}
				else {
					cout << "Error attempting to open output file. Filepath=\"" << outCSVFilepath << "\"" << endl;
				}
			}
		}
		else {
			cout << inFolderPath << " does not exist." << endl;
			return EXIT_FAILURE;
		}
	}
	catch (const filesystem_error& ex) {
		cout << "filesystem_error exception thrown: " << ex.what() << endl;
	}
	cout << "done" << endl;
}