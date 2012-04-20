#!/usr/bin/env python


# This is a testing scripts that tries to build vermont with all
# avaiable options. Do not use this to build vermont if you want
# to have a usable built of vermont.

# Note: We have a lot of options for configuring vermont available.
# This test builts will run for ages to complete all of them ...

OptionList = [ 
	'DEBUG', 
	'DISABLE_MEMMANAGER', 
	'IPFIXLOLIB_RAWDIR_SUPPORT', 
	'SUPPORT_DTLS', 
#	'SUPPORT_DTLS_OVER_SCTP', 
	'SUPPORT_MYSQL', 
	'SUPPORT_NETFLOWV9', 
#	'SUPPORT_ORACLE', 
	'SUPPORT_POSTGRESQL',
	'SUPPORT_SCTP',
	'USE_GSL',
#	'USE_PCAPMMAP',
#	'USE_PFRING',
	'WITH_TESTS',
	'WITH_TOOLS',
	'SUPPORT_MONGO'
]

def rm_rf(d):
	"""Recursively delete content of d. Used to clean the previous builts"""
	for path in (os.path.join(d,f) for f in os.listdir(d)):
		if os.path.isdir(path):
			rm_rf(path)
		else:
			os.unlink(path)

def do_build(optlist):
	buildstring = "cmake ../ "
	for i in optlist:
		buildstring += i + " "
	print "Trying to build vermont with options: \n\t",
	print buildstring
	rm_rf(".")
	if os.system(buildstring) != 0:
		print "Failed configuring vermont!"
		sys.exit(-1)

	if os.system("make") != 0:
		print "Failed building vermont!"
		sys.exit(-1)

def build_all_option_list(optList):
	build(list(), optList)	

def build(fixed, rest):
	if len(rest) == 0:
		do_build(fixed)
		return 

	# build option list recursively
	option = rest.pop(0)
	
	restOn = list(rest)
	restOff = list(rest)
	withOn = list(fixed)
	withOff = list(fixed)

	withOn.append("-D" + option + "=ON")
	withOff.append("-D" + option + "=OFF")
	
	build(withOn, restOn)
	build(withOff, restOff)

if __name__ == "__main__":
	import os
	import sys

	# we are working with rm -rf. Do not run this as root.
	if os.geteuid()==0:
		sys.exit("\nDo not run this script as root!!!");

	# we need to clean a previous make cache. If we don't do this
	# cmake will reuse the cache and does not create a new cache in 
	# vermont/build/. But we don't want to do this automatically, so
	# let the user decide if we wants to do this
	if os.access("./CMakeCache.txt", os.R_OK):
		print "Found a previous CMakeCache.txt. I cannot make the test builds while this cache is ",
		print "resides in the vermont source directory. If you want to make the test builds, please",
		print "remove the file CMakeCache.txt. Please note that you have to redo your insource built",
		print "afterwards..."
		sys.exit(-1)

	print "Creating build directory ..."
	if not os.access("./build/",  os.R_OK | os.W_OK):
		os.makedirs("./build/")
	os.chdir("./build/")
	print "Starting build process ..."
	build_all_option_list(OptionList)
	pass
