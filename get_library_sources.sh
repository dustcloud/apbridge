#!/bin/bash

# get_library_sources
# 
# Download and unpack the required libraries into the local directory

echo "Downloading required libraries..."

get_and_expand_url() {
	local url="$1"
	local name="$2"
	if [ -z "$1" ]; then
		echo "usage: get_and_expand_url <url>"
		return 1
	fi
	local base_file=`basename $url`

	echo "Downloading $name..."
	wget "$url" -O "$base_file"
	if [ $? -ne 0 ]; then
		echo "error: could not download $name"
		return 1
	fi
	
	echo "Unpacking $name..."
	gunzip -c "$base_file" | tar -xf -
	if [ $? -ne 0 ]; then
		echo "error: could not unpack $name"
		return 1
	fi
	echo ""
}

get_and_expand_url http://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.gz "Boost 1.60.0"
if [ $? -ne 0 ]; then
	exit 1
fi

get_and_expand_url https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz "protobuf 2.5.0"
if [ $? -ne 0 ]; then
	exit 1
fi

get_and_expand_url https://github.com/zeromq/zeromq3-x/releases/download/v3.2.5/zeromq-3.2.5.tar.gz "ZeroMQ 3.2.5"
if [ $? -ne 0 ]; then
	exit 1
fi

get_and_expand_url https://github.com/zeromq/czmq/releases/download/v1.3.2/czmq-1.3.2.tar.gz "CZMQ 1.3.2"
if [ $? -ne 0 ]; then
	exit 1
fi

get_and_expand_url http://archive.apache.org/dist/logging/log4cxx/0.10.0/apache-log4cxx-0.10.0.tar.gz "log4cxx 0.10.0"
if [ $? -ne 0 ]; then
	exit 1
fi

get_and_expand_url http://archive.apache.org/dist/apr/apr-1.4.6.tar.gz "apr 1.4.6"
if [ $? -ne 0 ]; then
	exit 1
fi

get_and_expand_url http://archive.apache.org/dist/apr/apr-util-1.5.1.tar.gz "apr-util 1.5.1"
if [ $? -ne 0 ]; then
	exit 1
fi

get_and_expand_url http://download-mirror.savannah.gnu.org/releases/gpsd/gpsd-3.11.tar.gz "gpsd 3.11"
if [ $? -ne 0 ]; then
	exit 1
fi

echo "Done getting library sources!"
