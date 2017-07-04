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
#ifndef itkArrayFireImageBridge_hxx
#define itkArrayFireImageBridge_hxx

#include "itkArrayFireImageBridge.h"
#include "itkNumericTraits.h"
#include <vector>

namespace itk
{
template<typename ImageType>
typename ImageType::Pointer
ArrayFireImageBridge::AfArrayToITKImage(const af::array& in, const bool& transposeIn)
{
  if (in.isempty())
    itkGenericExceptionMacro("Input is ITK_NULLPTR");

  typename ImageType::Pointer out = ImageType::New();

  af_dtype t = in.type();

  if (t==f64 || t==f32 || t==s16 || t==u16 ||t==u8)
    ITKConvertArray<ImageType>(in, transposeIn, out.GetPointer());
  else
    itkGenericExceptionMacro("Unsupported type for pixel component");

  return out;
}

template<typename TInputImageType>
af::array
ArrayFireImageBridge::ITKImageToAfArray(const TInputImageType* in)
{
  typedef TInputImageType                                        ImageType;
  typedef typename ImageType::PixelType                          InputPixelType;
  typedef typename itk::NumericTraits<InputPixelType>::ValueType ValueType;

  if (!in)
    itkGenericExceptionMacro("Input is ITK_NULLPTR");

  typename ImageType::RegionType  region = in->GetLargestPossibleRegion();
  typename ImageType::SizeType    size = region.GetSize();

  //TODO Add batch support later
  if (ImageType::ImageDimension > 2)
  {
    bool IsA2DImage = false;
    for (unsigned int dim = 2; (dim < ImageType::ImageDimension) && !IsA2DImage; dim++)
    {
      if (size[dim] != 1) {
        IsA2DImage= true;
      }
    }

    if (IsA2DImage) {
      itkGenericExceptionMacro("ArrayFire only supports 2D and 1D images");
    }
  }

  unsigned int channels = itk::NumericTraits<InputPixelType>::MeasurementVectorType::Dimension;

  if (channels != 1 && channels != 3 && channels != 4) {
    itkGenericExceptionMacro("ITKArrayFireBridge only supports grayscale and 3-channel data");
  }

  unsigned int w = static_cast< unsigned int >(size[0]);
  unsigned int h = static_cast< unsigned int >(size[1]);

  af::array out;

  const InputPixelType* inputPtr = const_cast<InputPixelType*>(in->GetBufferPointer());

  const af::dim4 dims(w, h, channels);

  af_array temp  = 0;
  af_dtype vtype = (af_dtype)af::dtype_traits<ValueType>::af_type;

  //FIXME Check if ITKImage is padded
  // If input is grayscale image
  if (channels==1) {
    af_err err = af_create_array(&temp, (void*)inputPtr, dims.ndims(), dims.get(), vtype);

    if (err!=AF_SUCCESS)
        itkGenericExceptionMacro("Creation of af::array from data failed");

    out = af::array(temp);
  } else {
    std::vector<ValueType> data = ArrayFireImageBridge::ITKRGBArrayToVectorArray(inputPtr, w, h);

    af_err err = af_create_array(&temp, (void*)data.data(), dims.ndims(), dims.get(), vtype);

    if (err!=AF_SUCCESS)
        itkGenericExceptionMacro("Creation of af::array from data failed");

    out = af::array(temp);
  }

  // Data loaded by af::loadImage into GPU memory is in coloumn major
  // format in comparision to the traditional style images are stored.
  // To avoid double transposes in some cases while converting this
  // af::array back to itk::Image, just return the loaded data's transpose.
  // In doing so, we can handle conversion of af::array to itk::Image
  // in a consistent fashion.
  return out.T(); //af::array::T() returns transposed af::array
}
}

#endif//itkArrayFireImageBridge_hxx
