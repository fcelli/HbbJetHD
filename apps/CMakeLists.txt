add_executable(dump_histos)
target_sources(dump_histos PRIVATE dump_histos.cpp)
target_link_libraries(dump_histos HbbJetHD ${ROOT_LIBRARIES})