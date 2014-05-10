#include <iostream>
#include <cstdlib>
using namespace std;

int main()
{
	cout << "Hello World!" << endl;
	system("java -jar JavaTransform.jar SOURCE Hello TARGET");
	return 0;
}