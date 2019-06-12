#include <atltime.h>
#undef min
#undef max
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <deque>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;
using namespace boost::date_time;

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

struct YYMMDDHHMMSS {
	int year, month, day, hour, min, sec;
};

int main() {
	const string inFolder = "../data_in";
	const string inFileExt = ".dat";
	const vector<string> inFilenames{ "acc1", "acc2", "acc3", "acc4", "acc5", "acc6", "acc7", "acc8" };
	const string outFilepath = "../data_out/acc-all.csv";
	const int averagingWindowLen = 100;

	/*
	ls -la --time-style=full-iso
total 740392
drwxrwxrwx 1 gruthen gruthen     4096 2019-06-12 14:35:28.036879100 +0300 .
drwxrwxrwx 1 gruthen gruthen     4096 2019-06-12 14:08:31.547802400 +0300 ..
-rwxrwxrwx 1 gruthen gruthen 98519085 2019-05-27 17:33:00.000000000 +0300 acc1.dat
-rwxrwxrwx 1 gruthen gruthen 96438273 2019-05-27 17:39:00.000000000 +0300 acc2.dat
-rwxrwxrwx 1 gruthen gruthen 93231151 2019-05-27 17:43:16.000000000 +0300 acc3.dat
-rwxrwxrwx 1 gruthen gruthen 93552642 2019-05-27 17:48:44.000000000 +0300 acc4.dat
-rwxrwxrwx 1 gruthen gruthen 97869849 2019-05-27 17:52:48.000000000 +0300 acc5.dat
-rwxrwxrwx 1 gruthen gruthen 95025164 2019-05-27 17:56:46.000000000 +0300 acc6.dat
-rwxrwxrwx 1 gruthen gruthen 92198946 2019-05-27 18:00:28.000000000 +0300 acc7.dat
-rwxrwxrwx 1 gruthen gruthen 91303975 2019-05-27 18:04:06.000000000 +0300 acc8.dat

2019-05-27 17:33:00
2019-05-27 17:39:00
2019-05-27 17:43:16
2019-05-27 17:48:44
2019-05-27 17:52:48
2019-05-27 17:56:46
2019-05-27 18:00:28
2019-05-27 18:04:06
*/

	const CTime fileCreatedTimes[8] = {
			{2019,05,27,17,33,00},
			{2019,05,27,17,39,00},
			{2019,05,27,17,43,16},
			{2019,05,27,17,48,44},
			{2019,05,27,17,52,48},
			{2019,05,27,17,56,46},
			{2019,05,27,18,00,28},
			{2019,05,27,18,04,06} };

	vector<string> inFilepaths;
	for (auto& fname : inFilenames)
		inFilepaths.push_back(inFolder + "/" + fname + inFileExt);

	// Estimate the start times from the number of samples and the file-creation time
	vector<unsigned int> fileSizesBytes;
	for (auto& fp : inFilepaths) {
		fileSizesBytes.push_back(file_size(fp.c_str()));
	}

	vector<CTimeSpan> fileDurations;
	for (auto& fs : fileSizesBytes) {
		int t = fs / (100 * 6); // 100Hz, 6-bytes per sample (x/y/z*ushort)
		const int secs = t % 60;
		t /= 60;
		const int mins = t % 60;
		t /= 60;
		const int hours = t % 24;
		t /= 24;
		const int days = t;
		fileDurations.push_back(CTimeSpan(days,hours, mins, secs));
	}

	vector<CTime> fileStartTimes;
	cout << "ID, Start, Duration, End" << endl;
	for (int i = 0; i != 8; ++i) {
		fileStartTimes.push_back(fileCreatedTimes[i] - fileDurations[i]);
		SYSTEMTIME st,ste;
		fileStartTimes.back().GetAsSystemTime(st);
		fileCreatedTimes[i].GetAsSystemTime(ste);
		auto& dur = fileDurations[i];
		cout << (i+1) << ", " <<
			"Start " << st.wDay << "." << st.wMonth << "." << st.wYear << " " << 
			st.wHour << ":" << setfill('0') << setw(2) << st.wMinute << ":" << setfill('0') << setw(2) << st.wSecond << ", " << 
			"Duration " << dur.GetTotalHours() << "h" << setfill('0') << setw(2) << dur.GetMinutes() << "m" << setfill('0') << setw(2) << dur.GetSeconds() << "s, " <<
			"End " << ste.wDay << "." << ste.wMonth << "." << ste.wYear << " " <<
			ste.wHour << ":" << setfill('0') << setw(2) << ste.wMinute << ":" << setfill('0') << setw(2) << ste.wSecond << 
			endl;
	}

	vector<std::ifstream> inFiles;
	for (auto& fpath : inFilepaths) {
		inFiles.push_back(std::ifstream(fpath.c_str(), ios::in | ios::binary));
		if (!inFiles.back().good()) {
			cout << "Error opening input file. Filepath=\"" << fpath << "\"" << endl;
			return -1;
		}
	}
	std::ofstream outFile(outFilepath.c_str(), ios::out);
	if (!outFile.good()) {
		cout << "Error opening output file. Filepath=\"" << outFilepath << "\"" << endl;
		return -1;
	}

	vector<vector<double> > filesData;

	int fileCount = 0;
	for (auto& inFile : inFiles) {
		cout << "\r" << "Processing input file " << ++fileCount << "/" << inFiles.size() << ". Filepath=\"" << inFilepaths[fileCount - 1] << "\"";
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