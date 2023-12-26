#ifndef MIDI_in

#define MIDI_in

#include <fstream>
#include <filesystem>
#include <string>
#include <cstdint>
#include <vector>

#include "MESSAGE_PRINT.h"
#include "MIDI_track.hpp"

class MIDI_input
{
public:
    MIDI_input(std::string path);
    ~MIDI_input();

    typedef struct
    {                         // ヘッダフォーマット
        uint16_t MIDI_format; // MIDIのフォーマット
        uint16_t MIDI_track_num;  // トラック数
        uint16_t resolution;  // 分解能(4分音符1個当たり時間単位？
    } MIDI_heard;

    MIDI_heard *heard_data; // ヘッダを入れる

    MIDI_track_data::track_data_format get_MIDI_data(uint8_t track_no,uint32_t data_no);

private:
    std::vector<uint8_t> file_data; // 読み込んだMIDIファイルを入れる

    std::vector<MIDI_track_data*> track_data;//トラックデータの配列を入れる。

    template <typename TYPE>
    TYPE endian_change(TYPE *data); // エンディアン変換

    template <typename TYPE>
    void endian_change_overwrite(TYPE *data); // エンディアン変換して上書き
};

MIDI_input::MIDI_input(std::string path)
{
    std::uintmax_t file_size = std::filesystem::file_size(path); // ファイルサイズを読みとり
    file_data.resize(file_size);

    size_t file_place = 0; // ファイルの場所を記録

    {
        std::ifstream ifs(path, std::ios::binary); // ファイルを開く
        if (!ifs)
        {
            ERROR_PRINT("ファイルを開けませんでした。", -1)
        }

        ifs.read(reinterpret_cast<char *>(&file_data[0]), file_size); // ファイルを読み込み

    } // ここでファイルが閉じる

    if (memcmp(&file_data[file_place], "MThd", 4))
    { // ヘッダの"MThd"があるか確認
        ERROR_PRINT("ファイルのフォーマットが違います", 0)
    }

    file_place += 4; // ヘッダの"MYhd"(4byte)

    heard_data = (MIDI_input::MIDI_heard *)&file_data[file_place + 4]; // データの先頭ポインタを渡す

    for (int i = 0; i < MIDI_input::endian_change((uint32_t *)&file_data[file_place]) / sizeof(uint16_t); i++)
    {
        MIDI_input::endian_change_overwrite((uint16_t *)&file_data[file_place + 4 + (i * sizeof(uint16_t))]); // ヘッダ部分のエンディアンを変換
    }

    file_place += (4 + MIDI_input::endian_change((uint32_t *)&file_data[4])); // ヘッダの領域分進める

    if ((heard_data->MIDI_format == 0) && (heard_data->MIDI_track_num == 1))
    { // フォーマット0
    }
    else if ((heard_data->MIDI_format == 1) && (heard_data->MIDI_track_num != 0))
    { // フォーマット1
    }
    else if (heard_data->MIDI_format == 2)
    { // フォーマット2(非対応)
        ERROR_PRINT("フォーマット2は非対応です。", 0)
    }
    else
    { // エラー
        ERROR_PRINT("ファイルが破損しています。", 0)
    }

    track_data.resize(heard_data->MIDI_track_num);//トラックデータを入れるメモリを確保

	for (int i = 0; i < heard_data->MIDI_track_num; i++) {
		if (memcmp(&file_data[file_place], "MTrk", 4)) {//ヘッダの"MThd"があるか確認
			ERROR_PRINT("ファイルのフォーマットが違います", 0)
		}

		file_place += 4;//トラックの"MYrk"(4byte)

		track_data[i] = new MIDI_track_data(MIDI_input::endian_change((uint32_t*)&file_data[file_place]), &file_data[file_place + 4]);//トラックデータをクラスに渡す

		file_place += (MIDI_input::endian_change((uint32_t*)&file_data[file_place]) + 4);//トラックデータのサイズぶん進める
	}
}

MIDI_input::~MIDI_input()
{
}

template <typename TYPE>
inline TYPE MIDI_input::endian_change(TYPE *data)
{
    TYPE num = 0;
    uint8_t *p_buf = (uint8_t *)data;
    for (int i = 0; i < sizeof(TYPE); i++)
    {
        num += p_buf[i] << (8 * (sizeof(TYPE) - 1 - i)); // 上位バイトを左シフトして加算。(進むほどシフト数が減る)
    }
    return num;
}

template <typename TYPE>
inline void MIDI_input::endian_change_overwrite(TYPE *data)
{
    TYPE num = 0;
    uint8_t *p_buf = (uint8_t *)data;
    for (int i = 0; i < sizeof(TYPE); i++)
    {
        num += p_buf[i] << (8 * (sizeof(TYPE) - 1 - i));
    }

    *data = num;
}

inline MIDI_track_data::track_data_format MIDI_input::get_MIDI_data(uint8_t track_no,uint32_t data_no){
    MIDI_track_data::track_data_format return_buf={0};

    if(track_no > heard_data->MIDI_track_num){//指定したトラックが実在しなかった場合
        return return_buf;
    }

    return_buf=track_data[track_no]->get_track_data(data_no);//トラックからデータをもらう

    return return_buf;

}

#endif