gppflags='-Wall -std=c++17 -shared -m32 -fPIC -Ofast -s -static'
time(
	echo "Compiling"
	g++ $gppflags main.cpp -o sample_plugin.so
	echo "Done")