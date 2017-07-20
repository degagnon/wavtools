/*
 * wavtools.cpp
 *
 *  Created on: Jul 19, 2017
 *      Author: david
 */

#include <iostream>
using namespace std;

int main(int argc, char** argv){
	cout << "Number of input args is " << argc << endl;
	cout << "Arg loc is " << argv << endl;
	cout << "Arg values are:" << endl;
	for(int i = 0; i < argc; ++i){
		cout << argv[i] << endl;
	}


	// Here is a simple comment to test git changes.

	return 0;
}


