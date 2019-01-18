gppflags='-Wall -shared -m32 -fPIC -Ofast -s -static -std=c++0x'
time(
	echo "Compiling"
	g++ $gppflags main.cpp -o sample_plugin.so
	echo "Done")
