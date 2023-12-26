#ifndef MIDI_track
#define MIDI_track

#include<fstream>
#include<filesystem>
#include<vector>
#include<cstring>
#include<string>
#include<cassert>

#include"MESSAGE_PRINT.h"

class MIDI_track_data
{
public:
	MIDI_track_data(size_t size, uint8_t* track_data);
	~MIDI_track_data();

	typedef struct {
		size_t time;
		size_t data_size;
		uint8_t* event_data;
	}track_data_format;

	MIDI_track_data::track_data_format get_track_data(uint32_t data_no);//n番目のデータを返す
private:
	uint8_t* p_track_data = nullptr;//トラックデータの始端ポインタ
	size_t data_size = 0;

	uint16_t status_count = 1;//get_track_data()高速化用変数
	size_t startcount_before = 0;

	uint8_t return_time(uint8_t* data, size_t* time_num);//特殊フォーマットの時間を返す


};



inline MIDI_track_data::MIDI_track_data(size_t size, uint8_t* data)
{
	assert(data);//nullptrチェック
	p_track_data = data;
	data_size = size;
}

inline MIDI_track_data::~MIDI_track_data()
{

}

inline MIDI_track_data::track_data_format MIDI_track_data::get_track_data(uint32_t data_no) {
	size_t count = 0;
	size_t return_data_num = 0;



	MIDI_track_data::track_data_format return_buf = {};

	if (data_no >= status_count - 1) {
		count = startcount_before;//前回呼ばれた時と同じか後ろを指定された時の処理
		status_count--;
	}
	else {
		status_count = 0;//前回より前をしてされた時の処理
		return_data_num = 0;
	}

	do {
		if (count >= data_size) {//領域外アクセス避け
			return_buf.data_size = 0;
			return return_buf;
		}

		startcount_before = count;
		count += return_time(&p_track_data[count], &return_buf.time);//時間を読み込み
		return_data_num = count;
		status_count++;

		switch (p_track_data[count] & 0xf0) {
		case 0x80://ノートオフ 8n kk vv チャンネル番号nのノート番号kkの音を止める。
			count += 3;
			return_buf.data_size = 3;

			break;

		case 0x90://ノートオン 9n kk vv チャンネルnでノート番号kkの音をベロシティvvで鳴らす。
			count += 3;
			return_buf.data_size = 3;
			break;

		case 0xA0://ポリフォニックキープレッシャー An kk vv チャンネルnで発音中のノート番号kkの音に対し、ベロシティvvの プレッシャー情報を与える。
			count += 3;
			return_buf.data_size = 3;
			break;

		case 0xB0://コントロールチェンジ Bn cc vv チャンネルnで、コントローラナンバーccに、値vvを送る。
			count += 3;
			return_buf.data_size = 3;
			break;

		case 0xC0://プログラムチェンジ Cn pp  チャンネルnで、プログラム(音色)をppに変更する。
			count += 2;
			return_buf.data_size = 2;
			break;

		case 0xD0://チャンネルプレッシャー Dn vv チャンネルnに対し、プレッシャー情報vvを送信する。
			count += 2;
			return_buf.data_size = 2;
			break;

		case 0xE0://ピッチベンド En mm ll チャンネルnに対し、ピッチベンド値llmmを送信する。リトルエンディアンなので注意。
			count += 3;
			return_buf.data_size = 3;
			break;

		case 0xF0://特殊な指示

			switch (p_track_data[count]) {
			case 0xF0://F0で始まるsysexイベント
				return_buf.data_size = (p_track_data[count + 1] + 2);
				count += (p_track_data[count + 1] + 2);

				break;

			case 0xF7://F7で始まるsysexイベント
				return_buf.data_size = (p_track_data[count + 1] + 2);
				count += (p_track_data[count + 1] + 2);

				break;

			case 0xFF://メタイベント
				return_buf.data_size = (p_track_data[count + 2] + 3);
				count += (p_track_data[count + 2] + 3);

				break;

			default:
				ERROR_PRINT("ファイルが 破損しています。",0);
                break;
			}
			break;

		default:
			printf("\n\n%#x\n", p_track_data[count]);
			uint8_t a = p_track_data[count];
			ERROR_PRINT("ファイルが破損しています。", 0);
			break;
		}

	} while (data_no >= status_count);//指定回繰り返す

	return_buf.event_data = &p_track_data[return_data_num];
	return return_buf;

}

inline uint8_t MIDI_track_data::return_time(uint8_t* data, size_t* time_num) {
	uint8_t timedata_size = 0;
	uint8_t data_count = 0;

	*time_num = 0;


	do {
		timedata_size++;
		*time_num = *time_num << 7;
		*time_num += (data[timedata_size - 1] & 0x7f);//上位1ビットは可変長の管理使用するため除外
	} while ((data[timedata_size - 1] & 0x80) != 0);//上位1ビットが1なら継続

	return timedata_size;

}

#endif