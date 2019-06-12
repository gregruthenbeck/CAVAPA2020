#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <deque>

using namespace std;

template<typename T>
struct Vec3 {
	T x, y, z;
};

typedef Vec3<unsigned short> AccelData;

template <typename T>
double Length(const Vec3<T>& v) {
	return sqrt((double)v.x * (double)v.x + 
				(double)v.y * (double)v.y + 
		        (double)v.z * (double)v.z);
}

template <typename T>
class WindowAverager {
public:
	void SetWindowLength(const int len) {
		data.resize(len, 0);
		oneOnLen = 1.0 / (double)len;
	}
	double AddGetAverage(const T& d) {
		data.pop_front();
		data.push_back(d);
		double total = 0.0;
		for (const auto& d : data) {
			total += (double)d;
		}
		return total * oneOnLen;
	}
	deque<T> data;
	double oneOnLen;
};


int main() {
	const vector<string> inFilepaths{ "acc (1).dat", "acc (2).dat", "acc (3).dat", "acc (4).dat", "acc (5).dat", "acc (6).dat",	"acc (7).dat", "acc (8).dat" };
	const string outFilepath = "acc-all.csv";
	const int averagingWindowLen = 100;

	vector<ifstream> inFiles;
	for (auto& fpath : inFilepaths) {
		inFiles.push_back(ifstream(fpath.c_str(), ios::in | ios::binary));
		if (!inFiles.back().good()) {
			cout << "Error opening input file. Filepath=\"" << fpath << "\"" << endl;
			return -1;
		}
	}
	ofstream outFile(outFilepath.c_str(), ios::out);
	if (!outFile.good()) {
		cout << "Error opening output file. Filepath=\"" << outFilepath << "\"" << endl;
		return -1;
	}

	vector<vector<double> > filesData;

	int fileCount = 0;
	for (auto& inFile : inFiles) {
		cout << "\r" << "Processing input file " << ++fileCount << "/" << inFiles.size() << ". Filepath=\"" << inFilepaths[fileCount-1] << "\"";
		// Data alignment (file header length can result in bad reads of 2-byte data chunks)
		char junk;
		inFile.read(&junk, 1);

		WindowAverager<double> windower;
		windower.SetWindowLength(averagingWindowLen);
		AccelData data;
		vector<double> fileData;
		/*
		There are 16,419,847 samples in the accelerometer file. At 100Hz, this represents
		16419847/100/60/60 = 45.6107 hrs. The file creation time (acc.dat) is ‎Monday, ‎27 ‎May ‎2019, ‏‎5:33:00 PM.
		The activities of interest occurred on Sunday afternoon from about 2:30pm onwards.
		That’s 27hrs33mins earlier than the file creation time. 45.6 - 27.5 = 18.1hrs from the
		start of the recordings is the start of the interesting data. 18.1 * 100*60*60=6.516E6 samples
		can be skipped at the beginning of hr.dat, and not more than 2hrs of the subsequent data is of
		interest (2*100*60*60=720,000 samples for the 2hrs at 100Hz).
		*/
		unsigned long start = 6.516E6L;// -(30 * 60 * 100); // start 30mins earlier (30*60*100)
		unsigned long duration = 720000;
		unsigned long count = 0L;
		unsigned long writtenCount = 0L;
		unsigned saveInterval = 100; // write every 'n' samples (skip the rest)
		while (!inFile.eof() && inFile.good()) {
			try {
				inFile.read((char*)& data, sizeof(AccelData));
				++count;
				if (count < start) {
					continue;
				}
				++writtenCount;
				if (writtenCount > duration) {
					break;
				}
				const double len = Length(data);
				const double ave = windower.AddGetAverage(len);
				if (writtenCount % saveInterval == 0) {
					fileData.push_back(ave - len);
				}
			}
			catch (...) {
			}
		}
		filesData.push_back(fileData);
	}
	cout << endl;
	cout << "Writing CSV file to \"" << outFilepath << "\"" << endl;
	size_t minSize = numeric_limits<size_t>::max();
	for (auto& d : filesData) {
		minSize = min(minSize, d.size());
	}
	const char sep = ',';
	for (size_t i = 0; i < minSize; ++i) {
		outFile << i << sep << ((double)i / 86400.0) << sep; // second column is helper for formatting seconds as time in excel
		for (auto& d : filesData) {
			outFile << d[i] << sep;
		}
		outFile << endl;
	}
	outFile.flush();
	outFile.close();
	cout << "done" << endl;
}