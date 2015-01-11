#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "10";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.12";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 4;
	static const long BUILD  = 480;
	static const long REVISION  = 2620;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 655;
	#define RC_FILEVERSION 1,4,480,2620
	#define RC_FILEVERSION_STRING "1, 4, 480, 2620\0"
	static const char FULLVERSION_STRING [] = "1.4.480.2620";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 80;
	

}
#endif //VERSION_H
