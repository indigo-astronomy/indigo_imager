// Copyright (c) 2019 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _PIXELFORMAT_H
#define _PIXELFORMAT_H

#include <inttypes.h>
#define pack(a, b, c, d)\
        ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define PIX_FMT_INDEX   pack('I','N','D',' ')

#define PIX_FMT_PJPG    pack('P','J','P','G')

#define PIX_FMT_Y8      pack('Y','0','1',' ')
#define PIX_FMT_Y16     pack('Y','0','2',' ')
#define PIX_FMT_Y32     pack('Y','0','4',' ')
#define PIX_FMT_F32     pack('F','0','4',' ')
#define PIX_FMT_RGB24   pack('R','G','B','3')
#define PIX_FMT_RGB48   pack('R','G','B','4')
#define PIX_FMT_RGB96   pack('R','G','B','8')
#define PIX_FMT_RGBF    pack('R','G','B','F')
#define PIX_FMT_3RGB24  pack('R','G','3','3')
#define PIX_FMT_3RGB48  pack('R','G','3','4')
#define PIX_FMT_3RGB96  pack('R','G','3','8')
#define PIX_FMT_3RGBF   pack('R','G','3','F')

#define PIX_FMT_SGBRG8  pack('G','B','R','G')
#define PIX_FMT_SGRBG8  pack('G','R','B','G')
#define PIX_FMT_SRGGB8  pack('R','G','G','B')
#define PIX_FMT_SBGGR8  pack('B','G','G','R')

#define PIX_FMT_SBGGR12 pack('B', 'G', '1', '2') /* 12  BGBG.. GRGR.. */
#define PIX_FMT_SGBRG12 pack('G', 'B', '1', '2') /* 12  GBGB.. RGRG.. */
#define PIX_FMT_SGRBG12 pack('B', 'A', '1', '2') /* 12  GRGR.. BGBG.. */
#define PIX_FMT_SRGGB12 pack('R', 'G', '1', '2') /* 12  RGRG.. GBGB.. */

#define PIX_FMT_SBGGR16 pack('B', 'G', '1', '6') /* 16  BGBG.. GRGR.. */
#define PIX_FMT_SGBRG16 pack('G', 'B', '1', '6') /* 16  GBGB.. RGRG.. */
#define PIX_FMT_SGRBG16 pack('G', 'R', '1', '6') /* 16  GRGR.. BGBG.. */
#define PIX_FMT_SRGGB16 pack('R', 'G', '1', '6') /* 16  RGRG.. GBGB.. */

#define PIX_FMT_SBGGR32 pack('B', 'G', '3', '2') /* 16  BGBG.. GRGR.. */
#define PIX_FMT_SGBRG32 pack('G', 'B', '3', '2') /* 16  GBGB.. RGRG.. */
#define PIX_FMT_SGRBG32 pack('G', 'R', '3', '2') /* 16  GRGR.. BGBG.. */
#define PIX_FMT_SRGGB32 pack('R', 'G', '3', '2') /* 16  RGRG.. GBGB.. */

#endif /* _PIXELFORMAT_H */
