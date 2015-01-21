#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "21";
	static const char MONTH[] = "01";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.01";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 188;
	static const long REVISION  = 1027;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 226;
	#define RC_FILEVERSION 1,1,188,1027
	#define RC_FILEVERSION_STRING "1, 1, 188, 1027\0"
	static const char FULLVERSION_STRING [] = "1.1.188.1027";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 88;
	

}
#endif //VERSION_H
