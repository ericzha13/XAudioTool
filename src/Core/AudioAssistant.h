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
const int sampleSize = 2; // 16λΪ2�ֽ�
constexpr static int audio_readsize_once = 1 * 256 * sizeof(short) * 10000;//����Ƶ����buffer
/*
�������С����Ƶ

*/
using namespace std;

enum class ToolsMsg
{
	/* Task Info */
	TaskMvw = 10000,     // ������Ϣ
	TaskSr,              // ʶ����Ϣ
	TaskSe,              // SE��Ϣ
};

using ToolsCallback = std::function<void(ToolsMsg, const json::value&, void*)>;



//��Ƶת������pcmתwav��֧�ֵ���������
//������֧�ָ�����Ƶ��ʽ
class AudioAssistant {
public:
	//AudioAssistant(ToolsCallback callback, void* callback_arg);
	AudioAssistant();
	AudioAssistant(const std::string res);
	AudioAssistant(const AudioAssistant&) = delete;
	AudioAssistant(AudioAssistant&&) = delete;
	AudioAssistant& operator=(const AudioAssistant&) = delete;
	AudioAssistant& operator=(AudioAssistant&&) = delete;

	//��������
	bool wav_to_pcm();
	bool wav_to_pcm(const char* str);

	bool split_audio(const std::string&& audioPath, const int&& chanel);

	bool set_target_path_or_file(const string&& str);
	bool cut_audio_timepoint(const string&& audioPath, const int&& chanel);

	~AudioAssistant();

private:
	std::filesystem::path m_current_working_folder = "";//exe�ļ���ǰ·��������
	std::filesystem::path m_prehandled_file = "";//�������ļ����ƣ�һ�㱣�漯�ɷ������·��
	std::filesystem::path m_preoutput_file = "";//�������ļ����ƣ�һ�㱣�漯�ɷ������·��
	std::filesystem::path m_output_pcm_folder = "";//һ���ǲ����·��


	std::unique_ptr<char[]> m_readpcm_buffer_common = nullptr;//һ������ָ�룬�����ʼ���빫�õĶ����ݻ���ռ�
	mutable std::mutex m_readpcm_buffer_mutex;//m_readpcm_buffer_common�������ñ�����Զ���ڿɱ�״̬
	std::unique_ptr<char[]> mp_tmp_input_cache[16];

	bool check_suffix(const std::string&& input_pcm_pcmfile);


};





//���ںϲ���Ƶ���࣬��ʼ����ʱ��֧�ִ�������Ƶ�ļ����ߴ�һ���ļ���
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

	void refilter_by_extension(const std::string ext);//������չ�������ļ�����Ĳ��Ǹ���չ�����ļ��޳�����
	bool start_merge();//��ʼ�ϲ���
	bool reset_output_file(fs::path new_output_file);


	//����������������Ƶ�����ϲ�
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
	fs::path m_default_output;//����ļ���·���������ã�������Ϊ��ǰĿ¼
	int m_num_of_material;//��������Ƶ��������Ҳ��ͨ����
	std::vector<std::ifstream> v_ifs_handle; // ���ڹ��� ifstream ����� vector ����
	ofstream m_ofs;//����ļ�������
};


/*
������Ƶ��ֵ��࣬��ʱ֧�ֵ����ļ����з֡�

���չ��ܣ�
��  ����һ���ļ��У�Ѱ�Ҹ��ļ��������е���Ƶ����ÿ����Ƶ�ļ����в�֡�
	��ֽ��ֱ�ӷŵ���ǰĿ¼���������ͬ���Ƶ���ǿ��ɾ����
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

	void refilter_by_extension(const std::string ext);//������չ�������ļ�����Ĳ��Ǹ���չ�����ļ��޳�����
	bool start_split();//��ʼ�зְ�


private:
	void deinit();

	std::unique_ptr<char[]> m_readpcm_buffer_common = nullptr;//һ������ָ�룬�����ʼ���빫�õĶ����ݻ���ռ�
	std::vector<fs::path> m_audio_pool;
	int m_input_chanel;//��������Ƶ������
	std::vector<std::ofstream> v_ofs_handle; // ���ڹ��� ofstream ����� vector ����
	ifstream m_ifs;//�����ļ�������
	
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

	long get_shortaudio_position();//��ȡ����Ƶ�ڳ���Ƶ��λ�ã�����ֵ�Ǻ��룬����֧�ֵ�ͨ��
	string get_shortaudio_position_str();
	

private:
	bool is_init_success = false;
	int findSubsequence(const short* longAudio, const short* shortAudio);

	std::unique_ptr<char[]> m_short_audio_buffer = nullptr;
	constexpr static int ShortAudioSize = 3 * 32000;//����Ƶֻ��ȡ3��
	int actual_shortaudio_length_read = 0;//ʵ�ʶ�ȡ���Ķ���Ƶ���ȣ�һ��С�ڵ���ShortAudioSize

	ifstream ifs_long_audio;
	std::unique_ptr<char[]> m_long_audio_buffer = nullptr;
	constexpr static int LongAudioSize = 30 * 1024 * 1024;//����Ƶÿ�ζ�ȡ30mb
	int actual_longaudio_length_read = 0;//ÿ��ʵ��read����Ƶ��õ��ֽ�
	long long_audio_total_size = 0;//����Ƶ���ܳ���

	bool m_has_found = false;
};


//����Ƶ���࣬Ŀǰʵ�ֵ�ͨ����Ƶ���з֣���ͨ����δʵ��
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

	constexpr static int ReadAudioSize = 5 * 1024 * 1024;//��Ƶÿ�ζ�ȡ5mb
	ifstream ifs_original_audio;
	ofstream ofs_output_audio;
	std::unique_ptr<char[]> m_original_audio_buffer = nullptr;
	long m_long_audio_total_size = 0;//����Ƶ���ܳ���
	
	bool get_start_and_end_ms(const std::string& time_str,long& start_ms,long& end_ms);
	bool cut_op_main(const long start_byte, const long end_byte);

	int audio_chanel = 1;
};



