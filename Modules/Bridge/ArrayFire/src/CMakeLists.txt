set(${itk-module}_SRC itkArrayFireImageBridge.cxx)

add_library(${itk-module} ${${itk-module}_SRC})
itk_module_link_dependencies()

target_link_libraries(${itk-module}
                      LINK_PUBLIC ${ArrayFire_LIBRARIES}
                      LINK_PUBLIC ${OpenCV_LIBS})

itk_module_target(${itk-module})
