
#include "tests.hpp"

struct LogPlug : lvtk::Plugin<LogPlug, lvtk::Log> {
    LogPlug (const lvtk::Args& args) : Plugin (args) {}
    void trace (const std::string& message) {
        log << message;
    }
};

class LogTest : public TestFixutre {
    CPPUNIT_TEST_SUITE (LogTest);
    CPPUNIT_TEST (integration);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() {}

protected:
    void integration() {
        lvtk::Descriptor<LogPlug> reg (LVTK_TEST_PLUGIN_URI);
        const auto& desc = lvtk::descriptors().back();
        CPPUNIT_ASSERT (strcmp (desc.URI, LVTK_TEST_PLUGIN_URI) == 0);

        lvtk::URIDirectory uris;

        LV2_Log_Log log;
        log.handle = this;
        log.printf = _printf;
        log.vprintf = _vprintf;
        LV2_Feature flog;
        flog.URI = LV2_LOG__log;
        flog.data = (void*) &log;
        
        const LV2_Feature* features[] = { 
            uris.get_map_feature(), 
            uris.get_unmap_feature(), 
            &flog, nullptr 
        };
        
        auto handle = (LogPlug*) desc.instantiate (&desc, 44100.0, "/fake/path", features);
        CPPUNIT_ASSERT (handle != nullptr);
        #define test_message "hello_logger"
        handle->trace (test_message);
        CPPUNIT_ASSERT (strcmp (test_message, buffer) == 0);
        CPPUNIT_ASSERT (msg_type == uris.map (LV2_LOG__Trace));
        desc.cleanup (handle);
        lvtk::descriptors().pop_back(); // needed so descriptor count test doesn't fail
    }

private:
    enum { buffer_size = 256 };
    char buffer [buffer_size];
    LV2_URID msg_type = 0;

	static int _printf (LV2_Log_Handle, LV2_URID, const char*, ...) { return 0; }

	static int _vprintf (LV2_Log_Handle handle, LV2_URID type, const char* msg, va_list args) {
        auto& buffer = static_cast<LogTest*>(handle)->buffer;
        static_cast<LogTest*>(handle)->msg_type = type;
        memset (buffer, 0, buffer_size);
        return vsprintf (buffer, msg, args);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION (LogTest);
