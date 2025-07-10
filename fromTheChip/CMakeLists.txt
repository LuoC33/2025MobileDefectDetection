set(WLAN_DRV_PATH "/home/yanxu/rtos_k230/output/k230_rtos_01studio_defconfig/rtsmart/kernel/components/drivers/wlan")

set(src main.cc pipeline.cc ai_base.cc yolov5.cc yolov8.cc yolo11.cc utils.cc 
    superpoint.cc ORBExtractor.cc feature_processor.cc SpatialFilter.cc 
    AppearanceFilter.cc GeometricVerifier.cc TemporalFilter.cc ObjectDatabase.cc 
    MultiLayerFilter.cc uart_transfer.cc yolo_compress.cc
    ../../../../rtsmart/libs/rtsmart_hal/drivers/uart/drv_uart.c
    ../../../../rtsmart/libs/rtsmart_hal/drivers/fpioa/drv_fpioa.c
    network_comm.cc
    )
set(bin yolo.elf)
set(MPP_LIB_PATH "/../../../rtsmart/userapps/lib")
set(MPP_RTT_PATH "/../../../rtsmart/userapps/sdk/rt-thread/lib/risc-v/rv64")


set(WLAN_LIB_PATH "/../../../rtsmart/kernel/bsp/maix3/drivers/extdrv/realtek/wlan_lib")

file(GLOB LIB_FILES 
    LIST_DIRECTORIES false
    PATHS ${MPP_LIB_PATH}
    NAMES "lib*.a"
)
file(GLOB RTT_LIB_FILES
    LIST_DIRECTORIES false
    PATHS ${MPP_RTT_PATH}
    NAMES "lib*.a"
)
file(GLOB WLAN_LIB_FILES
    LIST_DIRECTORIES false
    PATHS ${WLAN_LIB_PATH}
    NAMES "libwlan*.a"
)

foreach(LIB_FILE ${LIB_FILES})
    get_filename_component(LIB_NAME ${LIB_FILE} NAME_WE)
    string(REGEX REPLACE "^lib" "" LIB_NAME ${LIB_NAME})
    list(APPEND MPP_LIBS "-l${LIB_NAME}")
endforeach()
foreach(LIB_FILE ${RTT_LIB_FILES})
    get_filename_component(LIB_NAME ${LIB_FILE} NAME_WE)
    string(REGEX REPLACE "^lib" "" LIB_NAME ${LIB_NAME})
    list(APPEND RTT_LIBS "-l${LIB_NAME}")
endforeach()
foreach(LIB_FILE ${WLAN_LIB_FILES})
    get_filename_component(LIB_NAME ${LIB_FILE} NAME_WE)
    string(REGEX REPLACE "^lib" "" LIB_NAME ${LIB_NAME})
    list(APPEND WLAN_LIBS "-l${LIB_NAME}")
endforeach()

include_directories(../../../rtsmart/userapps/sdk/rt-thread/include/)
include_directories(../../../rtsmart/userapps)
include_directories(../../../rtsmart/userapps/sdk/rt-thread/components/drivers/)
include_directories(../../../rtsmart/userapps/sdk/rt-thread/components/dfs/)
include_directories(../../../rtsmart/kernel/rt-thread/components/drivers/include/)
include_directories(../../../../rtsmart/libs/rtsmart_hal/drivers/fpioa)
include_directories(../../../../rtsmart/libs/rtsmart_hal/drivers/uart)
include_directories(../../../rtsmart/kernel/rt-thread/include/)
include_directories(../../../rtsmart/kernel/rt-thread/components/drivers/wlan)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${nncase_sdk_root}/riscv64/rvvlib/include)
include_directories(${k230_mpp_root}/userapps/api/)
include_directories(${k230_mpp_root}/include)
include_directories(${k230_mpp_root}/include/comm)
include_directories(${k230_mpp_root}/userapps/sample/sample_vo)
link_directories(${nncase_sdk_root}/riscv64/rvvlib/)
link_directories(
    ${WLAN_LIB_PATH}
    ${CMAKE_SOURCE_DIR}/../../../rtsmart/userapps/lib
)


add_executable(${bin} ${src})
target_link_libraries(${bin} -Wl,--start-group rvv Nncase.Runtime.Native nncase.rt_modules.k230 functional_k230 sys vicap vb cam_device cam_engine
 hal oslayer ebase fpga isp_drv binder auto_ctrol common cam_caldb isi 3a buffer_management cameric_drv video_in virtual_hal start_engine cmd_buffer
 switch cameric_reg_drv t_database_c t_mxml_c t_json_c t_common_c vo connector sensor atomic dma -Wl,--end-group
)

target_link_libraries(${bin} -Wl,--start-group ${RTT_LIBS} opencv_imgcodecs opencv_imgproc opencv_core opencv_features2d opencv_flann opencv_calib3d zlib libjpeg-turbo libopenjp2 libpng libtiff libwebp  ${MPP_LIBS} csi_cv ${WLAN_LIBS} -Wl,--end-group)

install(TARGETS ${bin} DESTINATION bin)
