/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include <iostream>

#include "itkArrayFireImageBridge.h"
#include "itkImageFileReader.h"
#include "itkTestingComparisonImageFilter.h"

#include <arrayfire.h>

//-----------------------------------------------------------------------------
// Templated test function to do the heavy lifting for scalar case
//
template<typename TPixelType, unsigned int VDimension>
int itkArrayFireImageBridgeTestTemplatedScalar(char* argv)
{
    // typedefs
    const unsigned int Dimension =                              VDimension;
    typedef TPixelType                                          PixelType;
    typedef typename itk::NumericTraits<PixelType>::ValueType   ValueType;
    typedef itk::Image< PixelType, Dimension >                  ImageType;
    typedef itk::ImageFileReader<ImageType>                     ReaderType;
    typedef itk::Testing::ComparisonImageFilter<ImageType, ImageType>
        DifferenceFilterType;

    //
    // Read the image directly
    //
    typename ReaderType::Pointer reader = ReaderType::New();

    reader->SetFileName(argv);

    try {
        reader->Update();
    } catch( itk::ExceptionObject& e ) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Read image with pixel type "
        << typeid(PixelType).name() << " and dimension " << VDimension << std::endl;

    typename ImageType::Pointer baselineImage = reader->GetOutput();

    std::cout << "Test af::array -> itk::Image..." << std::endl;

    af::array inImg;

    try {
        // If we load grayscale image sending false to af::loadImage,
        // it will use default % of R, G, B channels which are different
        // from what ITK uses internally to compute luminance from RGB triplet
        //
        // ArrayFire conversion %s are
        //              (red=0.2126f, green=0.7152f, blue=0.0722f)
        // ITK conversion %s are
        //              (red=0.30f, green=0.59f, blue=0.11f)

        inImg = af::loadImageNative(argv);
    } catch (af::exception &e) {
        std::cerr << "Could not load input as af::array" << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    typename ImageType::Pointer af2ITKOut =
        itk::ArrayFireImageBridge::AfArrayToITKImage< ImageType >(inImg);

    if( af2ITKOut->GetLargestPossibleRegion() != baselineImage->GetLargestPossibleRegion() )
    {
        std::cerr << "Images didn't match: different largest possible region" << std::endl;
        return EXIT_FAILURE;
    }

    // Check results of af::array -> itk::Image
    typename DifferenceFilterType::Pointer differ = DifferenceFilterType::New();
    differ->SetValidInput(baselineImage);
    differ->SetTestInput(af2ITKOut);
    differ->Update();
    typename DifferenceFilterType::AccumulateType total = differ->GetTotalDifference();

    if (total != 0)
    {
        std::cerr << "Images didn't match for pixel type "
                  << typeid(PixelType).name()
                  << " for af::array -> ITK (scalar) with diff = "
                  << total << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Test itk::Image -> af::array..." << std::endl;

    af::array itk2afOut =
        itk::ArrayFireImageBridge::ITKImageToAfArray< ImageType >(baselineImage);

    // check results of itk::Image -> af::array
    float itkAfDiff = af::sum<float>( (itk2afOut != inImg).as(f32) );

    if (itkAfDiff > 1.0e-3)
    {
        std::cerr << "Images didn't match for pixel type "
            << typeid(PixelType).name() << " for ITK -> af::array (scalar)"
            << "; itkIplDiff = " << itkAfDiff << std::endl;
        return EXIT_FAILURE;
    }

    std::cout<<std::endl;

    return EXIT_SUCCESS;
}

template< typename TPixel >
int itkRunScalarTest( char* argv )
{
    if (itkArrayFireImageBridgeTestTemplatedScalar< TPixel, 2 >(argv) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// Main test
//
int itkArrayFireImageBridgeGrayScaleTest (int argc, char *argv[])
{
    //
    // Check arguments
    //
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << "scalar_image1 scalar_image2" << std::endl;
        return EXIT_FAILURE;
    }

    //
    // Test for scalar types
    //
    // Note: We don't test signed char because ITK seems to have trouble reading
    //       images with char pixels.
    //
    std::cout << "\n================================" << std::endl;
    if( itkRunScalarTest< unsigned char >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< short >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< unsigned short >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< float >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< double >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }

    std::cout << "\n================================" << std::endl;
    if( itkRunScalarTest< unsigned char >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< short >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< unsigned short >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< float >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunScalarTest< double >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
