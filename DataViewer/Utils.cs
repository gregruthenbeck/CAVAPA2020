using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DataViewer {
    class Utils {
        static public float[][] LoadCSVData(string filepath) {
            List<List<float>> data = new List<List<float>>();
            using (TextReader file = File.OpenText(filepath)) {
                string line = "";
                while ((line = file.ReadLine()) != null) {
                    string[] split = line.Split(new[] { ',' });
                    if (data.Count == 0) {
                        foreach (string s in split) {
                            if (s.Length > 0)
                                data.Add(new List<float>());
                        }
                    }
                    for (int i = 0; i < split.Length; i++) {
                        if (split[i].Length == 0)
                            continue;

                        data[i].Add(float.Parse(split[i]));
                    }
                }
            }
            float[][] arr = new float[data.Count][];
            for (int i = 0; i < data.Count; i++) {
                arr[i] = data[i].ToArray();
            }
            return arr;
        }

        static public void GetExtents(out float min, out float max, ref float[] data) {
            min = float.MaxValue;
            max = float.MinValue;
            foreach (var d in data) {
                min = Math.Min(min, d);
                max = Math.Max(max, d);
            }
        }
    }
}
