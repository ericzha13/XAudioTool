#pragma once


#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include "windows.h"
#include <sys\stat.h>
#include <fstream>
#include <conio.h>
#include <functional>
#include <shared_mutex>
#include <chrono>


#include "FileHelper.hpp"

#include "json.hpp"
#include "TimeHelp.hpp"

const int sampleRate = 16000;
const int sampleSize = 2; // 16位为2字节
constexpr static int audio_readsize_once = 1 * 256 * sizeof(short) * 10000;//读音频缓存buffer
/*
不处理过小的音频

*/
using namespace std;

enum class ToolsMsg
{
	/* Task Info */
	TaskMvw = 10000,     // 唤醒消息
	TaskSr,              // 识别消息
	TaskSe,              // SE消息
};

using ToolsCallback = std::function<void(ToolsMsg, const json::value&, void*)>;



//音频转换，将pcm转wav，支持单条和批量
//后续会支持更多音频格式
class AudioAssistant {
public:
	//AudioAssistant(ToolsCallback callback, void* callback_arg);
	AudioAssistant();
	AudioAssistant(const std::string res);
	AudioAssistant(const AudioAssistant&) = delete;
	AudioAssistant(AudioAssistant&&) = delete;
	AudioAssistant& operator=(const AudioAssistant&) = delete;
	AudioAssistant& operator=(AudioAssistant&&) = delete;

	//公共函数
	bool wav_to_pcm();
	bool wav_to_pcm(const char* str);

	bool split_audio(const std::string&& audioPath, const int&& chanel);

	bool set_target_path_or_file(const string&& str);
	bool cut_audio_timepoint(const string&& audioPath, const int&& chanel);

	~AudioAssistant();

private:
	std::filesystem::path m_current_working_folder = "";//exe文件当前路径，待用
	std::filesystem::path m_prehandled_file = "";//待处理文件名称，一般保存集成方传入的路径
	std::filesystem::path m_preoutput_file = "";//待处理文件名称，一般保存集成方传入的路径
	std::filesystem::path m_output_pcm_folder = "";//一般是产物的路径


	std::unique_ptr<char[]> m_readpcm_buffer_common = nullptr;//一个智能指针，用于最开始申请公用的读数据缓存空间
	mutable std::mutex m_readpcm_buffer_mutex;//m_readpcm_buffer_common的锁，该变量永远处于可变状态
	std::unique_ptr<char[]> mp_tmp_input_cache[16];

	bool check_suffix(const std::string&& input_pcm_pcmfile);


};





//用于合并音频的类，初始化的时候支持传入多个音频文件或者传一个文件夹
class MergeAudio
{
public:
	~MergeAudio() = default;
	MergeAudio();
	MergeAudio(const fs::path working_path);
	MergeAudio(const MergeAudio&) = delete;
	MergeAudio(MergeAudio&&) = delete;
	MergeAudio& operator=(const MergeAudio&) = delete;
	MergeAudio& operator=(MergeAudio&&) = delete;

	void refilter_by_extension(const std::string ext);//输入扩展名，将文件池里的不是该扩展名的文件剔除池子
	bool start_merge();//开始合并吧
	bool reset_output_file(fs::path new_output_file);


	//输入多个单声道的音频，并合并
	template<typename... Args>
	bool choose_audio_and_merge(Args&&... args) {
		
		(m_audio_pool.emplace_back(std::forward<Args>(args)), ...);

		m_num_of_material = m_audio_pool.size();
		if (!ast::utils::are_files_regular(m_audio_pool)) {
			deinit();
			std::cerr << "some file isnot regular_file " << std::endl;
			return false;
		}

		if (!start_merge()) {
			deinit();
			std::cerr << "start_merge failed " << std::endl;
			return false;
		}

		return true;
	}

private:
	void deinit();

	
	std::vector<fs::path> m_audio_pool;
	fs::path m_default_output;//输出文件的路径，可设置，不设置为当前目录
	int m_num_of_material;//待处理音频的数量，也是通道数
	std::vector<std::ifstream> v_ifs_handle; // 用于管理 ifstream 对象的 vector 容器
	ofstream m_ofs;//输出文件描述符
};


/*
用于音频拆分的类，暂时支持单个文件的切分。

最终功能：
①  传入一个文件夹，寻找该文件夹内所有的音频，对每个音频文件进行拆分。
	拆分结果直接放到当前目录，如果有相同名称的先强行删除。
*/
class SplitAudio
{
public:
	~SplitAudio() = default;
	SplitAudio()=delete;
	SplitAudio(const fs::path files,const int chanel);

	SplitAudio(const SplitAudio&) = delete;
	SplitAudio(SplitAudio&&) = delete;
	SplitAudio& operator=(const SplitAudio&) = delete;
	SplitAudio& operator=(SplitAudio&&) = delete;

	void refilter_by_extension(const std::string ext);//输入扩展名，将文件池里的不是该扩展名的文件剔除池子
	bool start_split();//开始切分吧


private:
	void deinit();

	std::unique_ptr<char[]> m_readpcm_buffer_common = nullptr;//一个智能指针，用于最开始申请公用的读数据缓存空间
	std::vector<fs::path> m_audio_pool;
	int m_input_chanel;//待输入音频的数量
	std::vector<std::ofstream> v_ofs_handle; // 用于管理 ofstream 对象的 vector 容器
	ifstream m_ifs;//输入文件描述符
	
};








class FindAudioPosition
{
public:
	~FindAudioPosition()= default;
	FindAudioPosition() = delete;
	FindAudioPosition(const fs::path short_path,const fs::path long_path);
	FindAudioPosition(const FindAudioPosition&) = delete;
	FindAudioPosition(FindAudioPosition&&) = delete;
	FindAudioPosition& operator=(const FindAudioPosition&) = delete;
	FindAudioPosition& operator=(FindAudioPosition&&) = delete;

	long get_shortaudio_position();//获取短音频在长音频的位置，返回值是毫秒，仅仅支持单通道
	string get_shortaudio_position_str();
	

private:
	bool is_init_success = false;
	int findSubsequence(const short* longAudio, const short* shortAudio);

	std::unique_ptr<char[]> m_short_audio_buffer = nullptr;
	constexpr static int ShortAudioSize = 3 * 32000;//短音频只读取3秒
	int actual_shortaudio_length_read = 0;//实际读取到的短音频长度，一般小于等于ShortAudioSize

	ifstream ifs_long_audio;
	std::unique_ptr<char[]> m_long_audio_buffer = nullptr;
	constexpr static int LongAudioSize = 30 * 1024 * 1024;//长音频每次读取30mb
	int actual_longaudio_length_read = 0;//每次实际read长音频获得的字节
	long long_audio_total_size = 0;//长音频的总长度

	bool m_has_found = false;
};


//切音频的类，目前实现单通道音频的切分，多通道暂未实现
class CutOrLengthenAudio
{
public:
	~CutOrLengthenAudio() = default;
	CutOrLengthenAudio() = delete;
	CutOrLengthenAudio(const fs::path original_path,int chanel = 1);
	CutOrLengthenAudio(const FindAudioPosition&) = delete;
	CutOrLengthenAudio(FindAudioPosition&&) = delete;
	CutOrLengthenAudio& operator=(const FindAudioPosition&) = delete;
	CutOrLengthenAudio& operator=(FindAudioPosition&&) = delete;

	bool cut_op(const std::string& cut_time_str);


private:
	bool is_init_success = false;

	constexpr static int ReadAudioSize = 5 * 1024 * 1024;//音频每次读取5mb
	ifstream ifs_original_audio;
	ofstream ofs_output_audio;
	std::unique_ptr<char[]> m_original_audio_buffer = nullptr;
	long m_long_audio_total_size = 0;//长音频的总长度
	
	bool get_start_and_end_ms(const std::string& time_str,long& start_ms,long& end_ms);
	bool cut_op_main(const long start_byte, const long end_byte);

	int audio_chanel = 1;
};



