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
#ifndef itkArrayFireImageBridge_h
#define itkArrayFireImageBridge_h

#include "itkImage.h"
#include "itkFixedArray.h"
#include "itkRGBPixel.h"
#include "itkRGBAPixel.h"
#include "itkDefaultConvertPixelTraits.h"
#include "itkConvertPixelBuffer.h"
#include <opencv2/core/core.hpp>
#include <arrayfire.h>
#include <vector>

namespace itk
{
ITK_ABI_EXPORT af::array CVMatToArray(const cv::Mat& input, bool transpose = false);
ITK_ABI_EXPORT af::array CVMatToArray(const std::vector<cv::Mat>& input, bool transpose = false);
ITK_ABI_EXPORT cv::Mat ArrayToCVMat(const af::array& input, bool transpose = false);

/** \class ArrayFireImageBridge
 * \brief This class provides static methods to convert between ArrayFire arrays
 * and itk::Image
 *
 * This class provides methods for the following conversions:
 *      af::Array -> itk::Image
 *      itk::Image -> af::Array
 *
 * Each method is templated over the type of itk::Image used. The conversions
 * copy the data and convert between types if necessary.
 *
 * \ingroup ITKBridgeArrayFire
 */
class ArrayFireImageBridge
{
    public:
        typedef ArrayFireImageBridge Self;

        template<typename ImageType>
        static typename ImageType::Pointer AfArrayToITKImage(const af::array& in,
                                                             const bool& transposeIn = false);

        template<typename TInputImageType>
        static af::array ITKImageToAfArray(const TInputImageType* in);

    private:
        ITK_DISALLOW_COPY_AND_ASSIGN(ArrayFireImageBridge);

        template< typename T>
        static std::vector<T>
        ITKRGBArrayToVectorArray(const T*, const unsigned, const unsigned)
        {
            //Placeholder implementation for non itk::RGBPixel pixel objects
            return std::vector<T>();
        }

        template< typename T, unsigned channels>
        static std::vector<T>
        ITKRGBArrayToVectorArrayHelper(const itk::FixedArray<T, channels>* inPtr,
                                       const unsigned width,
                                       const unsigned height)
        {
            std::vector<size_t> sliceOffsets;

            for (unsigned c=0; c<channels; ++c)
                sliceOffsets.push_back(c*width*height);

            std::vector<T> output(width*height*channels);

            for (unsigned j=0; j<height; ++j)
            {
                for (unsigned i=0; i<width; ++i)
                {
                    size_t sliceIndex = i + width * j;

                    for (unsigned c=0; c<channels; ++c)
                        output[sliceOffsets[c] + sliceIndex] = inPtr[sliceIndex].operator[](c);
                }
            }

            return output;
        }

        template< typename T>
        static std::vector<T>
        ITKRGBArrayToVectorArray(const itk::RGBPixel<T>* inPtr,
                                 const unsigned width,
                                 const unsigned height)
        {
            typedef itk::FixedArray<T, 3> FixedArray;

            const FixedArray* basePtr = reinterpret_cast<const FixedArray*>(inPtr);

            return ITKRGBArrayToVectorArrayHelper(basePtr, width, height);
        }

        template< typename T>
        static std::vector<T>
        ITKRGBArrayToVectorArray(const itk::RGBAPixel<T>* inPtr,
                                 const unsigned width,
                                 const unsigned height)
        {
            typedef itk::FixedArray<T, 4> FixedArray;

            const FixedArray* basePtr = reinterpret_cast<const FixedArray*>(inPtr);

            return ITKRGBArrayToVectorArrayHelper(basePtr, width, height);
        }

        /// IT IS ASSUMED THAT THE DATA IN af::array IS IN COLOUMN MAJOR FORMAT i.e. ALREADY
        /// TRANSPOSED. HOWEVER, THE CALLER CAN REQUEST INPUT TO BE TRANSPOSED BEFORE CONVERSION
        template<typename ImageType>
        static void ITKConvertArray(const af::array& in, const bool& transposeIn,
                                    ImageType* out)
        {
            typedef typename ImageType::PixelType                       PixelType;
            typedef typename itk::NumericTraits<PixelType>::ValueType   ValueType;

            unsigned int inChannels  = in.dims(2);

            if (inChannels != 1 && inChannels != 3 && inChannels != 4) {
                itkGenericExceptionMacro("Currently, ITKArrayFireBridge supports 1/3/4 channels only.");
            }

            af::array current = transposeIn ? transpose(in) : in;

            //// * Reorder of dims to (2, 1, 0, 3) changes sliced channels into
            ////   interleaved format and transposes the data as well
            //// * Reorder of dims to (1, 0, 2, 3) just transposes the data has no channels
            current = inChannels>1 ? af::reorder(current, 2, 1, 0, 3) : af::reorder(current, 1, 0, 2, 3);

            // itk::Convert function takes care of components as needed
            // when there is mismatch of input and output channel numbers
            af_dtype outArrayType = (af_dtype)af::dtype_traits<ValueType>::af_type;

            current = current.as(outArrayType);

            current.eval();

            typename ImageType::SizeType size;
            typename ImageType::IndexType start;
            typename ImageType::SpacingType spacing;
            typename ImageType::RegionType region;

            const bool noTranspose = !transposeIn;

            start.Fill(0); //Equivalent to assigning 0 to all 3 axes
            spacing.Fill(1); //Equivalent to assigning 1 to all 3 axes
            size[0] = noTranspose ? in.dims(1) : in.dims(0); // Size along X
            size[1] = noTranspose ? in.dims(0) : in.dims(1); // Size along Y

            region.SetSize(size);
            region.SetIndex(start);

            out->SetRegions(region);
            out->SetSpacing(spacing);
            out->Allocate();

            std::vector<ValueType> buffer(current.elements());

            current.host(reinterpret_cast<void*>(buffer.data()));

            // In ITK, color channels are interleaved format represented by itk::RGBPixel object
            ConvertPixelBuffer<ValueType, PixelType, itk::DefaultConvertPixelTraits<PixelType> >
                ::Convert(buffer.data(), inChannels,
                        reinterpret_cast<PixelType*>(out->GetPixelContainer()->GetBufferPointer()),
                        size[0]*size[1]);
        }
};
}

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkArrayFireImageBridge.hxx"
#endif

#endif//itkArrayFireImageBridge_h
