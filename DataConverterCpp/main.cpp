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

typedef unsigned char AccDataReadMode;
static const AccDataReadMode ADRM_MARKER = 0;
static const AccDataReadMode ADRM_DATA = 1;
struct AccDataReadMarker {
	unsigned char day, month, year, hour, min, sec;
};

// ".dat" files contain time markers with weird format
unsigned char hexToInt(unsigned char c) {
	return (c / 16 * 10) + c % 16;
}

AccDataReadMarker ReadFileMarker(std::ifstream& fs) {
	AccDataReadMarker m{};
	// Hex editors show that there are "AA AA" markers at regular intervals (which is 170dec)
	unsigned char d = 0;
	fs.read((char*)&d, 1);
	if (d != 170 && d != 0) {
		throw;
		return m;
	}
	if (d == 0) {
		return m;
	}
	fs.read((char*)& d, 1);
	if (d != 170 && d != 0) {
		throw;
		return m;
	}

	fs.read((char*)& m.day, 1);
	fs.read((char*)& m.month, 1);
	fs.read((char*)& m.year, 1);
	fs.read((char*)& m.hour, 1);
	fs.read((char*)& m.min, 1);
	fs.read((char*)& m.sec, 1);
	m.day = hexToInt(m.day);
	m.month = hexToInt(m.month);
	m.year = hexToInt(m.year);
	m.hour = hexToInt(m.hour);
	m.min = hexToInt(m.min);
	m.sec = hexToInt(m.sec);
	return m;
}

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

std::ostream& operator<<(std::ostream& os, const CTime& t) {
	SYSTEMTIME st;
	t.GetAsSystemTime(st);
	os << st.wDay << "." << st.wMonth << "." << st.wYear << " " <<
		st.wHour << ":" << setfill('0') << setw(2) << st.wMinute << ":" << setfill('0') << setw(2) << st.wSecond;
	return os;
}

std::ostream& operator<<(std::ostream& os, const CTimeSpan& dur) {
	os << dur.GetTotalHours() << "h" << 
		setfill('0') << setw(2) << dur.GetMinutes() << "m" << 
		setfill('0') << setw(2) << dur.GetSeconds() << "s";
	return os;
}

/*
	There are 16,419,847 samples in the 1st accelerometer file. At 100Hz, this represents
	16419847/100/60/60 = 45.6107 hrs. The file creation time (acc1.dat) is ‎Monday, ‎27 ‎May ‎2019, ‏‎5:33:00 PM.
	The activities of interest occurred on Sunday afternoon from about 2:30pm onwards.
	That’s 27hrs33mins earlier than the file creation time. 45.6 - 27.5 = 18.1hrs from the
	start of the recordings is the start of the interesting data. 18.1 * 100*60*60=6.516E6 samples
	can be skipped at the beginning of hr.dat, and not more than 2hrs of the subsequent data is of
	interest (2*100*60*60=720,000 samples for the 2hrs at 100Hz).
*/
CTimeSpan BytesToTime(const unsigned long bytes, const unsigned bytesPerSecond) {
	unsigned long t = bytes / bytesPerSecond; // 100Hz, 6-bytes per sample (x/y/z*ushort)
	const unsigned secs = t % 60;
	t /= 60;
	const unsigned mins = t % 60;
	t /= 60;
	const unsigned hours = t % 24;
	t /= 24;
	const unsigned days = t;
	return CTimeSpan(days, hours, mins, secs);
}

/*
ls -la --time-style=full-iso
	2019-05-27 17:33:00
	2019-05-27 17:39:00
	2019-05-27 17:43:16
	2019-05-27 17:48:44
	2019-05-27 17:52:48
	2019-05-27 17:56:46
	2019-05-27 18:00:28
	2019-05-27 18:04:06
*/
//const CTime fileEndTimes[8] = { // Doesn't affect output data (only logged info)
//		{2019,05,27,17,33,00},
//		{2019,05,27,17,39,00},
//		{2019,05,27,17,43,16},
//		{2019,05,27,17,48,44},
//		{2019,05,27,17,52,48},
//		{2019,05,27,17,56,46},
//		{2019,05,27,18,00,28},
//		{2019,05,27,18,04,06} };
//// Estimate the start times from the number of samples and the file-creation time
//vector<unsigned long> fileSizesBytes;
//for (auto& fp : inFilepaths)
//fileSizesBytes.push_back((unsigned long)file_size(fp.c_str()));
//
//vector<CTimeSpan> fileDurations;
//for (auto& fs : fileSizesBytes)
//fileDurations.push_back(BytesToTime(fs, samplingRateHz * 6)); // 100Hz, 6-bytes per sample (x/y/z*ushort)
//
//vector<CTime> fileStartTimes;
//for (int i = 0; i != 8; ++i)
//fileStartTimes.push_back(fileEndTimes[i] - fileDurations[i]);
//
//cout << "ID, Start, Duration, End" << endl;
//for (int i = 0; i != 8; ++i) {
//	cout << (i + 1) << ", Start " << fileStartTimes[i] <<
//		", Duration " << fileDurations[i] <<
//		", End " << fileEndTimes[i] << endl;
//}
//
//vector<unsigned long> startSampleIds;
//for (int i = 0; i != 8; ++i) {
//	CTimeSpan delta = roiStartTime - fileStartTimes[i]; // dist from start
//	startSampleIds.push_back((unsigned long)(100 * delta.GetTotalSeconds()));
//	//CTimeSpan delta = fileEndTimes[i] - startTime; // dist from end
//	//startSampleIds.push_back(100 * (unsigned long)(fileDurations[i].GetTotalSeconds() - delta.GetTotalSeconds()));
//}

template < class T >
std::ostream& operator << (std::ostream& os, const std::vector<T>& v) {
	os << "[";
	for (auto &i : v)
		os << " " << i;
	os << "]";
	return os;
}

int main() {
	const string inFolder = "../data_in";
	const string inFileExt = ".dat";
	const vector<string> inFilenames{ "acc1", "acc2", "acc3", "acc4", "acc5", "acc6", "acc7", "acc8" };
	const string outFilepath = "../data_out/acc-all.csv";
	const int samplingRateHz = 100;
	const int averagingWindowLen = 500;
	const int averagingWindowLen2 = 1;
	CTime roiStartTime(2019, 5, 26, 15, 0, 0); // The experiment started at about 2:30pm on Sunday 26/5/2019
	const unsigned long roiLen = 360000;	   // samples
	const unsigned saveInterval = 100;		   // write every 'n' samples (skip the rest. NB: AveWindow doesn't skip any)

	cout << "Input folder:\t\"" << inFolder << "\"" << endl;
	cout << "Input file extension:\t\"" << inFileExt << "\"" << endl;
	cout << "Input file names:\t" << inFilenames << endl;
	cout << "Output file-path:\t\"" << outFilepath << "\"" << endl;
	cout << "AveWinLen:\t" << averagingWindowLen << endl;
	cout << "AveWinLen2:\t" << averagingWindowLen2 << endl;
	cout << "ROI start time:\t" << roiStartTime << endl;
	cout << "SaveInterval:\t" << saveInterval << " (" << (saveInterval / samplingRateHz) << "Hz)" << endl;
	cout << "ROILen:\t" << roiLen << "samples (" << (roiLen / (samplingRateHz * 60 * 60)) << "hours = " << (roiLen / saveInterval) << "rows)" << endl;

	if ((saveInterval / samplingRateHz) != 1) {
		cout << "WARNING: CSV data is written assuming that output rows represent 1Hz" << endl;
	}

	vector<string> inFilepaths;
	for (auto& fname : inFilenames)
		inFilepaths.push_back(inFolder + "/" + fname + inFileExt);

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
	vector<unsigned long> startedAtSampleId(8, 0L);

	int fileCount = 0;
	for (auto& inFile : inFiles) {
		cout << "\r" << "Processing input file " << ++fileCount << "/" << inFiles.size() << ". Filepath=\"" << inFilepaths[fileCount - 1] << "\"";

		WindowAverager<double> windower;
		windower.SetWindowLength(averagingWindowLen);
		WindowAverager<double> windowerLong;
		windowerLong.SetWindowLength(averagingWindowLen2);
		AccelData data;
		vector<double> fileData;

		unsigned long start = numeric_limits<unsigned long>::max(); // set to 'count' when 'marker-time' == 'start-time'
		unsigned long count = 0L;
		unsigned long roiCount = 0L;
		CTime timeMarker, timeMarkerPrev;
		while (!inFile.eof() && inFile.good()) {
			try {
				if (count % 84 == 0) {
					AccDataReadMarker m = ReadFileMarker(inFile);
					timeMarker = CTime(2000 + m.year, m.month, m.day, m.hour, m.min, m.sec);
					if (count != 0) { // skip the delta check when we only have the first marker
						CTimeSpan delta = timeMarker - timeMarkerPrev;
						if (timeMarker.GetYear() != 2000 && timeMarkerPrev.GetYear() != 2000 && // Ok to have 'null' time stamps as markers
							delta.GetTotalSeconds() > 1) { // not ok to have deltas greater than 1s
							throw;
						}
					}
					timeMarkerPrev = timeMarker;
				}
				inFile.read((char*)& data, sizeof(AccelData));
				++count;
				auto delta = roiStartTime - timeMarker;
				if (delta.GetTotalSeconds() < 1) {
					start = count;
					startedAtSampleId[fileCount - 1] = start;
				}
				if (count < start) {
					continue;
				}
				++roiCount;
				if (roiCount > roiLen) {
					break;
				}
				const double len = Length(data);
				const double ave = windower.AddGetAverage(len);
				//const double longAve = windowerLong.AddGetAverage(ave);
				if (roiCount % saveInterval == 0) {
					fileData.push_back(abs(ave));
					//fileData.push_back(abs(ave - longAve));
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
		outFile << i << sep << ((double)i / 86400.0) << sep; // 2nd column is helper for formatting seconds as time in excel
		for (auto& d : filesData) {
			outFile << d[i] << sep;
		}
		outFile << endl;
	}
	outFile.flush();
	outFile.close();

	cout << "done" << endl;
}