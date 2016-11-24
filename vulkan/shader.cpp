#include "shader.h"
#include <fstream>

using namespace std;

shader::shader(const string& filename)
{
	ifstream file(filename, ios::ate, ios::binary);
	_source.resize(file.tellg());
	file.seekg(0);
	file.read(data(_source), std::size(_source));
}

const char* shader::source() const
{
	return data(_source);
}

size_t shader::size() const
{
	return std::size(_source);
}


