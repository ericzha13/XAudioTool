#include "AudioAssistant.h"

 
//ToolsCallback callback, void* callback_arg
AudioAssistant::AudioAssistant()
{
	//产生赋值和一些初始化
	m_current_working_folder = fs::current_path();
	m_output_pcm_folder = m_current_working_folder / "output";//有些功能需要输出大量音频，放到同一级路径不合适，此处放到同一级目录的out文件夹下

	//if (!filesystem::is_directory(m_output_pcm_folder / "out")) {
	//	filesystem::create_directory(m_output_pcm_folder / "out");
	//}

	//分配5.12mb的空间,用于通用的读文件缓存buffer
	m_readpcm_buffer_common = std::make_unique<char[]>(audio_readsize_once + 1);

	for (int i = 0; i < 16; i++) {
		mp_tmp_input_cache[i] = std::make_unique<char[]>(2 * 1024 * 1024);
	}
}

//传入默认路径或默认待处理的文件
AudioAssistant::AudioAssistant(const std::string res)
{
	if (fs::is_directory(res)) {
		m_current_working_folder = res;
		m_output_pcm_folder = m_current_working_folder / "output";//有些功能需要输出大量音频，放到同一级路径不合适，此处放到同一级目录的out文件夹下
		ast::utils::create_format_directory(m_current_working_folder, "output");
	}
	else if (fs::is_regular_file(res))
	{
		m_prehandled_file = res;
	}
	else {
		m_current_working_folder = fs::current_path();
		m_output_pcm_folder = m_current_working_folder / "output";//有些功能需要输出大量音频，放到同一级路径不合适，此处放到同一级目录的out文件夹下
	}

	m_readpcm_buffer_common = std::make_unique<char[]>(audio_readsize_once + 1);


	//申请中间缓存空间，用于读文件
	for (int i = 0; i < 16; i++) {
		mp_tmp_input_cache[i] = std::make_unique<char []>(2*1024*1024);
	}
}



AudioAssistant::~AudioAssistant()
{

}







//完善
bool AudioAssistant::wav_to_pcm(const char* str)
{
	//头部数据和输入有效性检测,如果输入不是wav格式头部数据的音频,则不操作
	if (!check_suffix(str)) { return false; }


	std::ifstream ifs(str, std::ios_base::in | std::ios_base::binary);
	ifs.seekg(44, std::ios_base::beg);


	fs::path tmp_path = str;
	std::ofstream m_ofs(tmp_path.parent_path() / (tmp_path.stem().string() + ".pcm"), std::ios::binary | std::ios::out);
	std::unique_lock<std::mutex> lock(m_readpcm_buffer_mutex);
	while (ifs.peek() != EOF) {
		//每次读取5.12MB
		memset(m_readpcm_buffer_common.get(), 0, audio_readsize_once);
		size_t read_size = ifs.read(m_readpcm_buffer_common.get(), audio_readsize_once).gcount();

		if (read_size != audio_readsize_once) {
			m_ofs.write(m_readpcm_buffer_common.get(), read_size);
			break;
		}

		m_ofs.write(m_readpcm_buffer_common.get(), audio_readsize_once);
	}

	std::cout << tmp_path.string() << "转为pcm格式成功" << std::endl;
	m_ofs.close();
	ifs.close();



	return true;
}

//使用set_target_path_or_file设置的文件来转换
bool AudioAssistant::wav_to_pcm()
{
	return wav_to_pcm(m_prehandled_file.string().c_str());
}


//判断文件后缀名是否和实际二进制头部数据一致，防止有些音频文件虽然是.wav结尾，但是本质是pcm格式
bool AudioAssistant::check_suffix(const std::string&& input_pcm_pcmfile)
{
	if (!fs::is_regular_file(input_pcm_pcmfile))
	{
		printf("ERR输入音频[%s]不存在或输入为空，或不是常规文件\n", input_pcm_pcmfile.c_str());
		return false;
	}
	else if (fs::file_size(input_pcm_pcmfile) < 44)
	{
		printf("输入文件%s大小不足44字节，函数返回\n", input_pcm_pcmfile.c_str());
		return false;
	}

	fs::path tmp_path = input_pcm_pcmfile;
	bool is_wav_fuffix = false;

	if (tmp_path.extension().string() == ".wav" ||
		tmp_path.extension().string() == ".WAV")
	{
		is_wav_fuffix = true;
	}
	else if (tmp_path.extension().string() == ".pcm" ||
		tmp_path.extension().string() == ".PCM")
	{
		is_wav_fuffix = false;
	}
	else
	{
		printf("输入文件不是pcm也不是wav，函数返回\n");
		return false;
	}

	//wav格式的3次ID检测0~3  12~15  36~39
	//wav格式，开头不是RIFF或者.  非wav格式,开头却是RIFF会返回错误
	//wav格式，开头是RIFF.       非wav格式,开头不是RIFF会返回正确
	std::ifstream ifs(input_pcm_pcmfile, std::ios_base::in | std::ios_base::binary);
	if (!ifs.is_open()) {
		std::cerr << input_pcm_pcmfile << "在check_suffix函数中打开失败" << std::endl;
		return false;
	}
	char tmp[5] = { 0 };
	ifs.read(tmp, 4);
	if ((strcmp(tmp, "RIFF") != 0 && is_wav_fuffix == true) ||
		(strcmp(tmp, "RIFF") == 0 && is_wav_fuffix == false))
	{
		ifs.close(); return false;
	}

	ifs.seekg(12, std::ios_base::beg);
	memset(tmp, 0, 5);
	ifs.read(tmp, 4);
	if ((strcmp(tmp, "fmt ") != 0 && is_wav_fuffix == true) ||
		(strcmp(tmp, "fmt ") == 0 && is_wav_fuffix == false))
	{
		ifs.close(); return false;
	}

	ifs.seekg(36, std::ios_base::beg);
	memset(tmp, 0, 5);
	ifs.read(tmp, 4);
	if ((strcmp(tmp, "data") != 0 && is_wav_fuffix == true) ||
		(strcmp(tmp, "data") == 0 && is_wav_fuffix == false))
	{
		ifs.close(); return false;
	}

	ifs.close();
	return true;
}


//设置路径，支持string或者path类型并保存变量，其他类型不支持，废弃
bool AudioAssistant::set_target_path_or_file(const std::string&& str)
{
	if (fs::is_directory(str))
	{
		m_current_working_folder = str;
		m_output_pcm_folder = m_current_working_folder / "output";
		ast::utils::create_format_directory(m_current_working_folder, "output");
	}
	else if (fs::is_regular_file(str))
	{
		m_prehandled_file = str;
		m_preoutput_file = ast::utils::create_format_directory<true>(m_prehandled_file, "output", m_prehandled_file.stem().string() + "_out.pcm");
		if (m_preoutput_file == "")std::cerr << "输出文件设置异常，->set_target_path_or_file" << std::endl;
	}
	else {
		std::cerr << "set_working_path路径异常，非常规文件，非文件夹" << std::endl;
		return false;
	}

	return true;

}




//把音频拆分成N个通道
bool AudioAssistant::split_audio(const std::string&& audioPath, const int&& chanel)
{

	if (chanel > 16 || chanel <= 1) { std::cout << "待拆分音频通道数异常" << std::endl; return false; }
	fs::path input_name_pcm_file = audioPath;
	if (!fs::is_regular_file(input_name_pcm_file)) { std::cerr << input_name_pcm_file << "不是一个文件或文件不存在" << audioPath << std::endl;  return false; }


	//打开输入文件
	std::ifstream m_ifs(input_name_pcm_file, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
	long long m_file_size = m_ifs.tellg();
	m_ifs.seekg(0, std::ios_base::beg);
	std::cout << "输入文件名：" << input_name_pcm_file << "文件大小：" << m_file_size << std::endl;
	if (m_file_size % (sizeof(short) * chanel) != 0) {
		std::cout << "待拆分输入音频无法做到N个通道均分，输出音频会存在长度不一致" << std::endl;
	}


	//打开输出文件
	std::unique_ptr<fs::path[]> output_pcmfile = std::make_unique<fs::path[]>(chanel);//所有输出文件的绝对路径
	std::unique_ptr<std::ofstream[]> m_ofs = std::make_unique<std::ofstream[]>(chanel);//所有输出文件的文件描述符

	auto target_path = ast::utils::create_format_directory(input_name_pcm_file.parent_path(), "output");;
	for (size_t i = 0; i < chanel; i++) {
		output_pcmfile[i] = fs::path(target_path) / (input_name_pcm_file.stem().string() + "_" + std::to_string(i + 1) + ".pcm");
		m_ofs[i].open(output_pcmfile[i], std::ios_base::out | std::ios_base::binary);
		printf("输出文件 = %s\n", output_pcmfile[i].string().c_str());
	}

	//处理音频
	{
		std::unique_lock<std::mutex> lock(m_readpcm_buffer_mutex);
		while (m_ifs.peek() != EOF) {

			memset(m_readpcm_buffer_common.get(), 0, audio_readsize_once);
			size_t real_size = m_ifs.read(m_readpcm_buffer_common.get(), audio_readsize_once).gcount();//读取的数据小于5120000 byte。

			for (int sample = 0; sample < real_size / 2; sample++) {
				m_ofs[sample % chanel].write((char*)((short*)m_readpcm_buffer_common.get() + sample), sizeof(short));
			}
		}
	}


	std::cout << "拆分完成，结果已输出到" << output_pcmfile[0].parent_path() << std::endl;
	//结束关闭文件
	for (size_t i = 0; i < chanel; i++) {
		m_ofs[i].close();
	}
	m_ifs.close();
	return 0;
}

SplitAudio::SplitAudio(const FileItem files)
{
	
	if (fs::is_regular_file(files.FilePath))
	{
		if (files.Chanel > 16 || files.Chanel <= 1) { throw "待拆分音频通道数异常"; }
		m_msg_queue.emplace(files);
		
		//申请缓存
		{
			m_readpcm_buffer_common = std::make_unique<char[]>(audio_readsize_once);
			if (!m_readpcm_buffer_common) {
				throw "m_readpcm_buffer_common buffer allocated failed";
			}
		}

		m_working_thread = std::thread(&SplitAudio::working_proc, this);
	}
	else {
		std::cerr << "The input parameter is not a regular file" << std::endl;
	}
}

SplitAudio::SplitAudio(void)
{
	//申请缓存
	{
		m_readpcm_buffer_common = std::make_unique<char[]>(audio_readsize_once);
		if (!m_readpcm_buffer_common) {
			throw "m_readpcm_buffer_common buffer allocated failed";
		}
	}
	m_working_thread = std::thread(&SplitAudio::working_proc, this);
}

//设置一些参数，当工作线程里有数据即上次的音频还未处理完时会返回失败。
bool SplitAudio::set_output_folder(const std::string& value)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (fs::is_directory(value)) {
		output_folder = value;
		is_set_output_folder = true;
		LOG(INFO) << "outputFolder set to " << value << " success";
		return true;
	}
	LOG(WARNING) << "outputFolder set to " << value << " failed, input is not a folder";

	return false;
}

void SplitAudio::reset_output_folder(void) 
{
	std::unique_lock<std::mutex> lock(m_mutex);
	is_set_output_folder = false;
	LOG(INFO) << "outputFolder set to null";
}

bool SplitAudio::add_audio(FileItem item)
{
	LogTraceFunction;
	if (!fs::is_regular_file(item.FilePath) || item.Chanel == 0 || item.FileFormat == "") {
		LOG(ERROR) << item.FilePath.string() << " is not a regular file or other illegal input.";
		return false;
	}

	{
		std::unique_lock<std::mutex> lock(m_msg_mutex);
		m_msg_queue.emplace(std::move(item));
		m_msg_cond.notify_one();
	}
	return true;
}


void SplitAudio::working_proc()
{
	LogTraceFunction;
	while (true) {
		LOG(INFO) << "new one";
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_thread_exit == true) puts("666");
		if (m_thread_exit) {
			m_running = false;
			return;
		}


		m_running = true;
		{
			std::unique_lock<std::mutex> lock(m_msg_mutex);
			if (m_msg_queue.empty()) {
				m_msg_cond.wait(lock);//释放lock，并进入等待
				continue;
			}
		}

		//get one item from msg_queue
		input_file = "";
		m_chanel = 0;
		{
			std::unique_lock<std::mutex> lock(m_msg_mutex);
			auto msg_item = std::move(m_msg_queue.front());
			input_file = msg_item.FilePath;
			m_chanel = msg_item.Chanel;
			m_msg_queue.pop();
		}
		

		{
			//开始工作
			m_ifs.open(input_file, std::ios_base::in | std::ios_base::binary);
			if (!m_ifs.is_open()) { LOG(ERROR) << "file: " << input_file << "open failed"; continue; }

			//创建输出文件并打开
			bool is_open_outputfile_success = true;
			if (output_folder == "" || is_set_output_folder == false) { 
				output_folder = input_file.parent_path(); 
				LOG(WARNING) << "output_folder is automatically set to " << output_folder;
			}
			for (int i = 0; i < m_chanel; i++)
			{
				std::string m_tmp = ast::utils::create_format_directory<true>(output_folder , input_file.stem().string(), "splitOut", "spout" + std::to_string(i) + ".pcm");
				if (m_tmp != "") { mv_audio_pool.emplace_back(m_tmp); }
				else { LOG(ERROR) << "some outputfile create failed , it shouldn't happen"; is_open_outputfile_success = false;}
			}
			if (!is_open_outputfile_success) {
				reset();
				continue;
			}

			m_input_chanel = mv_audio_pool.size();
			if (ast::utils::open_files(mv_audio_pool, v_ofs_handle) != 0) { 
				LOG(ERROR) << "some outputfile open failed ,it shouldn't happen";
				reset();
				continue;
			}

			//处理音频
			m_ifs.seekg(0, std::ios_base::beg);
			{
				while (m_ifs.peek() != EOF) {

					memset(m_readpcm_buffer_common.get(), 0, audio_readsize_once);
					size_t real_size = m_ifs.read(m_readpcm_buffer_common.get(), audio_readsize_once).gcount();//读取的数据小于5120000 byte。

					for (long sample = 0; sample < real_size / 2; sample++) {
						v_ofs_handle[sample % m_input_chanel].write((char*)((short*)m_readpcm_buffer_common.get() + sample), sizeof(short));
					}
				}
			}
			reset();
			LOG(INFO) << "split file " << input_file << " success~";
		}
	}
}

void SplitAudio::reset()
{
	m_ifs.close();
	for (auto it = 0;it < v_ofs_handle.size(); it++) {
		v_ofs_handle.at(it).close();
	}
	mv_audio_pool.clear();
	v_ofs_handle.clear();
	m_input_chanel = 0;
	output_folder = "";
}

void SplitAudio::clear_all()
{
	std::unique_lock<std::mutex> lock(m_msg_mutex);
	//和空队列交换，实现清空
	if (m_msg_queue.empty()) {
		std::queue<FileItem> t;
		m_msg_queue.swap(t);
	}
}

void SplitAudio::stop()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	
}

SplitAudio::~SplitAudio()
{
	m_thread_exit = true;

	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_msg_cond.notify_all();
	}

	if (m_working_thread.joinable()) {
		m_working_thread.join();
	}
}

//未完善
bool AudioAssistant::cut_audio_timepoint(const std::string&& audioPath, const int&& chanel) {

	long m_start = 0, m_end = 0;
	set_target_path_or_file(forward<const std::string>(audioPath));
	std::string pcm_file = "";
	std::filesystem::path intput_name_of_pcm_file_p = audioPath;
	std::filesystem::path output_name_of_pcm_file_p = "";


	//33分10秒到33分30秒
	m_start = 32000 * 1990 + 32 * 0;//秒+微秒
	m_end = 32000 * 2010 + 32 * 0;//秒+微秒


	printf("输入8通道音频\n");
	std::getline(std::cin, pcm_file);
	intput_name_of_pcm_file_p = pcm_file;//C:\Users\zmzha\Music\locate.pcm
	output_name_of_pcm_file_p = intput_name_of_pcm_file_p.parent_path() / (intput_name_of_pcm_file_p.stem().string() + "_cut.pcm");


	if (!std::filesystem::exists(intput_name_of_pcm_file_p) || !std::filesystem::is_regular_file(intput_name_of_pcm_file_p)) {
		printf("%s 不存在或者不是一个文件", intput_name_of_pcm_file_p.string().c_str());
		return -1;
	}


	printf("输出文件的位置%s\n", output_name_of_pcm_file_p.string().c_str());
	std::ifstream i_fs(intput_name_of_pcm_file_p, std::ios_base::in | std::ios_base::binary);
	std::ofstream o_fs(output_name_of_pcm_file_p, std::ios_base::out | std::ios_base::binary);//C:\Users\zmzha\Music\locate_cut.pcm

#define INPUT_CHANEL 8
	i_fs.seekg(m_start * INPUT_CHANEL, std::ios::beg);//设置读指针位置相对起始位置偏移  m_start字节

	short s1[INPUT_CHANEL] = { 0 };


	while (1) {
		if (i_fs.tellg() > m_end * INPUT_CHANEL) {
			puts("已处理到尾端点");
			break;
		}

		i_fs.read((char*)s1, sizeof(s1));
		o_fs.write((char*)s1, sizeof(s1));
	}

	//if (i_fs) i_fs.close();
	//if (o_fs) o_fs.close();
	i_fs.close();
	o_fs.close();
	return 0;
}


//获取长短音频的位置，返回值是毫秒
long FindAudioPosition::get_shortaudio_position()
{
	LogTraceFunction;
	if (!is_init_success) return -1;
	long accumulate_byte = 0;
	long t = 0;
	for (long cur = 0; cur < long_audio_total_size; cur+= actual_longaudio_length_read)
	{
		actual_longaudio_length_read = ifs_long_audio.read(m_long_audio_buffer.get(), LongAudioSize).gcount();
		ifs_long_audio.seekg(-actual_shortaudio_length_read,std::ios_base::cur);
		t = findSubsequence(std::move(reinterpret_cast<short*>(m_long_audio_buffer.get())), 
							std::move(reinterpret_cast<short*>(m_short_audio_buffer.get())));
		if (t > 0) {
			accumulate_byte += (t * 2);
			m_has_found = true;
			return accumulate_byte / 32;
		}
		else
		{
			accumulate_byte += (actual_longaudio_length_read- actual_shortaudio_length_read);
			m_has_found = false;
		}
	}
	return accumulate_byte/32;
}


std::string FindAudioPosition::get_shortaudio_position_str()
{
	LogTraceFunction;
	long posi_ms = get_shortaudio_position();


	if (posi_ms == -1) return "is_init failed";
	if (!m_has_found) return "can not find object in target audio";
	char buffer[64] = {0};

#ifdef _WIN32
	sprintf_s(buffer, "%02d:%02d:%02d.%03d", (posi_ms / 1000) / 3600, (posi_ms / 1000) % 3600 / 60, (posi_ms / 1000) % 3600 % 60, posi_ms%1000); //将毫秒数转换为字符串
#else
	snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", (posi_ms / 1000) / 3600, (posi_ms / 1000) % 3600 / 60, (posi_ms / 1000) % 3600 % 60, posi_ms % 1000);
#endif

	return buffer;
}

// 使用KMP算法在长音频中查找短音频的位置
int FindAudioPosition::findSubsequence(const short* longAudio, const short* shortAudio) 
{

	int n = actual_longaudio_length_read / 2;
	int m = actual_shortaudio_length_read / 2;


	// 构建next数组
	std::vector<int> next(m, 0);
	for (int i = 1, j = 0; i < m; i++) {
		while (j > 0 && shortAudio[i] != shortAudio[j]) {
			j = next[j - 1];
		}
		if (shortAudio[i] == shortAudio[j]) {
			j++;
		}
		next[i] = j;
	}

	// 使用next数组匹配长音频和短音频
	for (int i = 0, j = 0; i < n; i++) {
		while (j > 0 && longAudio[i] != shortAudio[j]) {
			j = next[j - 1];
		}
		if (longAudio[i] == shortAudio[j]) {
			j++;
		}
		if (j == m) {
			return i - m + 1;
		}
	}

	return -1; // 找不到
}


FindAudioPosition::FindAudioPosition(const fs::path short_path, const fs::path long_path)
{
	LogTraceFunction;
	std::unique_lock<std::mutex> lock(m_mutex);
	//内存申请
	{
		//直接读取整条短音频。
		m_short_audio_buffer = std::make_unique<char[]>(ShortAudioSize);
		m_long_audio_buffer = std::make_unique<char[]>(LongAudioSize);
		if (!m_short_audio_buffer || !m_long_audio_buffer) {
			LOG(ERROR) << "m_short_audio_buffer buffer allocated failed";
			throw "m_short_audio_buffer buffer allocated failed";
		}
	}
	
	//打开相关音频文件，并记录一些音频大小等信息
	{
		if (!fs::is_regular_file(short_path) || !fs::is_regular_file(long_path))
		{
			LOG(ERROR) << "input file is not exist";
			throw "input file is not exist";
		}

		//打开长短音频
		std::ifstream ifs_short_audio(short_path, std::ios_base::in | std::ios_base::binary);
		ifs_long_audio.open(long_path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
		if (!ifs_short_audio.is_open() || !ifs_long_audio.is_open()) {
			LOG(ERROR) << "audio open failed";
			throw "audio open failed";
		}

		actual_shortaudio_length_read = ifs_short_audio.read(m_short_audio_buffer.get(), ShortAudioSize).gcount();
		long_audio_total_size = ifs_long_audio.tellg();
		ifs_long_audio.seekg(0, std::ios_base::beg);
		ifs_short_audio.close();
	}


	is_init_success = true;
}

bool FindAudioPosition::set_long_audio_path(fs::path long_path)
{
	LogTraceFunction;
	if (!fs::is_regular_file(long_path))
	{
		LOG(ERROR) << long_path << "is not a regular file";
		return false;
	}

	{
		std::unique_lock<std::mutex> lock(m_mutex);
		ifs_long_audio.open(long_path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
		if (!ifs_long_audio.is_open()) {
			LOG(ERROR) << long_path << " open failed";
			return false;
		}
		long_audio_total_size = ifs_long_audio.tellg();
		ifs_long_audio.seekg(0, std::ios_base::beg);
	}

	return true;
}

bool FindAudioPosition::set_short_audio_path(fs::path short_path)
{
	LogTraceFunction;
	if (!fs::is_regular_file(short_path))
	{
		LOG(ERROR) << short_path << "is not a regular file";
		return false;
	}

	{
		std::unique_lock<std::mutex> lock(m_mutex);
		std::ifstream ifs_short_audio(short_path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
		if (!ifs_short_audio.is_open()) {
			LOG(ERROR) << short_path << " open failed";
			return false;
		}

		std::fill(m_short_audio_buffer.get(), m_short_audio_buffer.get() + ShortAudioSize, 0);
		actual_shortaudio_length_read = ifs_short_audio.read(m_short_audio_buffer.get(), ShortAudioSize).gcount();
		ifs_short_audio.close();
	}

	return true;
}

CutOrLengthenAudio::CutOrLengthenAudio(const fs::path original_path, const int chanel)
{

	//输入判断
	{
		if (!fs::is_regular_file(original_path))
		{
			throw "input file is not exist[FindAudioPosition::FindAudioPosition]";
		}
		
		if (chanel < 1)
		{
			throw "input file chanel invalid[FindAudioPosition::FindAudioPosition]";
		}
		audio_chanel = chanel;

		m_original_path = original_path;
	}

	//open input and target output file and get some file information
	{
		ifs_original_audio.open(original_path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
		if (!ifs_original_audio.is_open()) {
			throw "audio open failed";
		}
		m_long_audio_total_size = ifs_original_audio.tellg();
	}

	//Request memory
	{
		m_original_audio_buffer = std::make_unique<char[]>(ReadAudioSize * audio_chanel);
		if (!m_original_audio_buffer) {
			throw "m_original_audio_buffer buffer allocated failed";
		}
	}
}


//输出文件名变更和时间相关
bool CutOrLengthenAudio::cut_and_save(const std::string& cut_time_str)
{
	long start_byte = 0, end_byte = 0;
	if (!get_start_and_end_ms(cut_time_str, start_byte, end_byte)) {
		LOG(ERROR) << "Invalid time format,get start byte and end byte from string failed";
		return false;
	}

	if (m_long_audio_total_size < audio_chanel * end_byte * bitrate_ms
		|| start_byte > end_byte
		|| m_long_audio_total_size < audio_chanel * start_byte * bitrate_ms)
	{
		LOG(ERROR) << "The length of the audio to be captured exceeds the original audio length";
		return false;
	}

	ofs_output_audio.open(m_original_path.parent_path() / (m_original_path.stem().string() + "_cut_output_audio.pcm"), std::ios_base::out | std::ios_base::binary);
	
	if (!ofs_output_audio.is_open()) {
		fs::path temp_out = m_original_path.parent_path() / (m_original_path.stem().string() + "_cut_output_audio.pcm");
		LOG(ERROR) << temp_out << " open failed, please check out the outputfile";
		return false;
	}
	cut_op_main(start_byte * bitrate_ms, end_byte * bitrate_ms);
	ofs_output_audio.close();
	

	return true;
}

bool CutOrLengthenAudio::cut_to_buffer(const std::string& cut_time_str, void* buffer_dst,unsigned long buffer_dst_size)
{
	long start_byte = 0, end_byte = 0;
	if (!get_start_and_end_ms(cut_time_str, start_byte, end_byte)) {
		LOG(ERROR) << "Invalid time format";
		return false;
	}

	if (m_long_audio_total_size < audio_chanel * end_byte * bitrate_ms
		|| start_byte > end_byte 
		|| m_long_audio_total_size < audio_chanel * start_byte * bitrate_ms
		|| buffer_dst_size < audio_chanel*(end_byte - start_byte) * bitrate_ms)//用户数组如果无法容纳指定的长度，则直接报错，不处理。
	{
		LOG(ERROR) << "Invalid time_byte";
		return false;
	}

	cut_op_main(start_byte * bitrate_ms, end_byte * bitrate_ms, static_cast<char*>(buffer_dst), buffer_dst_size);
	return true;
}


//input:  hh:mm:ss.xxx-hh:mm:ss.xxx
//output: convert start and end string time to milliseconds
bool CutOrLengthenAudio::get_start_and_end_ms(const std::string& time_str, long& start_ms, long& end_ms)
{

	std::stringstream ss(time_str);
	std::string start_time_str, end_time_str;
	std::getline(ss, start_time_str, '-');
	std::getline(ss, end_time_str);

	if (!ast::utils::check_time_format(end_time_str) || !ast::utils::check_time_format(end_time_str))
	{
		return false;
	}
	
	start_ms = ast::utils::parse_time(start_time_str);
	end_ms   = ast::utils::parse_time(end_time_str);
	return true;
}


bool CutOrLengthenAudio::cut_op_main(const long start_byte,const long end_byte)
{
	long length = audio_chanel*(end_byte - start_byte);


	ifs_original_audio.seekg(audio_chanel * start_byte, std::ios_base::beg);
	unsigned long size_has_been_writed = 0;
	while (length > size_has_been_writed) {
		int real_read_size = 0;
		real_read_size = ifs_original_audio.read(m_original_audio_buffer.get(), ReadAudioSize * audio_chanel).gcount();

		if (length > real_read_size + size_has_been_writed)
		{
			ofs_output_audio.write(m_original_audio_buffer.get(), real_read_size);
		}
		else
		{
			ofs_output_audio.write(m_original_audio_buffer.get(), length - size_has_been_writed);
			break;
		}
	}

	ofs_output_audio.close();
	return true;
}

void CutOrLengthenAudio::cut_op_main(const long start_byte, const long end_byte, char* dst_buffer, unsigned long dst_buffer_size)
{

	long length = audio_chanel * (end_byte - start_byte);//待截取音频实际的字节大小
	unsigned long size_has_been_readed = 0;


	ifs_original_audio.seekg(start_byte * audio_chanel, std::ios_base::beg);

	while (size_has_been_readed < length) {

			int real_read_size = 0; 
			real_read_size = ifs_original_audio.read(m_original_audio_buffer.get(), ReadAudioSize* audio_chanel).gcount();
			if (length > real_read_size + size_has_been_readed 
				&& dst_buffer_size > real_read_size + size_has_been_readed) {
				//本次读取数据后，仍然没有取完待剪切音频长度，且此时用户数组空间能够容纳下所有所读的数据->读到多少数据，写入多少
				std::memcpy(dst_buffer + size_has_been_readed, m_original_audio_buffer.get(), real_read_size);
				size_has_been_readed += real_read_size;
			}
			else if (length < real_read_size + size_has_been_readed) {
				//本次读入的数据已经超过了需要截取的音频长度->填充数据到length长度
				std::memcpy(dst_buffer + size_has_been_readed, m_original_audio_buffer.get(), length - size_has_been_readed);
				break;
			}
			
	}
}

CutOrLengthenAudio::~CutOrLengthenAudio()
{
	ifs_original_audio.close();
	ofs_output_audio.close();
}



bool MergeAudio::start_merge() {

	LogTraceFunction;
	std::unique_lock<std::mutex> lock(m_work_mutex);
	CHECK_ERROR_RETURN((m_num_of_material > 16 || m_num_of_material < 1),"unsupported chanel[2~16], merge failed!",false);
	CHECK_WARNING(m_num_of_material == 1,"There is only one audio that has been processed, now it will be ignored");
	
	
	
	//申请n块临时内存，用于存放输入缓存
	std::vector<std::unique_ptr<char[]>> v_tmp_buffer(m_num_of_material);
	{
		try {
			for (auto& buffer : v_tmp_buffer) {
				buffer = std::make_unique<char[]>(audio_readsize_once);
			}
		}
		catch (const std::bad_alloc& e) {
			LOG(ERROR) << "Failed to allocate memory,<<" << e.what();
			return false;
		}
	}

	std::ofstream m_ofs;//输出文件描述符
	//打开输入输出文件
	{
		OPEN_ONE_FILE(m_ofs, m_default_output);//打开输出文件
		if (ast::utils::open_files(mv_audio_pool, v_ifs_handle))//打开所有输入文件，若失败则关闭之前所有的文件
		{
			LOG(ERROR) << "open input file failed ";
			m_ofs.close();
			return false;
		}
	}



	//读取5.12mb文件到内存，再从内存逐个操作。
	long long min_real_read_size = audio_readsize_once;//记录读取到的多个文件中，最小的那一个。
	while (min_real_read_size == audio_readsize_once)
	{
		
		for (auto road = 0; road < m_num_of_material; road++)
		{
			long read_size = v_ifs_handle.at(road).read(v_tmp_buffer.at(road).get(), audio_readsize_once).gcount();
			if (read_size != audio_readsize_once) {
				min_real_read_size = read_size;
			}
		}

		for (int cur = 0; cur < min_real_read_size; cur += 2)
		{
			for (short num = 0; num < m_num_of_material; num++)
			{
				if (m_ofs.write(v_tmp_buffer.at(num).get() + cur, sampleSize).bad()) {
					// 写入失败
					LOG(ERROR) << "write output pcm failed";
				}
			}
		}

	}

	
	//处理完毕,释放资源
	deinit();
	m_ofs.close();
	LOG(INFO) << "merge success, file:" << m_default_output;
	return true;
}




bool MergeAudio::setparam(MergeAudioParam key, const std::string& value)
{
	LogTraceFunction;
	std::unique_lock<std::mutex> lock(m_work_mutex);
	switch (key)
	{
	case MergeAudioParam::ResetOutputFile: {
		fs::file_status status = fs::status(fs::path(value).parent_path()); // 获取文件状态信息
		if ((status.permissions() & fs::perms::owner_write) != fs::perms::none) {
			m_default_output = value;
			LOG(INFO) << "ResetOutputFile->" << value << " success~";
			return true;
		}
		LOG(ERROR) << "ResetOutputFile->" << value << " failed!";
	}
		break;
	case MergeAudioParam::SetAudioFormat: {
		CHECK_ERROR_THROW(value.empty(), "The input string si empty");
		CHECK_ERROR_THROW(value.at(0) != '.', "The input string does not start with . Beginning");
		mv_audio_pool.clear();
		std::copy(mv_audio_pool_cache.begin(), mv_audio_pool_cache.end(), std::back_inserter(mv_audio_pool));

		LOG(INFO) << "before SetAudioFormat: mv_audio_pool.size()=" << mv_audio_pool.size();
		mv_audio_pool.erase(
			std::remove_if(mv_audio_pool.begin(), mv_audio_pool.end(), [&](const fs::path& p) {return p.extension() != value; })
			, mv_audio_pool.end());

		m_num_of_material = mv_audio_pool.size();
		LOG(INFO) << "after SetAudioFormat: mv_audio_pool.size()=" << mv_audio_pool.size();
	}
		break;
	default:
		break;
	}

	return false;
}

void MergeAudio::clear_audio() {
	LogTraceFunction;
	std::unique_lock<std::mutex> lock(m_work_mutex);

	mv_audio_pool_cache.clear();
	mv_audio_pool.clear();
	mv_input_pool.clear();
	m_num_of_material = 0;
}
void MergeAudio::deinit()
{
	for (int i = 0; i < v_ifs_handle.size(); i++)
	{
		v_ifs_handle.at(i).close();
	}

	v_ifs_handle.clear();
}




