idf_component_register(SRCS "CStreamer.cpp" "CRtspSession.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES lwip esp_hw_support freertos)

set_source_files_properties(CRtspSession.cpp PROPERTIES COMPILE_FLAGS "-Wno-format-truncation")
