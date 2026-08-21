#pragma once

#include <cstddef>

// フレーム終端でログを取り出し、各出力先へ配送する。
class LogSystem final
{
public:
    static constexpr size_t LogsPerFrame = 100;

    static void Flush();
    static void FlushAll();

private:
    static size_t FlushBatch(size_t maxCount);
};
