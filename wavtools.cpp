/*
 * wavtools.cpp
 *
 *  Created on: Jul 19, 2017
 *      Author: david
 */

#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

using namespace std;

int main(int argc, char** argv){
	cout << "Number of input args is " << argc << endl;
	cout << "Arg loc is " << argv << endl;
	cout << "Arg values are:" << endl;
	for(int i = 0; i < argc; ++i){
		cout << argv[i] << endl;
	}

	char* data;
	streampos filesize;

	ifstream file (argv[1], ios::in|ios::binary|ios::ate);
	if(file.is_open()){
		cout << "File opened." << endl;
		filesize = file.tellg();
		cout << "File size is " << filesize << " bytes" << endl;
		data = new char [filesize];
		file.seekg(ios::beg);
		file.read(data, filesize);
		file.close();
		cout << "File closed." << endl;
		for(int i = 0; i < 50; ++i){
			cout << data[i] << endl;
		}
		delete[] data;
	}
	else{
		cout << "File was not opened.";
	}

	return 0;
}


