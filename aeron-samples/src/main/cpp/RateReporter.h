/*
 * Copyright 2015 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AERON_RATEREPORTER_H
#define AERON_RATEREPORTER_H

#include <functional>
#include <atomic>
#include <thread>
#include <chrono>

namespace aeron {

using namespace std::chrono;

class RateReporter
{
public:
    typedef std::function<void(double, double, long, long)> on_rate_report_t;

    RateReporter(nanoseconds reportInterval, const on_rate_report_t& onReport) :
        m_reportInterval(reportInterval),
        m_onReport(onReport),
        m_lastTimestamp(steady_clock::now())
    {
    }

    void run()
    {
        do
        {
            std::this_thread::sleep_for(m_reportInterval);

            long currentTotalBytes = std::atomic_load(&m_totalBytes);
            long currentTotalMessages = std::atomic_load(&m_totalMessages);
            steady_clock::time_point currentTimestamp = steady_clock::now();

            const double timeSpanSec = duration<double, std::ratio<1,1>>(currentTimestamp - m_lastTimestamp).count();
            const double messagesPerSec = (currentTotalMessages - m_lastTotalMessages) / timeSpanSec;
            const double bytesPerSec = (currentTotalBytes - m_lastTotalBytes) / timeSpanSec;

            m_onReport(messagesPerSec, bytesPerSec, currentTotalMessages, currentTotalBytes);

            m_lastTotalBytes = currentTotalBytes;
            m_lastTotalMessages = currentTotalMessages;
            m_lastTimestamp = currentTimestamp;
        }
        while (!m_halt);
    }

    inline void halt()
    {
        m_halt = true;
    }

    inline void onMessage(long messages, long bytes)
    {
        long currentTotalBytes = std::atomic_load(&m_totalBytes);
        long currentTotalMessages = std::atomic_load(&m_totalMessages);

        std::atomic_store(&m_totalBytes, currentTotalBytes + bytes);
        std::atomic_store(&m_totalMessages, currentTotalMessages + messages);
    }

private:
    nanoseconds m_reportInterval;
    const on_rate_report_t m_onReport;
    std::atomic<bool> m_halt = { false };
    std::atomic<long> m_totalBytes = { 0 };
    std::atomic<long> m_totalMessages = { 0 };

    long m_lastTotalBytes = 0;
    long m_lastTotalMessages = 0;

    steady_clock::time_point m_lastTimestamp;
};

}

#endif //AERON_RATEREPORTER_H
