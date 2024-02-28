#pragma once

#ifdef _WIN32
#ifdef CORE_API_IMPORT
#define CORE_API __declspec(dllimport)
#else
#define CORE_API  __declspec(dllexport)
#endif

#include "windows.h"
#include <conio.h>

#else //CORE_API_IMPORT
#define CORE_API __attribute__((visibility("default")))
#endif //_WIN32


#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <mutex>
#include <queue>

#include <sys/stat.h>
#include <fstream>

#include <functional>
#include <shared_mutex>
#include <chrono>
#include <thread>

#include <glog/logging.h>
#include "FileHelper.hpp"

#include "json.hpp"
#include "TimeHelp.hpp"

const int sampleRate = 16000;
const int sampleSize = 2; // 16位为2字节
constexpr static int audio_readsize_once = 1 * 256 * sizeof(short) * 10000;//读音频缓存buffer
/*
不处理过小的音频

*/
//using namespace std;


#define CHECK_ERROR_THROW(condition,note)  if (condition) {LOG(ERROR) << note;throw note;}       
#define CHECK_ERROR_RETURN(condition,note,errno)  if (condition) {LOG(ERROR) << note;return errno;} 
#define CHECK_WARNING(condition,note) if (condition) {LOG(WARNING) << note;}       

//音频转换，将pcm转wav，支持单条和批量
//后续会支持更多音频格式
class AudioAssistant {
public:
	//AudioAssistant(ToolsCallback callback, void* callback_arg);
	AudioAssistant();
	AudioAssistant(const std::string res);
	AudioAssistant(const AudioAssistant&) = delete;
	AudioAssistant(AudioAssistant&&) = delete;
	CORE_API AudioAssistant& operator=(const AudioAssistant&) = delete;
	CORE_API AudioAssistant& operator=(AudioAssistant&&) = delete;

	//公共函数
	CORE_API bool wav_to_pcm();
	CORE_API bool wav_to_pcm(const char* str);

	CORE_API bool split_audio(const std::string&& audioPath, const int&& chanel);

	CORE_API bool set_target_path_or_file(const std::string&& str);
	CORE_API bool cut_audio_timepoint(const std::string&& audioPath, const int&& chanel);

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



enum class MergeAudioParam
{
	ResetOutputFile = 0,
	SetAudioFormat,//过滤设置的格式以外格式的音频。.pcm?.mp4？如果不设置那么默认合并输入的所有音频(不过滤)。
};

//用于合并音频的类，初始化的时候支持传入多个音频文件或者传一个文件夹
class MergeAudio
{
public:
	~MergeAudio() = default;
	MergeAudio() = delete;
	MergeAudio(const MergeAudio&) = delete;
	MergeAudio(MergeAudio&&) = delete;
	MergeAudio& operator=(const MergeAudio&) = delete;
	MergeAudio& operator=(MergeAudio&&) = delete;

	CORE_API MergeAudio(fs::path working_path, const char* extension = ".pcm");
	CORE_API bool start_merge();//开始合并吧
	CORE_API bool setparam(MergeAudioParam key, const std::string& value);
	CORE_API void clear_audio();

	template <typename... Args>
	CORE_API bool add_audio(Args... args) {
		LogTraceFunction;
		std::unique_lock<std::mutex> lock(m_work_mutex);
		process_add_audio(std::forward<Args>(args)...);
		return true;
	}

	//考虑加入addaudio和ｃｌｅａｒ接口,标志位，是否初始化成功

	template <typename... Args>
	CORE_API MergeAudio(Args... args) {
		processArgs(std::forward<Args>(args)...);

		for (auto it : mv_input_pool) {
			if (fs::is_regular_file(it)) {
				mv_audio_pool_cache.emplace_back(it);
			}
			else {
				CHECK_WARNING(!ast::utils::get_files_from_directory(it, "*", mv_audio_pool_cache), "get_files_from_directory" << it << "failed");
			}
		}
		m_num_of_material = mv_audio_pool_cache.size();
		std::copy(mv_audio_pool_cache.begin(), mv_audio_pool_cache.end(), std::back_inserter(mv_audio_pool));
		m_default_output = fs::current_path() / ("merge_output_" + ast::utils::get_format_time(true) + ".pcm");
		for (auto it : mv_audio_pool_cache) LOG(INFO) << it;
	}

private:
	void deinit();

	template<typename T>
	void process_add_audio(T&& arg) {
		if (fs::is_regular_file(arg)) {
			mv_audio_pool_cache.emplace_back(arg);
		}
		else {
			CHECK_WARNING(!ast::utils::get_files_from_directory(arg, "*", mv_audio_pool_cache), "get_files_from_directory" << arg << "failed");
		}
		return;
	}

	template <typename T, typename... Args>
	void process_add_audio(T&& arg, Args&&... rest) {
		if (fs::is_regular_file(arg)) {
			mv_audio_pool_cache.emplace_back(arg);
		}
		else {
			CHECK_WARNING(!ast::utils::get_files_from_directory(arg, "*", mv_audio_pool_cache), "get_files_from_directory" << arg << "failed");
		}
		process_add_audio(std::forward<Args>(rest)...);
		return;
	}


	template <typename T>
	void processArgs(T&& arg) {
		mv_input_pool.emplace_back(std::forward<T>(arg));
	}

	template <typename T, typename... Rest>
	void processArgs(T&& arg, Rest&&... rest) {
		mv_input_pool.emplace_back(std::forward<T>(arg));
		processArgs(std::forward<Rest>(rest)...);
	}


	std::vector<fs::path> mv_audio_pool;
	std::vector<fs::path> mv_audio_pool_cache;
	std::vector<fs::path> mv_input_pool;
	fs::path m_default_output;//输出文件的路径，可设置，不设置为当前目录
	size_t m_num_of_material = 0;//待处理音频的数量，也是通道数
	std::vector<std::ifstream> v_ifs_handle; // 用于管理 ifstream 对象的 vector 容器

	std::mutex m_work_mutex;//
};


/*
用于音频拆分的类，暂时支持单个文件的切分。

最终功能：
*/
class SplitAudio
{

public:
	struct FileItem {
		fs::path FilePath = "";
		unsigned int Chanel = 0;
		std::string FileFormat = ".pcm";
	};
	CORE_API ~SplitAudio();
	CORE_API SplitAudio(void);
	CORE_API SplitAudio(const FileItem files);

	SplitAudio(const SplitAudio&) = delete;
	SplitAudio(SplitAudio&&) = delete;
	SplitAudio& operator=(const SplitAudio&) = delete;
	SplitAudio& operator=(SplitAudio&&) = delete;

	CORE_API bool set_output_folder(const std::string& value);
	CORE_API void reset_output_folder();
	CORE_API bool add_audio(FileItem item);
	CORE_API void stop();
	CORE_API void clear_all();

private:
	void working_proc();
	void reset();

private:
	std::unique_ptr<char[]> m_readpcm_buffer_common = nullptr;//一个智能指针，用于最开始申请公用的读数据缓存空间
	std::vector<fs::path> mv_audio_pool;
	int m_input_chanel;//待输入音频的通道数
	std::vector<std::ofstream> v_ofs_handle; //用于管理 ofstream 对象的 vector 容器
	std::ifstream m_ifs;//输入文件描述符
	int m_chanel = 2;
	fs::path input_file;//当前在处理的文件名字
	fs::path output_folder = "";
	bool is_set_output_folder = false;
	
	std::queue<FileItem> m_msg_queue;//存放用户的音频数据(文件位置、音频路数、格式等)
	std::mutex m_msg_mutex;//为m_msg_queue队列设置的一个锁
	

	
	mutable std::mutex m_mutex;
	std::thread m_working_thread;
	std::atomic_bool m_thread_exit = false;
	std::atomic_bool m_running = false;
	std::condition_variable m_msg_cond;//在条件满足时执行，而不是一直忙等待
};








class FindAudioPosition
{
public:
	~FindAudioPosition() = default;
	FindAudioPosition() = delete;
	FindAudioPosition(const fs::path short_path, const fs::path long_path);
	FindAudioPosition(const FindAudioPosition&) = delete;
	FindAudioPosition(FindAudioPosition&&) = delete;
	FindAudioPosition& operator=(const FindAudioPosition&) = delete;
	FindAudioPosition& operator=(FindAudioPosition&&) = delete;

	std::string get_shortaudio_position_str();
	bool set_long_audio_path(fs::path long_path);
	bool set_short_audio_path(fs::path short_path);


private:
	bool is_init_success = false;
	int findSubsequence(const short* longAudio, const short* shortAudio);

	std::unique_ptr<char[]> m_short_audio_buffer = nullptr;
	constexpr static int ShortAudioSize = 3 * 32000;//短音频只读取3秒
	int actual_shortaudio_length_read = 0;//实际读取到的短音频长度，一般小于等于ShortAudioSize

	std::ifstream ifs_long_audio;
	std::unique_ptr<char[]> m_long_audio_buffer = nullptr;
	constexpr static int LongAudioSize = 30 * 1024 * 1024;//长音频每次读取30mb
	int actual_longaudio_length_read = 0;//每次实际read长音频获得的字节
	long long_audio_total_size = 0;//长音频的总长度

	bool m_has_found = false;

	mutable std::mutex m_mutex;
	long get_shortaudio_position();//获取短音频在长音频的位置，返回值是毫秒，仅仅支持单通道
};


//切音频的类，目前实现单通道音频的切分，需要支持多通道
class CutOrLengthenAudio
{
public:
	CORE_API ~CutOrLengthenAudio();
	CutOrLengthenAudio() = delete;
	CORE_API CutOrLengthenAudio(const fs::path original_path, int chanel = 1);
	CutOrLengthenAudio(const CutOrLengthenAudio&) = delete;
	CutOrLengthenAudio(CutOrLengthenAudio&&) = delete;
	CutOrLengthenAudio& operator=(const CutOrLengthenAudio&) = delete;
	CutOrLengthenAudio& operator=(CutOrLengthenAudio&&) = delete;

	CORE_API bool cut_and_save(const std::string& cut_time_str);
	CORE_API bool cut_to_buffer(const std::string& cut_time_str, void* buffer_dst, unsigned long buffer_dst_size);
	//bool set_chanel(int chanel);


private:
	bool is_init_success = false;

	constexpr static int ReadAudioSize = 5 * 1024 * 1024;//音频每次读取5mb
	std::ifstream ifs_original_audio;
	std::ofstream ofs_output_audio;
	std::unique_ptr<char[]> m_original_audio_buffer = nullptr;
	long m_long_audio_total_size = 0;//长音频的总长度
	fs::path m_original_path = "";

	bool get_start_and_end_ms(const std::string& time_str, long& start_ms, long& end_ms);
	bool cut_op_main(const long start_byte, const long end_byte);
	void cut_op_main(const long start_byte, const long end_byte, char* dst_buffer,unsigned long dst_buffer_size);

	int audio_chanel = 1;
	int bitrate_ms = 1 * 16000 * 2 / 1000;//每1ms的字节数
};