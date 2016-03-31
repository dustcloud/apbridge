#ifndef VERSION_H_
#define VERSION_H_

/*
 * Common version functions
 *
 * These common version functions are implemented independently in each
 * component, so that versions can be tracked separately.
 */

int getMajorVersion();
int getMinorVersion();
int getReleaseVersion();
int getBuildVersion();

const char* getBuildTimestamp();
const char* getBuildName();
const char* getVersionLabel();

#endif /* ! VERSION_H_ */
