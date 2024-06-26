#include "serial.hpp"

namespace Toolbox {

    void Serializer::pushBreakpoint() { m_breakpoints.push(m_out.tellp()); }
    Result<void, SerialError> Serializer::popBreakpoint() {
        if (m_breakpoints.empty()) {
            return make_serial_error<void>(
                *this, "No breakpoints to pop! (Proper serialization shouldn't have this happen)");
        }
        m_out.seekp(m_breakpoints.top(), std::ios::beg);
        m_breakpoints.pop();
        return {};
    }

    void Deserializer::pushBreakpoint() { m_breakpoints.push(m_in.tellg()); }
    Result<void, SerialError> Deserializer::popBreakpoint() {
        if (m_breakpoints.empty()) {
            return make_serial_error<void>(
                *this,
                "No breakpoints to pop! (Proper deserialization shouldn't have this happen)");
        }
        m_in.seekg(m_breakpoints.top(), std::ios::beg);
        m_breakpoints.pop();
        return {};
    }

}  // namespace Toolbox