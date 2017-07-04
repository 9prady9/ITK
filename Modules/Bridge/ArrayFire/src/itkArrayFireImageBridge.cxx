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
#include "itkArrayFireImageBridge.h"
#include <af/internal.h>

namespace itk
{

af::dtype GetAfDtypeFromCVType(int type)
{
    switch(type) {
        case CV_8U:   return u8;
        case CV_8S:   return b8; //b8 is boolean internally represented by char
        case CV_16U: return u16;
        case CV_16S: return s16;
        case CV_32S: return s32;
        case CV_32F: return f32;
        case CV_64F: return f64;
    }
    return f32;
}

af::array Convert(const cv::Mat& input, bool transpose)
{
    cv::Size size = input.size();
    int channels  = input.channels();

    af::dtype dtype = GetAfDtypeFromCVType(input.depth());

    dim_t scanWidth = input.step;

    af::array interleaved(af::dim4(scanWidth, size.height), input.data);

    af::dim4 dims(size.width, size.height, channels); //non-interleaved layout dimensions

    af::array sliced = af::constant(0, dims, dtype);

    for (int c=0; c<channels; ++c)
    {
        af::seq ch(c, scanWidth-1, channels);

        sliced(af::span, af::span, c) = interleaved(ch, af::span);
    }

    return (transpose ? af::transpose(sliced) : sliced);
}

af::array CVMatToArray(const cv::Mat& input, bool transpose)
{
    if (input.empty())
        return af::array();

    return Convert(input, transpose);
}

af::array CVMatToArray(const std::vector<cv::Mat>& inputs, bool transpose)
{
    if (inputs.size() == 0)
        return af::array();

    af::array output;
    for (unsigned i=0; i<inputs.size(); ++i)
        output = af::join(2, output, Convert(inputs[i], transpose));

    return output;
}

int GetCVTypeFromAfDtype(af::dtype type, size_t channels)
{
    switch(type) {
        case u8 : return CV_8UC(channels);
        case b8 : return CV_8SC(channels); //b8 is internally represented by char in ArrayFire
        case u16: return CV_16UC(channels);
        case s16: return CV_16SC(channels);
        case s32: return CV_32SC(channels);
        case f32: return CV_32FC(channels);
        case f64: return CV_64FC(channels);
        default:  return CV_32FC(channels);
    }
}

cv::Mat ArrayToCVMat(const af::array& input, bool transpose)
{
    if (input.isempty())
        return cv::Mat();

    if (input.dims(2)>3)
        throw std::runtime_error("OpenCV Images doesn't support more than 3 channels");

    af::array sliced = (transpose ? af::transpose(input) : input);

    const af::dim4 dims = sliced.dims();

    cv::Mat output(dims[0], dims[1], GetCVTypeFromAfDtype(sliced.type(), dims[2]));

    af::array interleaved = (dims[2]>1 ? af::reorder(sliced, 2, 1, 0) : af::reorder(sliced, 1, 0));

    af::array packed      = af::moddims(interleaved, af::dim4(dims[0]*dims[2], dims[1]));

    packed.host((void*)output.data);

    return output;
}
}
