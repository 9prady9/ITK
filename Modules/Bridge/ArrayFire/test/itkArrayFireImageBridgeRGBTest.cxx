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
#include "itkImageRegionConstIteratorWithIndex.h"

#include <arrayfire.h>

//-----------------------------------------------------------------------------
// Compare RGBPixel Images
//
template<typename TPixelValue, unsigned int VDimension>
TPixelValue
RGBImageTotalAbsDifference(
        const itk::Image<itk::RGBPixel<TPixelValue>, VDimension>* valid,
        const itk::Image<itk::RGBPixel<TPixelValue>, VDimension>* test)
{
    typedef itk::RGBPixel<TPixelValue>                           PixelType;
    typedef itk::Image<PixelType, VDimension>                    RGBImageType;
    typedef itk::ImageRegionConstIteratorWithIndex<RGBImageType> IterType;

    IterType validIt(valid, valid->GetLargestPossibleRegion());
    validIt.GoToBegin();

    IterType testIt(test, test->GetLargestPossibleRegion());
    testIt.GoToBegin();

    TPixelValue totalDiff = 0;

    while(!validIt.IsAtEnd())
    {
        PixelType validPx = validIt.Get();
        PixelType testPx = testIt.Get();

        if( validIt.GetIndex() != testIt.GetIndex() )
        {
            std::cerr << "validIt.GetIndex() != testIt.GetIndex()"
                      << validIt.GetIndex() << " - "
                      << testIt.GetIndex()
                      << std::endl;
            return 1;
        }

        TPixelValue localDiff = itk::NumericTraits< TPixelValue >::ZeroValue();

        for( unsigned int i = 0; i < 3; i++ )
        {
            localDiff += itk::Math::abs(validPx[i] - testPx[i]);
        }

        if( localDiff != itk::NumericTraits< TPixelValue >::ZeroValue() )
        {
            IterType testIt2 = testIt;
            ++testIt2;
            IterType validIt2 = validIt;
            ++validIt2;
            std::cerr << testIt.GetIndex() << " [ " << validPx << " " << validIt2.Get() << "] != [ " << testPx << " " << testIt2.Get() << " ]" << std::endl;
            return localDiff;
        }

        totalDiff += localDiff;

        ++validIt;
        ++testIt;
    }

    return totalDiff;
}

//-----------------------------------------------------------------------------
// Templated test function to do the heavy lifting for RGB case
//
template<typename TValue, unsigned int VDimension>
int itkArrayFireImageBridgeTestTemplatedRGB(char* argv)
{
    // typedefs
    const unsigned int Dimension =                          VDimension;
    typedef TValue                                          ValueType;
    typedef itk::RGBPixel< ValueType >                      PixelType;
    typedef typename PixelType::ComponentType               ComponentType;
    typedef itk::Image< PixelType, Dimension >              ImageType;
    typedef itk::ImageFileReader<ImageType>                 ReaderType;

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
        inImg  = af::loadImageNative(argv);
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

    ComponentType itkafDiff = RGBImageTotalAbsDifference<ComponentType, Dimension>(
            baselineImage, af2ITKOut);

    if ( itkafDiff != itk::NumericTraits< ComponentType >::ZeroValue() )
    {
        std::cerr << "Images didn't match for pixel type "
                  << typeid(PixelType).name()
                  << " for IplImage -> ITK (RGB), with image difference = "
                  << (double)itkafDiff
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Test itk::Image -> af::array..." << std::endl;

    af::array itk2afOut =
        itk::ArrayFireImageBridge::ITKImageToAfArray< ImageType >(baselineImage);

    float itkAfDiff = af::sum<float>( (itk2afOut!=inImg).as(f32) );

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

template< typename TValue >
int itkRunRGBTest( char* argv )
{
    if (itkArrayFireImageBridgeTestTemplatedRGB< TValue, 2 >(argv) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// Main test
//
int itkArrayFireImageBridgeRGBTest ( int argc, char *argv[] )
{
    //
    // Check arguments
    //
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << "rgb_image1 rgb_image2 rgb_image3" << std::endl;
        return EXIT_FAILURE;
    }

    //
    // Test for RGB types
    //
    // Note: OpenCV only supports unsigned char, unsigned short, and float for
    // color conversion
    //
    std::cout<<"========================================="<<std::endl;

    if( itkRunRGBTest< unsigned char >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunRGBTest< unsigned short >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunRGBTest< float >( argv[1] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }

    std::cout<<"========================================="<<std::endl;

    if( itkRunRGBTest< unsigned char >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunRGBTest< unsigned short >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunRGBTest< float >( argv[2] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }

    std::cout<<"========================================="<<std::endl;

    if( itkRunRGBTest< unsigned char >( argv[3] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunRGBTest< unsigned short >( argv[3] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    if( itkRunRGBTest< float >( argv[3] ) == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
