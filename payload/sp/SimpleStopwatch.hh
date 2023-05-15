#pragma once

#include <Common.hh>
extern "C" {
#include <revolution/os.h>
}

namespace SP {

class StopWatch {
public:
    StopWatch(const char* name) {
        m_startTime = OSTicksToNanoseconds(OSGetTick());
        m_name = name;
    };

    ~StopWatch() {
        f64 endTime = OSTicksToNanoseconds(OSGetTick());
        SP_LOG("%s timer %.6f", m_name, endTime - m_startTime);
    };

private:
    f64 m_startTime;
    const char* m_name;
};

}
