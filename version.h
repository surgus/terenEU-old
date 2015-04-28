#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 4;
	static const long BUILD  = 474;
	static const long REVISION  = 2551;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 514;
	#define RC_FILEVERSION 1,4,474,2551
	#define RC_FILEVERSION_STRING "1, 4, 474, 2551\0"
	static const char FULLVERSION_STRING [] = "1.4.474.2551";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 74;
	

}
#endif //VERSION_H
