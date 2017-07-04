itk_module_test()

set(ITK${itk-module}Tests
    itkArrayFireImageBridgeGrayScaleTest.cxx
    itkArrayFireImageBridgeRGBTest.cxx
    )

include_directories(${ITKBridgeArrayFire_INCLUDE_DIRS})

CreateTestDriver(${itk-module}
    "${${itk-module}-Test_LIBRARIES};${ArrayFire_LIBRARIES};${OpenCV_LIBS}"
    "${ITK${itk-module}Tests}")

itk_add_test(
    NAME ArrayFireImageBridgeGrayScaleTest
    COMMAND ${itk-module}TestDriver
    itkArrayFireImageBridgeGrayScaleTest
    DATA{Input/spider.jpg}
    DATA{Input/house.jpg}
    )

itk_add_test(
    NAME ArrayFireImageBridgeRGBTest
    COMMAND ${itk-module}TestDriver
    itkArrayFireImageBridgeRGBTest
    DATA{Input/trees_ctm.jpg}
    DATA{Input/man.jpg}
    DATA{Input/atlantis.png}
    )
