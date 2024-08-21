#include <iostream>
#include <windows.h>
#include <vector>
#include <fstream>
#include <string>
#include <limits>
#include <cmath>
#include <algorithm>
#include <cstdio>

using namespace std;

ifstream dir_in("Audio locations.txt"); /// Input directories

string ffmpeg_loc; /// FFmpeg location
string output_loc; /// Output directory location
string command; /// Command to be executed in loop

vector<string> audio_paths;
vector<long double> audio_duration;

bool analyse;

/// Types available decisions on console.
void leave_or_start_over() {
	cout << "Please type in the action you want to do next:\n";
	cout << "Leave (L)\n";
	cout << "Start over (S)\n";
}

/// Gets and sets FFmpeg location and Output directory.
void handle_FFmpeg_location_output_dir() {
	getline(dir_in, ffmpeg_loc);
	getline(dir_in, output_loc);
	dir_in.close();

	if (ffmpeg_loc == "" || output_loc == "") {
		ofstream write("Audio locations.txt");
		cout << "Please copy the location of your ffmpeg.exe or drag & drop it here.\n";
		getline(cin, ffmpeg_loc);

		cout << "Please type in the output directory or drag & drop it here.\n";
		getline(cin, output_loc);

		write << ffmpeg_loc << "\n" << output_loc;
		write.close();
	}
	output_loc.resize(output_loc.size() - 1);
}

/// Prepares 'command_runner' to analyse audio file
void set_analyse() {
	command = "-i <a> -f null -";
	analyse = true;
}

/// Prepares 'command_runner' to loop audio file
void set_loop() {
	command = "-y -i <a> -c:a pcm_s16le";
	analyse = false;
}

void get_audio_locations() {
	string curr_path;
	for (;;) {
		audio_paths.clear();
		audio_paths.shrink_to_fit();
		system("cls");

		cout << "Please drag & drop your audio files here in the order you want them looped.\nOnce you are done, press Enter.\n";

		while (getline(cin, curr_path) && curr_path != "") {
			audio_paths.emplace_back(curr_path);
		}
		system("cls");
		if (!audio_paths.size()) {
			cout << "You did not enter any audio file paths.\n";
			leave_or_start_over();

			char x;
			x = (char)getchar();
			if (x == 'L' || x == 'l')
				exit(0);
			else
				continue;
		}
		break;
	}
}

long double parse_duration() {
	long double duration = 0;
	ifstream fout("ffmpeg.txt"); /// Read FFmpeg output

    while (duration == 0) {
		string line = "";
        getline(fout, line);
        size_t line_it = line.find("Duration: ");

        /// Conversion from char to double
		if (line_it != numeric_limits<size_t>::max()) {
			line_it += 10; /// "Duration" size.
			string sDuration = line.substr(line_it, 11);
			unsigned short duration_component = 0, component = 0;
			for (size_t dur_it = 0; dur_it < sDuration.size(); dur_it++) {
				if (sDuration[dur_it] >= '0' && sDuration[dur_it] <= '9')
					duration_component = duration_component * 10 + sDuration[dur_it] - '0';
				else {
                    if (component == 0)
						duration += duration_component * 3600;
					else if (component == 1)
						duration += duration_component * 60;
					else
						duration += duration_component;
					component++;
					duration_component = 0;
				}
			}
			duration += (long double)duration_component / (long double)100;
		}
    }

	return duration;
}

void command_runner() {
	for (unsigned i = 0; i < audio_paths.size(); i++) {
		size_t r_limit;
		for (size_t j = audio_paths[i].size() - 1; j < numeric_limits<size_t>::max(); j--)
			if (audio_paths[i][j] == '.') {
				r_limit = j;
				break;
			}
		size_t l_limit;
		for (size_t j = r_limit; j < numeric_limits<size_t>::max(); j--)
			if (audio_paths[i][j] == '\\') {
				l_limit = j;
				break;
			}

		string audio_name;
		for (size_t j = l_limit + 1; j < r_limit; j++)
			audio_name += audio_paths[i][j];

		string looping_analyse = "";
		if (analyse)
			looping_analyse = "Analysing file ";
		else looping_analyse = "Looping file ";
		cout << looping_analyse << i + 1 << " out of " << audio_paths.size() << "...\n";

		/// Command patcher:
		string current_command = "";
		string file_type = "";
		size_t file_type_pos = 0;
		for (size_t it = 0; it < command.size(); it++) {
			if (command[it] == '<' && command[it + 1] == 'a')
				current_command += audio_paths[i], it += 2;
			else {
				if (command[it] != '.')
					current_command += command[it];
				else {
					file_type_pos = it;
					break;
				}
			}
		}
		if (file_type_pos)
			for (; file_type_pos < command.size(); file_type_pos++)
				file_type += command[file_type_pos];

		/// Executing command:
		string operation;
		if (analyse)
			operation += '"' + ffmpeg_loc + ' ' + current_command + " > ffmpeg.txt 2>&1" + '"';
		else {
			unsigned short times_to_loop = ceil(180 / audio_duration[i] + 1);
			operation += '"' + ffmpeg_loc + ' ' + "-stream_loop " + to_string(times_to_loop) + ' ' + current_command + " -t " + to_string((times_to_loop - 1) * audio_duration[i] + 8) + " -af \"afade=out:st=" + to_string((times_to_loop - 1) * audio_duration[i]) + ":d=8:curve=cub\" " + output_loc + audio_name + ".wav" + '"' + '"';

			ofstream timestamps(output_loc.substr(1, output_loc.size()) + audio_name + ".txt");
			unsigned short minutes = 0, times_looped = 0;
			long double seconds = 0.125;
			while (times_looped < times_to_loop) {
				timestamps << minutes << ":" << (seconds >= 10 ? to_string((short)floor(seconds)) : "0" + to_string((short)floor(seconds))) << " " + audio_name << " " << times_looped + 1 << "\n";
                seconds += audio_duration[i];
                times_looped++;
                if (seconds >= 60) {
					seconds -= 60;
					minutes++;
                }
			}
		}
		system(operation.c_str());

		if (analyse) {
			long double current_duration = parse_duration();
			audio_duration.emplace_back(current_duration);
		}
	}
}

int main() {
	cout << "Hello!\nTo begin using this program, press Enter.\n";
	getchar();

	handle_FFmpeg_location_output_dir();
	get_audio_locations();

	set_analyse();
	command_runner();
	remove("ffmpeg.txt");

	set_loop();
	command_runner();
}
