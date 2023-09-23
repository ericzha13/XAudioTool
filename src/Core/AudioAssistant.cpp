#include "AudioAssistant.h"


//ToolsCallback callback, void* callback_arg
AudioAssistant::AudioAssistant()
{
	//������ֵ��һЩ��ʼ��
	m_current_working_folder = filesystem::current_path();
	m_output_pcm_folder = m_current_working_folder / "output";//��Щ������Ҫ���������Ƶ���ŵ�ͬһ��·�������ʣ��˴��ŵ�ͬһ��Ŀ¼��out�ļ�����

	//if (!filesystem::is_directory(m_output_pcm_folder / "out")) {
	//	filesystem::create_directory(m_output_pcm_folder / "out");
	//}

	//����5.12mb�Ŀռ�,����ͨ�õĶ��ļ�����buffer
	m_readpcm_buffer_common = make_unique<char[]>(audio_readsize_once + 1);

	for (int i = 0; i < 16; i++) {
		mp_tmp_input_cache[i] = make_unique<char[]>(2 * 1024 * 1024);
	}
}

//����Ĭ��·����Ĭ�ϴ�������ļ�
AudioAssistant::AudioAssistant(const string res)
{
	if (filesystem::is_directory(res)) {
		m_current_working_folder = res;
		m_output_pcm_folder = m_current_working_folder / "output";//��Щ������Ҫ���������Ƶ���ŵ�ͬһ��·�������ʣ��˴��ŵ�ͬһ��Ŀ¼��out�ļ�����
		ast::utils::create_format_directory(m_current_working_folder, "output");
	}
	else if (filesystem::is_regular_file(res))
	{
		m_prehandled_file = res;
	}
	else {
		m_current_working_folder = filesystem::current_path();
		m_output_pcm_folder = m_current_working_folder / "output";//��Щ������Ҫ���������Ƶ���ŵ�ͬһ��·�������ʣ��˴��ŵ�ͬһ��Ŀ¼��out�ļ�����
	}

	m_readpcm_buffer_common = make_unique<char[]>(audio_readsize_once + 1);


	//�����м仺��ռ䣬���ڶ��ļ�
	for (int i = 0; i < 16; i++) {
		mp_tmp_input_cache[i] = make_unique<char []>(2*1024*1024);
	}
}



AudioAssistant::~AudioAssistant()
{

}







//����
bool AudioAssistant::wav_to_pcm(const char* str)
{
	//ͷ�����ݺ�������Ч�Լ��,������벻��wav��ʽͷ�����ݵ���Ƶ,�򲻲���
	if (!check_suffix(str)) { return false; }


	ifstream ifs(str, ios_base::in | ios_base::binary);
	ifs.seekg(44, ios_base::beg);


	filesystem::path tmp_path = str;
	ofstream m_ofs(tmp_path.parent_path() / (tmp_path.stem().string() + ".pcm"), ios::binary | ios::out);
	unique_lock<mutex> lock(m_readpcm_buffer_mutex);
	while (ifs.peek() != EOF) {
		//ÿ�ζ�ȡ5.12MB
		memset(m_readpcm_buffer_common.get(), 0, audio_readsize_once);
		size_t read_size = ifs.read(m_readpcm_buffer_common.get(), audio_readsize_once).gcount();

		if (read_size != audio_readsize_once) {
			m_ofs.write(m_readpcm_buffer_common.get(), read_size);
			break;
		}

		m_ofs.write(m_readpcm_buffer_common.get(), audio_readsize_once);
	}

	cout << tmp_path.string() << "תΪpcm��ʽ�ɹ�" << endl;
	m_ofs.close();
	ifs.close();



	return true;
}

//ʹ��set_target_path_or_file���õ��ļ���ת��
bool AudioAssistant::wav_to_pcm()
{
	return wav_to_pcm(m_prehandled_file.string().c_str());
}


//�ж��ļ���׺���Ƿ��ʵ�ʶ�����ͷ������һ�£���ֹ��Щ��Ƶ�ļ���Ȼ��.wav��β�����Ǳ�����pcm��ʽ
bool AudioAssistant::check_suffix(const string&& input_pcm_pcmfile)
{
	if (!filesystem::is_regular_file(input_pcm_pcmfile))
	{
		printf("ERR������Ƶ[%s]�����ڻ�����Ϊ�գ����ǳ����ļ�\n", input_pcm_pcmfile.c_str());
		return false;
	}
	else if (filesystem::file_size(input_pcm_pcmfile) < 44)
	{
		printf("�����ļ�%s��С����44�ֽڣ���������\n", input_pcm_pcmfile.c_str());
		return false;
	}

	filesystem::path tmp_path = input_pcm_pcmfile;
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
		printf("�����ļ�����pcmҲ����wav����������\n");
		return false;
	}

	//wav��ʽ��3��ID���0~3  12~15  36~39
	//wav��ʽ����ͷ����RIFF����.  ��wav��ʽ,��ͷȴ��RIFF�᷵�ش���
	//wav��ʽ����ͷ��RIFF.       ��wav��ʽ,��ͷ����RIFF�᷵����ȷ
	ifstream ifs(input_pcm_pcmfile, ios_base::in | ios_base::binary);
	if (!ifs.is_open()) {
		cerr << input_pcm_pcmfile << "��check_suffix�����д�ʧ��" << endl;
		return false;
	}
	char tmp[5] = { 0 };
	ifs.read(tmp, 4);
	if ((strcmp(tmp, "RIFF") != 0 && is_wav_fuffix == true) ||
		(strcmp(tmp, "RIFF") == 0 && is_wav_fuffix == false))
	{
		ifs.close(); return false;
	}

	ifs.seekg(12, ios_base::beg);
	memset(tmp, 0, 5);
	ifs.read(tmp, 4);
	if ((strcmp(tmp, "fmt ") != 0 && is_wav_fuffix == true) ||
		(strcmp(tmp, "fmt ") == 0 && is_wav_fuffix == false))
	{
		ifs.close(); return false;
	}

	ifs.seekg(36, ios_base::beg);
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


//����·����֧��string����path���Ͳ�����������������Ͳ�֧�֣�����
bool AudioAssistant::set_target_path_or_file(const string&& str)
{
	if (filesystem::is_directory(str))
	{
		m_current_working_folder = str;
		m_output_pcm_folder = m_current_working_folder / "output";
		ast::utils::create_format_directory(m_current_working_folder, "output");
	}
	else if (filesystem::is_regular_file(str))
	{
		m_prehandled_file = str;
		m_preoutput_file = ast::utils::create_format_directory<true>(m_prehandled_file, "output", m_prehandled_file.stem().string() + "_out.pcm");
		if (m_preoutput_file == "")cerr << "����ļ������쳣��->set_target_path_or_file" << endl;
	}
	else {
		cerr << "set_working_path·���쳣���ǳ����ļ������ļ���" << endl;
		return false;
	}

	return true;

}




//����Ƶ��ֳ�N��ͨ��
bool AudioAssistant::split_audio(const string&& audioPath, const int&& chanel)
{

	if (chanel > 16 || chanel <= 1) { cout << "�������Ƶͨ�����쳣" << endl; return false; }
	filesystem::path input_name_pcm_file = audioPath;
	if (!filesystem::is_regular_file(input_name_pcm_file)) { cerr << input_name_pcm_file << "����һ���ļ����ļ�������" << audioPath << endl;  return false; }


	//�������ļ�
	ifstream m_ifs(input_name_pcm_file, ios_base::in | ios_base::binary | ios_base::ate);
	long long m_file_size = m_ifs.tellg();
	m_ifs.seekg(0, ios_base::beg);
	cout << "�����ļ�����" << input_name_pcm_file << "�ļ���С��" << m_file_size << endl;
	if (m_file_size % (sizeof(short) * chanel) != 0) {
		cout << "�����������Ƶ�޷�����N��ͨ�����֣������Ƶ����ڳ��Ȳ�һ��" << endl;
	}


	//������ļ�
	unique_ptr<filesystem::path[]> output_pcmfile = make_unique<filesystem::path[]>(chanel);//��������ļ��ľ���·��
	unique_ptr<ofstream[]> m_ofs = make_unique<ofstream[]>(chanel);//��������ļ����ļ�������

	auto target_path = ast::utils::create_format_directory(input_name_pcm_file.parent_path(), "output");;
	for (size_t i = 0; i < chanel; i++) {
		output_pcmfile[i] = filesystem::path(target_path) / (input_name_pcm_file.stem().string() + "_" + to_string(i + 1) + ".pcm");
		m_ofs[i].open(output_pcmfile[i], ios_base::out | ios_base::binary);
		printf("����ļ� = %s\n", output_pcmfile[i].string().c_str());
	}

	//������Ƶ
	{
		unique_lock<mutex> lock(m_readpcm_buffer_mutex);
		while (m_ifs.peek() != EOF) {

			memset(m_readpcm_buffer_common.get(), 0, audio_readsize_once);
			size_t real_size = m_ifs.read(m_readpcm_buffer_common.get(), audio_readsize_once).gcount();//��ȡ������С��5120000 byte��

			for (int sample = 0; sample < real_size / 2; sample++) {
				m_ofs[sample % chanel].write((char*)((short*)m_readpcm_buffer_common.get() + sample), sizeof(short));
			}
		}
	}


	cout << "�����ɣ�����������" << output_pcmfile[0].parent_path() << endl;
	//�����ر��ļ�
	for (size_t i = 0; i < chanel; i++) {
		m_ofs[i].close();
	}
	m_ifs.close();
	return 0;
}

SplitAudio::SplitAudio(const fs::path files, const int chanel)
{
	
	if (filesystem::is_directory(files))
	{
		//������
	}
	else if (filesystem::is_regular_file(files))
	{
		if (chanel > 16 || chanel <= 1) { throw "�������Ƶͨ�����쳣"; }
		
		//�������ļ�
		ifstream m_ifs(files, ios_base::in | ios_base::binary);
		if (!m_ifs.is_open()) { throw "������Ƶ��ʧ��[SplitAudio::SplitAudio]"; }
		
		//��������ļ�����
		for (int i = 0; i < chanel; i++)
		{
			std::string m_tmp = ast::utils::create_format_directory<true>(files.parent_path(), "splitOut","spout"+std::to_string(i)+".pcm");
			if (m_tmp != "") { m_audio_pool.emplace_back(m_tmp); }
			else { std::cout << "some file open failed " << std::endl; }
		}
		
		m_input_chanel = m_audio_pool.size();
		if (ast::utils::open_files(m_audio_pool, v_ofs_handle) != 0) { throw "�����Ƶ��ʧ��[SplitAudio::SplitAudio]"; }
		
		//���뻺��
		{
			m_readpcm_buffer_common = make_unique<char[]>(audio_readsize_once);
			if (!m_readpcm_buffer_common) {
				throw "m_readpcm_buffer_common buffer allocated failed";
			}
		}
	}
	else {
		cerr << "set_working_path·���쳣���ǳ����ļ������ļ���" << endl;
	}
}

bool SplitAudio::start_split()
{
	//������Ƶ
	m_ifs.seekg(0,ios_base::beg);
	{
		while (m_ifs.peek() != EOF) {

			memset(m_readpcm_buffer_common.get(), 0, audio_readsize_once);
			size_t real_size = m_ifs.read(m_readpcm_buffer_common.get(), audio_readsize_once).gcount();//��ȡ������С��5120000 byte��

			for (long sample = 0; sample < real_size / 2; sample++) {
				v_ofs_handle[sample % m_input_chanel].write((char*)((short*)m_readpcm_buffer_common.get() + sample), sizeof(short));
			}
		}
	}
	return true;
}



//δ����
bool AudioAssistant::cut_audio_timepoint(const string&& audioPath, const int&& chanel) {

	long m_start = 0, m_end = 0;
	set_target_path_or_file(forward<const string>(audioPath));
	std::string pcm_file = "";
	std::filesystem::path intput_name_of_pcm_file_p = audioPath;
	std::filesystem::path output_name_of_pcm_file_p = "";


	//33��10�뵽33��30��
	m_start = 32000 * 1990 + 32 * 0;//��+΢��
	m_end = 32000 * 2010 + 32 * 0;//��+΢��


	printf("����8ͨ����Ƶ\n");
	std::getline(std::cin, pcm_file);
	intput_name_of_pcm_file_p = pcm_file;//C:\Users\zmzha\Music\locate.pcm
	output_name_of_pcm_file_p = intput_name_of_pcm_file_p.parent_path() / (intput_name_of_pcm_file_p.stem().string() + "_cut.pcm");


	if (!std::filesystem::exists(intput_name_of_pcm_file_p) || !std::filesystem::is_regular_file(intput_name_of_pcm_file_p)) {
		printf("%s �����ڻ��߲���һ���ļ�", intput_name_of_pcm_file_p.string().c_str());
		return -1;
	}


	printf("����ļ���λ��%s\n", output_name_of_pcm_file_p.string().c_str());
	std::ifstream i_fs(intput_name_of_pcm_file_p, std::ios_base::in | std::ios_base::binary);
	std::ofstream o_fs(output_name_of_pcm_file_p, std::ios_base::out | std::ios_base::binary);//C:\Users\zmzha\Music\locate_cut.pcm

#define INPUT_CHANEL 8
	i_fs.seekg(m_start * INPUT_CHANEL, std::ios::beg);//���ö�ָ��λ�������ʼλ��ƫ��  m_start�ֽ�

	short s1[INPUT_CHANEL] = { 0 };


	while (1) {
		if (i_fs.tellg() > m_end * INPUT_CHANEL) {
			puts("�Ѵ���β�˵�");
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


//��ȡ������Ƶ��λ�ã�����ֵ�Ǻ���
long FindAudioPosition::get_shortaudio_position()
{
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


string FindAudioPosition::get_shortaudio_position_str()
{
	long posi_ms = get_shortaudio_position();


	if (posi_ms == -1) return "is_init failed";
	if (!m_has_found) return "can not find object in target audio";
	char buffer[64] = {0};

	sprintf_s(buffer, "%02d:%02d:%02d.%03d", (posi_ms / 1000) / 3600, (posi_ms / 1000) % 3600 / 60, (posi_ms / 1000) % 3600 % 60, posi_ms%1000); //��������ת��Ϊ�ַ���
	

	return buffer;
}

// ʹ��KMP�㷨�ڳ���Ƶ�в��Ҷ���Ƶ��λ��
int FindAudioPosition::findSubsequence(const short* longAudio, const short* shortAudio) 
{
	int n = actual_longaudio_length_read / 2;
	int m = actual_shortaudio_length_read / 2;


	// ����next����
	vector<int> next(m, 0);
	for (int i = 1, j = 0; i < m; i++) {
		while (j > 0 && shortAudio[i] != shortAudio[j]) {
			j = next[j - 1];
		}
		if (shortAudio[i] == shortAudio[j]) {
			j++;
		}
		next[i] = j;
	}

	// ʹ��next����ƥ�䳤��Ƶ�Ͷ���Ƶ
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

	return -1; // �Ҳ���
}


FindAudioPosition::FindAudioPosition(const fs::path short_path, const fs::path long_path)
{
	using namespace std;

	//�ڴ�����
	{
		//ֱ�Ӷ�ȡ��������Ƶ��
		m_short_audio_buffer = std::make_unique<char[]>(ShortAudioSize);
		m_long_audio_buffer = std::make_unique<char[]>(LongAudioSize);
		if (!m_short_audio_buffer || !m_long_audio_buffer) {
			throw "m_short_audio_buffer buffer allocated failed";
		}
	}
	
	//�������Ƶ�ļ�������¼һЩ��Ƶ��С����Ϣ
	{
		if (!fs::is_regular_file(short_path) || !fs::is_regular_file(long_path))
		{
			throw "input file is not exist[FindAudioPosition::FindAudioPosition]";
		}

		//�򿪳�����Ƶ
		ifstream ifs_short_audio(short_path, ios_base::in | ios_base::binary);
		ifs_long_audio.open(long_path, ios_base::in | ios_base::binary | ios_base::ate);
		if (!ifs_short_audio.is_open() || !ifs_long_audio.is_open()) {
			throw "audio open failed";
		}

		actual_shortaudio_length_read = ifs_short_audio.read(m_short_audio_buffer.get(), ShortAudioSize).gcount();
		long_audio_total_size = ifs_long_audio.tellg();
		ifs_long_audio.seekg(0,ios_base::beg);
	}


	is_init_success = true;
}


CutOrLengthenAudio::CutOrLengthenAudio(const fs::path original_path, const int chanel)
{

	//�����ж�
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
	}

	//open input and target output file and get some file information
	{
		ifs_original_audio.open(original_path, ios_base::in | ios_base::binary | ios_base::ate);
		ofs_output_audio.open(original_path.parent_path() / "cut_output_audio.pcm", ios_base::out | ios_base::binary);
		if (!ifs_original_audio.is_open() || !ofs_output_audio.is_open()) {
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


//����ļ��������ʱ�����
bool CutOrLengthenAudio::cut_op(const std::string& cut_time_str)
{
	long start_byte = 0, end_byte = 0;
	if (!get_start_and_end_ms(cut_time_str, start_byte, end_byte)) {
		std::cout << "Invalid time format[CutOrLengthenAudio::cut_op]" << std::endl;
		return false;
	}


	cut_op_main(start_byte * 32, end_byte * 32);

	

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
	if (m_long_audio_total_size < end_byte || start_byte > end_byte)
	{
		std::cout << "Invalid time_byte [CutOrLengthenAudio::cut_op_main]" << std::endl;
		return false;
	}

	long length = audio_chanel*(end_byte - start_byte);

	if (audio_chanel == 1) {
		ifs_original_audio.seekg(start_byte, std::ios_base::beg);

		while (ifs_original_audio.peek() != EOF && length > 0) {
			if (length < ReadAudioSize)
			{
				ifs_original_audio.read(m_original_audio_buffer.get(), length);
				ofs_output_audio.write(m_original_audio_buffer.get(), length);
				break;
			}
			else
			{
				int real_read_size = 0;
				real_read_size = ifs_original_audio.read(m_original_audio_buffer.get(), ReadAudioSize).gcount();
				ofs_output_audio.write(m_original_audio_buffer.get(), real_read_size);
				length -= real_read_size;

			}
		}

		ofs_output_audio.close();
	}
	else {//��ͨ��
		ifs_original_audio.seekg(start_byte * audio_chanel, std::ios_base::beg);
		while (ifs_original_audio.peek() != EOF && length > 0) {
			if (length < ReadAudioSize * audio_chanel)
			{
				ifs_original_audio.read(m_original_audio_buffer.get(), length);
				ofs_output_audio.write(m_original_audio_buffer.get(), length);
				break;
			}
			else
			{
				int real_read_size = 0;
				real_read_size = ifs_original_audio.read(m_original_audio_buffer.get(), ReadAudioSize* audio_chanel).gcount();
				ofs_output_audio.write(m_original_audio_buffer.get(), real_read_size);
				length -= real_read_size;

			}
		}

		ofs_output_audio.close();
	}


	
	return true;
}



MergeAudio::MergeAudio(const fs::path working_path)
	:m_default_output(working_path/"merge_output.pcm"),
	m_num_of_material(0)
{
	if (!fs::is_directory(working_path)) {
		throw "invalid input [MergeAudio::MergeAudio]";
	}


	if (!ast::utils::get_files_from_directory(working_path, ".pcm", m_audio_pool)) {
		std::cout << "get_files_from_directory failed![MergeAudio::MergeAudio]" << std::endl;
	}
	m_num_of_material = m_audio_pool.size();
}

MergeAudio::MergeAudio()
	:m_num_of_material(0)
{
	m_default_output = fs::current_path() / "merge_output.pcm";
	
	
}


void MergeAudio::refilter_by_extension(const std::string ext)
{

	std::remove_if(m_audio_pool.begin(), m_audio_pool.end(), [&](const fs::path& p) {
		return p.extension() != ext;
		});

	m_num_of_material = m_audio_pool.size();
}

bool MergeAudio::start_merge() {

	if (m_num_of_material > 16 || m_num_of_material <= 1)
	{
		cerr << "unsupported chanel[2~16], merge failed��" << endl;
		return false;
	}
	else if (m_audio_pool.size() <= 1) {
		cerr << "no audio to merge!!" << endl;
		return false;
	}
	
	
	
	//����n����ʱ�ڴ棬���ڴ�����뻺��
	std::vector<std::unique_ptr<char[]>> v_tmp_buffer(m_num_of_material);
	{
		try {
			for (auto& buffer : v_tmp_buffer) {
				buffer = std::make_unique<char[]>(audio_readsize_once);
			}
		}
		catch (const std::bad_alloc& e) {
			std::cerr << "Failed to allocate memory: " << e.what() << std::endl;
			return false;
		}
	}


	//����������ļ�
	{
		OPEN_ONE_FILE(m_ofs, m_default_output);//������ļ�
		if (ast::utils::open_files(m_audio_pool, v_ifs_handle))//�����������ļ�����ʧ����ر�֮ǰ���е��ļ�
		{
			std::cerr << "open input file failed,in[MergeAudio::start_merge()] " << std::endl;
			return false;
		}
	}
	
		

	//��ȡ5.12mb�ļ����ڴ棬�ٴ��ڴ����������
	long long min_real_read_size = audio_readsize_once;//��¼��ȡ���Ķ���ļ��У���С����һ����
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
					// д��ʧ��
					std::cerr << "write failed [MergeAudio::start_merge() ] "<< std::endl;
				}
			}
		}

	}

	
	//�������,�ͷ���Դ
	deinit();
	cout << "merge success, file:" << m_default_output << endl;
	return true;
}


bool MergeAudio::reset_output_file(fs::path new_output_file)
{
	fs::file_status status = fs::status(new_output_file.parent_path()); // ��ȡ�ļ�״̬��Ϣ

	if ((status.permissions() & fs::perms::owner_write) != fs::perms::none) {
		m_default_output = new_output_file;
		return true;
	}

	return false;
}

void MergeAudio::deinit()
{
	for (int i = 0; i < v_ifs_handle.size(); i++)
	{
		v_ifs_handle.at(i).close();
	}
	m_ofs.close();
	v_ifs_handle.clear();
	m_audio_pool.clear();
	m_num_of_material = 0;
}




