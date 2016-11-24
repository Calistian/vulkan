#include "vulkan_shader.h"
#include <sstream>
#include <fstream>

using namespace std;
using namespace vulkan;

static const char const * prog_name = "C:\\VulkanSDK\\Bin32\\glslangValidator.exe";

static string createSPVFile(const std::string& filename, const std::string& stage)
{
	ostringstream oss;
	oss << prog_name << " ";
}

vulkan_shader::vulkan_shader(const std::string& filename, vk::Device device, vk::ShaderStageFlagBits stage)
{

}
